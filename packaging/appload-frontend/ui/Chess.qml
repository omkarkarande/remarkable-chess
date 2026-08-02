import QtQuick 2.5
import QtQuick.Controls 2.5
import net.asivery.AppLoad 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f7f7f2"

    signal close
    function unloading() {}

    property string pieces: "rnbqkbnrpppppppp                                PPPPPPPPRNBQKBNR"
    property string turn: "w"
    property string playerSide: "w"
    property int selected: -1
    property string pendingPromotionMove: ""
    property string message: "Connecting…"
    property string checkedSide: ""
    property string gameOver: ""
    property int skillLevel: 3
    property int boardSize: Math.max(240, Math.min(width - 88, height - 230))
    property int squareSize: Math.floor(boardSize / 8)
    property string titleText: gameOver === "checkmate" ? "CHECKMATE" : (checkedSide === turn ? "CHECK" : "CHESS")

    AppLoad {
        id: backend
        applicationID: "omi.remarkable-chess"
        onMessageReceived: (type, contents) => {
            if (type === 101) root.applyFen(contents)
            else if (type === 102) root.message = contents
            else if (type === 103) root.checkedSide = contents
            else if (type === 104) root.playerSide = contents
            else if (type === 105) root.gameOver = contents
        }
    }

    Timer {
        // Leave enough time for the player frame to become visible on e-ink.
        interval: 1500
        repeat: false
        running: root.turn !== root.playerSide && root.gameOver === ""
        onTriggered: backend.sendMessage(14, "go")
    }

    function applyFen(fen) {
        var fields = fen.split(" ")
        if (fields.length < 2) return
        var ranks = fields[0].split("/")
        if (ranks.length !== 8) return
        var next = ""
        for (var rank = 7; rank >= 0; --rank) {
            var text = ranks[7 - rank]
            for (var i = 0; i < text.length; ++i) {
                var ch = text.charAt(i)
                if (ch >= "1" && ch <= "8") {
                    for (var empty = 0; empty < Number(ch); ++empty) next += " "
                } else {
                    next += ch
                }
            }
        }
        if (next.length === 64) pieces = next
        turn = fields[1]
        selected = -1
        pendingPromotionMove = ""
        gameOver = ""
    }

    function isWhite(piece) { return piece >= "A" && piece <= "Z" }
    function row(square) { return Math.floor(square / 8) }
    function col(square) { return square % 8 }
    function at(square) { return pieces.charAt(square) }
    function sourceSquare(viewSquare) { return playerSide === "b" ? 63 - viewSquare : viewSquare }
    function viewPiece(viewSquare) { return at(sourceSquare(viewSquare)) }
    function squareName(square) { return String.fromCharCode(97 + col(square)) + String(8 - row(square)) }

    function squareIndex(notation) {
        return (Number(notation.charAt(1)) - 1) * 8 + (notation.charCodeAt(0) - 97)
    }
    function promotionViewSquare() {
        return playerSide === "b" ? 63 - squareIndex(pendingPromotionMove.substr(2, 2)) : squareIndex(pendingPromotionMove.substr(2, 2))
    }

    function isPromotionMove(fromSquare, toSquare) {
        var pawn = at(fromSquare)
        if (pawn.toUpperCase() !== "P") return false
        // QML board indices are in FEN/display order: row 0 is rank 8.
        var direction = isWhite(pawn) ? -1 : 1
        if (row(toSquare) - row(fromSquare) !== direction) return false
        if (!((isWhite(pawn) && row(toSquare) === 0) || (!isWhite(pawn) && row(toSquare) === 7))) return false
        var fileDelta = Math.abs(col(toSquare) - col(fromSquare))
        if (fileDelta === 0) return at(toSquare) === " "
        return fileDelta === 1 && at(toSquare) !== " " && isWhite(at(toSquare)) !== isWhite(pawn)
    }

    function tap(viewSquare) {
        var square = sourceSquare(viewSquare)
        if (pendingPromotionMove !== "" || gameOver !== "" || turn !== playerSide) return
        var piece = at(square)
        if (selected < 0) {
            if (piece !== " " && (isWhite(piece) ? "w" : "b") === playerSide) {
                selected = square
                message = "Select destination"
            }
            return
        }
        if (square === selected) {
            selected = -1
            return
        }
        if (piece !== " " && (isWhite(piece) ? "w" : "b") === playerSide) {
            selected = square
            message = "Select destination"
            return
        }
        var movingPiece = at(selected)
        if (isPromotionMove(selected, square)) {
            pendingPromotionMove = squareName(selected) + squareName(square)
            selected = -1
            message = "Choose a piece"
            return
        }
        backend.sendMessage(10, squareName(selected) + squareName(square))
        selected = -1
        message = "Thinking…"
    }

    function choosePromotion(choice) {
        if (pendingPromotionMove === "") return
        backend.sendMessage(10, pendingPromotionMove + choice.toLowerCase())
        pendingPromotionMove = ""
        message = "Thinking…"
    }

    function pieceSource(piece) {
        if (piece === " ") return ""
        return "pieces/" + (isWhite(piece) ? "w" : "b") + piece.toUpperCase() + ".png"
    }

    Column {
        id: gameLayout
        anchors.centerIn: parent
        width: root.boardSize
        spacing: 12

        Item {
            width: parent.width
            height: 84

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 0
                text: root.titleText
                color: "#111111"
                font.pixelSize: 42
                font.bold: true
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 47
                width: parent.width * 0.48
                text: root.message
                color: "#333333"
                font.pixelSize: 24
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }
            Column {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: 190
                spacing: 0
                Text { width: parent.width; text: "LEVEL " + root.skillLevel; color: "#222222"; font.pixelSize: 16; font.bold: true }
                Slider {
                    width: parent.width
                    height: 30
                    from: 0; to: 20; stepSize: 1; value: root.skillLevel
                    onMoved: { root.skillLevel = Math.round(value); backend.sendMessage(13, String(root.skillLevel)) }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: 14
                Item {
                    width: 40; height: 34
                    Rectangle { anchors.centerIn: parent; width: 27; height: 27; color: "transparent"; border.color: "#111111"; border.width: 2 }
                    Rectangle { anchors.centerIn: parent; width: 15; height: 2; color: "#111111" }
                    Rectangle { anchors.centerIn: parent; width: 2; height: 15; color: "#111111" }
                    MouseArea { anchors.fill: parent; onClicked: backend.sendMessage(11, "new") }
                }
                Item {
                    width: 40; height: 34
                    Image { anchors.centerIn: parent; width: 32; height: 32; source: "undo-icon.png"; fillMode: Image.PreserveAspectFit }
                    MouseArea { anchors.fill: parent; onClicked: backend.sendMessage(12, "undo") }
                }
            }
        }

        Grid {
            id: boardGrid
            width: root.squareSize * 8
            height: root.squareSize * 8
            columns: 8
            rows: 8
                Repeater {
                    model: 64
                    Rectangle {
                        width: root.squareSize
                        height: root.squareSize
                        property int source: root.sourceSquare(index)
                        property bool checkedKing: (root.checkedSide === "w" && root.at(source) === "K") || (root.checkedSide === "b" && root.at(source) === "k")
                        color: checkedKing ? "#b0b0a8" : ((Math.floor(index / 8) + index % 8) % 2 === 0 ? "#fbfbf7" : "#858580")
                        border.width: source === root.selected || checkedKing ? 7 : 2
                        border.color: source === root.selected || checkedKing ? "#111111" : "#202020"
                        Image {
                            anchors.centerIn: parent
                            width: parent.width * 0.82
                            height: parent.height * 0.82
                            source: root.pieceSource(root.at(parent.source))
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                        }
                        MouseArea { anchors.fill: parent; onClicked: root.tap(index) }
                    }
                }
            }

        Text {
            width: parent.width
            text: gameOver === "" ? "You are " + (playerSide === "w" ? "White" : "Black") + ". Tap a piece, then its destination." : gameOver === "checkmate" ? "Checkmate" : "Stalemate"
            color: "#555555"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Rectangle {
        id: promotionChooser
        z: 10
        visible: root.pendingPromotionMove !== ""
        width: root.squareSize * 2
        height: root.squareSize * 2
        property int viewSquare: root.promotionViewSquare()
        x: gameLayout.x + (root.boardSize - width) / 2
        y: gameLayout.y + 96 + (root.boardSize - height) / 2
        color: "#e8e8e1"
        border.color: "#111111"
        border.width: 4

        Grid {
            anchors.fill: parent
            columns: 2
            Repeater {
                model: ["Q", "R", "B", "N"]
                delegate: Rectangle {
                    width: root.squareSize
                    height: root.squareSize
                    color: "#fbfbf7"
                    border.color: "#111111"
                    border.width: 2
                    Image {
                        anchors.centerIn: parent
                        width: parent.width * 0.72
                        height: parent.height * 0.72
                        source: "pieces/" + (root.playerSide === "w" ? "w" : "b") + modelData + ".png"
                        fillMode: Image.PreserveAspectFit
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.choosePromotion(modelData) }
                }
            }
        }
    }
}
