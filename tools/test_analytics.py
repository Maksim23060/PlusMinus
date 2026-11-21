import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'ide'))
from analytics import analyze_source, analyze_bytecode, judge_test, summarize_benchmark, HistoryStore


class AnalyticsTests(unittest.TestCase):
    def test_ignore_strings_comments(self):
        a = analyze_source('print("while { if"); // go while\nwhile (1) { x = 2; }')
        self.assertEqual(a['loops'], 1)
        self.assertEqual(a['branches'], 0)
        self.assertEqual(a['go_calls'], 0)
        self.assertEqual(a['max_nesting'], 1)

    def test_bytecode(self):
        b = analyze_bytecode('  0  PushConst  1\n  1  Call  print\n  2  PushConst  2')
        self.assertEqual(b['instructions'], 3)
        self.assertEqual(b['opcodes']['PushConst'], 2)

    def test_test_judge(self):
        self.assertTrue(judge_test({'kind':'output','expected':'4\n'}, {'ok':True,'output':'4\r\n'})[0])
        self.assertTrue(judge_test({'kind':'error_contains','expected':'compile:','expect_ok':False}, {'ok':False,'error':'compile: bad'})[0])
        self.assertFalse(judge_test({'kind':'output','expected':'5'}, {'ok':True,'output':'4'})[0])

    def test_benchmark(self):
        s = summarize_benchmark([1., 2., 3., 4., 5.])
        self.assertEqual(s['median'], 3.)
        self.assertAlmostEqual(s['p95'], 4.8)

    def test_history(self):
        with tempfile.TemporaryDirectory() as d:
            db = HistoryStore(Path(d) / 'history.db')
            db.add('Run','smoke','print(1);',True,1.23, {'cpu':2})
            self.assertEqual(db.overview()['runs'], 1)
            self.assertEqual(db.recent()[0]['details']['cpu'], 2)


if __name__ == '__main__':
    unittest.main()
