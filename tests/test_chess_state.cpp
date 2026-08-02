#include "chess_state.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void test_new_game_has_standard_initial_position() {
    ChessState game = ChessState::newGame();

    assert(game.fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(game.sideToMove() == Color::White);
}

void test_pawn_move_updates_position_and_turn() {
    ChessState game = ChessState::newGame();

    assert(game.tryMove("e2", "e4"));
    assert(game.pieceAt("e4") == 'P');
    assert(game.pieceAt("e2") == '.');
    assert(game.fen() == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    assert(game.sideToMove() == Color::Black);
}

void test_save_and_load_resume_an_in_progress_game() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-state-test.fen";
    std::filesystem::remove(savePath);

    ChessState game = ChessState::newGame();
    assert(game.tryMove("e2", "e4"));
    assert(game.tryMove("c7", "c5"));
    game.save(savePath);

    ChessState restored = ChessState::loadOrNew(savePath);
    assert(restored.fen() == game.fen());
    assert(restored.sideToMove() == Color::White);

    std::filesystem::remove(savePath);
}

void test_move_that_leaves_king_in_check_is_rejected() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-check-test.fen";
    {
        std::ofstream output(savePath);
        output << "k3r3/8/8/8/8/8/8/4K2R w - - 0 1\n";
    }

    ChessState game = ChessState::loadOrNew(savePath);
    assert(game.inCheck(Color::White));
    assert(!game.tryMove("h1", "h2"));
    assert(game.pieceAt("h1") == 'R');
    std::filesystem::remove(savePath);
}

void test_checkmate_has_no_legal_move() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-checkmate-test.fen";
    {
        std::ofstream output(savePath);
        output << "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1\n";
    }

    ChessState game = ChessState::loadOrNew(savePath);
    assert(game.inCheck(Color::Black));
    assert(!game.hasLegalMove());
    assert(game.isCheckmate());
    std::filesystem::remove(savePath);
}

void test_en_passant_captures_a_pawn_after_its_two_square_advance() {
    ChessState game = ChessState::newGame();
    assert(game.tryMove("e2", "e4"));
    assert(game.tryMove("a7", "a6"));
    assert(game.tryMove("e4", "e5"));
    assert(game.tryMove("d7", "d5"));

    assert(game.tryMove("e5", "d6"));
    assert(game.pieceAt("d6") == 'P');
    assert(game.pieceAt("d5") == '.');
}

void test_castling_moves_the_rook_with_the_king() {
    ChessState game = ChessState::newGame();
    assert(game.tryMove("e2", "e4"));
    assert(game.tryMove("a7", "a6"));
    assert(game.tryMove("g1", "f3"));
    assert(game.tryMove("a6", "a5"));
    assert(game.tryMove("f1", "e2"));
    assert(game.tryMove("b7", "b6"));

    assert(game.tryMove("e1", "g1"));
    assert(game.pieceAt("g1") == 'K');
    assert(game.pieceAt("f1") == 'R');
    assert(game.pieceAt("h1") == '.');
}

void test_castling_clears_an_existing_en_passant_target() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-castling-en-passant-test.fen";
    {
        std::ofstream output(savePath);
        output << "4k3/8/8/8/8/8/8/R3K2R w K e3 12 7\n";
    }

    ChessState game = ChessState::loadOrNew(savePath);
    assert(game.tryMove("e1", "g1"));
    assert(game.fen() == "4k3/8/8/8/8/8/8/R4RK1 b - - 13 7");
    std::filesystem::remove(savePath);
}

void test_halfmove_clock_updates_and_round_trips_through_fen() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-halfmove-test.fen";
    {
        std::ofstream output(savePath);
        output << "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 17 9\n";
    }

    ChessState game = ChessState::loadOrNew(savePath);
    assert(game.fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 17 9");
    assert(game.tryMove("g1", "f3"));
    assert(game.fen().find(" - 18 9") != std::string::npos);
    assert(game.tryMove("g8", "f6"));
    assert(game.fen().find(" - 19 10") != std::string::npos);
    assert(game.tryMove("e2", "e4"));
    assert(game.fen().find(" e3 0 10") != std::string::npos);
    assert(game.tryMove("f6", "e4"));
    assert(game.fen().find(" - 0 11") != std::string::npos);
    game.save(savePath);
    assert(ChessState::loadOrNew(savePath).fen() == game.fen());
    std::filesystem::remove(savePath);
}

void test_invalid_semantic_fen_falls_back_to_a_new_game() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-invalid-fen-test.fen";
    const std::string initialFen = ChessState::newGame().fen();

    {
        std::ofstream output(savePath);
        output << "8/8/8/8/8/8/8/4K3 w - - 0 1\n";
    }
    assert(ChessState::loadOrNew(savePath).fen() == initialFen);

    {
        std::ofstream output(savePath, std::ios::trunc);
        output << "4k3/8/8/8/8/8/8/P3K3 w - - 0 1\n";
    }
    assert(ChessState::loadOrNew(savePath).fen() == initialFen);
    std::filesystem::remove(savePath);
}

void test_stalemate_is_not_checkmate() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-stalemate-test.fen";
    {
        std::ofstream output(savePath);
        output << "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1\n";
    }

    ChessState game = ChessState::loadOrNew(savePath);
    assert(!game.inCheck(Color::Black));
    assert(!game.hasLegalMove());
    assert(!game.isCheckmate());
    std::filesystem::remove(savePath);
}

void test_promotion_requires_an_explicit_choice_and_uses_it() {
    const auto savePath = std::filesystem::temp_directory_path() / "remarkable-chess-promotion-test.fen";
    {
        std::ofstream output(savePath);
        output << "7k/P7/8/8/8/5n2/8/4n2K w - - 0 1\n";
    }

    ChessState game = ChessState::loadOrNew(savePath);
    assert(game.hasLegalMove());
    assert(!game.tryMove("a7", "a8"));
    assert(game.pieceAt("a7") == 'P');
    assert(game.tryMove("a7", "a8", 'n'));
    assert(game.pieceAt("a8") == 'N');
    assert(game.sideToMove() == Color::Black);
    std::filesystem::remove(savePath);
}

}  // namespace

int main() {
    test_new_game_has_standard_initial_position();
    test_pawn_move_updates_position_and_turn();
    test_save_and_load_resume_an_in_progress_game();
    test_move_that_leaves_king_in_check_is_rejected();
    test_checkmate_has_no_legal_move();
    test_en_passant_captures_a_pawn_after_its_two_square_advance();
    test_castling_moves_the_rook_with_the_king();
    test_castling_clears_an_existing_en_passant_target();
    test_halfmove_clock_updates_and_round_trips_through_fen();
    test_invalid_semantic_fen_falls_back_to_a_new_game();
    test_stalemate_is_not_checkmate();
    test_promotion_requires_an_explicit_choice_and_uses_it();
    std::cout << "chess state tests passed\n";
}
