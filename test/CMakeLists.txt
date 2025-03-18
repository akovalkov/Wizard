  project(Wizard_tests)
  CPMAddPackage(NAME  doctest
    VERSION         2.4.11
    GIT_REPOSITORY  https://github.com/doctest/doctest.git
    GIT_SHALLOW     TRUE
    DONWLOAD_ONLY   TRUE
  )


  add_executable(${PROJECT_NAME} tmain.cpp test-parser.cpp test-render.cpp test-environment.cpp test-desc.cpp test-transform.cpp test-project.cpp test-utils.cpp)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
  enable_testing()
  add_test(${PROJECT_NAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME})

  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
  endif()

  target_link_libraries(${PROJECT_NAME} doctest::doctest Boost::json)
