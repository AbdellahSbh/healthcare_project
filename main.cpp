#include "crow.h"
#include <vector>
#include <string>
#include <regex>
#include <mutex>
#include <sstream>

struct Patient {
    int id;
    std::string name;
    std::string address;
    std::string medicalHistory;
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


CROW_ROUTE(app, "/")([]() {
    return "Welcome to the Healthcare System ";
    });


CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {

        auto qs = req.url_params;
        const char* name = qs.get("name");
        const char* address = qs.get("address");
        const char* medicalHistory = qs.get("medicalHistory");

        if (!name || !address || !medicalHistory) {
            return crow::response(400, "Missing required parameters: name, address, or medicalHistory");
        }

        Patient newPatient;
        newPatient.id = patients.size() + 1;
        newPatient.name = name;
        newPatient.address = address;
        newPatient.medicalHistory = medicalHistory;
        patients.push_back(newPatient);

        crow::json::wvalue response;
        response["message"] = "Patient registered successfully!";
        response["patientId"] = newPatient.id;
        return crow::response(response);
    }

    auto body = crow::json::load(req.body);
    if (!body) {
        return crow::response(400, "Invalid JSON data");
    }

    if (!body["name"] || !body["address"] || !body["medicalHistory"]) {
        return crow::response(400, "Missing required fields: name, address, or medicalHistory");
    }

    Patient newPatient;
    newPatient.id = patients.size() + 1;
    newPatient.name = body["name"].s();
    newPatient.address = body["address"].s();
    newPatient.medicalHistory = body["medicalHistory"].s();
    patients.push_back(newPatient);

    crow::json::wvalue response;
    response["message"] = "Patient registered successfully!";
    response["patientId"] = newPatient.id;
    return crow::response(response);
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

    crow::json::wvalue response;
    response["message"] = "Medical record added successfully!";
    response["recordId"] = newRecord.recordId;
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
