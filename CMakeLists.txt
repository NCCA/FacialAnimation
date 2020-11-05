cmake_minimum_required(VERSION 3.12)
# Name of the project
project(FacialAnimationBuild)
# This is the name of the Exe change this and it will change everywhere
set(TargetName FacialAnimation)
# Instruct CMake to run moc automatically when needed (Qt projects only)
set(CMAKE_AUTOMOC ON)
# find Qt libs
find_package(Qt5Widgets)
#find_package(Qt5 COMPONENTS Widgets OpenGL core Gui REQUIRED)
find_package(fmt CONFIG REQUIRED)
# use C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
# Set the name of the executable we want to build
add_executable(${TargetName})

# Add NGL include path
include_directories(include $ENV{HOME}/NGL/include)
target_sources(${TargetName} PRIVATE ${PROJECT_SOURCE_DIR}/src/main.cpp  
			${PROJECT_SOURCE_DIR}/src/NGLScene.cpp  
			${PROJECT_SOURCE_DIR}/include/NGLScene.h  
)
add_compile_definitions(ADDLARGEMODELS)

# see what platform we are on and set platform defines
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	message("Mac build")
	find_library(MACGL OpenGL)
	add_definitions(-DGL_SCILENCE_DEPRICATION)
	set ( PROJECT_LINK_LIBS ${MACGL})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set ( PROJECT_LINK_LIBS -lGL)
endif()

# now add NGL specific values
target_link_directories(${TargetName} PRIVATE $ENV{HOME}/NGL/lib )

# add exe and link libs that must be after the other defines

target_link_libraries(${TargetName} PRIVATE ${PROJECT_LINK_LIBS} NGL Qt5::Widgets fmt::fmt-header-only  )


add_custom_target(${TargetName}CopyShaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
		${CMAKE_CURRENT_BINARY_DIR}/shaders
		
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_BINARY_DIR}/models

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${CMAKE_CURRENT_BINARY_DIR}/fonts
		COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/models.txt
    ${CMAKE_CURRENT_BINARY_DIR}/models.txt

) 