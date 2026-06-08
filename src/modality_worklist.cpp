#include "modality_worklist.h"
#include <iostream>
#include <algorithm>

ModalityWorklist::ModalityWorklist(OrthancClient& client)
    : client_(client)
{
}

bool ModalityWorklist::Query(const std::string& patientName,
                              const std::string& startDate,
                              const std::string& modality)
{
    try {
        json query;
        query["Level"] = "ModalityWorklist";

        json queryParams;
        if (!patientName.empty()) {
            queryParams["PatientName"] = patientName;
        }
        if (!startDate.empty()) {
            queryParams["ScheduledProcedureStepStartDate"] = startDate;
        }
        if (!modality.empty()) {
            queryParams["Modality"] = modality;
        }
        query["Query"] = queryParams;

        auto result = client_.Find(query);

        entries_.clear();

        if (result.is_null()) {
            lastError_ = "No se recibieron resultados";
            return false;
        }

        if (result.is_array()) {
            for (const auto& entry : result) {
                if (!entry.is_null()) {
                    entries_.push_back(entry);
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

const json& ModalityWorklist::GetEntry(size_t index) const {
    static json emptyJson;
    if (index >= entries_.size()) return emptyJson;
    return entries_[index];
}

std::string ModalityWorklist::GetPatientName(size_t index) const {
    return GetTag(GetEntry(index), "PatientName");
}

std::string ModalityWorklist::GetPatientId(size_t index) const {
    return GetTag(GetEntry(index), "PatientID");
}

std::string ModalityWorklist::GetPatientBirthDate(size_t index) const {
    return GetTag(GetEntry(index), "PatientBirthDate");
}

std::string ModalityWorklist::GetPatientSex(size_t index) const {
    return GetTag(GetEntry(index), "PatientSex");
}

std::string ModalityWorklist::GetModality(size_t index) const {
    return GetScheduledTag(GetEntry(index), "Modality");
}

std::string ModalityWorklist::GetProcedureDescription(size_t index) const {
    std::string desc = GetScheduledTag(GetEntry(index), "ScheduledProcedureStepDescription");
    if (desc.empty()) {
        desc = GetTag(GetEntry(index), "RequestedProcedureDescription");
    }
    return desc;
}

std::string ModalityWorklist::GetScheduledDate(size_t index) const {
    return GetScheduledTag(GetEntry(index), "ScheduledProcedureStepStartDate");
}

std::string ModalityWorklist::GetScheduledTime(size_t index) const {
    return GetScheduledTag(GetEntry(index), "ScheduledProcedureStepStartTime");
}

std::string ModalityWorklist::GetScheduledStationAETitle(size_t index) const {
    return GetScheduledTag(GetEntry(index), "ScheduledStationAETitle");
}

std::string ModalityWorklist::GetRequestedProcedureId(size_t index) const {
    return GetTag(GetEntry(index), "RequestedProcedureID");
}

std::string ModalityWorklist::GetScheduledProcedureStepId(size_t index) const {
    return GetScheduledTag(GetEntry(index), "ScheduledProcedureStepID");
}

void ModalityWorklist::PrintWorklist() const {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              MODALITY WORKLIST - PACIENTES PROGRAMADOS          ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n";

    if (entries_.empty()) {
        std::cout << "\n  \033[33mNo se encontraron pacientes programados.\033[0m\n";
        return;
    }

    std::cout << "\n  \033[36m" << entries_.size() << " entrada(s) encontrada(s)\033[0m\n\n";

    for (size_t i = 0; i < entries_.size(); i++) {
        std::string name = GetPatientName(i);
        std::string pid = GetPatientId(i);
        std::string modality = GetModality(i);
        std::string date = GetScheduledDate(i);
        std::string time = GetScheduledTime(i);
        std::string desc = GetProcedureDescription(i);

        std::cout << "  \033[33m[" << (i + 1) << "]\033[0m ";
        std::cout << "\033[1m" << (name.empty() ? "(sin nombre)" : name) << "\033[0m\n";
        std::cout << "      ID: " << (pid.empty() ? "N/A" : pid);
        if (!modality.empty()) std::cout << " | Modalidad: " << modality;
        if (!date.empty()) std::cout << " | Fecha: " << date;
        if (!time.empty() && time.length() >= 4)
            std::cout << " " << time.substr(0, 2) << ":" << time.substr(2, 2);
        std::cout << "\n";
        if (!desc.empty()) {
            std::cout << "      Procedimiento: " << desc << "\n";
        }
    }
}

void ModalityWorklist::PrintEntryDetails(size_t index) const {
    if (index >= entries_.size()) {
        std::cerr << "\033[31m[ERROR] Indice de entrada invalido\033[0m\n";
        return;
    }

    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              DETALLES DE LA ENTRADA - WORKLIST                  ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n";

    std::cout << "\n  \033[1mDatos del paciente:\033[0m\n";
    std::cout << "  \033[1mNombre:\033[0m          " << GetPatientName(index) << "\n";
    std::cout << "  \033[1mID:\033[0m              " << GetPatientId(index) << "\n";
    std::cout << "  \033[1mFecha nacimiento:\033[0m " << GetPatientBirthDate(index) << "\n";
    std::cout << "  \033[1mSexo:\033[0m            " << GetPatientSex(index) << "\n";

    std::cout << "\n  \033[1mProcedimiento programado:\033[0m\n";
    std::cout << "  \033[1mModalidad:\033[0m       " << GetModality(index) << "\n";
    std::cout << "  \033[1mDescripcion:\033[0m     " << GetProcedureDescription(index) << "\n";
    std::cout << "  \033[1mFecha:\033[0m           " << GetScheduledDate(index) << "\n";
    std::string time = GetScheduledTime(index);
    if (!time.empty() && time.length() >= 4) {
        std::cout << "  \033[1mHora:\033[0m            " << time.substr(0, 2) << ":" << time.substr(2, 2) << "\n";
    }
    std::cout << "  \033[1mEstacion AET:\033[0m    " << GetScheduledStationAETitle(index) << "\n";
    std::cout << "  \033[1mSPS ID:\033[0m          " << GetScheduledProcedureStepId(index) << "\n";
    std::cout << "  \033[1mRP ID:\033[0m           " << GetRequestedProcedureId(index) << "\n";
}

std::string ModalityWorklist::GetTag(const json& entry, const std::string& tag,
                                      const std::string& defaultValue)
{
    if (entry.is_null()) return defaultValue;
    if (entry.contains("MainDicomTags") &&
        entry["MainDicomTags"].contains(tag)) {
        std::string val = entry["MainDicomTags"][tag].get<std::string>();
        if (!val.empty()) return val;
    }
    if (entry.contains(tag)) {
        std::string val = entry[tag].get<std::string>();
        if (!val.empty()) return val;
    }
    return defaultValue;
}

std::string ModalityWorklist::GetScheduledTag(const json& entry,
                                               const std::string& tag,
                                               const std::string& defaultValue)
{
    if (entry.is_null()) return defaultValue;
    try {
        if (entry.contains("MainDicomTags") &&
            entry["MainDicomTags"].contains("ScheduledProcedureStepSequence") &&
            entry["MainDicomTags"]["ScheduledProcedureStepSequence"].is_array() &&
            !entry["MainDicomTags"]["ScheduledProcedureStepSequence"].empty())
        {
            const auto& sps = entry["MainDicomTags"]["ScheduledProcedureStepSequence"][0];
            if (sps.contains(tag)) {
                std::string val = sps[tag].get<std::string>();
                if (!val.empty()) return val;
            }
        }
    } catch (...) {}
    return defaultValue;
}
