#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QAccessible>
#include <QQmlFileSelector>
#include <QDebug>
#include <QQuickImageProvider>

#include "ScannerController.h"
#include "DirectoryModel.h"
#include "PhotoTreeModel.h"
#include "PhotoRepository.h"
#include "DatabaseWorker.h"
#include "ApiWorker.h"
#include "settingsmanager.h"
#ifdef NEED_LICENSE
#include "licensemanager.h"
#endif
#include "FaceChipImageProvider.h"

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

#ifdef NEED_LICENSE
    LicenseManager licenseManager("http://box/mapi/"); 
    engine.rootContext()->setContextProperty("licenseManager", &licenseManager);
 //   licenseManager.checkLicense(); 
#endif
    
    ScannerController scannerController(repository,&settingsManager,nullptr);
#ifdef NEED_LICENSE
    scannerController.setlicenseManager(&licenseManager);
#endif
    engine.rootContext()->setContextProperty("scannerController",&scannerController);
    engine.addImageProvider(QLatin1String("facechip"), new FaceChipImageProvider(&scannerController));
    
    
    engine.loadFromModule("PhotoDBQml","Main");
    if(engine.rootObjects().isEmpty()) {
        qDebug() << "engine.rootObjects().isEmpty";
        //settingsManager.openSettingsRequested();
    }

    return app.exec();
    
}