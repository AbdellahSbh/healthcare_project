CREATE TABLE IF NOT EXISTS Patients (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    address TEXT NOT NULL,
    medicalHistory TEXT,
    hasInsurance INTEGER NOT NULL CHECK (hasInsurance IN (0, 1)),
    insuranceCompany TEXT
);

CREATE TABLE IF NOT EXISTS Doctors (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    specialty TEXT NOT NULL,
    contactInfo TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS Appointments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    patientId INTEGER NOT NULL,
    doctorId INTEGER NOT NULL,
    date TEXT NOT NULL,
    time TEXT NOT NULL,
    FOREIGN KEY (patientId) REFERENCES Patients(id),
    FOREIGN KEY (doctorId) REFERENCES Doctors(id)
);

CREATE TABLE IF NOT EXISTS Bills (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    patientId INTEGER NOT NULL,
    appointmentId INTEGER NOT NULL,
    medicationFee REAL DEFAULT 0.0,
    consultationFee REAL DEFAULT 0.0,
    surgeryFee REAL DEFAULT 0.0,
    totalFee REAL DEFAULT 0.0,
    isInsured INTEGER NOT NULL CHECK (isInsured IN (0, 1)),
    claimed INTEGER DEFAULT 0 CHECK (claimed IN (0, 1)),
    insuranceCompany TEXT,
    claimStatus TEXT DEFAULT 'Not Submitted',
    FOREIGN KEY (patientId) REFERENCES Patients(id),
    FOREIGN KEY (appointmentId) REFERENCES Appointments(id)
);

CREATE TABLE IF NOT EXISTS Prescriptions (
    prescriptionId INTEGER PRIMARY KEY AUTOINCREMENT,
    patientId INTEGER NOT NULL,
    doctorId INTEGER NOT NULL,
    medication TEXT NOT NULL,
    dosage TEXT NOT NULL,
    instructions TEXT,
    datePrescribed TEXT NOT NULL,
    FOREIGN KEY (patientId) REFERENCES Patients(id),
    FOREIGN KEY (doctorId) REFERENCES Doctors(id)
);
CREATE TABLE IF NOT EXISTS Inventory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemName TEXT NOT NULL,
    quantity INTEGER NOT NULL CHECK (quantity >= 0)
);

CREATE TABLE IF NOT EXISTS Notifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemName TEXT NOT NULL,
    message TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);


