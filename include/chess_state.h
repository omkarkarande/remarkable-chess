#pragma once

#include <array>
#include <filesystem>
#include <string>

// Platform-neutral chess state with move legality, check detection, and legal
// move enumeration. It intentionally persists the simple FEN boundary used by
// the app-loader backend.
enum class Color { White, Black };

class ChessState {
public:
    static ChessState newGame();
    static ChessState loadOrNew(const std::filesystem::path& savePath);

    bool tryMove(const std::string& from, const std::string& to, char promotion = '\0');
    [[nodiscard]] char pieceAt(const std::string& square) const;
    [[nodiscard]] std::string fen() const;
    [[nodiscard]] bool inCheck(Color color) const;
    [[nodiscard]] bool hasLegalMove() const;
    [[nodiscard]] bool isCheckmate() const;
    [[nodiscard]] Color sideToMove() const { return sideToMove_; }
    void save(const std::filesystem::path& savePath) const;

private:
    std::array<char, 64> board_{};
    Color sideToMove_ = Color::White;
    int fullMoveNumber_ = 1;
    int enPassantTarget_ = -1;
    bool whiteKingMoved_ = false;
    bool blackKingMoved_ = false;
    bool whiteKingRookMoved_ = false;
    bool whiteQueenRookMoved_ = false;
    bool blackKingRookMoved_ = false;
    bool blackQueenRookMoved_ = false;

    static bool isWhitePiece(char piece);
    static bool isBlackPiece(char piece);
    static bool parseSquare(const std::string& notation, int& square);
    static ChessState fromFen(const std::string& fen);
    [[nodiscard]] bool isLegalPieceMove(int from, int to) const;
    [[nodiscard]] bool pathIsClear(int from, int to) const;
    [[nodiscard]] bool isSquareAttacked(int square, Color attacker) const;
};
