cmake_minimum_required(VERSION 3.20)

project(GradientEditorProjectConverter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(${PROJECT_NAME} WIN32
    main.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE

    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /utf-8
        /W4
        /sdl

        /permissive-
        /Zc:char8_t-
        $<$<CONFIG:Debug>:/Zi>

        $<IF:$<CONFIG:Debug>,/Od,/O2>
        $<$<CONFIG:Release>:/Oi>

        $<IF:$<CONFIG:Debug>,/GL-,/GL>
        $<IF:$<CONFIG:Debug>,/Gy-,/Gy>
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>

        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>

        # Hybrid CRT
       	$<$<CONFIG:Debug>:/NODEFAULTLIB:libucrtd.lib>
        $<$<CONFIG:Debug>:/DEFAULTLIB:ucrtd.lib>
        $<$<NOT:$<CONFIG:Debug>>:/NODEFAULTLIB:libucrt.lib>
        $<$<NOT:$<CONFIG:Debug>>:/DEFAULTLIB:ucrt.lib>
    )
endif()
