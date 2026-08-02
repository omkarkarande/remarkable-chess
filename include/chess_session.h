#pragma once

#include "chess_state.h"

#include <filesystem>
#include <string>

// Platform-neutral interaction layer: the Qt/QML shell and its tests use the
// same select-then-move behavior and one isolated save file.
class ChessSession {
public:
    explicit ChessSession(std::filesystem::path savePath);

    bool tap(const std::string& square);
    [[nodiscard]] char pieceAt(const std::string& square) const { return state_.pieceAt(square); }
    [[nodiscard]] Color sideToMove() const { return state_.sideToMove(); }
    [[nodiscard]] const std::string& selectedSquare() const { return selectedSquare_; }

private:
    ChessState state_;
    std::filesystem::path savePath_;
    std::string selectedSquare_;

    [[nodiscard]] bool belongsToCurrentPlayer(char piece) const;
};
