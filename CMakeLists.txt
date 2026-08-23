cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(SHADERS_DIR shaders)
set(TEXTURES_DIR textures)
set(BINARY_DIR bin)
set(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine_platform")
set(RENDERER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine_renderer")
set(MEMORY_READER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/memory_reader")
set(GAME_DIR zerox_game)

Project(${PROJECT_NAME})

add_subdirectory(${PLATFORM_DIR})
add_subdirectory(${RENDERER_DIR})

list(APPEND SOURCES
    ${SRC_DIR}/game_library.cpp
    ${SRC_DIR}/game_library_watcher.cpp
    ${SRC_DIR}/zerox_engine_runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

add_custom_target(Assets ALL)

find_package(glfw3 3.4 REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    ${GAME_DIR}/include
    ${PLATFORM_DIR}/include
    ${RENDERER_DIR}/include
    "${RENDERER_DIR}/zerox_engine_glad/include"
    ${MEMORY_READER_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE zerox_engine_platform zerox_engine_renderer glfw)

add_custom_command(TARGET Assets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${SHADERS_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADERS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURES_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${TEXTURES_DIR}
    VERBATIM)

add_dependencies(${PROJECT_NAME} zerox_engine_platform zerox_engine_glad zerox_engine_renderer Assets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADERS_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${SHADERS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TEXTURES_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${TEXTURES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/"
    VERBATIM)
