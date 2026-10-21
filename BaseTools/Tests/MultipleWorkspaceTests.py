## @file
# Unit tests for package path resolution across workspaces.
#
# Copyright (c) 2026, NextSilicon. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import tempfile
import unittest

import TestTools

from Common.Misc import PathClass
from Common.MultipleWorkspace import MultipleWorkspace


class MultipleWorkspaceTests(unittest.TestCase):
    def setUp(self):
        self._saved_workspace = MultipleWorkspace.WORKSPACE
        self._saved_packages_path = MultipleWorkspace.PACKAGES_PATH
        self._temporary_directory = tempfile.TemporaryDirectory()
        self.workspace = os.path.join(self._temporary_directory.name, "edk2")
        os.mkdir(self.workspace)

    def tearDown(self):
        MultipleWorkspace.WORKSPACE = self._saved_workspace
        MultipleWorkspace.PACKAGES_PATH = self._saved_packages_path
        self._temporary_directory.cleanup()

    def _check_module_path(self, package_root, workspace=None):
        if workspace is None:
            workspace = self.workspace
        module = os.path.join("TestPkg", "Driver", "Driver.inf")
        module_path = os.path.join(package_root, module)
        os.makedirs(os.path.dirname(module_path))
        with open(module_path, "w"):
            pass

        MultipleWorkspace.setWs(workspace, package_root)
        path = PathClass(module, workspace)
        self.assertEqual(path.Root, package_root)
        self.assertEqual(path.File, module)
        self.assertEqual(path.Path, module_path)
        self.assertEqual(
            MultipleWorkspace.join(workspace, path.SubDir),
            os.path.dirname(module_path),
        )

    def testSiblingWithWorkspacePrefix(self):
        package_root = self.workspace + "-upstream-fadt"
        self.assertEqual(
            MultipleWorkspace.convertPackagePath(self.workspace, package_root),
            package_root,
        )
        self._check_module_path(package_root)

    def testSiblingModulePathWithWorkspacePrefix(self):
        self._check_module_path(self.workspace + "-upstream-fadt")

    def testSiblingPackage(self):
        self._check_module_path(os.path.join(self._temporary_directory.name, "packages"))

    def testNestedPackage(self):
        self._check_module_path(os.path.join(self.workspace, "packages"))

    def testWorkspaceAsPackageRoot(self):
        self._check_module_path(self.workspace)

    def testWorkspaceWithTrailingSeparator(self):
        self._check_module_path(
            self.workspace + "-upstream-fadt", os.path.join(self.workspace, "")
        )

    def testFilesystemRootWorkspace(self):
        root = os.path.abspath(os.sep)
        self.assertEqual(
            MultipleWorkspace.convertPackagePath(root, self.workspace),
            self.workspace,
        )


TheTestSuite = TestTools.MakeTheTestSuite(locals())

if __name__ == '__main__':
    unittest.main()
