import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Панель ручного сопоставления лиц и выбора эталона.
// Один из вариантов правой панели в PlatformLayout (наравне с
// PreviewScreen/ImportScreen), подключается тем же способом — через
// Loader и сигнал onCloseRequested.
//
// Ожидаемый контракт с C++: всё доступно через scannerController
// (тот же объект, что используется в остальном приложении) —
//   scannerController.personsModel            — QAbstractListModel: id, displayName, hasReference
//   scannerController.unresolvedRegionsModel  — QAbstractListModel: id, photoId, faceName
//   scannerController.personRegionsModel      — QAbstractListModel: id, photoId, faceName
//                                                (те же роли, что у unresolvedRegionsModel,
//                                                 но для регионов, уже привязанных к человеку;
//                                                 наполняется вызовом loadRegionsForPerson())
//   scannerController.createPerson(displayName) -> int
//   scannerController.assignRegionToPerson(regionId, personId)
//   scannerController.setPersonReference(personId, regionId)
//   scannerController.unassignRegion(regionId)
//   scannerController.loadRegionsForPerson(personId) — асинхронно наполняет personRegionsModel
// image provider "facechip":
//   image://facechip/region/<id>  — face_chip региона
//   image://facechip/person/<id>  — reference_chip человека
Item {
    id: root

    signal closeRequested()

    property int selectedPersonId: -1
    property int selectedRegionId: -1

    // Кнопки действий внизу активны только когда выбраны обе стороны
    // (или хотя бы одна — для unassign/reference на существующей записи)
    readonly property bool bothSelected: selectedPersonId !== -1 && selectedRegionId !== -1

    onSelectedPersonIdChanged: {
        if (selectedPersonId !== -1) {
            scannerController.loadRegionsForPerson(selectedPersonId)
        } else if (regionsTabBar.currentIndex === 1) {
            // Человек снят с выбора — на вкладке "Фото человека" смотреть
            // больше не на что, возвращаемся к неопознанным
            regionsTabBar.currentIndex = 0
        }
    }

    // Общий делегат для обеих сеток регионов (неопознанные / привязанные
    // к человеку) — вынесен наверх, чтобы не дублировать разметку
    Component {
        id: regionDelegate

        ItemDelegate {
            width: GridView.view.cellWidth - 4
            height: GridView.view.cellHeight - 4
            highlighted: model.id === root.selectedRegionId
            onClicked: root.selectedRegionId = model.id

            contentItem: ColumnLayout {
                spacing: 2

                Image {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    Layout.alignment: Qt.AlignHCenter
                    fillMode: Image.PreserveAspectCrop
                    source: "image://facechip/region/" + model.id
                }

                Label {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    text: model.faceName !== "" ? model.faceName : "—"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Label {
                    text: "Сопоставление лиц"
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: "Новый человек..."
                    onClicked: newPersonDialog.open()
                }
                Button {
                    text: "Закрыть"
                    onClicked: root.closeRequested()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            // Слева — справочник людей. Клик выбирает человека для
            // последующего назначения или установки эталона.
            Frame {
                SplitView.preferredWidth: parent.width * 0.4

                ColumnLayout {
                    anchors.fill: parent

                    Label {
                        text: "Люди"
                        font.bold: true
                    }

                    ListView {
                        id: peopleList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: scannerController.personsModel

                        delegate: ItemDelegate {
                            width: peopleList.width
                            highlighted: model.id === root.selectedPersonId
                            onClicked: root.selectedPersonId = model.id

                            contentItem: RowLayout {
                                spacing: 8

                                Image {
                                    Layout.preferredWidth: 48
                                    Layout.preferredHeight: 48
                                    fillMode: Image.PreserveAspectCrop
                                    source: model.hasReference
                                        ? "image://facechip/person/" + model.id
                                        : ""
                                    // Заглушка, пока эталон не выбран — иначе пустое
                                    // место в списке выглядит как ошибка загрузки
                                    Rectangle {
                                        anchors.fill: parent
                                        visible: !model.hasReference
                                        color: "#e0e0e0"
                                        border.color: "#bdbdbd"
                                        Label {
                                            anchors.centerIn: parent
                                            text: "?"
                                            color: "#9e9e9e"
                                        }
                                    }
                                }

                                Label {
                                    text: model.displayName
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            // Справа — две вкладки над одним и тем же местом:
            // неопознанные регионы (person_id IS NULL) и уже привязанные
            // к выбранному человеку. Вкладки вместо третьей колонки —
            // так остаётся место под миниатюры нормального размера,
            // а не полоска из двух-трёх штук в ряд.
            Frame {
                SplitView.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    TabBar {
                        id: regionsTabBar
                        Layout.fillWidth: true

                        TabButton {
                            text: "Неопознанные"
                        }
                        TabButton {
                            text: "Фото человека"
                            enabled: root.selectedPersonId !== -1
                        }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: regionsTabBar.currentIndex
                        onCurrentIndexChanged: {
                            if (currentIndex === 0) {
                                scannerController.loadUnresolvedRegions()
                            } else if (currentIndex === 1) {
                                scannerController.loadRegionsForPerson(root.selectedPersonId)
                            }
                        }    
                        // Неопознанные лица — сетка миниатюр, чтобы быстро
                        // окинуть взглядом много лиц сразу.
                        GridView {
                            id: regionsGrid
                            clip: true
                            cellWidth: 96
                            cellHeight: 112
                            model: scannerController.unresolvedRegionsModel
                            delegate: regionDelegate
                        }

                        // Лица, уже привязанные к выбранному человеку —
                        // удобно проверить, что распознавание не ошиблось,
                        // и при необходимости открепить лишнее или сменить эталон.
                        GridView {
                            id: personRegionsGrid
                            clip: true
                            cellWidth: 96
                            cellHeight: 112
                            model: scannerController.personRegionsModel
                            delegate: regionDelegate

                            Label {
                                anchors.centerIn: parent
                                visible: root.selectedPersonId === -1
                                text: "Выберите человека слева"
                                color: "#9e9e9e"
                            }
                        }
                    }
                }
            }
        }

        // Действия — активны в зависимости от того, что реально выбрано
        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent

                Button {
                    text: "Назначить выбранному человеку"
                    enabled: root.bothSelected
                    onClicked: {
                        scannerController.assignRegionToPerson(root.selectedRegionId, root.selectedPersonId)
                        root.selectedRegionId = -1
                        // Регион переехал из "Неопознанных" в подборку человека —
                        // обновляем вкладку "Фото человека", даже если она сейчас не активна
                        scannerController.loadRegionsForPerson(root.selectedPersonId)
                    }
                }

                Button {
                    text: "Сделать эталоном"
                    enabled: root.bothSelected
                    onClicked: scannerController.setPersonReference(root.selectedPersonId, root.selectedRegionId)
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Это не лицо / пропустить"
                    enabled: root.selectedRegionId !== -1
                    onClicked: {
                        scannerController.unassignRegion(root.selectedRegionId)
                        root.selectedRegionId = -1
                        // Могли откреплять и с вкладки "Фото человека" —
                        // подчищаем список, если человек выбран
                        if (root.selectedPersonId !== -1)
                            scannerController.loadRegionsForPerson(root.selectedPersonId)
                    }
                }
            }
        }
    }

    Dialog {
        id: newPersonDialog
        title: "Новый человек"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent

        onOpened: nameField.text = ""
        onAccepted: {
            if (nameField.text.trim().length > 0) {
                // createPerson асинхронный — id придёт через сигнал репозитория
                // и появится в personsModel сам, синхронно его тут не получить.
                // Автовыбор нового человека не делаем; при необходимости можно
                // добавить сигнал personCreated(id) на ScannerController и
                // Connections{} здесь, чтобы выделить его в списке автоматически.
                scannerController.createPerson(nameField.text.trim())
            }
        }

        TextField {
            id: nameField
            width: 260
            placeholderText: "Имя и фамилия"
        }
    }
}
