# LeonelOS

A completely new operating system written from scratch, designed with a clean and modular architecture.

## Architecture

```
Bootloader -> Kernel -> API -> Userspace
```

### Project Structure

```
LeonelOS/
├── boot/              # Bootloader and boot assets (NOT customizable)
├── src/
│   ├── kernel/        # Core kernel
│   │   ├── arch/      # Architecture-specific (x86_64)
│   │   ├── memory/    # Physical, virtual memory and heap
│   │   ├── process/   # Scheduler, threads, IPC
│   │   ├── filesystem/# VFS and drivers
│   │   ├── graphics/  # Framebuffer and boot UI
│   │   └── debug/     # Debug and serial console
│   ├── api/           # Public API layer
│   ├── libc/          # C standard library implementation
│   └── userspace/     # Userspace application templates
├── sdk/               # SDK for building LeonelOS applications
├── system/            # System themes, programs, users, config
├── tools/             # Build and utilities tools
├── tests/             # Unit and integration tests
└── docs/              # Documentation
```

## Development Roadmap

1. Project structure and CMake setup
2. UEFI bootloader
3. Kernel entry and framebuffer
4. Memory management
5. Interrupt handling and keyboard input
6. Basic shell
7. Processes and threading
8. Filesystem (VFS + RAMFS)
9. Userspace applications

## Building

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja
```

## Running

```bash
ninja run_qemu
```