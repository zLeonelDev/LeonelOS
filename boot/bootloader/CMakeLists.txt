set(BL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BL_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(BL_OBJ_DIR ${BL_BINARY_DIR}/obj)

set(BL_SOURCES
    main.c
    graphics.c
    bootui.c
    assets.c
    kernel_loader.c
    memory.c
    efilib.c
)

# Compiled for the Windows x64 ABI (matches the firmware calling convention),
# linked into a PE32+ image by lld-link.
set(BL_COMPILE_FLAGS
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
    -fshort-wchar
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -mno-sse2
    -fno-omit-frame-pointer
    -ffunction-sections
    -fdata-sections
    -O2
    -g
)

# Every header change must retrigger compilation (custom commands do not
# scan #includes automatically).
file(GLOB BL_HEADERS ${BL_DIR}/*.h)

set(BL_OBJS "")
foreach(src ${BL_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    set(obj ${BL_OBJ_DIR}/${name}.obj)
    list(APPEND BL_OBJS ${obj})
    add_custom_command(
        OUTPUT ${obj}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BL_OBJ_DIR}
        COMMAND ${LEONELOS_CLANG} --target=x86_64-pc-windows-msvc ${BL_COMPILE_FLAGS} -I${BL_DIR} -c ${BL_DIR}/${src} -o ${obj}
        DEPENDS ${BL_DIR}/${src} ${BL_HEADERS}
        COMMENT "CC  ${src}"
    )
endforeach()

set(BL_EFI ${BL_BINARY_DIR}/BOOTX64.EFI)
add_custom_command(
    OUTPUT ${BL_EFI}
    COMMAND ${LEONELOS_LLD_LINK}
        /entry:efi_main
        /subsystem:efi_application
        /nodefaultlib
        /dll
        /out:${BL_EFI}
        ${BL_OBJS}
    DEPENDS ${BL_OBJS}
    COMMENT "Linking BOOTX64.EFI"
)

add_custom_target(bootloader DEPENDS ${BL_EFI})
