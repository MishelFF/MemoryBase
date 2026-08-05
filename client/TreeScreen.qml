import QtQuick
import QtQuick.Controls

// Дерево папок/файлов базы. Общий компонент — используется и в
// desktop-раскладке (левая панель SplitView), и в Android-раскладке
// (отдельная страница StackView).
//
// Сам обновляет scannerController при выборе фото и дополнительно
// сигналит photoSelected(id) наверх — это нужно только Android-версии,
// чтобы решить, когда переключать экран (на desktop просмотр и так
// виден постоянно рядом).
TreeView {
    id: root
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
            var idx = root.index(row, column)
            root.selectionModel.setCurrentIndex(
                idx, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
            var id = root.model.photoId(idx)
            if (id > 0) {
                scannerController.selectPhoto(id)
                root.photoSelected(id)
            }
        }
    }

    onExpanded: function(row) {
        var cellPoint = Qt.point(0, row)
        var modelIdx = root.modelIndex(cellPoint)
        scannerController.photoTree.expand(modelIdx)
    }


    function revealPhoto(key) {
        if (!key)
            return

        var idx = root.model.indexForKey(key)
        if (!idx.valid)
            return

        root.expandToIndex(idx)
        root.forceLayout() // строки под новыми раскрытыми узлами должны пересчитаться перед positionViewAtRow

        var row = root.rowAtIndex(idx)
        if (row >= 0)
            root.positionViewAtRow(row, Qt.AlignVCenter)

        root.selectionModel.setCurrentIndex(
            idx, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
    }

    Connections {
        target: scannerController
        function onSelectedPhotoChanged() {
            root.revealPhoto(scannerController.selectedPhotoKey)
        }
    }
}
