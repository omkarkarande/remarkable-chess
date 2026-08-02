#include "chess_state.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr char kEmpty = '.';

bool isPiece(char value) {
    const std::string pieces = "prnbqkPRNBQK";
    return pieces.find(value) != std::string::npos;
}

int fileOf(int square) { return square % 8; }
int rankOf(int square) { return square / 8; }

}  // namespace

ChessState ChessState::newGame() {
    ChessState state;
    state.board_.fill(kEmpty);
    const std::string whiteBack = "RNBQKBNR";
    const std::string blackBack = "rnbqkbnr";
    for (int file = 0; file < 8; ++file) {
        state.board_[file] = whiteBack[file];
        state.board_[8 + file] = 'P';
        state.board_[48 + file] = 'p';
        state.board_[56 + file] = blackBack[file];
    }
    return state;
}

ChessState ChessState::loadOrNew(const std::filesystem::path& savePath) {
    std::ifstream input(savePath);
    if (!input) {
        return newGame();
    }

    std::string value;
    std::getline(input, value);
    try {
        return fromFen(value);
    } catch (const std::exception&) {
        return newGame();
    }
}

bool ChessState::tryMove(const std::string& fromNotation, const std::string& toNotation, char promotion) {
    int from = 0;
    int to = 0;
    if (!parseSquare(fromNotation, from) || !parseSquare(toNotation, to) || from == to) {
        return false;
    }

    const char piece = board_[from];
    const char destination = board_[to];
    const bool castling = std::tolower(piece) == 'k' && rankOf(from) == rankOf(to) && std::abs(fileOf(to) - fileOf(from)) == 2;
    if (castling) {
        const Color movingSide = sideToMove_;
        const bool kingSide = fileOf(to) > fileOf(from);
        const int homeRank = movingSide == Color::White ? 0 : 7;
        const int expectedFrom = homeRank * 8 + 4;
        const int expectedTo = homeRank * 8 + (kingSide ? 6 : 2);
        const int rookFrom = homeRank * 8 + (kingSide ? 7 : 0);
        const int rookTo = homeRank * 8 + (kingSide ? 5 : 3);
        const bool kingMoved = movingSide == Color::White ? whiteKingMoved_ : blackKingMoved_;
        const bool rookMoved = movingSide == Color::White
            ? (kingSide ? whiteKingRookMoved_ : whiteQueenRookMoved_)
            : (kingSide ? blackKingRookMoved_ : blackQueenRookMoved_);
        const char rook = movingSide == Color::White ? 'R' : 'r';
        const Color opponent = movingSide == Color::White ? Color::Black : Color::White;
        const int through = homeRank * 8 + (kingSide ? 5 : 3);
        const bool clear = board_[through] == kEmpty && board_[expectedTo] == kEmpty &&
            (!kingSide || board_[homeRank * 8 + 6] == kEmpty) &&
            (!kingSide ? board_[homeRank * 8 + 1] == kEmpty : true);
        if (from != expectedFrom || to != expectedTo || kingMoved || rookMoved || board_[rookFrom] != rook ||
            !clear || inCheck(movingSide) || isSquareAttacked(through, opponent) || isSquareAttacked(to, opponent)) {
            return false;
        }
        board_[to] = piece;
        board_[from] = kEmpty;
        board_[rookTo] = rook;
        board_[rookFrom] = kEmpty;
        if (movingSide == Color::White) { whiteKingMoved_ = true; }
        else { blackKingMoved_ = true; }
        enPassantTarget_ = -1;
        ++halfMoveClock_;
        if (sideToMove_ == Color::Black) ++fullMoveNumber_;
        sideToMove_ = opponent;
        return true;
    }

    const bool enPassant = std::tolower(piece) == 'p' && destination == kEmpty &&
        std::abs(fileOf(to) - fileOf(from)) == 1 && to == enPassantTarget_ &&
        rankOf(to) - rankOf(from) == (isWhitePiece(piece) ? 1 : -1) &&
        board_[rankOf(from) * 8 + fileOf(to)] == (isWhitePiece(piece) ? 'p' : 'P');
    if (!isPiece(piece) || (sideToMove_ == Color::White && !isWhitePiece(piece)) ||
        (sideToMove_ == Color::Black && !isBlackPiece(piece)) ||
        (isWhitePiece(piece) && isWhitePiece(destination)) ||
        (isBlackPiece(piece) && isBlackPiece(destination)) ||
        std::tolower(destination) == 'k' || (!enPassant && !isLegalPieceMove(from, to))) {
        return false;
    }

    const bool promotionRequired = std::tolower(piece) == 'p' &&
        (rankOf(to) == 0 || rankOf(to) == 7);
    const char selectedPromotion = static_cast<char>(std::tolower(static_cast<unsigned char>(promotion)));
    if ((promotionRequired && std::string{"qrbn"}.find(selectedPromotion) == std::string::npos) ||
        (!promotionRequired && promotion != '\0')) {
        return false;
    }

    const int enPassantCaptured = rankOf(from) * 8 + fileOf(to);
    const char capturedEnPassantPawn = enPassant ? board_[enPassantCaptured] : kEmpty;
    const bool capture = destination != kEmpty || enPassant;
    board_[to] = piece;
    board_[from] = kEmpty;
    if (enPassant) board_[enPassantCaptured] = kEmpty;
    const Color movingSide = sideToMove_;
    if (inCheck(movingSide)) {
        board_[from] = piece;
        board_[to] = destination;
        if (enPassant) board_[enPassantCaptured] = capturedEnPassantPawn;
        return false;
    }
    if (promotionRequired) {
        board_[to] = isWhitePiece(piece)
            ? static_cast<char>(std::toupper(static_cast<unsigned char>(selectedPromotion)))
            : selectedPromotion;
    }

    enPassantTarget_ = (std::tolower(piece) == 'p' && std::abs(rankOf(to) - rankOf(from)) == 2)
        ? (rankOf(from) + rankOf(to)) / 2 * 8 + fileOf(from) : -1;
    halfMoveClock_ = std::tolower(piece) == 'p' || capture ? 0 : halfMoveClock_ + 1;

    if (piece == 'K') whiteKingMoved_ = true;
    if (piece == 'k') blackKingMoved_ = true;
    if (from == 0 || to == 0) whiteQueenRookMoved_ = true;
    if (from == 7 || to == 7) whiteKingRookMoved_ = true;
    if (from == 56 || to == 56) blackQueenRookMoved_ = true;
    if (from == 63 || to == 63) blackKingRookMoved_ = true;

    if (sideToMove_ == Color::Black) {
        ++fullMoveNumber_;
    }
    sideToMove_ = sideToMove_ == Color::White ? Color::Black : Color::White;
    return true;
}

char ChessState::pieceAt(const std::string& notation) const {
    int square = 0;
    if (!parseSquare(notation, square)) {
        throw std::invalid_argument("Invalid chess square");
    }
    return board_[square];
}

std::string ChessState::fen() const {
    std::ostringstream output;
    for (int rank = 7; rank >= 0; --rank) {
        int emptySquares = 0;
        for (int file = 0; file < 8; ++file) {
            const char piece = board_[rank * 8 + file];
            if (piece == kEmpty) {
                ++emptySquares;
                continue;
            }
            if (emptySquares > 0) {
                output << emptySquares;
                emptySquares = 0;
            }
            output << piece;
        }
        if (emptySquares > 0) {
            output << emptySquares;
        }
        if (rank > 0) {
            output << '/';
        }
    }
    std::string castling;
    if (!whiteKingMoved_ && !whiteKingRookMoved_) castling += 'K';
    if (!whiteKingMoved_ && !whiteQueenRookMoved_) castling += 'Q';
    if (!blackKingMoved_ && !blackKingRookMoved_) castling += 'k';
    if (!blackKingMoved_ && !blackQueenRookMoved_) castling += 'q';
    const std::string enPassant = enPassantTarget_ < 0 ? "-" :
        std::string{static_cast<char>('a' + fileOf(enPassantTarget_)), static_cast<char>('1' + rankOf(enPassantTarget_))};
    output << (sideToMove_ == Color::White ? " w " : " b ") << (castling.empty() ? "-" : castling) << " " << enPassant << " " << halfMoveClock_ << " " << fullMoveNumber_;
    return output.str();
}

void ChessState::save(const std::filesystem::path& savePath) const {
    if (!savePath.parent_path().empty()) {
        std::filesystem::create_directories(savePath.parent_path());
    }
    const std::filesystem::path temporaryPath = savePath.string() + ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to write chess save file");
        }
        output << fen() << '\n';
        output.flush();
        if (!output) {
            throw std::runtime_error("Unable to finish chess save file");
        }
    }
    std::filesystem::rename(temporaryPath, savePath);
}

bool ChessState::isWhitePiece(char piece) {
    return piece >= 'A' && piece <= 'Z';
}

bool ChessState::isBlackPiece(char piece) {
    return piece >= 'a' && piece <= 'z';
}

bool ChessState::parseSquare(const std::string& notation, int& square) {
    if (notation.size() != 2 || notation[0] < 'a' || notation[0] > 'h' ||
        notation[1] < '1' || notation[1] > '8') {
        return false;
    }
    square = (notation[1] - '1') * 8 + (notation[0] - 'a');
    return true;
}

ChessState ChessState::fromFen(const std::string& fenText) {
    std::istringstream input(fenText);
    std::string placement;
    std::string side;
    std::string castling;
    std::string enPassant;
    int halfMove = 0;
    int fullMove = 0;
    if (!(input >> placement >> side >> castling >> enPassant >> halfMove >> fullMove) ||
        (side != "w" && side != "b") || halfMove < 0 || fullMove < 1) {
        throw std::runtime_error("Invalid FEN");
    }

    ChessState state;
    state.board_.fill(kEmpty);
    int rank = 7;
    int file = 0;
    for (char value : placement) {
        if (value == '/') {
            if (file != 8 || rank == 0) {
                throw std::runtime_error("Invalid FEN placement");
            }
            --rank;
            file = 0;
        } else if (value >= '1' && value <= '8') {
            file += value - '0';
            if (file > 8) {
                throw std::runtime_error("Invalid FEN spacing");
            }
        } else if (isPiece(value) && file < 8) {
            state.board_[rank * 8 + file] = value;
            ++file;
        } else {
            throw std::runtime_error("Invalid FEN piece");
        }
    }
    if (rank != 0 || file != 8) {
        throw std::runtime_error("Incomplete FEN placement");
    }
    int whiteKings = 0;
    int blackKings = 0;
    for (int square = 0; square < 64; ++square) {
        const char piece = state.board_[square];
        if (piece == 'K') ++whiteKings;
        if (piece == 'k') ++blackKings;
        if (std::tolower(static_cast<unsigned char>(piece)) == 'p' &&
            (rankOf(square) == 0 || rankOf(square) == 7)) {
            throw std::runtime_error("Invalid FEN pawn rank");
        }
    }
    if (whiteKings != 1 || blackKings != 1) {
        throw std::runtime_error("Invalid FEN king count");
    }
    state.sideToMove_ = side == "w" ? Color::White : Color::Black;
    state.halfMoveClock_ = halfMove;
    state.fullMoveNumber_ = fullMove;
    if (enPassant != "-") {
        int target = -1;
        if (!parseSquare(enPassant, target)) throw std::runtime_error("Invalid en passant square");
        state.enPassantTarget_ = target;
    }
    state.whiteKingMoved_ = castling.find('K') == std::string::npos && castling.find('Q') == std::string::npos;
    state.blackKingMoved_ = castling.find('k') == std::string::npos && castling.find('q') == std::string::npos;
    state.whiteKingRookMoved_ = castling.find('K') == std::string::npos;
    state.whiteQueenRookMoved_ = castling.find('Q') == std::string::npos;
    state.blackKingRookMoved_ = castling.find('k') == std::string::npos;
    state.blackQueenRookMoved_ = castling.find('q') == std::string::npos;
    return state;
}

bool ChessState::inCheck(Color color) const {
    const char king = color == Color::White ? 'K' : 'k';
    for (int square = 0; square < 64; ++square) {
        if (board_[square] == king) {
            return isSquareAttacked(square, color == Color::White ? Color::Black : Color::White);
        }
    }
    return true;
}

bool ChessState::hasLegalMove() const {
    for (int from = 0; from < 64; ++from) {
        const char piece = board_[from];
        if (!isPiece(piece) ||
            (sideToMove_ == Color::White ? !isWhitePiece(piece) : !isBlackPiece(piece))) {
            continue;
        }
        for (int to = 0; to < 64; ++to) {
            ChessState candidate = *this;
            const std::string fromNotation{static_cast<char>('a' + fileOf(from)),
                                           static_cast<char>('1' + rankOf(from))};
            const std::string toNotation{static_cast<char>('a' + fileOf(to)),
                                         static_cast<char>('1' + rankOf(to))};
            const bool promotion = std::tolower(piece) == 'p' && (rankOf(to) == 0 || rankOf(to) == 7);
            if (candidate.tryMove(fromNotation, toNotation, promotion ? 'q' : '\0')) {
                return true;
            }
        }
    }
    return false;
}

bool ChessState::isCheckmate() const {
    return inCheck(sideToMove_) && !hasLegalMove();
}

bool ChessState::isSquareAttacked(int square, Color attacker) const {
    for (int from = 0; from < 64; ++from) {
        const char piece = board_[from];
        if (!isPiece(piece) || (attacker == Color::White ? !isWhitePiece(piece) : !isBlackPiece(piece))) continue;
        const int dx = fileOf(square) - fileOf(from);
        const int dy = rankOf(square) - rankOf(from);
        const int absDx = std::abs(dx);
        const int absDy = std::abs(dy);
        switch (static_cast<char>(std::tolower(piece))) {
            case 'p': if (absDx == 1 && dy == (attacker == Color::White ? 1 : -1)) return true; break;
            case 'n': if ((absDx == 1 && absDy == 2) || (absDx == 2 && absDy == 1)) return true; break;
            case 'b': if (absDx == absDy && pathIsClear(from, square)) return true; break;
            case 'r': if ((dx == 0 || dy == 0) && pathIsClear(from, square)) return true; break;
            case 'q': if ((dx == 0 || dy == 0 || absDx == absDy) && pathIsClear(from, square)) return true; break;
            case 'k': if (std::max(absDx, absDy) == 1) return true; break;
        }
    }
    return false;
}

bool ChessState::isLegalPieceMove(int from, int to) const {
    const char piece = static_cast<char>(std::tolower(board_[from]));
    const int dx = fileOf(to) - fileOf(from);
    const int dy = rankOf(to) - rankOf(from);
    const int absDx = std::abs(dx);
    const int absDy = std::abs(dy);
    const bool destinationOccupied = board_[to] != kEmpty;

    switch (piece) {
        case 'p': {
            const int direction = isWhitePiece(board_[from]) ? 1 : -1;
            const int startRank = isWhitePiece(board_[from]) ? 1 : 6;
            if (absDx == 1 && dy == direction) {
                return destinationOccupied;
            }
            if (dx != 0 || destinationOccupied) {
                return false;
            }
            if (dy == direction) {
                return true;
            }
            return rankOf(from) == startRank && dy == 2 * direction &&
                   board_[(rankOf(from) + direction) * 8 + fileOf(from)] == kEmpty;
        }
        case 'n':
            return (absDx == 1 && absDy == 2) || (absDx == 2 && absDy == 1);
        case 'b':
            return absDx == absDy && pathIsClear(from, to);
        case 'r':
            return (dx == 0 || dy == 0) && pathIsClear(from, to);
        case 'q':
            return (dx == 0 || dy == 0 || absDx == absDy) && pathIsClear(from, to);
        case 'k':
            return std::max(absDx, absDy) == 1;
        default:
            return false;
    }
}

bool ChessState::pathIsClear(int from, int to) const {
    const int fileStep = (fileOf(to) > fileOf(from)) - (fileOf(to) < fileOf(from));
    const int rankStep = (rankOf(to) > rankOf(from)) - (rankOf(to) < rankOf(from));
    int file = fileOf(from) + fileStep;
    int rank = rankOf(from) + rankStep;
    while (file != fileOf(to) || rank != rankOf(to)) {
        if (board_[rank * 8 + file] != kEmpty) {
            return false;
        }
        file += fileStep;
        rank += rankStep;
    }
    return true;
}
