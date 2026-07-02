# Procesamiento Distribuido de Datos

Este proyecto implementa soluciones de **procesamiento paralelo y distribuido de datos** en **C++20**, orientadas al tratamiento eficiente de grandes conjuntos de datos en formato CSV. El repositorio contiene la práctica de **memoria compartida** con dos modos de procesamiento: compartido (`computeShared`) y distribuido (`computeDistributed`), ambos construidos con **CMake** y apoyados en la biblioteca auxiliar `libDomain`.

## 🚀 Características Principales

- **Procesamiento Paralelo y Distribuido**: Implementa dos enfoques de cómputo paralelo — memoria compartida y procesamiento distribuido — para el tratamiento eficiente de datos a gran escala.
- **Arquitectura Modular**: Separación clara entre los ejecutables (`computeShared`, `computeDistributed`) y la biblioteca de dominio `libDomain`, facilitando el mantenimiento y la reutilización del código.
- **Carga Genérica de CSV**: El módulo `CSVLoader.h` en la raíz del repositorio proporciona una utilidad reutilizable para la lectura de datasets en formato CSV.
- **Despliegue en Clúster**: Incluye el script `deploy_cluster.sh` y un fichero `config` para facilitar el despliegue y configuración en entornos de clúster.
- **Compilación con CMake**: Soporte para configuraciones **Debug** y **Release** mediante un flujo de compilación sencillo y reproducible.
- **Compatibilidad con C++20**: Preparado para compilarse con **GCC 14**, garantizando compatibilidad con el estándar moderno del lenguaje.

## ⚙️ Requisitos

Antes de compilar el proyecto, instala las dependencias necesarias:

```bash
sudo apt install build-essential git cmake libglm-dev g++-14 gcc-14
```

Después, configura **GCC 14** como compilador principal para asegurar compatibilidad con el estándar C++20:

```bash
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 10
```

## 📂 Preparación de Datos

El proyecto espera un archivo CSV externo como entrada. Coloca el dataset en una ruta accesible antes de ejecutar, por ejemplo:

```bash
~/data/aac/dataset.csv
```

El programa está preparado para trabajar con volúmenes de datos considerables (p. ej., archivos del tipo `300k.csv`), por lo que se recomienda usar la build en modo **Release** para evaluar el rendimiento real.

## 🛠️ Compilación

Sitúate en la carpeta de la práctica antes de compilar:

```bash
cd Practica1_memoria_compartida
```

### Build de desarrollo

Para compilar en modo **Debug**:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8
```

### Build optimizada

Para compilar en modo **Release**:

```bash
mkdir release && cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
```

## ▶️ Ejecución

Una vez compilado, lanza el ejecutable indicando la ruta al fichero CSV:

```bash
# Modo memoria compartida
./computeShared/computeShared ~/data/aac/dataset.csv

# Modo distribuido
./computeDistributed/computeDistributed ~/data/aac/dataset.csv
```

## 🖧 Despliegue en Clúster

El repositorio incluye utilidades para desplegar el procesamiento distribuido en un entorno de clúster:

- **`config`**: Fichero de configuración del clúster (hosts, parámetros de red, etc.).
- **`deploy_cluster.sh`**: Script de despliegue automático. Úsalo tras configurar el fichero `config`:

```bash
./deploy_cluster.sh
```

## 📦 Estructura del Proyecto

```
Procesamiento-Distribuido-de-Datos/
├── CSVLoader.h                         # Módulo genérico de carga de CSV
├── config                              # Configuración del clúster
├── deploy_cluster.sh                   # Script de despliegue distribuido
├── README.md                           # Este fichero
└── Practica1_memoria_compartida/
    ├── CMakeLists.txt                  # Configuración principal de CMake
    ├── computeShared/                  # Ejecutable de memoria compartida
    ├── computeDistributed/             # Ejecutable de procesamiento distribuido
    ├── libDomain/                      # Biblioteca auxiliar con la lógica de dominio
    ├── build/                          # Carpeta para builds de desarrollo (Debug)
    └── release/                        # Carpeta para builds optimizadas (Release)
```

## 🧰 Tecnologías Utilizadas

| Tecnología | Rol en el proyecto |
|---|---|
| **C++20** | Lenguaje principal, con soporte recomendado a través de GCC 14 |
| **CMake** | Sistema de configuración y generación de builds |
| **GCC/G++ 14** | Compiladores recomendados para el estándar C++20 |
| **MPI / Sockets** | Comunicación entre nodos en el modo distribuido |
| **CSV** | Formato de entrada de datos del programa |

## 🧪 Uso Previsto

Este proyecto está orientado a prácticas de **computación de altas prestaciones** y **procesamiento paralelo y distribuido**, en el contexto de asignaturas de arquitectura de computadores, sistemas distribuidos o algoritmos paralelos. El diseño dual (memoria compartida + distribución en clúster) permite comparar el rendimiento y la escalabilidad de ambos enfoques sobre un mismo dataset.

***

*Desarrollado como práctica de procesamiento paralelo y distribuido de datos en C++ con CMake, memoria compartida y despliegue en clúster.*
