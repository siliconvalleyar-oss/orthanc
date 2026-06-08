---
name: orthanc-reference
description: Use when the user asks about the Orthanc DICOM Viewer project structure, architecture, classes, or how the code is organized. Provides comprehensive reference for the codebase.
---

# Orthanc DICOM Viewer - Reference

## Project Overview

C++17 terminal + SDL2 DICOM medical image viewer with Orthanc REST API integration.

## Source Files

- **src/main.cpp** - Entry point, terminal UI menus, patient navigation, study/series/instance browser
- **src/orthanc_client.cpp** - REST client using libcurl (GET/PUT/POST to Orthanc API)
- **src/patient_worklist.cpp** - Patient list management, sorting, searching, DICOM tag extraction
- **src/dicom_editor.cpp** - DICOM metadata tag validation and modification via Orthanc API
- **src/dicom_viewer_sdl.cpp** - SDL2 graphical viewer with Window/Level, zoom, pan, annotations

## Header Files

- **include/json.hpp** - nlohmann/json single-header library (included in repo)

## Key Classes

### OrthancClient
- Constructor: `OrthancClient(baseUrl = "http://localhost:8042")`
- Methods: CheckConnection, GetSystemInfo, GetPatientIds, GetPatient, GetPatientStudies,
  GetStudy, GetStudySeries, GetSeries, GetSeriesInstances, GetInstance, GetInstanceFile,
  GetInstancePreview, UpdatePatient, UploadInstance, Find

### PatientWorklist
- Constructor: `PatientWorklist(OrthancClient&)`
- Methods: Refresh, GetPatientCount, GetPatient, GetPatientName, GetPatientSummary,
  GetPatientStudies, GetStudySeries, GetSeriesInstances, FindPatients, PrintWorklist,
  PrintPatientDetails, PrintPatientStudies, PrintStudySeries

### DicomEditor
- Constructor: `DicomEditor(OrthancClient&)`
- Methods: GetPatientTags, SetTag, SetTags, GetEditableTags, ValidateTag, GetTagDescription
- Editable tags: PatientName, PatientID, PatientBirthDate, PatientSex, PatientAge,
  PatientWeight, PatientSize, PatientAddress, PatientTelephoneNumbers, OtherPatientIDs

### DicomViewerSDL
- Constructor: `DicomViewerSDL(OrthancClient&, instanceId, title)`
- Series constructor: `DicomViewerSDL(OrthancClient&, vector<string> instanceIds, startIndex, title)`
- Controls: Left-drag = WL, Right-drag = Pan, Scroll = Zoom, R = Reset, W = Auto WL
- Annotations: Distance, Angle, ROI, Text
- Filters: Histogram Equalize, Smooth, Edge Detect

## API Endpoints Used

- GET `/patients`, `/patients/{id}`, `/patients/{id}/studies`
- GET `/studies/{id}`, `/studies/{id}/series`
- GET `/series/{id}`, `/series/{id}/instances`
- GET `/instances/{id}`, `/instances/{id}/file`, `/instances/{id}/preview`
- PUT `/patients/{id}` (metadata update)
- POST `/instances` (upload), `/tools/find` (C-FIND)
- GET `/system` (server status)
