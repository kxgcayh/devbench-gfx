cmake_minimum_required(VERSION 3.21)

project(DevBenchGFx VERSION 0.3.1 LANGUAGES CXX)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB HEADERS CONFIGURE_DEPENDS src/*.h include/*.h)

# Struct-independent on purpose: it uses no struct layouts that differ between runtimes.
add_commonlibsse_plugin(${PROJECT_NAME}
    AUTHOR "Kautsar Al Bana"
    SOURCES ${HEADERS} ${SOURCES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE include src)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
