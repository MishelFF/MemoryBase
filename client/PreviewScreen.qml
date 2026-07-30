import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Просмотр выбранной фотографии и её метаданных.
// Общий компонент — используется и в desktop-раскладке (правая панель
// SplitView), и в Android-раскладке (отдельная страница StackView).
//
// Метаданные показаны вертикальным списком "подпись/значение" —
// такое представление одинаково хорошо работает и в узкой мобильной
// колонке, и в широкой desktop-панели (в отличие от горизонтальной
// ленты, которая на телефоне потребовала бы скролла пальцем вбок).
ColumnLayout {
    id: root
    spacing: Theme.spacingMd

    Image {
        Layout.fillWidth: true
        Layout.preferredHeight: root.height * 0.55
        fillMode: Image.PreserveAspectFit
        source: scannerController.thumbnailSource
    }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Theme.divider
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: root.width
            spacing: 0

            Repeater {
                model: scannerController.photoInfo
                delegate: Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: rowLayout.implicitHeight + Theme.spacingSm * 2
                    color: index % 2 === 0 ? Theme.rowAlternate : "transparent"

                    RowLayout {
                        id: rowLayout
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSm
                        spacing: Theme.spacingSm

                        Label {
                            text: modelData.label
                            font.pointSize: Theme.fontSizeCaption
                            color: Theme.textSecondary
                            Layout.preferredWidth: parent.width * 0.4
                            wrapMode: Text.Wrap
                        }
                        Label {
                            text: modelData.value
                            font.pointSize: Theme.fontSizeSmall
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
