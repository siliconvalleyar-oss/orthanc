# Instalar dependencias del sistema
./install_dependencies.sh

# Compilar Orthanc y Stone (solo primera vez, toma varios minutos)
make compile-all

# Compilar el visor
make all

# Ejecutar con una imagen DICOM de ejemplo
make run FILE=/mnt/disk/src/orthanc/dicom/dcm_01/ct.dcm
