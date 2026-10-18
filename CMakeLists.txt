cmake_minimum_required(VERSION 3.20)
project(MechWarrior C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(mechwarrior_original STATIC
    src/Original/campaign_graphics.c
    src/Original/Campaign/MW_MAIN_081B.c
    src/Original/Campaign/MW_MAIN_081B_RESOURCES.c
    src/Original/Campaign/MW_MAIN_081B_TEXT.c
    src/Original/Campaign/MW_MAIN_081B_PLANET.c
    src/Original/Campaign/MW_MAIN_081B_REPAIR.c
    src/Original/Campaign/MW_MAIN_081B_PILOTS.c
    src/Original/Campaign/MW_MAIN_081B_TRAVEL.c
    src/Original/Campaign/MW_MAIN_081B_ECONOMY.c
    src/Original/Campaign/MW_MAIN_081B_CONTRACTS.c
    src/Original/Campaign/MW_MAIN_081B_DEPLOYMENT.c
    src/Original/Campaign/MW_MAIN_081B_OPTIONS.c
    src/Original/Campaign/MW_MAIN_SAVE_FORMAT.c
    src/Original/Campaign/MW_MAIN_081B_SAVE.c
    src/Original/Campaign/MW_MAIN_081B_NEWS.c
    src/Original/Campaign/MW_MAIN_081B_UI.c
    src/Original/Campaign/MW_MAIN_081B_SERVICE.c
    src/Original/Campaign/MW_MAIN_081B_MARKET.c
    src/Original/Campaign/MW_MAIN_081B_INPUT.c
    src/Original/Campaign/MW_MAIN_081B_STARTUP.c
    src/Original/Campaign/MW_MAIN_081B_EVENTS.c
    src/Original/Campaign/MW_MAIN_081B_EVENT_UI.c
    src/Original/Campaign/MW_MAIN_081B_DISK.c
    src/Original/Memory/MW_MAIN_1090.c
    src/Original/Memory/MW_MAIN_1090_OPTIONS.c
    src/Original/Memory/MW_0806_STATE.c
    src/Original/Launcher/MW_0806.c
    src/Original/Combat/BTECH_0810.c
    src/Original/Combat/BTECH_0810_IMPORT.c
    src/Original/Combat/BTECH_0810_PLATFORM.c
    src/Original/Combat/BTECH_0810_RESOURCES.c
    src/Original/Combat/BTECH_0810_WORLD.c
    src/Original/Combat/BTECH_0810_STARTUP.c
    src/Original/Combat/BTECH_0810_INPUT.c
    src/Original/Runtime/BTECH_1BF5_INPUT.c
    src/Original/Runtime/BTECH_1BF5_KEYBOARD.c
    src/Original/Combat/BTECH_0810_POSE.c
    src/Original/Combat/BTECH_0810_ANIMATION.c
    src/Original/Combat/BTECH_0810_MECH_TICK.c
    src/Original/Combat/BTECH_0810_SURFACE.c
    src/Original/Combat/BTECH_0810_MOVEMENT.c
    src/Original/Combat/BTECH_0810_DAMAGE.c
    src/Original/Combat/BTECH_17FB_DAMAGE.c
    src/Original/Combat/BTECH_0810_WEAPON_GEOMETRY.c
    src/Original/Combat/BTECH_0810_WEAPONS.c
    src/Original/Combat/BTECH_0810_PROJECTILES.c
    src/Original/Combat/BTECH_0810_PLAYER_CONTROLS.c
    src/Original/Combat/BTECH_0810_AI_MOTION.c
    src/Original/Combat/BTECH_0810_AI_WEAPONS.c
    src/Original/Combat/BTECH_0810_NAVIGATION.c
    src/Original/Combat/BTECH_0810_TACTICS.c
    src/Original/Combat/BTECH_0810_TACTICAL_NAVIGATION.c
    src/Original/Combat/BTECH_0810_AI.c
    src/Original/Combat/BTECH_0810_MAIN.c
    src/Original/Combat/BTECH_17FB_TACTICS.c
    src/Original/Combat/BTECH_0810_EFFECTS.c
    src/Original/Combat/BTECH_0810_SCENE.c
    src/Original/Memory/BTECH_304E_POSE.c
    src/Original/Memory/BTECH_304E_SCENE.c
    src/Original/Combat/BTECH_0810_ORDERING.c
    src/Original/Combat/BTECH_17FB_ORDERING.c
    src/Original/Combat/BTECH_17FB_ORDERS.c
    src/Original/Combat/BTECH_17FB_PANELS.c
    src/Original/Combat/BTECH_17FB_SCREEN.c
    src/Original/Combat/BTECH_17FB_MAP.c
    src/Original/Combat/BTECH_0810_COCKPIT.c
    src/Original/Combat/BTECH_0810_HUD.c
    src/Original/Combat/BTECH_0810_HUD_TEXT.c
    src/Original/Combat/BTECH_0810_RADAR.c
    src/Original/Combat/BTECH_0810_WEAPON_HUD.c
    src/Original/Combat/BTECH_0810_TARGET_HUD.c
    src/Original/Combat/BTECH_17FB_RADAR.c
    src/Original/Video/BTECH_2A36_PRESENT.c
    src/Original/Combat/BTECH_0810_GAUGES.c
    src/Original/Combat/BTECH_17FB_HUD.c
    src/Original/Combat/BTECH_0810_BRIEFING.c
    src/Original/Combat/BTECH_0810_DIALOGS.c
    src/Original/Combat/BTECH_0810_INSPECTION.c
    src/Original/Combat/BTECH_0810_STAGE_RECOVERY.c
    src/Original/Combat/BTECH_0810_RESULTS.c
    src/Original/Combat/BTECH_0810_RENDER.c
    src/Original/Combat/BTECH_0810_CONTROLS.c
    src/Original/Combat/BTECH_17FB_STATE.c
    src/Original/Audio/BTECH_1ADF.c
    src/Original/Audio/BTECH_1ADF_INSTALL.c
    src/Original/Audio/BTECH_229C.c
    src/Original/Audio/BTECH_0810_SOUND.c
    src/Original/Audio/BTECH_2134.c
    src/Original/Runtime/BTECH_0810_STREAMS.c
    src/Original/Runtime/BTECH_0810_DIVISION.c
    src/Original/Runtime/BTECH_0810_EXIT.c
    src/Original/Runtime/BTECH_1BF5_TAGS.c
    src/Original/Runtime/BTECH_1BF5_CLEANUP.c
    src/Original/Runtime/BTECH_1BF5_INSTALL.c
    src/Original/Runtime/BTECH_1BF5_DIVIDE.c
    src/Original/Runtime/BTECH_1BF5_ARCHIVE.c
    src/Original/Runtime/BTECH_1BF5_PLATFORM.c
    src/Original/Geometry/BTECH_22B6_RESOURCES.c
    src/Original/Geometry/BTECH_22B6_WORLD.c
    src/Original/Geometry/BTECH_22B6_STARTUP.c
    src/Original/Geometry/BTECH_22B6_CAMERA.c
    src/Original/Geometry/BTECH_22B6_COMPOSITION.c
    src/Original/Geometry/BTECH_22B6_RESOURCE_RENDER.c
    src/Original/Geometry/BTECH_22B6_VERTEX_CACHE.c
    src/Original/Geometry/BTECH_22B6_POLYGON_COLOURS.c
    src/Original/Geometry/BTECH_22B6_WIDE_POLYGONS.c
    src/Original/Geometry/BTECH_22B6_WORD_POLYGONS.c
    src/Original/Geometry/BTECH_22B6_WIDE_CLIP.c
    src/Original/Combat/BTECH_0810_GEOMETRY.c
    src/Original/Geometry/BTECH_22B6_HORIZON.c
    src/Original/Geometry/BTECH_22B6_PROJECTION.c
    src/Original/Geometry/BTECH_22B6_DIVISION.c
    src/Original/Video/BTECH_1BF5.c
    src/Original/Video/BTECH_2A36.c
    src/Original/Video/BTECH_1BF5_BITMAPS.c
    src/Original/Video/BTECH_2A36_BITMAPS.c
    src/Original/Video/BTECH_2A36_BLIT.c
    src/Original/Video/BTECH_2A36_CAPTURE.c
    src/Original/Video/BTECH_1BF5_SCREENS.c
    src/Original/Video/BTECH_1BF5_SHAPES.c
    src/Original/Video/BTECH_1BF5_ELLIPSE.c
    src/Original/Video/BTECH_1BF5_POLYGONS.c
    src/Original/Video/BTECH_1BF5_POLYGON_CLIP.c
    src/Original/Video/BTECH_1BF5_POLYGON_EDGES.c
    src/Original/Video/BTECH_1BF5_LINES.c
    src/Original/Video/BTECH_2A36_POLYGONS.c
    src/Original/Video/BTECH_2A36_LINES.c
    src/Original/Video/BTECH_2A36_PIXELS.c
    src/Original/Video/BTECH_1BF5_PIXELS.c
    src/Original/Video/BTECH_2A36_SPANS.c
    src/Original/Video/BTECH_2A36_SCREENS.c
    src/Original/Combat/BTECH_CAMPAIGN_TRANSFER.c
    src/Original/Memory/BTECH_304E.c
    src/Original/Memory/BTECH_304E_WORLD.c
    src/Original/Memory/BTECH_304E_SOUND.c
    src/Original/Memory/BTECH_304E_STARTUP.c
    src/Original/Memory/BTECH_304E_ORDERS.c
    src/Original/Memory/BTECH_304E_RUNTIME.c
    src/Original/Memory/BTECH_304E_VIDEO.c
    src/Original/Memory/BTECH_304E_COCKPIT.c
    src/Original/Memory/BTECH_304E_RENDER.c
    src/Original/Memory/BTECH_304E_POLYGONS.c
    src/Original/Memory/BTECH_2FEB.c
    src/Original/Memory/BTECH_1ADF_STATE.c
    src/Original/Memory/BTECH_0810_STATE.c
    src/Original/Memory/BTECH_1BF5_STATE.c
    src/Original/Memory/BTECH_22B6_DATA.c
    src/Support/CombatPlatform.c
    src/Support/CombatInstallation.c
    src/Original/Memory/MW_MAIN_1090_EVENTS.c
    src/Original/Memory/MW_MAIN_1AA5.c
    src/Original/Memory/MW_MAIN_28BE.c
    src/Original/Memory/MW_MAIN_0F41_STATE.c
    src/Original/Runtime/MW_MAIN_0F41.c
    src/Original/Runtime/MW_MAIN_0F41_SYNTH.c
    src/Original/Runtime/MW_MAIN_0800.c
    src/Original/Memory/MW_EGA_0800_STATE.c
    src/Original/Video/MW_EGA_0800.c
    src/SDL/SDLBackend_Files.c
    src/Support/NativeAddress.c
    src/Support/MidiAddress.c)
target_include_directories(mechwarrior_original PUBLIC src/include)
set_target_properties(mechwarrior_original PROPERTIES POSITION_INDEPENDENT_CODE ON)


option(MECHWARRIOR_BUILD_SDL "Build the playable SDL3 game" ON)
if(MECHWARRIOR_BUILD_SDL)
    set(MECHWARRIOR_SDL_SOURCE_DIR "" CACHE PATH "Optional existing SDL3 source tree; never downloaded")
    if(MECHWARRIOR_SDL_SOURCE_DIR)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
        add_subdirectory("${MECHWARRIOR_SDL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/sdl3")
    else()
        find_package(SDL3 3.2 CONFIG REQUIRED)
    endif()
    add_library(mechwarrior_sdl STATIC
        src/SDL/SDLBackend_Main.c src/SDL/SDLBackend_Input.c src/SDL/SDLBackend_Time.c
        src/SDL/SDLBackend_DebugControl.c
        src/SDL/SDLBackend_Pilot.c src/Support/SupportPilot.c src/Support/SupportPilotObservation.c
        src/SDL/SDLBackend_Video.c src/SDL/SDLBackend_CombatVideo.c src/SDL/SDLBackend_CombatPlatform.c
        src/SDL/SDLBackend_CombatProcess.c src/SDL/SDLBackend_Audio.c)
    target_link_libraries(mechwarrior_sdl PUBLIC mechwarrior_original SDL3::SDL3)
    if(NOT MSVC)
        target_link_libraries(mechwarrior_sdl PUBLIC m)
    endif()
    target_include_directories(mechwarrior_sdl PRIVATE src/SDL)
    add_executable(mechwarrior src/SDL/SDLMain.c)
    target_link_libraries(mechwarrior PRIVATE mechwarrior_sdl)
    if(TARGET SDL3::SDL3-shared)
        add_custom_command(TARGET mechwarrior POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3-shared>" "$<TARGET_FILE_DIR:mechwarrior>")
    endif()
    if(MSVC)
        target_link_libraries(mechwarrior PRIVATE dbghelp)
        # Retain source locations in optimized builds for user crash reports.
        foreach(diagnostic_target mechwarrior_original mechwarrior_sdl mechwarrior)
            target_compile_options(${diagnostic_target} PRIVATE $<$<CONFIG:Release>:/Zi>)
        endforeach()
        target_link_options(mechwarrior PRIVATE $<$<CONFIG:Release>:/DEBUG:FULL>)
    endif()
endif()
