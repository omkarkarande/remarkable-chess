import json
import unittest
from pathlib import Path


class ApploadFrontendManifestTests(unittest.TestCase):
    def test_frontend_manifest_has_required_appload_contract(self):
        app_root = Path(__file__).parents[1] / "packaging" / "appload-frontend"
        manifest = json.loads((app_root / "manifest.json").read_text())

        self.assertEqual("Chess", manifest["name"])
        self.assertEqual("omi.remarkable-chess", manifest["id"])
        self.assertTrue(manifest["loadsBackend"])
        self.assertEqual("/ui/Chess.qml", manifest["entry"])
        self.assertTrue(manifest["supportsScaling"])
        self.assertTrue((app_root / manifest["entry"].lstrip("/")).is_file())

    def test_frontend_delegates_game_state_and_controls_to_backend(self):
        qml = (Path(__file__).parents[1] / "packaging" / "appload-frontend" / "ui" / "Chess.qml").read_text()

        self.assertNotIn("LocalStorage", qml)
        self.assertIn("backend.sendMessage(11", qml)
        self.assertIn("backend.sendMessage(12", qml)
        self.assertIn("backend.sendMessage(13", qml)
        self.assertIn("type === 104", qml)
        self.assertIn("type === 105", qml)
        self.assertIn("CHECKMATE", qml)

    def test_frontend_offers_all_promotion_choices_before_sending_the_move(self):
        qml = (Path(__file__).parents[1] / "packaging" / "appload-frontend" / "ui" / "Chess.qml").read_text()

        self.assertIn('property string pendingPromotionMove: ""', qml)
        self.assertIn('function isPromotionMove(fromSquare, toSquare)', qml)
        self.assertIn('if (isPromotionMove(selected, square))', qml)
        self.assertIn('pendingPromotionMove = squareName(selected) + squareName(square)', qml)
        self.assertIn('pendingPromotionMove + choice', qml)
        self.assertIn('model: ["Q", "R", "B", "N"]', qml)
        self.assertIn('id: promotionChooser', qml)
        self.assertIn('columns: 2', qml)
        self.assertIn('x: gameLayout.x + (root.boardSize - width) / 2', qml)
        self.assertIn('y: gameLayout.y + 96 + (root.boardSize - height) / 2', qml)

    def test_backend_accepts_promotion_suffixes_from_players_and_engine(self):
        backend = (Path(__file__).parents[1] / "src" / "appload_backend.cpp").read_text()

        self.assertIn('contents.size() != 4 && contents.size() != 5', backend)
        self.assertIn('contents.size() == 5 ? contents[4] : \'\\0\'', backend)
        self.assertIn('reply->size() == 5 ? (*reply)[4] : \'\\0\'', backend)


if __name__ == "__main__":
    unittest.main()
