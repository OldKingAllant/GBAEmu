cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_COMPILER "clang++")

set(COMPILE_FLAGS "-Wall")
list(APPEND COMPILE_FLAGS "-O3")
list(APPEND COMPILE_FLAGS "-g")

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(FILES "${DIR}/audio_device/AudioDevice.cpp")
list(APPEND FILES "${DIR}/audio_device/sdl/SdlAudioDevice.cpp")
list(APPEND FILES "${DIR}/config/Config.cpp")
list(APPEND FILES "${DIR}/debugger/Debugger.cpp")
list(APPEND FILES "${DIR}/debugger/DisassembleTHUMB.cpp")
list(APPEND FILES "${DIR}/debugger/DebugWindow.cpp")
list(APPEND FILES "${DIR}/debugger/DisassembleARM.cpp")
list(APPEND FILES "${DIR}/emu/Emulator.cpp")
list(APPEND FILES "${DIR}/ImGui/imgui.cpp")
list(APPEND FILES "${DIR}/ImGui/imgui_draw.cpp")
list(APPEND FILES "${DIR}/ImGui/imgui_tables.cpp")
list(APPEND FILES "${DIR}/ImGui/imgui_widgets.cpp")
list(APPEND FILES "${DIR}/ImGui/backends/imgui_impl_opengl3.cpp")
list(APPEND FILES "${DIR}/ImGui/backends/imgui_impl_sdl2.cpp")
list(APPEND FILES "${DIR}/ImGui/backends/imgui_impl_sdlrenderer.cpp")
list(APPEND FILES "${DIR}/thirdparty/ImGuiFileDialog/ImGuiFileDialog.cpp")

list(APPEND FILES "${DIR}/source/apu/APU.cpp")
list(APPEND FILES "${DIR}/source/apu/Envelope.cpp")
list(APPEND FILES "${DIR}/source/apu/LenCounter.cpp")
list(APPEND FILES "${DIR}/source/apu/NoiseChannel.cpp")
list(APPEND FILES "${DIR}/source/apu/Sequencer.cpp")
list(APPEND FILES "${DIR}/source/apu/SquareChannel.cpp")
list(APPEND FILES "${DIR}/source/apu/Sweep.cpp")
list(APPEND FILES "${DIR}/source/apu/WaveChannel.cpp")

list(APPEND FILES "${DIR}/source/common/BitManip.cpp")
list(APPEND FILES "${DIR}/source/common/Error.cpp")
list(APPEND FILES "${DIR}/source/common/Logger.cpp")

list(APPEND FILES "${DIR}/source/cpu/arm/ARM_Implementation.cpp")
list(APPEND FILES "${DIR}/source/cpu/thumb/THUMB_Implementation.cpp")
list(APPEND FILES "${DIR}/source/cpu/core/ARM7TDI.cpp")
list(APPEND FILES "${DIR}/source/cpu/core/Pipeline.cpp")
list(APPEND FILES "${DIR}/source/cpu/core/Register.cpp")

list(APPEND FILES "${DIR}/source/gamepack/GamePack.cpp")
list(APPEND FILES "${DIR}/source/gamepack/Header.cpp")
list(APPEND FILES "${DIR}/source/gamepack/backups/Database.cpp")
list(APPEND FILES "${DIR}/source/gamepack/backups/EEPROM.cpp")
list(APPEND FILES "${DIR}/source/gamepack/backups/Flash.cpp")
list(APPEND FILES "${DIR}/source/gamepack/backups/SRAM.cpp")
list(APPEND FILES "${DIR}/source/gamepack/gpio/Gpio.cpp")
list(APPEND FILES "${DIR}/source/gamepack/gpio/RTC.cpp")
list(APPEND FILES "${DIR}/source/gamepack/mapping/FileMapping.cpp")
list(APPEND FILES "${DIR}/source/gamepack/mapping/linux.cpp")

list(APPEND FILES "${DIR}/source/memory/Bus.cpp")
list(APPEND FILES "${DIR}/source/memory/DirectMemoryAccess.cpp")
list(APPEND FILES "${DIR}/source/memory/EventScheduler.cpp")
list(APPEND FILES "${DIR}/source/memory/InterruptController.cpp")
list(APPEND FILES "${DIR}/source/memory/Keypad.cpp")
list(APPEND FILES "${DIR}/source/memory/MMIO.cpp")
list(APPEND FILES "${DIR}/source/memory/Prefetch.cpp")
list(APPEND FILES "${DIR}/source/memory/Timers.cpp")
list(APPEND FILES "${DIR}/source/memory/Timing.cpp")

list(APPEND FILES "${DIR}/source/ppu/Mode0.cpp")
list(APPEND FILES "${DIR}/source/ppu/Mode1.cpp")
list(APPEND FILES "${DIR}/source/ppu/Mode2.cpp")
list(APPEND FILES "${DIR}/source/ppu/Mode3.cpp")
list(APPEND FILES "${DIR}/source/ppu/Mode4.cpp")
list(APPEND FILES "${DIR}/source/ppu/Mode5.cpp")
list(APPEND FILES "${DIR}/source/ppu/ModeUtils.cpp")
list(APPEND FILES "${DIR}/source/ppu/Objects.cpp")
list(APPEND FILES "${DIR}/source/ppu/PPU.cpp")

list(APPEND FILES "${DIR}/main_test.cpp")

project(GBAEmu)

add_executable(gba_emu ${FILES})

target_compile_options(gba_emu PRIVATE ${COMPILE_FLAGS})

target_link_libraries(gba_emu PUBLIC fmt)
target_link_libraries(gba_emu PUBLIC GLEW)
target_link_libraries(gba_emu PUBLIC SDL2)
target_link_libraries(gba_emu PUBLIC SDL2_image)


