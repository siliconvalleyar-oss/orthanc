#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Instalador de dependencias para Orthanc DICOM Viewer${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"

# Detectar sistema operativo
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo -e "${YELLOW}Sistema detectado: Linux${NC}"
    
    # Detectar distribución
    if [ -f /etc/debian_version ]; then
        echo -e "${YELLOW}Distribución Debian/Ubuntu detectada${NC}"
        
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            mercurial \
            python3 \
            libsdl2-dev \
            libgl1-mesa-dev \
            libglu1-mesa-dev \
            libglew-dev \
            libboost-all-dev \
            libdcmtk-dev \
            libpugixml-dev \
            libjpeg-dev \
            libpng-dev \
            libtiff-dev \
            libcurl4-openssl-dev
            
    elif [ -f /etc/redhat-release ]; then
        echo -e "${YELLOW}Distribución RedHat/Fedora detectada${NC}"
        
        sudo yum install -y \
            gcc-c++ \
            cmake \
            mercurial \
            python3 \
            SDL2-devel \
            mesa-libGL-devel \
            glew-devel \
            boost-devel \
            dcmtk-devel \
            pugixml-devel \
            libjpeg-turbo-devel \
            libpng-devel \
            libtiff-devel \
            libcurl-devel
    else
        echo -e "${RED}Distribución no soportada para instalación automática${NC}"
        exit 1
    fi
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo -e "${YELLOW}Sistema detectado: macOS${NC}"
    
    # Verificar si Homebrew está instalado
    if ! command -v brew &> /dev/null; then
        echo -e "${RED}Homebrew no está instalado. Instalando...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    brew install \
        cmake \
        mercurial \
        sdl2 \
        glew \
        boost \
        dcmtk \
        pugixml \
        jpeg-turbo \
        libpng \
        libtiff \
        curl
        
else
    echo -e "${RED}Sistema operativo no soportado: $OSTYPE${NC}"
    exit 1
fi

echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Dependencias del sistema instaladas correctamente${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${YELLOW}Próximos pasos:${NC}"
echo "  1. Ejecutar: make compile-all (para compilar Orthanc y Stone)"
echo "  2. Ejecutar: make all (para compilar el visor)"
echo "  3. Ejecutar: make run FILE=tu_imagen.dcm"
echo ""
