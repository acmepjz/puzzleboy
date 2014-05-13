Project (PuzzleBoy)
CMake_Minimum_Required (VERSION 2.6)
Set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

Find_Package (SDL2 REQUIRED)
Find_Package (Freetype REQUIRED)
Find_Package (OpenGL REQUIRED)

Include_Directories(
	${CMAKE_SOURCE_DIR}
	${OPENGL_gl_INCLUDE_DIR}
	${SDL2_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
)

File (GLOB SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

Add_Executable (puzzleboy ${SOURCES})

Target_Link_Libraries (
	puzzleboy
	${OPENGL_gl_LIBRARY}
	${SDL2_LIBRARY}
	${FREETYPE_LIBRARIES}
)


