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
};

struct Doctor {
    int id;
    std::string name;
    std::string specialty;
    std::string contactInfo;
};



struct Appointment {
    int patientId; 
    std::string date;
    std::string time;
};
struct MedicalRecord {
    int recordId;
    int patientId;
    std::string visitDate;
    std::string notes;
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


void saveAppointmentsToFile() {
    json appointmentList = json::array();
    for (const auto& appointment : appointments) {
        appointmentList.push_back({
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
            {"notes", record.notes},
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
        record.notes = item["notes"];
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

int main() {
    crow::SimpleApp app;
loadPatientsFromFile();
loadAppointmentsFromFile();
loadMedicalRecordsFromFile();
loadDoctorsFromFile();




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

        Appointment newAppointment = {patientId, date, time};
        appointments.push_back(newAppointment);
        saveAppointmentsToFile();

        crow::json::wvalue response;
        response["message"] = "Appointment booked successfully!";
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

    Appointment newAppointment = {patientId, date, time};
    appointments.push_back(newAppointment);

    crow::json::wvalue response;
    response["message"] = "Appointment booked successfully!";
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

CROW_ROUTE(app, "/appointments").methods(crow::HTTPMethod::GET)([]() {
    crow::json::wvalue response;
    std::vector<crow::json::wvalue> appointmentList;
    for (const auto& appointment : appointments) {
        crow::json::wvalue appointmentInfo;
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
        const char* notes = qs.get("notes");
        const char* diagnosis = qs.get("diagnosis");

        if (!patientIdStr || !visitDate || !notes || !diagnosis) {
            return crow::response(400, "Missing required parameters: patientId, visitDate, notes, diagnosis");
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
            notes,
            diagnosis
        };
        medicalRecords.push_back(newRecord);

        crow::json::wvalue response;
        response["message"] = "Medical record added successfully!";
        response["recordId"] = newRecord.recordId;
        return crow::response(response);
    }
    auto body = crow::json::load(req.body);
    if (!body || !body["patientId"] || !body["visitDate"] || !body["notes"] || !body["diagnosis"]) {
        return crow::response(400, "Missing required fields");
    }

    int patientId = body["patientId"].i();
    std::string visitDate = body["visitDate"].s();
    std::string notes = body["notes"].s();
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

    MedicalRecord newRecord = { (int)medicalRecords.size() + 1, patientId, visitDate, notes, diagnosis };
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


    // Start the server on port 8080
    app.port(8080).multithreaded().run();
}
