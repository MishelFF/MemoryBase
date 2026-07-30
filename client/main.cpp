#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QAccessible>
#include <QQmlFileSelector>
#include "ScannerController.h"
#include "DirectoryModel.h"
#include "PhotoTreeModel.h"
#include "PhotoRepository.h"
#include "DatabaseWorker.h"
#include "ApiWorker.h"
#include "settingsmanager.h"

int main(int argc,char *argv[])
{
#ifdef Q_OS_ANDROID
    qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");
    qDebug() << qEnvironmentVariable("QT_ANDROID_DISABLE_ACCESSIBILITY");
#endif
    QGuiApplication app(argc,argv);
    QAccessible::setActive(false);
    QQmlApplicationEngine engine;
    QCoreApplication::setOrganizationName("MishelFF");
    QCoreApplication::setApplicationName("MemoryBase");
//    auto selector=new QQmlFileSelector(&engine);
//    selector->setExtraSelectors({"android"});
    
    qRegisterMetaType<PhotoRecord>("PhotoRecord");

    PhotoRepository *repository;
    bool useDBServer = CONNECTDATABASE;

#ifdef Q_OS_ANDROID

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


    
    engine.loadFromModule("PhotoDBQml","Main");
    if(engine.rootObjects().isEmpty()) 
            settingsManager.openSettingsRequested();
    

    return app.exec();
    
}