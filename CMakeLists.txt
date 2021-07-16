cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target macOS architectures")

project(OttoMatic
	VERSION 4.0.0
	LANGUAGES C CXX
)

set(GAME_TARGET ${PROJECT_NAME})
set(GAME_ICNS packaging/${PROJECT_NAME}.icns)

set(POMME_NO_VIDEO TRUE)		# compile Pomme without QuickTime movie support
set(POMME_NO_QD3D TRUE)			# compile Pomme without QuickDraw3D/3DMF support
set(POMME_NO_INPUT true)
set(POMME_NO_GRAPHICS true)

#------------------------------------------------------------------------------
# GLOBAL OPTIONS (BEFORE ADDING SUBDIRECTORIES)
#------------------------------------------------------------------------------

if(MSVC)
	# This lets us detect that MSVC supports C++20 features via the __cplusplus macro
	# (See: https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	add_compile_options(/Zc:__cplusplus)
	add_compile_definitions(UNICODE _UNICODE)

	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${GAME_TARGET})
endif()

#------------------------------------------------------------------------------
# DEPENDENCIES
#------------------------------------------------------------------------------

add_subdirectory(extern/Pomme)

find_package(SDL2 REQUIRED COMPONENTS main)
find_package(OpenGL REQUIRED)

#------------------------------------------------------------------------------
# GAME SOURCES
#------------------------------------------------------------------------------

set(GAME_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Write header file containing version info
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in ${GAME_SRCDIR}/Headers/version.h)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${GAME_SRCDIR}/*.c ${GAME_SRCDIR}/*.cpp)

file(GLOB_RECURSE GAME_HEADERS CONFIGURE_DEPENDS ${GAME_SRCDIR}/*.h)

set(GAME_ALL_SOURCES
	${GAME_SOURCES}
	${GAME_HEADERS}
)

#------------------------------------------------------------------------------
# SOURCE GROUPS
#------------------------------------------------------------------------------

source_group(TREE ${GAME_SRCDIR} PREFIX "" FILES ${GAME_ALL_SOURCES})

#------------------------------------------------------------------------------
# EXECUTABLE TARGET
#------------------------------------------------------------------------------

set(GAME_LIBRARIES
	Pomme
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
)

if(WIN32)
	# "WIN32" here is equivalent to /SUBSYSTEM:WINDOWS for MSVC
	add_executable(${GAME_TARGET} WIN32
		${GAME_ALL_SOURCES}
		"${CMAKE_CURRENT_SOURCE_DIR}/packaging/OttoMatic.exe.rc"
	)
elseif(APPLE)
	# Add required frameworks for KillMacMouseAcceleration
	list(APPEND GAME_LIBRARIES "-framework Foundation" "-framework IOKit")

	add_executable(${GAME_TARGET} MACOSX_BUNDLE
		${GAME_ALL_SOURCES}
		${GAME_ICNS}
		${SDL2_LIBRARIES}
	)
else()
	# Math lib, explicitly required on some Linux systems
	list(APPEND GAME_LIBRARIES m)

	add_executable(${GAME_TARGET}
		${GAME_ALL_SOURCES}
	)
endif()

target_include_directories(${GAME_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	extern/Pomme/src
	${GAME_SRCDIR}
	${GAME_SRCDIR}/Headers
)

target_link_libraries(${GAME_TARGET} ${GAME_LIBRARIES})

#------------------------------------------------------------------------------
# DEFINES
#------------------------------------------------------------------------------

add_compile_definitions(
	"$<$<CONFIG:DEBUG>:_DEBUG>"
)

if(WIN32)
	# On Win32, we need NOGDI and NOUSER to be able to define some Mac functions
	# whose names are otherwise taken by Windows APIs.
	target_compile_definitions(${GAME_TARGET} PRIVATE
		WIN32_LEAN_AND_MEAN
		NOGDI
		NOUSER
		_CRT_SECURE_NO_WARNINGS  # quit whining about snprintf_s
	)
endif()

#------------------------------------------------------------------------------
# COMPILER OPTIONS
#------------------------------------------------------------------------------

if(NOT MSVC)
	target_compile_options(${GAME_TARGET} PRIVATE
		-fexceptions
		-Wall
		-Wextra
		-Wshadow
		-Wno-multichar
		-Wno-unknown-pragmas
		$<$<CXX_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>
		-Werror=return-type
		-Wno-unused-parameter
		-Werror=implicit-function-declaration
		-Werror=return-type
	)
else()
	# By default, MSVC may add /EHsc to CMAKE_CXX_FLAGS, which we don't want (we use /EHs below)
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

	# By default, MSVC may add /W3 to CMAKE_CXX_FLAGS, which we don't want (we use /W4 below)
	# Note that this is not required with "cmake_minimum_required(VERSION 3.15)" or later
	string(REPLACE "/W3" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/W3" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})

	target_compile_options(${GAME_TARGET} PRIVATE
		/EHs	# synchronous exceptions; also, extern "C" functions may throw exceptions
		/W4
		/wd4068 # ignore unrecognized pragmas
		/wd4100 # unreferenced formal parameters
		/wd4201 # nonstandard extension (nameless struct)
		/wd4244 # conversion from double to float
		/wd4305 # truncation from double to float
	)
endif()

#------------------------------------------------------------------------------
# PLATFORM-SPECIFIC PACKAGING
#------------------------------------------------------------------------------

if(APPLE)
	set_target_properties(${GAME_TARGET} PROPERTIES
		# Set framework search path to (App bundle)/Contents/Frameworks so the game can use its embedded SDL2.framework
		XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"

		# Explicitly turn off code signing, otherwise downloaded app will be quarantined forever
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""

		RESOURCE ${GAME_ICNS}
		MACOSX_BUNDLE_ICON_FILE OttoMatic.icns
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
		MACOSX_BUNDLE_COPYRIGHT "© 2001 Pangea Software, Inc."
	)

	# Copy stuff to app bundle contents
	set_source_files_properties(${GAME_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set_source_files_properties(${SDL2_LIBRARIES} PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")

	set(BUNDLE_CONTENTS_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/..")
	set(APP_PARENT_DIR "${BUNDLE_CONTENTS_DIR}/../..")

	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		# Copy assets to app bundle
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${BUNDLE_CONTENTS_DIR}/Resources

		# Nuke SDL headers in embedded framework
		COMMAND rm -rf ${BUNDLE_CONTENTS_DIR}/Frameworks/SDL2.framework/Headers
		COMMAND rm -rf ${BUNDLE_CONTENTS_DIR}/Frameworks/SDL2.framework/Versions/Current/Headers

		# High-DPI support in Info.plist
#		plutil -replace NSHighResolutionCapable -bool true ${BUNDLE_CONTENTS_DIR}/Info.plist
	)
else()
	set(APP_PARENT_DIR "$<TARGET_FILE_DIR:${GAME_TARGET}>")

	# Copy assets besides executable
	add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${APP_PARENT_DIR}/Data)
endif()

# Copy SDL2 DLLs to output folder on Windows
if(WIN32)
	foreach(DLL ${SDL2_DLLS})
		add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${APP_PARENT_DIR})
	endforeach()

	set_target_properties(${GAME_TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Copy documentation to output folder
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packaging/ReadMe.txt.in ${CMAKE_CURRENT_BINARY_DIR}/ReadMe.txt)
#add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/ReadMe.txt ${APP_PARENT_DIR}/ReadMe.txt
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md ${APP_PARENT_DIR}/License.txt
#	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs ${APP_PARENT_DIR}/Documentation
#)

