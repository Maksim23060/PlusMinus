import json
import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'ide'))
from engine import execute, check


@unittest.skipIf(os.name == 'nt', 'Test helper is a POSIX executable')
class TelemetryTests(unittest.TestCase):
    def executable(self, folder, script):
        path = Path(folder) / 'runner'
        path.write_text('#!/usr/bin/env python3\n'+script,encoding='utf-8')
        path.chmod(path.stat().st_mode | stat.S_IXUSR)
        return path

    def test_runner_metadata_and_compatibility(self):
        with tempfile.TemporaryDirectory() as folder:
            runner = self.executable(folder, '''import json, sys\nsrc = sys.stdin.read()\nprint(json.dumps({'ok':True,'output':src,'result':'','variables':'','bytecode':'','error':'','metrics':{'compile_ms':0.1,'execute_ms':0.2,'program_instructions':4}}))\n''')
            report = execute(runner,'hello',timeout=4)
            self.assertTrue(report['ok'],report)
            self.assertTrue(check({'kind':'output','expected':'hello'},report))
            self.assertEqual(report['metrics']['program_instructions'],4)
            self.assertGreater(report['ms'],0)

    def test_timeout(self):
        with tempfile.TemporaryDirectory() as folder:
            runner = self.executable(folder, 'import time\ntime.sleep(4)\n')
            report = execute(runner,'',timeout=0.13)
            self.assertFalse(report['ok'])
            self.assertIn('timed out',report['error'])


if __name__=='__main__':
    unittest.main()
