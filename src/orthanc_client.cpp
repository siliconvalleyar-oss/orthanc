#include "orthanc_client.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdio>

// ============================================================
// Callback helpers for libcurl
// ============================================================

static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

static size_t WriteVectorCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::vector<char>* vec = static_cast<std::vector<char>*>(userp);
    vec->insert(vec->end(), static_cast<char*>(contents), static_cast<char*>(contents) + totalSize);
    return totalSize;
}

// ============================================================
// OrthancClient implementation
// ============================================================

OrthancClient::OrthancClient(const std::string& baseUrl)
    : baseUrl_(baseUrl)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

OrthancClient::~OrthancClient() {
    curl_global_cleanup();
}

bool OrthancClient::CheckConnection() {
    try {
        auto info = GetSystemInfo();
        return !info.is_null() && info.contains("Version");
    } catch (...) {
        return false;
    }
}

json OrthancClient::GetSystemInfo() {
    std::string response = HttpGet("/system");
    if (response.empty()) return nullptr;
    return json::parse(response);
}

std::vector<std::string> OrthancClient::GetPatientIds() {
    std::string response = HttpGet("/patients");
    if (response.empty()) return {};
    
    auto ids = json::parse(response);
    std::vector<std::string> result;
    for (const auto& id : ids) {
        result.push_back(id.get<std::string>());
    }
    return result;
}

json OrthancClient::GetPatient(const std::string& patientId) {
    std::string response = HttpGet("/patients/" + patientId);
    if (response.empty()) return nullptr;
    return json::parse(response);
}

json OrthancClient::GetPatientStudies(const std::string& patientId) {
    std::string response = HttpGet("/patients/" + patientId + "/studies");
    if (response.empty()) return nullptr;
    return json::parse(response);
}

json OrthancClient::GetStudy(const std::string& studyId) {
    std::string response = HttpGet("/studies/" + studyId);
    if (response.empty()) return nullptr;
    return json::parse(response);
}

json OrthancClient::GetStudySeries(const std::string& studyId) {
    std::string response = HttpGet("/studies/" + studyId + "/series");
    if (response.empty()) return nullptr;
    return json::parse(response);
}

json OrthancClient::GetSeries(const std::string& seriesId) {
    std::string response = HttpGet("/series/" + seriesId);
    if (response.empty()) return nullptr;
    return json::parse(response);
}

json OrthancClient::GetSeriesInstances(const std::string& seriesId) {
    std::string response = HttpGet("/series/" + seriesId + "/instances");
    if (response.empty()) return nullptr;
    return json::parse(response);
}

json OrthancClient::GetInstance(const std::string& instanceId) {
    std::string response = HttpGet("/instances/" + instanceId);
    if (response.empty()) return nullptr;
    return json::parse(response);
}

std::string OrthancClient::GetInstanceFile(const std::string& instanceId) {
    return HttpGet("/instances/" + instanceId + "/file");
}

std::vector<char> OrthancClient::GetInstancePreview(const std::string& instanceId) {
    return HttpGetBinary("/instances/" + instanceId + "/preview");
}

bool OrthancClient::UpdatePatient(const std::string& patientId, const json& tags) {
    try {
        auto result = HttpPut("/patients/" + patientId, tags);
        return !result.is_null();
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

json OrthancClient::UploadInstance(const std::string& filepath) {
    // Read file
    FILE* f = fopen(filepath.c_str(), "rb");
    if (!f) {
        lastError_ = "No se pudo abrir el archivo: " + filepath;
        return nullptr;
    }
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    std::vector<char> data(fsize);
    size_t bytesRead = fread(data.data(), 1, fsize, f);
    fclose(f);
    (void)bytesRead; // Suppress unused warning
    
    return HttpPostBinary("/instances", data);
}

json OrthancClient::Find(const json& query) {
    return HttpPost("/tools/find", query);
}

// ============================================================
// Private HTTP methods
// ============================================================

std::string OrthancClient::HttpGet(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";
    
    std::string url = baseUrl_ + endpoint;
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (res != CURLE_OK) {
        lastError_ = curl_easy_strerror(res);
        response.clear();
    } else if (httpCode >= 400) {
        lastError_ = "HTTP " + std::to_string(httpCode) + ": " + 
                     (response.empty() ? "Error del servidor" : response);
        response.clear();
    }
    
    curl_easy_cleanup(curl);
    return response;
}

std::vector<char> OrthancClient::HttpGetBinary(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    
    std::string url = baseUrl_ + endpoint;
    std::vector<char> response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteVectorCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (res != CURLE_OK) {
        lastError_ = curl_easy_strerror(res);
        response.clear();
    } else if (httpCode >= 400) {
        lastError_ = "HTTP " + std::to_string(httpCode) + ": Error del servidor";
        response.clear();
    }
    
    curl_easy_cleanup(curl);
    return response;
}

json OrthancClient::HttpPut(const std::string& endpoint, const json& data) {
    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;
    
    std::string url = baseUrl_ + endpoint;
    std::string jsonStr = data.dump();
    std::string response;
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonStr.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (res != CURLE_OK) {
        lastError_ = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return nullptr;
    } else if (httpCode >= 400) {
        lastError_ = "HTTP " + std::to_string(httpCode) + ": " + 
                     (response.empty() ? "Error del servidor" : response);
        curl_easy_cleanup(curl);
        return nullptr;
    }
    
    curl_easy_cleanup(curl);
    
    try {
        if (response.empty()) return json::object();
        return json::parse(response);
    } catch (const json::parse_error& e) {
        lastError_ = std::string(e.what());
        return nullptr;
    }
}

json OrthancClient::HttpPost(const std::string& endpoint, const json& data) {
    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;
    
    std::string url = baseUrl_ + endpoint;
    std::string jsonStr = data.dump();
    std::string response;
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonStr.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (res != CURLE_OK) {
        lastError_ = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return nullptr;
    } else if (httpCode >= 400) {
        lastError_ = "HTTP " + std::to_string(httpCode) + ": " + 
                     (response.empty() ? "Error del servidor" : response);
        curl_easy_cleanup(curl);
        return nullptr;
    }
    
    curl_easy_cleanup(curl);
    
    try {
        if (response.empty()) return json::object();
        return json::parse(response);
    } catch (const json::parse_error& e) {
        lastError_ = std::string(e.what());
        return nullptr;
    }
}

json OrthancClient::HttpPostBinary(const std::string& endpoint, const std::vector<char>& data) {
    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;
    
    std::string url = baseUrl_ + endpoint;
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    if (res != CURLE_OK) {
        lastError_ = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return nullptr;
    } else if (httpCode >= 400) {
        lastError_ = "HTTP " + std::to_string(httpCode) + ": " + 
                     (response.empty() ? "Error del servidor" : response);
        curl_easy_cleanup(curl);
        return nullptr;
    }
    
    curl_easy_cleanup(curl);
    
    try {
        if (response.empty()) return json::object();
        return json::parse(response);
    } catch (const json::parse_error& e) {
        lastError_ = std::string(e.what());
        return nullptr;
    }
}
