#ifndef ORTHANC_CLIENT_H
#define ORTHANC_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "json.hpp"

using json = nlohmann::json;

/**
 * @brief Cliente REST para comunicarse con el servidor Orthanc
 * 
 * Proporciona métodos para interactuar con la API REST de Orthanc:
 * consulta de pacientes, estudios, series, instancias y modificación de metadatos.
 */
class OrthancClient {
public:
    OrthancClient(const std::string& baseUrl = "http://localhost:8042");
    ~OrthancClient();

    // ========== Estado del servidor ==========
    /** Verifica si el servidor Orthanc está accesible */
    bool CheckConnection();
    
    /** Obtiene información del sistema Orthanc */
    json GetSystemInfo();

    // ========== Pacientes ==========
    /** Obtiene lista de IDs de todos los pacientes */
    std::vector<std::string> GetPatientIds();
    
    /** Obtiene datos completos de un paciente por su ID */
    json GetPatient(const std::string& patientId);
    
    /** Obtiene los estudios de un paciente */
    json GetPatientStudies(const std::string& patientId);

    // ========== Estudios ==========
    /** Obtiene datos de un estudio */
    json GetStudy(const std::string& studyId);
    
    /** Obtiene las series de un estudio */
    json GetStudySeries(const std::string& studyId);

    // ========== Series ==========
    /** Obtiene datos de una serie */
    json GetSeries(const std::string& seriesId);
    
    /** Obtiene las instancias (imágenes) de una serie */
    json GetSeriesInstances(const std::string& seriesId);

    // ========== Instancias ==========
    /** Obtiene datos de una instancia */
    json GetInstance(const std::string& instanceId);
    
    /** Obtiene el archivo DICOM original de una instancia */
    std::string GetInstanceFile(const std::string& instanceId);
    
    /** Obtiene el preview (thumbnail) de una instancia */
    std::vector<char> GetInstancePreview(const std::string& instanceId);

    // ========== Modificación de datos ==========
    /** Modifica los tags DICOM de un paciente */
    bool UpdatePatient(const std::string& patientId, const json& tags);
    
    /** Sube un archivo DICOM al servidor */
    json UploadInstance(const std::string& filepath);

    // ========== Búsqueda ==========
    /** Realiza una búsqueda C-FIND avanzada */
    json Find(const json& query);

    // ========== Utilidad ==========
    /** Obtiene el último error */
    std::string GetLastError() const { return lastError_; }

    /** Obtiene la URL base del servidor Orthanc */
    std::string GetBaseUrl() const { return baseUrl_; }

private:
    std::string baseUrl_;
    std::string lastError_;
    
    /** Ejecuta una petición GET y devuelve la respuesta como string */
    std::string HttpGet(const std::string& endpoint);
    
    /** Ejecuta una petición GET y devuelve datos binarios */
    std::vector<char> HttpGetBinary(const std::string& endpoint);
    
    /** Ejecuta una petición PUT con cuerpo JSON */
    json HttpPut(const std::string& endpoint, const json& data);
    
    /** Ejecuta una petición POST con cuerpo JSON */
    json HttpPost(const std::string& endpoint, const json& data);
    
    /** Ejecuta una petición POST con datos binarios (subida de archivo) */
    json HttpPostBinary(const std::string& endpoint, const std::vector<char>& data);
};

#endif // ORTHANC_CLIENT_H
