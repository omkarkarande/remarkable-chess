import json
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


class ApploadFrontendManifestTests(unittest.TestCase):
    @property
    def repository_root(self):
        return Path(__file__).parents[1]

    @property
    def app_root(self):
        return self.repository_root / "packaging" / "appload-frontend"

    def test_frontend_manifest_has_required_appload_contract(self):
        manifest = json.loads((self.app_root / "manifest.json").read_text())

        self.assertEqual("Chess", manifest["name"])
        self.assertEqual("omi.remarkable-chess", manifest["id"])
        self.assertTrue(manifest["loadsBackend"])
        self.assertEqual("/ui/Chess.qml", manifest["entry"])
        self.assertTrue(manifest["supportsScaling"])
        self.assertFalse(manifest["canHaveMultipleFrontends"])
        self.assertTrue((self.app_root / manifest["entry"].lstrip("/")).is_file())

    def test_qrc_contains_the_current_appload_entry_and_all_runtime_images(self):
        manifest = json.loads((self.app_root / "manifest.json").read_text())
        qrc = ET.parse(self.app_root / "application.qrc")
        resources = {element.text for element in qrc.findall(".//file")}
        expected_images = {
            "ui/undo-icon.png",
            *{f"ui/pieces/{side}{piece}.png" for side in ("w", "b") for piece in "KQRBNP"},
        }

        self.assertEqual(manifest["entry"].lstrip("/"), "ui/Chess.qml")
        self.assertIn(manifest["entry"].lstrip("/"), resources)
        self.assertSetEqual(resources, {manifest["entry"].lstrip("/"), *expected_images})
        # AppLoad receives the compiled QRC bundle, not loose QML files alone.
        self.assertGreater((self.app_root / "resources.rcc").stat().st_size, 0)

    def test_vellum_release_artifact_contract_is_canonical(self):
        packaging = self.repository_root / "packaging"
        external_manifest = json.loads((packaging / "external.manifest.armv7.json").read_text())
        velbuild = (packaging / "VELBUILD").read_text()

        self.assertEqual("remarkable_chess", external_manifest["application"])
        self.assertTrue(external_manifest["qtfb"])
        self.assertEqual("software", external_manifest["environment"]["QT_QUICK_BACKEND"])
        self.assertIn('license="GPL-2.0-or-later"', velbuild)
        self.assertIn("releases/download/$_release_tag/$_release_archive", velbuild)
        self.assertIn('install -Dm755 "$payload/backend/entry"', velbuild)
        self.assertIn('install -Dm755 "$payload/backend/stockfish"', velbuild)
        self.assertIn("/home/root/xovi/exthome/appload/$pkgname", velbuild)
        self.assertNotIn("nn-ab28990d4ea3.nnue", velbuild)
        self.assertNotIn("image=", velbuild)
        self.assertNotIn("build()", velbuild)
        self.assertFalse((packaging / "vellum" / "VELBUILD").exists())

    def test_frontend_delegates_game_state_and_controls_to_backend(self):
        qml = (self.app_root / "ui" / "Chess.qml").read_text()

        self.assertNotIn("LocalStorage", qml)
        self.assertIn("backend.sendMessage(11", qml)
        self.assertIn("backend.sendMessage(12", qml)
        self.assertIn("backend.sendMessage(13", qml)
        self.assertIn("type === 104", qml)
        self.assertIn("type === 105", qml)
        self.assertIn("CHECKMATE", qml)

    def test_frontend_explains_cold_engine_initialization_and_uses_one_second_reply_delay(self):
        qml = (self.app_root / "ui" / "Chess.qml").read_text()

        self.assertIn("interval: 1000", qml)
        self.assertNotIn("interval: 0", qml)
        self.assertIn('backend.sendMessage(14, "go")', qml)

    def test_frontend_blocks_moves_until_backend_state_arrives(self):
        qml = (self.app_root / "ui" / "Chess.qml").read_text()

        self.assertIn("property bool backendReady: false", qml)
        self.assertIn('if (type === 101 && root.applyFen(contents))', qml)
        self.assertIn('root.message = root.turn === "w" ? "White to move" : "Black to move"', qml)
        self.assertIn("if (expanded !== 8) return false", qml)
        self.assertIn("running: root.backendReady && root.turn !== root.playerSide", qml)
        self.assertIn("if (!backendReady || pendingPromotionMove", qml)

    def test_frontend_offers_all_promotion_choices_before_sending_the_move(self):
        qml = (self.app_root / "ui" / "Chess.qml").read_text()

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
        self.assertNotIn('function squareIndex(', qml)
        self.assertNotIn('function promotionViewSquare(', qml)

    def test_backend_accepts_promotion_suffixes_from_players_and_engine(self):
        backend = (self.repository_root / "src" / "appload_backend.cpp").read_text()

        self.assertIn('contents.size() != 4 && contents.size() != 5', backend)
        self.assertIn('contents.size() == 5 ? contents[4] : \'\\0\'', backend)
        self.assertIn('reply->size() == 5 ? (*reply)[4] : \'\\0\'', backend)


if __name__ == "__main__":
    unittest.main()
