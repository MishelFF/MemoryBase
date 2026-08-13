cmake_minimum_required(VERSION 3.21)

project(PhotoDB VERSION 0.1.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)
if (MSVC)
    add_compile_options(/bigobj /MP)
endif()

include(FetchContent)

set(PHOTODB_FACE_FULL OFF)

if(DEFINED VCPKG_MANIFEST_FEATURES)
    list(FIND VCPKG_MANIFEST_FEATURES "face-full" _face_full_index)
    if(NOT _face_full_index EQUAL -1)
        set(PHOTODB_FACE_FULL ON)
    endif()
endif()

if(PHOTODB_FACE_FULL)
    message(STATUS "PhotoDB face configuration: FULL (vcpkg dlib + OpenBLAS)")
else()
    message(STATUS "PhotoDB face configuration: LITE (embedded dlib source)")
endif()

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

if(NOT ANDROID AND NOT PHOTODB_FACE_FULL)
    FetchContent_Declare(
        dlib_source
        GIT_REPOSITORY https://github.com/davisking/dlib.git
        GIT_TAG v20.0
    )
    FetchContent_GetProperties(dlib_source)
    if(NOT dlib_source_POPULATED)
        FetchContent_Populate(dlib_source)
    endif()
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
    EmbedRegionParser.cpp
    EmbedRegionParser.h
    PhotoRegion.h
    FaceRegionRecord.h
    FaceRegionsModel.cpp
    FaceRegionsModel.h
    PersonRecord.h
    PersonsModel.cpp
    PersonsModel.h
    FaceChipImageProvider.h
    PhotoSearchResultsModel.cpp
    PhotoSearchResultsModel.h
    PhotoFilter.h

    settingsmanager.cpp
    settingsmanager.h

    FileKey.h
    PhotoRecord.h
    main.cpp
)
if(NOT ANDROID)
    list(APPEND SOURCES
        FaceRecognizer.cpp
        FaceRecognizer.h
        LicenseManager.cpp
        LicenseManager.h
    )
endif()

if(ANDROID)
    message(STATUS "Android build")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    QuickControls2
    Sql
    Network
)
if(NOT ANDROID)
    find_package(unofficial-sodium CONFIG REQUIRED)
    if(PHOTODB_FACE_FULL)
        find_package(dlib CONFIG REQUIRED)
    endif()
endif()


qt_policy(SET QTP0001 NEW)
if(ANDROID)
    qt_policy(SET QTP0002 NEW)
endif()
qt_policy(SET QTP0004 NEW)

qt_standard_project_setup()
qt_add_executable(PhotoDB  ${SOURCES})

if(NOT ANDROID AND NOT PHOTODB_FACE_FULL)
    message(STATUS "Using embedded dlib source: dlib/all/source.cpp")
    target_include_directories(PhotoDB PRIVATE "${dlib_source_SOURCE_DIR}")
    target_compile_definitions(PhotoDB PRIVATE
        DLIB_NO_GUI_SUPPORT
    )
    target_sources(PhotoDB PRIVATE "${dlib_source_SOURCE_DIR}/dlib/all/source.cpp")
endif()

if(ANDROID)
    set_property(TARGET PhotoDB PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(PhotoDB
    URI PhotoDBQml
    VERSION 1.0
    QML_FILES
        Main.qml
        PlatformLayout.qml
        +android/PlatformLayout.qml
        TreeScreen.qml
        PreviewScreen.qml
        ImportScreen.qml
        SettingsDialog.qml
        Theme.qml
        LicenseDialog.qml
        FaceMatchingScreen.qml
        PhotoFilterAccordion.qml
        DateRangeFilter.qml
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
if(NOT ANDROID)
    target_link_libraries(PhotoDB PRIVATE unofficial-sodium::sodium)
        if(PHOTODB_FACE_FULL)
            target_link_libraries(PhotoDB PRIVATE dlib::dlib)    
        endif()
endif()
include(GNUInstallDirs)

if(NOT ANDROID)
    install(TARGETS PhotoDB RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(FILES
        Main.qml
        PlatformLayout.qml
        TreeScreen.qml
        PreviewScreen.qml
        ImportScreen.qml
        SettingsDialog.qml
        Theme.qml
        LicenseDialog.qml
        PhotoFilterAccordion.qml
        FaceMatchingScreen.qml
        DateRangeFilter.qml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/PhotoDB/qml)

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