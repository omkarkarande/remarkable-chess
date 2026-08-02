#include "chess_session.h"

#include <cassert>
#include <filesystem>
#include <iostream>

namespace {

void test_tapping_source_then_destination_moves_and_persists() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-session-test.fen";
    std::filesystem::remove(savePath);

    ChessSession session(savePath);
    assert(session.tap("e2"));
    assert(session.selectedSquare() == "e2");
    assert(session.tap("e4"));
    assert(session.selectedSquare().empty());
    assert(session.pieceAt("e4") == 'P');

    ChessSession resumed(savePath);
    assert(resumed.pieceAt("e4") == 'P');
    assert(resumed.sideToMove() == Color::Black);

    std::filesystem::remove(savePath);
}

void test_invalid_destination_keeps_selection_for_retry() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-session-invalid-test.fen";
    std::filesystem::remove(savePath);

    ChessSession session(savePath);
    assert(session.tap("e2"));
    assert(!session.tap("e5"));
    assert(session.selectedSquare() == "e2");
    assert(session.tap("e4"));

    std::filesystem::remove(savePath);
}

}  // namespace

int main() {
    test_tapping_source_then_destination_moves_and_persists();
    test_invalid_destination_keeps_selection_for_retry();
    std::cout << "chess session tests passed\n";
}
