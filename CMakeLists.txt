cmake_minimum_required(VERSION 3.21)
project(Gargantua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(Gargantua
    src/main.cpp
    src/Camera.cpp
    src/Physics.cpp
    src/Renderer.cpp
    src/ShaderUtil.cpp
)

target_link_libraries(Gargantua PRIVATE OpenGL::GL glfw GLEW::GLEW glm::glm imgui::imgui)

if(MSVC)
    target_compile_options(Gargantua PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(Gargantua PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# Copia os shaders para junto do executável (carregados em tempo de execução, recarregáveis com F5)
if(EXISTS "${CMAKE_SOURCE_DIR}/shaders")
    add_custom_command(TARGET Gargantua POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:Gargantua>/shaders)
else()
    message(WARNING "Diretorio shaders ausente; o executavel nao podera iniciar o renderer.")
endif()

# Validação opcional dos shaders (glslangValidator vem com o Vulkan SDK)
find_program(GLSLANG_VALIDATOR
    NAMES glslangValidator
    HINTS
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/glslang"
        "$ENV{VULKAN_SDK}/Bin")
if(GLSLANG_VALIDATOR AND EXISTS "${CMAKE_SOURCE_DIR}/shaders")
    add_custom_target(validate_shaders
        COMMAND "${GLSLANG_VALIDATOR}" "${CMAKE_SOURCE_DIR}/shaders/raytrace.comp"
        COMMAND "${GLSLANG_VALIDATOR}" "${CMAKE_SOURCE_DIR}/shaders/taa.comp"
        COMMAND "${GLSLANG_VALIDATOR}" "${CMAKE_SOURCE_DIR}/shaders/bloom_down.comp"
        COMMAND "${GLSLANG_VALIDATOR}" "${CMAKE_SOURCE_DIR}/shaders/bloom_blur.comp"
        COMMAND "${GLSLANG_VALIDATOR}" "${CMAKE_SOURCE_DIR}/shaders/fullscreen.vert"
        COMMAND "${GLSLANG_VALIDATOR}" "${CMAKE_SOURCE_DIR}/shaders/tonemap.frag"
        COMMENT "Validando shaders GLSL (OpenGL)")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/shaders")
    message(WARNING "glslangValidator nao encontrado; os shaders nao serao validados.")
endif()
