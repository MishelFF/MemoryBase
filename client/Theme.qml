pragma Singleton
import QtQuick

// Design tokens for the app. Import as `Theme` (module URI: PhotoDBQml)
// and reference e.g. `Theme.background`, `Theme.spacingMd`, `Theme.fontSizeBody`.
// Keeping these in one place means changing the palette or spacing scale
// later touches only this file, not every .qml file that uses it.
QtObject {
    // --- Palette (Universal Light) ---
    readonly property color background: "#FFFFFF"
    readonly property color rowAlternate: "#F5F5F5"
    readonly property color divider: "#808080"
    readonly property color textPrimary: "#000000"
    readonly property color textSecondary: "#606060"
    readonly property color accent: "#0078D4"
    readonly property color accentPressed: "#005A9E"
    readonly property color danger: "#D32F2F"
    readonly property color success: "#0F7B0F"

    // --- Spacing scale ---
    readonly property int spacingXxs: 2
    readonly property int spacingXs: 6
    readonly property int spacingSm: 8
    readonly property int spacingMd: 10
    readonly property int spacingLg: 16
    readonly property int spacingXl: 20

    // --- Typography (font.pointSize) ---
    readonly property int fontSizeCaption: 8
    readonly property int fontSizeSmall: 9
    readonly property int fontSizeTitle: 14
}
