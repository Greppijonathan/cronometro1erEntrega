# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v5.3.1/components/bootloader/subproject"
  "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader"
  "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix"
  "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix/tmp"
  "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix/src"
  "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Sistemas embebidos/Cursos/Sistemas operativos de tiempo real/cronometro1erEntrega/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
