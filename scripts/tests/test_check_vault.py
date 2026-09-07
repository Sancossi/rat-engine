import sys
from pathlib import Path
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from check_vault import check, prose


class VaultCheckTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.write('production/Task board.base', 'file.inFolder("production/bugs")\ntype == "bug"\n  - type: cards\n    name: Current sprint\n    filters:\n      - sprint == "__NO_ACTIVE_SPRINT__"\n')

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')

    def test_alias_filename_and_same_note_heading(self):
        self.write('note.md', '---\ntype: note\n---\n# Different title\n[[note|Title]] [[#Heading]]\n')
        self.assertEqual(check(self.root), [])

    def test_heading_is_not_filename(self):
        self.write('note.md', '---\ntype: note\n---\n# Title\n[[Title]]\n')
        self.assertIn('unresolved or ambiguous', '\n'.join(check(self.root)))

    def test_inline_cpp_attributes_are_not_links(self):
        self.write('note.md', '---\ntype: note\n---\nUse `[[nodiscard]]` and ``x ` [[example]]``. [[note]]\n')
        self.assertEqual(check(self.root), [])

    def test_fences_and_duplicate_filename(self):
        self.write('a/note.md', '---\ntype: note\n---\n~~~md\n[[missing]]\n~~~\n[[note]]\n')
        self.write('b/note.md', '---\ntype: note\n---\n')
        failures = '\n'.join(check(self.root))
        self.assertNotIn('missing', failures)
        self.assertIn('ambiguous', failures)
        self.assertEqual(list(prose('````md\n```\n[[hidden]]\n````\nvisible')), ['visible'])

    def test_invalid_required_enum_and_duplicate_fields(self):
        self.write('bad.md', '---\ntype: bug\nstatus: In progress\nstatus: In review\nseverity: Huge\n---\n')
        failures = '\n'.join(check(self.root))
        for expected in ('missing required area', 'invalid status', 'invalid severity', 'duplicate field status'):
            self.assertIn(expected, failures)

    def test_current_sprint_and_board_mismatch(self):
        for n in (1, 2):
            self.write(f'production/sprints/Sprint {n}.md', '---\ntype: sprint\nstatus: In Progress\ndates: future\ngoal: test\ncurrent: true\n---\n')
        self.assertIn('multiple current sprints', '\n'.join(check(self.root)))
        (self.root / 'production/sprints/Sprint 2.md').unlink()
        self.assertIn('must filter Sprint 1', '\n'.join(check(self.root)))

    def test_current_sprint_with_matching_board_and_assigned_bug(self):
        self.write('production/sprints/Sprint 1.md', '---\ntype: sprint\nstatus: In Progress\ndates: now\ngoal: test\ncurrent: true\n---\n')
        board = self.root / 'production/Task board.base'
        board.write_text(board.read_text().replace('__NO_ACTIVE_SPRINT__', 'Sprint 1'))
        self.write('bug.md', '---\ntype: bug\narea: Engine\nstatus: Investigating\nreview: In review\nseverity: High\nsprint: Sprint 1\n---\n')
        self.assertEqual(check(self.root), [])

    def test_unknown_sprint_and_missing_bug_board(self):
        self.write('task.md', '---\ntype: task\narea: Engine\nstatus: Done\ntask_type: Chore\nsprint: Sprint 9\n---\n')
        self.write('production/Task board.base', 'name: Current sprint\n')
        failures = '\n'.join(check(self.root))
        self.assertIn('unknown sprint Sprint 9', failures)
        self.assertIn('board must include bug', failures)


if __name__ == '__main__':
    unittest.main()
