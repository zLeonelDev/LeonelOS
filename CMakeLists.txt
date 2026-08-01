cmake_minimum_required(VERSION 3.20)
project(LeonelOS VERSION 0.1.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Toolchain (adjustable via -DLEONELOS_<TOOL>=...)
# ---------------------------------------------------------------------------
set(LEONELOS_CLANG      "C:/msys64/clang64/bin/clang.exe"       CACHE FILEPATH "clang for kernel + bootloader")
set(LEONELOS_LD         "C:/msys64/mingw64/bin/ld.lld.exe"       CACHE FILEPATH "ld.lld ELF linker")
set(LEONELOS_NASM       "C:/msys64/mingw64/bin/nasm.exe"        CACHE FILEPATH "nasm assembler")
set(LEONELOS_LLD_LINK   "C:/msys64/mingw64/bin/lld-link.exe"    CACHE FILEPATH "lld-link COFF linker")
set(LEONELOS_TRUNCATE   "C:/msys64/usr/bin/truncate.exe"        CACHE FILEPATH "truncate")
set(LEONELOS_MFORMAT    "C:/msys64/mingw64/bin/mformat.exe"     CACHE FILEPATH "mtools mformat")
set(LEONELOS_MMD        "C:/msys64/mingw64/bin/mmd.exe"         CACHE FILEPATH "mtools mmd")
set(LEONELOS_MCOPY      "C:/msys64/mingw64/bin/mcopy.exe"       CACHE FILEPATH "mtools mcopy")
set(LEONELOS_XORRISO    "C:/msys64/usr/bin/xorriso.exe"         CACHE FILEPATH "xorriso")
set(LEONELOS_QEMU       "C:/msys64/mingw64/bin/qemu-system-x86_64.exe" CACHE FILEPATH "qemu")
set(LEONELOS_OVMF       "C:/msys64/mingw64/share/qemu/edk2-x86_64-code.fd" CACHE FILEPATH "OVMF firmware")

# Host tools (tools/image_tool) use the mingw-w64 compiler.
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "C:/msys64/mingw64/bin/gcc.exe")
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "C:/msys64/mingw64/bin/g++.exe")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/kernel)
add_subdirectory(boot/bootloader)
add_subdirectory(tools)

# SDK and tests are not part of the boot milestone.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit)
    add_subdirectory(tests)
endif()
file(GLOB _SDK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdk/*.c ${CMAKE_CURRENT_SOURCE_DIR}/sdk/*.cpp)
if(_SDK_SOURCES)
    add_subdirectory(sdk)
endif()

# ---------------------------------------------------------------------------
# Boot image: FAT ESP + hybrid ISO
# ---------------------------------------------------------------------------
set(BOOTLOADER_EFI ${CMAKE_BINARY_DIR}/boot/bootloader/BOOTX64.EFI)
set(KERNEL_ELF     ${CMAKE_BINARY_DIR}/src/kernel/kernel.elf)
set(ASSET_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/boot/assets)
set(ESP_IMG        ${CMAKE_BINARY_DIR}/leonelos-esp.img)
set(ISO_IMG        ${CMAKE_BINARY_DIR}/leonelos.iso)

set(LEONELOS_ENV ${CMAKE_COMMAND} -E env MTOOLS_SKIP_CHECK=1)

add_custom_command(
    OUTPUT ${ESP_IMG}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${ESP_IMG}
    COMMAND ${LEONELOS_TRUNCATE} -s 16M ${ESP_IMG}
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MFORMAT} -i ${ESP_IMG} ::
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MMD} -i ${ESP_IMG} ::/EFI
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MMD} -i ${ESP_IMG} ::/EFI/BOOT
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MMD} -i ${ESP_IMG} ::/boot
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MMD} -i ${ESP_IMG} ::/boot/assets
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MCOPY} -i ${ESP_IMG} ${BOOTLOADER_EFI} ::/EFI/BOOT/BOOTX64.EFI
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MCOPY} -i ${ESP_IMG} ${KERNEL_ELF} ::/kernel.elf
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MCOPY} -i ${ESP_IMG} ${ASSET_DIR}/leonelos.rgba ::/boot/assets/leonelos.rgba
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MCOPY} -i ${ESP_IMG} ${ASSET_DIR}/icon.rgba ::/boot/assets/icon.rgba
    COMMAND ${LEONELOS_ENV} ${LEONELOS_MCOPY} -i ${ESP_IMG} ${ASSET_DIR}/spinner.rgba ::/boot/assets/spinner.rgba
    DEPENDS ${BOOTLOADER_EFI} ${KERNEL_ELF}
    COMMENT "Creating FAT ESP image"
)

add_custom_command(
    OUTPUT ${ISO_IMG}
    COMMAND ${LEONELOS_XORRISO} -as mkisofs -o ${ISO_IMG}
        -volid "LEONELOS"
        -append_partition 2 0xEF ${ESP_IMG}
        -e --interval:appended_partition_2:all::
        -no-emul-boot
        -iso_mbr_part_type 0xEF
        -isohybrid-gpt-basdat
        -no-pad
    DEPENDS ${ESP_IMG}
    COMMENT "Creating hybrid ISO"
)

add_custom_target(leonelos_image DEPENDS ${ISO_IMG} ${ESP_IMG})
add_dependencies(leonelos_image bootloader kernel)
add_custom_target(build_all DEPENDS leonelos_image)

add_custom_target(run_qemu
    COMMAND ${LEONELOS_QEMU} -drive if=pflash,format=raw,unit=0,readonly=on,file=${LEONELOS_OVMF}
        -m 512M -smp 2 -serial stdio -cdrom ${ISO_IMG}
    DEPENDS leonelos_image
    COMMENT "Running LeonelOS in QEMU"
)
