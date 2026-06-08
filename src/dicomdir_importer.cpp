#include "dicomdir_importer.h"
#include <iostream>
#include <algorithm>
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcdicdir.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcfilefo.h>

DicomdirImporter::DicomdirImporter(OrthancClient& client)
    : client_(client)
{
}

bool DicomdirImporter::Scan(const std::string& dicomdirPath) {
    entries_.clear();

    DcmDicomDir dicomdir(dicomdirPath.c_str());
    if (dicomdir.error().bad()) {
        lastError_ = "No se pudo abrir DICOMDIR: " + dicomdirPath;
        return false;
    }

    return ExtractEntries(dicomdirPath);
}

bool DicomdirImporter::ExtractEntries(const std::string& dicomdirPath) {
    DcmDicomDir dicomdir(dicomdirPath.c_str());
    DcmDirectoryRecord& root = dicomdir.getRootRecord();

    // Determine base directory
    std::string baseDir = dicomdirPath;
    size_t sep = baseDir.find_last_of("/\\");
    if (sep != std::string::npos) {
        baseDir = baseDir.substr(0, sep);
    } else {
        baseDir = ".";
    }

    // Iterate PATIENT records
    DcmDirectoryRecord* patRec = root.getSub(0);
    while (patRec) {
        DicomdirEntry entry;

        OFString val;
        if (patRec->findAndGetOFString(DCM_PatientName, val).good())
            entry.patientName = val.c_str();
        if (patRec->findAndGetOFString(DCM_PatientID, val).good())
            entry.patientId = val.c_str();

        // Iterate STUDY records
        DcmDirectoryRecord* studyRec = patRec->getSub(0);
        while (studyRec) {
            if (studyRec->findAndGetOFString(DCM_StudyDate, val).good())
                entry.studyDate = val.c_str();
            if (studyRec->findAndGetOFString(DCM_StudyDescription, val).good())
                entry.studyDesc = val.c_str();

            // Iterate SERIES records
            DcmDirectoryRecord* seriesRec = studyRec->getSub(0);
            while (seriesRec) {
                if (seriesRec->findAndGetOFString(DCM_Modality, val).good())
                    entry.modality = val.c_str();

                // Iterate IMAGE records
                DcmDirectoryRecord* imgRec = seriesRec->getSub(0);
                while (imgRec) {
                    OFString pathVal;
                    if (imgRec->findAndGetOFString(DCM_ReferencedFileID, pathVal).good()) {
                        std::string relPath = pathVal.c_str();
                        std::replace(relPath.begin(), relPath.end(), '\\', '/');
                        entry.filePath = baseDir + "/" + relPath;

                        DicomdirEntry e = entry;
                        entries_.push_back(e);
                    }
                    imgRec = seriesRec->nextSub(imgRec);
                }
                seriesRec = studyRec->nextSub(seriesRec);
            }
            studyRec = patRec->nextSub(studyRec);
        }
        patRec = root.nextSub(patRec);
    }

    return !entries_.empty();
}

const DicomdirEntry& DicomdirImporter::GetEntry(size_t index) const {
    static DicomdirEntry empty;
    if (index >= entries_.size()) return empty;
    return entries_[index];
}

void DicomdirImporter::PrintEntries() const {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              IMPORTAR DICOMDIR - ARCHIVOS ENCONTRADOS            ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════╝\n";

    if (entries_.empty()) {
        std::cout << "\n  \033[33mNo se encontraron entradas en el DICOMDIR.\033[0m\n";
        return;
    }

    std::cout << "\n  \033[36m" << entries_.size() << " archivo(s) encontrado(s)\033[0m\n\n";

    for (size_t i = 0; i < entries_.size(); i++) {
        const auto& e = entries_[i];
        std::cout << "  \033[33m[" << (i + 1) << "]\033[0m ";
        std::cout << "\033[1m" << (e.patientName.empty() ? "(sin nombre)" : e.patientName) << "\033[0m\n";
        std::cout << "      ID: " << (e.patientId.empty() ? "N/A" : e.patientId);
        if (!e.modality.empty()) std::cout << " | Mod: " << e.modality;
        if (!e.studyDate.empty()) std::cout << " | Fecha: " << e.studyDate;
        std::cout << "\n";
        if (!e.studyDesc.empty()) std::cout << "      Estudio: " << e.studyDesc << "\n";
        std::cout << "      Archivo: " << e.filePath << "\n";
    }
}

int DicomdirImporter::ImportAll() {
    int success = 0;
    for (size_t i = 0; i < entries_.size(); i++) {
        if (UploadFile(entries_[i].filePath)) {
            success++;
        }
    }
    return success;
}

int DicomdirImporter::ImportSelected(const std::vector<size_t>& indices) {
    int success = 0;
    for (size_t idx : indices) {
        if (idx < entries_.size() && UploadFile(entries_[idx].filePath)) {
            success++;
        }
    }
    return success;
}

bool DicomdirImporter::UploadFile(const std::string& filepath) {
    try {
        auto result = client_.UploadInstance(filepath);
        if (!result.is_null()) {
            std::cout << "    \033[32mOK\033[0m " << filepath << "\n";
            return true;
        }
        std::cout << "    \033[31mFAIL\033[0m " << filepath << " (" << client_.GetLastError() << ")\n";
        return false;
    } catch (const std::exception& e) {
        std::cout << "    \033[31mFAIL\033[0m " << filepath << " (" << e.what() << ")\n";
        return false;
    }
}
