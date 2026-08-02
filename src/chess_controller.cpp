#include "chess_controller.h"

#include <QDir>
#include <QStandardPaths>

namespace {
std::filesystem::path gameSavePath() {
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return std::filesystem::path((QDir(configRoot).filePath("game.fen")).toStdString());
}
}  // namespace

ChessController::ChessController(QObject* parent) : QObject(parent), session_(gameSavePath()) {}

QString ChessController::selectedSquare() const {
    return QString::fromStdString(session_.selectedSquare());
}

QString ChessController::turnLabel() const {
    return session_.sideToMove() == Color::White ? "White to move" : "Black to move";
}

QString ChessController::pieceAt(int file, int rank) const {
    return glyphFor(session_.pieceAt(notationFor(file, rank).toStdString()));
}

void ChessController::tap(int file, int rank) {
    session_.tap(notationFor(file, rank).toStdString());
    emit stateChanged();
}

QString ChessController::notationFor(int file, int rank) {
    if (file < 0 || file > 7 || rank < 0 || rank > 7) {
        return {};
    }
    return QString(QChar('a' + file)) + QString::number(rank + 1);
}

QString ChessController::glyphFor(char piece) {
    switch (piece) {
        case 'K': return QString::fromUtf8("♔");
        case 'Q': return QString::fromUtf8("♕");
        case 'R': return QString::fromUtf8("♖");
        case 'B': return QString::fromUtf8("♗");
        case 'N': return QString::fromUtf8("♘");
        case 'P': return QString::fromUtf8("♙");
        case 'k': return QString::fromUtf8("♚");
        case 'q': return QString::fromUtf8("♛");
        case 'r': return QString::fromUtf8("♜");
        case 'b': return QString::fromUtf8("♝");
        case 'n': return QString::fromUtf8("♞");
        case 'p': return QString::fromUtf8("♟");
        default: return {};
    }
}
