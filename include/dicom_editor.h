#ifndef DICOM_EDITOR_H
#define DICOM_EDITOR_H

#include <string>
#include <map>
#include "json.hpp"
#include "orthanc_client.h"

using json = nlohmann::json;

/**
 * @brief Editor de metadatos DICOM
 * 
 * Permite modificar los tags DICOM de pacientes almacenados en Orthanc,
 * incluyendo PatientName, PatientID, PatientBirthDate y PatientSex.
 * Los cambios se envían al servidor Orthanc mediante PUT /patients/{id}.
 */
class DicomEditor {
public:
    DicomEditor(OrthancClient& client);

    /**
     * @brief Obtiene los tags editables de un paciente
     * @return Mapa de tag -> valor actual
     */
    std::map<std::string, std::string> GetPatientTags(const std::string& patientId);

    /**
     * @brief Modifica un tag específico de un paciente
     * @param patientId ID de Orthanc del paciente
     * @param tag Nombre del tag DICOM (ej: "PatientName")
     * @param value Nuevo valor
     * @return true si la operación fue exitosa
     */
    bool SetTag(const std::string& patientId, const std::string& tag, const std::string& value);

    /**
     * @brief Modifica múltiples tags de un paciente de una sola vez
     * @param patientId ID de Orthanc del paciente
     * @param tags Mapa de tag -> valor
     * @return true si la operación fue exitosa
     */
    bool SetTags(const std::string& patientId, const std::map<std::string, std::string>& tags);

    /**
     * @brief Obtiene el último mensaje de error
     */
    std::string GetLastError() const { return lastError_; }

    /**
     * @brief Lista de tags DICOM editables para pacientes
     */
    static std::vector<std::string> GetEditableTags();

    /**
     * @brief Valida un valor para un tag DICOM
     * @param tag Nombre del tag
     * @param value Valor a validar
     * @return Mensaje de error vacío si es válido
     */
    static std::string ValidateTag(const std::string& tag, const std::string& value);

    /**
     * @brief Obtiene un nombre descriptivo para un tag
     */
    static std::string GetTagDescription(const std::string& tag);

private:
    OrthancClient& client_;
    std::string lastError_;
};

#endif // DICOM_EDITOR_H
