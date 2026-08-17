import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    title: "Новая страна"
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    property var bboxRows: [] 

    onOpened: {
        nameField.text = ""
        bboxRows = []
    }

    onAccepted: {
        if (nameField.text.length === 0)
            return
        scannerController.addCountryWithBBoxes(nameField.text, bboxRows)
    }

    ColumnLayout {
        width: 420
        spacing: Theme.spacingMd

        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "Название страны"
        }

        Label {
            text: "Границы (bbox) — необязательно, можно добавить несколько (материк + острова/эксклавы)"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Repeater {
            model: root.bboxRows.length
            delegate: RowLayout {
                Layout.fillWidth: true
                property int rowIndex: index

                TextField {
                    Layout.preferredWidth: 90
                    placeholderText: "lat min"
                    text: root.bboxRows[rowIndex].latMin
                    validator: DoubleValidator { bottom: -90; top: 90 }
                    onTextEdited: {
                        var copy = root.bboxRows.slice()
                        copy[rowIndex] = Object.assign({}, copy[rowIndex], { latMin: parseFloat(text) || 0 })
                        root.bboxRows = copy
                    }
                }
                TextField {
                    Layout.preferredWidth: 90
                    placeholderText: "lat max"
                    text: root.bboxRows[rowIndex].latMax
                    validator: DoubleValidator { bottom: -90; top: 90 }
                    onTextEdited: {
                        var copy = root.bboxRows.slice()
                        copy[rowIndex] = Object.assign({}, copy[rowIndex], { latMax: parseFloat(text) || 0 })
                        root.bboxRows = copy
                    }
                }
                TextField {
                    Layout.preferredWidth: 90
                    placeholderText: "lon min"
                    text: root.bboxRows[rowIndex].lonMin
                    validator: DoubleValidator { bottom: -180; top: 180 }
                    onTextEdited: {
                        var copy = root.bboxRows.slice()
                        copy[rowIndex] = Object.assign({}, copy[rowIndex], { lonMin: parseFloat(text) || 0 })
                        root.bboxRows = copy
                    }
                }
                TextField {
                    Layout.preferredWidth: 90
                    placeholderText: "lon max"
                    text: root.bboxRows[rowIndex].lonMax
                    validator: DoubleValidator { bottom: -180; top: 180 }
                    onTextEdited: {
                        var copy = root.bboxRows.slice()
                        copy[rowIndex] = Object.assign({}, copy[rowIndex], { lonMax: parseFloat(text) || 0 })
                        root.bboxRows = copy
                    }
                }

                ToolButton {
                    text: "✕"
                    onClicked: {
                        var copy = root.bboxRows.slice()
                        copy.splice(rowIndex, 1)
                        root.bboxRows = copy
                    }
                }
            }
        }

        Button {
            text: "+ добавить bbox"
            onClicked: {
                root.bboxRows = root.bboxRows.concat([{ latMin: 0, latMax: 0, lonMin: 0, lonMax: 0 }])
            }
        }
    }
}