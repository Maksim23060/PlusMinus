"""Pure-Python metrics, built-in regression cases and persistent run history.

Source metrics are explicitly heuristic; runtime and bytecode metrics are
reported separately to avoid presenting guesses as observed measurements.
"""
from __future__ import annotations

import hashlib
import json
import re
import sqlite3
import statistics
from contextlib import contextmanager
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

KEYWORDS = {'if', 'else', 'while', 'func', 'proc', 'return', 'go', 'sync', 'lock',
            'unlock', 'print', 'len', 'resize', 'sin', 'cos', 'pow', 'fact', 'multifact'}


def sanitize_source(source: str) -> tuple[str, int]:
    """Mask comments and literals while retaining line breaks and offsets."""
    result = list(source)
    comments = 0
    i = 0
    while i < len(source):
        if source.startswith('//', i):
            comments += 1
            j = source.find('\n', i)
            if j < 0:
                j = len(source)
            for k in range(i, j):
                result[k] = ' '
            i = j
        elif source.startswith('/*', i):
            comments += 1
            j = source.find('*/', i + 2)
            j = len(source) if j < 0 else j + 2
            for k in range(i, j):
                if source[k] != '\n':
                    result[k] = ' '
            i = j
        elif source[i] in ('"', "'"):
            quote = source[i]
            j = i + 1
            while j < len(source):
                if source[j] == '\\':
                    j += 2
                elif source[j] == quote:
                    j += 1
                    break
                else:
                    j += 1
            for k in range(i, min(j, len(source))):
                if source[k] != '\n':
                    result[k] = ' '
            i = j
        else:
            i += 1
    return ''.join(result), comments


def analyze_source(source: str) -> dict:
    masked, comment_count = sanitize_source(source)
    nonempty = sum(bool(line.strip()) for line in masked.splitlines())
    tokens = re.findall(r'\b[A-Za-z_]\w*\b', masked)
    counts = Counter(token for token in tokens if token in KEYWORDS)
    max_depth = depth = 0
    for char in masked:
        if char == '{':
            depth += 1
            max_depth = max(max_depth, depth)
        elif char == '}':
            depth = max(0, depth - 1)
    variables = set(re.findall(r'(?<![=!<>])\b([A-Za-z_]\w*)\s*=(?!=)', masked)) - KEYWORDS
    function_names = re.findall(r'\b(?:func|proc)\s+([A-Za-z_]\w*)', masked)
    decisions = counts['if'] + counts['while'] + len(re.findall(r'&&|\|\|', masked))
    warnings = []
    if max_depth >= 5:
        warnings.append(f'High syntactic nesting depth: {max_depth}')
    if counts['go'] and not counts['sync']:
        warnings.append('Concurrent calls without an explicit sync keyword; review synchronization')
    if counts['lock'] != counts['unlock']:
        warnings.append('Unbalanced lock/unlock keyword counts; inspect the control flow')
    if nonempty >= 300:
        warnings.append('Large source file: consider extracting functions')
    return {
        'lines': len(source.splitlines()), 'code_lines': nonempty, 'comments': comment_count,
        'functions': counts['func'], 'procedures': counts['proc'],
        'function_names': function_names, 'variables': len(variables),
        'branches': counts['if'], 'loops': counts['while'], 'go_calls': counts['go'],
        'sync': counts['sync'], 'locks': counts['lock'],
        'max_nesting': max_depth, 'complexity_estimate': 1 + decisions,
        'warnings': warnings, 'method': 'Lexical heuristic; not an AST-based correctness proof',
    }


def analyze_bytecode(listing: str) -> dict:
    counts = Counter()
    for line in listing.splitlines():
        match = re.match(r'^\s*\d+\s+([A-Za-z_][A-Za-z_0-9]*)\b', line)
        if match:
            counts[match.group(1)] += 1
    return {'instructions': sum(counts.values()), 'opcodes': dict(counts.most_common())}


BUILTIN_TESTS = [
    {'name': 'Arithmetic precedence', 'source': 'print(2 + 3 * 4);',
     'mode': 'run', 'kind': 'output', 'expected': '14\n', 'expect_ok': True},
    {'name': 'Variable assignment', 'source': 'x = 10; print(x);',
     'mode': 'run', 'kind': 'output', 'expected': '10\n', 'expect_ok': True},
    {'name': 'Compilation and bytecode', 'source': 'print(2);',
     'mode': 'compile', 'kind': 'bytecode_contains', 'expected': 'Call', 'expect_ok': True},
    {'name': 'Syntax error detected', 'source': 'if (1) {',
     'mode': 'run', 'kind': 'error_contains', 'expected': 'compile:', 'expect_ok': False},
    {'name': 'Infinite loop timeout', 'source': 'while (1) {}',
     'mode': 'run', 'kind': 'error_contains', 'expected': 'timed out', 'expect_ok': False, 'timeout': 0.2},
]


def judge_test(case: dict, report: dict) -> tuple[bool, str]:
    if bool(report.get('ok')) != bool(case.get('expect_ok', True)):
        return False, f"Unexpected execution state: {report.get('error', '')}"
    actual = str(report.get(case.get('kind', 'output').removesuffix('_contains'), ''))
    expected = str(case.get('expected', ''))
    if case.get('kind') in ('error_contains', 'bytecode_contains'):
        ok = expected in actual
    else:
        ok = actual.replace('\r\n', '\n') == expected.replace('\r\n', '\n')
    return ok, '' if ok else f'Expected: {expected!r}; actual: {actual!r}'


def summarize_benchmark(times: list[float]) -> dict:
    if not times:
        return {}
    ordered = sorted(times)
    def percentile(p):
        pos = (len(ordered) - 1) * p / 100
        lo = int(pos)
        hi = min(lo + 1, len(ordered) - 1)
        return ordered[lo] * (hi - pos) + ordered[hi] * (pos - lo)
    return {'runs': len(times), 'mean': statistics.mean(times),
            'median': statistics.median(times), 'minimum': ordered[0], 'maximum': ordered[-1],
            'stdev': statistics.stdev(times) if len(times) > 1 else 0.0,
            'p95': percentile(95), 'samples': times}


class HistoryStore:
    def __init__(self, path: Path | None = None):
        self.path = path or Path.home() / '.plusminus' / 'studio.db'
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self._connect() as db:
            db.execute('''CREATE TABLE IF NOT EXISTS runs (
                 id INTEGER PRIMARY KEY AUTOINCREMENT, created TEXT NOT NULL,
                 category TEXT NOT NULL, label TEXT NOT NULL, ok INTEGER NOT NULL,
                 elapsed_ms REAL NOT NULL, source_hash TEXT NOT NULL,
                 details TEXT NOT NULL)''')

    @contextmanager
    def _connect(self):
        connection = sqlite3.connect(str(self.path), timeout=5)
        try:
            with connection:
                yield connection
        finally:
            connection.close()

    def add(self, category: str, label: str, source: str, ok: bool,
            elapsed_ms: float, details: dict | None = None) -> int:
        with self._connect() as db:
            cur = db.execute('''INSERT INTO runs
                (created,category,label,ok,elapsed_ms,source_hash,details)
                VALUES(?,?,?,?,?,?,?)''',
                (datetime.now(timezone.utc).isoformat(timespec='seconds'), category, label,
                 int(ok), float(elapsed_ms), hashlib.sha256(source.encode('utf-8')).hexdigest()[:12],
                 json.dumps(details or {}, ensure_ascii=False)))
            return int(cur.lastrowid)

    def recent(self, limit: int = 150) -> list[dict]:
        with self._connect() as db:
            rows = db.execute('''SELECT id,created,category,label,ok,elapsed_ms,source_hash,details
               FROM runs ORDER BY id DESC LIMIT ?''', (limit,)).fetchall()
        return [dict(zip(('id','created','category','label','ok','elapsed_ms','source_hash','details'),
                         (*row[:7], json.loads(row[7])))) for row in rows]

    def overview(self) -> dict:
        with self._connect() as db:
            total, passed = db.execute('SELECT COUNT(*), COALESCE(SUM(ok),0) FROM runs').fetchone()
            last = db.execute('''SELECT elapsed_ms FROM runs WHERE category='Run'
                                  ORDER BY id DESC LIMIT 1''').fetchone()
        return {'runs': total, 'passed': passed,
                'last_ms': last[0] if last else None}
