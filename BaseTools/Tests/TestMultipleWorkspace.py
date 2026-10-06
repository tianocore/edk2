## @file
# Unit tests for workspace macro expansion.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

"""Regression tests for workspace macro expansion."""

import os
import tempfile
import unittest
from unittest import mock

from Common.MultipleWorkspace import MultipleWorkspace


class TestMultipleWorkspace(unittest.TestCase):
    """Check workspace resolution without changing compiler option formatting."""

    def setUp(self) -> None:
        """Create isolated roots and restore shared workspace state after each test."""
        temporary = tempfile.TemporaryDirectory(prefix="multiple workspace ")
        self.addCleanup(temporary.cleanup)
        self.workspace = os.path.join(temporary.name, "workspace")
        self.packages = [
            os.path.join(temporary.name, "first package"),
            os.path.join(temporary.name, "last package"),
        ]
        for directory in [self.workspace] + self.packages:
            os.mkdir(directory)
        self.addCleanup(setattr, MultipleWorkspace, "WORKSPACE", MultipleWorkspace.WORKSPACE)
        self.addCleanup(setattr, MultipleWorkspace, "PACKAGES_PATH", MultipleWorkspace.PACKAGES_PATH)
        MultipleWorkspace.setWs(self.workspace, os.pathsep.join(self.packages))

    def create_file(self, root: str, relative: str) -> str:
        """Create an empty file under a test root and return its path."""
        path = os.path.join(root, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w"):
            pass
        return path

    def test_workspace_takes_precedence(self) -> None:
        """Prefer an existing workspace file regardless of option quoting."""
        relative = "Config/Settings.txt"
        self.create_file(self.workspace, relative)
        self.create_file(self.packages[0], relative)
        for quote in ("", '"', "'"):
            with self.subTest(quote=quote):
                option = "--config=" + quote + "$(WORKSPACE)/" + relative + quote
                self.assertEqual(
                    option.replace("$(WORKSPACE)", self.workspace),
                    MultipleWorkspace.handleWsMacro(option),
                )

    def test_quoted_paths_with_spaces(self) -> None:
        """Resolve quoted paths with spaces while retaining their original quoting."""
        relative = "Config/Build Settings.txt"
        self.create_file(self.packages[0], relative)
        for option in (
            '"$(WORKSPACE)/Config/Build Settings.txt"',
            '--config="$(WORKSPACE)/Config/Build Settings.txt"',
            "--config='$(WORKSPACE)/Config/Build Settings.txt'",
            '"--config=$(WORKSPACE)/Config/Build Settings.txt"',
            "'--config=$(WORKSPACE)/Config/Build Settings.txt'",
            '--config="$(WORKSPACE)"/Config/"Build Settings.txt"',
        ):
            with self.subTest(option=option):
                self.assertEqual(
                    option.replace("$(WORKSPACE)", self.packages[0]),
                    MultipleWorkspace.handleWsMacro(option),
                )

    def test_opposite_quote_inside_quoted_path(self) -> None:
        """Preserve a literal apostrophe inside a double-quoted filename."""
        relative = "Config/Developer's Settings.txt"
        self.create_file(self.packages[0], relative)
        option = '--config="$(WORKSPACE)/' + relative + '"'
        self.assertEqual(
            option.replace("$(WORKSPACE)", self.packages[0]),
            MultipleWorkspace.handleWsMacro(option),
        )

    def test_first_existing_package_path_wins(self) -> None:
        """Prefer the first package root containing an unquoted path."""
        relative = "Include/Header.h"
        for directory in self.packages:
            self.create_file(directory, relative)
        option = "-I$(WORKSPACE)/" + relative
        self.assertEqual(
            option.replace("$(WORKSPACE)", self.packages[0]),
            MultipleWorkspace.handleWsMacro(option),
        )

    def test_later_package_path_is_searched(self) -> None:
        """Search later package roots when earlier roots lack the file."""
        relative = "Include/Header.h"
        self.create_file(self.packages[1], relative)
        option = '-I"$(WORKSPACE)/' + relative + '"'
        self.assertEqual(
            option.replace("$(WORKSPACE)", self.packages[1]),
            MultipleWorkspace.handleWsMacro(option),
        )

    def test_missing_path_stays_workspace_relative(self) -> None:
        """Keep missing paths rooted in the workspace rather than a package root."""
        for option in ("-I$(WORKSPACE)/Missing", '-I"$(WORKSPACE)/Missing Directory"'):
            with self.subTest(option=option):
                self.assertEqual(
                    option.replace("$(WORKSPACE)", self.workspace),
                    MultipleWorkspace.handleWsMacro(option),
                )

    def test_no_package_paths(self) -> None:
        """Expand the workspace macro when no package roots are configured."""
        MultipleWorkspace.setWs(self.workspace)
        option = '"$(WORKSPACE)/Missing Directory"'
        self.assertEqual(
            option.replace("$(WORKSPACE)", self.workspace),
            MultipleWorkspace.handleWsMacro(option),
        )

    def test_no_macro_is_unchanged(self) -> None:
        """Leave strings without a workspace macro unchanged."""
        for option in ("", '  -O2\t-DNAME="a b"  ', "$(OTHER)/Include"):
            with self.subTest(option=option):
                self.assertEqual(option, MultipleWorkspace.handleWsMacro(option))

    def test_multiple_options_preserve_whitespace_and_quotes(self) -> None:
        """Resolve each option independently without changing surrounding whitespace."""
        self.create_file(self.workspace, "Config/Settings.txt")
        self.create_file(self.packages[0], "Include/Header.h")
        option = '  -O2\t--config="$(WORKSPACE)/Config/Settings.txt"  -I$(WORKSPACE)/Include/Header.h\n'
        expected = (
            '  -O2\t--config="'
            + self.workspace
            + '/Config/Settings.txt"  -I'
            + self.packages[0]
            + "/Include/Header.h\n"
        )
        self.assertEqual(expected, MultipleWorkspace.handleWsMacro(option))

    def test_windows_backslashes_are_preserved(self) -> None:
        """Check Windows-style paths without treating backslashes as escapes."""
        MultipleWorkspace.WORKSPACE = r"C:\Main Workspace"
        MultipleWorkspace.PACKAGES_PATH = [r"D:\First Package", r"E:\Last Package"]
        option = r'/I"$(WORKSPACE)\Include Files\Header.h"'
        path = r"D:\First Package\Include Files\Header.h"
        with mock.patch("Common.LongFilePathOs.path.exists", side_effect=lambda candidate: candidate == path) as exists:
            self.assertEqual(
                option.replace("$(WORKSPACE)", MultipleWorkspace.PACKAGES_PATH[0]),
                MultipleWorkspace.handleWsMacro(option),
            )
            self.assertEqual(
                [
                    mock.call(r"C:\Main Workspace\Include Files\Header.h"),
                    mock.call(path),
                ],
                exists.call_args_list,
            )


def TheTestSuite() -> unittest.TestSuite:
    """Return the regression suite for the BaseTools test runner."""
    return unittest.defaultTestLoader.loadTestsFromTestCase(TestMultipleWorkspace)


if __name__ == "__main__":
    unittest.main()
