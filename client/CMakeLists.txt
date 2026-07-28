cmake_minimum_required(VERSION 3.21)

project(PhotoDB VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

include(FetchContent)

FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 10.0.0
)
FetchContent_MakeAvailable(tinyxml2)
set(BUILD_DEMO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    TinyEXIF
    GIT_REPOSITORY https://github.com/cdcseacave/TinyEXIF.git
    GIT_TAG master
)
FetchContent_GetProperties(tinyexif)
if(NOT tinyexif_POPULATED)
    FetchContent_Populate(tinyexif)
endif()

add_library(TinyEXIF STATIC
    ${tinyexif_SOURCE_DIR}/TinyEXIF.cpp
    ${tinyexif_SOURCE_DIR}/TinyEXIF.h
)
target_include_directories(TinyEXIF PUBLIC ${tinyexif_SOURCE_DIR})
if(TARGET tinyxml2)
    target_link_libraries(TinyEXIF PUBLIC tinyxml2)
else()
    target_link_libraries(TinyEXIF PUBLIC tinyxml2_static)
endif()

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
if(ANDROID)
    qt_policy(SET QTP0002 NEW)
endif()
qt_standard_project_setup()
qt_add_executable(PhotoDB
    ${SOURCES}
)

if(ANDROID)
    set_property(TARGET PhotoDB PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()

qt_add_qml_module(PhotoDB
    URI PhotoDBQml
    VERSION 1.0
    QML_FILES Main.qml SettingsDialog.qml Theme.qml
)

# qtquickcontrols2.conf должен лежать в корне ресурсов (":/qtquickcontrols2.conf"),
# именно там Qt Quick Controls ищет его при старте — отдельно от префикса QML-модуля.
qt_add_resources(PhotoDB "app_config"
    PREFIX "/"
    FILES qtquickcontrols2.conf
)

# Не открывать консольное окно при запуске на Windows (обычный GUI-subsystem exe).
# На остальных платформах это свойство ни на что не влияет.
set_target_properties(PhotoDB PROPERTIES
    WIN32_EXECUTABLE ON
)

target_link_libraries(PhotoDB PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
    TinyEXIF
)
include(GNUInstallDirs)

if(NOT ANDROID)
    install(TARGETS PhotoDB RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(FILES Main.qml SettingsDialog.qml DESTINATION ${CMAKE_INSTALL_DATADIR}/PhotoDB/qml)

    # QSql PSQL-драйвер (Qt6::Sql) грузит libpq динамически через QLibrary в рантайме,
    # windeployqt эту зависимость не видит и не копирует — тащим её из vcpkg сами.
    if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
        set(_VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
        if(EXISTS "${_VCPKG_BIN_DIR}")
            file(GLOB _PQ_RUNTIME_DLLS
                "${_VCPKG_BIN_DIR}/libpq.dll"
                "${_VCPKG_BIN_DIR}/libcrypto*.dll"
                "${_VCPKG_BIN_DIR}/libssl*.dll"
                "${_VCPKG_BIN_DIR}/libiconv*.dll"
                "${_VCPKG_BIN_DIR}/zlib1.dll"
            )
            install(FILES ${_PQ_RUNTIME_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})
        else()
            message(WARNING "vcpkg bin dir not found at ${_VCPKG_BIN_DIR} — libpq runtime won't be packaged")
        endif()
    endif()

    include(InstallRequiredSystemLibraries)

    set(CPACK_PACKAGE_NAME "PhotoDB")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_FILE_NAME "PhotoDB-${PROJECT_VERSION}-win64")
    set(CPACK_GENERATOR "ZIP")

    set(CPACK_INSTALLED_DIRECTORIES "${CMAKE_INSTALL_PREFIX};/")

    include(CPack)
endif()