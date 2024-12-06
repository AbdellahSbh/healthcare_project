#include "crow.h"
#include <vector>
#include <string>
#include <regex>
#include <mutex>
#include <sstream>
#include <iomanip>

struct Patient {
    std::string idNumber; 
    std::string name;
    std::string address;
    std::string medicalHistory;
};  

struct Appointment {
    std::string idNumber; 
    std::string date;
    std::string time;
};

std::vector<Patient> patients;
std::vector<Appointment> appointments;
std::mutex dataMutex; 

bool isValidIdNumber(const std::string& idNumber) {
    std::regex idNumberRegex(R"(\d{17}[\dX])");
    return std::regex_match(idNumber, idNumberRegex);
}

bool isIdNumberDuplicate(const std::string& idNumber) {
    std::lock_guard<std::mutex> lock(dataMutex);
    for (const auto& patient : patients) {
        if (patient.idNumber == idNumber) {
            return true;
        }
    }
    return false;
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
        return "Welcome to the Digital Healthcare System API!";
    });


    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        auto qs = req.url_params;
        const char* name = qs.get("name");
        const char* address = qs.get("address");
        const char* medicalHistory = qs.get("medicalHistory");
        const char* idNumber = qs.get("idNumber");

        if (!name || !address || !medicalHistory || !idNumber) {
            return crow::response(400, "Missing required parameters: name, address, medicalHistory, idNumber.");
        }

        std::string idNumberStr = idNumber;
        if (!isValidIdNumber(idNumberStr)) {
            return crow::response(400, "Invalid ID number format. It should be 18 characters long, with digits and possibly ending in 'X'.");
        }

        if (isIdNumberDuplicate(idNumberStr)) {
            return crow::response(400, "ID number is already registered.");
        }

        Patient newPatient;
        newPatient.idNumber = idNumberStr;
        newPatient.name = name;
        newPatient.address = address;
        newPatient.medicalHistory = medicalHistory;

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            patients.push_back(newPatient);
        }

        crow::json::wvalue response;
        response["message"] = "Patient registered successfully!";
        response["idNumber"] = newPatient.idNumber;
        return crow::response(200, response);
    });

    CROW_ROUTE(app, "/patients").methods(crow::HTTPMethod::GET)([]() {
        crow::json::wvalue response;
        response["patients"] = crow::json::wvalue::list(); // 初始化为数组类型

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            size_t index = 0; // 显式索引
            for (const auto& patient : patients) {
                // 动态分配 JSON 对象
                response["patients"][index]["idNumber"] = patient.idNumber;
                response["patients"][index]["name"] = patient.name;
                response["patients"][index]["address"] = patient.address;
                response["patients"][index]["medicalHistory"] = patient.medicalHistory;
                index++; // 增加索引
            }
        }

        return crow::response(200, response);
        });

    CROW_ROUTE(app, "/appointments").methods(crow::HTTPMethod::GET)([]() {
        crow::json::wvalue response;
        response["appointments"] = crow::json::wvalue::list(); // 初始化为数组类型

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            size_t index = 0; // 显式索引
            for (const auto& appointment : appointments) {
                // 动态分配 JSON 对象
                response["appointments"][index]["idNumber"] = appointment.idNumber;
                response["appointments"][index]["date"] = appointment.date;
                response["appointments"][index]["time"] = appointment.time;
                index++; // 增加索引
            }
        }

        return crow::response(200, response);
        });

    CROW_ROUTE(app, "/book_appointment").methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        auto qs = req.url_params;

        const char* idNumber = qs.get("idNumber");
        const char* date = qs.get("date");
        const char* time = qs.get("time");
        if (!idNumber || !date || !time) {
            return crow::response(400, "Missing required parameters: idNumber, date, time.");
        }

        std::string idNumberStr = idNumber;
        bool patientExists = false;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            for (const auto& patient : patients) {
                if (patient.idNumber == idNumberStr) {
                    patientExists = true;
                    break;
                }
            }
        }
        if (!patientExists) {
            return crow::response(404, "Patient not found.");
        }
        if (!isValidAppointmentTime(time)) {
            return crow::response(400, "Invalid appointment time. It must be between 09:00 and 17:00, and in 10-minute intervals.");
        }
        if (isAppointmentSlotTaken(date, time)) {
            return crow::response(400, "Appointment slot already taken.");
        }
        Appointment newAppointment = { idNumberStr, date, time };
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            appointments.push_back(newAppointment);
        }
        crow::json::wvalue response;
        response["message"] = "Appointment booked successfully!";
        response["idNumber"] = idNumberStr;
        response["date"] = date;
        response["time"] = time;
        return crow::response(200, response);
    });
    app.port(8080).multithreaded().run();
}
