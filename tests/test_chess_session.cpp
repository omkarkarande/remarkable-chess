#include "chess_session.h"

#include <cassert>
#include <filesystem>
#include <fstream>
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

void test_invalid_squares_return_false_without_changing_selection() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-session-invalid-square-test.fen";
    std::filesystem::remove(savePath);

    ChessSession session(savePath);
    assert(!session.tap("z9"));
    assert(session.selectedSquare().empty());
    assert(session.tap("e2"));
    assert(!session.tap("e9"));
    assert(session.selectedSquare() == "e2");

    std::filesystem::remove(savePath);
}

void test_chosen_promotion_is_applied_and_persisted() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-session-promotion-test.fen";
    {
        std::ofstream output(savePath);
        output << "7k/P7/8/8/8/5n2/8/4n2K w - - 0 1\n";
    }

    ChessSession session(savePath);
    assert(session.tap("a7"));
    assert(session.tap("a8", 'r'));
    assert(session.pieceAt("a8") == 'R');
    ChessSession resumed(savePath);
    assert(resumed.pieceAt("a8") == 'R');
    std::filesystem::remove(savePath);
}

void test_save_failure_leaves_state_and_selection_unchanged() {
    const auto blockingPath = std::filesystem::temp_directory_path() / "remarkable-chess-session-save-blocker";
    std::filesystem::remove_all(blockingPath);
    {
        std::ofstream output(blockingPath);
        output << "not a directory";
    }
    const auto savePath = blockingPath / "game.fen";

    ChessSession session(savePath);
    assert(session.tap("e2"));
    assert(!session.tap("e4"));
    assert(session.selectedSquare() == "e2");
    assert(session.pieceAt("e2") == 'P');
    assert(session.pieceAt("e4") == '.');
    assert(session.sideToMove() == Color::White);
    std::filesystem::remove(blockingPath);
}

}  // namespace

int main() {
    test_tapping_source_then_destination_moves_and_persists();
    test_invalid_destination_keeps_selection_for_retry();
    test_invalid_squares_return_false_without_changing_selection();
    test_chosen_promotion_is_applied_and_persisted();
    test_save_failure_leaves_state_and_selection_unchanged();
    std::cout << "chess session tests passed\n";
}
