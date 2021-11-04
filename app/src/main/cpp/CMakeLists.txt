cmake_minimum_required(VERSION 3.4.1)

project(jzIntvImGui)
set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCE_FILES main.cpp
        main_window.cpp
        configuration_window.cpp
        utils/strings.cpp
        utils/time.cpp
        utils/ini.cpp
        utils/messages.cpp
        utils/memory.cpp
        utils/file_system.cpp
        utils/images.cpp
        utils/gui_events.cpp
        utils/events.cpp
        utils/controls.cpp
        utils/screen.cpp
        utils/exceptions.cpp
        imgui_scrollable.cpp
        popup.cpp
        jzintv/misc/file_crc32.c
        jzintv/misc/crc32.c
        jzintv/lzoe/lzoe.c
        jzintv/plat/plat_gen.c
        jzintv/minilzo/minilzo.c
        jzintv/jzintv.c
        jzintv/pads/pads_cgc_win32.c
        jzintv/pads/pads_intv2pc.c
        jzintv/cfg/cfg.c
        jzintv/cfg/usage.c
        jzintv/cfg/mapping.c
        jzintv/misc/crc16.c
        jzintv/misc/avl.c
        jzintv/misc/ll.c
        jzintv/misc/types.c
        jzintv/misc/jzprint.c
        jzintv/misc/printer.c
        jzintv/plat/gnu_getopt.c
        jzintv/event/event.c
        jzintv/event/event_tbl.c
        jzintv/gfx/palette.c
        jzintv/gfx/gfx.c
        jzintv/mvi/mvi.c
        jzintv/debug/debug.c
        jzintv/debug/debug_dasm1600.c
        jzintv/util/symtab.c
        jzintv/debug/source.c
        jzintv/periph/periph.c
        jzintv/cp1600/cp1600.c
        jzintv/cp1600/op_decode.c
        jzintv/cp1600/op_exec.c
        jzintv/cp1600/emu_link.c
        jzintv/cp1600/op_exec_ext.c
        jzintv/cp1600/req_q.c
        jzintv/cp1600/tbl/fn_cond_br.c
        jzintv/cp1600/tbl/fn_dir_2op.c
        jzintv/cp1600/tbl/fn_imm_2op.c
        jzintv/cp1600/tbl/fn_impl_1op_a.c
        jzintv/cp1600/tbl/fn_impl_1op_b.c
        jzintv/cp1600/tbl/fn_ind_2op.c
        jzintv/cp1600/tbl/fn_reg_1op.c
        jzintv/cp1600/tbl/fn_reg_2op.c
        jzintv/cp1600/tbl/fn_rot_1op.c
        jzintv/cp1600/tbl/formats.c
        jzintv/mem/mem.c
        jzintv/icart/icart.c
        jzintv/icart/icartrom.c
        jzintv/icart/icartbin.c
        jzintv/icart/icartfile.c
        jzintv/icart/icarttag.c
        jzintv/icart/icarttag_authors.c
        jzintv/icart/icarttag_printer.c
        jzintv/stic/stic.c
        jzintv/pads/pads.c
        jzintv/pads/pads_cgc.c
        jzintv/ay8910/ay8910.c
        jzintv/ecs/ecs.c
        jzintv/ivoice/ivoice.c
        jzintv/speed/speed.c
        jzintv/file/file.c
        jzintv/file/elfi.c
        jzintv/metadata/metadata.c
        jzintv/metadata/cfgvar_metadata.c
        jzintv/metadata/icarttag_metadata.c
        jzintv/metadata/print_metadata.c
        jzintv/bincfg/bincfg.c
        jzintv/bincfg/bincfg_lex.c
        jzintv/bincfg/bincfg_grmr.tab.c
        jzintv/bincfg/legacy.c
        jzintv/gif/gif_enc.c
        jzintv/gif/lzw_enc.c
        jzintv/demo/demo.c
        jzintv/name/name.c
        jzintv/name/name_list.c
        jzintv/serializer/serializer.c
        jzintv/jlp/jlp.c
        jzintv/scale/scale2x.c
        jzintv/scale/scale3x.c
        jzintv/zlib/deflate.c
        jzintv/zlib/adler32.c
        jzintv/zlib/trees.c
        jzintv/zlib/zutil.c
        jzintv/avi/avi.c
        jzintv/cheat/cheat.c
        jzintv/plat/plat_sdl.c
        jzintv/plat/plat_lib.c
        jzintv/event/event_sdl.c
        jzintv/event/event_sdl2.c
        jzintv/gfx/gfx_sdl2.c
        jzintv/gfx/gfx_scale.c
        jzintv/gfx/gfx_prescale.c
        jzintv/snd/snd_sdl.c
        jzintv/joy/joy_sdl.c
        jzintv/mouse/mouse_sdl.c
        jzintv/locutus/locutus.cpp
        jzintv/locutus/luigi.cpp
        jzintv/locutus/locutus_adapt.cpp
        jzintv/locutus/locutus_types.cpp
        jzintv/locutus/loc_to_bin.cpp
        jzintv/locutus/bin_to_loc.cpp
        jzintv/locutus/rom_to_loc.cpp
        jzintv/svn_revision.c
        )

##########################
#  Generic configuration #
##########################
if (ANDROID)
    set(LIBS_FOLDER ${PROJECT_SOURCE_DIR}/../../../libs)
    add_definitions(-DUSE_SDL2 -DLINUX -DDOSTMP -DUSE_BOOKMARK)
else ()
    if (UNIX)
        find_package(OpenGL REQUIRED)
        find_package(glfw3)
        add_definitions(-DUSE_SDL2 -DLINUX -DDOSTMP -DDIRECT_INTV2PC)
    else ()
        add_definitions(-DUSE_SDL2 -DWIN32 -DDOSTMP -DDIRECT_INTV2PC)
    endif ()
    set(LIBS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs)
endif ()

set(CPP_FOLDER ${LIBS_FOLDER}/../src/main/cpp)
include_directories(${CPP_FOLDER} ${CPP_FOLDER}/jzintv)

#############
#  LibSDL2  #
#############
if (ANDROID)
    #################
    #  From sources #
    #################
    set(SDL2_FOLDER_STRING SDL2-2.0.12)
    set(SDL2_FOLDER ${LIBS_FOLDER}/${SDL2_FOLDER_STRING})
    set(SDL2_INCLUDE_DIR ${SDL2_FOLDER}/include)
    if (ANDROID)
        set(SDL2_LIBRARY SDL2 SDL2main)
    else ()
        # Da tenere se si vorranno compilare i sorgenti da WIN
        if (WIN32)
            set(SDL2_LIBRARY SDL2 SDL2main)
            list(APPEND TARGET_LINK_LIBRARIES mingw32)
        endif ()
    endif ()
    add_subdirectory(${SDL2_FOLDER} buildSdl2)
else ()
    ##########################
    ## From precompiled lib  #
    ##########################
    set(SDL2_FOLDER_STRING SDL2-Precompiled)
    set(SDL2_FOLDER ${LIBS_FOLDER}/${SDL2_FOLDER_STRING})
    set(SDL2_INCLUDE_DIR "${SDL2_FOLDER}/include/SDL2")
    set(SDL2_LIB_DIR "${SDL2_FOLDER}/lib")
    add_library(SDL2 STATIC IMPORTED)
    add_library(SDL2main STATIC IMPORTED)
    if (WIN32)
        set_property(TARGET SDL2 PROPERTY IMPORTED_LOCATION "${SDL2_LIB_DIR}/libSDL2.a")
        set_property(TARGET SDL2main PROPERTY IMPORTED_LOCATION "${SDL2_LIB_DIR}/libSDL2main.a")
        set(SDL2_LIBRARY mingw32 SDL2main SDL2 winmm imm32 version setupapi)
    else ()
        find_package(SDL2 REQUIRED)
        list(APPEND SDL2_LIBRARY ${SDL2_LIBRARIES} dl)
    endif ()
endif ()
list(APPEND TARGET_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR})
list(APPEND TARGET_LINK_LIBRARIES ${SDL2_LIBRARY})

###################
#  LibSDL2_image  #
###################
if (ANDROID)
    #################
    #  From sources #
    #################
    set(SDL2_IMAGE_FOLDER_STRING SDL2_image-2.0.5)
    set(SDL2_IMAGE_FOLDER ${LIBS_FOLDER}/${SDL2_IMAGE_FOLDER_STRING})
    set(SDL2_IMAGE_INCLUDE_DIR ${SDL2_IMAGE_FOLDER})
    file(GLOB SDL_IMAGE_SOURCES ${SDL2_IMAGE_FOLDER}/*.c)
    list(APPEND SOURCE_FILES ${SDL_IMAGE_SOURCES})
    add_library(SDL2_image SHARED ${SDL_IMAGE_SOURCES})
    target_link_libraries(SDL2_image SDL2)
    include_directories(${SDL2_INCLUDE_DIR})

    ############
    #  LibPng  #
    ############
    set(LIB_PNG_FOLDER ${SDL2_IMAGE_FOLDER}/external/libpng-1.6.37)
    add_subdirectory(${LIB_PNG_FOLDER} buildSdl2_png)
    add_definitions(-DLOAD_PNG)
    target_link_libraries(SDL2_image png)
    list(APPEND TARGET_LINK_LIBRARIES png)
    include_directories(${LIB_PNG_FOLDER})

else ()
    ##########################
    ## From precompiled lib  #
    ##########################
    set(SDL2_IMAGE_FOLDER_STRING SDL2_image-Precompiled)
    set(SDL2_IMAGE_FOLDER ${LIBS_FOLDER}/${SDL2_IMAGE_FOLDER_STRING})
    set(SDL2_IMAGE_INCLUDE_DIR "${SDL2_IMAGE_FOLDER}/include/SDL2")
    set(SDL2_IMAGE_LIB_DIR "${SDL2_IMAGE_FOLDER}/lib")

    add_library(SDL2_image STATIC IMPORTED)

    if (WIN32)
        set(SDL2_IMAGE_DLL_DIR "${SDL2_IMAGE_FOLDER}/bin")
        add_library(png SHARED "${SDL2_IMAGE_DLL_DIR}/libpng16-16.dll")
        set_target_properties(png PROPERTIES LINKER_LANGUAGE CXX)
        set_property(TARGET SDL2_image PROPERTY IMPORTED_LOCATION "${SDL2_IMAGE_LIB_DIR}/libSDL2_image.a")
        set(SDL2_IMAGE_LIBRARY SDL2_image png)
    else ()
        set(SDL2_image_DIR ${LIBS_FOLDER}/cmake)
        find_package(SDL2_image REQUIRED)
        list(APPEND SDL2_IMAGE_LIBRARY ${SDL2_IMAGE_LIBRARIES})
    endif ()
endif ()
list(APPEND TARGET_INCLUDE_DIRECTORIES ${SDL2_IMAGE_INCLUDE_DIR})
list(APPEND TARGET_LINK_LIBRARIES ${SDL2_IMAGE_LIBRARY})

###############
#  Dear ImGui #
###############
set(IMGUI_FOLDER_STRING imgui)
set(IMGUI_PATH ${LIBS_FOLDER}/${IMGUI_FOLDER_STRING})
set(IMGUI_IMPL_ROOT_PATH ${LIBS_FOLDER}/imgui_impl)

if (ANDROID)
    set(IMGUI_IMPL_PATH ${IMGUI_IMPL_ROOT_PATH}/android)
    set(GL_PROFILES "GL_PROFILE_GLES2" "GL_PROFILE_GLES3")
else ()
    if (WIN32)
        set(IMGUI_IMPL_PATH ${IMGUI_IMPL_ROOT_PATH}/desktop)
    else ()
        if (UNIX)
            set(IMGUI_IMPL_PATH ${IMGUI_IMPL_ROOT_PATH}/desktop)
        endif ()
    endif ()
endif ()

file(GLOB IMGUI_FILES
        ${IMGUI_PATH}/*.cpp
        ${IMGUI_IMPL_PATH}/*.cpp
        )

list(APPEND SOURCE_FILES ${IMGUI_FILES})
list(APPEND TARGET_INCLUDE_DIRECTORIES ${IMGUI_PATH})
list(APPEND TARGET_INCLUDE_DIRECTORIES ${IMGUI_IMPL_PATH})
list(APPEND TARGET_COMPILE_DEFINITIONS ${GL_PROFILES})

#########
#  GLM  #
#########
set(GLM_FOLDER_STRING glm-0.9.9.8)
set(GLM_PATH ${LIBS_FOLDER}/${GLM_FOLDER_STRING})
add_subdirectory(${GLM_PATH} buildGlm)
include_directories(${GLM_PATH})

##########
#  Glad  #
##########
set(GLAD_FOLDER_STRING glad)
set(GLAD_PATH ${LIBS_FOLDER}/${GLAD_FOLDER_STRING})
add_library(glad ${GLAD_PATH}/src/glad.c)
include_directories(${GLAD_PATH}/include)
list(APPEND TARGET_LINK_LIBRARIES glad)

if (WIN32)
    ################
    #  DLL_Dialog  #
    ################
    set(DLL_DIALOG_FOLDER_STRING WinDialog/Release)
    set(DLL_DIALOG_DIR ${LIBS_FOLDER}/${DLL_DIALOG_FOLDER_STRING})
    link_directories(${DLL_DIALOG_DIR})
    list(APPEND TARGET_LINK_LIBRARIES WinDialog.dll)
else ()
    set(IMGUI_FILE_DIALOG_FOLDER_STRING ImGuiFileDialog)
    set(IMGUI_FILE_DIALOG_DIR ${LIBS_FOLDER}/${IMGUI_FILE_DIALOG_FOLDER_STRING})
    file(GLOB IMGUI_FILE_DIALOG_FILES ${IMGUI_FILE_DIALOG_DIR}/*.cpp )
    include_directories(${IMGUI_FILE_DIALOG_DIR})
    list(APPEND SOURCE_FILES ${IMGUI_FILE_DIALOG_FILES})
endif ()

###########
#  Global #
###########
# Build as a library for Android, as an executable otherwise
if (ANDROID)
    add_library(jzIntvImGui SHARED ${SOURCE_FILES})
else ()
    add_executable(jzIntvImGui ${SOURCE_FILES} configuration_window.cpp)
    if (WIN32)
        add_custom_command(TARGET jzIntvImGui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_IMAGE_DLL_DIR}/libpng16-16.dll" $<TARGET_FILE_DIR:jzIntvImGui>)
        add_custom_command(TARGET jzIntvImGui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL_DIALOG_DIR}/WinDialog.dll" $<TARGET_FILE_DIR:jzIntvImGui>)
        add_custom_command(TARGET jzIntvImGui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBS_FOLDER}/Win32/zlib1.dll" $<TARGET_FILE_DIR:jzIntvImGui>)
        add_custom_command(TARGET jzIntvImGui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBS_FOLDER}/Win32/libstdc++-6.dll" $<TARGET_FILE_DIR:jzIntvImGui>)
        add_custom_command(TARGET jzIntvImGui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBS_FOLDER}/Win32/libgcc_s_dw2-1.dll" $<TARGET_FILE_DIR:jzIntvImGui>)
    endif ()
endif ()

target_link_libraries(jzIntvImGui ${TARGET_LINK_LIBRARIES})
target_include_directories(jzIntvImGui PRIVATE ${TARGET_INCLUDE_DIRECTORIES})
target_compile_definitions(jzIntvImGui PRIVATE ${TARGET_COMPILE_DEFINITIONS})

