## @file
# Tests for FMMT CLI exit status.
#
# Copyright (c) 2026, Zhaoqi Xu <lzy00419@outlook.com>. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import subprocess
import sys
import unittest
from pathlib import Path

_BASETOOLS = Path(__file__).resolve().parent.parent
_PYTHON_SRC = _BASETOOLS / 'Source' / 'Python'
_FMMT_DIR = _PYTHON_SRC / 'FMMT'
_FMMT_PY = _FMMT_DIR / 'FMMT.py'


def _fmmt_env():
    env = os.environ.copy()
    extra = os.pathsep.join((str(_PYTHON_SRC), str(_FMMT_DIR)))
    env['PYTHONPATH'] = extra + os.pathsep + env.get('PYTHONPATH', '')
    return env


class TestFmmtExit(unittest.TestCase):
    def test_view_missing_file_exits_nonzero(self):
        result = subprocess.run(
            [sys.executable, str(_FMMT_PY), '-v', 'not-a-file'],
            env=_fmmt_env(),
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(
            result.returncode,
            0,
            msg='FMMT -v not-a-file must fail: stdout=%r stderr=%r' % (
                result.stdout, result.stderr),
        )
        combined = result.stdout + result.stderr
        self.assertIn('Invalid inputfile', combined)

    def test_help_exits_zero(self):
        result = subprocess.run(
            [sys.executable, str(_FMMT_PY), '-h'],
            env=_fmmt_env(),
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
