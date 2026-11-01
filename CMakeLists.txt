cmake_minimum_required(VERSION 3.16)
project(raven_sample C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Where Raven comes from, in this order:
#   1. an installed package        (-DCMAKE_PREFIX_PATH=<Raven install folder>)
#   2. a local checkout of Raven   (-DRAVEN_SOURCE_DIR=<path to the Raven repo>)
#   3. downloaded from GitHub      (the default: nothing to install by hand)
# ---------------------------------------------------------------------------
set(RAVEN_SOURCE_DIR "" CACHE PATH "Path to a local Raven checkout (optional)")

find_package(raven QUIET)

if(NOT raven_FOUND)
	# Only the library: no examples, tests or designer.
	set(RAVEN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(RAVEN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(RAVEN_BUILD_TOOLS OFF CACHE BOOL "" FORCE)

	if(RAVEN_SOURCE_DIR)
		message(STATUS "Using Raven from ${RAVEN_SOURCE_DIR}")
		add_subdirectory("${RAVEN_SOURCE_DIR}" raven EXCLUDE_FROM_ALL)
	else()
		message(STATUS "Raven not found: downloading it from GitHub")
		include(FetchContent)
		# Pinned to a commit so every build uses the same Raven (its API still
		# changes). To update: replace the hash with a newer commit or tag of
		# https://github.com/oscar-soirey/Raven and test again.
		FetchContent_Declare(raven
			GIT_REPOSITORY https://github.com/oscar-soirey/Raven.git
			GIT_TAG 1d5c735fed18ef054bc47bc08c4cd50d0e63b376)
		FetchContent_MakeAvailable(raven)
	endif()

	# GCC warns about a message buffer inside Raven's own theme.c. It is not
	# in this project and does not affect it, so keep it out of your build log.
	if(TARGET raven AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
		target_compile_options(raven PRIVATE -Wno-format-truncation)
	endif()
endif()

add_executable(raven_sample src/main.c)
target_link_libraries(raven_sample PRIVATE raven::raven)
if(MSVC)
	target_compile_definitions(raven_sample PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# main.xml and strings.csv are read when the program runs, from its working
# folder. VS Code starts it in assets/ (see .vscode/), so saving main.xml while
# the program runs updates the window without building again. An installed
# program looks for them next to the executable (see src/main.c).

# A release build on Windows should not open a console window behind the GUI.
if(MSVC)
	target_link_options(raven_sample PRIVATE
		"$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>" "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>")
elseif(WIN32)
	target_link_options(raven_sample PRIVATE "$<$<CONFIG:Release>:-mwindows>")
endif()

# ---------------------------------------------------------------------------
# Packaging: `cpack` makes a .deb on Linux, a .zip (or an installer .exe) on
# Windows. Everything below belongs to the component "app", so the files that
# Raven itself installs (headers, static library, docs) stay out of the package.
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

if(WIN32)
	set(SAMPLE_BIN_DIR ".")
	set(SAMPLE_ASSETS_DIR "assets")
else()
	set(SAMPLE_BIN_DIR "${CMAKE_INSTALL_BINDIR}")
	set(SAMPLE_ASSETS_DIR "${CMAKE_INSTALL_DATADIR}/raven-sample")
endif()

install(TARGETS raven_sample RUNTIME DESTINATION "${SAMPLE_BIN_DIR}" COMPONENT app)
install(FILES assets/main.xml assets/strings.csv DESTINATION "${SAMPLE_ASSETS_DIR}" COMPONENT app)
if(UNIX AND NOT APPLE)
	install(FILES packaging/raven-sample.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications" COMPONENT app)
	install(FILES packaging/raven-sample.png
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps" COMPONENT app)
endif()

set(CPACK_PACKAGE_NAME "raven-sample")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Your Name")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A small program built with the Raven GUI library")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Raven Sample")
set(CPACK_COMPONENTS_ALL app)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

if(WIN32)
	# zip: unzip and run raven_sample.exe.  Installer: cpack -G NSIS (needs NSIS).
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_NSIS_DISPLAY_NAME "Raven Sample")
	set(CPACK_NSIS_INSTALLED_ICON_NAME "raven_sample.exe")
	set(CPACK_PACKAGE_EXECUTABLES "raven_sample" "Raven Sample")
else()
	set(CPACK_GENERATOR "DEB")
	set(CPACK_DEB_COMPONENT_INSTALL ON)
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <you@example.com>")
	set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
	# libc and libX11 are found from the binary; Raven loads Vulkan at run time, so say it here.
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libvulkan1")
	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "mesa-vulkan-drivers")
	set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
endif()

include(CPack)
