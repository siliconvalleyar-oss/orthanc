#ifndef MODALITY_WORKLIST_H
#define MODALITY_WORKLIST_H

#include <string>
#include <vector>
#include "json.hpp"
#include "orthanc_client.h"

using json = nlohmann::json;

class ModalityWorklist {
public:
    ModalityWorklist(OrthancClient& client);

    bool Query(const std::string& patientName = "",
               const std::string& startDate = "",
               const std::string& modality = "");

    size_t GetEntryCount() const { return entries_.size(); }
    const json& GetEntry(size_t index) const;

    std::string GetPatientName(size_t index) const;
    std::string GetPatientId(size_t index) const;
    std::string GetPatientBirthDate(size_t index) const;
    std::string GetPatientSex(size_t index) const;
    std::string GetModality(size_t index) const;
    std::string GetProcedureDescription(size_t index) const;
    std::string GetScheduledDate(size_t index) const;
    std::string GetScheduledTime(size_t index) const;
    std::string GetScheduledStationAETitle(size_t index) const;
    std::string GetRequestedProcedureId(size_t index) const;
    std::string GetScheduledProcedureStepId(size_t index) const;

    void PrintWorklist() const;
    void PrintEntryDetails(size_t index) const;

    std::string GetLastError() const { return lastError_; }

private:
    OrthancClient& client_;
    std::vector<json> entries_;
    std::string lastError_;

    static std::string GetTag(const json& entry, const std::string& tag,
                              const std::string& defaultValue = "");
    static std::string GetScheduledTag(const json& entry, const std::string& tag,
                                       const std::string& defaultValue = "");
};

#endif
