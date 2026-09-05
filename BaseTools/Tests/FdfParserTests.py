## @file
# Unit tests for FDF parser macro scope handling.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import tempfile
import unittest

from Common import GlobalData
from Common.MultipleWorkspace import MultipleWorkspace
import GenFds.FdfParser as FdfParserModule
from GenFds.FdfParser import FdfParser, Warning
from GenFds.GenFdsGlobalVariable import GenFdsGlobalVariable


class FdfParserMacroScopeTests(unittest.TestCase):
    def setUp(self):
        self._saved_global_defines = GlobalData.gGlobalDefines
        self._saved_platform_defines = GlobalData.gPlatformDefines
        self._saved_command_line_defines = GlobalData.gCommandLineDefines
        self._saved_platform_pcds = GlobalData.gPlatformPcds
        self._saved_active_platform = GlobalData.gActivePlatform
        self._saved_fdf_parser = GlobalData.gFdfParser
        self._saved_global_workspace = GlobalData.gWorkspace
        self._saved_workspace = MultipleWorkspace.WORKSPACE
        self._saved_packages_path = MultipleWorkspace.PACKAGES_PATH
        self._saved_workspace_dir = GenFdsGlobalVariable.WorkSpaceDir
        self._saved_genfds_active_platform = GenFdsGlobalVariable.ActivePlatform
        self._saved_include_file_list = FdfParserModule.AllIncludeFileList[:]

        self._temporary_directory = tempfile.TemporaryDirectory()
        GlobalData.gGlobalDefines = {}
        GlobalData.gPlatformDefines = {}
        GlobalData.gCommandLineDefines = {}
        GlobalData.gPlatformPcds = {}
        GlobalData.gActivePlatform = None
        GlobalData.gWorkspace = self._temporary_directory.name
        MultipleWorkspace.setWs(self._temporary_directory.name)
        GenFdsGlobalVariable.WorkSpaceDir = self._temporary_directory.name
        GenFdsGlobalVariable.ActivePlatform = None
        FdfParserModule.AllIncludeFileList.clear()

    def tearDown(self):
        GlobalData.gGlobalDefines = self._saved_global_defines
        GlobalData.gPlatformDefines = self._saved_platform_defines
        GlobalData.gCommandLineDefines = self._saved_command_line_defines
        GlobalData.gPlatformPcds = self._saved_platform_pcds
        GlobalData.gActivePlatform = self._saved_active_platform
        GlobalData.gFdfParser = self._saved_fdf_parser
        GlobalData.gWorkspace = self._saved_global_workspace
        MultipleWorkspace.WORKSPACE = self._saved_workspace
        MultipleWorkspace.PACKAGES_PATH = self._saved_packages_path
        GenFdsGlobalVariable.WorkSpaceDir = self._saved_workspace_dir
        GenFdsGlobalVariable.ActivePlatform = self._saved_genfds_active_platform
        FdfParserModule.AllIncludeFileList[:] = self._saved_include_file_list
        self._temporary_directory.cleanup()

    def _write_file(self, relative_path, content):
        path = os.path.join(self._temporary_directory.name, relative_path)
        directory = os.path.dirname(path)
        if directory:
            os.makedirs(directory, exist_ok=True)
        with open(path, "w", newline="\n") as output_file:
            output_file.write(content)
        return path

    def _preprocess(self, fdf_content, included_files):
        for relative_path, content in included_files.items():
            self._write_file(relative_path, content)

        fdf_path = self._write_file("Test.fdf", fdf_content)
        parser = FdfParser(fdf_path)
        parser.Preprocess()
        return "".join(parser.Profile.FileLinesList)

    def test_local_macro_expands_include_in_same_section(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LOCAL = SameSection\n"
            "!include $(LOCAL).inc\n",
            {"SameSection.inc": "SAME_SECTION_MARKER\n"},
        )

        self.assertIn("SAME_SECTION_MARKER", result)

    def test_local_macro_is_undefined_in_later_section(self):
        with self.assertRaisesRegex(Warning, "The Macro LOCAL is not defined"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE LOCAL = Leaked\n"
                "\n"
                "[FV.SECOND]\n"
                "!include $(LOCAL).inc\n",
                {"Leaked.inc": "LEAKED_INCLUDE_MARKER\n"},
            )

    def test_defines_macro_expands_in_multiple_sections(self):
        result = self._preprocess(
            "[Defines]\n"
            "DEFINE SHARED = Shared\n"
            "\n"
            "[FV.FIRST]\n"
            "!include $(SHARED).inc\n"
            "\n"
            "[FV.SECOND]\n"
            "!include $(SHARED).inc\n",
            {"Shared.inc": "SHARED_MARKER\n"},
        )

        self.assertEqual(2, result.count("SHARED_MARKER"))

    def test_local_macros_resolve_independently_by_section(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LOCAL = First\n"
            "!include $(LOCAL).inc\n"
            "\n"
            "[FV.SECOND]\n"
            "DEFINE LOCAL = Second\n"
            "!include $(LOCAL).inc\n",
            {
                "First.inc": "FIRST_MARKER\n",
                "Second.inc": "SECOND_MARKER\n",
            },
        )

        self.assertEqual(1, result.count("FIRST_MARKER"))
        self.assertEqual(1, result.count("SECOND_MARKER"))

    def test_nested_include_uses_active_section_macro(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LOCAL = Nested\n"
            "!include Outer.inc\n",
            {
                "Outer.inc": "!include $(LOCAL).inc\n",
                "Nested.inc": "NESTED_MARKER\n",
            },
        )

        self.assertIn("NESTED_MARKER", result)

    def test_command_line_macro_overrides_local_macro(self):
        GlobalData.gCommandLineDefines = {"LOCAL": "CommandLine"}

        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LOCAL = Local\n"
            "!include $(LOCAL).inc\n",
            {
                "CommandLine.inc": "COMMAND_LINE_MARKER\n",
                "Local.inc": "LOCAL_MARKER\n",
            },
        )

        self.assertIn("COMMAND_LINE_MARKER", result)
        self.assertNotIn("LOCAL_MARKER", result)

    def test_define_before_first_section_remains_invalid(self):
        with self.assertRaisesRegex(
            Warning,
            "macro cannot be defined in Rule section or out of section",
        ):
            self._preprocess(
                "DEFINE OUTSIDE = Value\n"
                "[FV.FIRST]\n",
                {},
            )

    def test_quoted_defines_header_does_not_change_macro_scope(self):
        with self.assertRaisesRegex(Warning, "The Macro LOCAL is not defined"):
            self._preprocess(
                "[FV.FIRST]\n"
                "UI = \"label [Defines] text\"\n"
                "DEFINE LOCAL = Leaked\n"
                "[FV.SECOND]\n"
                "!include $(LOCAL).inc\n",
                {"Leaked.inc": "LEAKED_MARKER\n"},
            )

    def test_quoted_fv_header_does_not_change_macro_scope(self):
        with self.assertRaisesRegex(Warning, "The Macro LOCAL is not defined"):
            self._preprocess(
                "[FV.FIRST]\n"
                "UI = \"label [FV.SECOND] text\"\n"
                "DEFINE LOCAL = Leaked\n"
                "[FV.SECOND]\n"
                "!include $(LOCAL).inc\n",
                {"Leaked.inc": "LEAKED_MARKER\n"},
            )

    def test_section_header_must_close_on_same_line(self):
        with self.assertRaisesRegex(Warning, r"expected '\]'"):
            self._preprocess(
                "[FV.FIRST\n"
                "[FV.SECOND]\n",
                {},
            )

    def test_inactive_malformed_section_header_does_not_change_macro_scope(self):
        GlobalData.gCommandLineDefines = {"FEATURE": "FALSE"}

        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LOCAL = Active\n"
            "!if $(FEATURE) == TRUE\n"
            "[FV.INACTIVE\n"
            "!endif\n"
            "!include $(LOCAL).inc\n",
            {"Active.inc": "ACTIVE_MARKER\n"},
        )

        self.assertIn("ACTIVE_MARKER", result)

    def test_quoted_conditional_does_not_change_active_state(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "UI = \"label !if FALSE text\"\n"
            "DEFINE LOCAL = Active\n"
            "!include $(LOCAL).inc\n",
            {"Active.inc": "ACTIVE_MARKER\n"},
        )

        self.assertIn("ACTIVE_MARKER", result)

    def test_nested_include_comments_do_not_change_macro_scope(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LOCAL = Active\n"
            "!include ScopeComment.inc\n"
            "!include $(LOCAL).inc\n",
            {
                "ScopeComment.inc": (
                    "/*\n"
                    "[Defines]\n"
                    "DEFINE LOCAL = Comment\n"
                    "*/\n"
                ),
                "Active.inc": "ACTIVE_MARKER\n",
                "Comment.inc": "COMMENT_MARKER\n",
            },
        )

        self.assertIn("ACTIVE_MARKER", result)
        self.assertNotIn("COMMENT_MARKER", result)


    def test_inactive_include_is_still_expanded(self):
        GlobalData.gCommandLineDefines = {"FEATURE": "FALSE"}

        with self.assertRaisesRegex(Warning, "include file does not exist"):
            self._preprocess(
                "[FV.FIRST]\n"
                "!if $(FEATURE) == TRUE\n"
                "!include Missing.inc\n"
                "!endif\n",
                {},
            )

    def test_elseif_selects_local_macro(self):
        GlobalData.gCommandLineDefines = {
            "FIRST_ENABLED": "FALSE",
            "SECOND_ENABLED": "TRUE",
        }

        result = self._preprocess(
            "[FV.FIRST]\n"
            "!if $(FIRST_ENABLED) == TRUE\n"
            "DEFINE LOCAL = First\n"
            "!elseif $(SECOND_ENABLED) == TRUE\n"
            "DEFINE LOCAL = Second\n"
            "!else\n"
            "DEFINE LOCAL = Else\n"
            "!endif\n"
            "!include $(LOCAL).inc\n",
            {
                "First.inc": "FIRST_MARKER\n",
                "Second.inc": "SECOND_MARKER\n",
                "Else.inc": "ELSE_MARKER\n",
            },
        )

        self.assertIn("SECOND_MARKER", result)
        self.assertNotIn("FIRST_MARKER", result)
        self.assertNotIn("ELSE_MARKER", result)

    def test_else_selects_local_macro(self):
        GlobalData.gCommandLineDefines = {
            "FIRST_ENABLED": "FALSE",
            "SECOND_ENABLED": "FALSE",
        }

        result = self._preprocess(
            "[FV.FIRST]\n"
            "!if $(FIRST_ENABLED) == TRUE\n"
            "DEFINE LOCAL = First\n"
            "!elseif $(SECOND_ENABLED) == TRUE\n"
            "DEFINE LOCAL = Second\n"
            "!else\n"
            "DEFINE LOCAL = Else\n"
            "!endif\n"
            "!include $(LOCAL).inc\n",
            {
                "First.inc": "FIRST_MARKER\n",
                "Second.inc": "SECOND_MARKER\n",
                "Else.inc": "ELSE_MARKER\n",
            },
        )

        self.assertIn("ELSE_MARKER", result)
        self.assertNotIn("FIRST_MARKER", result)
        self.assertNotIn("SECOND_MARKER", result)

    def test_ifdef_and_ifndef_select_local_macros(self):
        result = self._preprocess(
            "[Defines]\n"
            "DEFINE PRESENT = Defined\n"
            "[FV.FIRST]\n"
            "!ifdef PRESENT\n"
            "DEFINE IFDEF_LOCAL = Ifdef\n"
            "!endif\n"
            "!ifndef MISSING\n"
            "DEFINE IFNDEF_LOCAL = Ifndef\n"
            "!endif\n"
            "!include $(IFDEF_LOCAL).inc\n"
            "!include $(IFNDEF_LOCAL).inc\n",
            {
                "Ifdef.inc": "IFDEF_MARKER\n",
                "Ifndef.inc": "IFNDEF_MARKER\n",
            },
        )

        self.assertIn("IFDEF_MARKER", result)
        self.assertIn("IFNDEF_MARKER", result)

    def test_macro_cannot_generate_conditional_directive(self):
        GlobalData.gCommandLineDefines = {
            "FEATURE": "TRUE",
            "SWITCH": "!else",
        }

        with self.assertRaisesRegex(
            Warning,
            "macro expansion cannot generate a section header or preprocessing directive",
        ):
            self._preprocess(
                "[FV.FIRST]\n"
                "!if $(FEATURE) == TRUE\n"
                "$(SWITCH)\n"
                "DEFINE PICK = Evil\n"
                "!endif\n"
                "!include $(PICK).inc\n",
                {"Evil.inc": "EVIL_MARKER\n"},
            )

    def test_empty_macro_line_does_not_raise_index_error(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE EMPTY =\n"
            "$(EMPTY)\n",
            {},
        )

        self.assertNotIn("$(EMPTY)", result)


    def test_inactive_define_resolves_include_without_leaking(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "!if FALSE\n"
            "DEFINE INC = Inactive\n"
            "!include $(INC).inc\n"
            "!endif\n",
            {"Inactive.inc": "INACTIVE_MARKER\n"},
        )

        self.assertNotIn("INACTIVE_MARKER", result)
        self.assertNotIn("INC", result)


    def test_inactive_define_does_not_escape_branch(self):
        with self.assertRaisesRegex(Warning, "The Macro INC is not defined"):
            self._preprocess(
                "[FV.FIRST]\n"
                "!if FALSE\n"
                "DEFINE INC = Inactive\n"
                "!include $(INC).inc\n"
                "!endif\n"
                "!include $(INC).inc\n",
                {"Inactive.inc": "INACTIVE_MARKER\n"},
            )

    def test_set_updates_condition_before_include_expansion(self):
        GlobalData.gPlatformPcds = {"gTest.PcdGate": "TRUE"}

        result = self._preprocess(
            "[FV.FIRST]\n"
            "SET gTest.PcdGate = FALSE\n"
            "!if $(gTest.PcdGate) == TRUE\n"
            "DEFINE INC = First\n"
            "!else\n"
            "DEFINE INC = Second\n"
            "!endif\n"
            "!include $(INC).inc\n",
            {
                "First.inc": "FIRST_MARKER\n",
                "Second.inc": "SECOND_MARKER\n",
            },
        )

        self.assertIn("SECOND_MARKER", result)
        self.assertNotIn("FIRST_MARKER", result)

    def test_cyclic_macro_in_section_header_is_rejected(self):
        with self.assertRaisesRegex(Warning, "macro cannot be used in section header"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE LOOP = $(LOOP)\n"
                "[$(LOOP)]\n",
                {},
            )

    def test_cyclic_macro_in_include_path_is_rejected(self):
        with self.assertRaisesRegex(Warning, "Cyclic macro expansion"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE LOOP = $(LOOP)\n"
                "!include $(LOOP).inc\n",
                {},
            )

    def test_growing_cyclic_macro_in_include_path_is_rejected(self):
        with self.assertRaisesRegex(Warning, "Cyclic macro expansion"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE LOOP = $(LOOP)x\n"
                "!include $(LOOP).inc\n",
                {},
            )

    def test_growing_cyclic_macro_on_line_is_rejected(self):
        with self.assertRaisesRegex(Warning, "Cyclic macro expansion"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE LOOP = $(LOOP)x\n"
                "$(LOOP)\n",
                {},
            )

    def test_growing_cyclic_macro_after_prefix_is_rejected(self):
        with self.assertRaisesRegex(Warning, "Cyclic macro expansion"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE LOOP = $(LOOP)x\n"
                "VALUE = $(LOOP)\n",
                {},
            )
    def test_mutually_cyclic_macros_in_include_path_are_rejected(self):
        with self.assertRaisesRegex(Warning, "Cyclic macro expansion"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE FIRST = $(SECOND)x\n"
                "DEFINE SECOND = $(FIRST)y\n"
                "!include $(FIRST).inc\n",
                {},
            )
    def test_repeated_and_nested_macros_expand_without_cycle(self):
        result = self._preprocess(
            "[FV.FIRST]\n"
            "DEFINE LEAF = Part\n"
            "DEFINE NAME = $(LEAF)$(LEAF)\n"
            "!include $(NAME).inc\n",
            {"PartPart.inc": "REPEATED_NESTED_MARKER\n"},
        )

        self.assertIn("REPEATED_NESTED_MARKER", result)
    def test_deep_acyclic_macro_chain_expands_without_recursion_error(self):
        chain_length = 1100
        lines = ["[FV.FIRST]\n"]
        for index in range(chain_length):
            value = "$(M%d)" % (index + 1) if index < chain_length - 1 else "Final"
            lines.append("DEFINE M%d = %s\n" % (index, value))
        lines.append("!include $(M0).inc\n")

        result = self._preprocess(
            "".join(lines),
            {"Final.inc": "DEEP_CHAIN_MARKER\n"},
        )

        self.assertIn("DEEP_CHAIN_MARKER", result)
    def test_nested_selected_macros_remain_in_inactive_parent_branch(self):
        self._preprocess(
            "[FV.FIRST]\n"
            "!if FALSE\n"
            "!if TRUE\n"
            "DEFINE INC = Nested\n"
            "!endif\n"
            "!include $(INC).inc\n"
            "!endif\n",
            {"Nested.inc": "NESTED_MARKER\n"},
        )

    def test_macro_generated_error_directive_is_rejected(self):
        with self.assertRaisesRegex(Warning, "macro expansion cannot generate"):
            self._preprocess(
                "[FV.FIRST]\n"
                "DEFINE DIRECTIVE = !error\n"
                "$(DIRECTIVE) generated message\n",
                {},
            )
def TheTestSuite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
        FdfParserMacroScopeTests
    )


if __name__ == "__main__":
    unittest.main()
