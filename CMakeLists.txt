cmake_minimum_required(VERSION 3.20)
project(qflow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(QFLOW_BUILD_PYTHON "Build Python bindings via pybind11" ON)

find_package(Threads REQUIRED)

add_library(qflow_core
  cpp/src/version.cpp
  cpp/src/market_data.cpp
  cpp/src/engine.cpp
  cpp/src/feed.cpp
  cpp/src/portfolio.cpp
  cpp/src/strategy.cpp
  cpp/src/backtest.cpp
  cpp/src/indicator.cpp
  cpp/src/protocol.cpp
  cpp/src/risk.cpp
  cpp/src/gateway.cpp
)

target_include_directories(qflow_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
)

target_link_libraries(qflow_core PUBLIC Threads::Threads)

add_executable(qflow_cli cpp/src/main.cpp)
target_link_libraries(qflow_cli PRIVATE qflow_core)

enable_testing()

function(qflow_add_test test_name source)
  add_executable(${test_name} ${source})
  target_link_libraries(${test_name} PRIVATE qflow_core)
  add_test(NAME ${test_name} COMMAND ${test_name} ${ARGN})
endfunction()

qflow_add_test(qflow_market_data_test cpp/tests/market_data_test.cpp)
qflow_add_test(qflow_layout_test cpp/tests/layout_test.cpp)
qflow_add_test(qflow_engine_test cpp/tests/engine_test.cpp)
qflow_add_test(qflow_feed_test cpp/tests/feed_test.cpp
  ${CMAKE_SOURCE_DIR}/data/sample_ticks.csv)
qflow_add_test(qflow_portfolio_test cpp/tests/portfolio_test.cpp)
qflow_add_test(qflow_strategy_test cpp/tests/strategy_test.cpp)
qflow_add_test(qflow_backtest_test cpp/tests/backtest_test.cpp)
qflow_add_test(qflow_indicator_test cpp/tests/indicator_test.cpp)
qflow_add_test(qflow_spsc_test cpp/tests/spsc_test.cpp)
qflow_add_test(qflow_protocol_test cpp/tests/protocol_test.cpp)
qflow_add_test(qflow_risk_test cpp/tests/risk_test.cpp)

if(QFLOW_BUILD_PYTHON)
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
  )
  FetchContent_MakeAvailable(pybind11)

  pybind11_add_module(qflow_python cpp/python/qflow_module.cpp)
  target_link_libraries(qflow_python PRIVATE qflow_core)
  set_target_properties(qflow_python PROPERTIES OUTPUT_NAME "qflow")

  add_test(
    NAME python_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/examples/smoke_qflow.py
  )
  set_tests_properties(python_smoke PROPERTIES
    ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:qflow_python>"
  )
endif()
