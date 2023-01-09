# Generated Cmake Pico project file
cmake_minimum_required(VERSION 3.13)

set(TARGET "usb-2-232-kbd")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# I experimented with USB PIO support but currently it does not support Dual hosting, just host and device. which is useless to me. 
# Dual host support should be added in future so I'll keep this for reference
# https://github.com/bgni/minimal-pico-tinyusb-pio-project
#
#---------------------------------------------------------------------
#      set(PICO_TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)
#      #
#      # Set path to Pico-PIO-USB
#      #
#      if (NOT PICO_PIO_USB_PATH)
#          if(DEFINED ENV{PICO_PIO_USB_PATH})
#              set(PICO_PIO_USB_PATH $ENV{PICO_PIO_USB_PATH})
#              message("Setting PICO_PIO_USB_PATH from environment ('${PICO_PIO_USB_PATH}')")
#          else()
#              set(PICO_PIO_USB_PATH ${PICO_TINYUSB_PATH}/hw/mcu/raspberry_pi/Pico-PIO-USB)
#              message("Setting PICO_PIO_USB_PATH from git submodule path ('${PICO_PIO_USB_PATH}')")
#          endif()
#      else()
#          message("PICO_PIO_USB_PATH is ('${PICO_PIO_USB_PATH}')")
#      endif()
#
#      #
#      # Setup paths to Pico-PIO-USB
#      #
#      set(PICO_PIO_USB_SRC "${PICO_PIO_USB_PATH}/src")
#
#      # use tinyusb implementation
#      target_compile_definitions(usb-2-232-kbd PRIVATE PIO_USB_USE_TINYUSB)
#---------------------------------------------------------------------



# Set path to Pico-SDK
set(PICO_SDK_PATH "C:/Pico/pico-sdk")

# For TinyUSB
set(FAMILY rp2040)

# Pull in Raspberry Pi Pico SDK (must be before project)
# Include pico_sdk_import.cmake from pico-sdk (instead of copying)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)


#set( CMAKE_CXX_FLAGS "-Wall -Wextra")
#set( CMAKE_C_FLAGS "-Wall -Wextra")
set( CMAKE_C_FLAGS_RELEASE "-O2" )
set( CMAKE_CXX_FLAGS_RELEASE "-O2" )

project(usb-2-232-kbd C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
#add_executable(usb-2-232 ${CMAKE_CURRENT_LIST_DIR})
add_executable(usb-2-232-kbd)


target_sources(usb-2-232-kbd PRIVATE
  usb-2-232-kbd.c
  include/hid_app.c
  include/serial.c
  include/utils.c
  include/core_1.c
  include/ps2dev.c
  include/hid_con.c
 )



# Make sure TinyUSB can find tusb_config.h
target_include_directories(usb-2-232-kbd PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_host
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(usb-2-232-kbd pico_stdlib hardware_flash hardware_sync tinyusb_board tinyusb_host pico_multicore)

include_directories(include/)
link_directories(include/)

pico_set_program_name(${TARGET} "usb-2-232-kbd")
pico_set_program_description(${TARGET} "USB to serial mouse & AT Keyboard adaptor.")
pico_set_program_version(${TARGET} "1.0")

pico_enable_stdio_usb(usb-2-232-kbd 0)
pico_enable_stdio_uart(usb-2-232-kbd 1)

pico_add_extra_outputs(usb-2-232-kbd)
