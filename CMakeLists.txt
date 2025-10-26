cmake_minimum_required(VERSION 3.29)


# ------- FETCH DEPENDENCIES -------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
	imgui_app
    GIT_REPOSITORY "git@github.com:0xcafed00d/imgui_app.git"
    GIT_TAG main
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui_app)

# ------- PROJECT SETTINGS ---------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)

if(MSVC)
	add_compile_definitions(NOMINMAX)
	add_compile_definitions( _SCL_SECURE_NO_WARNINGS)
	add_compile_options(/W4 /WX)
	add_compile_options(/MP)
endif()

# linux settings
if(UNIX)
	# warning level 4 and all warnings as errors
	add_compile_options(-Wall -Wextra -Werror)
	add_compile_options(-Wno-unused-variable)
	add_compile_options(-Wno-unused-parameter)
endif()

# ------- project executable ----------------------------------------------------------------------------------
project(floppymonkey CXX )

add_executable(
	${CMAKE_PROJECT_NAME}
    WIN32
	src/main.cpp
    src/drivefunc.cpp
    src/drivefunc.h
    src/drivewindow.cpp
    src/drivewindow.h
    src/drivetask.cpp
    src/drivetask.h
    src/task.h
    src/winfunc.cpp
    src/winfunc.h
)

target_link_directories(${CMAKE_PROJECT_NAME}
	PRIVATE
	${SDL2_LIB_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} 
	PRIVATE 
	SDL2::SDL2main
	SDL2::SDL2-static
	imgui_app
)

