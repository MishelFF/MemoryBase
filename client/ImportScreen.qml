import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Панель импорта новых файлов / создания миниатюр / поиска
// отсутствующих файлов. Общий компонент — используется и в
// desktop-раскладке (панель, подменяемая StackLayout-ом), и в
// Android-раскладке (отдельная страница StackView).
//
// Самодостаточен: сам владеет диалогом выбора точки монтирования
// (mountDlg), чтобы не тянуть перекрёстные ссылки на id из
// родительских файлов — раскладки просто передают pendingAction и
// rootFolder и слушают closeRequested().
ScrollView {
    id: root
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    property string pendingAction: ""
    property string rootFolder: ""
    signal closeRequested()

    FolderDialog {
        id: mountDlg
        onAccepted: {
            let rawUrl = mountDlg.selectedFolder || mountDlg.folder
            if (rawUrl) {
                let urlString = Qt.resolvedUrl(rawUrl).toString()
                mountPointField.text = urlString.replace(/^file:\/\/\/?/, "")
            }
        }
    }

    ColumnLayout {
        width: root.width
        spacing: Theme.spacingLg

        Label {
            text: root.pendingAction === "scan"
                  ? "Импорт новых файлов"
                  : root.pendingAction === "thumbnails"
                  ? "Создание миниатюр"
                  : "Проверка отсутствующих файлов"
            font.bold: true
            font.pointSize: Theme.fontSizeTitle
        }

        Label { text: "Имя носителя (media):" }
        ComboBox {
            id: mediaNameField
            editable: true
            Layout.fillWidth: true
            model: scannerController.knownMedia
            enabled: !scannerController.importRunning
            onEditTextChanged: {
                let mp = scannerController.mountPointFor(editText)
                if (mp !== "")
                    mountPointField.text = mp
            }
        }

        Label { text: "Точка монтирования (корень носителя):" }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: mountPointField
                Layout.fillWidth: true
                enabled: !scannerController.importRunning
                placeholderText: "например, D:/ или /media/Disk0"
            }
            Button {
                text: "..."
                enabled: !scannerController.importRunning
                onClicked: mountDlg.open()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            Button {
                text: "Начать"
                Layout.fillWidth: true
                enabled: mediaNameField.editText.trim() !== ""
                    && mountPointField.text.trim() !== ""
                    && !scannerController.importRunning
                onClicked: {
                    if (root.pendingAction === "scan")
                        scannerController.scanFolder(mediaNameField.editText, mountPointField.text, root.rootFolder)
                    else if (root.pendingAction === "thumbnails")
                        scannerController.generateMissingThumbnails(mediaNameField.editText, mountPointField.text, root.rootFolder)
                    else if (root.pendingAction === "missing")
                        scannerController.findMissingFiles(mediaNameField.editText, mountPointField.text, root.rootFolder)
                }
            }
            Button {
                text: "Закрыть"
                Layout.fillWidth: true
                enabled: !scannerController.importRunning
                onClicked: root.closeRequested()
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: Math.max(scannerController.importTotal, 1)
            value: scannerController.importProcessed
            visible: scannerController.importRunning || scannerController.importTotal > 0
        }

        Label {
            text: scannerController.importTotal > 0
              ? "Обработано: %1 / %2".arg(scannerController.importProcessed).arg(scannerController.importTotal)
              : ""
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            visible: root.pendingAction === "missing" && scannerController.missingFiles.length > 0

            Repeater {
                model: scannerController.missingFiles
                delegate: Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: missingRow.implicitHeight + Theme.spacingXs * 2
                    color: index % 2 === 0 ? Theme.rowAlternate : "transparent"

                    ColumnLayout {
                        id: missingRow
                        anchors.fill: parent
                        anchors.margins: Theme.spacingXs
                        spacing: 0

                        Label {
                            text: modelData.path
                            font.pointSize: Theme.fontSizeCaption
                            color: Theme.textSecondary
                            elide: Text.ElideLeft
                            Layout.fillWidth: true
                        }
                        Label {
                            text: modelData.file
                            font.pointSize: Theme.fontSizeCaption
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
