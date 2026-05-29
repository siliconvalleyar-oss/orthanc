#!/bin/bash

# Script de ejemplo para ejecutar el visor
# Uso: ./run_example.sh ruta/a/tu/imagen.dcm

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ $# -eq 0 ]; then
    echo -e "${RED}Error: Debes especificar un archivo DICOM${NC}"
    echo -e "${YELLOW}Uso: $0 <archivo_dicom>${NC}"
    exit 1
fi

if [ ! -f "$1" ]; then
    echo -e "${RED}Error: El archivo '$1' no existe${NC}"
    exit 1
fi

if [ ! -f "bin/dicom_viewer" ]; then
    echo -e "${RED}Error: El visor no está compilado. Ejecuta 'make all' primero${NC}"
    exit 1
fi

echo -e "${GREEN}Ejecutando visor con archivo: $1${NC}"
./bin/dicom_viewer "$1"
