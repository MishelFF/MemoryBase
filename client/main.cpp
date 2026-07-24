#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include "ScannerController.h"
#include "DirectoryModel.h"
#include "PhotoTreeModel.h"
#include "PhotoRepository.h"
#include "DatabaseWorker.h"
#include "ApiWorker.h"
#include "settingsmanager.h"

int main(int argc,char *argv[])
{
    QGuiApplication app(argc,argv);
    QQmlApplicationEngine engine;
    QCoreApplication::setOrganizationName("MishelMF");
    QCoreApplication::setApplicationName("MemoryBase");
    
    qRegisterMetaType<PhotoRecord>("PhotoRecord");

    PhotoRepository *repository;
    bool useDBServer = CONNECTDATABASE;

#ifdef Q_OS_ANDROID

    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    repository= new ApiWorker;

#else

    if(useDBServer){
        repository = new DatabaseWorker();
    }
    else{
        repository = new ApiWorker();
    }

#endif

    SettingsManager settingsManager;
    engine.rootContext()->setContextProperty("settingsManager", &settingsManager);

    ScannerController scannerController(repository,&settingsManager,nullptr);

    engine.rootContext()->setContextProperty("scannerController",&scannerController);


    
    engine.loadFromModule("PhotoDB","Main");
    if(engine.rootObjects().isEmpty()) 
            settingsManager.openSettingsRequested();
    

    return app.exec();
    
}