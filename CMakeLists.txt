set(PROJECT_NAME GBA_Emulator)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "apu/APU.hpp"
    "apu/BaseChannel.hpp"
    "apu/Envelope.hpp"
    "apu/LenCounter.hpp"
    "apu/NoiseChannel.hpp"
    "apu/Sequencer.hpp"
    "apu/SquareChannel.hpp"
    "apu/Sweep.hpp"
    "apu/WaveChannel.hpp"
    "audio_device/AudioDevice.hpp"
    "audio_device/filters/HighPassFilter.hpp"
    "audio_device/filters/LowPassFilter.hpp"
    "audio_device/sdl/SdlAudioDevice.hpp"
    "common/BitManip.hpp"
    "common/DebugCommon.hpp"
    "common/Defs.hpp"
    "common/Error.hpp"
    "common/FixedSizeAllocator.hpp"
    "common/ImGuiColors.hpp"
    "common/Logger.hpp"
    "common/SSE.hpp"
    "common/static_for.hpp"
    "config/Config.hpp"
    "cpu/arm/ARM_Implementation.hpp"
    "cpu/arm/Instruction.hpp"
    "cpu/arm/TableGen.hpp"
    "cpu/core/ARM7TDI.hpp"
    "cpu/core/CPSR.hpp"
    "cpu/core/CPUContext.hpp"
    "cpu/core/InterpreterCache.hpp"
    "cpu/core/Pipeline.hpp"
    "cpu/core/Register.hpp"
    "cpu/ProcessorDefs.hpp"
    "cpu/thumb/Instruction.hpp"
    "cpu/thumb/TableGen2.hpp"
    "cpu/thumb/THUMB_Implementation.hpp"
    "debugger/Debugger.hpp"
    "debugger/DebugStructures.hpp"
    "debugger/DebugWindow.hpp"
    "debugger/DisassembleARM.hpp"
    "debugger/DisassembleTHUMB.hpp"
    "emu/ActionReplay.hpp"
    "emu/Cheats.hpp"
    "emu/CodeBreaker.hpp"
    "emu/Emulator.hpp"
    "emu/GameShark.hpp"
    "emu/SaveState.hpp"
    "gamepack/backups/Backup.hpp"
    "gamepack/backups/Database.hpp"
    "gamepack/backups/EEPROM.hpp"
    "gamepack/backups/Flash.hpp"
    "gamepack/backups/SRAM.hpp"
    "gamepack/GamePack.hpp"
    "gamepack/gpio/Gpio.hpp"
    "gamepack/gpio/GpioDeviceBase.hpp"
    "gamepack/gpio/RTC.hpp"
    "gamepack/Header.hpp"
    "gamepack/mapping/FileMapping.hpp"
    "gamepack/mapping/windows.hpp"
    "gamepack/mapping/linux.hpp"
    "memory/Bus.hpp"
    "memory/DirectMemoryAccess.hpp"
    "memory/DMAFireType.hpp"
    "memory/EventScheduler.hpp"
    "memory/InterruptController.hpp"
    "memory/Keypad.hpp"
    "memory/MMIO.hpp"
    "memory/Timers.hpp"
    "memory/Timing.hpp"
    "ppu/ModeUtils.inl"
    "ppu/PPU.hpp"
    "shared_obj/ProcedureWrapper.hpp"
    "video/renderers/detail/OpengGLFunctions.hpp"
    "video/renderers/OpenGL_Renderer.hpp"
    "video/VideoOutput.hpp"
)
source_group("Header Files" FILES ${Header_Files})

set(Header_Files__ImGui
    "ImGui/imconfig.h"
    "ImGui/imgui.h"
    "ImGui/imgui_internal.h"
    "ImGui/imstb_rectpack.h"
    "ImGui/imstb_textedit.h"
    "ImGui/imstb_truetype.h"
)
source_group("Header Files\\ImGui" FILES ${Header_Files__ImGui})

set(Header_Files__ImGui__backends
    "ImGui/backends/imgui_impl_opengl3.h"
    "ImGui/backends/imgui_impl_sdl2.h"
    "ImGui/backends/imgui_impl_sdlrenderer.h"
    "ImGui/backends/imgui_impl_vulkan.h"
)
source_group("Header Files\\ImGui\\backends" FILES ${Header_Files__ImGui__backends})

set(Header_Files__RetroAchievements
    "emu/RetroAchievements.hpp"
)
source_group("Header Files\\RetroAchievements" FILES ${Header_Files__RetroAchievements})

set(Header_Files__hle
    "hle/BiosAffine.hpp"
    "hle/BiosHle.hpp"
    "hle/BiosMath.hpp"
    "hle/BiosMisc.hpp"
)
source_group("Header Files\\hle" FILES ${Header_Files__hle})

set(Resource_Files
    ".gitignore"
    "assets/base.frag"
    "assets/base.vert"
    "common/LINKS.txt"
)
source_group("Resource Files" FILES ${Resource_Files})

set(Source_Files
    "audio_device/AudioDevice.cpp"
    "audio_device/sdl/SdlAudioDevice.cpp"
    "config/Config.cpp"
    "debugger/Debugger.cpp"
    "debugger/DisassembleARM.cpp"
    "debugger/DisassembleTHUMB.cpp"
    "emu/ActionReplay.cpp"
    "emu/Cheats.cpp"
    "emu/CodeBreaker.cpp"
    "emu/Emulator.cpp"
    "emu/GameShark.cpp"
    "emu/SaveState.cpp"
    "main.cpp"
    "source/apu/APU.cpp"
    "source/apu/Envelope.cpp"
    "source/apu/LenCounter.cpp"
    "source/apu/NoiseChannel.cpp"
    "source/apu/Sequencer.cpp"
    "source/apu/SquareChannel.cpp"
    "source/apu/Sweep.cpp"
    "source/apu/WaveChannel.cpp"
    "source/common/BitManip.cpp"
    "source/common/Error.cpp"
    "source/common/Logger.cpp"
    "source/cpu/arm/ARM_Implementation.cpp"
    "source/cpu/core/ARM7TDI.cpp"
    "source/cpu/core/InterpreterCache.cpp"
    "source/cpu/core/Pipeline.cpp"
    "source/cpu/core/Register.cpp"
    "source/cpu/thumb/THUMB_Implementation.cpp"
    "source/gamepack/backups/Database.cpp"
    "source/gamepack/backups/EEPROM.cpp"
    "source/gamepack/backups/Flash.cpp"
    "source/gamepack/backups/SRAM.cpp"
    "source/gamepack/GamePack.cpp"
    "source/gamepack/gpio/Gpio.cpp"
    "source/gamepack/gpio/RTC.cpp"
    "source/gamepack/Header.cpp"
    "source/gamepack/mapping/FileMapping.cpp"
    "source/memory/Bus.cpp"
    "source/memory/DirectMemoryAccess.cpp"
    "source/memory/EventScheduler.cpp"
    "source/memory/InterruptController.cpp"
    "source/memory/Keypad.cpp"
    "source/memory/MMIO.cpp"
    "source/memory/Prefetch.cpp"
    "source/memory/Timers.cpp"
    "source/memory/Timing.cpp"
    "source/ppu/Mode0.cpp"
    "source/ppu/Mode1.cpp"
    "source/ppu/Mode2.cpp"
    "source/ppu/Mode3.cpp"
    "source/ppu/Mode4.cpp"
    "source/ppu/Mode5.cpp"
    "source/ppu/ModeUtils.cpp"
    "source/ppu/Objects.cpp"
    "source/ppu/PPU.cpp"
    "video/renderers/OpenGL_Renderer.cpp"
)

if(WIN32)
    list(APPEND ${Source_Files} "source/gamepack/mapping/windows.cpp")
else()
    list(APPEND ${Source_Files} "source/gamepack/mapping/linux.cpp")
endif()

source_group("Source Files" FILES ${Source_Files})

set(Source_Files__ImGui
    "ImGui/imgui.cpp"
    "ImGui/imgui_demo.cpp"
    "ImGui/imgui_draw.cpp"
    "ImGui/imgui_tables.cpp"
    "ImGui/imgui_widgets.cpp"
    "ImGui/misc/cpp/imgui_stdlib.cpp"
)
source_group("Source Files\\ImGui" FILES ${Source_Files__ImGui})

set(Source_Files__ImGuiFileDialog
    "thirdparty/ImGuiFileDialog/ImGuiFileDialog.cpp"
)
source_group("Source Files\\ImGuiFileDialog" FILES ${Source_Files__ImGuiFileDialog})

set(Source_Files__ImGui__backends
    "ImGui/backends/imgui_impl_opengl3.cpp"
    "ImGui/backends/imgui_impl_sdl2.cpp"
    "ImGui/backends/imgui_impl_sdlrenderer.cpp"
)
source_group("Source Files\\ImGui\\backends" FILES ${Source_Files__ImGui__backends})

set(Source_Files__RetroAchievements
    "emu/RetroAchievements.cpp"
)
source_group("Source Files\\RetroAchievements" FILES ${Source_Files__RetroAchievements})

set(Source_Files__hle
    "hle/BiosAffine.cpp"
    "hle/BiosHle.cpp"
    "hle/BiosMath.cpp"
    "hle/BiosMisc.cpp"
)
source_group("Source Files\\hle" FILES ${Source_Files__hle})

set(ALL_FILES
    ${Header_Files}
    ${Header_Files__ImGui}
    ${Header_Files__ImGui__backends}
    ${Header_Files__RetroAchievements}
    ${Header_Files__hle}
    ${Resource_Files}
    ${Source_Files}
    ${Source_Files__ImGui}
    ${Source_Files__ImGuiFileDialog}
    ${Source_Files__ImGui__backends}
    ${Source_Files__RetroAchievements}
    ${Source_Files__hle}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE GBAEmulator)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()
################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG;"
            "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /Ob2;
                /Oi;
                /Gy;
                /Ot;
                /W4
            >
            /permissive-;
            /sdl;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /Oi;
                /Gy
            >
            /permissive-;
            /sdl;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /INCREMENTAL
            >
            $<$<CONFIG:Release>:
                /OPT:REF;
                /LTCG:PGOptimize;
                /OPT:ICF;
                /INCREMENTAL:NO
            >
            /DEBUG;
            /SUBSYSTEM:CONSOLE
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /INCREMENTAL
            >
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF;
                /INCREMENTAL:NO
            >
            /DEBUG;
            /SUBSYSTEM:CONSOLE
        )
    endif()
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    RetroAchievements
)

target_link_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/RetroAchievements")
target_link_libraries(${PROJECT_NAME} PRIVATE "RetroAchievements")


