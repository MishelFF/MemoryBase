pragma Singleton
import QtQuick

// Design tokens for the app. Import as `Theme` (module URI: PhotoDBQml)
// and reference e.g. `Theme.background`, `Theme.spacingMd`, `Theme.fontSizeBody`.
// Keeping these in one place means changing the palette or spacing scale
// later touches only this file, not every .qml file that uses it.
QtObject {
    // --- Palette (matches Universal Light, extend as needed) ---
    readonly property color background: "#FFFFFF"
    readonly property color surface: "#F3F3F3"
    readonly property color border: "#E0E0E0"
    readonly property color textPrimary: "#1A1A1A"
    readonly property color textSecondary: "#5C5C5C"
    readonly property color accent: "#0078D4"
    readonly property color accentPressed: "#005A9E"
    readonly property color danger: "#C42B1C"
    readonly property color success: "#0F7B0F"

    // --- Spacing scale (4px base grid) ---
    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 16
    readonly property int spacingLg: 24
    readonly property int spacingXl: 32

    // --- Corner radius (Universal is square by default; 0 keeps it native) ---
    readonly property int radiusNone: 0
    readonly property int radiusSm: 2

    // --- Typography ---
    readonly property int fontSizeCaption: 12
    readonly property int fontSizeBody: 14
    readonly property int fontSizeSubtitle: 18
    readonly property int fontSizeTitle: 24
    readonly property string fontFamily: "Segoe UI"
}
