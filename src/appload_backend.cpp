#include "chess_state.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace {
constexpr std::int32_t kSystemTerminate = -1;
constexpr std::int32_t kSystemNewCoordinator = -2;
constexpr std::int32_t kMoveRequest = 10;
constexpr std::int32_t kNewGameRequest = 11;
constexpr std::int32_t kUndoRequest = 12;
constexpr std::int32_t kSetSkillLevel = 13;
constexpr std::int32_t kEngineMoveRequest = 14;
constexpr std::int32_t kStateUpdate = 101;
constexpr std::int32_t kStatusUpdate = 102;
constexpr std::int32_t kCheckUpdate = 103;
constexpr std::int32_t kPlayerSideUpdate = 104;
constexpr std::int32_t kGameOverUpdate = 105;

struct PacketHeader {
    std::int32_t type;
    std::int32_t length;
};

bool readExact(int fd, void* buffer, std::size_t length) {
    auto* cursor = static_cast<char*>(buffer);
    while (length > 0) {
        const ssize_t received = recv(fd, cursor, length, 0);
        if (received <= 0) return false;
        cursor += received;
        length -= static_cast<std::size_t>(received);
    }
    return true;
}

bool sendMessage(int fd, std::int32_t type, const std::string& contents) {
    const PacketHeader header{type, static_cast<std::int32_t>(contents.size())};
    return send(fd, &header, sizeof(header), 0) == static_cast<ssize_t>(sizeof(header)) &&
           (contents.empty() || send(fd, contents.data(), contents.size(), 0) == static_cast<ssize_t>(contents.size()));
}

class Stockfish {
public:
    explicit Stockfish(const std::filesystem::path& executable) { start(executable); }
    ~Stockfish() { stop(); }

    Stockfish(const Stockfish&) = delete;
    Stockfish& operator=(const Stockfish&) = delete;

    void setSkillLevel(int level) { skillLevel_ = std::clamp(level, 0, 20); }

    std::optional<std::string> bestMove(const std::string& fen) {
        writeLine("setoption name UCI_LimitStrength value false");
        writeLine("setoption name Skill Level value " + std::to_string(skillLevel_));
        writeLine("isready");
        waitFor("readyok");
        writeLine("position fen " + fen);
        writeLine("go movetime 450");
        std::string line;
        while (readLine(line)) {
            if (line.rfind("bestmove ", 0) == 0) {
                std::istringstream words(line);
                std::string label;
                std::string move;
                words >> label >> move;
                if (move.size() == 4 || (move.size() == 5 && std::string{"qrbn"}.find(move[4]) != std::string::npos)) return move;
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

private:
    pid_t pid_ = -1;
    int input_ = -1;
    int output_ = -1;
    int skillLevel_ = 3;
    std::string pending_;

    void start(const std::filesystem::path& executable) {
        int toEngine[2]{};
        int fromEngine[2]{};
        if (pipe(toEngine) != 0 || pipe(fromEngine) != 0) throw std::runtime_error("Unable to create Stockfish pipes");
        pid_ = fork();
        if (pid_ < 0) throw std::runtime_error("Unable to start Stockfish");
        if (pid_ == 0) {
            dup2(toEngine[0], STDIN_FILENO);
            dup2(fromEngine[1], STDOUT_FILENO);
            dup2(fromEngine[1], STDERR_FILENO);
            close(toEngine[0]); close(toEngine[1]); close(fromEngine[0]); close(fromEngine[1]);
            execl(executable.c_str(), executable.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        close(toEngine[0]);
        close(fromEngine[1]);
        input_ = toEngine[1];
        output_ = fromEngine[0];
        writeLine("uci");
        waitFor("uciok");
        writeLine("isready");
        waitFor("readyok");
    }

    void stop() {
        if (input_ >= 0) {
            writeLine("quit");
            close(input_);
            input_ = -1;
        }
        if (output_ >= 0) {
            close(output_);
            output_ = -1;
        }
        if (pid_ > 0) {
            waitpid(pid_, nullptr, 0);
            pid_ = -1;
        }
    }

    void writeLine(const std::string& line) {
        const std::string data = line + "\n";
        if (input_ < 0 || write(input_, data.data(), data.size()) != static_cast<ssize_t>(data.size())) {
            throw std::runtime_error("Stockfish input failed");
        }
    }

    bool readLine(std::string& line) {
        for (;;) {
            const auto newline = pending_.find('\n');
            if (newline != std::string::npos) {
                line = pending_.substr(0, newline);
                pending_.erase(0, newline + 1);
                return true;
            }
            std::array<char, 512> buffer{};
            const ssize_t count = read(output_, buffer.data(), buffer.size());
            if (count <= 0) return false;
            pending_.append(buffer.data(), static_cast<std::size_t>(count));
        }
    }

    void waitFor(const std::string& expected) {
        std::string line;
        while (readLine(line)) if (line == expected) return;
        throw std::runtime_error("Stockfish startup failed");
    }
};

int connectToAppLoad(const char* socketPath) {
    const int socketFd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (socketFd < 0) throw std::runtime_error("Unable to create AppLoad socket");
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (std::string(socketPath).size() >= sizeof(address.sun_path)) throw std::runtime_error("AppLoad socket path too long");
    std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", socketPath);
    if (connect(socketFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close(socketFd);
        throw std::runtime_error("Unable to connect to AppLoad");
    }
    return socketFd;
}

std::filesystem::path executableSibling(const char* argv0, const char* name) {
    return std::filesystem::absolute(argv0).parent_path() / name;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) return 2;
    try {
        const int socketFd = connectToAppLoad(argv[1]);
        const auto savePath = std::filesystem::path("/home/root/.config/remarkable-chess/game.fen");
        const auto playerSidePath = std::filesystem::path("/home/root/.config/remarkable-chess/player-side");
        ChessState game = ChessState::loadOrNew(savePath);
        Stockfish engine(executableSibling(argv[0], "stockfish"));
        std::vector<ChessState> history;
        std::mt19937 randomEngine(std::random_device{}());
        Color playerSide = Color::White;
        {
            std::ifstream input(playerSidePath);
            char savedSide = '\0';
            if (input >> savedSide && savedSide == 'b') playerSide = Color::Black;
        }

        const auto playerSideName = [&]() { return playerSide == Color::White ? "w" : "b"; };
        const auto turnStatus = [&]() {
            if (game.isCheckmate()) return std::string("Checkmate");
            if (!game.hasLegalMove()) return std::string("Stalemate");
            return game.sideToMove() == Color::White ? std::string("White to move") : std::string("Black to move");
        };
        const auto sendState = [&]() {
            sendMessage(socketFd, kStateUpdate, game.fen());
            const bool checked = game.inCheck(game.sideToMove());
            sendMessage(socketFd, kCheckUpdate, checked ? (game.sideToMove() == Color::White ? "w" : "b") : "");
            sendMessage(socketFd, kPlayerSideUpdate, playerSideName());
            if (game.isCheckmate()) sendMessage(socketFd, kGameOverUpdate, "checkmate");
            else if (!game.hasLegalMove()) sendMessage(socketFd, kGameOverUpdate, "stalemate");
        };
        const auto makeEngineMove = [&]() {
            if (game.isCheckmate() || !game.hasLegalMove()) return false;
            history.push_back(game);
            const auto reply = engine.bestMove(game.fen());
            if (!reply || !game.tryMove(reply->substr(0, 2), reply->substr(2, 2),
                                        reply->size() == 5 ? (*reply)[4] : '\0')) {
                history.pop_back();
                return false;
            }
            game.save(savePath);
            return true;
        };
        const auto startNewGame = [&]() {
            game = ChessState::newGame();
            history.clear();
            playerSide = std::uniform_int_distribution<int>(0, 1)(randomEngine) == 0 ? Color::White : Color::Black;
            {
                std::ofstream output(playerSidePath, std::ios::trunc);
                output << (playerSide == Color::White ? 'w' : 'b') << '\n';
            }
            game.save(savePath);
            sendState();
            // The frontend schedules the one-second engine opening when needed.
            // Avoid a burst of extra AppLoad messages while it is reconstructing the board.
        };

        for (;;) {
            PacketHeader header{};
            if (!readExact(socketFd, &header, sizeof(header))) break;
            if (header.length < 0 || header.length > 1024) break;
            std::string contents(static_cast<std::size_t>(header.length), '\0');
            if (header.length > 0 && !readExact(socketFd, contents.data(), contents.size())) break;
            if (header.type == kSystemTerminate) break;
            if (header.type == kSystemNewCoordinator) {
                sendState();
                sendMessage(socketFd, kStatusUpdate, turnStatus());
                continue;
            }
            if (header.type == kSetSkillLevel) {
                try {
                    const int level = std::stoi(contents);
                    if (level < 0 || level > 20) throw std::out_of_range("skill level");
                    engine.setSkillLevel(level);
                    sendMessage(socketFd, kStatusUpdate, "Skill level " + std::to_string(level));
                } catch (...) {
                    sendMessage(socketFd, kStatusUpdate, "Skill level must be 0–20");
                }
                continue;
            }
            if (header.type == kEngineMoveRequest) {
                if (game.sideToMove() != playerSide && !game.isCheckmate() && game.hasLegalMove()) {
                    sendMessage(socketFd, kStatusUpdate, "Thinking…");
                    if (!makeEngineMove()) sendMessage(socketFd, kStatusUpdate, "Engine has no reply");
                    sendState();
                    sendMessage(socketFd, kStatusUpdate, turnStatus());
                }
                continue;
            }
            if (header.type == kNewGameRequest) {
                startNewGame();
                continue;
            }
            if (header.type == kUndoRequest) {
                std::optional<ChessState> restoredGame;
                while (!history.empty()) {
                    ChessState candidate = history.back();
                    history.pop_back();
                    if (candidate.sideToMove() == playerSide) {
                        restoredGame = candidate;
                        break;
                    }
                }
                if (restoredGame) {
                    game = *restoredGame;
                    game.save(savePath);
                    sendState();
                    sendMessage(socketFd, kStatusUpdate, turnStatus());
                } else {
                    sendMessage(socketFd, kStatusUpdate, "Nothing to undo");
                }
                continue;
            }
            if (header.type != kMoveRequest || (contents.size() != 4 && contents.size() != 5)) continue;
            if (game.isCheckmate() || !game.hasLegalMove()) {
                sendMessage(socketFd, kStatusUpdate, turnStatus());
                sendState();
                continue;
            }
            if (game.sideToMove() != playerSide) {
                sendMessage(socketFd, kStatusUpdate, "Wait for the engine");
                continue;
            }
            const ChessState beforePlayerMove = game;
            if (!game.tryMove(contents.substr(0, 2), contents.substr(2, 2),
                              contents.size() == 5 ? contents[4] : '\0')) {
                sendMessage(socketFd, kStatusUpdate, "That move is not available");
                sendState();
                continue;
            }
            history.push_back(beforePlayerMove);
            game.save(savePath);
            if (game.isCheckmate() || !game.hasLegalMove()) {
                sendState();
                sendMessage(socketFd, kStatusUpdate, turnStatus());
                continue;
            }
            // Publish the player's board first and return to the UI event loop.
            // The frontend timer will request the engine only after that frame has had
            // time to become visible on e-ink.
            sendState();
            sendMessage(socketFd, kStatusUpdate, "Thinking…");
        }
        close(socketFd);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Chess backend: " << error.what() << std::endl;
        return 1;
    }
}
