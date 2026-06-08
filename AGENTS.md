# Orthanc DICOM Viewer - Project Context

This is a C++17 DICOM medical image viewer that connects to an Orthanc DICOM server.

## Project Structure

- `src/` - Source files (main.cpp, orthanc_client.cpp, patient_worklist.cpp, dicom_editor.cpp, dicom_viewer_sdl.cpp)
- `include/` - Headers (orthanc_client.h, patient_worklist.h, dicom_editor.h, dicom_viewer_sdl.h, json.hpp)
- `bin/` - Compiled executables
- `obj/` - Object files
- `scripts/` - Helper scripts (install_dependencies.sh, link.sh, run_example.sh)
- `dicom/` - Local DICOM files directory

## Build System

- Compiler: g++ (C++17)
- Build tool: GNU Make
- Command: `make all` to build, `make run` to execute
- Dependencies: SDL2, libcurl, DCMTK, nlohmann-json

## Architecture

- **OrthancClient** (orthanc_client.h/cpp) - REST API client for Orthanc server (libcurl-based)
- **PatientWorklist** (patient_worklist.h/cpp) - Patient list management from Orthanc
- **DicomEditor** (dicom_editor.h/cpp) - Metadata tag editor for DICOM patients
- **DicomViewerSDL** (dicom_viewer_sdl.h/cpp) - SDL2-based graphical DICOM image viewer

## Key Details

- Default Orthanc endpoint: `http://localhost:8042`
- DICOM import folder: `dicom/` (scans for .dcm files)
- Viewer controls: Left-click drag = Window/Level, Right-click drag = Pan, Scroll = Zoom
- Terminal-based UI with color output and box-drawing characters

## Development

- Install dependencies: `./scripts/install_dependencies.sh` or `make install-deps`
- Build: `make all`
- Clean: `make clean` (objects only) or `make distclean` (objects + binary)
- Run: `make run`
