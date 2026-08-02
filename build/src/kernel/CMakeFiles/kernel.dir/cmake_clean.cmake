file(REMOVE_RECURSE
  "CMakeFiles/kernel"
  "kernel.elf"
  "obj/bootui.o"
  "obj/cpu.o"
  "obj/debug.o"
  "obj/framebuffer.o"
  "obj/gdt.o"
  "obj/heap.o"
  "obj/idt.o"
  "obj/interrupts.o"
  "obj/io.o"
  "obj/irq.o"
  "obj/isr.o"
  "obj/isrs.o"
  "obj/kernel.o"
  "obj/keyboard.o"
  "obj/physical.o"
  "obj/pic.o"
  "obj/start.o"
  "obj/timer.o"
  "obj/virtual.o"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/kernel.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
