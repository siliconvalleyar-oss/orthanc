---
name: dicom-dev
description: Use when the user asks about DICOM development, Orthanc REST API, DICOM metadata editing, image rendering, or adding new features to the DICOM viewer. Covers coding patterns, API integration, and DICOM concepts used in the project.
---

# DICOM Development Guide

## Orthanc REST API Integration

All communication uses libcurl via `OrthancClient`. Each HTTP method is wrapped:

```cpp
// Pattern for adding new API calls:
// 1. Add public method in include/orthanc_client.h
// 2. Implement in src/orthanc_client.cpp using HttpGet/HttpPut/HttpPost

json OrthancClient::GetSystemInfo() {
    std::string response = HttpGet("/system");
    return json::parse(response);
}
```

### Adding a new endpoint

1. Add method declaration to `OrthancClient` in `include/orthanc_client.h`
2. Implement in `src/orthanc_client.cpp` using the private HTTP helpers
3. Available helpers: `HttpGet`, `HttpGetBinary`, `HttpPut`, `HttpPost`, `HttpPostBinary`
4. All helpers handle JSON parsing, HTTP error codes, and libcurl errors

## DICOM Tag Editing Pattern

Tags are edited via the Orthanc PUT endpoint for patients:

```cpp
// Orthanc expects: { "MainDicomTags": { "PatientName": "New Name", ... } }
json body;
body["MainDicomTags"] = {{"PatientName", "New Name"}};
client.UpdatePatient(patientId, body);
```

### Adding editable tags

1. Add tag name to `DicomEditor::GetEditableTags()` in `src/dicom_editor.cpp`
2. Add validation in `DicomEditor::ValidateTag()`
3. Add description in `DicomEditor::GetTagDescription()`

## Image Rendering Pipeline (SDL2)

```
DICOM file → Download via Orthanc REST → DCMTK parse → Window/Level render → SDL2 display
```

- `DicomViewerSDL::DownloadFromOrthanc()` - fetches instance file
- `DicomViewerSDL::LoadDicomFromOrthanc()` - parses with DCMTK
- `DicomViewerSDL::RenderWithWindowLevel(center, width)` - pixel value mapping
- Supported filters: HistogramEqualize, Smooth (blur), EdgeDetect

## Adding New Features

### New UI menu option
1. Add menu item in `ShowMainMenu()` 
2. Add handler in `main()` switch statement
3. Create function following pattern: `void ShowFeature(OrthancClient& client)`

### New annotation type
1. Define struct in `include/dicom_viewer_sdl.h`
2. Add state variables to `DicomViewerSDL` class
3. Implement render method
4. Handle input in mouse/key event handlers

## DICOM Value Representations (VR) for Tags

- **PatientName (PN)**: `^` delimited (Last^First^Middle)
- **PatientID (LO)**: Alphanumeric string
- **PatientBirthDate (DA)**: `YYYYMMDD` format
- **PatientSex (CS)**: `M`, `F`, or `O`
- **PatientAge (AS)**: 3 digits + unit (`035Y`, `002M`, `003W`, `001D`)
- **PatientWeight (DS)**: Decimal number in kg
