"""QA assets must not turn harmless authoring names/hierarchy into build failures."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('native_fixtures', ROOT / 'games/rat-expedition/tools/build_native_fixtures.py')
fixtures = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixtures)


class NativeFixtureGenerationTests(unittest.TestCase):
    def test_renamed_wall_and_root_still_generate_all_native_packages(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            assets = root / 'Assets'
            assets.mkdir()
            for name in ('Courtyard', 'Sluice', 'ExpeditionProject'):
                text = (fixtures.ASSETS / (name + '.sdscene')).read_text(encoding='utf8')
                text = text.replace('Name: courtyard-wall', 'Name: A renamed wall')
                text = text.replace('Name: expedition_courtyard', 'Name: A renamed courtyard')
                (assets / (name + '.sdscene')).write_text(text, encoding='utf8')
            previous = fixtures.ASSETS
            try:
                fixtures.ASSETS = assets
                fixtures.build(root / 'generated/QA')
            finally:
                fixtures.ASSETS = previous
            invalid = fixtures.Scene(root / 'generated/QA/Scene-invalid-size.sdscene')
            wall = invalid.part('courtyard-wall')
            self.assertIn('Name: A renamed wall', wall)
            self.assertIn('Size: {X: 0, Y: 1.8, Z: .5}', wall)
            self.assertTrue((root / 'generated/Rat.Expedition.Qualification.sdpkg').is_file())

    def test_writer_does_not_promote_non_root_entity_to_root(self):
        scene = fixtures.Scene(fixtures.ASSETS / 'Courtyard.sdscene')
        child_id = scene.id('courtyard-wall')
        # The writer must preserve the native root list, independently of parts.
        scene.roots.remove(child_id)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            scene.write(root, 'nested')
            restored = fixtures.Scene(root / 'nested.sdscene')
            self.assertNotIn(child_id, restored.roots)
            self.assertEqual(restored.id('courtyard-wall'), child_id)
