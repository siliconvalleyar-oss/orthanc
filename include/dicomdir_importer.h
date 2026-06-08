#ifndef DICOMDIR_IMPORTER_H
#define DICOMDIR_IMPORTER_H

#include <string>
#include <vector>
#include "json.hpp"
#include "orthanc_client.h"

using json = nlohmann::json;

struct DicomdirEntry {
    std::string patientName;
    std::string patientId;
    std::string studyDate;
    std::string studyDesc;
    std::string modality;
    std::string filePath;
};

class DicomdirImporter {
public:
    DicomdirImporter(OrthancClient& client);

    bool Scan(const std::string& dicomdirPath = "DICOMDIR");
    size_t GetEntryCount() const { return entries_.size(); }
    const DicomdirEntry& GetEntry(size_t index) const;
    void PrintEntries() const;

    int ImportAll();
    int ImportSelected(const std::vector<size_t>& indices);

    std::string GetLastError() const { return lastError_; }

private:
    OrthancClient& client_;
    std::vector<DicomdirEntry> entries_;
    std::string lastError_;

    bool ExtractEntries(const std::string& dicomdirPath);
    bool UploadFile(const std::string& filepath);
};

#endif
