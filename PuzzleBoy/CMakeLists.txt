Project (PuzzleBoy)
CMake_Minimum_Required (VERSION 2.6)
Set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# check if "override" is supported
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
        message(WARNING "GCC version < 4.7, override unsupported")
        add_definitions(-Doverride=)
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.0)
        message(WARNING "Clang version < 3.0, override unsupported")
        add_definitions(-Doverride=)
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0)
        message(WARNING "MSVC version < 2003, override unsupported")
        add_definitions(-Doverride=)
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with MSVC and GCC.")
endif()

Find_Package (SDL2 REQUIRED)
Find_Package (Freetype REQUIRED)

# Find OpenGL or OpenGLES
Find_Package (OpenGL)
if(OPENGL_FOUND)
	Set (MY_OPENGL_INCLUDE_DIR ${OPENGL_gl_INCLUDE_DIR})
	Set (MY_OPENGL_LIBRARY ${OPENGL_gl_LIBRARY})
else()
	Find_Package (OpenGLES REQUIRED)
	if(OPENGLES_FOUND)
		message(STATUS "Found OpenGL ES")
		add_definitions(-DUSE_OPENGLES)
		Set (MY_OPENGL_INCLUDE_DIR ${OPENGLES_INCLUDE_DIR})
		Set (MY_OPENGL_LIBRARY ${OPENGLES_LIBRARIES})
	else()
		message(FATAL_ERROR "No OpenGL or OpenGL ES found!")
	endif()
endif()

Include_Directories(
	${CMAKE_SOURCE_DIR}
	${MY_OPENGL_INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
)

File (GLOB SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

Add_Executable (puzzleboy ${SOURCES})

Target_Link_Libraries (
	puzzleboy
	${MY_OPENGL_LIBRARY}
	${SDL2_LIBRARY}
	${FREETYPE_LIBRARIES}
)


