import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root

    width: 1060
    height: 880
    minimumWidth: 900
    minimumHeight: 720
    visible: true
    title: "REWORK TIME"
    color: "#0b0e14"

    readonly property color cText: "#d7dee8"
    readonly property color cDim: "#6a7684"
    readonly property color cAccent: "#4d6bfe"
    readonly property color cGold: "#e5c07b"
    readonly property color cSilver: "#b8c4d4"
    readonly property color cBronze: "#d19a66"

    readonly property var entries: backend.top
    readonly property real maxSeconds: {
        var m = 1
        for (var i = 0; i < entries.length; ++i)
            m = Math.max(m, entries[i].seconds)
        return m
    }

    function rankColor(rank) {
        return rank === 0 ? cGold : rank === 1 ? cSilver : cBronze
    }

    // Подиум: 2 место слева, 1 в центре, 3 справа
    function slotForRank(rank) {
        return rank === 0 ? 1 : rank === 1 ? 0 : 2
    }

    // крестик прячет окно в трей (обработчик ставит C++ через Tray::attach)
    onClosing: (close) => {
        close.accepted = false
        tray.hideToTray()
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0d1220" }
            GradientStop { position: 1.0; color: "#080b12" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        // ---------- шапка ----------
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                width: 44
                height: 44
                radius: 22
                color: "#141a28"
                border.width: 2
                border.color: cAccent

                Rectangle {
                    width: 3
                    height: 11
                    radius: 2
                    color: cGold
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 9
                }
                Rectangle {
                    width: 3
                    height: 9
                    radius: 2
                    color: "#98c379"
                    x: parent.width / 2 + 3
                    y: parent.height / 2 + 2
                    rotation: 32
                    transformOrigin: Item.TopLeft
                }
            }

            ColumnLayout {
                spacing: 1
                Text {
                    text: "REWORK TIME"
                    color: cAccent
                    font.family: "Consolas"
                    font.pixelSize: 26
                    font.letterSpacing: 4
                    font.bold: true
                }
                Text {
                    text: "экранное время"
                    color: cDim
                    font.family: "Consolas"
                    font.pixelSize: 12
                    font.letterSpacing: 2
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: pauseText.implicitWidth + 36
                height: 38
                radius: 10
                color: backend.paused ? "#2a1d12" : "transparent"
                border.width: 1
                border.color: backend.paused ? cBronze : "#1b2433"

                Text {
                    id: pauseText
                    anchors.centerIn: parent
                    text: backend.paused ? "▶  дальше" : "❚❚  пауза"
                    color: backend.paused ? cGold : "#8b949e"
                    font.family: "Consolas"
                    font.pixelSize: 13
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.paused = !backend.paused
                }
            }
        }

        // ---------- периоды ----------
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [
                    { label: "день", value: 0 },
                    { label: "неделя", value: 1 },
                    { label: "месяц", value: 2 },
                    { label: "год", value: 3 }
                ]

                delegate: Rectangle {
                    required property var modelData
                    readonly property bool active: backend.period === modelData.value

                    width: label.implicitWidth + 30
                    height: 36
                    radius: 9
                    color: active ? "#26379e" : "transparent"
                    border.width: 1
                    border.color: active ? cAccent : "#1b2433"

                    Text {
                        id: label
                        anchors.centerIn: parent
                        text: modelData.label
                        color: active ? "#eaefff" : "#8b949e"
                        font.family: "Consolas"
                        font.pixelSize: 13
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.period = modelData.value
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        // ---------- сумма ----------
        Rectangle {
            Layout.fillWidth: true
            height: 104
            radius: 16
            color: "#101520"
            border.width: 1
            border.color: "#1b2433"

            Rectangle {
                width: 4
                height: parent.height - 28
                radius: 2
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                gradient: Gradient {
                    GradientStop { position: 0.0; color: cAccent }
                    GradientStop { position: 1.0; color: "#98c379" }
                }
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.leftMargin: 36
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: backend.caption
                    color: cDim
                    font.family: "Consolas"
                    font.pixelSize: 12
                    font.letterSpacing: 3
                }
                Text {
                    text: backend.total
                    color: "#f2f6fc"
                    font.family: "Consolas"
                    font.pixelSize: 42
                    font.bold: true
                }
            }
        }

        // ---------- подиум ----------
        Item {
            id: podium

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 380

            readonly property real baseY: height - 54
            readonly property real maxH: Math.max(90, baseY - 92)

            Repeater {
                model: Math.min(3, root.entries.length)

                delegate: Item {
                    id: barItem

                    required property int index
                    readonly property var entry: root.entries[index]
                    readonly property int rank: index
                    readonly property color col: root.rankColor(rank)
                    readonly property real ratio: entry ? Math.min(1, entry.seconds / root.maxSeconds) : 0
                    readonly property real barW: Math.min(180, (podium.width - 90) / 3.2)
                    readonly property real centerX: podium.width / 2 + (root.slotForRank(rank) - 1) * (barW + 36)
                    property real barH: 80 + ratio * (podium.maxH - 80)

                    Behavior on barH {
                        NumberAnimation { duration: 900; easing.type: Easing.OutCubic }
                    }

                    Rectangle {
                        width: barItem.barW + 80
                        height: barItem.barH + 40
                        x: barItem.centerX - width / 2
                        y: podium.baseY - barItem.barH + 10
                        radius: width / 2
                        opacity: 0.15
                        color: barItem.col
                    }

                    Rectangle {
                        id: column
                        width: barItem.barW
                        height: barItem.barH
                        x: barItem.centerX - width / 2
                        y: podium.baseY - height
                        radius: 14
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: barItem.col }
                            GradientStop { position: 1.0; color: Qt.darker(barItem.col, 1.6) }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            text: barItem.rank + 1
                            color: "#080b12"
                            opacity: 0.3
                            font.family: "Consolas"
                            font.pixelSize: 52
                            font.bold: true
                        }

                        // иконка: картинка из exe, а без неё — буква
                        Item {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 12
                            width: 34
                            height: 34

                            Rectangle {
                                anchors.fill: parent
                                radius: 9
                                color: "#080b12"
                                opacity: 0.35
                            }
                            Text {
                                anchors.centerIn: parent
                                text: entry && entry.app ? entry.app.charAt(0).toUpperCase() : ""
                                color: "#eaefff"
                                font.family: "Consolas"
                                font.pixelSize: 18
                                font.bold: true
                            }
                            Image {
                                anchors.fill: parent
                                smooth: true
                                fillMode: Image.PreserveAspectFit
                                source: entry && entry.icon ? entry.icon : ""
                                visible: ("" + source).length > 0
                            }
                        }
                    }

                    ColumnLayout {
                        x: barItem.centerX - barItem.barW / 2 - 22
                        y: podium.baseY + 8
                        width: barItem.barW + 44
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: barItem.entry ? barItem.entry.app : ""
                            color: root.cText
                            font.family: "Consolas"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: barItem.entry ? barItem.entry.time : ""
                            color: barItem.rank === 0 ? root.cGold : root.cSilver
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }

            // корона над первым местом
            Canvas {
                id: crown
                width: 66
                height: 50
                visible: root.entries.length > 0

                property real topBarH: root.entries.length > 0
                    ? 80 + Math.min(1, root.entries[0].seconds / root.maxSeconds) * (podium.maxH - 80)
                    : 0
                property real phase: 0

                x: podium.width / 2 - width / 2
                y: podium.baseY - topBarH - 76 + Math.sin(phase) * 4

                Behavior on topBarH {
                    NumberAnimation { duration: 900; easing.type: Easing.OutCubic }
                }

                NumberAnimation on phase {
                    from: 0
                    to: Math.PI * 2
                    duration: 2600
                    loops: Animation.Infinite
                    running: crown.visible
                }

                onPhaseChanged: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    var w = width, h = height

                    ctx.shadowColor = "rgba(229,192,123,0.55)"
                    ctx.shadowBlur = 16

                    var g = ctx.createLinearGradient(0, 0, 0, h)
                    g.addColorStop(0, "#ffe9ae")
                    g.addColorStop(1, "#e5c07b")
                    ctx.fillStyle = g
                    ctx.strokeStyle = "#fff3c4"
                    ctx.lineWidth = 1.5

                    ctx.beginPath()
                    ctx.moveTo(0, h)
                    ctx.lineTo(0, h * 0.34)
                    ctx.lineTo(w * 0.22, h * 0.64)
                    ctx.lineTo(w * 0.5, 2)
                    ctx.lineTo(w * 0.78, h * 0.64)
                    ctx.lineTo(w, h * 0.34)
                    ctx.lineTo(w, h)
                    ctx.closePath()
                    ctx.fill()
                    ctx.stroke()

                    ctx.shadowBlur = 0
                    ctx.fillStyle = "#4d6bfe"
                    ctx.beginPath()
                    ctx.arc(w * 0.5, h * 0.52, 3, 0, Math.PI * 2)
                    ctx.fill()
                    ctx.fillStyle = "#98c379"
                    ctx.beginPath()
                    ctx.arc(w * 0.2, h * 0.74, 2.4, 0, Math.PI * 2)
                    ctx.fill()
                    ctx.beginPath()
                    ctx.arc(w * 0.8, h * 0.74, 2.4, 0, Math.PI * 2)
                    ctx.fill()
                }
            }
        }

        // ---------- список ----------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            Repeater {
                model: root.entries.length

                delegate: RowLayout {
                    id: rowItem

                    required property int index
                    readonly property var entry: root.entries[index]
                    readonly property real share: entry ? entry.seconds / root.maxSeconds : 0

                    Layout.fillWidth: true
                    spacing: 12

                    Item {
                        Layout.preferredWidth: 22
                        Layout.preferredHeight: 22

                        Rectangle {
                            anchors.fill: parent
                            radius: 5
                            color: "#1b2433"
                            visible: !rowItem.entry || !rowItem.entry.icon
                        }
                        Text {
                            anchors.centerIn: parent
                            text: rowItem.entry && rowItem.entry.app ? rowItem.entry.app.charAt(0).toUpperCase() : ""
                            color: "#8fa3ff"
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.bold: true
                            visible: !rowItem.entry || !rowItem.entry.icon
                        }
                        Image {
                            anchors.fill: parent
                            smooth: true
                            fillMode: Image.PreserveAspectFit
                            source: rowItem.entry && rowItem.entry.icon ? rowItem.entry.icon : ""
                            visible: ("" + source).length > 0
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true
                                text: rowItem.entry ? rowItem.entry.app : ""
                                color: root.cText
                                font.family: "Consolas"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                            Text {
                                text: rowItem.entry ? rowItem.entry.time : ""
                                color: rowItem.index === 0 ? root.cGold : "#8fa3ff"
                                font.family: "Consolas"
                                font.pixelSize: 13
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: "#121a26"

                            Rectangle {
                                height: parent.height
                                radius: 3
                                width: parent.width * rowItem.share
                                color: rowItem.index === 0 ? root.cGold
                                     : rowItem.index === 1 ? root.cSilver
                                     : rowItem.index === 2 ? root.cBronze : root.cAccent

                                Behavior on width {
                                    NumberAnimation { duration: 700; easing.type: Easing.OutCubic }
                                }
                            }
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: backend.status
            color: "#55606d"
            font.family: "Consolas"
            font.pixelSize: 11
            elide: Text.ElideMiddle
        }
    }
}
