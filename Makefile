# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS = -lcurl -lpthread 

# SDL2 flags
SDL2_CFLAGS = $(shell sdl2-config --cflags 2>/dev/null || echo "-I/usr/include/SDL2 -D_REENTRANT")
SDL2_LIBS = $(shell sdl2-config --libs 2>/dev/null || echo "-lSDL2")

# DCMTK flags
DCMTK_CFLAGS = -I/usr/include/dcmtk
DCMTK_LIBS = -ldcmimgle -ldcmimage -ldcmdata -ldcmnet -loflog -lofstd

# Directorios
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INC_DIR = include

# Archivos fuente y objetos
SOURCES = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/orthanc_client.cpp \
	$(SRC_DIR)/patient_worklist.cpp \
	$(SRC_DIR)/dicom_editor.cpp \
	$(SRC_DIR)/dicom_viewer_sdl.cpp

OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/dicom_worklist

# Colores para output
GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[1;33m
CYAN = \033[0;36m
NC = \033[0m

# Regla principal
all: $(TARGET)

# Regla para crear el ejecutable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	@echo "$(YELLOW)Enlazando objetos...$(NC)"
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(SDL2_LIBS) $(DCMTK_LIBS)
	@echo "$(GREEN)✓ Ejecutable creado: $(TARGET)$(NC)"

# Regla para compilar objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "$(YELLOW)Compilando $<...$(NC)"
	$(CXX) $(CXXFLAGS) $(SDL2_CFLAGS) $(DCMTK_CFLAGS) -I$(INC_DIR) -c $< -o $@

# Limpiar archivos objeto
clean:
	@echo "$(YELLOW)Limpiando archivos objeto...$(NC)"
	@rm -rf $(OBJ_DIR)/*.o
	@echo "$(GREEN)✓ Objetos limpiados$(NC)"

# Limpiar completamente (objetos y ejecutable)
distclean: clean
	@echo "$(YELLOW)Limpiando ejecutable...$(NC)"
	@rm -rf $(TARGET)
	@rm -rf bin/dicom_viewer  # Legacy binary
	@echo "$(GREEN)✓ Ejecutable eliminado$(NC)"

# Instalar dependencias (solo Linux)
install-deps:
	@echo "$(YELLOW)Instalando dependencias del sistema...$(NC)"
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && \
		sudo apt-get install -y build-essential cmake \
		libcurl4-openssl-dev nlohmann-json3-dev \
		libsdl2-dev libdcmtk-dev; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum install -y gcc-c++ cmake libcurl-devel; \
		echo "$(YELLOW)Descargar nlohmann-json manualmente desde github.com/nlohmann/json$(NC)"; \
	else \
		echo "$(RED)Sistema no soportado para instalación automática$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ Dependencias instaladas$(NC)"

# Ejecutar
run: $(TARGET)
	./$(TARGET)

# Ayuda
help:
	@echo "════════════════════════════════════════════════════════"
	@echo "Orthanc DICOM Worklist & Editor - Comandos disponibles:"
	@echo "════════════════════════════════════════════════════════"
	@echo ""
	@echo "  $(GREEN)make all$(NC)            Compilar el proyecto"
	@echo "  $(GREEN)make clean$(NC)          Limpiar archivos objeto"
	@echo "  $(GREEN)make distclean$(NC)      Limpiar todo (objetos + ejecutable)"
	@echo "  $(GREEN)make run$(NC)            Ejecutar la aplicación"
	@echo "  $(GREEN)make install-deps$(NC)   Instalar dependencias del sistema"
	@echo "  $(GREEN)make help$(NC)           Mostrar esta ayuda"
	@echo ""
	@echo "════════════════════════════════════════════════════════"

.PHONY: all clean distclean run help install-deps
