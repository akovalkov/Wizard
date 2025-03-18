cmake_minimum_required (VERSION 3.27)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Wizard")

option(BUILD_TESTING "Build unit tests" ON)

# Добавьте источник в исполняемый файл этого проекта.
add_executable (${PROJECT_NAME} main.cpp helper.cpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

# add dependencies
include(cmake/CPM.cmake)

CPMAddPackage(
  NAME Boost
  VERSION 1.87.0
  URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.gz
  URL_HASH SHA256=78fbf579e3caf0f47517d3fb4d9301852c3154bfecdc5eeebd9b2b0292366f5b
  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES program_options\\\;json" # Note the escapes!
)

target_link_libraries(${PROJECT_NAME} Boost::program_options Boost::json)

if(BUILD_TESTING)
  
add_subdirectory(test)
endif()
