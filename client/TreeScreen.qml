import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


// Сам обновляет scannerController при выборе фото и дополнительно
// сигналит photoSelected(id) наверх — это нужно только Android-версии,
// чтобы решить, когда переключать экран (на desktop просмотр и так
// виден постоянно рядом).

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: navTabBar
            Layout.fillWidth: true
            TabButton { text: "Дерево" }
            TabButton { text: "Поиск" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: navTabBar.currentIndex

            TreeView {
                id: root_tree
                clip: true

                signal photoSelected(int id)

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                model: scannerController.photoTree
                selectionBehavior: TableView.SelectRows
                selectionMode: TableView.SingleSelection
                selectionModel: ItemSelectionModel {
                    model: scannerController.photoTree
                }

                delegate: TreeViewDelegate {
                    id: treeDelegate
                    text: display
                    implicitHeight: 44 // высота строки с запасом под touch на Android

                    onClicked: {
                        var idx = root_tree.index(row, column)
                        root_tree.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
                        var id = root_tree.model.photoId(idx)
                        if (id > 0) {
                            scannerController.selectPhoto(id)
                            root_tree.photoSelected(id)
                        }
                    }
                }

                onExpanded: function(row) {
                    var cellPoint = Qt.point(0, row)
                    var modelIdx = root_tree.modelIndex(cellPoint)
                    scannerController.photoTree.expand(modelIdx)
                }


                function revealPhoto(key) {
                    if (!key)
                        return

                    var idx = root_tree.model.indexForKey(key)
                    if (!idx.valid)
                        return

                    root_tree.expandToIndex(idx)
                    root_tree.forceLayout() // строки под новыми раскрытыми узлами должны пересчитаться перед positionViewAtRow

                    var row = root_tree.rowAtIndex(idx) 
                    if (row >= 0)
                        root_tree.positionViewAtRow(row, Qt.AlignVCenter)

                    root_tree.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
                }

                Connections {
                    target: scannerController
                    function onSelectedPhotoChanged() {
                        root_tree.revealPhoto(scannerController.selectedPhotoKey)
                    }
                }
            }

            // Вкладка "Поиск"
            ColumnLayout {
                spacing: 0

                PhotoFilterAccordion {
                    Layout.fillWidth: true
                    onApply: function(filter) {
                        scannerController.searchPhotos(filter)
                    }
                }

                ListView {
                    id: resultsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: scannerController.searchResultsModel
                    focus: true
                    keyNavigationEnabled: true
                    keyNavigationWraps: false
                    highlightMoveDuration: 80

                    Keys.onReturnPressed: if (currentIndex >= 0) resultsList.activate(currentIndex)
                    Keys.onEnterPressed: if (currentIndex >= 0) resultsList.activate(currentIndex)
                    onCurrentIndexChanged: {
                        if (currentIndex >= 0) scannerController.selectSearchResult(currentIndex)
                    }
                   Connections {
                        target: scannerController
                        function onNavigationChanged() {
                            if (resultsList.currentIndex !== scannerController.searchListCurrentIndex)
                                resultsList.currentIndex = scannerController.searchListCurrentIndex
                        }
                    }

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        onClicked: resultsList.currentIndex = index
                         contentItem: RowLayout {
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Label {
                                    text: model.file
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: model.mediaName + " · " + model.path
                                    font.pixelSize: 11
                                    color: "#757575"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                            Label {
                                visible: model.matchCount > 0
                                text: "★ " + model.matchCount
                                color: "#757575"
                            }
                        }
                    
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: parent.count === 0
                        text: "Задайте фильтр и нажмите «Найти»"
                        color: "#9e9e9e"
                    }
                }
            }
        }
    }
}
