"""Regression checks for the cross-platform launcher."""
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import launch

class LauncherTests(unittest.TestCase):
    def test_missing_dependencies(self):
        with patch.object(launch, 'has_tk', return_value=False), \
             patch.object(launch, 'find_cmake', return_value=None), \
             patch.object(launch, 'has_compiler', return_value=False):
            self.assertEqual(launch.missing_dependencies(), ['tk', 'cmake', 'cxx'])

    def test_existing_runner_reused(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'Parser.cpp'
            runner = Path(directory) / 'plusminus_runner'
            source.write_text('int x;')
            runner.write_text('bin')
            with patch.object(launch, 'SOURCES', (source,)):
                self.assertFalse(launch.needs_build(runner))
                os.utime(source, (runner.stat().st_mtime + 10, runner.stat().st_mtime + 10))
                self.assertTrue(launch.needs_build(runner))

    def test_check_never_installs(self):
        with patch.object(sys, 'argv', ['launch.py', '--check']), \
             patch.object(launch, 'missing_dependencies', return_value=['tk']), \
             patch.object(launch, 'source_runner', return_value=None), \
             patch.object(launch, 'find_cmake', return_value=None), \
             patch.object(launch, 'has_compiler', return_value=True), \
             patch.object(launch, 'has_tk', return_value=False), \
             patch.object(launch, 'run') as runner:
            self.assertEqual(launch.main(), 1)
            runner.assert_not_called()
