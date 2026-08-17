import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // false desktop true Android
    property bool enableAnimatedSwipe: false

    // Индикатор 
    property bool navigating: false

    function goNext() {
        if (!scannerController.hasNextPhoto)
            return
        root.navigating = true
        scannerController.selectNextPhoto()
    }
    function goPrevious() {
        if (!scannerController.hasPreviousPhoto)
            return
        root.navigating = true
        scannerController.selectPreviousPhoto()
    }
    Connections {
        target: scannerController
        function onNavigationChanged() {
            root.navigating = false
        }
    }

    Image {
        id: previewImage
        x: 0
        y: 0
        width: root.width
        height: root.height // фото теперь растянуто на всю панель

        fillMode: Image.PreserveAspectFit
        source: scannerController.thumbnailSource
        Behavior on x {
            enabled: root.enableAnimatedSwipe && !swipeDragHandler.active
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    // Подпись имени файла сверху
    Rectangle {
        id: captionBar
        anchors {
            top: previewImage.top
            left: previewImage.left
            right: previewImage.right
        }
        height: captionText.implicitHeight + 12 * 2
        color: '#1c000000' 

        Text {
            id: captionText
            anchors {
                left: parent.left
                right: navIndicator.left
                verticalCenter: parent.verticalCenter
                leftMargin: 12
                rightMargin: 6
            }
            elide: Text.ElideMiddle
            color: '#030303' 
            font.pixelSize: 16
            text: scannerController.selectedPhotoFolder
                  ? scannerController.selectedPhotoFolder + " / " + scannerController.selectedPhotoName
                  : scannerController.selectedPhotoName
        }
    }

    // Индикатор мигает 
    Rectangle {
        id: navIndicator
        width: 10
        height: 10
        radius: width / 2
        color: "#FFB74D" 
        visible: root.navigating

        anchors {
            verticalCenter: captionBar.verticalCenter
            right: captionBar.right
            rightMargin: Theme.spacingMd
        }

        SequentialAnimation on opacity {
            running: root.navigating
            loops: Animation.Infinite
            NumberAnimation { from: 1.0; to: 0.25; duration: 450; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 0.25; to: 1.0; duration: 450; easing.type: Easing.InOutQuad }
        }
    }

    // Клавиатура стрелки
    Shortcut {
        sequence: "Left"
        enabled: scannerController.hasPreviousPhoto
        onActivated: root.goPrevious()
    }
    Shortcut {
        sequence: "Right"
        enabled: scannerController.hasNextPhoto
        onActivated: root.goNext()
    }

    // Свайп
    MouseArea {
        anchors.fill: previewImage
        enabled: !root.enableAnimatedSwipe

        property real startX: 0
        readonly property real threshold: 60 // px — защита от случайного клика/тапа

        onPressed: (mouse) => startX = mouse.x
        onReleased: (mouse) => {
            const dx = mouse.x - startX
            if (dx < -threshold)
                root.goNext()
            else if (dx > threshold)
                root.goPrevious()
        }
    }

    //  Свайп анимированный
    DragHandler {
        id: swipeDragHandler
        target: previewImage
        enabled: root.enableAnimatedSwipe
        xAxis.enabled: true
        yAxis.enabled: false

        readonly property real dragThreshold: 80 // px

        onActiveChanged: {
            if (active)
                return

            if (previewImage.x < -dragThreshold)
                root.goNext()
            else if (previewImage.x > dragThreshold)
                root.goPrevious()

            previewImage.x = 0 
        }
    }

    // Кнопки-стрелки 
    Rectangle {
        id: prevButton
        visible: scannerController.hasPreviousPhoto
        width: 44
        height: 44
        radius: width / 2
        color: "#66000000"

        anchors.left: previewImage.left
        anchors.bottom: previewImage.bottom
        anchors.margins: Theme.spacingMd
        anchors.bottomMargin: infoDrawer.handleHeight + Theme.spacingMd

        Text {
            anchors.centerIn: parent
            text: "\u2039" // ‹
            color: "white"
            font.pixelSize: 24
        }

        TapHandler {
            onTapped: root.goPrevious()
        }
    }

    Rectangle {
        id: nextButton
        visible: scannerController.hasNextPhoto
        width: 44
        height: 44
        radius: width / 2
        color: "#66000000"

        anchors.right: previewImage.right
        anchors.bottom: previewImage.bottom
        anchors.margins: Theme.spacingMd
        anchors.bottomMargin: infoDrawer.handleHeight + Theme.spacingMd

        Text {
            anchors.centerIn: parent
            text: "\u203A" // ›
            color: "white"
            font.pixelSize: 24
        }

        TapHandler {
            onTapped: root.goNext()
        }
    }

    Rectangle {
        id: infoDrawer

        readonly property real handleHeight: 28
        readonly property real expandedHeight: Math.min(root.height * 0.6, contentColumn.implicitHeight + handleHeight + Theme.spacingMd)
        property bool expanded: false

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: handleHeight
        color: "#CC1A1A1A" 

        Behavior on height {
            enabled: !dragHandle.active
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }

        onExpandedChanged: height = expanded ? expandedHeight : handleHeight

        Item {
            id: handleArea
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: infoDrawer.handleHeight

            Rectangle {
                id: handle
                width: 40
                height: 4
                radius: 2
                color: "#66FFFFFF"
                anchors.centerIn: parent
            }

            TapHandler {
                onTapped: infoDrawer.expanded = !infoDrawer.expanded
            }

            DragHandler {
                id: dragHandle
                target: null // высотой управляем сами, не двигаем Item
                xAxis.enabled: false
                yAxis.enabled: true

                property real startHeight: 0

                onActiveChanged: {
                    if (active) {
                        startHeight = infoDrawer.height
                    } else {
                        infoDrawer.expanded = infoDrawer.height > infoDrawer.expandedHeight * 0.35
                        infoDrawer.height = infoDrawer.expanded ? infoDrawer.expandedHeight : infoDrawer.handleHeight
                    }
                }
                onTranslationChanged: {
                    var h = startHeight - translation.y
                    infoDrawer.height = Math.max(infoDrawer.handleHeight, Math.min(infoDrawer.expandedHeight, h))
                }
            }
        }

        Flickable {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: parent.bottom
                topMargin: infoDrawer.handleHeight
                margins: Theme.spacingMd
            }
            clip: true
            visible: infoDrawer.expanded
            contentHeight: contentColumn.implicitHeight
            interactive: contentHeight > height

            Column {
                id: contentColumn
                width: parent.width
                spacing: Theme.spacingSm

                Repeater {
                    model: scannerController.photoInfo
                    delegate: Row {
                        width: contentColumn.width
                        spacing: Theme.spacingMd
                        Text {
                            width: parent.width * 0.35
                            text: modelData.label
                            color: "#AAFFFFFF"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width * 0.65 - parent.spacing
                            text: modelData.value
                            textFormat: Text.RichText
                            color: "white"
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            onLinkActivated: Qt.openUrlExternally(link)
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
                                acceptedButtons: Qt.NoButton  
                            }
                        }
                    }
                }
                RowLayout {
                    width: contentColumn.width
                    spacing: Theme.spacingMd
                    Text {
                        Layout.preferredWidth: parent.width * 0.35
                        text: "Страна"
                        color: "#AAFFFFFF"
                        font.pixelSize: 13
                    }
                    ComboBox {
                        id: photoCountryCombo
                        Layout.fillWidth: true
                        textRole: "text"
                        valueRole: "id"
                        model: [{ id: -1, text: "Не задано" }].concat(
                            scannerController.countryList.map(c => ({ id: c.id, text: c.name }))
                        )
                        Connections {
                            target: scannerController
                            function onSelectedPhotoChanged() {
                                var idx = 0
                                for (var i = 0; i < photoCountryCombo.model.length; i++) {
                                    if (photoCountryCombo.model[i].id === scannerController.selectedPhotoCountryId) {
                                        idx = i; break
                                    }
                                }
                                photoCountryCombo.currentIndex = idx
                            }
                        }
                        onActivated: scannerController.setPhotoCountry(currentValue)
                    }
                }
                Button {
                    text: "Добавить место"
                    visible: scannerController.suggestedPlaceName !== "" || /* есть GPS */ true
                    onClicked: addPlaceDialog.open()
                }
 
            }
        }
    }
                   Dialog {
                    id: addPlaceDialog
                    title: "Новое место"
                    modal: true
                    standardButtons: Dialog.Ok | Dialog.Cancel
                    onOpened: {
                        placeNameField.text = scannerController.suggestedPlaceName
                        radiusField.text = "5"
                        var idx = -1
                        for (var i = 0; i < scannerController.countryList.length; i++) {
                            if (scannerController.countryList[i].id === scannerController.suggestedCountryId) {
                                idx = i; break
                            }
                        }
                        countryCombo.currentIndex = idx
                    }
                    onAccepted: {
                        var countryId = countryCombo.currentIndex >= 0
                            ? scannerController.countryList[countryCombo.currentIndex].id
                            : -1
                        scannerController.addPlace(placeNameField.text, parseFloat(radiusField.text), countryId)
                    }
                    Column {
                        spacing: Theme.spacingMd
                        width: 300
                        TextField {
                            id: placeNameField
                            width: parent.width
                            placeholderText: "Название места"
                        }
                        TextField {
                            id: radiusField
                            width: parent.width
                            placeholderText: "Радиус, км"
                            validator: DoubleValidator { bottom: 0.1 }
                        }
                        ComboBox {
                            id: countryCombo
                            width: parent.width
                            model: scannerController.countryList
                            textRole: "name"
                            editable: true 
                            onAccepted: {
                                if (find(editText) === -1 && editText.length > 0)
                                    scannerController.addCountry(editText) // добавит в справочник
                            }
                        }
                    }
                }
}
