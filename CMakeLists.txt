cmake_minimum_required(VERSION 4.3.1 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine)
set(CONFIG_DIR config)
set(AUDIO_DIR audio)
set(MINIAUDIO_DIR miniaudio)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(SHADERS_DIR shaders)
set(TEXTURES_DIR textures)
set(BINARY_DIR bin)
set(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine_platform")
set(RENDERER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine_renderer")
set(MEMORY_READER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/memory_reader")
set(PUGIXML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pugixml")

Project(${PROJECT_NAME})

add_subdirectory(${PLATFORM_DIR})
add_subdirectory(${RENDERER_DIR})

list(APPEND SOURCES
    ${SRC_DIR}/game_library.cpp
    ${SRC_DIR}/game_library_watcher.cpp
    ${SRC_DIR}/render_command_generator.cpp
    ${SRC_DIR}/zerox_miniaudio_player.cpp
    ${SRC_DIR}/zerox_engine_logger.cpp
    ${SRC_DIR}/zerox_engine_runtime.cpp
    ${SRC_DIR}/zerox_glfw_window.cpp
    ${SRC_DIR}/zerox_engine_input_glfw.cpp
    ${SRC_DIR}/config/zerox_config_loader.cpp
    ${SRC_DIR}/config/zerox_config_parser.cpp
    ${SRC_DIR}/config/zerox_graphics_config.cpp
    ${SRC_DIR}/zerox_engine_context.cpp
    ${SRC_DIR}/zerox_frame_builder.cpp
    ${SRC_DIR}/zerox_memory_map.cpp
    ${MINIAUDIO_DIR}/miniaudio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

add_custom_target(Assets ALL)

if (WIN32)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glfw")
elseif (LINUX)
    find_package(glfw3 3.4 REQUIRED)
endif()

set(zerox_memory_bridge_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../zerox_memory_bridge/install/share/cmake/zerox_memory_bridge")
find_package(zerox_memory_bridge CONFIG REQUIRED)

add_subdirectory(${PUGIXML_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    ${MINIAUDIO_DIR}
    ${PLATFORM_DIR}/include
    ${RENDERER_DIR}/include
    "${RENDERER_DIR}/zerox_engine_glad/include"
    ${MEMORY_READER_DIR}/include
    ${PUGIXML_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE zerox_engine_platform zerox_engine_renderer zerox_memory_bridge::zerox_memory_bridge glfw pugixml::static)

add_custom_command(TARGET Assets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${AUDIO_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${AUDIO_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${SHADERS_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADERS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURES_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${TEXTURES_DIR}
    VERBATIM)

if (WIN32)
    add_custom_command(TARGET Assets POST_BUILD
        COMMAND if exist "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}" ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${CONFIG_DIR}
        VERBATIM)
elseif (LINUX)
    add_custom_command(TARGET Assets POST_BUILD
        COMMAND test ! -d "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}" || ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}" $<TARGET_FILE_DIR:${PROJECT_NAME}>/${CONFIG_DIR}
        VERBATIM)
endif()

add_dependencies(${PROJECT_NAME} zerox_engine_platform zerox_engine_glad zerox_engine_renderer Assets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${AUDIO_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${AUDIO_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADERS_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${SHADERS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TEXTURES_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${TEXTURES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/"
    VERBATIM)

if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND if exist "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${CONFIG_DIR}" ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${CONFIG_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${CONFIG_DIR}"
        VERBATIM)
elseif (LINUX)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND test ! -d "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${CONFIG_DIR}" || ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${CONFIG_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR}/$<CONFIG>/${CONFIG_DIR}"
        VERBATIM)
endif()