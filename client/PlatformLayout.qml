import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// Desktop-раскладка главного экрана: MenuBar сверху, дерево слева +
// просмотр/импорт справа в SplitView, статус снизу.
//
// На Android вместо этого файла Qt подставит +android/PlatformLayout.qml
// (тот же интерфейс: свойства rootFolder/statusMessage, сигналы
// openFolderDialogRequested/openSettingsDialogRequested, функция handleBack()) —
// см. механизм QQmlFileSelector.
Item {
    id: root

    property string rootFolder: ""
    property string statusMessage: "Не подключен"
    property string pendingAction: ""
    property bool importPanelVisible: false
    property bool facesPanelVisible: false

    signal openFolderDialogRequested()
    signal openSettingsDialogRequested()

    // На десктопе нет аппаратной кнопки "назад" — обрабатывать нечего.
    function handleBack() {
        return false
    }
    AddCountryDialog { id: addCountryDialog }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        MenuBar {
            Layout.fillWidth: true

            Menu {
                title: "Файл"
                MenuItem {
                    text: "Выбрать папку..."
                    onTriggered: root.openFolderDialogRequested()
                }
                MenuItem {
                    text: "Настройки..."
                    onTriggered: root.openSettingsDialogRequested()
                }
                MenuItem { 
                    text: "Активация лицензии..."
                    onTriggered: licenseDialog.open()
                }
                MenuSeparator {}
                MenuItem {
                    text: "Выход"
                    onTriggered: Qt.quit()
                }
            }
            Menu {
                title: "База"
                MenuItem {
                    text: "Импорт новых файлов"
                    enabled: root.rootFolder !== ""
                    onTriggered: {
                        root.pendingAction = "scan"
                        root.facesPanelVisible = false
                        root.importPanelVisible = true
                    }
                }
                MenuItem {
                    text: "Создать недостающие миниатюры"
                    enabled: root.rootFolder !== ""
                    onTriggered: {
                        root.pendingAction = "thumbnails"
                        root.facesPanelVisible = false
                        root.importPanelVisible = true
                    }
                }
                MenuItem {
                    text: "Проверить отсутствующие файлы"
                    enabled: root.rootFolder !== ""
                    onTriggered: {
                        root.pendingAction = "missing"
                        root.facesPanelVisible = false
                        root.importPanelVisible = true
                    }
                }
                }
            Menu {
                title: "Лица"
                MenuItem {
                    text: "Сопоставление лиц..."
                    onTriggered: {
                         root.importPanelVisible = false
                         root.facesPanelVisible = true
                    }
                }
            }
            Menu {
                title: "Места"
                MenuItem {
                    text: "Новая страна"
                    onTriggered: addCountryDialog.open()
                }
                MenuItem {
                    text: "Присвоить страну выделенной папке…"
                    onTriggered: assignCountryDialog.open()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            // слева дерево
            Frame {
                SplitView.preferredWidth: parent.width * 0.4
                TreeScreen {
                    id: treeScreen
                    anchors.fill: parent
                }
            }

            // справа предпросмотр / панель импорта/ панель лиц.
            
            Frame {
                SplitView.fillWidth: true
                Loader {
                    anchors.fill: parent
                    sourceComponent: root.facesPanelVisible ? facesComponent : (root.importPanelVisible ? importComponent : previewComponent)
                }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Label {
                    text: root.rootFolder
                    Layout.fillWidth: true
                }
                Label {
                    text: root.statusMessage
                }
            }
        }
    }

    Component {
        id: previewComponent
        PreviewScreen {}
    }

    Component {
        id: importComponent
        ImportScreen {
            pendingAction: root.pendingAction
            rootFolder: root.rootFolder
            onCloseRequested: root.importPanelVisible = false
        }
    }
    Component {
        id: facesComponent
        FaceMatchingScreen {
            onCloseRequested: root.facesPanelVisible = false
        }
    }

    MessageDialog {
        id: messageDialog
    }

    Connections {
        target: licenseManager

        function onActivationFailed(message) {
            messageDialog.title = "Ошибка"
            messageDialog.text = message
            messageDialog.open()
        }

        function onActivationSucceeded() {
            messageDialog.title = "Успешно"
            messageDialog.text = "Лицензия активирована."
            messageDialog.open()
        }
        function onActivationStateChanged() {
        licenseDialog.title = "Лицензия"

        if (licenseManager.isActivated)
            licenseDialog.text = "Лицензия успешно активирована."
        else
            licenseDialog.text = "Лицензия не найдена или недействительна."

        licenseDialog.open()
        }
    }
    Dialog {
    id: assignCountryDialog
    title: "Присвоить страну папке"
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Ok | Dialog.Cancel

    onOpened: assignCountryCombo.currentIndex = -1

    onAccepted: {
        if (assignCountryCombo.currentIndex < 0) return
        var countryId = scannerController.countryList[assignCountryCombo.currentIndex].id
        scannerController.assignCountryToFolder(countryId)
    }

    ColumnLayout {
        width: 320
        spacing: Theme.spacingMd

        Label {
            text: "Будет проставлено для всех фото в выбранной ветке."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ComboBox {
            id: assignCountryCombo
            Layout.fillWidth: true
            model: scannerController.countryList
            textRole: "name"
        }
    }
    }
}
