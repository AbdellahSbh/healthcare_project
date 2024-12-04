#include "crow.h"
#include <vector>
#include <string>

// Patient structure to store patient details
struct Patient {
    int id;
    std::string name;
    std::string address;
    std::string medicalHistory;
};

// Appointment structure to store appointment details
struct Appointment {
    int patientId;  // ID of the patient
    std::string date;
    std::string time;
};

// Global storage for patients and appointments
std::vector<Patient> patients;
std::vector<Appointment> appointments;

int main() {
    crow::SimpleApp app;

    // Root route - Welcome message
    CROW_ROUTE(app, "/")([]() {
        return "Welcome to the Healthcare System API";
    });

    // Patient Registration Route
CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {
        // Extract query parameters
        auto qs = req.url_params;
        const char* name = qs.get("name");
        const char* address = qs.get("address");
        const char* medicalHistory = qs.get("medicalHistory");

        // Validate query parameters
        if (!name || !address || !medicalHistory) {
            return crow::response(400, "Missing required parameters: name, address, or medicalHistory");
        }

        // Create and register a new patient
        Patient newPatient;
        newPatient.id = patients.size() + 1;
        newPatient.name = name;
        newPatient.address = address;
        newPatient.medicalHistory = medicalHistory;
        patients.push_back(newPatient);

        // Return success response
        crow::json::wvalue response;
        response["message"] = "Patient registered successfully!";
        response["patientId"] = newPatient.id;
        return crow::response(response);
    }

    // Handle POST request for JSON input (kept for compatibility)
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
    // Appointment Booking Route
CROW_ROUTE(app, "/book_appointment").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([](const crow::request& req) {
    if (req.method == crow::HTTPMethod::GET) {
        // Parse query parameters from the URL
        auto qs = req.url_params;
        const char* patientIdStr = qs.get("patientId");
        const char* date = qs.get("date");
        const char* time = qs.get("time");

        // Validate query parameters
        if (!patientIdStr || !date || !time) {
            return crow::response(400, "Missing required query parameters: patientId, date, or time");
        }

        int patientId = std::atoi(patientIdStr);

        // Check if the patient exists
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

        // Check if the appointment slot is already taken
        for (const auto& appointment : appointments) {
            if (appointment.date == date && appointment.time == time) {
                return crow::response(400, "Appointment slot already taken");
            }
        }

        // Create and book a new appointment
        Appointment newAppointment = {patientId, date, time};
        appointments.push_back(newAppointment);

        // Return success response
        crow::json::wvalue response;
        response["message"] = "Appointment booked successfully!";
        return crow::response(response);
    }

    // Handle POST requests for JSON input (unchanged)
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

    // Check if the patient exists
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

    // Check if the appointment slot is already taken
    for (const auto& appointment : appointments) {
        if (appointment.date == date && appointment.time == time) {
            return crow::response(400, "Appointment slot already taken");
        }
    }

    // Create and book a new appointment
    Appointment newAppointment = {patientId, date, time};
    appointments.push_back(newAppointment);

    // Return success response
    crow::json::wvalue response;
    response["message"] = "Appointment booked successfully!";
    return crow::response(response);
});

    // Start the server on port 8080
    app.port(8080).multithreaded().run();
}
