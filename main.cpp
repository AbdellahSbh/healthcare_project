#include "crow.h"
#include <vector>
#include <string>
#include <regex>
#include <mutex>
#include <sstream>
#include <fstream>           
#include <nlohmann/json.hpp> 
using json = nlohmann::json;


struct Patient {
    int id;
    std::string name;
    std::string address;
    std::string medicalHistory;
    int doctorId;
    bool hasInsurance;
    std::string insuranceProvider;
};


struct Doctor {
    int id;
    std::string name;
    std::string specialty;
    std::string contactInfo;
};

struct Bill {
    int billId;
    int appointmentId;
    int patientId;
    std::vector<std::pair<std::string, double>> services;
    double totalCost;
};

std::vector<Bill> bills;


struct Appointment {
    int id;
    int patientId; 
    std::string date;
    std::string time;
};
struct MedicalRecord {
    int recordId;
    int patientId;
    std::string visitDate;
    std::string prescription;
    std::string diagnosis;
};

std::vector<MedicalRecord> medicalRecords;
std::vector<Patient> patients;
std::vector<Appointment> appointments;
std::mutex dataMutex; 
std::vector<Doctor> doctors;


void saveToFile(const std::string& filename, const json& data) {
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << data.dump(4); 
        file.close();
    }
}

void savePatientsToFile() {
    json patientList = json::array();
    for (const auto& patient : patients) {
        patientList.push_back({
            {"id", patient.id},
            {"name", patient.name},
            {"address", patient.address},
            {"medicalHistory", patient.medicalHistory},
            {"doctorId", patient.doctorId}
        });
    }
    saveToFile("patients.json", patientList);
}

void saveBillsToFile() {
    json billList = json::array();
    for (const auto& bill : bills) {
        json servicesJson = json::array();
        for (const auto& service : bill.services) {
            servicesJson.push_back({{"name", service.first}, {"cost", service.second}});
        }
        billList.push_back({
            {"billId", bill.billId},
            {"appointmentId", bill.appointmentId},
            {"patientId", bill.patientId},
            {"services", servicesJson},
            {"totalCost", bill.totalCost}
        });
    }
    saveToFile("bills.json", billList);
}


void saveAppointmentsToFile() {
    json appointmentList = json::array();
    for (const auto& appointment : appointments) {
        appointmentList.push_back({
            {"id", appointment.id},
            {"patientId", appointment.patientId},
            {"date", appointment.date},
            {"time", appointment.time}
        });
    }
    saveToFile("appointments.json", appointmentList);
}


void saveMedicalRecordsToFile() {
    json recordList = json::array();
    for (const auto& record : medicalRecords) {
        recordList.push_back({
            {"recordId", record.recordId},
            {"patientId", record.patientId},
            {"visitDate", record.visitDate},
            {"prescription", record.prescription},
            {"diagnosis", record.diagnosis}
        });
    }
    saveToFile("medical_records.json", recordList);
}

void saveDoctorsToFile() {
    json doctorList = json::array();
    for (const auto& doctor : doctors) {
        doctorList.push_back({
            {"id", doctor.id},
            {"name", doctor.name},
            {"specialty", doctor.specialty},
            {"contactInfo", doctor.contactInfo}
        });
    }
    saveToFile("doctors.json", doctorList);
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


void loadBillsFromFile() {
    ensureFileExists("bills.json");
    json billList;
    loadFromFile("bills.json", billList);

    for (const auto& item : billList) {
        Bill bill;
        bill.billId = item["billId"];
        bill.appointmentId = item["appointmentId"];
        bill.patientId = item["patientId"];
        for (const auto& service : item["services"]) {
            bill.services.emplace_back(service["name"], service["cost"]);
        }
        bill.totalCost = item["totalCost"];
        bills.push_back(bill);
    }
}



void loadPatientsFromFile() {
    ensureFileExists("patients.json");
    json patientList;
    loadFromFile("patients.json", patientList);

    for (const auto& item : patientList) {
        if (!item.contains("id") || !item.contains("name") ||
            !item.contains("address") || !item.contains("medicalHistory")) {
            continue;  // Skip invalid entries
        }

        Patient patient;
        patient.id = item["id"];
        patient.name = item["name"];
        patient.address = item["address"];
        patient.medicalHistory = item["medicalHistory"];
        patients.push_back(patient);
    }
}

void loadDoctorsFromFile() {
    ensureFileExists("doctors.json");
    json doctorList;
    loadFromFile("doctors.json", doctorList);

    for (const auto& item : doctorList) {
        Doctor doctor;
        doctor.id = item["id"];
        doctor.name = item["name"];
        doctor.specialty = item["specialty"];
        doctor.contactInfo = item["contactInfo"];
        doctors.push_back(doctor);
    }
}



void loadAppointmentsFromFile() {
    json appointmentList;
    loadFromFile("appointments.json", appointmentList);

    for (const auto& item : appointmentList) {
        Appointment appointment;

        if (!item.contains("id") || !item.contains("patientId") || 
            !item.contains("date") || !item.contains("time")) {
            continue;  // Skip invalid entries
        }

        appointment.id = item["id"];
        appointment.patientId = item["patientId"];
        appointment.date = item["date"];
        appointment.time = item["time"];
        appointments.push_back(appointment);
    }
}


void loadMedicalRecordsFromFile() {
    json recordList;
    loadFromFile("medical_records.json", recordList);
    for (const auto& item : recordList) {
        MedicalRecord record;
        record.recordId = item["recordId"];
        record.patientId = item["patientId"];
        record.visitDate = item["visitDate"];
        record.prescription = item["prescription"];
        record.diagnosis = item["diagnosis"];
        medicalRecords.push_back(record);
    }
}



bool isValidAppointmentTime(const std::string& time) {
    std::regex timeRegex(R"(^([0-1][0-9]|2[0-3]):([0-5][0-9])$)");
    if (!std::regex_match(time, timeRegex)) {
        return false;
    }

    int hour, minute;
    char colon;
    std::istringstream(time) >> hour >> colon >> minute;

    if (hour < 9 || hour > 17 || (hour == 17 && minute > 0)) {
        return false;
    }
    if (minute % 10 != 0) {
        return false;
    }

    return true;
}

bool isValidDate(const std::string& date) {
    std::regex dateRegex(R"(\d{4}-\d{2}-\d{2})");
    return std::regex_match(date, dateRegex);
}


bool isAppointmentSlotTaken(const std::string& date, const std::string& time) {
    std::lock_guard<std::mutex> lock(dataMutex);
    for (const auto& appointment : appointments) {
        if (appointment.date == date && appointment.time == time) {
            return true;
        }
    }
    return false;
}


void fileInsuranceClaim(const Bill& bill, const Patient& patient) {
    if (!patient.hasInsurance) {
        std::cerr << "Patient does not have insurance coverage." << std::endl;
        return;
    }
    // Simulate claim filing
    std::cout << "Filing claim with " << patient.insuranceProvider
              << " for appointment ID: " << bill.appointmentId << std::endl;
}


int main() {
    crow::SimpleApp app;
loadPatientsFromFile();
loadAppointmentsFromFile();
loadMedicalRecordsFromFile();
loadDoctorsFromFile();
loadBillsFromFile();





CROW_ROUTE(app, "/")([]() {
    return "Welcome to the Healthcare System ";
    });


CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {
        auto qs = req.url_params;
        const char* name = qs.get("name");
        const char* address = qs.get("address");
        const char* medicalHistory = qs.get("medicalHistory");
        const char* doctorIdStr = qs.get("doctorId");

        if (!name || !address || !medicalHistory || !doctorIdStr) {
            return crow::response(400, "Missing required parameters: name, address, medicalHistory, or doctorId");
        }

        int doctorId = std::atoi(doctorIdStr);
        bool doctorExists = false;
        for (const auto& doctor : doctors) {
            if (doctor.id == doctorId) {
                doctorExists = true;
                break;
            }
        }
        if (!doctorExists) {
            return crow::response(404, "Doctor not found");
        }

        Patient newPatient;
        newPatient.id = patients.size() + 1;
        newPatient.name = name;
        newPatient.address = address;
        newPatient.medicalHistory = medicalHistory;
        newPatient.doctorId = doctorId;
        patients.push_back(newPatient);
        savePatientsToFile();

        crow::json::wvalue response;
        response["message"] = "Patient registered successfully!";
        response["patientId"] = newPatient.id;
        return crow::response(response); 
    }

    
    return crow::response(405, "Method Not Allowed");
});

CROW_ROUTE(app, "/book_appointment").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {
        auto qs = req.url_params;
        const char* patientIdStr = qs.get("patientId");
        const char* date = qs.get("date");
        const char* time = qs.get("time");
        
        if (!patientIdStr || !date || !time) {
            return crow::response(400, "Missing required query parameters: patientId, date, or time");
        }

        int patientId = std::atoi(patientIdStr);
        bool patientExists = false;
        for (const auto& patient : patients) {
            if (patient.id == patientId) {
                patientExists = true;
                break;
            }
        }
        if (!patientExists) {
            return crow::response(404, "Patient not found");
        }
        for (const auto& appointment : appointments) {
            if (appointment.date == date && appointment.time == time) {
                return crow::response(400, "Appointment slot already taken");
            }
        }

        Appointment newAppointment = {static_cast<int>(appointments.size()) + 1, patientId, date, time};
        appointments.push_back(newAppointment);
        saveAppointmentsToFile();

        crow::json::wvalue response;
        response["message"] = "Appointment booked successfully!";
        response["appointmentId"] = newAppointment.id;
        return crow::response(response);
    }

    auto body = crow::json::load(req.body);
    if (!body) {
        return crow::response(400, "Invalid JSON data");
    }

    if (!body["patientId"] || !body["date"] || !body["time"]) {
        return crow::response(400, "Missing required fields: patientId, date, or time");
    }

    int patientId = body["patientId"].i();
    std::string date = body["date"].s();
    std::string time = body["time"].s();

    bool patientExists = false;
    for (const auto& patient : patients) {
        if (patient.id == patientId) {
            patientExists = true;
            break;
        }
    }
    if (!patientExists) {
        return crow::response(404, "Patient not found");
    }

    for (const auto& appointment : appointments) {
        if (appointment.date == date && appointment.time == time) {
            return crow::response(400, "Appointment slot already taken");
        }
    }

    Appointment newAppointment = {static_cast<int>(appointments.size()) + 1, patientId, date, time};
    appointments.push_back(newAppointment);
    saveAppointmentsToFile();

    crow::json::wvalue response;
    response["message"] = "Appointment booked successfully!";
    response["appointmentId"] = newAppointment.id;
    return crow::response(response);
});

CROW_ROUTE(app, "/patients").methods(crow::HTTPMethod::GET)([]() {
    crow::json::wvalue response;
    std::vector<crow::json::wvalue> patientList;
    for (const auto& patient : patients) {
        crow::json::wvalue patientInfo;
        patientInfo["id"] = patient.id;
        patientInfo["name"] = patient.name;
        patientInfo["address"] = patient.address;
        patientInfo["medicalHistory"] = patient.medicalHistory;
        patientList.push_back(std::move(patientInfo));
    }
    response["patients"] = std::move(patientList);
    return crow::response(response);
});

CROW_ROUTE(app, "/generate_bill").methods(crow::HTTPMethod::GET)([](const crow::request& req) {
    auto qs = req.url_params;
    const char* appointmentIdStr = qs.get("appointmentId");
    const char* patientIdStr = qs.get("patientId");
    const char* servicesStr = qs.get("services");

    if (!appointmentIdStr || !patientIdStr || !servicesStr) {
        return crow::response(400, "Missing required parameters: appointmentId, patientId, services");
    }

    int appointmentId = std::atoi(appointmentIdStr);
    int patientId = std::atoi(patientIdStr);

    bool appointmentExists = false;
    for (const auto& appointment : appointments) {
        if (appointment.appointmentId == appointmentId && appointment.patientId == patientId) {
            appointmentExists = true;
            break;
        }
    }
    if (!appointmentExists) {
        return crow::response(404, "Appointment not found for the provided ID and patient.");
    }

    std::vector<std::pair<std::string, double>> services;
    double totalCost = 0.0;

    std::stringstream ss(servicesStr);
    std::string service;
    while (std::getline(ss, service, ',')) {
        auto pos = service.find(':');
        if (pos == std::string::npos) {
            return crow::response(400, "Invalid service format. Use 'Service:Cost,Service:Cost'");
        }

        std::string serviceName = service.substr(0, pos);
        double cost = std::stod(service.substr(pos + 1));
        services.emplace_back(serviceName, cost);
        totalCost += cost;
    }

    Bill newBill = {
        static_cast<int>(bills.size() + 1),
        appointmentId,
        patientId,
        services,
        totalCost
    };

    bills.push_back(newBill);
    saveBillsToFile();

    crow::json::wvalue response;
    response["message"] = "Bill generated successfully!";
    response["billId"] = newBill.billId;
    response["appointmentId"] = appointmentId;
    response["patientId"] = patientId;
    response["totalCost"] = totalCost;
    return crow::response(response);
});


CROW_ROUTE(app, "/file_claim/<int>").methods(crow::HTTPMethod::POST)([](int appointmentId) {
    // Find the appointment
    auto appointmentIt = std::find_if(appointments.begin(), appointments.end(), [&](const Appointment& appointment) {
        return appointment.id == appointmentId;
    });
    if (appointmentIt == appointments.end()) {
        return crow::response(404, "Appointment not found.");
    }

    auto billIt = std::find_if(bills.begin(), bills.end(), [&](const Bill& bill) {
        return bill.appointmentId == appointmentId;
    });
    if (billIt == bills.end()) {
        return crow::response(404, "Bill not found for the given appointment.");
    }

    int patientId = appointmentIt->patientId;
    auto patientIt = std::find_if(patients.begin(), patients.end(), [&](const Patient& patient) {
        return patient.id == patientId;
    });
    if (patientIt == patients.end()) {
        return crow::response(404, "Patient not found for the given appointment.");
    }

    if (!patientIt->hasInsurance) {
        return crow::response(400, "Patient does not have insurance.");
    }

    fileInsuranceClaim(*billIt, *patientIt);

    return crow::response(200, "Insurance claim filed successfully.");
});



CROW_ROUTE(app, "/appointments").methods(crow::HTTPMethod::GET)([]() {
    crow::json::wvalue response;
    std::vector<crow::json::wvalue> appointmentList;

    for (const auto& appointment : appointments) {
        crow::json::wvalue appointmentInfo;
        appointmentInfo["id"] = appointment.id;           
        appointmentInfo["patientId"] = appointment.patientId;
        appointmentInfo["date"] = appointment.date;
        appointmentInfo["time"] = appointment.time;
        appointmentList.push_back(std::move(appointmentInfo));
    }

    response["appointments"] = std::move(appointmentList);
    return crow::response(response);
});


CROW_ROUTE(app, "/register_doctor").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {
        auto qs = req.url_params;
        const char* name = qs.get("name");
        const char* specialty = qs.get("specialty");
        const char* contactInfo = qs.get("contactInfo");

        if (!name || !specialty || !contactInfo) {
            return crow::response(400, "Missing required parameters: name, specialty, or contactInfo");
        }

        Doctor newDoctor;
        newDoctor.id = doctors.size() + 1;
        newDoctor.name = name;
        newDoctor.specialty = specialty;
        newDoctor.contactInfo = contactInfo;
        doctors.push_back(newDoctor);
        saveDoctorsToFile();

        crow::json::wvalue response;
        response["message"] = "Doctor registered successfully!";
        response["doctorId"] = newDoctor.id;
        return crow::response(response);
    }

    auto body = crow::json::load(req.body);
    if (!body || !body["name"] || !body["specialty"] || !body["contactInfo"]) {
        return crow::response(400, "Missing required fields: name, specialty, or contactInfo");
    }

    Doctor newDoctor;
    newDoctor.id = doctors.size() + 1;
    newDoctor.name = body["name"].s();
    newDoctor.specialty = body["specialty"].s();
    newDoctor.contactInfo = body["contactInfo"].s();
    doctors.push_back(newDoctor);
    saveDoctorsToFile();

    crow::json::wvalue response;
    response["message"] = "Doctor registered successfully!";
    response["doctorId"] = newDoctor.id;
    return crow::response(response);
});


CROW_ROUTE(app, "/add_record").methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {
        auto qs = req.url_params;
        const char* patientIdStr = qs.get("patientId");
        const char* visitDate = qs.get("visitDate");
        const char* prescription = qs.get("prescription");
        const char* diagnosis = qs.get("diagnosis");

        if (!patientIdStr || !visitDate || !prescription || !diagnosis) {
            return crow::response(400, "Missing required parameters: patientId, visitDate, prescription, diagnosis");
        }

        int patientId = std::atoi(patientIdStr);

        bool patientExists = false;
        for (const auto& patient : patients) {
            if (patient.id == patientId) {
                patientExists = true;
                break;
            }
        }
        if (!patientExists) {
            return crow::response(404, "Patient not found");
        }
        MedicalRecord newRecord = {
            (int)medicalRecords.size() + 1,
            patientId,
            visitDate,
            prescription,
            diagnosis
        };
        medicalRecords.push_back(newRecord);

        crow::json::wvalue response;
        response["message"] = "Medical record added successfully!";
        response["recordId"] = newRecord.recordId;
        return crow::response(response);
    }
    auto body = crow::json::load(req.body);
    if (!body || !body["patientId"] || !body["visitDate"] || !body["prescription"] || !body["diagnosis"]) {
        return crow::response(400, "Missing required fields");
    }

    int patientId = body["patientId"].i();
    std::string visitDate = body["visitDate"].s();
    std::string prescription = body["prescription"].s();
    std::string diagnosis = body["diagnosis"].s();

    bool patientExists = false;
    for (const auto& patient : patients) {
        if (patient.id == patientId) {
            patientExists = true;
            break;
        }
    }
    if (!patientExists) {
        return crow::response(404, "Patient not found");
    }

    MedicalRecord newRecord = { (int)medicalRecords.size() + 1, patientId, visitDate, prescription, diagnosis };
    medicalRecords.push_back(newRecord);
    saveMedicalRecordsToFile();

    crow::json::wvalue response;
    response["message"] = "Medical record added successfully!";
    response["recordId"] = newRecord.recordId;
    return crow::response(response);
});


CROW_ROUTE(app, "/doctors").methods(crow::HTTPMethod::GET)([]() {
    crow::json::wvalue response;
    std::vector<crow::json::wvalue> doctorList;
    for (const auto& doctor : doctors) {
        crow::json::wvalue doctorInfo;
        doctorInfo["id"] = doctor.id;
        doctorInfo["name"] = doctor.name;
        doctorInfo["specialty"] = doctor.specialty;
        doctorInfo["contactInfo"] = doctor.contactInfo;
        doctorList.push_back(std::move(doctorInfo));
    }
    response["doctors"] = std::move(doctorList);
    return crow::response(response);
});


CROW_ROUTE(app, "/patients_by_doctor/<int>").methods(crow::HTTPMethod::GET)([](int doctorId) {
    crow::json::wvalue response;
    std::vector<crow::json::wvalue> patientList;

    for (const auto& patient : patients) {
        if (patient.doctorId == doctorId) {
            crow::json::wvalue patientInfo;
            patientInfo["id"] = patient.id;
            patientInfo["name"] = patient.name;
            patientInfo["address"] = patient.address;
            patientInfo["medicalHistory"] = patient.medicalHistory;
            patientInfo["doctorId"] = patient.doctorId;
            patientList.push_back(std::move(patientInfo));
        }
    }

    if (patientList.empty()) {
        return crow::response(404, "No patients found for this doctor");
    }

    response["patients"] = std::move(patientList);
    return crow::response(response);
});


CROW_ROUTE(app, "/medical_history/<int>").methods(crow::HTTPMethod::GET)([](int patientId) {
    crow::json::wvalue response;
    std::vector<crow::json::wvalue> recordList;

    for (const auto& record : medicalRecords) {
        if (record.patientId == patientId) {
            crow::json::wvalue recordInfo;
            recordInfo["patientId"] = record.patientId;
            recordInfo["recordId"] = record.recordId;
            recordInfo["visitDate"] = record.visitDate;
            recordInfo["prescription"] = record.prescription;
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
    app.port(8080).multithreaded().run();
}
