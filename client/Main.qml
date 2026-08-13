import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

// Общая точка входа для desktop и Android.
// Здесь живёт то, что НЕ зависит от платформы: состояние окна,
// диалоги, связи с backend (scannerController/settingsManager).
//
// Конкретное расположение элементов на экране делегировано компоненту
// PlatformLayout — у него есть desktop-версия (этот же каталог) и
// Android-версия (каталог +android/). Qt сам выбирает нужную при
// сборке/запуске через механизм QQmlFileSelector, никаких
// if (Qt.platform.os === ...) в коде не нужно.
ApplicationWindow {
    id: window

    // Актуально только для desktop; на Android окно разворачивается
    // на весь экран автоматически.
    width: 1400
    height: 900
    visible: true
    title: "Photo Database"

    property string rootFolder: ""
    property string statusMessage: "Не подключен"

    SettingsDialog {
        id: settingsDlg
    }
    Connections {
        target: settingsManager
        function onOpenSettingsRequested() {
            settingsDlg.open()
        }
    }

    FolderDialog {
        id: rootFolderDialog
        onAccepted: {
            let rawUrl = rootFolderDialog.selectedFolder || rootFolderDialog.folder
            if (rawUrl) {
                let urlString = Qt.resolvedUrl(rawUrl).toString()
                if (urlString && urlString !== "undefined") {
                    window.rootFolder = urlString.replace(/^file:\/\/\/?/, "")
                    window.statusMessage = window.rootFolder
                }
            }
        }
    }
    LicenseDialog { id: licenseDlg }
    Connections {
        target: scannerController
        function onStatus(message) {
            window.statusMessage = message
        }
    }

    // На Android аппаратная/жестовая кнопка "назад" должна сначала
    // закрыть шторку/вернуться на предыдущий экран, а не свернуть
    // приложение. PlatformLayout сам решает, актуально ли это для
    // него (desktop-версия просто ничего не делает).
    onClosing: (close) => {
        if (platformLayout.handleBack()) {
            close.accepted = false
        }
    }

    PlatformLayout {
        id: platformLayout
        anchors.fill: parent
        rootFolder: window.rootFolder
        statusMessage: window.statusMessage
        onOpenFolderDialogRequested: rootFolderDialog.open()
        onOpenSettingsDialogRequested: settingsDlg.open()
    }
}
