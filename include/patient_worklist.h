#ifndef PATIENT_WORKLIST_H
#define PATIENT_WORKLIST_H

#include <string>
#include <vector>
#include <functional>
#include "json.hpp"
#include "orthanc_client.h"

using json = nlohmann::json;

/**
 * @brief Worklist de pacientes - lista de trabajo para visualización DICOM
 * 
 * Gestiona la obtención y presentación de pacientes desde Orthanc,
 * permitiendo navegar por pacientes, estudios, series e instancias.
 */
class PatientWorklist {
public:
    PatientWorklist(OrthancClient& client);

    /** Obtiene todos los pacientes desde Orthanc */
    bool Refresh();

    /** Obtiene el número de pacientes en la worklist */
    size_t GetPatientCount() const { return patients_.size(); }

    /** Obtiene un paciente por índice */
    const json& GetPatient(size_t index) const;

    /** Obtiene el nombre formateado de un paciente */
    std::string GetPatientName(size_t index) const;

    /** Obtiene información resumida de un paciente */
    std::string GetPatientSummary(size_t index) const;

    /** Obtiene los estudios de un paciente */
    json GetPatientStudies(size_t index) const;

    /** Obtiene las series de un estudio */
    json GetStudySeries(const std::string& studyId) const;

    /** Obtiene las instancias de una serie */
    json GetSeriesInstances(const std::string& seriesId) const;

    /** Busca pacientes por nombre */
    std::vector<size_t> FindPatients(const std::string& query) const;

    /** Imprime la worklist en consola */
    void PrintWorklist() const;

    /** Imprime detalles de un paciente */
    void PrintPatientDetails(size_t index) const;

    /** Imprime los estudios de un paciente */
    void PrintPatientStudies(size_t index) const;

    /** Imprime las series de un estudio */
    void PrintStudySeries(const std::string& studyId) const;

private:
    OrthancClient& client_;
    std::vector<json> patients_;

    /** Extrae un tag DICOM de forma segura */
    static std::string GetDicomTag(const json& patient, const std::string& tag, const std::string& defaultValue = "N/A");

    /** Obtiene el patientId del objeto */
    static std::string GetPatientId(const json& patient);
};

#endif // PATIENT_WORKLIST_H
