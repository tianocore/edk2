## @file
# Unit tests for DSC parser component scope handling.
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
from CommonDataClass.DataClass import MODEL_EFI_LIBRARY_CLASS
from CommonDataClass.DataClass import MODEL_FILE_DSC
from CommonDataClass.DataClass import MODEL_META_DATA_BUILD_OPTION
from CommonDataClass.DataClass import MODEL_META_DATA_COMPONENT
from CommonDataClass.DataClass import MODEL_PCD_DYNAMIC_HII
from CommonDataClass.DataClass import MODEL_PCD_FIXED_AT_BUILD
from Common.Misc import PathClass
from Workspace.MetaFileParser import DscParser
from Workspace.MetaFileParser import MetaFileParser
from Workspace.MetaFileTable import MetaFileStorage
from Workspace.WorkspaceDatabase import WorkspaceDatabase


class DscParserComponentScopeTests(unittest.TestCase):
    def setUp(self):
        self._saved_workspace = GlobalData.gWorkspace
        self._saved_global_defines = GlobalData.gGlobalDefines
        self._saved_platform_defines = GlobalData.gPlatformDefines
        self._saved_command_line_defines = GlobalData.gCommandLineDefines
        self._saved_edk_global = GlobalData.gEdkGlobal
        self._saved_platform_pcds = GlobalData.gPlatformPcds
        self._saved_platform_other_pcds = GlobalData.gPlatformOtherPcds
        self._saved_meta_files = MetaFileParser.MetaFiles.copy()
        self._saved_storage_cache = MetaFileStorage._ObjectCache.copy()
        self._saved_included_files = DscParser.IncludedFiles.copy()

        self._temporary_directory = tempfile.TemporaryDirectory()
        workspace = self._temporary_directory.name
        GlobalData.gWorkspace = workspace
        GlobalData.gGlobalDefines = {"WORKSPACE": workspace}
        GlobalData.gPlatformDefines = {}
        GlobalData.gCommandLineDefines = {}
        GlobalData.gEdkGlobal = {}
        GlobalData.gPlatformPcds = {}
        GlobalData.gPlatformOtherPcds = {}

    def _write_file(self, relative_path, content):
        path = os.path.join(self._temporary_directory.name, relative_path)
        directory = os.path.dirname(path)
        if directory:
            os.makedirs(directory, exist_ok=True)
        with open(path, "w", newline="\n") as output_file:
            output_file.write(content)
        return path

    def _create_parser(
        self,
        platform_name,
        platform_content,
        included_files,
        active_architecture="IA32",
    ):
        for relative_path, content in included_files.items():
            self._write_file(relative_path, content)

        platform_path = self._write_file(platform_name, platform_content)
        path = PathClass(platform_path)
        database = WorkspaceDatabase()
        table = MetaFileStorage(database, path, MODEL_FILE_DSC)
        return DscParser(path, MODEL_FILE_DSC, active_architecture, table)

    @staticmethod
    def _platform_content(
        include_path,
        define_architecture=True,
        macro_architecture="IA32",
    ):
        architecture_definition = (
            f"  DEFINE PEI_ARCH = {macro_architecture}\n"
            if define_architecture
            else ""
        )
        return (
            "[Defines]\n"
            "  PLATFORM_NAME = ParserTest\n"
            "  PLATFORM_GUID = 11111111-2222-3333-4444-555555555555\n"
            "  PLATFORM_VERSION = 0.1\n"
            "  DSC_SPECIFICATION = 0x00010005\n"
            "  OUTPUT_DIRECTORY = Build/ParserTest\n"
            "  SUPPORTED_ARCHITECTURES = IA32|X64\n"
            "  BUILD_TARGETS = DEBUG\n"
            "  SKUID_IDENTIFIER = DEFAULT\n"
            f"{architecture_definition}"
            "\n"
            f"!include {include_path}\n"
        )

    @staticmethod
    def _component_content(section):
        return (
            f"[{section}]\n"
            "  TestPkg/TestPeim.inf {\n"
            "    <LibraryClasses>\n"
            "      TestLib|TestPkg/TestLib.inf\n"
            "    <PcdsFixedAtBuild>\n"
            "      gTestPkgTokenSpaceGuid.PcdTest|TRUE\n"
            "  }\n"
        )

    def _assert_component_private_records(self, parser, architecture):
        components = parser[MODEL_META_DATA_COMPONENT, architecture]
        self.assertEqual(1, len(components))
        self.assertEqual(architecture, components[0][3])
        component_id = components[0][6]

        library_classes = parser[
            MODEL_EFI_LIBRARY_CLASS, architecture, None, component_id
        ]
        self.assertEqual(1, len(library_classes))
        self.assertEqual(architecture, library_classes[0][3])

        fixed_pcds = parser[
            MODEL_PCD_FIXED_AT_BUILD, architecture, None, component_id
        ]
        self.assertEqual(1, len(fixed_pcds))
        self.assertEqual(architecture, fixed_pcds[0][3])

        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, architecture],
        )
        self.assertEqual(
            [],
            parser[MODEL_PCD_FIXED_AT_BUILD, architecture],
        )

    def tearDown(self):
        MetaFileParser.MetaFiles.clear()
        MetaFileParser.MetaFiles.update(self._saved_meta_files)
        MetaFileStorage._ObjectCache.clear()
        MetaFileStorage._ObjectCache.update(self._saved_storage_cache)
        DscParser.IncludedFiles.clear()
        DscParser.IncludedFiles.update(self._saved_included_files)
        GlobalData.gWorkspace = self._saved_workspace
        GlobalData.gGlobalDefines = self._saved_global_defines
        GlobalData.gPlatformDefines = self._saved_platform_defines
        GlobalData.gCommandLineDefines = self._saved_command_line_defines
        GlobalData.gEdkGlobal = self._saved_edk_global
        GlobalData.gPlatformPcds = self._saved_platform_pcds
        GlobalData.gPlatformOtherPcds = self._saved_platform_other_pcds
        self._temporary_directory.cleanup()

    def test_private_records_use_expanded_component_architecture(self):
        parser = self._create_parser(
            "Platform.dsc",
            self._platform_content("Child.inc.dsc"),
            {
                "Child.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                )
            },
        )
        self._assert_component_private_records(parser, "IA32")

    def test_macro_architecture_is_independent_of_active_architecture(self):
        parser = self._create_parser(
            "DifferentArch.dsc",
            self._platform_content(
                "DifferentArch.inc.dsc",
                macro_architecture="X64",
            ),
            {
                "DifferentArch.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                )
            },
            active_architecture="IA32",
        )
        self._assert_component_private_records(parser, "X64")

        self.assertEqual(
            [],
            parser[MODEL_META_DATA_COMPONENT, "IA32"],
        )

    def test_aarch64_private_records_use_expanded_component_architecture(self):
        platform_content = self._platform_content(
            "Aarch64.inc.dsc",
            macro_architecture="AARCH64",
        ).replace(
            "SUPPORTED_ARCHITECTURES = IA32|X64",
            "SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64",
        )
        parser = self._create_parser(
            "Aarch64.dsc",
            platform_content,
            {
                "Aarch64.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                )
            },
            active_architecture="AARCH64",
        )
        self._assert_component_private_records(parser, "AARCH64")

    def test_literal_component_architecture_is_unchanged(self):
        parser = self._create_parser(
            "Literal.dsc",
            self._platform_content("Literal.inc.dsc", False),
            {
                "Literal.inc.dsc": self._component_content(
                    "Components.IA32"
                )
            },
        )
        self._assert_component_private_records(parser, "IA32")

    def test_unresolved_component_architecture_macro_is_unchanged(self):
        parser = self._create_parser(
            "Unresolved.dsc",
            self._platform_content("Unresolved.inc.dsc", False),
            {
                "Unresolved.inc.dsc": self._component_content(
                    "Components.$(UNKNOWN_ARCH)"
                )
            },
        )
        self._assert_component_private_records(parser, "$(UNKNOWN_ARCH)")

    def test_nested_include_private_records_use_expanded_architecture(self):
        parser = self._create_parser(
            "Nested.dsc",
            self._platform_content("Intermediate.inc.dsc"),
            {
                "Intermediate.inc.dsc": "!include Leaf.inc.dsc\n",
                "Leaf.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                ),
            },
        )
        self._assert_component_private_records(parser, "IA32")

    # An !include'd file is equivalent to pasting its content into the DSC
    # file (DSC spec 2.2.5), and macros defined in [Defines] are global to
    # included files (DSC spec 2.2.6). A top-level section header in an
    # included file is therefore expanded even though it is not owned by a
    # Component.
    def test_top_level_include_scope_is_expanded(self):
        parser = self._create_parser(
            "TopLevelInclude.dsc",
            self._platform_content("TopLevelInclude.inc.dsc"),
            {
                "TopLevelInclude.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH)]\n"
                    "  TestLib|TestPkg/TestLib.inf\n"
                )
            },
        )

        library_classes = parser[
            MODEL_EFI_LIBRARY_CLASS,
            "IA32",
            None,
            -1,
        ]
        self.assertEqual(1, len(library_classes))
        self.assertEqual("IA32", library_classes[0][3])
        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, "$(PEI_ARCH)"],
        )

    def test_top_level_include_scope_uses_macro_from_other_include(self):
        platform_content = self._platform_content(
            "Library.inc.dsc",
            define_architecture=False,
        ).replace(
            "!include Library.inc.dsc\n",
            "!include Defines.inc.dsc\n!include Library.inc.dsc\n",
        )
        parser = self._create_parser(
            "OtherInclude.dsc",
            platform_content,
            {
                "Defines.inc.dsc": "  DEFINE PEI_ARCH = IA32\n",
                "Library.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH)]\n"
                    "  TestLib|TestPkg/TestLib.inf\n"
                ),
            },
        )

        library_classes = parser[MODEL_EFI_LIBRARY_CLASS, "IA32"]
        self.assertEqual(1, len(library_classes))
        self.assertEqual("IA32", library_classes[0][3])

    def test_module_type_macro_is_expanded(self):
        platform_content = self._platform_content(
            "ModuleType.inc.dsc",
        ).replace(
            "!include ModuleType.inc.dsc\n",
            (
                "  DEFINE TEST_SEC = sec\n"
                "\n"
                "[LibraryClasses.common.$(TEST_SEC)]\n"
                "  MainLib|TestPkg/MainLib.inf\n"
                "\n"
                "!include ModuleType.inc.dsc\n"
            ),
        )
        parser = self._create_parser(
            "ModuleType.dsc",
            platform_content,
            {
                "ModuleType.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH).$(TEST_SEC)]\n"
                    "  IncludedLib|TestPkg/IncludedLib.inf\n"
                ),
            },
        )

        library_classes = parser[MODEL_EFI_LIBRARY_CLASS, "IA32"]
        self.assertEqual(
            [
                ("MainLib", "COMMON", "SEC"),
                ("IncludedLib", "IA32", "SEC"),
            ],
            [(item[0], item[3], item[4]) for item in library_classes],
        )

    def test_build_option_module_type_macro_is_expanded(self):
        parser = self._create_parser(
            "BuildOption.dsc",
            self._platform_content("BuildOption.inc.dsc").replace(
                "  DEFINE PEI_ARCH = IA32\n",
                "  DEFINE PEI_ARCH = IA32\n  DEFINE TEST_SEC = SEC\n",
            ),
            {
                "BuildOption.inc.dsc": (
                    "[BuildOptions.$(PEI_ARCH).EDKII.$(TEST_SEC)]\n"
                    "  GCC:*_*_*_CC_FLAGS = -DTEST\n"
                ),
            },
        )

        build_options = parser[MODEL_META_DATA_BUILD_OPTION, "IA32"]
        self.assertEqual(1, len(build_options))
        self.assertEqual(
            ["IA32", "EDKII", "SEC"],
            build_options[0][3:6],
        )

    def test_sku_and_default_store_macros_are_expanded(self):
        parser = self._create_parser(
            "SkuStore.dsc",
            self._platform_content("SkuStore.inc.dsc").replace(
                "  DEFINE PEI_ARCH = IA32\n",
                (
                    "  DEFINE PEI_ARCH = IA32\n"
                    "  DEFINE TEST_SKU = Default\n"
                    "  DEFINE TEST_STORE = Standard\n"
                ),
            ),
            {
                "SkuStore.inc.dsc": (
                    "[PcdsDynamicHii.$(PEI_ARCH).$(TEST_SKU).$(TEST_STORE)]\n"
                    "  gTestPkgTokenSpaceGuid.PcdHii|L\"Var\"|gTestGuid|0x0|0x1\n"
                ),
            },
        )

        hii_pcds = parser[MODEL_PCD_DYNAMIC_HII, "IA32"]
        self.assertEqual(1, len(hii_pcds))
        self.assertEqual(
            ["IA32", "Default", "Standard"],
            hii_pcds[0][3:6],
        )

    def test_lowercase_architecture_macro_value_is_uppercased(self):
        platform_content = self._platform_content(
            "LowerArch.inc.dsc",
            macro_architecture="ia32",
        ).replace(
            "!include LowerArch.inc.dsc\n",
            (
                "[LibraryClasses.$(PEI_ARCH)]\n"
                "  MainLib|TestPkg/MainLib.inf\n"
                "\n"
                "!include LowerArch.inc.dsc\n"
            ),
        )
        parser = self._create_parser(
            "LowerArch.dsc",
            platform_content,
            {
                "LowerArch.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH)]\n"
                    "  IncludedLib|TestPkg/IncludedLib.inf\n"
                ),
            },
        )

        library_classes = parser[MODEL_EFI_LIBRARY_CLASS, "IA32", None, -1]
        self.assertEqual(
            [("MainLib", "IA32"), ("IncludedLib", "IA32")],
            [(item[0], item[3]) for item in library_classes],
        )
        self.assertEqual([], parser[MODEL_EFI_LIBRARY_CLASS, "ia32"])

    def test_lowercase_component_architecture_macro_value_is_uppercased(self):
        parser = self._create_parser(
            "LowerComponent.dsc",
            self._platform_content(
                "LowerComponent.inc.dsc",
                macro_architecture="ia32",
            ),
            {
                "LowerComponent.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH)"
                ),
            },
        )
        self._assert_component_private_records(parser, "IA32")

    def _create_platform_and_include_parser(self, name, section_template):
        # Place the same sections in the platform DSC and in an !include'd
        # file, so both the raw parse and the post-processing paths are used.
        platform_content = self._platform_content(f"{name}.inc.dsc").replace(
            f"!include {name}.inc.dsc\n",
            (
                section_template.format(Origin="Main")
                + "\n"
                + f"!include {name}.inc.dsc\n"
            ),
        )
        return self._create_parser(
            f"{name}.dsc",
            platform_content,
            {
                f"{name}.inc.dsc": section_template.format(Origin="Included"),
            },
        )

    def test_arch_section_macro_is_not_visible_to_common_section_header(self):
        parser = self._create_platform_and_include_parser(
            "ArchToCommon",
            (
                "[LibraryClasses.IA32]\n"
                "  DEFINE TEST_SEC = SEC\n"
                "  {Origin}ArchLib|TestPkg/ArchLib.inf\n"
                "\n"
                "[LibraryClasses.common.$(TEST_SEC)]\n"
                "  {Origin}Lib|TestPkg/TestLib.inf\n"
            ),
        )

        library_classes = [
            (item[0], item[3], item[4])
            for item in parser[MODEL_EFI_LIBRARY_CLASS, "IA32"]
            if not item[0].endswith("ArchLib")
        ]
        self.assertEqual(
            [
                ("MainLib", "COMMON", "$(TEST_SEC)"),
                ("IncludedLib", "COMMON", "$(TEST_SEC)"),
            ],
            library_classes,
        )

    def test_arch_section_macro_is_not_visible_to_other_arch_header(self):
        parser = self._create_platform_and_include_parser(
            "ArchToArch",
            (
                "[LibraryClasses.IA32]\n"
                "  DEFINE OTHER_ARCH = X64\n"
                "  {Origin}ArchLib|TestPkg/ArchLib.inf\n"
                "\n"
                "[LibraryClasses.$(OTHER_ARCH)]\n"
                "  {Origin}Lib|TestPkg/TestLib.inf\n"
            ),
        )

        self.assertEqual([], parser[MODEL_EFI_LIBRARY_CLASS, "X64"])
        self.assertEqual(
            [("MainLib", "$(OTHER_ARCH)"), ("IncludedLib", "$(OTHER_ARCH)")],
            [
                (item[0], item[3])
                for item in parser[MODEL_EFI_LIBRARY_CLASS, "$(OTHER_ARCH)"]
            ],
        )

    def test_section_macro_is_not_visible_to_own_module_type_header(self):
        parser = self._create_platform_and_include_parser(
            "OwnModuleType",
            (
                "[LibraryClasses.common.$(SELF_SEC)]\n"
                "  DEFINE SELF_SEC = SEC\n"
                "  {Origin}FirstLib|TestPkg/FirstLib.inf\n"
                "  {Origin}SecondLib|TestPkg/SecondLib.inf\n"
            ),
        )

        self.assertEqual(
            [
                ("MainFirstLib", "$(SELF_SEC)"),
                ("MainSecondLib", "$(SELF_SEC)"),
                ("IncludedFirstLib", "$(SELF_SEC)"),
                ("IncludedSecondLib", "$(SELF_SEC)"),
            ],
            [
                (item[0], item[4])
                for item in parser[MODEL_EFI_LIBRARY_CLASS, "IA32"]
            ],
        )

    def test_section_macro_is_not_visible_to_own_arch_header(self):
        parser = self._create_platform_and_include_parser(
            "OwnArch",
            (
                "[LibraryClasses.$(SELF_ARCH)]\n"
                "  DEFINE SELF_ARCH = IA32\n"
                "  {Origin}FirstLib|TestPkg/FirstLib.inf\n"
                "  {Origin}SecondLib|TestPkg/SecondLib.inf\n"
            ),
        )

        self.assertEqual([], parser[MODEL_EFI_LIBRARY_CLASS, "IA32"])
        self.assertEqual(
            [
                ("MainFirstLib", "$(SELF_ARCH)"),
                ("MainSecondLib", "$(SELF_ARCH)"),
                ("IncludedFirstLib", "$(SELF_ARCH)"),
                ("IncludedSecondLib", "$(SELF_ARCH)"),
            ],
            [
                (item[0], item[3])
                for item in parser[MODEL_EFI_LIBRARY_CLASS, "$(SELF_ARCH)"]
            ],
        )

    # A [Defines] macro is positional: a later definition overrides it for
    # the remainder of the file (DSC spec 2.2.6). An !include'd file is
    # equivalent to pasting its content into the DSC file (DSC spec 2.2.5),
    # so a macro redefined in an included file applies to the section headers
    # that follow it, including those of the including file.
    def test_macro_redefined_in_include_applies_to_later_section_headers(self):
        section = (
            "[PcdsFixedAtBuild.$(PEI_ARCH)]\n"
            "  gTestPkgTokenSpaceGuid.{Name}|1\n"
        )
        included_content = (
            "[Defines]\n"
            "  DEFINE PEI_ARCH = X64\n"
            "\n"
            + section.format(Name="PcdIncluded")
        )
        include_content = self._platform_content("Redefine.inc.dsc").replace(
            "!include Redefine.inc.dsc\n",
            (
                section.format(Name="PcdBefore")
                + "\n"
                + "!include Redefine.inc.dsc\n"
                + "\n"
                + section.format(Name="PcdAfter")
            ),
        )
        paste_content = include_content.replace(
            "!include Redefine.inc.dsc\n",
            included_content,
        )

        for name, platform_content, included_files in (
            ("RedefineInclude", include_content,
             {"Redefine.inc.dsc": included_content}),
            ("RedefinePaste", paste_content, {}),
        ):
            with self.subTest(name):
                parser = self._create_parser(
                    f"{name}.dsc",
                    platform_content,
                    included_files,
                )
                self.assertEqual(
                    [
                        ("PcdBefore", "IA32"),
                        ("PcdIncluded", "X64"),
                        ("PcdAfter", "X64"),
                    ],
                    [
                        (item[1], item[3])
                        for item in (
                            parser[MODEL_PCD_FIXED_AT_BUILD, "IA32"]
                            + parser[MODEL_PCD_FIXED_AT_BUILD, "X64"]
                        )
                    ],
                )

    def test_macro_redefined_in_include_under_defines_section(self):
        parser = self._create_parser(
            "RedefineDefines.dsc",
            self._platform_content("RedefineDefines.inc.dsc")
            + "\n"
            + "[PcdsFixedAtBuild.$(PEI_ARCH)]\n"
            + "  gTestPkgTokenSpaceGuid.PcdAfter|1\n",
            {"RedefineDefines.inc.dsc": "  DEFINE PEI_ARCH = X64\n"},
        )

        self.assertEqual([], parser[MODEL_PCD_FIXED_AT_BUILD, "IA32"])
        self.assertEqual(
            [("PcdAfter", "X64")],
            [
                (item[1], item[3])
                for item in parser[MODEL_PCD_FIXED_AT_BUILD, "X64"]
            ],
        )

    def test_command_line_macro_overrides_macro_redefined_in_include(self):
        GlobalData.gCommandLineDefines["PEI_ARCH"] = "IA32"
        parser = self._create_parser(
            "RedefineCommandLine.dsc",
            self._platform_content("RedefineCommandLine.inc.dsc")
            + "\n"
            + "[PcdsFixedAtBuild.$(PEI_ARCH)]\n"
            + "  gTestPkgTokenSpaceGuid.PcdAfter|1\n",
            {"RedefineCommandLine.inc.dsc": "  DEFINE PEI_ARCH = X64\n"},
        )

        self.assertEqual([], parser[MODEL_PCD_FIXED_AT_BUILD, "X64"])
        self.assertEqual(
            [("PcdAfter", "IA32")],
            [
                (item[1], item[3])
                for item in parser[MODEL_PCD_FIXED_AT_BUILD, "IA32"]
            ],
        )

    def test_reprocessing_does_not_reuse_component_owner_mapping(self):
        GlobalData.gCommandLineDefines["ENABLE_INCLUDE"] = "FALSE"
        platform_content = self._platform_content("Reprocess.inc.dsc").replace(
            "!include Reprocess.inc.dsc\n",
            (
                "!if $(ENABLE_INCLUDE) == TRUE\n"
                "!include Reprocess.inc.dsc\n"
                "!endif\n"
            ),
        )
        parser = self._create_parser(
            "Reprocess.dsc",
            platform_content,
            {
                "Reprocess.inc.dsc": (
                    "[LibraryClasses.$(PEI_ARCH)]\n"
                    "  TestLib|TestPkg/TestLib.inf\n"
                )
            },
        )

        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, "$(PEI_ARCH)"],
        )

        GlobalData.gCommandLineDefines["ENABLE_INCLUDE"] = "TRUE"
        parser.DoPostProcess()

        library_classes = parser[
            MODEL_EFI_LIBRARY_CLASS,
            "IA32",
            None,
            -1,
        ]
        self.assertEqual(1, len(library_classes))
        self.assertEqual("IA32", library_classes[0][3])
        self.assertEqual(
            [],
            parser[MODEL_EFI_LIBRARY_CLASS, "$(PEI_ARCH)"],
        )

    def test_private_subsection_include_keeps_expanded_architecture(self):
        parser = self._create_parser(
            "SubsectionInclude.dsc",
            self._platform_content("SubsectionInclude.inc.dsc"),
            {
                "SubsectionInclude.inc.dsc": (
                    "[Components.$(PEI_ARCH), Components.X64]\n"
                    "  TestPkg/TestPeim.inf {\n"
                    "    <LibraryClasses>\n"
                    "      !include PrivateLibrary.inc.dsc\n"
                    "    <PcdsFixedAtBuild>\n"
                    "      gTestPkgTokenSpaceGuid.PcdTest|TRUE\n"
                    "  }\n"
                ),
                "PrivateLibrary.inc.dsc": (
                    "TestLib|TestPkg/TestLib.inf\n"
                ),
            },
        )
        self._assert_component_private_records(parser, "IA32")
        self._assert_component_private_records(parser, "X64")

    def test_multi_architecture_private_records_keep_matching_owners(self):
        parser = self._create_parser(
            "MultiArch.dsc",
            self._platform_content("MultiArch.inc.dsc"),
            {
                "MultiArch.inc.dsc": self._component_content(
                    "Components.$(PEI_ARCH), Components.X64"
                )
            },
        )
        self._assert_component_private_records(parser, "IA32")
        self._assert_component_private_records(parser, "X64")

    def test_repeated_macro_sections_keep_private_records_with_their_owner(self):
        parser = self._create_parser(
            "RepeatedSections.dsc",
            self._platform_content("RepeatedSections.inc.dsc"),
            {
                "RepeatedSections.inc.dsc": (
                    "[Components.$(PEI_ARCH)]\n"
                    "  TestPkg/FirstPeim.inf\n"
                    "\n"
                    + self._component_content("Components.$(PEI_ARCH)")
                )
            },
        )

        components = parser[MODEL_META_DATA_COMPONENT, "IA32"]
        self.assertEqual(
            ["TestPkg/FirstPeim.inf", "TestPkg/TestPeim.inf"],
            [component[0] for component in components],
        )
        first_component_id = components[0][6]
        private_component_id = components[1][6]

        self.assertEqual(
            [],
            parser[
                MODEL_EFI_LIBRARY_CLASS,
                "IA32",
                None,
                first_component_id,
            ],
        )
        self.assertEqual(
            1,
            len(
                parser[
                    MODEL_EFI_LIBRARY_CLASS,
                    "IA32",
                    None,
                    private_component_id,
                ]
            ),
        )
        self.assertEqual(
            1,
            len(
                parser[
                    MODEL_PCD_FIXED_AT_BUILD,
                    "IA32",
                    None,
                    private_component_id,
                ]
            ),
        )


def TheTestSuite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
        DscParserComponentScopeTests
    )


if __name__ == "__main__":
    unittest.main()
