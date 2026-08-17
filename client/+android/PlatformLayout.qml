import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string rootFolder: ""
    property string statusMessage: "Не подключен"
    property bool enableAnimatedSwipe: false
    signal openFolderDialogRequested()
    signal openSettingsDialogRequested()
    AddCountryDialog { id: addCountryDialog }
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
                text: "Лица"
                font.bold: true
                font.pointSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            ItemDelegate {
                text: "Сопоставление лиц..."
                Layout.fillWidth: true
                height: 48
                onClicked: {
                    drawer.close()
                    stackView.push(facesPageComponent)
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            Label {
                text: "Места"
                font.bold: true
                font.pointSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            ItemDelegate {
                text: "Новая страна"
                Layout.fillWidth: true
                height: 48
                onClicked: {
                    drawer.close()
                    addCountryDialog.open()
                }
            }
            ItemDelegate {
                text: "Присвоить страну папке"
                enabled: root.rootFolder !== ""
                Layout.fillWidth: true
                height: 48
                onClicked: {
                    drawer.close()
                    assignCountryDialog.open()
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

    // список/дерево
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

    // просмотр фото
    Component {
        id: previewPageComponent
        Page {
            property string screenTitle: "Просмотр фото"
            PreviewScreen {
                anchors.fill: parent
                enableAnimatedSwipe: root.enableAnimatedSwipe
            }
        }
    }

   
    Dialog {
    id: assignCountryDialog
    title: "Присвоить страну папке"
    modal: true
    anchors.centerIn: parent
    standardButtons: Dialog.Ok | Dialog.Cancel

    onOpened: assignCountryCombo.currentIndex = -1

    onAccepted: {
        if (assignCountryCombo.currentIndex < 0)
            return
        var countryId = scannerController.countryList[assignCountryCombo.currentIndex].id
        scannerController.assignCountryToFolder(countryId)
    }

    ColumnLayout {
        width: Math.min(root.width * 0.85, 340)
        spacing: Theme.spacingMd

        Label {
            text: "Будет проставлено для всех фото в выбранной ветке (выделите папку в дереве перед вызовом)."
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
