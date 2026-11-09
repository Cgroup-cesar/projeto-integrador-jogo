# CMake do PROJETO PRECURSOR (alternativa ao Makefile, para quem não tem a raylib instalada)
#
#   cmake -B build
#   cmake --build build
#   ctest --test-dir build          (os mesmos testes do make test)
#   cmake --build build --target smoke
#
# Se a raylib (5.5 ou mais nova) estiver instalada, ela é usada. Se não, o CMake baixa a raylib 6.0
# com FetchContent. Para forçar o download: cmake -B build -DPRECURSOR_BAIXAR_RAYLIB=ON
cmake_minimum_required(VERSION 3.20)
project(precursor C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PRECURSOR_BAIXAR_RAYLIB "Baixar a raylib 6.0 mesmo se houver uma instalada" OFF)

if(NOT PRECURSOR_BAIXAR_RAYLIB)
    find_package(raylib 5.5 QUIET)
endif()

if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

set(AVISOS -Wall -Wextra)
if(MSVC)
    set(AVISOS /W4)
endif()

file(GLOB FONTES_JOGO CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.c)

add_executable(precursor ${FONTES_JOGO})
target_include_directories(precursor PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(precursor PRIVATE RAYMATH_STATIC_INLINE)
target_compile_options(precursor PRIVATE ${AVISOS})
target_link_libraries(precursor PRIVATE raylib)
if(UNIX AND NOT APPLE)
    target_link_libraries(precursor PRIVATE m)
endif()

# Testes sem janela: os módulos puros (logica.c, regras.c) não incluem raylib.h.
# Eles usam só o raymath.h, que é só de cabeçalho; por isso o caminho de include da raylib entra aqui.
function(precursor_teste nome)
    add_executable(${nome} ${CMAKE_SOURCE_DIR}/tests/${nome}.c ${ARGN})
    target_include_directories(${nome} PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
    target_compile_definitions(${nome} PRIVATE RAYMATH_STATIC_INLINE)
    target_compile_options(${nome} PRIVATE ${AVISOS})
    target_link_libraries(${nome} PRIVATE raylib)
    if(UNIX AND NOT APPLE)
        target_link_libraries(${nome} PRIVATE m)
    endif()
    add_test(NAME ${nome} COMMAND ${nome})
endfunction()

enable_testing()
precursor_teste(test_logica src/logica.c src/regras.c)
precursor_teste(test_regras src/regras.c src/logica.c src/colecoes.c)
precursor_teste(test_fluxo src/regras.c src/logica.c)

add_custom_target(smoke
    COMMAND precursor --smoke
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS precursor
    COMMENT "Piloto automático: joga a partida inteira pelos dois finais")

add_custom_target(run
    COMMAND precursor
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS precursor)

add_custom_target(readme-check
    COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_readme_refs.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
