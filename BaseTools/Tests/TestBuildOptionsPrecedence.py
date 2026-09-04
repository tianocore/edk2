## @file
# Unit tests for [BuildOptions] section precedence handling.
#
# Tests PlatformAutoGen._ExpandGroupedBuildOption() and the per-section
# BUILDRULEFAMILY vs FAMILY precedence rules implemented in
# PlatformAutoGen._ExpandBuildOption().
#
# Background:
#
# A DSC (and any DSC it !includes) may contain multiple [BuildOptions]
# sections. Each option key carries a tool chain FAMILY prefix, for example:
#
#   [BuildOptions]
#     CLANGPDB:*_*_*_CC_FLAGS = -D SOMETHING
#     GCC:*_*_*_CC_FLAGS      = -D SOMETHING_1
#     *_*_*_CC_FLAGS          = -D SOMETHING_2
#
# Within a single [BuildOptions] section _ExpandBuildOption() applies a
# precedence rule: if any key's FAMILY matches the tool's BUILDRULEFAMILY,
# then keys that only match the tool's FAMILY (but not its BUILDRULEFAMILY)
# are dropped for that section. Keys with no FAMILY prefix always apply.
#
# Copyright (c) Microsoft Corporation.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import re
import shutil
import subprocess
import sys
import threading
import unittest
from collections import OrderedDict
from pathlib import Path

# Add BaseTools Python source to path
_TESTS_DIR = Path(__file__).resolve().parent
_PYTHON_SRC = str(_TESTS_DIR.parent / 'Source' / 'Python')
if _PYTHON_SRC not in sys.path:
    sys.path.insert(0, _PYTHON_SRC)

from AutoGen.PlatformAutoGen import PlatformAutoGen
from Common.DataType import EDKII_NAME


class TestExpandGroupedBuildOption(unittest.TestCase):
    """Test PlatformAutoGen._ExpandGroupedBuildOption() precedence handling.

    A PlatformAutoGen instance is created without running __init__ and only
    the attributes consumed by _ExpandBuildOption()/_ExpandGroupedBuildOption()
    are populated. A synthetic tool definition is passed explicitly so the
    real production logic is exercised without a workspace or toolchain.
    """

    # Build context used by the precedence logic.
    BUILD_TARGET     = 'DEBUG'
    TOOL_CHAIN       = 'CLANGPDB'
    ARCH             = 'X64'
    BUILD_RULE_FAMILY = 'CLANGPDB'

    # Synthetic tool definition modeled on the real CLANGPDB toolchain, where
    # CC has FAMILY=GCC and BUILDRULEFAMILY=CLANGPDB (GCC is used as a FAMILY
    # while CLANGPDB is the BUILDRULEFAMILY).
    # => a "CLANGPDB:" prefixed key is a BUILDRULEFAMILY match.
    # => a "GCC:"      prefixed key is a FAMILY-only match.
    TOOL_DEF = {
        'CC': {
            'FAMILY':          'GCC',
            'BUILDRULEFAMILY': 'CLANGPDB',
        },
    }

    # Option key string: TARGET_TAGNAME_ARCH_TOOL_ATTRIBUTE
    CC_FLAGS = '*_*_*_CC_FLAGS'

    def _make_pa(self):
        """Create a PlatformAutoGen stub with the minimal precedence inputs."""
        # Bypass AutoGen.__new__ (which requires constructor args and caches
        # instances) so only the attributes used by the precedence logic are set.
        pa = object.__new__(PlatformAutoGen)
        pa.BuildTarget = self.BUILD_TARGET
        pa.ToolChain   = self.TOOL_CHAIN
        pa.Arch        = self.ARCH
        # BuildRuleFamily is a cached_property (non-data descriptor); writing
        # the instance attribute pre-seeds the cache.
        pa.BuildRuleFamily = self.BUILD_RULE_FAMILY
        return pa

    @staticmethod
    def _group(*entries):
        """Build one [BuildOptions] section as an OrderedDict.

        Each entry is (family, key_string, value) with CodeBase defaulting
        to EDKII. Pass a 4-tuple to override the CodeBase.
        """
        section = OrderedDict()
        for entry in entries:
            if len(entry) == 4:
                family, key, code_base, value = entry
            else:
                family, key, value = entry
                code_base = EDKII_NAME
            section[(family, key, code_base)] = value
        return section

    # (description, module_style, option_groups, expected_result)
    #
    # option_groups is a list of sections (each an OrderedDict produced by
    # _group); expected_result is the merged {Tool: {Attr: Value}} dict.
    def _test_cases(self):
        return [
            # TC1: Single section containing both a BUILDRULEFAMILY match
            # (CLANGPDB) and a FAMILY-only match (GCC). Within one section the
            # BUILDRULEFAMILY match wins and the FAMILY-only key is dropped.
            ('TC1: single section BUILDRULEFAMILY beats FAMILY',
             None,
             [self._group(('CLANGPDB', self.CC_FLAGS, '-brule'),
                          ('GCC',      self.CC_FLAGS, '-fam'))],
             {'CC': {'FLAGS': '-brule'}}),

            # TC2: BUILDRULEFAMILY match in section 1, FAMILY-only match in
            # section 2. Both options are kept.
            ('TC2: BUILDRULEFAMILY then FAMILY across sections',
             None,
             [self._group(('CLANGPDB', self.CC_FLAGS, '-brule')),
              self._group(('GCC',      self.CC_FLAGS, '-fam'))],
             {'CC': {'FLAGS': '-brule -fam'}}),

            # TC3: Same as TC2 but section order reversed. Order of the
            # merged FLAGS follows the order of the sections.
            ('TC3: FAMILY then BUILDRULEFAMILY across sections',
             None,
             [self._group(('GCC',      self.CC_FLAGS, '-fam')),
              self._group(('CLANGPDB', self.CC_FLAGS, '-brule'))],
             {'CC': {'FLAGS': '-fam -brule'}}),

            # TC4: Keys with no FAMILY prefix always apply. Two empty-family
            # FLAGS in different sections are both kept and appended.
            ('TC4: empty-family FLAGS across sections are appended',
             None,
             [self._group(('', self.CC_FLAGS, '-c1')),
              self._group(('', self.CC_FLAGS, '-c2'))],
             {'CC': {'FLAGS': '-c1 -c2'}}),

            # TC5: Neither section has a BUILDRULEFAMILY match, so each
            # independently falls back to FAMILY matching. Both GCC FLAGS are
            # kept and appended.
            ('TC5: FAMILY-only FLAGS across sections are appended',
             None,
             [self._group(('GCC', self.CC_FLAGS, '-g1')),
              self._group(('GCC', self.CC_FLAGS, '-g2'))],
             {'CC': {'FLAGS': '-g1 -g2'}}),

            # TC6: BUILDRULEFAMILY match in both sections. Both are kept and
            # appended (each section selects its BUILDRULEFAMILY option).
            ('TC6: BUILDRULEFAMILY in both sections are appended',
             None,
             [self._group(('CLANGPDB', self.CC_FLAGS, '-b1')),
              self._group(('CLANGPDB', self.CC_FLAGS, '-b2'))],
             {'CC': {'FLAGS': '-b1 -b2'}}),

            # TC7: A FAMILY prefix that matches neither the tool FAMILY nor
            # the BUILDRULEFAMILY is dropped entirely.
            ('TC7: non-matching family prefix is dropped',
             None,
             [self._group(('MSFT', self.CC_FLAGS, '-nomatch'),
                          ('',     self.CC_FLAGS, '-keep'))],
             {'CC': {'FLAGS': '-keep'}}),

            # TC8: No option groups produces an empty merged result.
            ('TC8: empty option groups yield empty result',
             None,
             [],
             {}),
        ]

    def test_expand_grouped_build_option(self):
        """Parameterized precedence test for _ExpandGroupedBuildOption."""
        for desc, module_style, groups, expected in self._test_cases():
            with self.subTest(desc):
                pa = self._make_pa()
                result = pa._ExpandGroupedBuildOption(
                    groups, module_style, ToolDef=self.TOOL_DEF
                )
                self.assertEqual(result, expected)

    def test_grouped_preserves_family_lost_when_merged(self):
        """Regression: grouped evaluation must keep the FAMILY option that a
        single merged dictionary would have dropped.

        Section 1 contributes a BUILDRULEFAMILY (CLANGPDB) match and section 2
        contributes a FAMILY-only (GCC) match. Evaluating the two sections
        merged into one dict drops the GCC option; evaluating them as
        independent groups keeps both.
        """
        pa = self._make_pa()

        section1 = self._group(('CLANGPDB', self.CC_FLAGS, '-brule'))
        section2 = self._group(('GCC',      self.CC_FLAGS, '-fam'))

        # Grouped (fixed) behavior keeps both options.
        grouped = pa._ExpandGroupedBuildOption(
            [section1, section2], None, ToolDef=self.TOOL_DEF
        )
        self.assertEqual(grouped, {'CC': {'FLAGS': '-brule -fam'}})

        # A single merged section (pre-fix behavior) loses the FAMILY option
        # because the BUILDRULEFAMILY match short-circuits the FAMILY pass.
        merged_section = OrderedDict()
        merged_section.update(section1)
        merged_section.update(section2)
        merged = pa._ExpandBuildOption(
            merged_section, None, ToolDef=self.TOOL_DEF
        )
        self.assertEqual(merged, {'CC': {'FLAGS': '-brule'}})
        self.assertNotEqual(grouped, merged)


class TestFunctionalBuildOptionsMerge(unittest.TestCase):
    """Functional tests that build a real module to confirm [BuildOptions]
    sections from a DSC and an !included DSC.inc are merged correctly.

    This targets the CLANGPDB toolchain specifically because it is a tool
    chain where the CC tool's FAMILY (GCC) differs from its BUILDRULEFAMILY
    (CLANGPDB).

    Each test:
      1. Generates a package with a trivial BASE library, a DSC, and a
         DSC.inc that the DSC pulls in with !include.
      2. Puts one CC_FLAGS option in the DSC [BuildOptions] section and a
         second CC_FLAGS option in the DSC.inc [BuildOptions] section, using
         BUILDRULEFAMILY (CLANGPDB:) and FAMILY (GCC:) prefixes.
      3. Builds the library and reads the generated module Makefile.
      4. Asserts both options appear in CC_FLAGS, and that an option with a
         non-matching family prefix (MSFT:) is dropped.

    Prerequisites:
    - edksetup has been run (sets WORKSPACE and puts 'build' on PATH).
    - BaseTools C binaries are built.
    - The selected toolchain (default CLANGPDB) and arch (default X64)
      are installed and working.
    """

    WORKSPACE = None
    TOOLCHAIN = None
    ARCH = None
    BUILD_AVAILABLE = False

    PKG_NAME = 'TestBuildOptMergePkg'
    PLATFORM_NAME = 'TestBuildOptMerge'
    LIB_NAME = 'TestBuildOptMergeLib'

    # Markers injected via [BuildOptions]; presence/absence is checked in the
    # generated CC_FLAGS. Harmless preprocessor defines that do not affect the
    # trivial source compiling.
    MARKER_BRULE   = 'MARKER_BUILDRULEFAMILY'   # via CLANGPDB: prefix
    MARKER_FAMILY  = 'MARKER_FAMILY'            # via GCC: prefix
    MARKER_NOMATCH = 'MARKER_NOMATCH'           # via MSFT: prefix (dropped)

    # --- File content constants / templates ---

    DEC_CONTENT = """\
[Defines]
  DEC_SPECIFICATION = 0x00010005
  PACKAGE_NAME      = TestBuildOptMergePkg
  PACKAGE_GUID      = 6B7E1C2A-5F4D-4E2B-9A3C-1D2E3F4A5B6C
  PACKAGE_VERSION   = 1.0
"""

    LIB_C_SOURCE = """\
#include <Base.h>

UINTN
EFIAPI
TestBuildOptMergeAdd (
  IN UINTN  A,
  IN UINTN  B
  )
{
  return A + B;
}
"""

    LIB_INF_CONTENT = """\
[Defines]
  INF_VERSION    = 0x00010005
  BASE_NAME      = TestBuildOptMergeLib
  FILE_GUID      = 2C3D4E5F-6A7B-4C8D-9E0F-1A2B3C4D5E6F
  MODULE_TYPE    = BASE
  VERSION_STRING = 1.0
  LIBRARY_CLASS  = TestBuildOptMergeLib

[Sources]
  TestBuildOptMergeLib.c

[Packages]
  MdePkg/MdePkg.dec
"""

    # DSC carries one [BuildOptions] section; the {dsc_build_options} block is
    # inserted there, and the !include pulls in a second section from the inc.
    DSC_TEMPLATE = """\
[Defines]
  PLATFORM_NAME           = TestBuildOptMerge
  PLATFORM_GUID           = 3F2A1B0C-9D8E-4F7A-8B6C-5D4E3F2A1B0C
  PLATFORM_VERSION        = 1.0
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/TestBuildOptMerge
  SUPPORTED_ARCHITECTURES = {arch}
  BUILD_TARGETS           = DEBUG
  SKUID_IDENTIFIER        = DEFAULT

[BuildOptions]
{dsc_build_options}

!include TestBuildOptMergePkg/TestBuildOptMerge.dsc.inc

[Components]
  TestBuildOptMergePkg/TestBuildOptMergeLib/TestBuildOptMergeLib.inf
"""

    DSC_INC_TEMPLATE = """\
[BuildOptions]
{inc_build_options}
"""

    @classmethod
    def _detect_option(cls, name, default):
        """Read a --<name>[=value] command line option (default if absent)."""
        for i, arg in enumerate(sys.argv):
            if arg == f'--{name}' and i + 1 < len(sys.argv):
                return sys.argv[i + 1]
            if arg.startswith(f'--{name}='):
                return arg.split('=', 1)[1]
        return default

    @classmethod
    def setUpClass(cls):
        cls.WORKSPACE = os.environ.get('WORKSPACE')
        if cls.WORKSPACE is None:
            return
        cls.WORKSPACE = os.path.abspath(cls.WORKSPACE)
        cls.TOOLCHAIN = cls._detect_option('toolchain', 'CLANGPDB')
        cls.ARCH = cls._detect_option('arch', 'X64')
        try:
            result = subprocess.run(
                'build --version', capture_output=True, text=True,
                timeout=30, cwd=cls.WORKSPACE, shell=True
            )
            cls.BUILD_AVAILABLE = result.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass

    @classmethod
    def tearDownClass(cls):
        """Remove the generated test package directory."""
        if cls.WORKSPACE:
            pkg_dir = Path(cls.WORKSPACE, cls.PKG_NAME)
            if pkg_dir.is_dir():
                shutil.rmtree(pkg_dir, ignore_errors=True)

    def setUp(self):
        if self.WORKSPACE is None:
            self.fail("WORKSPACE environment variable is not set. "
                      "Run edksetup before running tests.")
        if not self.BUILD_AVAILABLE:
            self.fail("edk2 'build' command not found in PATH. "
                      "Run edksetup before running tests.")

    # --- Test package generation ---

    def _create_test_package(self, dsc_build_options, inc_build_options):
        """Create the package with the library, DSC, and DSC.inc.

        Args:
            dsc_build_options: Text placed in the DSC [BuildOptions] section.
            inc_build_options: Text placed in the DSC.inc [BuildOptions]
                section.

        Returns:
            Absolute path to the created package directory.
        """
        pkg = Path(self.WORKSPACE, self.PKG_NAME)
        pkg.mkdir(parents=True, exist_ok=True)
        (pkg / f'{self.PKG_NAME}.dec').write_text(self.DEC_CONTENT)

        lib = pkg / self.LIB_NAME
        lib.mkdir(parents=True, exist_ok=True)
        (lib / f'{self.LIB_NAME}.c').write_text(self.LIB_C_SOURCE)
        (lib / f'{self.LIB_NAME}.inf').write_text(self.LIB_INF_CONTENT)

        (pkg / f'{self.PLATFORM_NAME}.dsc').write_text(
            self.DSC_TEMPLATE.format(
                arch=self.ARCH, dsc_build_options=dsc_build_options))
        (pkg / f'{self.PLATFORM_NAME}.dsc.inc').write_text(
            self.DSC_INC_TEMPLATE.format(inc_build_options=inc_build_options))

        return pkg

    def _run_build(self, dsc_path):
        """Run the edk2 build for the library component.

        Returns:
            Tuple of (returncode, stdout, stderr).
        """
        rel_dsc = os.path.relpath(dsc_path, self.WORKSPACE)
        cmd = (
            f'build -p {rel_dsc}'
            f' -a {self.ARCH} -b DEBUG -t {self.TOOLCHAIN} --quiet'
        )

        proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, bufsize=1, cwd=self.WORKSPACE, shell=True,
        )
        stdout_lines, stderr_lines = [], []
        verbose = '-v' in sys.argv or '--verbose' in sys.argv

        def reader(pipe, sink, stream):
            for line in pipe:
                if stream:
                    stream.write(line)
                    stream.flush()
                sink.append(line)

        threads = [
            threading.Thread(target=reader,
                             args=(proc.stdout, stdout_lines,
                                   sys.stdout if verbose else None)),
            threading.Thread(target=reader,
                             args=(proc.stderr, stderr_lines,
                                   sys.stderr if verbose else None)),
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=600)
        proc.stdout.close()
        proc.stderr.close()
        proc.wait(timeout=600)
        return proc.returncode, ''.join(stdout_lines), ''.join(stderr_lines)

    def _read_cc_flags(self):
        """Locate the library's generated module makefile and return CC_FLAGS.

        The module makefile name depends on the build's make flavor:
        'Makefile' for nmake (MSFT family) and 'GNUmakefile' for GNU make
        (GCC/CLANGPDB family). Both define CC_FLAGS, so search for either.

        Returns:
            The CC_FLAGS string from the module makefile.
        """
        build_root = Path(
            self.WORKSPACE, 'Build', self.PLATFORM_NAME,
            f'DEBUG_{self.TOOLCHAIN}', self.ARCH
        )
        makefiles = []
        for name in ('Makefile', 'GNUmakefile'):
            makefiles += list(build_root.rglob(f'{self.LIB_NAME}/{name}'))
        self.assertTrue(
            makefiles,
            f"No generated module makefile found under {build_root}")
        text = makefiles[0].read_text()
        match = re.search(r'^CC_FLAGS\s*=\s*(.*)$', text, re.MULTILINE)
        self.assertIsNotNone(
            match, f"CC_FLAGS not found in {makefiles[0]}")
        return match.group(1)

    def _build_and_get_cc_flags(self, dsc_build_options, inc_build_options):
        """Generate the package, build it, and return the module CC_FLAGS."""
        # Start from a clean output tree so a stale Makefile can't mask a
        # dropped option.
        build_root = Path(self.WORKSPACE, 'Build', self.PLATFORM_NAME)
        if build_root.is_dir():
            shutil.rmtree(build_root, ignore_errors=True)

        pkg = self._create_test_package(dsc_build_options, inc_build_options)
        dsc_path = pkg / f'{self.PLATFORM_NAME}.dsc'

        rc, stdout, stderr = self._run_build(dsc_path)
        self.assertEqual(
            rc, 0,
            f"Build failed (rc={rc}).\nstdout:\n{stdout}\nstderr:\n{stderr}")
        return self._read_cc_flags()

    # --- Tests ---

    def test_buildrulefamily_then_family_across_sections(self):
        """DSC has the BUILDRULEFAMILY (CLANGPDB) option, DSC.inc has the
        FAMILY-only (GCC) option. After merging, both must survive.

        Before the fix, evaluating the merged sections together let the
        CLANGPDB BUILDRULEFAMILY match suppress the GCC FAMILY-only option.
        """
        dsc_opts = f'  CLANGPDB:*_*_*_CC_FLAGS = -D {self.MARKER_BRULE}'
        inc_opts = f'  GCC:*_*_*_CC_FLAGS = -D {self.MARKER_FAMILY}'

        cc_flags = self._build_and_get_cc_flags(dsc_opts, inc_opts)

        self.assertIn(self.MARKER_BRULE, cc_flags,
                      f"BUILDRULEFAMILY option missing.\nCC_FLAGS: {cc_flags}")
        self.assertIn(self.MARKER_FAMILY, cc_flags,
                      f"FAMILY option dropped (regression).\n"
                      f"CC_FLAGS: {cc_flags}")

    def test_family_then_buildrulefamily_across_sections(self):
        """Reverse of the above: DSC has the FAMILY-only (GCC) option and
        DSC.inc has the BUILDRULEFAMILY (CLANGPDB) option. Both must survive
        regardless of which section carries which prefix.
        """
        dsc_opts = f'  GCC:*_*_*_CC_FLAGS = -D {self.MARKER_FAMILY}'
        inc_opts = f'  CLANGPDB:*_*_*_CC_FLAGS = -D {self.MARKER_BRULE}'

        cc_flags = self._build_and_get_cc_flags(dsc_opts, inc_opts)

        self.assertIn(self.MARKER_BRULE, cc_flags,
                      f"BUILDRULEFAMILY option missing.\nCC_FLAGS: {cc_flags}")
        self.assertIn(self.MARKER_FAMILY, cc_flags,
                      f"FAMILY option dropped (regression).\n"
                      f"CC_FLAGS: {cc_flags}")

    def test_nonmatching_family_dropped_after_merge(self):
        """A non-matching family prefix (MSFT) in the DSC.inc must still be
        dropped after merging, while the matching options from both sections
        are kept.
        """
        dsc_opts = f'  CLANGPDB:*_*_*_CC_FLAGS = -D {self.MARKER_BRULE}'
        inc_opts = (
            f'  GCC:*_*_*_CC_FLAGS = -D {self.MARKER_FAMILY}\n'
            f'  MSFT:*_*_*_CC_FLAGS = -D {self.MARKER_NOMATCH}'
        )

        cc_flags = self._build_and_get_cc_flags(dsc_opts, inc_opts)

        self.assertIn(self.MARKER_BRULE, cc_flags,
                      f"BUILDRULEFAMILY option missing.\nCC_FLAGS: {cc_flags}")
        self.assertIn(self.MARKER_FAMILY, cc_flags,
                      f"FAMILY option dropped (regression).\n"
                      f"CC_FLAGS: {cc_flags}")
        self.assertNotIn(self.MARKER_NOMATCH, cc_flags,
                         f"Non-matching family option leaked into flags.\n"
                         f"CC_FLAGS: {cc_flags}")


if __name__ == '__main__':
    unittest.main()
