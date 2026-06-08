---
name: orthanc-build
description: Use when the user asks about building, compiling, cleaning, running, or installing dependencies for the Orthanc DICOM Viewer project. Covers all Makefile targets and build troubleshooting.
---

# Orthanc DICOM Viewer - Build System

## Quick Commands

| Command | Description |
|---------|-------------|
| `make all` | Compile the full project |
| `make run` | Build (if needed) and run the application |
| `make clean` | Remove object files only |
| `make distclean` | Remove objects + executable |
| `make install-deps` | Install system dependencies |
| `make help` | Show help with all targets |

## Dependencies

- **Compiler**: g++ with C++17 support
- **Libraries**: SDL2, libcurl, DCMTK (dcmimgle, dcmimage, dcmdata, dcmnet, oflog, ofstd), nlohmann-json
- **Install**: `./scripts/install_dependencies.sh` or `make install-deps`

## Build Details

- Source: `src/*.cpp` → Object: `obj/*.o` → Binary: `bin/dicom_worklist`
- Flags: `-std=c++17 -Wall -Wextra -O2 -pthread`
- Link: `-lcurl -lpthread -lSDL2 -ldcmimgle -ldcmimage -ldcmdata -ldcmnet -loflog -lofstd`

## Troubleshooting

- If SDL2 is missing: install `libsdl2-dev`
- If libcurl is missing: install `libcurl4-openssl-dev`
- If DCMTK is missing: install `libdcmtk-dev`
- If nlohmann/json.hpp is not found: it's included in `include/json.hpp`
