# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/m1220/esp/v5.2/esp-idf/components/bootloader/subproject"
  "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader"
  "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix"
  "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix/tmp"
  "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix/src"
  "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/m1220/Documents/ESP32-C3/BME280/BME280_RESP/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
