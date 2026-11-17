## @file
# Tests for FMMT -c / FmmtConf.ini path handling.
#
# Copyright (c) 2026, Zhaoqi Xu <lzy00419@outlook.com>. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

_BASETOOLS = Path(__file__).resolve().parent.parent
_PYTHON_SRC = _BASETOOLS / 'Source' / 'Python'
_FMMT_DIR = _PYTHON_SRC / 'FMMT'

for _p in (str(_FMMT_DIR), str(_PYTHON_SRC)):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from FMMT.FMMT import FMMT
from core.GuidTools import GUIDTools


class TestFmmtConfig(unittest.TestCase):
    def setUp(self):
        self._old_conf = os.environ.get('FmmtConfPath')
        self._old_path = os.environ.get('PATH')
        os.environ.pop('FmmtConfPath', None)
        self.tmpdir = tempfile.mkdtemp()

    def tearDown(self):
        if self._old_conf is None:
            os.environ.pop('FmmtConfPath', None)
        else:
            os.environ['FmmtConfPath'] = self._old_conf
        if self._old_path is None:
            os.environ.pop('PATH', None)
        else:
            os.environ['PATH'] = self._old_path
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_set_dest_path_preserves_config_file(self):
        cfg = os.path.join(self.tmpdir, 'CustomFmmtConf.ini')
        with open(cfg, 'w') as f:
            f.write('# empty\n')
        dummy_input = os.path.join(self.tmpdir, 'input.fd')
        with open(dummy_input, 'wb') as f:
            f.write(b'\0')

        fmmt = FMMT()
        fmmt.SetConfigFilePath(cfg)
        fmmt.SetDestPath(dummy_input)
        self.assertEqual(os.environ.get('FmmtConfPath'), os.path.abspath(cfg))

    def test_set_config_file_uses_file_path_not_directory(self):
        cfg = os.path.join(self.tmpdir, 'CustomFmmtConf.ini')
        with open(cfg, 'w') as f:
            f.write(
                'a31280ad-481e-41b6-95e8-127f4c984779 TIANO UniqueTestToolXYZ\n'
            )
        os.environ['FmmtConfPath'] = os.path.abspath(cfg)

        tools = GUIDTools()
        tools.SetConfigFile()
        self.assertEqual(tools.tooldef_file, os.path.abspath(cfg))

        tools.LoadingTools()
        commands = [item.command for item in tools.tooldef.values()]
        self.assertIn('UniqueTestToolXYZ', commands)

    def test_set_config_file_without_env_does_not_raise(self):
        os.environ.pop('FmmtConfPath', None)
        tools = GUIDTools()
        tools.SetConfigFile()


if __name__ == '__main__':
    unittest.main()
