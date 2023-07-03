# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Embedded/Espressif/frameworks/esp-idf-v4.4.3/components/bootloader/subproject"
  "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader"
  "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader-prefix"
  "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader-prefix/tmp"
  "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader-prefix/src"
  "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Embedded/Espressif/frameworks/BTL_KTVXL/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
