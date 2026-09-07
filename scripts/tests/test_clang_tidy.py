import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("run_clang_tidy", Path(__file__).parents[1] / "run_clang_tidy.py")
tidy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tidy)


class ClangTidySelectionTest(unittest.TestCase):
    def test_exact_target_selection_deduplicates_and_excludes_vendor_and_renderer(self):
        with tempfile.TemporaryDirectory() as directory:
            entries = [{"directory": directory, "file": f"{target}.cpp",
                        "output": f"{directory}/CMakeFiles/{target}.dir/a.cpp.o"}
                       for target in ["rat_core", "rat_editor_logic", "rat_core_extra", "rat_engine", "imgui"]]
            entries.append(entries[0].copy())
            selected = tidy.selected_entries(entries)
            self.assertEqual([v["target"] for v in selected], ["rat_core", "rat_editor_logic"])
            self.assertTrue(all(Path(v["file"]).is_absolute() for v in selected))

    def test_windows_separators_and_missing_target_fail_closed(self):
        entries = [{"directory": ".", "file": "a.cpp", "output": r"build\CMakeFiles\rat_core.dir\a.obj"}]
        with self.assertRaises(ValueError):
            tidy.selected_entries(entries)
        entries.append({"directory": ".", "file": "b.cpp",
                        "command": r"cl /FoCMakeFiles\rat_editor_logic.dir\b.obj /c b.cpp"})
        # Command fallback also accepts MSVC's joined /Fo output switch.
        self.assertEqual(len(tidy.selected_entries(entries)), 2)


if __name__ == "__main__":
    unittest.main()
