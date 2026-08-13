import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// ввод даты в PhotoFilterAccordion. Взято целиком
Item {
    id: root

    // На вход/выход — обычные JS Date 
    property date dateFrom: new Date()
    property date dateTo: new Date()

    readonly property var monthNames: ["Январь", "Февраль", "Март", "Апрель", "Май", "Июнь","Июль", "Август", "Сентябрь", "Октябрь", "Ноябрь", "Декабрь"]

    property int precision: 0 // 0 - Год, 1 - Месяц, 2 - Период

    property int yearValue: new Date().getFullYear()
    property int monthValue: new Date().getMonth() // 0..11

    property int fromYear: new Date().getFullYear()
    property int fromMonth: 1 // 1..12, для симметрии с "человеческим" вводом
    property int fromDay: 1
    property int toYear: new Date().getFullYear()
    property int toMonth: 12
    property int toDay: 31

    function recompute() {
        if (precision === 0) {
            dateFrom = new Date(yearValue, 0, 1, 0, 0, 0)
            dateTo = new Date(yearValue, 11, 31, 23, 59, 59)
        } else if (precision === 1) {
            dateFrom = new Date(yearValue, monthValue, 1, 0, 0, 0)
            dateTo = new Date(yearValue, monthValue + 1, 0, 23, 59, 59)
        } else {
            dateFrom = new Date(fromYear, fromMonth - 1, fromDay, 0, 0, 0)
            dateTo = new Date(toYear, toMonth - 1, toDay, 23, 59, 59)
        }
    }

    Component.onCompleted: recompute()

    RowLayout {
        anchors.fill: parent
        spacing: 8

        ComboBox {
            id: precisionCombo
            model: ["Год", "Месяц", "Период"]
            currentIndex: root.precision
            onActivated: {
                root.precision = currentIndex
                root.recompute()
            }
        }

        // Год 
        RowLayout {
            visible: root.precision === 0
            SpinBox {
                from: 1970
                to: new Date().getFullYear() + 1
                value: root.yearValue
                editable: true
                onValueChanged: { root.yearValue = value; root.recompute() }
            }
        }

        // Месяц
        RowLayout {
            visible: root.precision === 1
            ComboBox {
                model: root.monthNames
                currentIndex: root.monthValue
                onActivated: { root.monthValue = currentIndex; root.recompute() }
            }
            SpinBox {
                from: 1970
                to: new Date().getFullYear() + 1
                value: root.yearValue
                editable: true
                onValueChanged: { root.yearValue = value; root.recompute() }
            }
        }

        // Период
        RowLayout {
            visible: root.precision === 2
            spacing: 4

            Label { text: "с" }
            SpinBox {
                implicitWidth: 60
                from: 1; to: 31
                value: root.fromDay
                onValueChanged: { root.fromDay = value; root.recompute() }
            }
            SpinBox {
                implicitWidth: 60
                from: 1; to: 12
                value: root.fromMonth
                onValueChanged: { root.fromMonth = value; root.recompute() }
            }
            SpinBox {
                implicitWidth: 80
                from: 1970; to: new Date().getFullYear() + 1
                value: root.fromYear
                editable: true
                onValueChanged: { root.fromYear = value; root.recompute() }
            }

            Label { text: "по" }
            SpinBox {
                implicitWidth: 60
                from: 1; to: 31
                value: root.toDay
                onValueChanged: { root.toDay = value; root.recompute() }
            }
            SpinBox {
                implicitWidth: 60
                from: 1; to: 12
                value: root.toMonth
                onValueChanged: { root.toMonth = value; root.recompute() }
            }
            SpinBox {
                implicitWidth: 80
                from: 1970; to: new Date().getFullYear() + 1
                value: root.toYear
                editable: true
                onValueChanged: { root.toYear = value; root.recompute() }
            }
        }
    }
}
