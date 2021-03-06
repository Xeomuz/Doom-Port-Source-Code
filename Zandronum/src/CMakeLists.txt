cmake_minimum_required( VERSION 2.4 )

if( COMMAND cmake_policy )
	cmake_policy( SET CMP0003 NEW )
endif( COMMAND cmake_policy )

include( CheckCXXSourceCompiles )
include( CheckFunctionExists )
include( FindPkgConfig )
include( FindOpenGL )
# [BL] I can't seem to find where ZDoom gets this
include( CheckLibraryExists )

option( NO_ASM "Disable assembly code" )
if( CMAKE_COMPILER_IS_GNUCXX )
	option( NO_STRIP "Do not strip Release or MinSizeRel builds" )
endif( CMAKE_COMPILER_IS_GNUCXX )

if( CMAKE_SIZEOF_VOID_P MATCHES "8" )
	set( X64 64 )
endif( CMAKE_SIZEOF_VOID_P MATCHES "8" )

# You can either use "make install" on the FMOD distribution to put it
# in standard system locations, or you can unpack the FMOD distribution
# in the root of the zdoom tree. e.g.:
# zdoom
#   docs
#   fmodapi<version>linux[64] -or simply- fmod
#   jpeg-6b
#   ...

# Construct version numbers for searching for the FMOD library on Linux.
set( MINOR_VERSIONS "50" "49" "48" "47" "46" "45" "44" "43" "42" "41"
	"40" "39" "38" "37" "36" "35" "34" "33" "32" "31" "30" "29" "28"
	"27" "26" "25" "24" "23" "22" "21" "20" "21" "19" "18" "17" "16"
	"15" "14" "13" "12" "11" "10" "09" "08" "07" "06" "05" "04" "03"
	"02" "01" "00" )
set( MAJOR_VERSIONS "26" "24" "22" "20" )
set( FMOD_DIR_VERSIONS ${FMOD_DIR_VERSIONS} "../fmod" )
foreach( majver ${MAJOR_VERSIONS} )
	foreach( minver ${MINOR_VERSIONS} )
		set( FMOD_VERSIONS ${FMOD_VERSIONS} "fmodex${X64}-4.${majver}.${minver}" )
		set( FMOD_DIR_VERSIONS ${FMOD_DIR_VERSIONS} "${CMAKE_HOME_DIRECTORY}/fmodapi4${majver}${minver}linux${X64}" )
	endforeach( minver ${MINOR_VERSIONS} )
	foreach( dir ${FMOD_DIR_VERSIONS} )
		set( FMOD_LOCAL_INC_DIRS ${FMOD_LOCAL_INC_DIRS} "${dir}/api/inc" )
		set( FMOD_LOCAL_LIB_DIRS ${FMOD_LOCAL_LIB_DIRS} "${dir}/api/lib" )
	endforeach( dir ${FMOD_DIR_VERSIONS} )
endforeach( majver ${MAJOR_VERSIONS} )

if( WIN32 )
	if( X64 )
		set( WIN_TYPE Win64 )
		set( XBITS x64 )
	else( X64 )
		set( WIN_TYPE Win32 )
		set( XBITS x86 )
	endif( X64 )
	
	add_definitions( -D_WIN32 )
	
	set( FMOD_SEARCH_PATHS
		"C:/Program Files/FMOD SoundSystem/FMOD Programmers API ${WIN_TYPE}/api"
		"C:/Program Files (x86)/FMOD SoundSystem/FMOD Programmers API ${WIN_TYPE}/api"
		"E:/Program Files (x86)/FMOD SoundSystem/FMOD Programmers API ${WIN_TYPE}/api" )
	set( FMOD_INC_PATH_SUFFIXES PATH_SUFFIXES inc )
	set( FMOD_LIB_PATH_SUFFIXES PATH_SUFFIXES lib )
	set( NASM_NAMES nasmw nasm )
	
	find_path( D3D_INCLUDE_DIR d3d9.h
		PATHS ENV DXSDK_DIR
		PATH_SUFFIXES Include )
	if( NOT D3D_INCLUDE_DIR )
		message( SEND_ERROR "Could not find DirectX 9 header files" )
	else( NOT D3D_INCLUDE_DIR )
		include_directories( ${D3D_INCLUDE_DIR} )
	endif( NOT D3D_INCLUDE_DIR )

	find_library( DX_ddraw_LIBRARY ddraw
		PATHS ENV DXSDK_DIR
		PATH_SUFFIXES Lib Lib/${XBITS} )
	find_library( DX_dxguid_LIBRARY dxguid
		PATHS ENV DXSDK_DIR
		PATH_SUFFIXES Lib Lib/${XBITS} )
	find_library( DX_dinput8_LIBRARY dinput8
		PATHS ENV DXSDK_DIR
		PATH_SUFFIXES Lib Lib/${XBITS} )

	set( DX_LIBS_FOUND YES )
	if( NOT DX_ddraw_LIBRARY )
		set( DX_LIBS_FOUND NO )
	endif( NOT DX_ddraw_LIBRARY )
	if( NOT DX_dxguid_LIBRARY )
		set( DX_LIBS_FOUND NO )
	endif( NOT DX_dxguid_LIBRARY )
	if( NOT DX_dinput8_LIBRARY )
		set( DX_LIBS_FOUND NO )
	endif( NOT DX_dinput8_LIBRARY )

	if( NOT DX_LIBS_FOUND )
		message( FATAL_ERROR "Could not find DirectX 9 libraries" )
	endif( NOT DX_LIBS_FOUND )

	set( ZDOOM_LIBS
		wsock32
		winmm
		"${DX_ddraw_LIBRARY}"
		"${DX_dxguid_LIBRARY}"
		"${DX_dinput8_LIBRARY}"
		ole32
		user32
		gdi32
		comctl32
		comdlg32
		ws2_32
		setupapi )
else( WIN32 )
	if( APPLE )
		set( FMOD_SEARCH_PATHS "/Developer/FMOD Programmers API Mac/api" )
		set( FMOD_INC_PATH_SUFFIXES PATH_SUFFIXES inc )
		set( FMOD_LIB_PATH_SUFFIXES PATH_SUFFIXES lib )
		set( NO_GTK ON )

		# [BL] We need to compile without OpenGL for now.
		# Same goes for the ASM code.
		set( NO_GL ON )
		set( NO_ASM ON )
	else( APPLE )
		option( NO_GTK "Disable GTK+ dialogs (Not applicable to Windows)" )
		option( VALGRIND "Add special Valgrind sequences to self-modifying code" )
		# [BB] Added SERVERONLY, NO_SOUND and NO_GL.
		option( SERVERONLY "Build a server only binary" )
		option( NO_SOUND "Turn of sound support" )
		option( NO_GL "Turn of OpenGL support" )

		# [BB] The server doesn't need sound, GTK or OpenGL.
		if( SERVERONLY )
			set( NO_SOUND ON )
			set( NO_GTK ON )
			set( NO_GL ON )
			add_definitions ( -DSERVER_ONLY=1 )
			set( ZDOOM_EXE_NAME ${ZDOOM_EXE_NAME}-server )
		endif( SERVERONLY )

		set( FMOD_SEARCH_PATHS
			/usr/local/include
			/usr/local/include/fmodex
			/usr/include
			/usr/include/fmodex
			/opt/local/include
			/opt/local/include/fmodex
			/opt/include
			/opt/include/fmodex )
		set( FMOD_INC_PATH_SUFFIXES PATH_SUFFIXES fmodex )

		# Use GTK+ for the IWAD picker, if available.
		if( NOT NO_GTK )
			pkg_check_modules( GTK2 gtk+-2.0 )
			if( GTK2_FOUND )
				set( ZDOOM_LIBS ${ZDOOM_LIBS} ${GTK2_LIBRARIES} )
				include_directories( ${GTK2_INCLUDE_DIRS} )
			else( GTK2_FOUND )
				set( NO_GTK ON )
			endif( GTK2_FOUND )
		endif( NOT NO_GTK )
	endif( APPLE )
	set( NASM_NAMES nasm )
	
	if( NO_GTK )
		add_definitions( -DNO_GTK=1 )
	endif( NO_GTK )
	
	# Non-Windows version also needs SDL
	find_package( SDL )
	if( NOT SDL_FOUND )
		message( SEND_ERROR "SDL is required for building." )
	endif( NOT SDL_FOUND )
	set( ZDOOM_LIBS ${ZDOOM_LIBS} "${SDL_LIBRARY}" )
	# [BB] Don't put the SDL path in inverted commas. At least on FreeBSD 8.1 this
	# variable contains two directories.
	include_directories( ${SDL_INCLUDE_DIR} )

	find_path( FPU_CONTROL_DIR fpu_control.h )
	if( FPU_CONTROL_DIR )
		include_directories( ${FPU_CONTROL_DIR} )
		add_definitions( -DHAVE_FPU_CONTROL )
	endif( FPU_CONTROL_DIR )

	# [BB] Only the Windows version has the server console GUI.
	add_definitions( -DNO_SERVER_GUI=1 )

	# [BB] Turning off sound needs special handling in the code, activated with the define NO_SOUND.
	if( NO_SOUND )
		add_definitions( -DNO_SOUND=1 )
	endif( NO_SOUND )

	# [BB] Turning off OpenGL needs special handling in the code, activated with the define NO_GL.
	if( NO_GL )
		add_definitions( -DNO_GL=1 )
	endif( NO_GL )
endif( WIN32 )

if( X64 )
	set( NO_ASM ON )
endif( X64 )

# Check if we have OpenGL
# [BB] Only if necessary
if( NOT NO_GL )
	if( NOT OPENGL_FOUND )
		message( FATAL_ERROR "OpenGL is required for building." )
	endif( NOT OPENGL_FOUND )
	if( NOT OPENGL_GLU_FOUND )
		message( FATAL_ERROR "OpenGL GLU is required for building." )
	endif( NOT OPENGL_GLU_FOUND )
	
	set( ZDOOM_LIBS ${ZDOOM_LIBS} ${OPENGL_LIBRARIES} )
	include_directories( ${OPENGL_INCLUDE_DIR} )
endif( NOT NO_GL )

# Decide on the name of the FMOD library we want to use.

if( NOT FMOD_LIB_NAME AND MSVC )
	set( FMOD_LIB_NAME fmodex${X64}_vc )
endif( NOT FMOD_LIB_NAME AND MSVC )

if( NOT FMOD_LIB_NAME AND BORLAND )
	set( FMOD_LIB_NAME fmodex${X64}_bc )
endif( NOT FMOD_LIB_NAME AND BORLAND )

if( NOT FMOD_LIB_NAME )
	set( FMOD_LIB_NAME fmodex${X64} )
endif( NOT FMOD_LIB_NAME )


# Search for FMOD include files

if( NOT WIN32 )
	find_path( FMOD_INCLUDE_DIR fmod.hpp
		PATHS ${FMOD_LOCAL_INC_DIRS} )
endif( NOT WIN32 )

if( NOT FMOD_INCLUDE_DIR )
	find_path( FMOD_INCLUDE_DIR fmod.hpp
		PATHS ${FMOD_SEARCH_PATHS}
		${FMOD_INC_PATH_SUFFIXES} )
endif( NOT FMOD_INCLUDE_DIR )

if( FMOD_INCLUDE_DIR )
	message( STATUS "FMOD include files found at ${FMOD_INCLUDE_DIR}" )
else( FMOD_INCLUDE_DIR )
	# [BB] Without sound, we don't need FMOD.
	if ( NOT NO_SOUND )
		message( SEND_ERROR "Could not find FMOD include files" )
	endif ( NOT NO_SOUND )
endif( FMOD_INCLUDE_DIR )


# Search for FMOD library

if( WIN32 OR APPLE )
	find_library( FMOD_LIBRARY ${FMOD_LIB_NAME}
		PATHS ${FMOD_SEARCH_PATHS}
		${FMOD_LIB_PATH_SUFFIXES} )
else( WIN32 OR APPLE )
	find_library( FMOD_LIBRARY
		NAMES ${FMOD_VERSIONS}
		PATHS ${FMOD_LOCAL_LIB_DIRS} )
endif( WIN32 OR APPLE )

if( FMOD_LIBRARY )
	message( STATUS "FMOD library found at ${FMOD_LIBRARY}" )
else( FMOD_LIBRARY )
	# [BB] Without sound, we don't need FMOD.
	if ( NOT NO_SOUND )
		message( SEND_ERROR "Could not find FMOD library" )
	endif ( NOT NO_SOUND )
endif( FMOD_LIBRARY )


# Search for NASM

if( CMAKE_SYSTEM_PROCESSOR MATCHES powerpc )
	if( NOT NO_ASM )
		message( STATUS "Disabling assembly code for PowerPC." )
		set( NO_ASM ON )
	endif( NOT NO_ASM )
endif( CMAKE_SYSTEM_PROCESSOR MATCHES powerpc )

if( NOT NO_ASM )
	if( UNIX AND X64 )
		find_program( GAS_PATH as )
		
		if( GAS_PATH )
			set( ASSEMBLER ${GAS_PATH} )
		else( GAS_PATH )
			message( STATUS "Could not find as. Disabling assembly code." )
			set( NO_ASM ON )
		endif( GAS_PATH )
	else( UNIX AND X64 )
		find_program( NASM_PATH NAMES ${NASM_NAMES} )
		find_program( YASM_PATH yasm )

		if( X64 )
			if( YASM_PATH )
				set( ASSEMBLER ${YASM_PATH} )
			else( YASM_PATH )
				message( STATUS "Could not find YASM. Disabling assembly code." )
				set( NO_ASM ON )
			endif( YASM_PATH )
		else( X64 )
			if( NASM_PATH )
				set( ASSEMBLER ${NASM_PATH} )
			else( NASM_PATH )
				message( STATUS "Could not find NASM. Disabling assembly code." )
				set( NO_ASM ON )
			endif( NASM_PATH )
		endif( X64 )
	endif( UNIX AND X64 )

	# I think the only reason there was a version requirement was because the
	# executable name for Windows changed from 0.x to 2.0, right? This is
	# how to do it in case I need to do something similar later.
	
	#	execute_process( COMMAND ${NASM_PATH} -v
	#		OUTPUT_VARIABLE NASM_VER_STRING )
	#	string( REGEX REPLACE ".*version ([0-9]+[.][0-9]+).*" "\\1" NASM_VER "${NASM_VER_STRING}" )
	#	if( NOT NASM_VER LESS 2 )
	#		message( SEND_ERROR "NASM version should be 2 or later. (Installed version is ${NASM_VER}.)" )
	#	endif( NOT NASM_VER LESS 2 )
endif( NOT NO_ASM )

if( NOT NO_ASM )
	# Valgrind support is meaningless without assembly code.
	if( VALGRIND )
		add_definitions( -DVALGRIND_AWARE=1 )
		# If you're Valgrinding, you probably want to keep symbols around.
		set( NO_STRIP ON )
	endif( VALGRIND )

	# Tell CMake how to assemble our files
	if( UNIX )
		set( ASM_OUTPUT_EXTENSION .o )
		if( X64 )
			set( ASM_FLAGS )
			set( ASM_SOURCE_EXTENSION .s )
		else( X64 )
			set( ASM_FLAGS -f elf -DM_TARGET_LINUX -i${CMAKE_CURRENT_SOURCE_DIR}/ )
			set( ASM_SOURCE_EXTENSION .asm )
		endif( X64 )
	else( UNIX )
		set( ASM_OUTPUT_EXTENSION .obj )
		set( ASM_SOURCE_EXTENSION .asm )
		if( X64 )
			set( ASM_FLAGS -f win64 -DWIN32 -DWIN64 )
		else( X64 )
			set( ASM_FLAGS -f win32 -DWIN32 -i${CMAKE_CURRENT_SOURCE_DIR}/ )
		endif( X64 )
	endif( UNIX )
	if( WIN32 )
		set( FIXRTEXT fixrtext )
	endif( WIN32 )
	message( STATUS "Selected assembler: ${ASSEMBLER}" )
	MACRO( ADD_ASM_FILE indir infile )
		set( ASM_OUTPUT_${infile} "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/zdoom.dir/${indir}/${infile}${ASM_OUTPUT_EXTENSION}" )
		if( WIN32 )
			set( FIXRTEXT_${infile} COMMAND ${FIXRTEXT} "${ASM_OUTPUT_${infile}}" )
		endif( WIN32 )
		add_custom_command( OUTPUT ${ASM_OUTPUT_${infile}}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/zdoom.dir/${indir}
			COMMAND ${ASSEMBLER} ${ASM_FLAGS} -o"${ASM_OUTPUT_${infile}}" "${CMAKE_CURRENT_SOURCE_DIR}/${indir}/${infile}${ASM_SOURCE_EXTENSION}"
			${FIXRTEXT_${infile}}
			DEPENDS ${indir}/${infile}.asm ${FIXRTEXT} )
		set( ASM_SOURCES ${ASM_SOURCES} "${ASM_OUTPUT_${infile}}" )
	ENDMACRO( ADD_ASM_FILE )
endif( NOT NO_ASM )

# Set up flags for GCC

if( CMAKE_COMPILER_IS_GNUCXX )
	set( REL_CXX_FLAGS "-fno-rtti" )
	if( NOT PROFILE )
		set( REL_CXX_FLAGS "${REL_CXX_FLAGS} -fomit-frame-pointer" )
	endif( NOT PROFILE )
	set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${REL_CXX_FLAGS}" )
	set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${REL_CXX_FLAGS}" )
	set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${REL_CXX_FLAGS}" )

	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused -fno-strict-aliasing" )

	if( NOT NO_STRIP )
		set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s" )
		set (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -s" )
	endif( NOT NO_STRIP )
endif( CMAKE_COMPILER_IS_GNUCXX )

# Check for functions that may or may not exist.

CHECK_FUNCTION_EXISTS( filelength FILELENGTH_EXISTS )
if( FILELENGTH_EXISTS )
	add_definitions( -DHAVE_FILELENGTH=1 )
endif( FILELENGTH_EXISTS )

CHECK_FUNCTION_EXISTS( strupr STRUPR_EXISTS )
if( NOT STRUPR_EXISTS )
	add_definitions( -DNEED_STRUPR=1 )
endif( NOT STRUPR_EXISTS )

CHECK_FUNCTION_EXISTS( stricmp STRICMP_EXISTS )
if( NOT STRICMP_EXISTS )
	add_definitions( -Dstricmp=strcasecmp )
endif( NOT STRICMP_EXISTS )

CHECK_FUNCTION_EXISTS( strnicmp STRNICMP_EXISTS )
if( NOT STRNICMP_EXISTS )
	add_definitions( -Dstrnicmp=strncasecmp )
endif( NOT STRNICMP_EXISTS )

if( NOT MSVC )
	add_definitions( -D__forceinline=inline )
endif( NOT MSVC )

if( UNIX )
	CHECK_LIBRARY_EXISTS( rt clock_gettime "" CLOCK_GETTIME_IN_RT )
	if( NOT CLOCK_GETTIME_IN_RT )
		CHECK_FUNCTION_EXISTS( clock_gettime CLOCK_GETTIME_EXISTS )
		if( NOT CLOCK_GETTIME_EXISTS )
			message( STATUS "Could not find clock_gettime. Timing statistics will not be available." )
			add_definitions( -DNO_CLOCK_GETTIME )
		endif( NOT CLOCK_GETTIME_EXISTS )
	else( NOT CLOCK_GETTIME_IN_RT )
		set( ZDOOM_LIBS ${ZDOOM_LIBS} rt )
	endif( NOT CLOCK_GETTIME_IN_RT )
endif( UNIX )

CHECK_CXX_SOURCE_COMPILES(
	"#include <stdarg.h>
	int main() { va_list list1, list2; va_copy(list1, list2); return 0; }"
	HAS_VA_COPY )
if( NOT HAS_VA_COPY )
	CHECK_CXX_SOURCE_COMPILES(
		"#include <stdarg.h>
		int main() { va_list list1, list2; __va_copy(list1, list2); return 0; }"
		HAS___VA_COPY )
	if( HAS___VA_COPY )
		add_definitions( -Dva_copy=__va_copy )
	else( HAS___VA_COPY )
		add_definitions( -DNO_VA_COPY )
	endif( HAS___VA_COPY )
endif( NOT HAS_VA_COPY )

# Update svnrevision.h

add_custom_target( revision_check ALL
	COMMAND ${CMAKE_BINARY_DIR}/tools/updaterevision/updaterevision . src/svnrevision.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS updaterevision )

# Libraries ZDoom needs

set( ZDOOM_LIBS ${ZDOOM_LIBS} "${ZLIB_LIBRARIES}" "${JPEG_LIBRARIES}" )
# [BB] Without sound, we don't need FMOD.
if ( NOT NO_SOUND )
	set( ZDOOM_LIBS ${ZDOOM_LIBS} "${FMOD_LIBRARY}" )
	include_directories( "${ZLIB_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${FMOD_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" )
else ( NOT NO_SOUND )
	include_directories( "${ZLIB_INCLUDE_DIR}" "${JPEG_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" )
endif ( NOT NO_SOUND )

# Start defining source files for ZDoom

if( WIN32 )
	set( SYSTEM_SOURCES_DIR win32 )
	set( SYSTEM_SOURCES
		win32/eaxedit.cpp
		win32/fb_d3d9.cpp
		win32/fb_d3d9_wipe.cpp
		win32/fb_ddraw.cpp
		win32/hardware.cpp
		win32/helperthread.cpp
		win32/i_cd.cpp
		win32/i_crash.cpp
		win32/i_input.cpp
		win32/i_main.cpp
		win32/i_movie.cpp
		win32/i_system.cpp
		win32/st_start.cpp
		win32/win32gliface.cpp
		win32/win32video.cpp )
	# [BB] Add files for G15 support and the Windows server console.
	set( SYSTEM_SOURCES ${SYSTEM_SOURCES}
		win32/g15/include/EZ_LCD.cpp
		win32/g15/include/LCDAnimatedBitmap.cpp
		win32/g15/include/LCDBase.cpp
		win32/g15/include/LCDBitmap.cpp
		win32/g15/include/LCDCollection.cpp
		win32/g15/include/LCDGfx.cpp
		win32/g15/include/LCDIcon.cpp
		win32/g15/include/LCDManager.cpp
		win32/g15/include/LCDOutput.cpp
		win32/g15/include/LCDProgressBar.cpp
		win32/g15/include/LCDScrollingText.cpp
		win32/g15/include/LCDStreamingText.cpp
		win32/g15/include/LCDText.cpp
		win32/serverconsole/serverconsole.cpp
		win32/serverconsole/serverconsole_dmflags.cpp
		win32/serverconsole/serverconsole_settings.cpp )
	if( CMAKE_COMPILER_IS_GNUCXX )
		# CMake is not set up to compile and link rc files with GCC. :(
		add_custom_command( OUTPUT zdoom-rc.o
			COMMAND windres -o zdoom-rc.o -i ${CMAKE_CURRENT_SOURCE_DIR}/win32/zdoom.rc
			DEPENDS win32/zdoom.rc )
		set( SYSTEM_SOURCES ${SYSTEM_SOURCES} zdoom-rc.o )
	else( CMAKE_COMPILER_IS_GNUCXX )
		set( SYSTEM_SOURCES ${SYSTEM_SOURCES} win32/zdoom.rc )
		# [BB] Added RC file for the Windows server console.
		set( SYSTEM_SOURCES ${SYSTEM_SOURCES} win32/serverconsole/serverconsole.rc )
	endif( CMAKE_COMPILER_IS_GNUCXX )
else( WIN32 )
	set( SYSTEM_SOURCES_DIR sdl )
	set( SYSTEM_SOURCES
		sdl/crashcatcher.c
		sdl/hardware.cpp
		sdl/i_cd.cpp
		sdl/i_input.cpp
		sdl/i_main.cpp
		sdl/i_movie.cpp
		sdl/i_system.cpp
		sdl/sdlglvideo.cpp
		sdl/sdlvideo.cpp
		sdl/st_start.cpp )
	if( APPLE )
		set( SYSTEM_SOURCES ${SYSTEM_SOURCES} sdl/SDLMain.m )
	endif( APPLE )
endif( WIN32 )

if( NO_ASM )
	add_definitions( -DNOASM )
else( NO_ASM )
	if( X64 )
		ADD_ASM_FILE( asm_x86_64 tmap3 )
	else( X64 )
		ADD_ASM_FILE( asm_ia32 a )
		ADD_ASM_FILE( asm_ia32 misc )
		ADD_ASM_FILE( asm_ia32 tmap )
		ADD_ASM_FILE( asm_ia32 tmap2 )
		ADD_ASM_FILE( asm_ia32 tmap3 )
	endif( X64 )
	if( WIN32 )
		if( NOT X64 )
			ADD_ASM_FILE( win32 wrappers )
		endif( NOT X64 )
	endif( WIN32 )
endif( NO_ASM )

add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xlat_parser.c ${CMAKE_CURRENT_BINARY_DIR}/xlat_parser.h
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/xlat/xlat_parser.y .
	COMMAND ${CMAKE_BINARY_DIR}/tools/lemon/lemon xlat_parser.y
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS lemon ${CMAKE_CURRENT_SOURCE_DIR}/xlat/xlat_parser.y )

add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h
	COMMAND ${CMAKE_BINARY_DIR}/tools/re2c/re2c --no-generation-date -s -o ${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h ${CMAKE_CURRENT_SOURCE_DIR}/sc_man_scanner.re
	DEPENDS re2c ${CMAKE_CURRENT_SOURCE_DIR}/sc_man_scanner.re )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

if( CMAKE_SYSTEM_PROCESSOR MATCHES powerpc )
	set( X86_SOURCES )
	set( NOT_X86 ON )
else( CMAKE_SYSTEM_PROCESSOR MATCHES powerpc )
	set( X86_SOURCES nodebuild_classify_sse2.cpp )
endif( CMAKE_SYSTEM_PROCESSOR MATCHES powerpc )

# [BB] Only complile the gl sources, if we want GL support.
if( NOT NO_GL )
	set( GL_SOURCES
		gl/a_dynlight.cpp
		gl/gfxfuncs.cpp
		gl/gl_bsp.cpp
		gl/gl_clipper.cpp
		gl/gl_clock.cpp
		gl/gl_cycler.cpp
		gl/gl_data.cpp
		gl/gl_decal.cpp
		gl/gl_drawinfo.cpp
		gl/gl_dynlight.cpp
		gl/gl_fakeflat.cpp
		gl/gl_flats.cpp
		gl/gl_framebuffer.cpp
		gl/gl_geometric.cpp
		gl/gl_glow.cpp
		gl/gl_hirestex.cpp
		gl/gl_hqresize.cpp
		gl/gl_light.cpp
		gl/gl_menu.cpp
		gl/gl_missingtexture.cpp
		gl/gl_models.cpp
		gl/gl_models_md2.cpp
		gl/gl_models_md3.cpp
		gl/gl_nodes.cpp
		gl/gl_portal.cpp
		gl/gl_scene.cpp
		gl/gl_shader.cpp
		gl/gl_sky.cpp
		gl/gl_skydome.cpp
		gl/gl_sprite.cpp
		gl/gl_swshaders.cpp
		gl/gl_texture.cpp
		gl/gltexture.cpp
		gl/gl_translate.cpp
		gl/gl_vertex.cpp
		gl/gl_walls.cpp
		gl/gl_walls_draw.cpp
		gl/gl_weapon.cpp
		gl/gl_wipe.cpp
		gl/r_render/r_opengl.cpp
	)
else( NOT NO_GL )
	set( GL_SOURCES sdl/glstubs.cpp )
endif( NOT NO_GL )


add_executable( zdoom WIN32
	autostart.cpp
	${ASM_SOURCES}
	${SYSTEM_SOURCES}
	${X86_SOURCES}
	x86.cpp
	am_map.cpp
	announcer.cpp #ST
	astar.cpp #ST
	#b_bot.cpp
	#b_func.cpp
	#b_game.cpp
	#b_move.cpp
	#b_think.cpp
	#bbannouncer.cpp
	botcommands.cpp #ST
	botpath.cpp #ST
	bots.cpp #ST
	browser.cpp #ST
	callvote.cpp #ST
	campaign.cpp #ST
	c_bind.cpp
	c_cmds.cpp
	c_console.cpp
	c_cvars.cpp
	c_dispatch.cpp
	c_expr.cpp
	chat.cpp #ST
	cl_commands.cpp #ST
	cl_demo.cpp #ST
	cl_main.cpp  #ST
	cl_pred.cpp #ST
	cl_statistics.cpp #ST
	cmdlib.cpp
	colormatcher.cpp
	compatibility.cpp
	configfile.cpp
	#ct_chat.cpp
	# [BB] Unforunately Skulltag's system that ensures that only one of the cvars cooperative, deathmatch and teamgame
	# is true at any time makes it important in which order the object files are linked. Otherwise using "+deathmatch 1" as
	# argument will not work properly. This definitely is absolutely awful and should ne made independent of the linking order.
	# For the time being, the linking order is changed to fix the problems.
	#cooperative.cpp #ST
	d_dehacked.cpp
	d_iwad.cpp
	d_main.cpp
	d_net.cpp
	d_netinfo.cpp
	d_protocol.cpp
	deathmatch.cpp #ST
	decallib.cpp
	dobject.cpp
	dobjgc.cpp
	dobjtype.cpp
	domination.cpp #ST
	doomdef.cpp
	doomstat.cpp
	dsectoreffect.cpp
	dthinker.cpp
	duel.cpp #ST
	f_finale.cpp
	f_wipe.cpp
	farchive.cpp
	files.cpp
	g_game.cpp
	g_hub.cpp
	g_level.cpp
	g_mapinfo.cpp
	g_skill.cpp
	gameconfigfile.cpp
	gamemode.cpp #ST
	gi.cpp
	#hu_scores.cpp
	huffman.cpp #ST
	i_net.cpp
	info.cpp
	invasion.cpp #ST
	joinqueue.cpp #ST
	lastmanstanding.cpp #ST
	lumpconfigfile.cpp
	m_alloc.cpp
	m_argv.cpp
	m_bbox.cpp
	m_cheat.cpp
	m_menu.cpp
	m_misc.cpp
	m_oldrandom.cpp #ST
	m_options.cpp
	m_png.cpp
	m_random.cpp
	maprotation.cpp #ST
	md5.cpp
	medal.cpp #ST
	mus2midi.cpp
	name.cpp
	network.cpp #ST
	networkshared.cpp #ST
	network/nettraffic.cpp #ST
	nodebuild.cpp
	nodebuild_classify_nosse2.cpp
	nodebuild_events.cpp
	nodebuild_extract.cpp
	nodebuild_gl.cpp
	nodebuild_utility.cpp
	p_3dfloors.cpp
	p_3dmidtex.cpp
	p_acs.cpp
	p_buildmap.cpp
	p_ceiling.cpp
	p_conversation.cpp
	p_doors.cpp
	p_effect.cpp
	p_enemy.cpp
	p_enemy_a_lookex.cpp
	p_floor.cpp
	p_interaction.cpp
	p_lights.cpp
	p_linkedsectors.cpp
	p_lnspec.cpp
	p_map.cpp
	p_maputl.cpp
	p_mobj.cpp
	p_pillar.cpp
	p_plats.cpp
	p_pspr.cpp
	p_saveg.cpp
	p_sectors.cpp
	p_setup.cpp
	p_sight.cpp
	p_slopes.cpp
	p_spec.cpp
	p_states.cpp
	p_switch.cpp
	p_teleport.cpp
	p_terrain.cpp
	p_things.cpp
	p_tick.cpp
	p_trace.cpp
	p_udmf.cpp
	p_user.cpp
	p_writemap.cpp
	p_xlat.cpp
	parsecontext.cpp
	platform.cpp #ST
	po_man.cpp
	possession.cpp #ST
	r_anim.cpp
	r_bsp.cpp
	r_data.cpp
	r_draw.cpp
	r_drawt.cpp
	r_interpolate.cpp
	r_main.cpp
	r_plane.cpp
	r_polymost.cpp
	r_segs.cpp
	r_sky.cpp
	r_things.cpp
	r_translate.cpp
	r_3dfloors.cpp
	s_advsound.cpp
	s_environment.cpp
	s_playlist.cpp
	s_sndseq.cpp
	s_sound.cpp
	sc_man.cpp
	scoreboard.cpp #ST
	sectinfo.cpp #ST
	st_stuff.cpp
	stats.cpp
	stringtable.cpp
	survival.cpp #ST
	sv_ban.cpp #ST
	sv_commands.cpp #ST
	sv_main.cpp #ST
	sv_master.cpp #ST
	sv_rcon.cpp #ST
	sv_save.cpp #ST
	tables.cpp
	team.cpp #ST
	teaminfo.cpp
	tempfiles.cpp
	unlagged.cpp #ST
	v_collection.cpp
	v_draw.cpp
	v_font.cpp
	v_palette.cpp
	v_pfx.cpp
	v_text.cpp
	v_video.cpp
	w_wad.cpp
	wi_stuff.cpp
	zstrformat.cpp
	zstring.cpp
	cooperative.cpp #ST [BB] This needs to be linked after deathmatch.cpp and team.cpp (see above).
	g_doom/a_doomartifacts.cpp #ST
	g_doom/a_doommisc.cpp
	g_doom/doom_sbar.cpp #ST doesn't use the SBARINFO version of the Doom status bar yet.
	g_heretic/a_hereticmisc.cpp
	g_heretic/heretic_sbar.cpp
	g_hexen/a_hexenmisc.cpp
	g_hexen/hexen_sbar.cpp
	g_raven/a_artitele.cpp
	g_raven/a_minotaur.cpp
	g_strife/a_strifestuff.cpp
	g_strife/strife_sbar.cpp
	g_shared/a_action.cpp
	g_shared/a_armor.cpp
	g_shared/a_artifacts.cpp
	g_shared/a_bridge.cpp
	g_shared/a_camera.cpp
	g_shared/a_debris.cpp
	g_shared/a_decals.cpp
	g_shared/a_fastprojectile.cpp
	g_shared/a_flags.cpp #ST
	g_shared/a_flashfader.cpp
	g_shared/a_fountain.cpp
	g_shared/a_hatetarget.cpp
	g_shared/a_icon.cpp #ST
	g_shared/a_keys.cpp
	g_shared/a_lightning.cpp
	g_shared/a_mapmarker.cpp
	g_shared/a_morph.cpp
	g_shared/a_movingcamera.cpp
	g_shared/a_pickups.cpp
	g_shared/a_puzzleitems.cpp
	g_shared/a_quake.cpp
	g_shared/a_randomspawner.cpp
	g_shared/a_returnzone.cpp #ST
	g_shared/a_secrettrigger.cpp
	g_shared/a_sectoraction.cpp
	g_shared/a_setcolor.cpp
	g_shared/a_skies.cpp
	g_shared/a_soundenvironment.cpp
	g_shared/a_soundsequence.cpp
	g_shared/a_spark.cpp
	g_shared/a_specialspot.cpp
	g_shared/a_springpad.cpp #ST
	g_shared/a_waterzone.cpp
	g_shared/a_weaponpiece.cpp
	g_shared/a_weapons.cpp
	g_shared/hudmessages.cpp
	g_shared/sbarinfo_display.cpp
	g_shared/sbarinfo_parser.cpp
	g_shared/sbar_mugshot.cpp
	g_shared/shared_hud.cpp
	g_shared/shared_sbar.cpp
	g_shared/st_hud.cpp #ST
	${GL_SOURCES}
	oplsynth/fmopl.cpp
	oplsynth/mlopl.cpp
	oplsynth/mlopl_io.cpp
	oplsynth/music_opldumper_mididevice.cpp
	oplsynth/music_opl_mididevice.cpp
	oplsynth/opl_mus_player.cpp
	sfmt/SFMT.cpp
	sound/fmodsound.cpp
	sound/i_music.cpp
	sound/i_sound.cpp
	sound/music_cd.cpp
	sound/music_dumb.cpp
	sound/music_midistream.cpp
	sound/music_midi_base.cpp
	sound/music_midi_midiout.cpp
	sound/music_midi_timidity.cpp
	sound/music_mus_midiout.cpp
	sound/music_mus_opl.cpp
	sound/music_spc.cpp
	sound/music_stream.cpp
	sound/music_timidity_mididevice.cpp
	sound/music_win_mididevice.cpp
	textures/automaptexture.cpp
	textures/bitmap.cpp
	textures/buildtexture.cpp
	textures/canvastexture.cpp
	textures/ddstexture.cpp
	textures/flattexture.cpp
	textures/imgztexture.cpp
	textures/jpegtexture.cpp
	textures/multipatchtexture.cpp
	textures/patchtexture.cpp
	textures/pcxtexture.cpp
	textures/pngtexture.cpp
	textures/rawpagetexture.cpp
	textures/texture.cpp
	textures/texturemanager.cpp
	textures/tgatexture.cpp
	textures/warptexture.cpp
	thingdef/olddecorations.cpp
	thingdef/thingdef.cpp
	thingdef/thingdef_codeptr.cpp
	thingdef/thingdef_data.cpp
	thingdef/thingdef_exp.cpp
	thingdef/thingdef_expression.cpp
	thingdef/thingdef_parse.cpp
	thingdef/thingdef_properties.cpp
	thingdef/thingdef_states.cpp
	timidity/common.cpp
	timidity/instrum.cpp
	timidity/instrum_dls.cpp
	timidity/instrum_font.cpp
	timidity/instrum_sf2.cpp
	timidity/mix.cpp
	timidity/playmidi.cpp
	timidity/resample.cpp
	timidity/timidity.cpp
	win32/g15/g15.cpp #ST
	xlat/parse_xlat.cpp
	../upnpnat/upnpnat.cpp #ST
	../upnpnat/xmlParser.cpp #ST
	#fragglescript/t_fspic.cpp
	#fragglescript/t_func.cpp
	#fragglescript/t_load.cpp
	#fragglescript/t_oper.cpp
	#fragglescript/t_parse.cpp
	#fragglescript/t_prepro.cpp
	#fragglescript/t_script.cpp
	#fragglescript/t_spec.cpp
	#fragglescript/t_variable.cpp
	#fragglescript/t_cmd.cpp
	autozend.cpp
)

set_source_files_properties( xlat/parse_xlat.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xlat_parser.c" )
set_source_files_properties( sc_man.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sc_man_scanner.h" )

# [BB] Without sound, we don't need snes_spc.
if( NOT NO_SOUND )
	set( ZDOOM_LIBS ${ZDOOM_LIBS} snes_spc )
endif( NOT NO_SOUND )

target_link_libraries( zdoom ${ZDOOM_LIBS} gdtoa dumb bzip2 lzma )
include_directories( .
	g_doom
	g_heretic
	g_hexen
	g_raven
	g_strife
	g_shared
	fragglescript
	Linux
	oplsynth
	sound
	textures
	thingdef
	timidity
	xlat
	../snes_spc/snes_spc
	../gdtoa
	../dumb/include
	../upnpnat/ #ST
	${CMAKE_BINARY_DIR}/gdtoa
	${SYSTEM_SOURCES_DIR} )

add_dependencies( zdoom revision_check )

# RUNTIME_OUTPUT_DIRECTORY does not exist in CMake 2.4.
# Linux distributions are slow to adopt 2.6. :(
set_target_properties( zdoom PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ZDOOM_OUTPUT_DIR} )
set_target_properties( zdoom PROPERTIES OUTPUT_NAME ${ZDOOM_EXE_NAME} )

if( NOT WIN32 )
	FILE( WRITE ${CMAKE_CURRENT_BINARY_DIR}/link-make "if [ ! -e ${ZDOOM_OUTPUT_DIR}/${ZDOOM_EXE_NAME} ]; then ln -sf ${CMAKE_CURRENT_BINARY_DIR}/${ZDOOM_EXE_NAME} ${ZDOOM_OUTPUT_DIR}/${ZDOOM_EXE_NAME}; fi" )
	add_custom_command( TARGET zdoom POST_BUILD
		COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/link-make
		COMMAND /bin/sh -c ${CMAKE_CURRENT_BINARY_DIR}/link-make )
endif( NOT WIN32 )

if( CMAKE_COMPILER_IS_GNUCXX )
	# GCC misoptimizes this file
	set_source_files_properties( oplsynth/fmopl.cpp PROPERTIES COMPILE_FLAGS "-fno-tree-dominator-opts -fno-tree-fre" )
	
	# Compile this one file with SSE2 support.
	set_source_files_properties( nodebuild_classify_sse2.cpp PROPERTIES COMPILE_FLAGS "-msse2 -mfpmath=sse" )

	# Need to enable intrinsics for this file.
	if( NOT NOT_X86 )
		set_source_files_properties( x86.cpp PROPERTIES COMPILE_FLAGS "-msse2 -mmmx" )
	endif( NOT NOT_X86 )
endif( CMAKE_COMPILER_IS_GNUCXX )

if( MSVC )
	# Compile this one file with SSE2 support.
	set_source_files_properties( nodebuild_classify_sse2.cpp PROPERTIES COMPILE_FLAGS "/arch:SSE2" )
endif( MSVC )
