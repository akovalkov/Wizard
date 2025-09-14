cmake_minimum_required (VERSION 3.27)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("reverser")

option(BUILD_TESTING "Build unit tests" ON)

# project sources
add_executable (${PROJECT_NAME} main.cpp helper.cpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

# add dependencies
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME Boost
    VERSION 1.89.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.7z
    URL_HASH SHA256=252149f583054515b10688855b24ca1c16072b5118e603b8fe017894d319c262
    OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES program_options\\\;json" # Note the escapes!
)

target_link_libraries(${PROJECT_NAME} Boost::program_options Boost::json)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
