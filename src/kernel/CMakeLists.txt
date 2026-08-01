set(KERNEL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(KERNEL_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(KERNEL_OBJ_DIR ${KERNEL_BINARY_DIR}/obj)

# ---------------------------------------------------------------------------
# Milestone build: only the sources required for the boot screen are compiled.
# The rest (memory, interrupts, scheduler, ...) gets wired in later milestones.
# ---------------------------------------------------------------------------
set(KERNEL_SOURCES
    ${KERNEL_DIR}/kernel.cpp
    ${KERNEL_DIR}/debug.cpp
    ${KERNEL_DIR}/arch/x86_64/io.cpp
    ${KERNEL_DIR}/arch/x86_64/cpu/cpu.cpp
    ${KERNEL_DIR}/arch/x86_64/cpu/gdt.cpp
    ${KERNEL_DIR}/arch/x86_64/cpu/idt.cpp
    ${KERNEL_DIR}/arch/x86_64/interrupts/interrupts.cpp
    ${KERNEL_DIR}/arch/x86_64/interrupts/isr.cpp
    ${KERNEL_DIR}/arch/x86_64/interrupts/irq.cpp
    ${KERNEL_DIR}/arch/x86_64/interrupts/pic.cpp
    ${KERNEL_DIR}/arch/x86_64/drivers/timer/timer.cpp
    ${KERNEL_DIR}/arch/x86_64/drivers/keyboard/keyboard.cpp
    ${KERNEL_DIR}/graphics/bootui/bootui.c
    ${KERNEL_DIR}/graphics/framebuffer/framebuffer.c
)

set(KERNEL_ASM_SOURCES
    ${KERNEL_DIR}/arch/x86_64/start.asm
    ${KERNEL_DIR}/arch/x86_64/interrupts/isrs.asm
)

set(KERNEL_INCLUDE_DIRS
    ${KERNEL_DIR}
    ${KERNEL_DIR}/arch/x86_64
    ${KERNEL_DIR}/arch/x86_64/cpu
    ${KERNEL_DIR}/arch/x86_64/gdt
    ${KERNEL_DIR}/arch/x86_64/idt
    ${KERNEL_DIR}/arch/x86_64/interrupts
    ${KERNEL_DIR}/arch/x86_64/paging
    ${KERNEL_DIR}/arch/x86_64/syscall
    ${KERNEL_DIR}/memory/physical
    ${KERNEL_DIR}/memory/virtual
    ${KERNEL_DIR}/memory/heap
    ${KERNEL_DIR}/process/threads
    ${KERNEL_DIR}/process/scheduler
    ${KERNEL_DIR}/process/ipc
    ${KERNEL_DIR}/filesystem/vfs
    ${KERNEL_DIR}/filesystem/drivers
    ${KERNEL_DIR}/graphics/framebuffer
    ${KERNEL_DIR}/graphics/renderer
    ${KERNEL_DIR}/graphics/bootui
    ${KERNEL_DIR}/arch/x86_64/drivers/timer
    ${KERNEL_DIR}/arch/x86_64/drivers/keyboard
    ${KERNEL_DIR}/debug
)

set(KERNEL_C_FLAGS
    -ffreestanding
    -fno-stack-protector
    -fno-pic
    -fno-pie
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -mno-sse2
    -fno-builtin
    -mcmodel=kernel
    -mgeneral-regs-only
    -fno-omit-frame-pointer
    -ffunction-sections
    -fdata-sections
    -O2
    -g
)

set(KERNEL_CXX_FLAGS
    ${KERNEL_C_FLAGS}
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -std=c++20
)

set(KERNEL_INCLUDES "")
foreach(dir ${KERNEL_INCLUDE_DIRS})
    list(APPEND KERNEL_INCLUDES "-I${dir}")
endforeach()

# Every header change must retrigger compilation (custom commands do not
# scan #includes automatically).
file(GLOB_RECURSE KERNEL_HEADERS ${KERNEL_DIR}/*.h)

set(KERNEL_OBJS "")
foreach(src ${KERNEL_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    set(obj ${KERNEL_OBJ_DIR}/${name}.o)
    list(APPEND KERNEL_OBJS ${obj})
    if(src MATCHES "\\.cpp$")
        set(flags ${KERNEL_CXX_FLAGS})
    else()
        set(flags ${KERNEL_C_FLAGS})
    endif()
    add_custom_command(
        OUTPUT ${obj}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${KERNEL_OBJ_DIR}
        COMMAND ${LEONELOS_CLANG} --target=x86_64-unknown-elf ${flags} ${KERNEL_INCLUDES} -c ${src} -o ${obj}
        DEPENDS ${src} ${KERNEL_HEADERS}
        COMMENT "CC  ${src}"
    )
endforeach()

foreach(src ${KERNEL_ASM_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    set(obj ${KERNEL_OBJ_DIR}/${name}.o)
    list(APPEND KERNEL_OBJS ${obj})
    add_custom_command(
        OUTPUT ${obj}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${KERNEL_OBJ_DIR}
        COMMAND ${LEONELOS_NASM} -f elf64 -g ${src} -o ${obj}
        DEPENDS ${src}
        COMMENT "ASM ${src}"
    )
endforeach()

set(KERNEL_ELF ${KERNEL_BINARY_DIR}/kernel.elf)
add_custom_command(
    OUTPUT ${KERNEL_ELF}
    COMMAND ${LEONELOS_LD} -m elf_x86_64
        -T ${KERNEL_DIR}/linker.ld
        --gc-sections
        --build-id=none
        -o ${KERNEL_ELF} ${KERNEL_OBJS}
    DEPENDS ${KERNEL_OBJS} ${KERNEL_DIR}/linker.ld
    COMMENT "Linking kernel.elf"
)

add_custom_target(kernel DEPENDS ${KERNEL_ELF})

set_target_properties(kernel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
