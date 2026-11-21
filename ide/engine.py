"""Subprocess boundary: no interpreter code runs in the GUI process."""
import json
import subprocess
import threading
import time
from pathlib import Path


def find_runner():
    base = Path(__file__).resolve().parent
    for folder in (base / 'build', base / 'build' / 'Release', base / 'build' / 'Debug', base):
        for name in ('plusminus_runner', 'plusminus_runner.exe'):
            if (folder / name).is_file():
                return folder / name
    return None


def execute(path, source, mode='run', timeout=5.0, cancel=None):
    start = time.perf_counter()
    cancel = cancel or threading.Event()
    report = dict(ok=False, output='', result='', variables='', bytecode='', error='')
    process = None
    try:
        if cancel.is_set():
            raise RuntimeError('Stopped by user')
        process = subprocess.Popen([str(Path(path).resolve()), mode], stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        data = source.encode('utf-8')
        while True:
            remaining = timeout - (time.perf_counter() - start)
            if cancel.is_set() or remaining <= 0:
                process.kill()
                process.communicate()
                raise RuntimeError('Stopped by user' if cancel.is_set() else 'Execution timed out')
            try:
                stdout, stderr = process.communicate(input=data, timeout=min(0.1, remaining))
                break
            except subprocess.TimeoutExpired:
                data = None
        try:
            decoded = json.loads(stdout.decode('utf-8', errors='replace'))
            if not isinstance(decoded, dict) or not isinstance(decoded.get('ok'), bool):
                raise ValueError('Invalid runner report')
            report.update({k: decoded[k] for k in report if k in decoded})
            if process.returncode and report['ok']:
                report.update(ok=False, error=f'Runner exited with code {process.returncode}')
        except (ValueError, UnicodeError):
            report['error'] = f'Runner exited with code {process.returncode}; invalid response.'
        if stderr:
            report['error'] += ('\n' if report['error'] else '') + stderr.decode('utf-8', errors='replace')
    except (OSError, RuntimeError) as exc:
        report['error'] = str(exc)
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.communicate()
    report['ms'] = round((time.perf_counter() - start)*1000, 2)
    return report


def check(case, report):
    # Exact comparison apart from platform-specific line endings.
    normalize = lambda s: s.replace('\r\n', '\n')
    return report['ok'] and normalize(report[case['kind']]) == normalize(case['expected'])
