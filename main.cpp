#include "crow.h"
#include <vector>
#include <string>
#include <regex>
#include <mutex>
#include <sstream>
#include <fstream>
#include <algorithm> // std::find_if
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Data Structures 

// Represents a patient
struct Patient {
    int id;
    std::string name;
    std::string address;
    std::string medicalHistory;
    bool hasInsurance;
    std::string insuranceCompany;
};

// Represents a doctor
struct Doctor {
    int id;
    std::string name;
    std::string specialty;
    std::string contactInfo;
};

// Represents an appointment
struct Appointment {
    int patientId;
    int doctorId;
    std::string date;  // YYYY-MM-DD
    std::string time;  // HH:MM
};

// Represents a medical record
struct MedicalRecord {
    int recordId;
    int patientId;
    int doctorId;
    std::string visitDate;  // YYYY-MM-DD
    std::string notes;
    std::string diagnosis;
};

// Represents an inventory item (e.g., medication, bandage, syringe, etc.)
struct InventoryItem {
    int itemId;
    std::string itemName;
    int quantity;
};

// Represents an item used in the prescription
struct PrescriptionItem {
    std::string itemName;
    int usedAmount;
};

// Represents a prescription (which may contain multiple items used)
struct Prescription {
    int prescriptionId;
    int patientId;
    int doctorId;
    std::string datePrescribed; // YYYY-MM-DD
    std::string instructions;   // general instructions
    // multiple items used in this prescription
    std::vector<PrescriptionItem> itemsUsed;
};

// Represents a bill
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

// Global Data and Mutex 
std::vector<Patient> patients;
std::vector<Doctor> doctors;
std::vector<Appointment> appointments;
std::vector<MedicalRecord> medicalRecords;
std::vector<Prescription> prescriptions;  // Updated: each prescription can include multiple items
std::vector<Bill> bills;
std::vector<InventoryItem> inventoryItems;

std::mutex dataMutex;      // Protects patients, doctors, appointments, etc.
std::mutex inventoryMutex; // Protects inventoryItems

//  File I/O Helpers
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

// Load / Save Functions for each data type

// Patients
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

// Doctors
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

// Appointments
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

// MedicalRecords
void saveMedicalRecordsToFile() {
    json arr = json::array();
    for (auto& r : medicalRecords) {
        arr.push_back({
            {"recordId", r.recordId},
            {"patientId", r.patientId},
            {"doctorId", r.doctorId},
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
            !item.contains("doctorId") || !item.contains("visitDate") ||
            !item.contains("notes") || !item.contains("diagnosis")) {
            continue;
        }
        MedicalRecord r;
        r.recordId = item["recordId"].get<int>();
        r.patientId = item["patientId"].get<int>();
        r.doctorId = item["doctorId"].get<int>();
        r.visitDate = item["visitDate"].get<std::string>();
        r.notes = item["notes"].get<std::string>();
        r.diagnosis = item["diagnosis"].get<std::string>();
        medicalRecords.push_back(r);
    }
}

// Prescriptions
void savePrescriptionsToFile() {
    json arr = json::array();
    for (auto& p : prescriptions) {
        // Convert itemsUsed to JSON array
        json itemsArr = json::array();
        for (auto& used : p.itemsUsed) {
            itemsArr.push_back({
                {"itemName", used.itemName},
                {"usedAmount", used.usedAmount}
                });
        }

        arr.push_back({
            {"prescriptionId", p.prescriptionId},
            {"patientId", p.patientId},
            {"doctorId", p.doctorId},
            {"datePrescribed", p.datePrescribed},
            {"instructions", p.instructions},
            {"itemsUsed", itemsArr}
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
            !item.contains("doctorId") || !item.contains("datePrescribed") ||
            !item.contains("instructions") || !item.contains("itemsUsed")) {
            continue;
        }
        Prescription p;
        p.prescriptionId = item["prescriptionId"].get<int>();
        p.patientId = item["patientId"].get<int>();
        p.doctorId = item["doctorId"].get<int>();
        p.datePrescribed = item["datePrescribed"].get<std::string>();
        p.instructions = item["instructions"].get<std::string>();

        // Parse itemsUsed
        auto itemsArr = item["itemsUsed"];
        for (auto& usedItem : itemsArr) {
            if (!usedItem.contains("itemName") || !usedItem.contains("usedAmount")) {
                continue;
            }
            PrescriptionItem pi;
            pi.itemName = usedItem["itemName"].get<std::string>();
            pi.usedAmount = usedItem["usedAmount"].get<int>();
            p.itemsUsed.push_back(pi);
        }
        prescriptions.push_back(p);
    }
}

// Bills
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

// Inventory
void saveInventoryToFile() {
    json arr = json::array();
    {
        std::lock_guard<std::mutex> lock(inventoryMutex);
        for (auto& item : inventoryItems) {
            arr.push_back({
                {"itemId", item.itemId},
                {"itemName", item.itemName},
                {"quantity", item.quantity}
                });
        }
    }
    saveToFile("inventory.json", arr);
}

void loadInventoryFromFile() {
    ensureFileExists("inventory.json");
    json arr;
    loadFromFile("inventory.json", arr);

    {
        std::lock_guard<std::mutex> lock(inventoryMutex);
        inventoryItems.clear();
        for (auto& jitem : arr) {
            if (!jitem.contains("itemId") || !jitem.contains("itemName") ||
                !jitem.contains("quantity")) {
                continue;
            }
            InventoryItem item;
            item.itemId = jitem["itemId"].get<int>();
            item.itemName = jitem["itemName"].get<std::string>();
            item.quantity = jitem["quantity"].get<int>();
            inventoryItems.push_back(item);
        }
    }
}

// Utility Functions 
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

// Check and notify if stock is below a threshold (e.g. < 100)
void checkAndNotifyLowStock(const InventoryItem& item) {
    if (item.quantity < 100) {
        // This is just a log example. In real use case, connect to email or other notifications
        CROW_LOG_INFO << "[Warning] Item '" << item.itemName
            << "' is low in stock! Current quantity: " << item.quantity
            << ". Please restock soon!";
    }
}

// Main
int main() {
    crow::SimpleApp app;

    // Load existing data
    loadPatientsFromFile();
    loadDoctorsFromFile();
    loadAppointmentsFromFile();
    loadMedicalRecordsFromFile();
    loadPrescriptionsFromFile();
    loadBillsFromFile();
    loadInventoryFromFile();

    // Home route
    CROW_ROUTE(app, "/")([]() {
        return "Welcome to the Healthcare System (English version, all GET methods).";
        });

    // Register new patient (GET)
    // Example: /register?name=John&address=NY&medicalHistory=History&insuranceCompany=XYZ
    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            auto qs = req.url_params;
            const char* name = qs.get("name");
            const char* address = qs.get("address");
            const char* medicalHistory = qs.get("medicalHistory");
            const char* insuranceCompany = qs.get("insuranceCompany"); // optional

            if (!name || !address || !medicalHistory) {
                return crow::response(400, "Missing required parameters: name, address, medicalHistory");
            }

            bool hasInsurance = false;
            std::string insCompany;
            if (insuranceCompany != nullptr && std::string(insuranceCompany).size() > 0) {
                hasInsurance = true;
                insCompany = insuranceCompany;
            }

            Patient newPatient;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                newPatient.id = (int)patients.size() + 1;
            }
            newPatient.name = name;
            newPatient.address = address;
            newPatient.medicalHistory = medicalHistory;
            newPatient.hasInsurance = hasInsurance;
            newPatient.insuranceCompany = insCompany;

            {
                std::lock_guard<std::mutex> lock(dataMutex);
                patients.push_back(newPatient);
            }
            savePatientsToFile();

            crow::json::wvalue resp;
            resp["message"] = "Patient registered successfully";
            resp["patientId"] = newPatient.id;
            resp["hasInsurance"] = newPatient.hasInsurance;
            resp["insuranceCompany"] = newPatient.insuranceCompany;
            return crow::response(resp);
        });

    // Book appointment (GET)
    // Example: /book_appointment?patientId=1&doctorId=1&date=2025-01-02&time=09:00
    CROW_ROUTE(app, "/book_appointment").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
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

            // Validate patient
            auto patientIt = std::find_if(patients.begin(), patients.end(), [&](const Patient& p) {
                return p.id == patientId;
                });
            if (patientIt == patients.end()) {
                return crow::response(404, "Patient not found");
            }

            // Validate doctor
            auto doctorIt = std::find_if(doctors.begin(), doctors.end(), [&](const Doctor& d) {
                return d.id == doctorId;
                });
            if (doctorIt == doctors.end()) {
                return crow::response(404, "Doctor not found");
            }

            // Validate date/time
            std::string dateStr(date);
            std::string timeStr(time);
            if (!isValidDate(dateStr)) {
                return crow::response(400, "Invalid date format (YYYY-MM-DD)");
            }
            if (!isValidAppointmentTime(timeStr)) {
                return crow::response(400, "Invalid time (09:00 ~ 17:00, 10-min increments)");
            }

            // Check if slot is taken
            if (isAppointmentSlotTaken(doctorId, dateStr, timeStr)) {
                return crow::response(400, "This slot is already taken for this doctor");
            }

            // Create appointment
            Appointment newAppt = { patientId, doctorId, dateStr, timeStr };
            int appointmentIndex = 0;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                appointments.push_back(newAppt);
                appointmentIndex = (int)appointments.size(); // indexing
            }
            saveAppointmentsToFile();

            // Create a Bill with 0 fees
            Bill newBill;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                newBill.billId = (int)bills.size() + 1;
                newBill.patientId = patientId;
                newBill.appointmentId = appointmentIndex;
                newBill.medicationFee = 0.0;
                newBill.consultationFee = 0.0;
                newBill.surgeryFee = 0.0;
                newBill.totalFee = 0.0;
                newBill.isInsured = patientIt->hasInsurance;
                newBill.claimed = false;
                newBill.insuranceCompany = patientIt->insuranceCompany;
                newBill.claimStatus = "Not Submitted";
                bills.push_back(newBill);
            }
            saveBillsToFile();

            crow::json::wvalue resp;
            resp["message"] = "Appointment booked successfully";
            resp["appointment"] = {
                {"patientId", patientId},
                {"doctorId",  doctorId},
                {"date", dateStr},
                {"time", timeStr}
            };
            resp["bill"] = {
                {"billId", newBill.billId},
                {"isInsured", newBill.isInsured},
                {"insuranceCompany", newBill.insuranceCompany},
                {"status", newBill.claimStatus}
            };
            return crow::response(resp);
        });

    // View details of a single patient by ID (GET)
    // example: /patients/1
    CROW_ROUTE(app, "/patients/<int>").methods(crow::HTTPMethod::GET)(
        [](int id) {
            auto patientIt = std::find_if(patients.begin(), patients.end(), [&](const Patient& p) {
                return p.id == id;
                });

            if (patientIt == patients.end()) {
                return crow::response(404, "Patient not found");
            }

            crow::json::wvalue p;
            p["id"] = patientIt->id;
            p["name"] = patientIt->name;
            p["address"] = patientIt->address;
            p["medicalHistory"] = patientIt->medicalHistory;
            p["hasInsurance"] = patientIt->hasInsurance;
            p["insuranceCompany"] = patientIt->insuranceCompany;

            // Add this patient's prescriptions
            std::vector<crow::json::wvalue> prescArr;
            for (auto& pr : prescriptions) {
                if (pr.patientId == id) {
                    crow::json::wvalue item;
                    item["prescriptionId"] = pr.prescriptionId;
                    item["doctorId"] = pr.doctorId;
                    item["datePrescribed"] = pr.datePrescribed;
                    item["instructions"] = pr.instructions;

                    // Convert itemsUsed
                    std::vector<crow::json::wvalue> itemsUsedJson;
                    for (auto& used : pr.itemsUsed) {
                        crow::json::wvalue u;
                        u["itemName"] = used.itemName;
                        u["usedAmount"] = used.usedAmount;
                        itemsUsedJson.push_back(std::move(u));
                    }
                    item["itemsUsed"] = std::move(itemsUsedJson);

                    prescArr.push_back(std::move(item));
                }
            }
            p["prescriptions"] = std::move(prescArr);

            crow::json::wvalue resp;
            resp["patient"] = std::move(p);
            return crow::response(resp);
        });

    // View all patients (GET)
    CROW_ROUTE(app, "/patients").methods(crow::HTTPMethod::GET)(
        []() {
            crow::json::wvalue resp;
            std::vector<crow::json::wvalue> arr;
            for (auto& pat : patients) {
                crow::json::wvalue p;
                p["id"] = pat.id;
                p["name"] = pat.name;
                p["address"] = pat.address;
                p["medicalHistory"] = pat.medicalHistory;
                p["hasInsurance"] = pat.hasInsurance;
                p["insuranceCompany"] = pat.insuranceCompany;

                // patient's prescriptions
                std::vector<crow::json::wvalue> prescArr;
                for (auto& pr : prescriptions) {
                    if (pr.patientId == pat.id) {
                        crow::json::wvalue item;
                        item["prescriptionId"] = pr.prescriptionId;
                        item["doctorId"] = pr.doctorId;
                        item["datePrescribed"] = pr.datePrescribed;
                        item["instructions"] = pr.instructions;

                        // itemsUsed
                        std::vector<crow::json::wvalue> itemsUsedJson;
                        for (auto& used : pr.itemsUsed) {
                            crow::json::wvalue u;
                            u["itemName"] = used.itemName;
                            u["usedAmount"] = used.usedAmount;
                            itemsUsedJson.push_back(std::move(u));
                        }
                        item["itemsUsed"] = std::move(itemsUsedJson);

                        prescArr.push_back(std::move(item));
                    }
                }
                p["prescriptions"] = std::move(prescArr);

                arr.push_back(std::move(p));
            }
            resp["patients"] = std::move(arr);
            return crow::response(resp);
        });

    // View appointments (GET)
    // optional param: ?patientId=<id> to filter
    CROW_ROUTE(app, "/appointments").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            auto patient_id_param = req.url_params.get("patientId");
            crow::json::wvalue resp;
            std::vector<crow::json::wvalue> arr;

            for (auto& a : appointments) {
                if (patient_id_param && std::to_string(a.patientId) != patient_id_param) {
                    continue;
                }
                crow::json::wvalue item;
                item["patientId"] = a.patientId;
                item["doctorId"] = a.doctorId;
                item["date"] = a.date;
                item["time"] = a.time;
                arr.push_back(std::move(item));
            }

            resp["appointments"] = std::move(arr);
            return crow::response(resp);
        });

    // Register a new doctor (GET)
    // Example: /register_doctor?name=DrSmith&specialty=Surgery&contactInfo=123456
    CROW_ROUTE(app, "/register_doctor").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            auto qs = req.url_params;
            const char* name = qs.get("name");
            const char* specialty = qs.get("specialty");
            const char* contactInfo = qs.get("contactInfo");

            if (!name || !specialty || !contactInfo) {
                return crow::response(400, "Missing required parameters: name, specialty, contactInfo");
            }

            Doctor d;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                d.id = (int)doctors.size() + 1;
            }
            d.name = name;
            d.specialty = specialty;
            d.contactInfo = contactInfo;

            {
                std::lock_guard<std::mutex> lock(dataMutex);
                doctors.push_back(d);
            }
            saveDoctorsToFile();

            crow::json::wvalue resp;
            resp["message"] = "Doctor registered successfully";
            resp["doctorId"] = d.id;
            return crow::response(resp);
        });

    // Add medical record (GET)
    // Example: /add_medical_record?patientId=1&doctorId=1&diagnosis=Flu&notes=Test
    CROW_ROUTE(app, "/add_medical_record").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            auto qs = req.url_params;
            const char* patientIdStr = qs.get("patientId");
            const char* doctorIdStr = qs.get("doctorId");
            const char* diagnosis = qs.get("diagnosis");
            const char* notes = qs.get("notes");

            if (!patientIdStr || !doctorIdStr || !diagnosis || !notes) {
                return crow::response(400, "Missing required parameters: patientId, doctorId, diagnosis, notes");
            }

            int patientId = std::atoi(patientIdStr);
            int doctorId = std::atoi(doctorIdStr);

            // Validate patient
            auto patIt = std::find_if(patients.begin(), patients.end(), [&](const Patient& p) {
                return p.id == patientId;
                });
            if (patIt == patients.end()) {
                return crow::response(404, "Patient not found");
            }

            // Validate doctor
            auto docIt = std::find_if(doctors.begin(), doctors.end(), [&](const Doctor& d) {
                return d.id == doctorId;
                });
            if (docIt == doctors.end()) {
                return crow::response(404, "Doctor not found");
            }

            // Find the most recent appointment for (patientId, doctorId)
            std::string chosenDate;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                for (auto it = appointments.rbegin(); it != appointments.rend(); ++it) {
                    if (it->patientId == patientId && it->doctorId == doctorId) {
                        chosenDate = it->date;
                        break;
                    }
                }
            }
            if (chosenDate.empty()) {
                return crow::response(400, "No appointment found for this patient/doctor combination");
            }

            // Create the MedicalRecord
            MedicalRecord newRecord;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                newRecord.recordId = (int)medicalRecords.size() + 1;
            }
            newRecord.patientId = patientId;
            newRecord.doctorId = doctorId;
            newRecord.visitDate = chosenDate;
            newRecord.notes = notes;
            newRecord.diagnosis = diagnosis;

            {
                std::lock_guard<std::mutex> lock(dataMutex);
                medicalRecords.push_back(newRecord);
            }
            saveMedicalRecordsToFile();

            crow::json::wvalue resp;
            resp["message"] = "Medical record added successfully";
            resp["recordId"] = newRecord.recordId;
            resp["doctorId"] = newRecord.doctorId;
            resp["patientId"] = newRecord.patientId;
            resp["visitDate"] = newRecord.visitDate;
            return crow::response(resp);
        });

    // View all doctors (GET)
    CROW_ROUTE(app, "/doctors").methods(crow::HTTPMethod::GET)(
        []() {
            crow::json::wvalue resp;
            std::vector<crow::json::wvalue> arr;
            for (auto& d : doctors) {
                crow::json::wvalue item;
                item["id"] = d.id;
                item["name"] = d.name;
                item["specialty"] = d.specialty;
                item["contactInfo"] = d.contactInfo;
                arr.push_back(std::move(item));
            }
            resp["doctors"] = std::move(arr);
            return crow::response(resp);
        });

    // Add Prescription (multiple items) (GET)
    // Example:
    // /add_prescription?patientId=1&doctorId=1&datePrescribed=2025-01-02&instructions=TakeAfterMeal&itemsUsed=Bandage,10;Syringe,5;MedicationX,2
    CROW_ROUTE(app, "/add_prescription").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            try {
                auto qs = req.url_params;
                const char* patientIdStr = qs.get("patientId");
                const char* doctorIdStr = qs.get("doctorId");
                const char* datePrescribed = qs.get("datePrescribed");
                const char* instructions = qs.get("instructions");
                const char* itemsUsedStr = qs.get("itemsUsed");

                if (!patientIdStr || !doctorIdStr || !datePrescribed || !instructions || !itemsUsedStr) {
                    return crow::response(400, "Missing required parameters: "
                        "patientId, doctorId, datePrescribed, "
                        "instructions, itemsUsed");
                }

                int patientId = std::atoi(patientIdStr);
                int doctorId = std::atoi(doctorIdStr);
                std::string dateStr(datePrescribed);

                // Parse itemsUsed string
                std::vector<PrescriptionItem> parsedItems;
                {
                    std::string raw(itemsUsedStr);
                    std::stringstream ss(raw);
                    std::string segment;
                    while (std::getline(ss, segment, ';')) {
                        if (segment.empty()) continue;
                        auto commaPos = segment.find(',');
                        if (commaPos == std::string::npos) {
                            return crow::response(400, "Invalid format in itemsUsed");
                        }
                        std::string itemName = segment.substr(0, commaPos);
                        std::string amountStr = segment.substr(commaPos + 1);

                        if (itemName.empty() || amountStr.empty()) {
                            return crow::response(400, "Invalid segment in itemsUsed");
                        }
                        int usedAmount = std::atoi(amountStr.c_str());
                        if (usedAmount <= 0) {
                            return crow::response(400, "Invalid usedAmount in itemsUsed");
                        }
                        parsedItems.push_back({ itemName, usedAmount });
                    }
                    if (parsedItems.empty()) {
                        return crow::response(400, "No valid items parsed from itemsUsed");
                    }
                }

                // Lock both dataMutex and inventoryMutex for atomic operations
                {
                    std::scoped_lock<std::mutex, std::mutex> lock(dataMutex, inventoryMutex);

                    // Validate patient and doctor
                    auto pIt = std::find_if(patients.begin(), patients.end(), [&](const Patient& p) {
                        return p.id == patientId;
                        });
                    if (pIt == patients.end()) {
                        return crow::response(404, "Patient not found");
                    }

                    auto dIt = std::find_if(doctors.begin(), doctors.end(), [&](const Doctor& d) {
                        return d.id == doctorId;
                        });
                    if (dIt == doctors.end()) {
                        return crow::response(404, "Doctor not found");
                    }

                    // Validate date format
                    if (!isValidDate(dateStr)) {
                        return crow::response(400, "Invalid date format (YYYY-MM-DD)");
                    }

                    // Check and deduct inventory
                    for (auto& pi : parsedItems) {
                        auto invIt = std::find_if(inventoryItems.begin(), inventoryItems.end(),
                            [&](const InventoryItem& inv) {
                                return inv.itemName == pi.itemName;
                            });
                        if (invIt == inventoryItems.end()) {
                            return crow::response(400, "Item " + pi.itemName + " not found in inventory");
                        }
                        if (invIt->quantity < pi.usedAmount) {
                            return crow::response(400, "Insufficient inventory for item " + pi.itemName);
                        }
                        // Deduct inventory
                        invIt->quantity -= pi.usedAmount;
                        if (invIt->quantity < 0) {
                            invIt->quantity = 0; // safety net
                        }
                        checkAndNotifyLowStock(*invIt);
                    }

                    // 2.3 Create and save prescription
                    Prescription newPres;
                    newPres.prescriptionId = static_cast<int>(prescriptions.size()) + 1;
                    newPres.patientId = patientId;
                    newPres.doctorId = doctorId;
                    newPres.datePrescribed = dateStr;
                    newPres.instructions = instructions;
                    newPres.itemsUsed = parsedItems;
                    prescriptions.push_back(newPres);


                    {
                        // Save prescriptions to file
                        json pArr = json::array();
                        for (auto& pr : prescriptions) {
                            json itemsArr = json::array();
                            for (auto& used : pr.itemsUsed) {
                                itemsArr.push_back({
                                    {"itemName", used.itemName},
                                    {"usedAmount", used.usedAmount}
                                    });
                            }
                            pArr.push_back({
                                {"prescriptionId", pr.prescriptionId},
                                {"patientId", pr.patientId},
                                {"doctorId", pr.doctorId},
                                {"datePrescribed", pr.datePrescribed},
                                {"instructions", pr.instructions},
                                {"itemsUsed", itemsArr}
                                });
                        }
                        saveToFile("prescriptions.json", pArr);

                        json invArr = json::array();
                        for (auto& invItem : inventoryItems) {
                            invArr.push_back({
                                {"itemId", invItem.itemId},
                                {"itemName", invItem.itemName},
                                {"quantity", invItem.quantity}
                                });
                        }
                        saveToFile("inventory.json", invArr);
                    }
                } // release dataMutex  inventoryMutex

                // http respond
                crow::json::wvalue resp;
                resp["message"] = "Prescription added successfully";
               
                resp["prescriptionId"] = static_cast<int>(prescriptions.size()); // 或 newPres.prescriptionId

                std::vector<crow::json::wvalue> usedArr;
                for (auto& x : parsedItems) {
                    crow::json::wvalue tmp;
                    tmp["itemName"] = x.itemName;
                    tmp["usedAmount"] = x.usedAmount;
                    usedArr.push_back(std::move(tmp));
                }
                resp["itemsUsed"] = std::move(usedArr);

                return crow::response(resp);

            }
            catch (const std::exception& e) {
                CROW_LOG_ERROR << "[add_prescription] Exception: " << e.what();
                return crow::response(500, std::string("Internal Server Error: ") + e.what());
            }
            catch (...) {
                CROW_LOG_ERROR << "[add_prescription] Unknown exception occurred.";
                return crow::response(500, "Unknown exception in add_prescription");
            }
        });

    // View all bills (GET)
    CROW_ROUTE(app, "/bills").methods(crow::HTTPMethod::GET)(
        []() {
            crow::json::wvalue resp;
            std::vector<crow::json::wvalue> arr;
            for (auto& b : bills) {
                crow::json::wvalue item;
                item["billId"] = b.billId;
                item["patientId"] = b.patientId;
                item["appointmentId"] = b.appointmentId;
                item["medicationFee"] = b.medicationFee;
                item["consultationFee"] = b.consultationFee;
                item["surgeryFee"] = b.surgeryFee;
                item["totalFee"] = b.totalFee;
                item["isInsured"] = b.isInsured;
                item["claimed"] = b.claimed;
                item["insuranceCompany"] = b.insuranceCompany;
                item["claimStatus"] = b.claimStatus;
                arr.push_back(std::move(item));
            }
            resp["bills"] = std::move(arr);
            return crow::response(resp);
        });

    // Update a bill (GET)
    // Example: /update_bill?billId=1&medicationFee=10.0&consultationFee=20.0&surgeryFee=0.0
    CROW_ROUTE(app, "/update_bill").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            auto qs = req.url_params;
            const char* billIdStr = qs.get("billId");
            const char* medicationFeeStr = qs.get("medicationFee");
            const char* consultationFeeStr = qs.get("consultationFee");
            const char* surgeryFeeStr = qs.get("surgeryFee");

            if (!billIdStr) {
                return crow::response(400, "Missing required parameter: billId");
            }
            int billId = std::atoi(billIdStr);

            std::lock_guard<std::mutex> lock(dataMutex);
            auto it = std::find_if(bills.begin(), bills.end(), [&](const Bill& b) {
                return b.billId == billId;
                });
            if (it == bills.end()) {
                return crow::response(404, "Bill not found");
            }

            if (medicationFeeStr) {
                it->medicationFee = std::atof(medicationFeeStr);
            }
            if (consultationFeeStr) {
                it->consultationFee = std::atof(consultationFeeStr);
            }
            if (surgeryFeeStr) {
                it->surgeryFee = std::atof(surgeryFeeStr);
            }

            it->totalFee = it->medicationFee + it->consultationFee + it->surgeryFee;
            saveBillsToFile();

            crow::json::wvalue resp;
            resp["message"] = "Bill updated successfully";
            resp["billId"] = it->billId;
            resp["totalFee"] = it->totalFee;
            return crow::response(resp);
        });

    // View medical records for a patient
    // Example: /medical_record/1
    CROW_ROUTE(app, "/medical_record/<int>").methods(crow::HTTPMethod::GET)(
        [](int patientId) {
            crow::json::wvalue response;
            std::vector<crow::json::wvalue> recordList;

            for (const auto& record : medicalRecords) {
                if (record.patientId == patientId) {
                    crow::json::wvalue recordInfo;
                    recordInfo["recordId"] = record.recordId;
                    recordInfo["patientId"] = record.patientId;
                    recordInfo["doctorId"] = record.doctorId;
                    recordInfo["visitDate"] = record.visitDate;
                    recordInfo["notes"] = record.notes;
                    recordInfo["diagnosis"] = record.diagnosis;
                    recordList.push_back(std::move(recordInfo));
                }
            }

            if (recordList.empty()) {
                return crow::response(404, "No medical records found for this patient");
            }

            response["medicalRecords"] = std::move(recordList);
            return crow::response(response);
        });

    // Submit insurance claim (GET)
    // Example: /submit_claim?billId=1
    CROW_ROUTE(app, "/submit_claim").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
            auto qs = req.url_params;
            const char* billIdStr = qs.get("billId");
            if (!billIdStr) {
                return crow::response(400, "Missing required parameter: billId");
            }
            int billId = std::atoi(billIdStr);

            std::lock_guard<std::mutex> lock(dataMutex);
            auto it = std::find_if(bills.begin(), bills.end(), [&](const Bill& b) {
                return b.billId == billId;
                });
            if (it == bills.end()) {
                return crow::response(404, "Bill not found");
            }

            if (!it->isInsured) {
                return crow::response(400, "This bill is not for an insured patient");
            }
            if (it->claimed) {
                return crow::response(400, "This bill has already been claimed");
            }

            it->claimed = true;
            it->claimStatus = "Pending";
            saveBillsToFile();

            crow::json::wvalue resp;
            resp["message"] = "Insurance claim submitted";
            resp["billId"] = it->billId;
            resp["claimStatus"] = it->claimStatus;
            return crow::response(resp);
        });

    // View the entire inventory (GET)
    // Example: /inventory
    CROW_ROUTE(app, "/inventory").methods(crow::HTTPMethod::GET)(
        []() {
            crow::json::wvalue resp;
            std::vector<crow::json::wvalue> arr;

            {
                std::lock_guard<std::mutex> lock(inventoryMutex);
                for (auto& item : inventoryItems) {
                    crow::json::wvalue temp;
                    temp["itemId"] = item.itemId;
                    temp["itemName"] = item.itemName;
                    temp["quantity"] = item.quantity;
                    arr.push_back(std::move(temp));
                }
            }

            resp["inventory"] = std::move(arr);
            return crow::response(resp);
        });

    // Manually update or create an inventory item (GET)
    // Example: /update_inventory_item?itemName=Bandage&quantity=500
    CROW_ROUTE(app, "/update_inventory_item").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req) {
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

            {
                std::lock_guard<std::mutex> lock(inventoryMutex);
                // judge if the item exists
                auto it = std::find_if(inventoryItems.begin(), inventoryItems.end(),
                    [&](const InventoryItem& i) {
                        return i.itemName == itemName;
                    });

                if (it == inventoryItems.end()) {
                    // add item
                    InventoryItem newItem;
                    newItem.itemId = static_cast<int>(inventoryItems.size()) + 1;
                    newItem.itemName = itemName;
                    newItem.quantity = newQuantity;
                    inventoryItems.push_back(newItem);

   
                    checkAndNotifyLowStock(newItem);
                }
                else {
                    it->quantity = newQuantity;
                    checkAndNotifyLowStock(*it);
                }
                isUpdated = true;
            } 

            if (isUpdated) {
                saveInventoryToFile();
            }

            crow::json::wvalue resp;
            resp["message"] = "Inventory updated successfully";
            resp["itemName"] = itemName;
            resp["quantity"] = newQuantity;
            return crow::response(resp);
        });
    // Start server on port 8080
    app.port(8080).multithreaded().run();
    return 0;
}
