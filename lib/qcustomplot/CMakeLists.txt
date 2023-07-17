cmake_minimum_required(VERSION 3.20)

project(qcustomplot LANGUAGES CXX)

find_package(Qt6 COMPONENTS Widgets PrintSupport REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(qcustomplot SHARED)
target_compile_definitions(qcustomplot PRIVATE QCUSTOMPLOT_COMPILE_LIBRARY)
target_include_directories(qcustomplot PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_sources(qcustomplot PRIVATE ${CMAKE_CURRENT_LIST_DIR}/qcustomplot.cpp)
target_link_libraries(qcustomplot PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport)