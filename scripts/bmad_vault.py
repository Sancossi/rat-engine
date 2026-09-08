"""Project BMad setup checks and one-way, active-sprint vault projection.

Uses the vault's existing scalar parser. JSON output is valid YAML 1.2; the
projection is disposable and can never update the vault or approve pickup.
"""
from pathlib import Path
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import shutil
import subprocess
import sys

from check_vault import frontmatter, STATUS


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path('docs/bmad/generated/sprint-status.yaml')
TASK_STATUS = {'Not started': 'backlog', 'In progress': 'in-progress',
               'In review': 'review', 'Blocked': 'backlog', 'Done': 'done',
               'Archived': 'done'}
BUG_STATUS = {'Open': 'backlog', 'Investigating': 'in-progress',
              'Fixed': 'done', 'Wont Fix': 'done'}


def read_fields(path):
    fields, errors = frontmatter(path.read_text(encoding='utf-8-sig'))
    if errors:
        raise ValueError(f'{path}: {", ".join(errors)}')
    return fields


def project(vault):
    """Return deterministic native status data with lossless source metadata."""
    vault = Path(vault)
    inputs = sorted(p for folder in ('tasks', 'bugs', 'sprints')
                    for p in (vault / 'production' / folder).glob('*.md'))
    board = vault / 'production/Task board.base'
    if board.is_file():
        inputs.append(board)
    digest = hashlib.sha256()
    for path in inputs:
        digest.update(path.relative_to(vault).as_posix().encode('utf-8') + b'\0')
        digest.update(path.read_bytes() + b'\0')
    active = []
    for path in (vault / 'production/sprints').glob('*.md'):
        fields = read_fields(path)
        if fields.get('current') == 'true':
            if fields.get('type') != 'sprint' or fields.get('status') != 'In Progress':
                raise ValueError(f'{path}: current sprint must be In Progress')
            import re
            match = re.match(r'^(Sprint (\d+))(?:\b)', path.stem)
            if not match:
                raise ValueError(f'{path}: expected Sprint N filename')
            active.append((match[1], match[2]))
    if len(active) > 1:
        raise ValueError('Multiple current sprints; refusing ambiguous projection')
    label, number = active[0] if active else ('__NO_ACTIVE_SPRINT__', None)
    statuses, paths, metadata = {}, {}, {}
    cards = []
    for folder, expected in (('tasks', 'task'), ('bugs', 'bug')):
        for path in sorted((vault / 'production' / folder).glob('*.md')):
            fields = read_fields(path)
            if number is None or fields.get('sprint') != label:
                continue
            if fields.get('type') != expected:
                raise ValueError(f'{path}: expected type {expected}')
            status = fields.get('status')
            if status not in STATUS[expected]:
                raise ValueError(f'{path}: unsupported {expected} status {status!r}')
            review = fields.get('review', 'Pending')
            if review not in {'Pending', 'In review', 'Approved', 'Needs fixes'}:
                raise ValueError(f'{path}: invalid review {review!r}')
            cards.append((path, fields, review))
    if cards:
        # A compatibility grouping for this sprint, not a second design epic.
        statuses[f'epic-{number}'] = 'in-progress'
    for index, (path, fields, review) in enumerate(cards, 1):
        kind, status = fields['type'], fields['status']
        key = f'{number}-{index}-{kind}-{path.stem}'
        projected = (TASK_STATUS if kind == 'task' else BUG_STATUS)[status]
        if kind == 'bug' and status == 'Investigating' and review == 'In review':
            projected = 'review'
        statuses[key] = projected
        paths[key] = 'vault/' + path.relative_to(vault).as_posix()
        metadata[key] = {
            'type': kind, 'status': status, 'review': review,
            'blocked': status == 'Blocked' or review == 'Needs fixes',
            'excluded': status in {'Archived', 'Wont Fix'},
            'completed': status in {'Done', 'Fixed'},
            'pickup_authorized': False,
        }
    return {
        'generated_by': 'scripts/bmad_vault.py (DO NOT EDIT)',
        'source_sha256': digest.hexdigest(), 'project': 'Rat Expedition',
        'project_key': 'RAT', 'tracking_system': 'file-system',
        'story_location': 'vault/production', 'current_sprint': label,
        'development_status': statuses, 'card_paths': paths,
        'vault_metadata': metadata,
    }


def matches(path, expected):
    try:
        actual = json.loads(path.read_text(encoding='utf-8'))
    except (OSError, ValueError):
        return False
    if not isinstance(actual, dict):
        return False
    for field in ('generated', 'last_updated'):
        if field not in actual:
            return False
        actual.pop(field)
    return actual == expected


def sync(root):
    expected = project(root / 'vault')
    output = root / OUTPUT
    if not matches(output, expected):
        stamp = datetime.now(timezone.utc).isoformat(timespec='seconds')
        expected.update(generated=stamp, last_updated=stamp)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(expected, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    return expected


def configure(root):
    """Materialize tracked policy through upstream's supported team overrides."""
    try:
        import tomllib
    except ModuleNotFoundError:
        raise ValueError('BMad configure requires Python 3.11+; sync/check remain Python 3.9 compatible') from None
    custom = root / '_bmad/custom'
    if not custom.is_dir():
        raise ValueError('BMad is not installed; run scripts/setup-bmad.ps1')
    source = root / 'tools/bmad'
    shutil.copyfile(source / 'team-config.toml', custom / 'config.toml')
    policy = (source / 'workflow-policy.toml').read_text(encoding='utf-8')
    count = 0
    for skill in sorted((root / '.agents/skills').iterdir()):
        config = skill / 'customize.toml'
        if not skill.name.startswith(('gds-', 'bmad-')) or not config.is_file():
            continue
        if 'workflow' not in tomllib.loads(config.read_text(encoding='utf-8')):
            continue
        override = policy
        if skill.name == 'gds-gdd':
            override += '\noutput_dir = "{project-root}/vault"\noutput_folder_name = "game"\n'
        (custom / f'{skill.name}.toml').write_text(override, encoding='utf-8')
        count += 1
    # GDS legacy skills load YAML directly; keep those generated views consistent
    # with the central TOML overrides, including inherited sprint_artifacts.
    core = {'user_name': 'bogor', 'project_name': 'Rat Expedition',
            'communication_language': 'Russian', 'document_output_language': 'Russian',
            'output_folder': 'vault'}
    gds = dict(core, game_dev_experience='intermediate', planning_artifacts='vault/game',
               implementation_artifacts='docs/bmad/generated', sprint_artifacts='docs/bmad/generated',
               project_knowledge='docs', primary_platform=['unity', 'unreal', 'godot', 'other'])
    for module, data in (('core', core), ('gds', gds)):
        (root / f'_bmad/{module}/config.yaml').write_text(
            '# Generated by scripts/bmad_vault.py configure; edit tools/bmad sources.\n'
            + json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    licenses = root / 'docs/bmad/licenses'
    for path in licenses.glob('*.txt'):
        shutil.copyfile(path, root / '_bmad' / path.name)
    sync(root)
    print(f'Configured {count} workflow overrides; vault remains authoritative')


def smoke(root):
    versions = json.loads((root / 'tools/bmad/versions.json').read_text(encoding='utf-8'))
    manifest = (root / '_bmad/_config/manifest.yaml').read_text(encoding='utf-8')
    for expected in (f'version: {versions["core"]}', f'version: {versions["gds_tag"]}',
                     f'sha: {versions["gds_sha"]}'):
        if expected not in manifest:
            raise ValueError(f'Installed manifest does not contain {expected}')
    with (root / '_bmad/_config/skill-manifest.csv').open(encoding='utf-8', newline='') as stream:
        skills = list(csv.DictReader(stream))
    if len(skills) != versions['skills']:
        raise ValueError(f'Expected {versions["skills"]} skills, found {len(skills)}')
    for entry in skills:
        # Installer 6.12.0 CSV paths refer to pre-deployment locations. Codex's
        # supported path is .agents/skills/<name>/SKILL.md, not that stale column.
        skill = root / '.agents/skills' / entry['name'] / 'SKILL.md'
        if not skill.is_file() or f'name: {entry["name"]}' not in skill.read_text(encoding='utf-8'):
            raise ValueError(f'Installed skill missing or mismatched: {skill}')
    required = {'bmad-help', 'bmad-review', 'gds-create-game-brief', 'gds-gdd',
                'gds-create-narrative', 'gds-game-architecture', 'gds-create-epics-and-stories',
                'gds-create-story', 'gds-dev-story', 'gds-code-review',
                'gds-sprint-status', 'gds-playtest-plan', 'gds-test-design'}
    with (root / '_bmad/_config/bmad-help.csv').open(encoding='utf-8', newline='') as stream:
        names = {row['skill'] for row in csv.DictReader(stream)}
    if not required <= names:
        raise ValueError(f'Help catalog missing skills: {sorted(required - names)}')
    def resolved(script, *args):
        command = [sys.executable, str(root / '_bmad/scripts' / script),
                   '--project-root', str(root), *args]
        result = subprocess.run(command, capture_output=True, encoding='utf-8', check=True)
        return json.loads(result.stdout)
    config = resolved('resolve_config.py')
    if config['core']['communication_language'] != 'Russian' or config['core']['document_output_language'] != 'Russian':
        raise ValueError('Effective config is not Russian')
    for key, value in {'planning_artifacts': 'vault/game',
                       'implementation_artifacts': 'docs/bmad/generated',
                       'sprint_artifacts': 'docs/bmad/generated',
                       'project_knowledge': 'docs'}.items():
        if config['modules']['gds'][key] != value:
            raise ValueError(f'Unexpected effective path: {key}')
        if json.loads((root / '_bmad/gds/config.yaml').read_text(encoding='utf-8').split('\n', 1)[1])[key] != value:
            raise ValueError(f'Legacy YAML differs from effective config: {key}')
    for name in ('gds-gdd', 'gds-create-story', 'gds-sprint-status', 'gds-code-review'):
        workflow = resolved('resolve_customization.py', '--skill', str(root / '.agents/skills' / name), '--key', 'workflow')['workflow']
        if 'file:docs/bmad/project-context.md' not in workflow.get('persistent_facts', []):
            raise ValueError(f'Project workflow policy missing in {name}')
    for name in ('bmad-core-MIT.txt', 'bmad-gds-MIT.txt'):
        if not (root / '_bmad' / name).is_file():
            raise ValueError(f'Missing retained upstream license: {name}')
    if not matches(root / OUTPUT, project(root / 'vault')):
        raise ValueError('Projection is missing or stale: run sync')
    print(f'PASS: {len(skills)} installed skills, help catalog, exact versions, Russian config, '
          'legacy paths, 4 native workflow resolvers, licenses, current vault projection')
    print('Static/resolver smoke only; no interactive skill reload or game/GUI execution claimed.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('sync', 'check', 'configure', 'smoke'))
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args()
    root = args.root.resolve()
    try:
        if args.command == 'sync':
            result = sync(root)
            print(f'Generated projection: {result["current_sprint"]}')
        elif args.command == 'check':
            if not matches(root / OUTPUT, project(root / 'vault')):
                raise ValueError('Missing, stale or edited BMad projection; run python scripts/bmad_vault.py sync')
            print('BMad projection matches authoritative vault')
        elif args.command == 'configure':
            configure(root)
        else:
            smoke(root)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
