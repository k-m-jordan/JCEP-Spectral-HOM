cmake_minimum_required(VERSION 3.22)

project(Spectral_HOM)

set(CMAKE_BUILD_TYPE Release)

if(NOT EXISTS ${QT6_INSTALL})
    message(FATAL_ERROR "No Qt 6 path provided. Rerun with the option '-DQT6_INSTALL=/path/to/qt6'")
endif()

# Compiler-specific configuration
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "-O3 -march=native -Wno-deprecated-declarations -Wno-stringop-overflow")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "/EHsc /O2 /arch:AVX2 /fp:fast /GL")

    set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS})
    set(Boost_NO_WARN_NEW_VERSIONS 1)
else()
    message(FATAL_ERROR "Only GCC and MSVC compilers configured; CMakeLists.txt must be modified with appropriate compiler flags")
endif()

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set(EIGEN_INSTALL ./lib/eigen-3.4.0)
set(TIMSORT_INSTALL ./lib/timsort)
set(QCUSTOMPLOT_INSTALL ./lib/qcustomplot)
set(DLIB_INSTALL ./lib/dlib-19.24)
list(APPEND CMAKE_PREFIX_PATH
        ${QT6_INSTALL}
        ${EIGEN_INSTALL}
    )

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(EIGEN_BUILD_DOC OFF)

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
        dlib::dlib
        pcl_common
        pcl_octree
    )
target_include_directories(Spectral_HOM PUBLIC src/)
target_include_directories(Spectral_HOM SYSTEM PUBLIC
        ${EIGEN_INSTALL}
        ${TIMSORT_INSTALL}/include
        ${QCUSTOMPLOT_INSTALL}
        ${DLIB_INSTALL}
        ${PCL_INCLUDE_DIRS}
    )