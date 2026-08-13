import QtQuick
import QtQuick.Controls

// Просмотр текущей фотографии. Общий компонент — desktop и Android.
//
// Фото растянуто на всю область компонента (панель метаданных снизу —
// теперь выезжающая шторка поверх фото, а не отдельная секция под ним).
//
// Навигация между фото реализована пятью независимыми входными
// точками, все идут через goNext()/goPrevious() ниже, которые уже
// сами дергают scannerController.selectNextPhoto()/selectPreviousPhoto()
// и включают индикатор "идёт работа":
//   - стрелки клавиатуры (Shortcut, работают на обеих платформах)
//   - свайп по изображению — способ зависит от enableAnimatedSwipe
//     (задаётся снаружи, из PlatformLayout конкретной платформы)
//   - кнопки-стрелки поверх фото
Item {
    id: root

    // false (по умолчанию, desktop) — простой свайп без анимации:
    //   MouseArea, порог по расстоянию, реакция только на отпускание.
    // true (Android, задаётся в +android/PlatformLayout.qml) —
    //   анимированный свайп: фото едет за пальцем через DragHandler,
    //   с плавным возвратом/анимацией после отпускания.
    property bool enableAnimatedSwipe: false

    // Индикатор "идёт работа": включается в момент запроса навигации,
    // выключается по приходу navigationChanged от scannerController
    // (сигнал стреляет, когда посчитаны новые hasNextPhoto/hasPreviousPhoto
    // — то есть когда переход реально завершился).
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

        // x — обычная (не anchors) привязка: анимированному варианту
        // свайпа (DragHandler) нужно двигать x во время
        // перетаскивания, а anchors.left/right этому бы мешали
        // (anchors постоянно принудительно возвращали бы x к 0).
        x: 0
        y: 0
        width: root.width
        height: root.height // фото теперь растянуто на всю панель

        fillMode: Image.PreserveAspectFit
        source: scannerController.thumbnailSource

        // Анимированный возврат на место после отпускания свайпа —
        // активен только пока сам DragHandler не тянет (иначе Behavior
        // будет "тормозить" палец во время самого перетаскивания).
        Behavior on x {
            enabled: root.enableAnimatedSwipe && !swipeDragHandler.active
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    // ---- Подпись папки/имени файла сверху, поверх фото ----
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

    // ---- Индикатор "идёт работа" — огонёк в углу ----
    // Виден и мигает с момента нажатия кнопки/свайпа/стрелки до
    // прихода navigationChanged от контроллера.
    Rectangle {
        id: navIndicator
        width: 10
        height: 10
        radius: width / 2
        color: "#FFB74D" // янтарный — "идёт работа"
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

    // ---- Клавиатура: стрелки влево/вправо ----
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

    // ---- Свайп, вариант A: простой (desktop по умолчанию) ----
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

    // ---- Свайп, вариант B: анимированный (Android) ----
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

            previewImage.x = 0 // анимируется через Behavior on x выше
        }
    }

    // ---- Кнопки-стрелки поверх фото (снизу, полупрозрачные) ----
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

    // ---- Шторка метаданных снизу ----
    // По умолчанию скрыта — виден только узкий "хвостик"-ручка внизу
    // экрана. Тап по ручке разворачивает шторку вверх поверх фото
    // (фото при этом не сжимается — оно всегда на всю панель, шторка
    // просто лежит выше по z).
    //
    // ВНИМАНИЕ: делегат Repeater ниже предполагает, что каждый элемент
    // scannerController.photoInfo — это объект вида {label, value}.
    // Если реальная структура QVariantList другая — поправьте роли в
    // delegate под неё.
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
        // height не привязан напрямую к expanded — во время драга им
        // управляет DragHandler императивно (та же схема, что и у
        // previewImage.x при свайпе выше). expanded лишь фиксирует
        // конечное состояние после отпускания/тапа.
        height: handleHeight
        color: "#CC1A1A1A" // тёмный, полупрозрачный — читаемо поверх любого фото

        Behavior on height {
            enabled: !dragHandle.active
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }

        onExpandedChanged: height = expanded ? expandedHeight : handleHeight

        // ---- Зона ручки: вся полоска handleHeight по всей ширине, а
        // не только видимый "хвостик" — иначе за реальную ручку
        // попасть пальцем/курсором почти нереально.
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
                        // Довожение: больше трети раскрытой высоты — открываем полностью, иначе закрываем.
                        infoDrawer.expanded = infoDrawer.height > infoDrawer.expandedHeight * 0.35
                        infoDrawer.height = infoDrawer.expanded ? infoDrawer.expandedHeight : infoDrawer.handleHeight
                    }
                }
                onTranslationChanged: {
                    // translation.y растёт вниз при движении пальца вниз;
                    // тянем вверх (translation.y < 0) — высота должна расти.
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
                            color: "white"
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
