#include "dicom_editor.h"
#include <iostream>
#include <algorithm>
#include <regex>

DicomEditor::DicomEditor(OrthancClient& client)
    : client_(client)
{
}

std::map<std::string, std::string> DicomEditor::GetPatientTags(const std::string& patientId) {
    std::map<std::string, std::string> tags;
    
    try {
        auto patient = client_.GetPatient(patientId);
        if (patient.is_null()) {
            lastError_ = "Paciente no encontrado: " + patientId;
            return tags;
        }
        
        // Tags editables desde MainDicomTags
        if (patient.contains("MainDicomTags")) {
            auto& mainTags = patient["MainDicomTags"];
            for (const auto& tag : GetEditableTags()) {
                if (mainTags.contains(tag)) {
                    tags[tag] = mainTags[tag].get<std::string>();
                } else {
                    tags[tag] = "";
                }
            }
        }
        
    } catch (const std::exception& e) {
        lastError_ = e.what();
    }
    
    return tags;
}

bool DicomEditor::SetTag(const std::string& patientId, const std::string& tag, const std::string& value) {
    std::map<std::string, std::string> tags;
    tags[tag] = value;
    return SetTags(patientId, tags);
}

bool DicomEditor::SetTags(const std::string& patientId, const std::map<std::string, std::string>& tags) {
    try {
        // Validar tags
        for (const auto& [tag, value] : tags) {
            std::string error = ValidateTag(tag, value);
            if (!error.empty()) {
                lastError_ = error;
                return false;
            }
        }
        
        // Primero obtenemos el paciente actual para preservar otros campos
        auto patient = client_.GetPatient(patientId);
        if (patient.is_null()) {
            lastError_ = "Paciente no encontrado: " + patientId;
            return false;
        }
        
        // Construir el payload con los tags a modificar
        // Orthanc espera un objeto con los tags DICOM en formato "MainDicomTags"
        json payload;
        if (patient.contains("MainDicomTags")) {
            payload = patient["MainDicomTags"];
        }
        
        for (const auto& [tag, value] : tags) {
            payload[tag] = value;
        }
        
        // Enviar la actualización a Orthanc
        json body;
        body["MainDicomTags"] = payload;
        
        bool success = client_.UpdatePatient(patientId, body);
        
        if (!success) {
            lastError_ = client_.GetLastError();
            if (lastError_.empty()) lastError_ = "Error al actualizar el paciente en Orthanc";
            return false;
        }
        
        std::cout << "\033[32m✓ Tags actualizados correctamente en Orthanc\033[0m\n";
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

std::vector<std::string> DicomEditor::GetEditableTags() {
    return {
        "PatientName",
        "PatientID",
        "PatientBirthDate",
        "PatientSex",
        "PatientAge",
        "PatientWeight",
        "PatientSize",
        "PatientAddress",
        "PatientTelephoneNumbers",
        "OtherPatientIDs"
    };
}

std::string DicomEditor::ValidateTag(const std::string& tag, const std::string& value) {
    if (tag == "PatientName") {
        if (value.empty()) return "El nombre del paciente no puede estar vacío";
        if (value.length() > 255) return "El nombre es demasiado largo (máx. 255 caracteres)";
        return "";
    }
    
    if (tag == "PatientID") {
        if (value.empty()) return "El ID del paciente no puede estar vacío";
        return "";
    }
    
    if (tag == "PatientBirthDate") {
        if (value.empty()) return ""; // Opcional
        // Formato: YYYYMMDD
        std::regex dateRegex("^\\d{8}$");
        if (!std::regex_match(value, dateRegex)) {
            return "Formato de fecha inválido. Use YYYYMMDD (ej: 19850315)";
        }
        return "";
    }
    
    if (tag == "PatientSex") {
        if (!value.empty() && value != "M" && value != "F" && value != "O") {
            return "Sexo inválido. Use M, F, O o déjelo vacío";
        }
        return "";
    }
    
    if (tag == "PatientAge") {
        if (value.empty()) return "";
        std::regex ageRegex("^\\d{3}[YMWD]$");
        if (!std::regex_match(value, ageRegex)) {
            return "Formato de edad inválido. Use 3 dígitos + unidad (Y/M/W/D). Ej: 035Y";
        }
        return "";
    }
    
    if (tag == "PatientWeight" || tag == "PatientSize") {
        if (value.empty()) return "";
        // Debe ser un número
        try {
            std::stof(value);
        } catch (...) {
            return "Debe ser un valor numérico";
        }
        return "";
    }
    
    return ""; // Tags desconocidos se aceptan sin validación
}

std::string DicomEditor::GetTagDescription(const std::string& tag) {
    static const std::map<std::string, std::string> descriptions = {
        {"PatientName", "Nombre del paciente"},
        {"PatientID", "ID del paciente"},
        {"PatientBirthDate", "Fecha de nacimiento (YYYYMMDD)"},
        {"PatientSex", "Sexo (M/F/O)"},
        {"PatientAge", "Edad (XXX formato: 035Y)"},
        {"PatientWeight", "Peso (kg)"},
        {"PatientSize", "Talla (cm)"},
        {"PatientAddress", "Dirección"},
        {"PatientTelephoneNumbers", "Teléfono"},
        {"OtherPatientIDs", "Otros IDs"}
    };
    
    auto it = descriptions.find(tag);
    if (it != descriptions.end()) return it->second;
    return tag;
}
