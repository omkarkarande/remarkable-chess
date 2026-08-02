import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 1404
    height: 1872
    title: "Chess"
    color: "#f7f7f2"

    Component.onCompleted: console.log("Chess QML geometry", width, height, boardSize, squareSize)

    readonly property int boardSize: Math.min(width - 96, height - 300)
    readonly property int squareSize: Math.floor(boardSize / 8)

    // Diagnostic base layer: this remains behind the board if child content renders.
    Rectangle {
        anchors.fill: parent
        color: "#101010"
    }

    Column {
        anchors.centerIn: parent
        spacing: 28

        Label {
            width: root.boardSize
            text: chess.turnLabel
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 42
            font.bold: true
            color: "#111111"
        }

        Grid {
            id: board
            columns: 8
            width: root.squareSize * 8
            height: root.squareSize * 8

            Repeater {
                model: 64
                delegate: Rectangle {
                    required property int index
                    readonly property int file: index % 8
                    readonly property int screenRow: Math.floor(index / 8)
                    readonly property int rank: 7 - screenRow
                    readonly property string square: String.fromCharCode(97 + file) + String(rank + 1)
                    readonly property bool selected: chess.selectedSquare === square
                    readonly property bool dark: (file + screenRow) % 2 === 1

                    width: root.squareSize
                    height: root.squareSize
                    color: selected ? "#777777" : (dark ? "#c6c6be" : "#f7f7f2")
                    border.width: 1
                    border.color: "#202020"

                    Text {
                        anchors.centerIn: parent
                        text: chess.pieceAt(file, rank)
                        font.pixelSize: Math.floor(root.squareSize * 0.72)
                        color: "#111111"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: chess.tap(file, rank)
                    }
                }
            }
        }

        Label {
            width: root.boardSize
            text: "Tap a piece, then tap its destination. Your game saves after every move."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 28
            color: "#303030"
        }
    }
}
