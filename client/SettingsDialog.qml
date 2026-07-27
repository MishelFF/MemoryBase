import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: settingsDialog
    title: "Настройки подключения"
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: 400

    // Загружаем текущие значения при открытии
    onAboutToShow: {
        hostField.text = settingsManager.dbHost
        portField.value = settingsManager.dbPort
        dbNameField.text = settingsManager.dbName
        userField.text = settingsManager.dbUser
        passwordField.text = settingsManager.dbPassword
        apiUrlField.text = settingsManager.apiUrl
        errorLabel.text = ""
    }

    onAccepted: {
        var ok = settingsManager.saveAll(
            hostField.text, portField.value, dbNameField.text,
            userField.text, passwordField.text, apiUrlField.text
        )
        if (!ok) {
            // Диалог не закрываем — покажем ошибку и откроем заново
            settingsDialog.open()
        }
    }

    Connections {
        target: settingsManager
        function onErrorOccurred(message) {
            errorLabel.text = message
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        GridLayout {
            columns: 2
            columnSpacing: 10
            rowSpacing: 8
            Layout.fillWidth: true

            Label { text: "Хост БД:" }
            TextField { id: hostField; Layout.fillWidth: true }

            Label { text: "Порт:" }
            SpinBox { id: portField; from: 1; to: 65535; value: 5432; editable: true }

            Label { text: "Имя БД:" }
            TextField { id: dbNameField; Layout.fillWidth: true }

            Label { text: "Пользователь:" }
            TextField { id: userField; Layout.fillWidth: true }

            Label { text: "Пароль:" }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: passwordField
                    Layout.fillWidth: true
                    echoMode: showPassBtn.checked ? TextInput.Normal : TextInput.Password
                }
                Button {
                    id: showPassBtn
                    text: "👁"
                    checkable: true
                    implicitWidth: 30
                }
            }

            Label { text: "API URL:" }
            TextField {
                id: apiUrlField
                Layout.fillWidth: true
                color: settingsManager.isValidApiUrl(text) || text.length === 0
                       ? "black" : "red"
            }
        }

        Label {
            id: errorLabel
            color: "red"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}