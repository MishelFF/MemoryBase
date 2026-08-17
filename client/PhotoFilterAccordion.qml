import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal apply(var filter)

    property bool expanded: false
    implicitHeight: expanded ? summaryRow.height + detailsColumn.height : summaryRow.height

    Behavior on implicitHeight {
        NumberAnimation { duration: 120 }
    }

    property bool limitEnabled: false
    property int maxCount: 200

    property bool mediaEnabled: false
    property var selectedMedia: [] 

    property bool dateEnabled: false
    property date dateFrom: new Date()
    property date dateTo: new Date()

    property bool facesEnabled: false
    property var selectedPersonIds: [] // JS-массив int
    property bool facesUseDescriptor: false
    property real similarityThreshold: 0.6

    property string sortBy: "date" // "date" | "matchCount"

    property bool countryEnabled: false
    property int selectedCountryId: -1
    property bool placeEnabled: false
    property int selectedPlaceId: -1
    // Краткое текстовое резюме для свёрнутого состояния
    readonly property string summaryText: {
        const parts = []
        if (limitEnabled) parts.push("до " + maxCount)
        if (mediaEnabled) parts.push("Медиа: " + selectedMedia.length)
        if (dateEnabled) parts.push("Даты заданы")
        if (countryEnabled && selectedCountryId >= 0) parts.push("Страна")
        if (placeEnabled && selectedPlaceId >= 0) parts.push("Место")
        if (facesEnabled) parts.push("Лица: " + selectedPersonIds.length)
        return parts.length > 0 ? parts.join(" · ") : "Фильтр не задан"
    }
    ColumnLayout {
        width: parent.width
        spacing: 0

        RowLayout {
            id: summaryRow
            Layout.fillWidth: true

            ToolButton {
                text: root.expanded ? "▾" : "▸"
                onClicked: root.expanded = !root.expanded
            }

            Label {
                text: root.summaryText
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            id: detailsColumn
            Layout.fillWidth: true
            visible: root.expanded
            spacing: 8

            // --- Максимум фото ---
            RowLayout {
                Layout.fillWidth: true
                CheckBox {
                    checked: root.limitEnabled
                    onCheckedChanged: root.limitEnabled = checked
                }
                Label { text: "Максимум фото:" }
                SpinBox {
                    enabled: root.limitEnabled
                    from: 1
                    to: 100000
                    value: root.maxCount
                    onValueChanged: root.maxCount = value
                }
            }

            // --- Media ---
            RowLayout {
                Layout.fillWidth: true
                CheckBox {
                    checked: root.mediaEnabled
                    onCheckedChanged: root.mediaEnabled = checked
                }
                Label { text: "Медиа:" }
            }
            Flow {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                enabled: root.mediaEnabled
                opacity: root.mediaEnabled ? 1.0 : 0.5

                Repeater {
                    model: scannerController.knownMedia
                    delegate: CheckBox {
                        text: modelData
                        checked: root.selectedMedia.indexOf(modelData) !== -1
                        onCheckedChanged: {
                            const idx = root.selectedMedia.indexOf(modelData)
                            if (checked && idx === -1)
                                root.selectedMedia = root.selectedMedia.concat([modelData])
                            else if (!checked && idx !== -1) {
                                const copy = root.selectedMedia.slice()
                                copy.splice(idx, 1)
                                root.selectedMedia = copy
                            }
                        }
                    }
                }
            }

            // --- Диапазон дат ---
            RowLayout {
                Layout.fillWidth: true
                CheckBox {
                    checked: root.dateEnabled
                    onCheckedChanged: root.dateEnabled = checked
                }
                Label { text: "Даты:" }
            }
            RowLayout {
                Layout.leftMargin: 32
                enabled: root.dateEnabled
                opacity: root.dateEnabled ? 1.0 : 0.5
                DateRangeFilter {
                    Layout.fillWidth: true
                    implicitHeight: 40
                    onDateFromChanged: root.dateFrom = dateFrom
                    onDateToChanged: root.dateTo = dateTo
                }
            }
            RowLayout {
                Layout.fillWidth: true
                CheckBox {
                    checked: root.placeEnabled
                    onCheckedChanged: root.placeEnabled = checked
                }
                Label { text: "Место:" }
                ComboBox {
                    Layout.fillWidth: true
                    enabled: root.placeEnabled
                    opacity: root.placeEnabled ? 1.0 : 0.5
                    textRole: "text"
                    valueRole: "id"
                    model: [{ id: -1, text: "Любое" }].concat(
                        scannerController.placeList.map(p => ({ id: p.id, text: p.name }))
                    )
                    Component.onCompleted: currentIndex = 0
                    onActivated: root.selectedPlaceId = currentValue
                }
                CheckBox {
                    checked: root.countryEnabled
                    onCheckedChanged: root.countryEnabled = checked
                }
                Label { text: "Страна:" }
                ComboBox {
                    Layout.fillWidth: true
                    enabled: root.countryEnabled
                    opacity: root.countryEnabled ? 1.0 : 0.5
                    textRole: "text"
                    valueRole: "id"
                    model: [{ id: -1, text: "Любая" }].concat(
                        scannerController.countryList.map(c => ({ id: c.id, text: c.name }))
                    )
                    Component.onCompleted: currentIndex = 0
                    onActivated: root.selectedCountryId = currentValue
                }
            }

            // --- Место ---
//            RowLayout {
//                Layout.fillWidth: true
//            }
            // --- Лица ---
            RowLayout {
                Layout.fillWidth: true
                CheckBox {
                    checked: root.facesEnabled
                    onCheckedChanged: root.facesEnabled = checked
                }
                Label { text: "Лица (совпадение хотя бы с одним):" }
            }

            RowLayout {
                Layout.leftMargin: 32
                enabled: root.facesEnabled
                opacity: root.facesEnabled ? 1.0 : 0.5

                Switch {
                    checked: root.facesUseDescriptor
                    onCheckedChanged: root.facesUseDescriptor = checked
                }
                Label {
                    text: root.facesUseDescriptor
                        ? "По похожести (найдёт и непривязанные лица)"
                        : "Только явно привязанные регионы"
                }
            }

            RowLayout {
                Layout.leftMargin: 32
                visible: root.facesUseDescriptor
                enabled: root.facesEnabled
                opacity: root.facesEnabled ? 1.0 : 0.5

                Label { text: "Порог похожести:" }
                Slider {
                    Layout.fillWidth: true
                    from: 0.3
                    to: 1.0
                    value: root.similarityThreshold
                    onValueChanged: root.similarityThreshold = value
                }
                Label { text: root.similarityThreshold.toFixed(2) }
            }
            ListView {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.preferredHeight: 120
                enabled: root.facesEnabled
                opacity: root.facesEnabled ? 1.0 : 0.5
                clip: true
                model: scannerController.personsModel

                delegate: CheckBox {
                    width: ListView.view.width
                    text: model.displayName
                    checked: root.selectedPersonIds.indexOf(model.id) !== -1
                    onCheckedChanged: {
                        const idx = root.selectedPersonIds.indexOf(model.id)
                        if (checked && idx === -1)
                            root.selectedPersonIds = root.selectedPersonIds.concat([model.id])
                        else if (!checked && idx !== -1) {
                            const copy = root.selectedPersonIds.slice()
                            copy.splice(idx, 1)
                            root.selectedPersonIds = copy
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { text: "Сортировка:" }
                ComboBox {
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { text: "По дате", value: "date" },
                        { text: "По числу найденных лиц", value: "matchCount" },
                    ]
                    enabled: root.facesEnabled || currentValue === "date"
                    onActivated: root.sortBy = currentValue
                    Component.onCompleted: currentIndex = 0
                }
            }

            Button {
                text: "Найти"
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    const filter = {
                        limitEnabled: root.limitEnabled,
                        maxCount: root.maxCount,
                        mediaEnabled: root.mediaEnabled,
                        media: root.selectedMedia,
                        dateEnabled: root.dateEnabled,
                        dateFrom: root.dateFrom,
                        dateTo: root.dateTo,
                       countryEnabled: root.countryEnabled,
                        countryId: root.selectedCountryId,
                        placeEnabled: root.placeEnabled,
                        placeId: root.selectedPlaceId,
                        facesEnabled: root.facesEnabled,
                        personIds: root.selectedPersonIds,
                        facesUseDescriptor: root.facesUseDescriptor,
                        similarityThreshold: root.similarityThreshold,
                        sortBy: root.sortBy,
                    }
                    root.apply(filter)
                }
            }
        }
    }
}
