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
            rootFolder = selectedFolder?.toLocalFile()?.replace(/^.:/, "") || ""
       //  rootFolder = selectedFolder.toString().replace("file:///","")
            console.log(selectedFolder)
        //    FolderTree.setRootFolder(rootFolder)
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
                onTriggered:
                    scannerController.scanFolder(rootFolder)
            }
            MenuItem
            {
                text: "Создать недостающие миниатюры"
                enabled: rootFolder !== ""
                onTriggered:
                    scannerController.generateMissingThumbnails(rootFolder)
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
            ColumnLayout
            {
                anchors.fill: parent
                spacing: 10
                Image
                {
                    id: preview
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    fillMode: Image.PreserveAspectFit
                    source: scannerController.thumbnailSource
                }
                Rectangle
                {
                    Layout.fillWidth: true
                    height: 1
                    color: "#808080"
                }
                GridLayout
                {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 6
                    Label { text: "Имя" }
                    Label { text: "" }
                    Label { text: "Дата" }
                    Label { text: "" }
                    Label { text: "Размер" }
                    Label { text: "" }
                    Label { text: "Камера" }
                    Label { text: "" }
                    Label { text: "Производитель" }
                    Label { text: "" }
                    Label { text: "Ширина" }
                    Label { text: "" }
                    Label { text: "Высота" }
                    Label { text: "" }
                    Label { text: "MD5" }
                    Label { text: "" }
                    Label { text: "GPS" }
                    Label { text: "" }
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


