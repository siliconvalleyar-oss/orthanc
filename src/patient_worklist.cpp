#include "patient_worklist.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

PatientWorklist::PatientWorklist(OrthancClient& client)
    : client_(client)
{
}

bool PatientWorklist::Refresh() {
    try {
        auto ids = client_.GetPatientIds();
        patients_.clear();
        
        for (const auto& id : ids) {
            auto patient = client_.GetPatient(id);
            if (!patient.is_null()) {
                patients_.push_back(patient);
            }
        }
        
        // Ordenar alfabéticamente por nombre
        std::sort(patients_.begin(), patients_.end(),
            [](const json& a, const json& b) {
                std::string nameA = GetDicomTag(a, "PatientName");
                std::string nameB = GetDicomTag(b, "PatientName");
                return nameA < nameB;
            });
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "\033[31m[ERROR] Error al refrescar worklist: " << e.what() << "\033[0m" << std::endl;
        return false;
    }
}

const json& PatientWorklist::GetPatient(size_t index) const {
    static json emptyJson;
    if (index >= patients_.size()) return emptyJson;
    return patients_[index];
}

std::string PatientWorklist::GetPatientName(size_t index) const {
    return GetDicomTag(GetPatient(index), "PatientName");
}

std::string PatientWorklist::GetPatientSummary(size_t index) const {
    if (index >= patients_.size()) return "";
    
    const auto& p = patients_[index];
    std::string name = GetDicomTag(p, "PatientName");
    std::string id = GetDicomTag(p, "PatientID");
    std::string sex = GetDicomTag(p, "PatientSex");
    std::string birth = GetDicomTag(p, "PatientBirthDate");
    
    std::string summary = name + " | ID: " + id;
    if (!birth.empty() && birth != "N/A") summary += " | Nac: " + birth;
    if (!sex.empty() && sex != "N/A" && sex != "O") summary += " | Sexo: " + (sex == "M" ? "Masculino" : sex == "F" ? "Femenino" : sex);
    
    // Contar estudios
    if (p.contains("Studies")) {
        size_t studyCount = p["Studies"].size();
        summary += " | Estudios: " + std::to_string(studyCount);
    }
    
    return summary;
}

json PatientWorklist::GetPatientStudies(size_t index) const {
    if (index >= patients_.size()) return json::array();
    
    const auto& patient = patients_[index];
    std::string patientId = GetPatientId(patient);
    
    try {
        return client_.GetPatientStudies(patientId);
    } catch (...) {
        return json::array();
    }
}

json PatientWorklist::GetStudySeries(const std::string& studyId) const {
    try {
        return client_.GetStudySeries(studyId);
    } catch (...) {
        return json::array();
    }
}

json PatientWorklist::GetSeriesInstances(const std::string& seriesId) const {
    try {
        return client_.GetSeriesInstances(seriesId);
    } catch (...) {
        return json::array();
    }
}

std::vector<size_t> PatientWorklist::FindPatients(const std::string& query) const {
    std::vector<size_t> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (size_t i = 0; i < patients_.size(); i++) {
        std::string name = GetDicomTag(patients_[i], "PatientName");
        std::string id = GetDicomTag(patients_[i], "PatientID");
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        
        if (name.find(lowerQuery) != std::string::npos ||
            id.find(lowerQuery) != std::string::npos) {
            results.push_back(i);
        }
    }
    
    return results;
}

void PatientWorklist::PrintWorklist() const {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                    LISTA DE PACIENTES (WORKLIST)                ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n";
    
    if (patients_.empty()) {
        std::cout << "\n  \033[33mNo hay pacientes en el servidor Orthanc.\033[0m\n";
        std::cout << "  Use la opción de escaneo para importar archivos DICOM.\n";
        return;
    }
    
    std::cout << "\n  \033[36mPacientes encontrados: " << patients_.size() << "\033[0m\n\n";
    
    for (size_t i = 0; i < patients_.size(); i++) {
        const auto& p = patients_[i];
        std::string name = GetDicomTag(p, "PatientName");
        std::string pid = GetDicomTag(p, "PatientID");
        std::string sex = GetDicomTag(p, "PatientSex");
        std::string birth = GetDicomTag(p, "PatientBirthDate");
        
        std::cout << "  \033[33m[" << (i + 1) << "]\033[0m ";
        std::cout << "\033[1m" << name << "\033[0m\n";
        std::cout << "      ID: " << pid;
        if (!sex.empty() && sex != "N/A") std::cout << " | Sexo: " << sex;
        if (!birth.empty() && birth != "N/A") std::cout << " | Nac: " << birth;
        
        if (p.contains("Studies")) {
            std::cout << " | Estudios: " << p["Studies"].size();
        }
        std::cout << "\n";
    }
}

void PatientWorklist::PrintPatientDetails(size_t index) const {
    if (index >= patients_.size()) {
        std::cerr << "\033[31m[ERROR] Índice de paciente inválido\033[0m\n";
        return;
    }
    
    const auto& p = patients_[index];
    
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                    DETALLES DEL PACIENTE                        ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n";
    
    std::cout << "  \033[1mNombre:\033[0m        " << GetDicomTag(p, "PatientName") << "\n";
    std::cout << "  \033[1mID del paciente:\033[0m " << GetDicomTag(p, "PatientID") << "\n";
    std::cout << "  \033[1mFecha nacimiento:\033[0m " << GetDicomTag(p, "PatientBirthDate") << "\n";
    std::cout << "  \033[1mSexo:\033[0m           " << GetDicomTag(p, "PatientSex") << "\n";
    std::cout << "  \033[1mOrthanc ID:\033[0m     " << GetPatientId(p) << "\n";
    
    if (p.contains("Studies")) {
        std::cout << "  \033[1mEstudios:\033[0m       " << p["Studies"].size() << "\n";
    }
}

void PatientWorklist::PrintPatientStudies(size_t index) const {
    if (index >= patients_.size()) return;
    
    auto studies = GetPatientStudies(index);
    
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                 ESTUDIOS DEL PACIENTE                           ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n";
    
    if (studies.is_null() || studies.empty()) {
        std::cout << "\n  \033[33mNo hay estudios para este paciente.\033[0m\n";
        return;
    }
    
    for (size_t i = 0; i < studies.size(); i++) {
        const auto& study = studies[i];
        std::string date = GetDicomTag(study, "StudyDate");
        std::string desc = GetDicomTag(study, "StudyDescription");
        std::string studyId = GetDicomTag(study, "StudyID");
        std::string modalities = "";
        
        if (study.contains("Series")) {
            modalities = std::to_string(study["Series"].size()) + " series";
        }
        
        std::cout << "\n  \033[33m[" << (i + 1) << "]\033[0m ";
        std::cout << "\033[1m" << (desc.empty() || desc == "N/A" ? "Sin descripción" : desc) << "\033[0m\n";
        if (!date.empty() && date != "N/A") std::cout << "      Fecha: " << date;
        if (!modalities.empty()) std::cout << " | " << modalities;
        std::cout << "\n";
    }
}

void PatientWorklist::PrintStudySeries(const std::string& studyId) const {
    auto series = GetStudySeries(studyId);
    
    std::cout << "\n  Series del estudio:\n";
    
    if (series.is_null() || series.empty()) {
        std::cout << "    \033[33mNo hay series.\033[0m\n";
        return;
    }
    
    for (size_t i = 0; i < series.size(); i++) {
        const auto& s = series[i];
        std::string modality = GetDicomTag(s, "Modality");
        std::string desc = GetDicomTag(s, "SeriesDescription");
        std::string num = GetDicomTag(s, "SeriesNumber");
        
        std::cout << "    \033[33m[" << (i + 1) << "]\033[0m ";
        if (!modality.empty() && modality != "N/A") std::cout << "[" << modality << "] ";
        if (!num.empty() && num != "N/A") std::cout << "#" << num << " ";
        std::cout << (desc.empty() || desc == "N/A" ? "Sin descripción" : desc);
        
        if (s.contains("Instances")) {
            std::cout << " (" << s["Instances"].size() << " imágenes)";
        }
        std::cout << "\n";
    }
}

// ============================================================
// Static helpers
// ============================================================

std::string PatientWorklist::GetDicomTag(const json& obj, const std::string& tag, const std::string& defaultValue) {
    if (obj.is_null()) return defaultValue;
    
    // Try MainDicomTags first (patient/study/series level)
    if (obj.contains("MainDicomTags") && obj["MainDicomTags"].contains(tag)) {
        std::string val = obj["MainDicomTags"][tag].get<std::string>();
        if (!val.empty()) return val;
    }
    
    // Try PatientMainDicomTags (study level)
    if (obj.contains("PatientMainDicomTags") && obj["PatientMainDicomTags"].contains(tag)) {
        std::string val = obj["PatientMainDicomTags"][tag].get<std::string>();
        if (!val.empty()) return val;
    }
    
    // Direct property
    if (obj.contains(tag)) {
        std::string val = obj[tag].get<std::string>();
        if (!val.empty()) return val;
    }
    
    return defaultValue;
}

std::string PatientWorklist::GetPatientId(const json& patient) {
    if (patient.contains("ID")) {
        return patient["ID"].get<std::string>();
    }
    if (patient.contains("OrthancId")) {
        return patient["OrthancId"].get<std::string>();
    }
    return "unknown";
}
