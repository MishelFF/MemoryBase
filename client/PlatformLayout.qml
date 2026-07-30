import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Desktop-раскладка главного экрана: MenuBar сверху, дерево слева +
// просмотр/импорт справа в SplitView, статус снизу.
//
// На Android вместо этого файла Qt подставит qml/+android/PlatformLayout.qml
// (тот же интерфейс: свойства rootFolder/statusMessage, сигналы
// openFolderDialogRequested/openSettingsDialogRequested, функция handleBack()) —
// см. механизм QQmlFileSelector.
Item {
    id: root

    property string rootFolder: ""
    property string statusMessage: "Не подключен"
    property string pendingAction: ""
    property bool importPanelVisible: false
/*
    signal openFolderDialogRequested()
    signal openSettingsDialogRequested()

    // На десктопе нет аппаратной кнопки "назад" — обрабатывать нечего.
    function handleBack() {
        return false
    }

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
                        root.importPanelVisible = true
                    }
                }
                MenuItem {
                    text: "Создать недостающие миниатюры"
                    enabled: root.rootFolder !== ""
                    onTriggered: {
                        root.pendingAction = "thumbnails"
                        root.importPanelVisible = true
                    }
                }
                MenuItem {
                    text: "Проверить отсутствующие файлы"
                    enabled: root.rootFolder !== ""
                    onTriggered: {
                        root.pendingAction = "missing"
                        root.importPanelVisible = true
                    }
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
                TreeScreen { anchors.fill: parent }
            }

            // справа предпросмотр / панель импорта
            Frame {
                SplitView.fillWidth: true
                StackLayout {
                    anchors.fill: parent
                    currentIndex: root.importPanelVisible ? 1 : 0

                    PreviewScreen {}

                    ImportScreen {
                        pendingAction: root.pendingAction
                        rootFolder: root.rootFolder
                        onCloseRequested: root.importPanelVisible = false
                    }
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
    }*/
}
