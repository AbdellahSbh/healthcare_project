#include "crow.h"
#include <vector>
#include <string>

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

std::vector<Patient> patients;
std::vector<Appointment> appointments;

int main() {
    crow::SimpleApp app;


    CROW_ROUTE(app, "/")([]() {
        return "Welcome to the Healthcare System API";
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

    // Start the server on port 8080
    app.port(8080).multithreaded().run();
}
