"""Validate the vault's deliberately flat scalar frontmatter and wikilink targets.

No YAML dependency: nested/list metadata is retained but not interpreted. Required
production fields must be single-line scalars. This checker never rewrites notes.
"""
from pathlib import Path
import argparse
import re


STATUS = {
    'task': {'Not started', 'In progress', 'In review', 'Blocked', 'Done', 'Archived'},
    'bug': {'Open', 'Investigating', 'Fixed', 'Wont Fix'},
    'sprint': {'Planned', 'In Progress', 'Done'},
    'roadmap': {'Idea', 'Planned', 'In Progress', 'Done'},
    'adr': {'Proposed', 'Accepted', 'Deprecated', 'Superseded', 'Rejected'},
}
REQUIRED = {
    'task': ('area', 'status', 'task_type'),
    'bug': ('area', 'status', 'severity'),
    'sprint': ('status', 'dates', 'goal', 'current'),
    'roadmap': ('area', 'priority', 'status', 'target'),
    'adr': ('area', 'status', 'decided'),
}


def prose(text):
    """Drop fenced examples (both Markdown fence kinds, including longer fences)."""
    fence = None
    for line in text.splitlines():
        marker = re.match(r'^ {0,3}(`{3,}|~{3,})(.*)$', line)
        if marker:
            run, tail = marker.groups()
            if fence is None:
                fence = run
            elif run[0] == fence[0] and len(run) >= len(fence) and not tail.strip():
                fence = None
            continue
        if fence is None:
            # A matching backtick run delimits inline code, even if its contents
            # include shorter runs (Markdown's multi-backtick code spans).
            yield re.sub(r'(?<!`)(`+)(?!`)(.*?)\1(?!`)', '', line)


def frontmatter(text):
    lines = text.splitlines()
    if not lines or lines[0] != '---':
        return {}, ['missing frontmatter']
    try:
        end = lines.index('---', 1)
    except ValueError:
        return {}, ['unterminated frontmatter']
    fields, errors = {}, []
    for line in lines[1:end]:
        match = re.match(r'^([a-z_]+):\s*(.*)$', line)
        if match:
            key, value = match.groups()
            if key in fields:
                errors.append(f'duplicate field {key}')
            fields[key] = value.strip().strip('\"\'')
    return fields, errors


def check(root):
    root = Path(root)
    files = [p for p in root.rglob('*') if p.is_file() and not any(x.startswith('.') for x in p.relative_to(root).parts)]
    stems = {}
    for path in files:
        stems.setdefault(path.stem, []).append(path)
    errors, active, sprints, assigned = [], [], set(), []
    for path in files:
        if path.suffix != '.md' or 'templates' in path.relative_to(root).parts:
            continue
        text = path.read_text(encoding='utf-8-sig')
        fields, failures = frontmatter(text)
        kind = fields.get('type')
        if kind not in {'task', 'bug', 'sprint', 'roadmap', 'adr', 'note', 'index', 'moc'}:
            failures.append(f'invalid type {kind!r}')
        for key in REQUIRED.get(kind, ()):
            # target and decided can be explicitly empty for future work.
            if key not in fields or (not fields[key] and key not in {'target', 'decided'}):
                failures.append(f'missing required {key}')
        enums = {'area': {'Engine', 'Game', 'Production'},
                 'task_type': {'Feature', 'Bug', 'Chore', 'Research'},
                 'severity': {'Critical', 'High', 'Medium', 'Low'},
                 'priority': {'P0', 'P1', 'P2', 'P3'},
                 'review': {'Pending', 'In review', 'Approved', 'Needs fixes'},
                 'current': {'true', 'false'}}
        if kind in STATUS:
            enums['status'] = STATUS[kind]
        for key, values in enums.items():
            if key in fields and fields[key] not in values:
                failures.append(f'invalid {key}: {fields[key]}')
        if kind == 'sprint':
            label = re.match(r'Sprint \d+', path.stem)
            if not label:
                failures.append('sprint filename must start with Sprint N')
            else:
                sprints.add(label[0])
                if fields.get('current') == 'true':
                    active.append(label[0])
                    if fields.get('status') != 'In Progress':
                        failures.append('current sprint must be In Progress')
        if fields.get('sprint'):
            assigned.append((path, fields['sprint']))
        for line in prose(text):
            for link in re.findall(r'\[\[([^\]]+)\]\]', line):
                target = link.split('|', 1)[0].split('#', 1)[0].strip()
                if not target:  # same-note heading
                    continue
                if '/' in target:
                    candidates = [root / target, path.parent / target]
                    found = any(p.is_file() or p.with_suffix('.md').is_file() for p in candidates)
                else:
                    candidates = [p for p in stems.get(Path(target).stem, [])
                                  if not Path(target).suffix or p.name == target]
                    found = len(candidates) == 1
                if not found:
                    failures.append(f'unresolved or ambiguous wikilink: {target}')
        errors.extend(f'{path.relative_to(root)}: {failure}' for failure in failures)
    errors.extend(f'{p.relative_to(root)}: unknown sprint {s}' for p, s in assigned if s not in sprints)
    if len(active) > 1:
        errors.append('multiple current sprints: ' + ', '.join(active))
    board = root / 'production/Task board.base'
    if board.exists():
        text = board.read_text(encoding='utf-8-sig')
        section = re.search(r'name: Current sprint\n(.*?)(?=\n  - type:|\Z)', text, re.S)
        expected = active[0] if len(active) == 1 else '__NO_ACTIVE_SPRINT__'
        if not section or f'sprint == "{expected}"' not in section[1]:
            errors.append(f'Task board.base: Current sprint must filter {expected}')
        if 'file.inFolder("production/bugs")' not in text or 'type == "bug"' not in text:
            errors.append('Task board.base: board must include bug cards')
    else:
        errors.append('missing production/Task board.base')
    return errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', nargs='?', type=Path, default=Path(__file__).resolve().parents[1] / 'vault')
    args = parser.parse_args()
    failures = check(args.root)
    print('\n'.join(failures) if failures else 'Vault checks passed.')
    raise SystemExit(bool(failures))
