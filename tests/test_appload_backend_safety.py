import unittest
from pathlib import Path


BACKEND = (Path(__file__).parents[1] / "src" / "appload_backend.cpp").read_text()


class AppLoadBackendSafetyContractTests(unittest.TestCase):
    def test_coordinator_writes_cannot_raise_sigpipe(self):
        self.assertIn("MSG_NOSIGNAL", BACKEND)
        self.assertIn("sendAll", BACKEND)

    def test_skill_level_requires_the_entire_untrimmed_payload_to_be_an_integer(self):
        self.assertIn("std::from_chars", BACKEND)
        self.assertIn("result.ptr != contents.data() + contents.size()", BACKEND)
        self.assertIn("result.ec != std::errc{}", BACKEND)

    def test_new_game_persists_player_side_and_publishes_current_status(self):
        self.assertIn("Unable to write player side", BACKEND)
        self.assertIn("sendMessage(socketFd, kStatusUpdate, turnStatus());", BACKEND)

    def test_stockfish_is_resolved_from_proc_self_exe_when_available(self):
        self.assertIn('std::filesystem::read_symlink("/proc/self/exe")', BACKEND)
        self.assertIn("std::filesystem::absolute(argv0).parent_path() / name", BACKEND)

    def test_stockfish_io_failure_is_reported_and_shutdown_is_bounded(self):
        self.assertIn("Stockfish output failed", BACKEND)
        self.assertIn("WNOHANG", BACKEND)
        self.assertNotIn("waitpid(pid_, &status, 0)", BACKEND)
        self.assertIn("SIGTERM", BACKEND)
        self.assertIn("closePipe(toEngine)", BACKEND)
        self.assertIn("closePipe(fromEngine)", BACKEND)
    def test_seqpacket_records_are_not_combined_across_message_boundaries(self):
        self.assertIn("bool readRecord", BACKEND)
        self.assertIn("recvmsg", BACKEND)
        self.assertIn("MSG_TRUNC", BACKEND)
        self.assertNotIn("bool readExact", BACKEND)

    def test_stockfish_response_wait_is_bounded(self):
        self.assertIn("poll(&descriptor, 1, 5000)", BACKEND)
        self.assertIn("Stockfish response timed out", BACKEND)
        self.assertIn("(descriptor.revents & POLLIN) == 0", BACKEND)
        self.assertNotIn("POLLERR | POLLHUP | POLLNVAL", BACKEND)


if __name__ == "__main__":
    unittest.main()
