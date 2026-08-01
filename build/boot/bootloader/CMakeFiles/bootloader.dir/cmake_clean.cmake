file(REMOVE_RECURSE
  "BOOTX64.EFI"
  "CMakeFiles/bootloader"
  "obj/assets.obj"
  "obj/bootui.obj"
  "obj/efilib.obj"
  "obj/graphics.obj"
  "obj/kernel_loader.obj"
  "obj/main.obj"
  "obj/memory.obj"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/bootloader.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
