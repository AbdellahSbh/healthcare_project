#include "crow.h"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <regex>
#include <mutex>
#include <sstream>
#include <exception>
#include <iostream>
#include <fstream>
#include <algorithm> // std::find_if
#include <nlohmann/json.hpp>
using json = nlohmann::json;



sqlite3* initDatabase(const std::string& dbPath, const std::string& schemaPath) {
    sqlite3* db;

    // Open the SQLite database
    if (sqlite3_open(dbPath.c_str(), &db)) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db)));
    }

    // Load the schema file
    std::ifstream schemaFile(schemaPath);
    if (!schemaFile.is_open()) {
        sqlite3_close(db);
        throw std::runtime_error("Cannot open schema file: " + schemaPath);
    }

    std::stringstream buffer;
    buffer << schemaFile.rdbuf();
    std::string schema = buffer.str();
    schemaFile.close();

    // Execute the schema to create tables
    char* errMsg = nullptr;
    if (sqlite3_exec(db, schema.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        throw std::runtime_error("Failed to execute schema: " + error);
    }

    std::cout << "Database initialized successfully with schema." << std::endl;
    return db;
}




//Data Structures
struct Patient {
    int id;
    std::string name;
    std::string address;
    std::string medicalHistory;
    bool hasInsurance;
    std::string insuranceCompany;
};

struct Doctor {
    int id;
    std::string name;
    std::string specialty;
    std::string contactInfo;
};

struct Appointment {
    int patientId;
    int doctorId;
    std::string date;  // YYYY-MM-DD
    std::string time;  // HH:MM
};

struct MedicalRecord {
    int recordId;
    int patientId;
    std::string visitDate;  // YYYY-MM-DD
    std::string notes;
    std::string diagnosis;
};

struct Prescription {
    int prescriptionId;
    int patientId;
    int doctorId;
    std::string medication;
    std::string dosage;
    std::string instructions;
    std::string datePrescribed; // YYYY-MM-DD
};

struct Bill {
    int billId;
    int patientId;
    int appointmentId;
    double medicationFee;
    double consultationFee;
    double surgeryFee;
    double totalFee;
    bool isInsured;
    bool claimed;
    std::string insuranceCompany;
    std::string claimStatus; // e.g. "Pending", "Approved", "Denied"
};

//Global Data and Mutex
std::vector<Patient> patients;
std::vector<Doctor> doctors;
std::vector<Appointment> appointments;
std::vector<MedicalRecord> medicalRecords;
std::vector<Prescription> prescriptions;
std::vector<Bill> bills;
std::mutex dataMutex;

//File Operations
void saveToFile(const std::string& filename, const json& data) {
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << data.dump(4);
        file.close();
    }
}

void ensureFileExists(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::ofstream newFile(filename);
        newFile << "[]" << std::endl;
        newFile.close();
    }
}

void loadFromFile(const std::string& filename, json& data) {
    std::ifstream file(filename);
    if (file.is_open()) {
        file >> data;
        file.close();
    }
}

//Load / Save Functions 
void savePatientsToFile() {
    json arr = json::array();
    for (auto& p : patients) {
        arr.push_back({
            {"id", p.id},
            {"name", p.name},
            {"address", p.address},
            {"medicalHistory", p.medicalHistory},
            {"hasInsurance", p.hasInsurance},
            {"insuranceCompany", p.insuranceCompany}
            });
    }
    saveToFile("patients.json", arr);
}

void loadPatientsFromFile() {
    ensureFileExists("patients.json");
    json arr;
    loadFromFile("patients.json", arr);

    for (auto& item : arr) {
        if (!item.contains("id") || !item.contains("name") ||
            !item.contains("address") || !item.contains("medicalHistory") ||
            !item.contains("hasInsurance") || !item.contains("insuranceCompany")) {
            continue;
        }
        Patient p;
        p.id = item["id"].get<int>();
        p.name = item["name"].get<std::string>();
        p.address = item["address"].get<std::string>();
        p.medicalHistory = item["medicalHistory"].get<std::string>();
        p.hasInsurance = item["hasInsurance"].get<bool>();
        p.insuranceCompany = item["insuranceCompany"].get<std::string>();
        patients.push_back(p);
    }
}

void saveDoctorsToFile() {
    json arr = json::array();
    for (auto& d : doctors) {
        arr.push_back({
            {"id", d.id},
            {"name", d.name},
            {"specialty", d.specialty},
            {"contactInfo", d.contactInfo}
            });
    }
    saveToFile("doctors.json", arr);
}

void loadDoctorsFromFile() {
    ensureFileExists("doctors.json");
    json arr;
    loadFromFile("doctors.json", arr);

    for (auto& item : arr) {
        if (!item.contains("id") || !item.contains("name") ||
            !item.contains("specialty") || !item.contains("contactInfo")) {
            continue;
        }
        Doctor d;
        d.id = item["id"].get<int>();
        d.name = item["name"].get<std::string>();
        d.specialty = item["specialty"].get<std::string>();
        d.contactInfo = item["contactInfo"].get<std::string>();
        doctors.push_back(d);
    }
}

void saveAppointmentsToFile() {
    json arr = json::array();
    for (auto& a : appointments) {
        arr.push_back({
            {"patientId", a.patientId},
            {"doctorId", a.doctorId},
            {"date", a.date},
            {"time", a.time}
            });
    }
    saveToFile("appointments.json", arr);
}

void checkAndNotifyLowStock(const std::string& itemName, int quantity, sqlite3* db) {
    if (quantity < 10) {
        std::string message = "Low stock warning: " + itemName +
                              " has only " + std::to_string(quantity) + " items left.";

        // Insert into the Notifications table
        std::string query = "INSERT INTO Notifications (itemName, message) VALUES (?, ?)";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, itemName.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                std::cout << "Notification saved: " << message << std::endl;
            }
            sqlite3_finalize(stmt);
        }
    }
}


void loadAppointmentsFromFile() {
    ensureFileExists("appointments.json");
    json arr;
    loadFromFile("appointments.json", arr);

    for (auto& item : arr) {
        if (!item.contains("patientId") || !item.contains("doctorId") ||
            !item.contains("date") || !item.contains("time")) {
            continue;
        }
        Appointment a;
        a.patientId = item["patientId"].get<int>();
        a.doctorId = item["doctorId"].get<int>();
        a.date = item["date"].get<std::string>();
        a.time = item["time"].get<std::string>();
        appointments.push_back(a);
    }
}

void saveMedicalRecordsToFile() {
    json arr = json::array();
    for (auto& r : medicalRecords) {
        arr.push_back({
            {"recordId", r.recordId},
            {"patientId", r.patientId},
            {"visitDate", r.visitDate},
            {"notes", r.notes},
            {"diagnosis", r.diagnosis}
            });
    }
    saveToFile("medical_records.json", arr);
}

void loadMedicalRecordsFromFile() {
    ensureFileExists("medical_records.json");
    json arr;
    loadFromFile("medical_records.json", arr);

    for (auto& item : arr) {
        if (!item.contains("recordId") || !item.contains("patientId") ||
            !item.contains("visitDate") || !item.contains("notes") ||
            !item.contains("diagnosis")) {
            continue;
        }
        MedicalRecord r;
        r.recordId = item["recordId"].get<int>();
        r.patientId = item["patientId"].get<int>();
        r.visitDate = item["visitDate"].get<std::string>();
        r.notes = item["notes"].get<std::string>();
        r.diagnosis = item["diagnosis"].get<std::string>();
        medicalRecords.push_back(r);
    }
}

void savePrescriptionsToFile() {
    json arr = json::array();
    for (auto& p : prescriptions) {
        arr.push_back({
            {"prescriptionId", p.prescriptionId},
            {"patientId", p.patientId},
            {"doctorId", p.doctorId},
            {"medication", p.medication},
            {"dosage", p.dosage},
            {"instructions", p.instructions},
            {"datePrescribed", p.datePrescribed}
            });
    }
    saveToFile("prescriptions.json", arr);
}

void loadPrescriptionsFromFile() {
    ensureFileExists("prescriptions.json");
    json arr;
    loadFromFile("prescriptions.json", arr);

    for (auto& item : arr) {
        if (!item.contains("prescriptionId") || !item.contains("patientId") ||
            !item.contains("doctorId") || !item.contains("medication") ||
            !item.contains("dosage") || !item.contains("instructions") ||
            !item.contains("datePrescribed")) {
            continue;
        }
        Prescription p;
        p.prescriptionId = item["prescriptionId"].get<int>();
        p.patientId = item["patientId"].get<int>();
        p.doctorId = item["doctorId"].get<int>();
        p.medication = item["medication"].get<std::string>();
        p.dosage = item["dosage"].get<std::string>();
        p.instructions = item["instructions"].get<std::string>();
        p.datePrescribed = item["datePrescribed"].get<std::string>();
        prescriptions.push_back(p);
    }
}

void saveBillsToFile() {
    json arr = json::array();
    for (auto& b : bills) {
        arr.push_back({
            {"billId", b.billId},
            {"patientId", b.patientId},
            {"appointmentId", b.appointmentId},
            {"medicationFee", b.medicationFee},
            {"consultationFee", b.consultationFee},
            {"surgeryFee", b.surgeryFee},
            {"totalFee", b.totalFee},
            {"isInsured", b.isInsured},
            {"claimed", b.claimed},
            {"insuranceCompany", b.insuranceCompany},
            {"claimStatus", b.claimStatus}
            });
    }
    saveToFile("bills.json", arr);
}

void loadBillsFromFile() {
    ensureFileExists("bills.json");
    json arr;
    loadFromFile("bills.json", arr);

    for (auto& item : arr) {
        if (!item.contains("billId") || !item.contains("patientId") ||
            !item.contains("appointmentId") || !item.contains("medicationFee") ||
            !item.contains("consultationFee") || !item.contains("surgeryFee") ||
            !item.contains("totalFee") || !item.contains("isInsured") ||
            !item.contains("claimed") || !item.contains("insuranceCompany") ||
            !item.contains("claimStatus")) {
            continue;
        }
        Bill b;
        b.billId = item["billId"].get<int>();
        b.patientId = item["patientId"].get<int>();
        b.appointmentId = item["appointmentId"].get<int>();
        b.medicationFee = item["medicationFee"].get<double>();
        b.consultationFee = item["consultationFee"].get<double>();
        b.surgeryFee = item["surgeryFee"].get<double>();
        b.totalFee = item["totalFee"].get<double>();
        b.isInsured = item["isInsured"].get<bool>();
        b.claimed = item["claimed"].get<bool>();
        b.insuranceCompany = item["insuranceCompany"].get<std::string>();
        b.claimStatus = item["claimStatus"].get<std::string>();
        bills.push_back(b);
    }
}

// ------------------ Utility Functions ------------------
bool isValidAppointmentTime(const std::string& time) {
    std::regex timeRegex(R"(^([0-1][0-9]|2[0-3]):([0-5][0-9])$)");
    if (!std::regex_match(time, timeRegex)) {
        return false;
    }
    int hour, minute;
    char colon;
    std::istringstream(time) >> hour >> colon >> minute;
    // Only allow from 09:00 to 17:00
    if (hour < 9 || hour > 17 || (hour == 17 && minute > 0)) {
        return false;
    }
    // Only allow 10-minute increments
    if (minute % 10 != 0) {
        return false;
    }
    return true;
}

bool isValidDate(const std::string& date) {
    std::regex dateRegex(R"(^\d{4}-\d{2}-\d{2}$)");
    return std::regex_match(date, dateRegex);
}

// Check if a specific doctor already has an appointment for the given date/time
bool isAppointmentSlotTaken(int doctorId, const std::string& date, const std::string& time) {
    std::lock_guard<std::mutex> lock(dataMutex);
    for (auto& appt : appointments) {
        if (appt.doctorId == doctorId && appt.date == date && appt.time == time) {
            return true;
        }
    }
    return false;
}


int main() {
    crow::SimpleApp app;

    sqlite3* db = nullptr;
    try {
        db = initDatabase("healthcare.db", "database.sql");
    } catch (const std::exception& e) {
        std::cerr << "Error initializing database: " << e.what() << std::endl;
        return 1;
    }
    // Load existing data
    loadPatientsFromFile();
    loadDoctorsFromFile();
    loadAppointmentsFromFile();
    loadMedicalRecordsFromFile();
    loadPrescriptionsFromFile();
    loadBillsFromFile();

    // Home route
    CROW_ROUTE(app, "/")([]() {
        return "Welcome to the Healthcare System (English version, all GET methods).";
        });


    //  Register new patient
    // Example: /register?name=John&address=NY&medicalHistory=SomeHistory&insuranceCompany=XYZ
   CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;

    const char* name = qs.get("name");
    const char* address = qs.get("address");
    const char* medicalHistory = qs.get("medicalHistory");
    const char* insuranceCompany = qs.get("insuranceCompany");

    if (!name || !address || !medicalHistory) {
        return crow::response(400, "Missing required parameters: name, address, medicalHistory");
    }

    bool hasInsurance = (insuranceCompany != nullptr); // If insuranceCompany exists, set hasInsurance to true

    // Insert into SQLite
    std::string sql = "INSERT INTO Patients (name, address, medicalHistory, hasInsurance, insuranceCompany) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, address, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, medicalHistory, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, hasInsurance ? 1 : 0);
    sqlite3_bind_text(stmt, 5, insuranceCompany ? insuranceCompany : "", -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to execute statement");
    }

    int id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Patient registered successfully";
    resp["id"] = id;
    return crow::response(resp);
});


    // Book appointment 
    // Example:
    // /book_appointment?patientId=1&doctorId=1&date=2025-01-02&time=09:00
    // After booking, automatically add a Bill (with 0 fees) create or update a medicalRecord for the patient's appointment history
 CROW_ROUTE(app, "/book_appointment").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* patientIdStr = qs.get("patientId");
    const char* doctorIdStr = qs.get("doctorId");
    const char* date = qs.get("date");
    const char* time = qs.get("time");

    if (!patientIdStr || !doctorIdStr || !date || !time) {
        return crow::response(400, "Missing required parameters: patientId, doctorId, date, time");
    }

    int patientId = std::atoi(patientIdStr);
    int doctorId = std::atoi(doctorIdStr);

    // Insert appointment
    std::string insertAppointment = "INSERT INTO Appointments (patientId, doctorId, date, time) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertAppointment.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare appointment statement");
    }
    sqlite3_bind_int(stmt, 1, patientId);
    sqlite3_bind_int(stmt, 2, doctorId);
    sqlite3_bind_text(stmt, 3, date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, time, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to execute appointment statement");
    }
    int appointmentId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    // Insert bill
    std::string insertBill = "INSERT INTO Bills (patientId, appointmentId, isInsured) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(db, insertBill.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare bill statement");
    }
    sqlite3_bind_int(stmt, 1, patientId);
    sqlite3_bind_int(stmt, 2, appointmentId);
    sqlite3_bind_int(stmt, 3, 0);  // Example: no insurance for simplicity

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to execute bill statement");
    }
    int billId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Appointment and bill created successfully";
    resp["appointmentId"] = appointmentId;
    resp["billId"] = billId;
    return crow::response(resp);
});


    //  view all patients 
    // Show each patient's prescriptions in the response

    CROW_ROUTE(app, "/patients").methods(crow::HTTPMethod::GET)([&db]() {
    std::string query = "SELECT * FROM Patients";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    crow::json::wvalue resp;
    std::vector<crow::json::wvalue> patientsArray;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue patient;
        patient["id"] = sqlite3_column_int(stmt, 0);
        patient["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        patient["address"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        patient["medicalHistory"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        patient["hasInsurance"] = sqlite3_column_int(stmt, 4);
        patient["insuranceCompany"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

        patientsArray.push_back(patient);
    }
    sqlite3_finalize(stmt);

    resp["patients"] = std::move(patientsArray);
    return crow::response(resp);
});



    // view all appointments 

CROW_ROUTE(app, "/appointments").methods(crow::HTTPMethod::GET)([&db]() {
    std::string query = "SELECT * FROM Appointments";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    crow::json::wvalue resp;
    std::vector<crow::json::wvalue> appointmentsArray;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue appointment;
        appointment["id"] = sqlite3_column_int(stmt, 0);
        appointment["patientId"] = sqlite3_column_int(stmt, 1);
        appointment["doctorId"] = sqlite3_column_int(stmt, 2);
        appointment["date"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        appointment["time"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        appointmentsArray.push_back(std::move(appointment));
    }
    sqlite3_finalize(stmt);

    resp["appointments"] = std::move(appointmentsArray);
    return crow::response(resp);
});



    // Rregister a new doctor 
    // Example:
    // /register_doctor?name=DrSmith&specialty=Surgery&contactInfo=xxx
CROW_ROUTE(app, "/register_doctor").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* name = qs.get("name");
    const char* specialty = qs.get("specialty");
    const char* contactInfo = qs.get("contactInfo");

    if (!name || !specialty || !contactInfo) {
        return crow::response(400, "Missing required parameters: name, specialty, contactInfo");
    }

    std::string sql = "INSERT INTO Doctors (name, specialty, contactInfo) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, specialty, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, contactInfo, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to execute statement");
    }

    int id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Doctor registered successfully";
    resp["id"] = id;
    return crow::response(resp);
});



    // view doctors 

   CROW_ROUTE(app, "/doctors").methods(crow::HTTPMethod::GET)([&db]() {
    std::string query = "SELECT * FROM Doctors";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    crow::json::wvalue resp;
    std::vector<crow::json::wvalue> doctorsArray;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue doctor;
        doctor["id"] = sqlite3_column_int(stmt, 0);
        doctor["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        doctor["specialty"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        doctor["contactInfo"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        doctorsArray.push_back(std::move(doctor));
    }
    sqlite3_finalize(stmt);

    resp["doctors"] = std::move(doctorsArray);
    return crow::response(resp);
});


    // Add prescription 
    // Example:
    // /add_prescription?patientId=1&doctorId=1&medication=ABC&dosage=1tablet&instructions=AfterMeal&datePrescribed=2025-01-02
    CROW_ROUTE(app, "/add_prescription").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* patientIdStr = qs.get("patientId");
    const char* doctorIdStr = qs.get("doctorId");
    const char* medication = qs.get("medication");
    const char* dosage = qs.get("dosage");
    const char* instructions = qs.get("instructions");
    const char* datePrescribed = qs.get("datePrescribed");

    if (!patientIdStr || !doctorIdStr || !medication || !dosage || !instructions || !datePrescribed) {
        return crow::response(400, "Missing required parameters");
    }

    int patientId = std::atoi(patientIdStr);
    int doctorId = std::atoi(doctorIdStr);

    // Validate patient
    std::string checkPatient = "SELECT id FROM Patients WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, checkPatient.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare patient check statement");
    }
    sqlite3_bind_int(stmt, 1, patientId);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return crow::response(404, "Patient not found");
    }
    sqlite3_finalize(stmt);

    // Validate doctor
    std::string checkDoctor = "SELECT id FROM Doctors WHERE id = ?";
    if (sqlite3_prepare_v2(db, checkDoctor.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare doctor check statement");
    }
    sqlite3_bind_int(stmt, 1, doctorId);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return crow::response(404, "Doctor not found");
    }
    sqlite3_finalize(stmt);

    // Insert prescription
    std::string sql = "INSERT INTO Prescriptions (patientId, doctorId, medication, dosage, instructions, datePrescribed) VALUES (?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare prescription statement");
    }

    sqlite3_bind_int(stmt, 1, patientId);
    sqlite3_bind_int(stmt, 2, doctorId);
    sqlite3_bind_text(stmt, 3, medication, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, dosage, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, instructions, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, datePrescribed, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to execute prescription statement");
    }

    int prescriptionId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Prescription added successfully";
    resp["prescriptionId"] = prescriptionId;
    return crow::response(resp);
});

    //  View /bills (GET)
CROW_ROUTE(app, "/bills").methods(crow::HTTPMethod::GET)([&db]() {
    std::string sql = "SELECT * FROM Bills";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    crow::json::wvalue resp;
    std::vector<crow::json::wvalue> bills;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue bill;
        bill["id"] = sqlite3_column_int(stmt, 0);
        bill["patientId"] = sqlite3_column_int(stmt, 1);
        bill["appointmentId"] = sqlite3_column_int(stmt, 2);
        bill["medicationFee"] = sqlite3_column_double(stmt, 3);
        bill["consultationFee"] = sqlite3_column_double(stmt, 4);
        bill["surgeryFee"] = sqlite3_column_double(stmt, 5);
        bill["totalFee"] = sqlite3_column_double(stmt, 6);
        bill["isInsured"] = sqlite3_column_int(stmt, 7);
        bill["claimed"] = sqlite3_column_int(stmt, 8);
        bill["insuranceCompany"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        bill["claimStatus"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        bills.push_back(std::move(bill));
    }
    sqlite3_finalize(stmt);

    resp["bills"] = std::move(bills);
    return crow::response(resp);
});



    // Example:
    // /update_bill?billId=1&medicationFee=10.0&consultationFee=20.0&surgeryFee=0.0
 CROW_ROUTE(app, "/update_bill").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* billIdStr = qs.get("billId");
    const char* medicationFeeStr = qs.get("medicationFee");
    const char* consultationFeeStr = qs.get("consultationFee");
    const char* surgeryFeeStr = qs.get("surgeryFee");

    if (!billIdStr || !medicationFeeStr || !consultationFeeStr || !surgeryFeeStr) {
        return crow::response(400, "Missing required parameters");
    }

    int billId = std::atoi(billIdStr);
    double medicationFee = std::atof(medicationFeeStr);
    double consultationFee = std::atof(consultationFeeStr);
    double surgeryFee = std::atof(surgeryFeeStr);
    double totalFee = medicationFee + consultationFee + surgeryFee;

    std::string query = "UPDATE Bills SET medicationFee = ?, consultationFee = ?, surgeryFee = ?, totalFee = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }

    sqlite3_bind_double(stmt, 1, medicationFee);
    sqlite3_bind_double(stmt, 2, consultationFee);
    sqlite3_bind_double(stmt, 3, surgeryFee);
    sqlite3_bind_double(stmt, 4, totalFee);
    sqlite3_bind_int(stmt, 5, billId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to update bill");
    }
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Bill updated successfully";
    resp["billId"] = billId;
    resp["totalFee"] = totalFee;
    return crow::response(resp);
});


    // Example:
    // /ask_for_billing?billId=1
 CROW_ROUTE(app, "/ask_for_billing").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* billIdStr = qs.get("billId");

    if (!billIdStr) {
        return crow::response(400, "Missing required parameter: billId");
    }

    int billId = std::atoi(billIdStr);

    // Verify if the bill exists and is insured
    std::string query = "SELECT isInsured, claimed FROM Bills WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }
    sqlite3_bind_int(stmt, 1, billId);

    bool isInsured = false, alreadyClaimed = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        isInsured = sqlite3_column_int(stmt, 0) != 0;
        alreadyClaimed = sqlite3_column_int(stmt, 1) != 0;
    } else {
        sqlite3_finalize(stmt);
        return crow::response(404, "Bill not found");
    }
    sqlite3_finalize(stmt);

    if (!isInsured) {
        return crow::response(400, "This bill is not for an insured patient");
    }
    if (alreadyClaimed) {
        return crow::response(400, "This bill has already been claimed");
    }

    // Update bill to mark it as claimed
    query = "UPDATE Bills SET claimed = 1, claimStatus = 'Pending' WHERE id = ?";
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }
    sqlite3_bind_int(stmt, 1, billId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to update claim status");
    }
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Insurance claim submitted";
    resp["billId"] = billId;
    resp["claimStatus"] = "Pending";
    return crow::response(resp);
});


        // Approve Claim
// Example: /approve_insurance?billId=1
CROW_ROUTE(app, "/approve_insurance").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* billIdStr = qs.get("billId");

    if (!billIdStr) {
        return crow::response(400, "Missing required parameter: billId");
    }

    int billId = std::atoi(billIdStr);

    // Verify if the bill exists and has a pending claim
    std::string query = "SELECT claimStatus FROM Bills WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }
    sqlite3_bind_int(stmt, 1, billId);

    std::string claimStatus;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        claimStatus = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    } else {
        sqlite3_finalize(stmt);
        return crow::response(404, "Bill not found");
    }
    sqlite3_finalize(stmt);

    if (claimStatus != "Pending") {
        return crow::response(400, "Claim is not in a pending state");
    }

    // Update bill to mark it as approved
    query = "UPDATE Bills SET claimStatus = 'Approved' WHERE id = ?";
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }
    sqlite3_bind_int(stmt, 1, billId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return crow::response(500, "Failed to update claim status");
    }
    sqlite3_finalize(stmt);

    crow::json::wvalue resp;
    resp["message"] = "Claim approved successfully";
    resp["billId"] = billId;
    return crow::response(resp);
});


CROW_ROUTE(app, "/inventory").methods(crow::HTTPMethod::GET)([&db]() {
    std::string query = "SELECT * FROM Inventory";
    sqlite3_stmt* stmt;
    crow::json::wvalue resp;
    std::vector<crow::json::wvalue> items;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            crow::json::wvalue item;
            item["id"] = sqlite3_column_int(stmt, 0);
            item["itemName"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            item["quantity"] = sqlite3_column_int(stmt, 2);
            items.push_back(std::move(item));
        }
        sqlite3_finalize(stmt);
    } else {
        return crow::response(500, "Failed to fetch inventory");
    }

    resp["inventory"] = std::move(items);
    return crow::response(resp);
});

CROW_ROUTE(app, "/update_inventory_item").methods(crow::HTTPMethod::GET)([&db](const crow::request& req) {
    auto qs = req.url_params;
    const char* itemName = qs.get("itemName");
    const char* quantityStr = qs.get("quantity");

    if (!itemName || !quantityStr) {
        return crow::response(400, "Missing 'itemName' or 'quantity'");
    }

    int newQuantity = std::atoi(quantityStr);
    if (newQuantity < 0) {
        return crow::response(400, "Quantity cannot be negative");
    }

    bool isUpdated = false;
    sqlite3_stmt* stmt;

    // Check if the item exists
    std::string query = "SELECT quantity FROM Inventory WHERE itemName = ?";
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return crow::response(500, "Failed to prepare statement");
    }
    sqlite3_bind_text(stmt, 1, itemName, -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = true;
    }
    sqlite3_finalize(stmt);

    if (exists) {
        // Update the existing item
        query = "UPDATE Inventory SET quantity = ? WHERE itemName = ?";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "Failed to prepare statement");
        }
        sqlite3_bind_int(stmt, 1, newQuantity);
        sqlite3_bind_text(stmt, 2, itemName, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            isUpdated = true;
        }
        sqlite3_finalize(stmt);
    } else {
        // Add the new item
        query = "INSERT INTO Inventory (itemName, quantity) VALUES (?, ?)";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "Failed to prepare statement");
        }
        sqlite3_bind_text(stmt, 1, itemName, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, newQuantity);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            isUpdated = true;
        }
        sqlite3_finalize(stmt);
    }

    // Generate a low-stock notification if quantity < 10
    if (newQuantity < 10) {
        query = "INSERT INTO Notifications (itemName, message) VALUES (?, ?)";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "Failed to prepare notification statement");
        }
        std::string message = "Low stock warning: " + std::string(itemName) + " has only " + std::to_string(newQuantity) + " items left. Please add stock ";
        sqlite3_bind_text(stmt, 1, itemName, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return crow::response(500, "Failed to insert notification");
        }
        sqlite3_finalize(stmt);
    }

    crow::json::wvalue resp;
    resp["message"] = "Inventory updated successfully";
    resp["itemName"] = itemName;
    resp["quantity"] = newQuantity;

    return crow::response(resp);
});






    // Start server on port 8080
    app.port(8080).multithreaded().run();
    sqlite3_close(db);
    return 0;
}
