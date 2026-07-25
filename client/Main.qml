import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {

    width:1700
    height:900
    visible:true
    title: "Photo Database on BOX"

    property string mediaLabel:"WD1000"
    property string rootFolder: ""
    property bool importPanelVisible: false   
    property string pendingAction: ""         
         
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
        id:dlg
        onAccepted:
        {
            let rawUrl = dlg.selectedFolder || dlg.folder;
            if (rawUrl) {
                let urlString = Qt.resolvedUrl(rawUrl).toString();
                if (urlString && urlString !== "undefined") {
                    rootFolder = urlString.replace(/^file:\/\/\/?/, "");
                    console.log("Успешно получен путь:", rootFolder);
                    statusText.text = rootFolder;
                }
            }
        }
    }
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
    Connections {
        target: scannerController 
        function onStatus(message)
        {
            console.log(message)
            statusText.text = message 
        }
    }
    menuBar: MenuBar{
        Menu
        {
            title: "Файл"
            MenuItem
            {
                text: "Выбрать папку..."
                onTriggered: dlg.open()
            }
            MenuItem
            {
                text: "Настройки..."
                onTriggered: settingsDlg.open()
            }
            MenuSeparator{}
            MenuItem
            {
                text: "Выход"
                onTriggered: Qt.quit()
            }
        }
        Menu
        {
            title: "База"
            MenuItem
            {
                text: "Импорт новых файлов"
                enabled: rootFolder !== ""
                onTriggered:{
                    pendingAction = "scan"
                    importPanelVisible = true
                }
            }
            MenuItem
            {
                text: "Создать недостающие миниатюры"
                enabled: rootFolder !== ""
                onTriggered:{
                    pendingAction = "thumbnails"
                    importPanelVisible = true
                }
            }
            MenuItem {
                text: "Проверить отсутствующие файлы"
                enabled: rootFolder !== ""
                onTriggered: {
                    pendingAction = "missing"
                    importPanelVisible = true
                }
            }
        }
    }
    SplitView{
        anchors.fill: parent
        orientation: Qt.Horizontal
        // слева дерево
        Frame{
            SplitView.preferredWidth: parent.width * 0.4
            TreeView
            {
                id: photoTreeView
                anchors.fill: parent
                clip: true
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                model:  scannerController.photoTree
                selectionBehavior: TableView.SelectRows
                selectionMode: TableView.SingleSelection
                selectionModel: ItemSelectionModel {
                        model: scannerController.photoTree
                }
                delegate: TreeViewDelegate {
                        id: treeDelegate
                        text: display
                        onClicked: {
                            var idx = photoTreeView.index(row, column)
                            console.log("Клик по строке. row=", row, "index=", idx)
                            // явно обновляем модель выбора
                            photoTreeView.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
                            var id = photoTreeView.model.photoId(idx)
                            console.log("Photo ID для листа:", id)
                            if (id > 0) {
                                scannerController.selectPhoto(id)
                            }
                        }
                }
                onExpanded: function(row){
                    var cellPoint = Qt.point(0, row)
                    var modelIdx = photoTreeView.modelIndex(cellPoint)
                    console.log("Expand node row", row ," index " ,modelIdx)
                    scannerController.photoTree.expand(modelIdx)
                }
                Connections {
                    target: photoTreeView.selectionModel
                    function onCurrentChanged(current, previous) {
                        console.log("Выделен новый элемент. QModelIndex:", current)
                //        var id = photoTreeView.model.photoId(current)
                //        console.log("Полученный Photo ID для листа:", id)
                //        if (id > 0) {
                //            console.log("Полученный Photo ID для листа:", id)
                //            scannerController.selectPhoto(id)
                //        }
                    }
                }
            }
        }
        // справа предпросмотр
        Frame
        {
            SplitView.fillWidth: true
            StackLayout {
                anchors.fill: parent
                currentIndex: importPanelVisible ? 1 : 0
                ColumnLayout {
                    spacing: 10
                    Image {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        fillMode: Image.PreserveAspectFit
                        source: scannerController.thumbnailSource
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#808080"
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                        RowLayout {
                            height: parent.height
                            spacing: 0
                            Repeater {
                                model: scannerController.photoInfo
                                delegate: Rectangle {
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 130
                                    color: index % 2 === 0 ? "#f5f5f5" : "transparent"
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 2
                                        Label {
                                            text: modelData.label
                                            font.pointSize: 8
                                            color: "#606060"
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: modelData.value
                                            font.pointSize: 9
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            ToolTip.visible: hovered
                                            ToolTip.text: modelData.value
                                            HoverHandler { id: hoverHandler }
                                            property bool hovered: hoverHandler.hovered
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                ColumnLayout {
                    spacing: 16
                    Layout.margins: 20
                    Label {
                        text: pendingAction === "scan"
                              ? "Импорт новых файлов"
                              : pendingAction === "thumbnails"
                              ? "Создание миниатюр"
                              : "Проверка отсутствующих файлов"
                        font.bold: true
                        font.pointSize: 14
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
                        Button {
                            text: "Начать"
                            enabled: mediaNameField.editText.trim() !== ""
                                && mountPointField.text.trim() !== ""
                                && !scannerController.importRunning
                            onClicked: {
                                if (pendingAction === "scan")
                                    scannerController.scanFolder(mediaNameField.editText, mountPointField.text, rootFolder)
                                else if (pendingAction === "thumbnails")
                                    scannerController.generateMissingThumbnails(mediaNameField.editText, mountPointField.text, rootFolder)
                                else if (pendingAction === "missing")
                                    scannerController.findMissingFiles(mediaNameField.editText, mountPointField.text, rootFolder)
    
                            }
                        }
                        Button {
                            text: "Закрыть"
                            enabled: !scannerController.importRunning
                            onClicked: importPanelVisible = false
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
ScrollView {
    Layout.fillWidth: true
    Layout.preferredHeight: 150
    clip: true
    visible: pendingAction === "missing" && scannerController.missingFiles.length > 0

    ColumnLayout {
        width: parent.width
        spacing: 0

        Repeater {
            model: scannerController.missingFiles

            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                color: index % 2 === 0 ? "#f5f5f5" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 8

                    Label {
                        text: modelData.path
                        font.pointSize: 8
                        color: "#606060"
                        elide: Text.ElideLeft
                        Layout.preferredWidth: 160
                    }
                    Label {
                        text: modelData.file
                        font.pointSize: 8
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}

                    Item { Layout.fillHeight: true }  // прижимает контент к верху
                }
            }
        }
    }
    footer: ToolBar{
        RowLayout
        {
            anchors.fill: parent
            Label
            {
                text: rootFolder
                Layout.fillWidth: true
            }
            Label
            {
                id: statusText
                text: "Не подключен"
            }
        }
    }
}


