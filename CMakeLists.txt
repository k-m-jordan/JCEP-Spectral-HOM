cmake_minimum_required(VERSION 3.22)

set(CMAKE_BUILD_TYPE Debug)

project(Spectral_HOM)

if(NOT EXISTS ${QT6_INSTALL})
    message(FATAL_ERROR "No Qt 6 path provided. Rerun with the option '-DQT6_INSTALL=/path/to/qt6'")
endif()

# Compiler-specific configuration
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "-O3")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "/EHsc")
else()
    message(FATAL_ERROR "Only GCC and MSVC compilers configured; CMakeLists.txt must be modified with appropriate compiler flags")
endif()

set(EIGEN_INSTALL ./lib/eigen-3.4.0)
set(TIMSORT_INSTALL ./lib/timsort)
set(QCUSTOMPLOT_INSTALL ./lib/qcustomplot)
set(DLIB_INSTALL ./lib/dlib-19.24)
list(APPEND CMAKE_PREFIX_PATH
        ${QT6_INSTALL}
        ${EIGEN_INSTALL}
    )

add_subdirectory(${EIGEN_INSTALL})
add_subdirectory(${QCUSTOMPLOT_INSTALL})
add_subdirectory(${DLIB_INSTALL}/dlib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(PCL 1.3 REQUIRED)

add_executable(Spectral_HOM
        src/main.cpp
        src/ui/ui.h
        src/ui/MainWindow.cpp
        src/ui/AppActions.cpp
        src/tpx3/tpx3.h
        src/tpx3/Tpx3Image.cpp
        src/tpx3/LoadRawFileThread.cpp
        src/ui/log.h
        src/ui/LogPanel.cpp
        src/tpx3/PixelData.cpp
        src/ui/threadutils.h
        src/ui/BgThread.cpp
        src/ui/FileInputPanel.cpp
        src/ui/FileInputSettingsPanel.cpp
        src/ui/FileImportProgressBar.cpp
        src/fileview/fileview.h
        src/fileview/FileViewer.cpp
        src/fileview/RawImageView.cpp
        src/fileview/ToTDistributionView.cpp
        src/ui/SetImageMaskDialog.cpp
        src/fileview/ClusteredImageView.cpp
        src/tpx3/LinePair.cpp
        src/fileview/StartStopHistogramView.cpp
        src/fileview/DToADistributionView.cpp
        src/fileview/SpatialCorrelationView.cpp
        src/fileview/FileViewPanel.cpp
        src/fileview/LinePlotView.cpp
        src/fileview/Hist2DView.cpp)
target_link_libraries(Spectral_HOM
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        qcustomplot
        pcl_common
        pcl_octree
        dlib::dlib
    )
target_include_directories(Spectral_HOM PUBLIC src/)
target_include_directories(Spectral_HOM SYSTEM PUBLIC
        ${EIGEN_INSTALL}
        ${TIMSORT_INSTALL}/include
        ${QCUSTOMPLOT_INSTALL}
        ${PCL_INCLUDE_DIRS}
        ${DLIB_INSTALL}
    )
target_precompile_headers(Spectral_HOM PUBLIC
        ${QCUSTOMPLOT_INSTALL}/qcustomplot.h)
link_directories(Spectral_HOM PUBLIC
        ${PCL_LIBRARY_DIRS}
    )
add_definitions(
        ${PCL_DEFINITIONS}
    )

# Use this if you have a missing DLL error
#add_custom_command(TARGET Spectral_HOM POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        "${PROJECT_BINARY_DIR}/lib/qcustomplot/qcustomplot.dll"
#        $<TARGET_FILE_DIR:Spectral_HOM>)
