#include "chess_session.h"

#include <cctype>
#include <exception>
#include <utility>

ChessSession::ChessSession(std::filesystem::path savePath)
    : state_(ChessState::loadOrNew(savePath)), savePath_(std::move(savePath)) {}

bool ChessSession::tap(const std::string& square, char promotion) {
    if (!isValidSquare(square)) {
        return false;
    }

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

    ChessState candidate = state_;
    if (!candidate.tryMove(selectedSquare_, square, promotion)) {
        return false;
    }
    try {
        candidate.save(savePath_);
    } catch (const std::exception&) {
        return false;
    }

    state_ = std::move(candidate);
    selectedSquare_.clear();
    return true;
}

bool ChessSession::belongsToCurrentPlayer(char piece) const {
    if (piece == '.') {
        return false;
    }
    return sideToMove() == Color::White ? std::isupper(static_cast<unsigned char>(piece)) != 0
                                        : std::islower(static_cast<unsigned char>(piece)) != 0;
}

bool ChessSession::isValidSquare(const std::string& square) {
    return square.size() == 2 && square[0] >= 'a' && square[0] <= 'h' &&
           square[1] >= '1' && square[1] <= '8';
}
