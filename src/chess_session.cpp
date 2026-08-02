#include "chess_session.h"

#include <cctype>

ChessSession::ChessSession(std::filesystem::path savePath)
    : state_(ChessState::loadOrNew(savePath)), savePath_(std::move(savePath)) {}

bool ChessSession::tap(const std::string& square) {
    const char tappedPiece = state_.pieceAt(square);
    if (selectedSquare_.empty()) {
        if (!belongsToCurrentPlayer(tappedPiece)) {
            return false;
        }
        selectedSquare_ = square;
        return true;
    }

    if (belongsToCurrentPlayer(tappedPiece)) {
        selectedSquare_ = square;
        return true;
    }

    if (!state_.tryMove(selectedSquare_, square)) {
        return false;
    }
    selectedSquare_.clear();
    state_.save(savePath_);
    return true;
}

bool ChessSession::belongsToCurrentPlayer(char piece) const {
    if (piece == '.') {
        return false;
    }
    return sideToMove() == Color::White ? std::isupper(static_cast<unsigned char>(piece)) != 0
                                        : std::islower(static_cast<unsigned char>(piece)) != 0;
}
