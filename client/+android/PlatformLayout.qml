import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Android-раскладка главного экрана: ToolBar с гамбургер-меню сверху,
// Drawer вместо MenuBar, StackView вместо SplitView (список файлов /
// просмотр фото / импорт — отдельные экраны с навигацией "назад").
//
// Реализует тот же интерфейс, что и desktop-версия из родительского
// каталога (rootFolder, statusMessage, openFolderDialogRequested,
// openSettingsDialogRequested, handleBack()) — Main.qml не знает и не
// должен знать, какая из двух раскладок сейчас используется.
Item {
    id: root

    property string rootFolder: ""
    property string statusMessage: "Не подключен"
    property string pendingAction: ""

    signal openFolderDialogRequested()
    signal openSettingsDialogRequested()

    // Вызывается из Main.qml по onClosing (аппаратная/жестовая кнопка
    // "назад"). true = событие обработано здесь, окно закрывать не надо.
    function handleBack() {
        if (drawer.opened) {
            drawer.close()
            return true
        }
        if (stackView.depth > 1) {
            stackView.pop()
            return true
        }
        return false
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXs
                anchors.rightMargin: Theme.spacingXs
                spacing: Theme.spacingXs

                ToolButton {
                    text: "\u2630" // ☰
                    font.pointSize: Theme.fontSizeTitle
                    onClicked: drawer.open()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Label {
                        text: stackView.currentItem && stackView.currentItem.screenTitle
                              ? stackView.currentItem.screenTitle : "PhotoDB"
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: root.statusMessage
                        font.pointSize: Theme.fontSizeCaption
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                ToolButton {
                    text: "\u2039" // ‹
                    visible: stackView.depth > 1
                    font.pointSize: Theme.fontSizeTitle
                    onClicked: stackView.pop()
                }
            }
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: treePageComponent
        }
    }

    Drawer {
        id: drawer
        width: Math.min(root.width * 0.82, 340)
        height: root.height
        edge: Qt.LeftEdge

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingMd
            spacing: Theme.spacingXs

            Label {
                text: "Файл"
                font.bold: true
                font.pointSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            ItemDelegate {
                text: "Выбрать папку..."
                Layout.fillWidth: true
                height: 48
                onClicked: { drawer.close(); root.openFolderDialogRequested() }
            }
            ItemDelegate {
                text: "Настройки..."
                Layout.fillWidth: true
                height: 48
                onClicked: { drawer.close(); root.openSettingsDialogRequested() }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            Label {
                text: "База"
                font.bold: true
                font.pointSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            ItemDelegate {
                text: "Импорт новых файлов"
                enabled: root.rootFolder !== ""
                Layout.fillWidth: true
                height: 48
                onClicked: {
                    root.pendingAction = "scan"
                    drawer.close()
                    stackView.push(importPageComponent)
                }
            }
            ItemDelegate {
                text: "Создать недостающие миниатюры"
                enabled: root.rootFolder !== ""
                Layout.fillWidth: true
                height: 48
                onClicked: {
                    root.pendingAction = "thumbnails"
                    drawer.close()
                    stackView.push(importPageComponent)
                }
            }
            ItemDelegate {
                text: "Проверить отсутствующие файлы"
                enabled: root.rootFolder !== ""
                Layout.fillWidth: true
                height: 48
                onClicked: {
                    root.pendingAction = "missing"
                    drawer.close()
                    stackView.push(importPageComponent)
                }
            }

            Item { Layout.fillHeight: true }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }
            ItemDelegate {
                text: "Выход"
                Layout.fillWidth: true
                height: 48
                onClicked: Qt.quit()
            }
        }
    }

    // ---- Экран 1: список/дерево файлов ----
    Component {
        id: treePageComponent
        Page {
            property string screenTitle: "PhotoDB"
            TreeScreen {
                anchors.fill: parent
                onPhotoSelected: stackView.push(previewPageComponent)
            }
        }
    }

    // ---- Экран 2: просмотр фото + метаданные ----
    Component {
        id: previewPageComponent
        Page {
            property string screenTitle: "Просмотр фото"
            PreviewScreen {
                anchors.fill: parent
            }
        }
    }

    // ---- Экран 3: импорт / сканирование / поиск отсутствующих файлов ----
    Component {
        id: importPageComponent
        Page {
            property string screenTitle: root.pendingAction === "scan"
                ? "Импорт новых файлов"
                : root.pendingAction === "thumbnails"
                ? "Создание миниатюр"
                : "Проверка отсутствующих файлов"
            ImportScreen {
                anchors.fill: parent
                pendingAction: root.pendingAction
                rootFolder: root.rootFolder
                onCloseRequested: stackView.pop()
            }
        }
    }
}
