import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


Dialog {
    id: root
    title: "Активация лицензии"
    modal: true
    standardButtons: Dialog.Close
    // Открывшись, сразу показываем актуальное состояние (на случай,
    // если лицензия уже активирована на этой машине).
    onOpened: keyField.text = ""

    ColumnLayout {
        width: 360
        spacing: Theme.spacingMd

        Label {
            visible: licenseManager.isActivated
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: "Лицензия активна.\nКлиент: %1\nРедакция: %2\nДействует до: %3"
                .arg(licenseManager.client)
                .arg(licenseManager.edition)
                .arg(licenseManager.expires.toLocaleDateString ? licenseManager.expires.toLocaleDateString() : licenseManager.expires)
        }

        Label {
            visible: !licenseManager.isActivated
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: "Лицензия не активирована. Введите ключ активации, полученный при покупке."
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            TextField {
                id: keyField
                Layout.fillWidth: true
                enabled: !licenseManager.isChecking
                placeholderText: "XXXX-XXXX-XXXX-XXXX"
            }

            Button {
                text: licenseManager.isChecking ? "..." : "Активировать"
                enabled: !licenseManager.isChecking && keyField.text.trim() !== ""
                onClicked: licenseManager.activate(keyField.text.trim())
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: licenseManager.isChecking
            running: licenseManager.isChecking
        }

        Label {
            visible: licenseManager.lastError !== ""
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: "#c0392b"
            text: licenseManager.lastError
        }
    }

    Connections {
        target: licenseManager
        function onActivationSucceeded() {
            keyField.text = ""
        }
    }
}
