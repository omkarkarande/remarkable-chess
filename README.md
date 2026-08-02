# reMarkable Chess

A native, offline single-player chess app for the **reMarkable 2**, built for AppLoad and backed by Stockfish. It is designed for e-ink: high-contrast pieces, a full-width board, and deliberate visible turn transitions.

> **Status:** an actively developed personal project. It is tested directly through AppLoad; Vellum/reManager packaging remains future work.

## Why this exists

This project was made as a modern alternative to [LinusCDE/chessmarkable](https://github.com/LinusCDE/chessmarkable). That original project was a useful starting point and reference, but it is not maintained for newer reMarkable software versions. This implementation targets a current AppLoad-based workflow and keeps all game state in the app's own configuration directory.

## Features

- Native AppLoad QML frontend plus an authoritative C++ chess backend.
- Offline Stockfish opponent with a live **Skill Level 0–20** control.
- Legal move enforcement, including check and self-check protection.
- Check, checkmate, and stalemate detection.
- Castling with persisted castling rights.
- En passant with persisted FEN target state.
- Pawn promotion chooser with Queen, Rook, Bishop, and Knight piece images.
- Random player colour for new games; the board rotates so the player's colour is at the bottom.
- Undo to the latest position where it is the player's turn.
- A staged e-ink turn flow: the player's move is shown first, then Stockfish replies after a short delay.
- App-owned persistence only:
  - game position: `~/.config/remarkable-chess/game.fen`
  - assigned player side: `~/.config/remarkable-chess/player-side`

## Architecture

- `src/chess_state.cpp` — platform-neutral legal chess state and FEN persistence.
- `src/appload_backend.cpp` — AppLoad protocol, Stockfish UCI integration, game/session authority.
- `packaging/appload-frontend/ui/Chess.qml` — e-ink QML interface.
- `tests/` — state/session tests and AppLoad frontend/backend contract tests.

The app does not write reMarkable documents or modify Xochitl resources.

## Local tests

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude \
  tests/test_chess_state.cpp src/chess_state.cpp \
  -o /tmp/remarkable-chess-state-tests && /tmp/remarkable-chess-state-tests

clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude \
  tests/test_chess_session.cpp src/chess_state.cpp src/chess_session.cpp \
  -o /tmp/remarkable-chess-session-tests && /tmp/remarkable-chess-session-tests

python3 -m unittest tests/test_appload_manifest.py -v
```

## Build for rM2

The backend is cross-compiled for ARMv7 with the official reMarkable SDK. The AppLoad frontend is compiled into `resources.rcc` using the SDK `rcc` tool. See the project CMake configuration and `packaging/appload-frontend/` for the current direct-test layout.

## Credits and licenses

- Chess piece artwork is based on the Lichess CBurnett set; see `packaging/appload-frontend/ASSET-NOTICES.md`.
- Engine: [Stockfish](https://github.com/official-stockfish/Stockfish).
- Original reMarkable chess reference: [LinusCDE/chessmarkable](https://github.com/LinusCDE/chessmarkable).

This repository is licensed under the [MIT License](LICENSE).
