/**
 * Orthanc DICOM Worklist & Metadata Editor
 * 
 * Interfaz de terminal interactiva que permite:
 * - Ver lista de pacientes (worklist) desde Orthanc
 * - Navegar por estudios, series e instancias
 * - Editar metadatos DICOM de pacientes
 * - Visualizar imágenes DICOM con visor gráfico SDL2
 * - Escanear e importar archivos DICOM locales
 */

#include "orthanc_client.h"
#include "patient_worklist.h"
#include "dicom_editor.h"
#include "dicom_viewer_sdl.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>

// ============================================================
// Utility functions
// ============================================================

void ClearScreen() {
    std::cout << "\033[2J\033[1;1H";
}

void PrintHeader() {
    std::cout << "\033[1;36m";
    std::cout << "  \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n";
    std::cout << "  \xe2\x95\x91           ORTHANC DICOM VIEWER - WORKLIST & EDITOR              \xe2\x95\x91\n";
    std::cout << "  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\033[0m\n";
}

void PrintSeparator() {
    std::cout << "  \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n";
}

void PressEnterToContinue() {
    std::cout << "\n  \033[33mPresione Enter para continuar...\033[0m";
    std::string dummy;
    std::getline(std::cin, dummy);
}

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// ============================================================
// Menu: Main
// ============================================================

void ShowMainMenu() {
    std::cout << "\n";
    std::cout << "  \033[1mMENU PRINCIPAL\033[0m\n";
    std::cout << "\n";
    std::cout << "  \033[33m[1]\033[0m Lista de Pacientes (Worklist)\n";
    std::cout << "  \033[33m[2]\033[0m Buscar Pacientes\n";
    std::cout << "  \033[33m[3]\033[0m Escanear carpeta DICOM local\n";
    std::cout << "  \033[33m[4]\033[0m Estado del servidor Orthanc\n";
    std::cout << "  \033[33m[0]\033[0m Salir\n";
    std::cout << "\n  \033[1mSeleccione una opcion: \033[0m";
}

// ============================================================
// Functions to view instances in SDL viewer
// ============================================================

void ShowInstancesForSeries(OrthancClient& client, const std::string& patientName,
                            const json& instances) {
    ClearScreen();
    PrintHeader();
    std::cout << "\n  \033[1mInstancias de la serie\033[0m\n";

    if (instances.is_null() || instances.empty()) {
        std::cout << "\n  \033[33mNo hay instancias en esta serie.\033[0m\n";
        PressEnterToContinue();
        return;
    }

    for (size_t i = 0; i < instances.size(); i++) {
        std::cout << "  \033[33m[" << (i + 1) << "]\033[0m Instancia: ";
        if (instances[i].contains("ID")) {
            std::cout << instances[i]["ID"].get<std::string>();
        }
        std::cout << "\n";
    }
    std::cout << "\n  \033[32mTotal: " << instances.size() << " imagenes\033[0m\n";

    std::cout << "\n  \033[33mVer alguna imagen? (numero o 0 para volver): \033[0m";
    std::string viewChoice;
    std::getline(std::cin, viewChoice);

    int viewNum = 0;
    try { viewNum = std::stoi(viewChoice); } catch (...) {}

    if (viewNum > 0 && viewNum <= (int)instances.size()) {
        std::string instId;
        std::string instDesc;
        if (instances[viewNum - 1].contains("ID")) {
            instId = instances[viewNum - 1]["ID"].get<std::string>();
        }
        if (instances[viewNum - 1].contains("MainDicomTags") &&
            instances[viewNum - 1]["MainDicomTags"].contains("InstanceNumber")) {
            instDesc = instances[viewNum - 1]["MainDicomTags"]["InstanceNumber"].get<std::string>();
        } else {
            instDesc = "Imagen #" + std::to_string(viewNum);
        }

        ClearScreen();
        PrintHeader();
        std::cout << "\n  \033[36mAbriendo visor grafico DICOM...\033[0m\n";
        std::cout << "  Paciente: " << patientName << "\n";
        std::cout << "  Instancia: " << instDesc << "\n\n";
        std::cout << "  \033[33mControles del visor:\033[0m\n";
        std::cout << "    Click Izquierdo + arrastrar = Ajustar Window/Level\n";
        std::cout << "    Click Derecho + arrastrar   = Panoramica (Pan)\n";
        std::cout << "    Rueda del raton             = Zoom\n";
        std::cout << "    Tecla R                     = Resetear vista\n";
        std::cout << "    Tecla W                     = Auto Window/Level\n";
        std::cout << "    Tecla ESC / Q               = Cerrar visor\n";
        std::cout << "\n  \033[33m(Puede tomar un momento descargar y procesar la imagen)\033[0m\n\n";

        DicomViewerSDL viewer(client, instId,
            "Orthanc DICOM - " + patientName + " - " + instDesc);
        viewer.Run();

        ClearScreen();
        PrintHeader();
        std::cout << "\n  \033[32mVisor cerrado.\033[0m\n";
    }
}

// ============================================================
// Menu: Patient List / Worklist
// ============================================================

void ShowPatientActions(OrthancClient& client, PatientWorklist& worklist, size_t patientIndex) {
    if (patientIndex >= worklist.GetPatientCount()) return;
    
    auto& patient = worklist.GetPatient(patientIndex);
    std::string patientId;
    if (patient.contains("ID")) patientId = patient["ID"].get<std::string>();
    std::string patientName = worklist.GetPatientName(patientIndex);
    
    DicomEditor editor(client);
    
    bool exitPatient = false;
    while (!exitPatient) {
        ClearScreen();
        PrintHeader();
        
        std::cout << "\n  \033[1mPaciente:\033[0m " << patientName << "\n";
        PrintSeparator();
        
        std::cout << "\n  \033[1mACCIONES DISPONIBLES\033[0m\n";
        std::cout << "\n";
        std::cout << "  \033[33m[1]\033[0m Ver detalles del paciente\n";
        std::cout << "  \033[33m[2]\033[0m Ver estudios del paciente (con visor de imagenes)\n";
        std::cout << "  \033[33m[3]\033[0m Editar metadatos DICOM\n";
        std::cout << "  \033[33m[0]\033[0m Volver al menu principal\n";
        std::cout << "\n  \033[1mSeleccione una opcion: \033[0m";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            ClearScreen();
            PrintHeader();
            worklist.PrintPatientDetails(patientIndex);
            PressEnterToContinue();
        }
        else if (choice == "2") {
            ClearScreen();
            PrintHeader();
            std::cout << "\n  \033[1mPaciente:\033[0m " << patientName << "\n";
            worklist.PrintPatientStudies(patientIndex);
            
            auto studies = worklist.GetPatientStudies(patientIndex);
            if (!studies.is_null() && !studies.empty()) {
                std::cout << "\n  \033[33mVer series de algun estudio? (numero o 0 para salir): \033[0m";
                std::string studyChoice;
                std::getline(std::cin, studyChoice);
                
                int studyNum = 0;
                try { studyNum = std::stoi(studyChoice); } catch (...) {}
                
                if (studyNum > 0 && studyNum <= (int)studies.size()) {
                    std::string studyId;
                    if (studies[studyNum - 1].contains("ID")) {
                        studyId = studies[studyNum - 1]["ID"].get<std::string>();
                    }
                    
                    ClearScreen();
                    PrintHeader();
                    std::cout << "\n  \033[1mEstudio #" << studyNum << "\033[0m\n";
                    worklist.PrintStudySeries(studyId);
                    
                    auto series = worklist.GetStudySeries(studyId);
                    if (!series.is_null() && !series.empty()) {
                        std::cout << "\n  \033[33mVer instancias de alguna serie? (numero o 0 para salir): \033[0m";
                        std::string seriesChoice;
                        std::getline(std::cin, seriesChoice);
                        
                        int seriesNum = 0;
                        try { seriesNum = std::stoi(seriesChoice); } catch (...) {}
                        
                        if (seriesNum > 0 && seriesNum <= (int)series.size()) {
                            std::string seriesId;
                            if (series[seriesNum - 1].contains("ID")) {
                                seriesId = series[seriesNum - 1]["ID"].get<std::string>();
                            }
                            
                            auto instances = worklist.GetSeriesInstances(seriesId);
                            ShowInstancesForSeries(client, patientName, instances);
                        }
                    }
                    PressEnterToContinue();
                }
            }
            PressEnterToContinue();
        }
        else if (choice == "3") {
            ClearScreen();
            PrintHeader();
            
            std::cout << "\n  \033[1mEDITOR DE METADATOS DICOM\033[0m\n";
            std::cout << "  Paciente: " << patientName << "\n";
            PrintSeparator();
            
            auto tags = editor.GetPatientTags(patientId);
            
            std::cout << "\n  \033[1mTags actuales:\033[0m\n\n";
            int tagIndex = 0;
            std::vector<std::string> tagNames;
            for (const auto& [tag, value] : tags) {
                tagNames.push_back(tag);
                tagIndex++;
                std::cout << "  \033[33m[" << tagIndex << "]\033[0m "
                          << DicomEditor::GetTagDescription(tag) << "\n";
                std::cout << "     Tag: " << tag << "\n";
                std::cout << "     Valor actual: \033[36m" << (value.empty() ? "(vacio)" : value) << "\033[0m\n\n";
            }
            
            std::cout << "  \033[33m[0]\033[0m Volver\n";
            std::cout << "\n  \033[1mSeleccione el tag a editar (numero): \033[0m";
            
            std::string tagChoice;
            std::getline(std::cin, tagChoice);
            
            int tagNum = 0;
            try { tagNum = std::stoi(tagChoice); } catch (...) {}
            
            if (tagNum > 0 && tagNum <= (int)tagNames.size()) {
                std::string selectedTag = tagNames[tagNum - 1];
                std::string currentValue = tags[selectedTag];
                
                std::cout << "\n  Editando: \033[1m" << DicomEditor::GetTagDescription(selectedTag) << "\033[0m\n";
                std::cout << "  Tag DICOM: " << selectedTag << "\n";
                std::cout << "  Valor actual: \033[36m" << (currentValue.empty() ? "(vacio)" : currentValue) << "\033[0m\n";
                std::cout << "\n  \033[33mNuevo valor (Enter para mantener actual): \033[0m";
                
                std::string newValue;
                std::getline(std::cin, newValue);
                newValue = Trim(newValue);
                
                if (!newValue.empty()) {
                    std::string validationError = DicomEditor::ValidateTag(selectedTag, newValue);
                    if (!validationError.empty()) {
                        std::cout << "\n  \033[31m[ERROR] " << validationError << "\033[0m\n";
                    } else {
                        std::cout << "\n  \033[33mConfirma el cambio?\033[0m\n";
                        std::cout << "    " << DicomEditor::GetTagDescription(selectedTag) << ": ";
                        std::cout << "\033[36m" << (currentValue.empty() ? "(vacio)" : currentValue) << "\033[0m";
                        std::cout << " -> \033[33m" << newValue << "\033[0m\n";
                        std::cout << "  \033[33mGuardar? (s/N): \033[0m";
                        
                        std::string confirm;
                        std::getline(std::cin, confirm);
                        confirm = ToUpper(Trim(confirm));
                        
                        if (confirm == "S" || confirm == "SI") {
                            if (editor.SetTag(patientId, selectedTag, newValue)) {
                                std::cout << "\n  \033[32mTag actualizado exitosamente\033[0m\n";
                            } else {
                                std::cout << "\n  \033[31m[ERROR] " << editor.GetLastError() << "\033[0m\n";
                            }
                        } else {
                            std::cout << "\n  \033[33mCambio cancelado.\033[0m\n";
                        }
                    }
                } else {
                    std::cout << "\n  \033[33mValor sin cambios.\033[0m\n";
                }
                PressEnterToContinue();
            }
        }
        else if (choice == "0") {
            exitPatient = true;
        }
    }
}

// ============================================================
// Menu: Worklist
// ============================================================

void ShowWorklist(OrthancClient& client, PatientWorklist& worklist) {
    ClearScreen();
    PrintHeader();
    
    worklist.PrintWorklist();
    
    if (worklist.GetPatientCount() > 0) {
        std::cout << "\n  \033[33mSeleccione un paciente para ver acciones (numero o 0 para volver): \033[0m";
        std::string choice;
        std::getline(std::cin, choice);
        
        int patientNum = 0;
        try { patientNum = std::stoi(choice); } catch (...) {}
        
        if (patientNum > 0 && patientNum <= (int)worklist.GetPatientCount()) {
            ShowPatientActions(client, worklist, patientNum - 1);
        }
    } else {
        PressEnterToContinue();
    }
}

// ============================================================
// Menu: Search Patients
// ============================================================

void ShowSearch(OrthancClient& client, PatientWorklist& worklist) {
    ClearScreen();
    PrintHeader();
    
    std::cout << "\n  \033[1mBUSCAR PACIENTES\033[0m\n";
    std::cout << "\n  Introduce un termino de busqueda (nombre o ID):\n";
    std::cout << "  \033[33m> \033[0m";
    
    std::string query;
    std::getline(std::cin, query);
    query = Trim(query);
    
    if (!query.empty()) {
        auto results = worklist.FindPatients(query);
        
        ClearScreen();
        PrintHeader();
        
        std::cout << "\n  \033[1mResultados para: \033[33m" << query << "\033[0m\n";
        PrintSeparator();
        
        if (results.empty()) {
            std::cout << "\n  \033[33mNo se encontraron pacientes con ese criterio.\033[0m\n";
        } else {
            std::cout << "\n  \033[36m" << results.size() << " paciente(s) encontrado(s):\033[0m\n\n";
            
            for (size_t i = 0; i < results.size(); i++) {
                size_t idx = results[i];
                std::cout << "  \033[33m[" << (i + 1) << "]\033[0m "
                          << worklist.GetPatientSummary(idx) << "\n";
            }
            
            std::cout << "\n  \033[33mSeleccione un paciente (numero o 0 para volver): \033[0m";
            std::string choice;
            std::getline(std::cin, choice);
            
            int patientNum = 0;
            try { patientNum = std::stoi(choice); } catch (...) {}
            
            if (patientNum > 0 && patientNum <= (int)results.size()) {
                ShowPatientActions(client, worklist, results[patientNum - 1]);
            }
        }
    }
    
    PressEnterToContinue();
}

// ============================================================
// Menu: Scan Local DICOM Folder
// ============================================================

void ShowScanFolder(OrthancClient& client) {
    ClearScreen();
    PrintHeader();
    
    std::cout << "\n  \033[1mESCANEAR CARPETA DICOM LOCAL\033[0m\n";
    std::cout << "\n  Escaneando carpeta \033[36mdicom/\033[0m...\n";
    
    DIR* dir = opendir("dicom");
    if (!dir) {
        std::cout << "\n  \033[31m[ERROR] No se encontro la carpeta 'dicom/'\033[0m\n";
        PressEnterToContinue();
        return;
    }
    
    std::vector<std::string> dcmFiles;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".dcm") {
            dcmFiles.push_back(filename);
        }
    }
    closedir(dir);
    
    std::cout << "\n  \033[36mSe encontraron " << dcmFiles.size() << " archivos .dcm\033[0m\n";
    
    if (!dcmFiles.empty()) {
        std::cout << "\n  Archivos encontrados:\n";
        for (size_t i = 0; i < dcmFiles.size() && i < 10; i++) {
            std::cout << "    \033[33m[" << (i + 1) << "]\033[0m " << dcmFiles[i] << "\n";
        }
        if (dcmFiles.size() > 10) {
            std::cout << "    ... y " << (dcmFiles.size() - 10) << " mas\n";
        }
        
        std::cout << "\n  \033[33mImportar todos los archivos a Orthanc? (s/N): \033[0m";
        std::string confirm;
        std::getline(std::cin, confirm);
        confirm = ToUpper(Trim(confirm));
        
        if (confirm == "S" || confirm == "SI") {
            std::cout << "\n  Importando archivos...\n";
            
            int successCount = 0;
            int failCount = 0;
            
            for (const auto& filename : dcmFiles) {
                std::string filepath = "dicom/" + filename;
                std::cout << "    Subiendo " << filename << "... ";
                
                try {
                    auto result = client.UploadInstance(filepath);
                    if (!result.is_null()) {
                        std::cout << "\033[32m\xe2\x9c\x93\033[0m\n";
                        successCount++;
                    } else {
                        std::cout << "\033[31m\xe2\x9c\x97 (" << client.GetLastError() << ")\033[0m\n";
                        failCount++;
                    }
                } catch (const std::exception& e) {
                    std::cout << "\033[31m\xe2\x9c\x97 (" << e.what() << ")\033[0m\n";
                    failCount++;
                }
            }
            
            std::cout << "\n  \033[32mImportacion completada:\033[0m\n";
            std::cout << "    Exitosos: " << successCount << "\n";
            std::cout << "    Fallidos: " << failCount << "\n";
        } else {
            std::cout << "\n  \033[33mImportacion cancelada.\033[0m\n";
        }
    }
    
    PressEnterToContinue();
}

// ============================================================
// Menu: System Status
// ============================================================

void ShowSystemStatus(OrthancClient& client) {
    ClearScreen();
    PrintHeader();
    
    std::cout << "\n  \033[1mESTADO DEL SERVIDOR ORTHANC\033[0m\n";
    PrintSeparator();
    
    bool connected = client.CheckConnection();
    
    if (connected) {
        std::cout << "\n  \033[32mConexion establecida\033[0m\n";
        
        try {
            auto info = client.GetSystemInfo();
            if (!info.is_null()) {
                std::cout << "\n  \033[1mInformacion del servidor:\033[0m\n";
                if (info.contains("Name"))
                    std::cout << "    Nombre: " << info["Name"].get<std::string>() << "\n";
                if (info.contains("Version"))
                    std::cout << "    Version: " << info["Version"].get<std::string>() << "\n";
                if (info.contains("DicomAet"))
                    std::cout << "    DICOM AET: " << info["DicomAet"].get<std::string>() << "\n";
                if (info.contains("DicomPort"))
                    std::cout << "    DICOM Port: " << info["DicomPort"] << "\n";
                if (info.contains("HttpPort"))
                    std::cout << "    HTTP Port: " << info["HttpPort"] << "\n";
                if (info.contains("DatabaseVersion"))
                    std::cout << "    DB Version: " << info["DatabaseVersion"] << "\n";
            }
        } catch (...) {
            std::cout << "\n  \033[31mError al obtener informacion del servidor\033[0m\n";
        }
    } else {
        std::cout << "\n  \033[31mNo se pudo conectar al servidor Orthanc\033[0m\n";
        std::cout << "\n  Asegurese de que Orthanc esta corriendo en:\n";
        std::cout << "    \033[36mhttp://localhost:8042\033[0m\n";
    }
    
    PressEnterToContinue();
}

// ============================================================
// Main
// ============================================================

int main() {
    OrthancClient client("http://localhost:8042");
    PatientWorklist worklist(client);
    
    if (client.CheckConnection()) {
        worklist.Refresh();
    }
    
    bool running = true;
    
    while (running) {
        ClearScreen();
        PrintHeader();
        
        bool connected = client.CheckConnection();
        if (connected) {
            std::cout << "\033[32m  Conectado a Orthanc (" << worklist.GetPatientCount() << " pacientes)\033[0m\n";
        } else {
            std::cout << "\033[31m  Orthanc NO disponible\033[0m\n";
        }
        
        ShowMainMenu();
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            if (connected) {
                worklist.Refresh();
                ShowWorklist(client, worklist);
            } else {
                std::cout << "\n  \033[31m[ERROR] No hay conexion con Orthanc\033[0m\n";
                PressEnterToContinue();
            }
        }
        else if (choice == "2") {
            if (connected) {
                worklist.Refresh();
                ShowSearch(client, worklist);
            } else {
                std::cout << "\n  \033[31m[ERROR] No hay conexion con Orthanc\033[0m\n";
                PressEnterToContinue();
            }
        }
        else if (choice == "3") {
            ShowScanFolder(client);
        }
        else if (choice == "4") {
            ShowSystemStatus(client);
        }
        else if (choice == "0") {
            running = false;
            ClearScreen();
            std::cout << "\n  \033[1;36mHasta luego!\033[0m\n\n";
        }
        else {
            std::cout << "\n  \033[31mOpcion invalida. Intente de nuevo.\033[0m\n";
            PressEnterToContinue();
        }
    }
    
    return 0;
}
