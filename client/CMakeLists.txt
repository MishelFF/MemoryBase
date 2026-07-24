cmake_minimum_required(VERSION 3.21)

project(PhotoDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)
set(SOURCES

    ScannerController.cpp
    ScannerController.h

    DatabaseWorker.cpp
    DatabaseWorker.h

    ApiWorker.cpp
    ApiWorker.h

    PhotoRepository.h

    PhotoTreeModel.cpp
    PhotoTreeModel.h

    DirectoryModel.cpp
    DirectoryModel.h

    ImportTask.cpp
    ImportTask.h

    settingsmanager.cpp
    settingsmanager.h

    exif/TinyEXIF.cpp
    exif/TinyEXIF.h

    exif/tinyxml2.cpp
    exif/tinyxml2.h

    FileKey.h
    PhotoRecord.h
    main.cpp
)

if(ANDROID)
    message(STATUS "Android build")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    QuickControls2
    Sql
    Network
)
qt_policy(SET QTP0001 NEW)
qt_standard_project_setup()
qt_add_executable(PhotoDB
    ${SOURCES}
)
qt_add_qml_module(PhotoDB
    URI PhotoDB
    VERSION 1.0
    QML_FILES Main.qml SettingsDialog.qml
)

target_link_libraries(PhotoDB PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
)
