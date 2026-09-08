"""Observable safety regressions for the one-way vault compatibility view."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from bmad_vault import project, sync, matches, OUTPUT


class BmadVaultTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.vault = self.root / 'vault'
        self.sprint('Sprint 16', True)

    def write(self, path, text):
        path = self.vault / path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')
        return path

    def sprint(self, name, active):
        return self.write(f'production/sprints/{name}.md',
                          f'---\ntype: sprint\nstatus: {"In Progress" if active else "Done"}\ncurrent: {str(active).lower()}\n---\n')

    def card(self, name, kind, status, review='Pending', sprint='Sprint 16'):
        return self.write(f'production/{"tasks" if kind == "task" else "bugs"}/{name}.md',
                          f'---\ntype: {kind}\nstatus: {status}\nreview: {review}\nsprint: {sprint}\n---\n# {name}\n')

    def test_task_and_bug_review_split_and_no_automatic_pickup(self):
        self.card('work', 'task', 'In review')
        self.card('bug', 'bug', 'Investigating', 'In review')
        self.card('fix-needed', 'bug', 'Investigating', 'Needs fixes')
        data = project(self.vault)
        for key, item in data['vault_metadata'].items():
            expected = 'in-progress' if item['review'] == 'Needs fixes' else 'review'
            self.assertEqual(data['development_status'][key], expected)
            self.assertFalse(item['pickup_authorized'])
            if item['type'] == 'bug':
                self.assertEqual(item['status'], 'Investigating')
        self.assertTrue(any(x['blocked'] for x in data['vault_metadata'].values()))

    def test_all_statuses_and_lossy_native_states_retain_meaning(self):
        task_map = {'Not started': 'backlog', 'In progress': 'in-progress', 'In review': 'review',
                    'Blocked': 'backlog', 'Done': 'done', 'Archived': 'done'}
        bug_map = {'Open': 'backlog', 'Investigating': 'in-progress', 'Fixed': 'done', 'Wont Fix': 'done'}
        for kind, statuses in (('task', task_map), ('bug', bug_map)):
            for index, status in enumerate(statuses):
                self.card(f'{kind}-{index}', kind, status)
        data = project(self.vault)
        for key, item in data['vault_metadata'].items():
            expected = (task_map if item['type'] == 'task' else bug_map)[item['status']]
            self.assertEqual(data['development_status'][key], expected)
            self.assertEqual(item['excluded'], item['status'] in {'Archived', 'Wont Fix'})
            self.assertEqual(item['completed'], item['status'] in {'Done', 'Fixed'})
            self.assertEqual(item['blocked'], item['status'] == 'Blocked')

    def test_only_current_sprint_and_explicit_empty_between_sprints(self):
        self.card('current', 'task', 'In progress')
        self.card('future', 'task', 'Not started', sprint='Sprint 17')
        self.card('past', 'task', 'Done', sprint='Sprint 15')
        self.assertEqual(len(project(self.vault)['card_paths']), 1)
        self.sprint('Sprint 16', False)
        data = project(self.vault)
        self.assertEqual(data['current_sprint'], '__NO_ACTIVE_SPRINT__')
        self.assertEqual(data['development_status'], {})

    def test_duplicate_sprints_and_invalid_statuses_fail_closed(self):
        self.sprint('Sprint 17', True)
        with self.assertRaisesRegex(ValueError, 'Multiple current'):
            project(self.vault)
        self.sprint('Sprint 17', False)
        self.card('invalid', 'bug', 'In review')
        with self.assertRaisesRegex(ValueError, 'unsupported bug'):
            project(self.vault)

    def test_source_change_and_hand_edit_are_detected_without_vault_mutation(self):
        card = self.card('gate', 'task', 'Blocked')
        before = card.read_bytes()
        sync(self.root)
        self.assertEqual(card.read_bytes(), before)
        path = self.root / OUTPUT
        self.assertTrue(matches(path, project(self.vault)))
        data = json.loads(path.read_text(encoding='utf-8'))
        key = next(iter(data['card_paths']))
        data['development_status'][key] = 'ready-for-dev'
        path.write_text(json.dumps(data), encoding='utf-8')
        self.assertFalse(matches(path, project(self.vault)))
        sync(self.root)
        self.assertEqual(card.read_bytes(), before)
        card.write_text(card.read_text() + 'Changed acceptance\n', encoding='utf-8')
        self.assertFalse(matches(path, project(self.vault)))

    def test_duplicate_slugs_remain_distinct_and_sync_is_idempotent(self):
        self.card('same', 'task', 'Not started')
        self.card('same', 'bug', 'Open')
        sync(self.root)
        data = project(self.vault)
        self.assertEqual(len(set(data['card_paths'].values())), 2)
        before = (self.root / OUTPUT).read_bytes()
        sync(self.root)
        self.assertEqual((self.root / OUTPUT).read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
