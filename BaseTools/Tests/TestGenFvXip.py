## @file
# Unit and functional tests for GenFv XIP rebase behavior
#
# Tests the ,XIP suffix generation in Python GenFds and the
# ForceRebase/XIP decision logic in C GenFv.
#
# Test Plan Summary:
#
# FfsRebase() in GenFvInternalLib.c decides whether to rebase each PE/COFF
# image in an FV based on three inputs: ForceRebase, BaseAddress, and XipFile[].
#
#   ForceRebase  BaseAddress  XipFile[]  Result
#   -----------  -----------  ---------  ---------------------------------
#   -1 (unset)   0            any        No rebase (early return)
#   0  (FALSE)   any          any        No rebase (early return)
#   1  (TRUE)    any          FALSE      No rebase (skip non-XIP file)
#   1  (TRUE)    any          TRUE       Rebase (XIP file selected)
#   -1 (unset)   != 0         any        Rebase ALL files (legacy path)
#
# Unit Tests (TestDetermineXipEnabled):
#   11 parameterized subtests calling FfsInfStatement.DetermineXipEnabled()
#   with RuleComplexFile and RuleSimpleFile objects to verify Xip attribute
#   parsing (TRUE/FALSE/None, case insensitive, boolean vs string).
#
# Functional Tests (TestFunctionalBuildXipRebase):
#   8 test cases using real edk2 builds with generated DSC/FDF files
#   containing a test package (2 PEIMs + 1 DXE driver). Each test
#   verifies:
#     1. FV INF file has correct ,XIP suffix on EFI_FILE_NAME entries
#     2. FV map file shows correct rebase status (Fixed Flash Address)
#     3. PE/COFF ImageBase in the FV binary matches expected value
#
#   TC1: ForceRebase=unset, Base=0        -> no rebase
#   TC2: ForceRebase=unset, Base!=0       -> rebase all (legacy)
#   TC3: ForceRebase=FALSE, Base!=0       -> no rebase
#   TC4: ForceRebase=TRUE,  all Xip=TRUE  -> rebase all
#   TC5: ForceRebase=TRUE,  selective Xip -> rebase only Xip=TRUE
#   TC6: ForceRebase=TRUE,  no Xip        -> no rebase
#   TC7: ForceRebase=TRUE,  mixed Xip     -> rebase Xip=TRUE only
#   TC8: ForceRebase=TRUE,  Base=0, Xip   -> rebase (force overrides)
#
# Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import ctypes
import os
import re
import shutil
import subprocess
import sys
import threading
import unittest
from pathlib import Path

# Add BaseTools Python source to path
_TESTS_DIR = Path(__file__).resolve().parent
_PYTHON_SRC = str(_TESTS_DIR.parent / 'Source' / 'Python')
if _PYTHON_SRC not in sys.path:
    sys.path.insert(0, _PYTHON_SRC)

from GenFds.RuleComplexFile import RuleComplexFile
from GenFds.RuleSimpleFile import RuleSimpleFile
from GenFds.EfiSection import EfiSection
from GenFds.FfsInfStatement import FfsInfStatement
from FirmwareStorageFormat.FvHeader import EFI_FIRMWARE_VOLUME_HEADER
from FirmwareStorageFormat.FfsFileHeader import EFI_FFS_FILE_HEADER
from FirmwareStorageFormat.SectionHeader import (
    EFI_COMMON_SECTION_HEADER,
    EFI_SECTION_PE32,
)
from FirmwareStorageFormat.PECOFFHeader import (
    EFI_IMAGE_DOS_HEADER,
    EFI_IMAGE_DOS_SIGNATURE,
    EFI_IMAGE_NT_HEADERS32,
    EFI_IMAGE_NT_HEADERS64,
    EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC,
    EFI_IMAGE_NT_SIGNATURE,
)


class TestDetermineXipEnabled(unittest.TestCase):
    """Test FfsInfStatement.DetermineXipEnabled() with real Rule objects.

    This calls the actual production code that determines whether XIP
    is enabled based on a Rule object's Xip attribute(s).
    """

    _UNSET = object()  # sentinel: do not set the Xip attribute

    # (rule_class, section_xip_list, rule_xip, expected, description)
    #
    # For RuleComplexFile:
    #   section_xip_list is a list of Xip values per EfiSection (_UNSET = no attr).
    #   rule_xip is ignored (_UNSET).
    #
    # For RuleSimpleFile:
    #   section_xip_list is None (no SectionList used).
    #   rule_xip is the value to assign to rule.Xip (_UNSET = leave default).
    TEST_CASES = [
        # RuleComplexFile: one section with Xip='TRUE'.
        # DetermineXipEnabled iterates SectionList looking for any section with
        # Xip set to 'TRUE' (case-insensitive string match).
        # A single section set to 'TRUE' should return True (XIP-eligible).
        (RuleComplexFile, ['TRUE'],         _UNSET, True,
         'Complex: section Xip=TRUE'),
        # RuleComplexFile: Xip='true' (lowercase).
        # The FDF parser normalizes keywords but DetermineXipEnabled uses
        # case-insensitive matching. Verifies 'true' is equivalent to 'TRUE'.
        (RuleComplexFile, ['true'],         _UNSET, True,
         'Complex: section Xip=true (case insensitive)'),
        # RuleComplexFile: one section with Xip='FALSE'.
        # An explicit 'FALSE' must not be confused with 'TRUE'. Confirms the
        # string 'FALSE' does not accidentally match the 'TRUE' comparison.
        (RuleComplexFile, ['FALSE'],        _UNSET, False,
         'Complex: section Xip=FALSE'),
        # RuleComplexFile: section with no Xip attribute set.
        # EfiSection.__init__ does not set Xip by default. Verifies that when
        # the FDF rule omits the Xip keyword, DetermineXipEnabled returns False
        # without raising an AttributeError.
        (RuleComplexFile, [_UNSET],         _UNSET, False,
         'Complex: section with no Xip attribute'),
        # RuleComplexFile: two sections, only the second has Xip=TRUE.
        # DetermineXipEnabled should return True if ANY section in the list has
        # Xip=TRUE, not just the first. Tests iteration with Xip=TRUE at index 1.
        (RuleComplexFile, [_UNSET, 'TRUE'], _UNSET, True,
         'Complex: multiple sections, one Xip=TRUE'),
        # RuleComplexFile: empty SectionList.
        # Edge case: a complex rule with no sections should safely return False
        # without raising an exception from iterating an empty list.
        (RuleComplexFile, [],               _UNSET, False,
         'Complex: empty section list'),
        # RuleSimpleFile: Xip='TRUE' (string).
        # Unlike RuleComplexFile, RuleSimpleFile stores Xip directly on the
        # rule object. Verifies the string 'TRUE' path via isinstance check.
        (RuleSimpleFile,  None, 'TRUE',  True,
         'Simple: Xip=TRUE (string)'),
        # RuleSimpleFile: Xip=True (Python boolean).
        # The FDF parser may set Xip as a boolean True rather than the string
        # 'TRUE'. Verifies that a Python boolean True is recognized as XIP.
        (RuleSimpleFile,  None, True,    True,
         'Simple: Xip=True (boolean)'),
        # RuleSimpleFile: Xip='FALSE' (string).
        # Verifies that an explicit 'FALSE' string causes DetermineXipEnabled
        # to return False. Complement of the 'TRUE' string test.
        (RuleSimpleFile,  None, 'FALSE', False,
         'Simple: Xip=FALSE'),
        # RuleSimpleFile: default Xip value from RuleClassObject.__init__.
        # When constructed without setting Xip, the default is False (boolean).
        # Represents the common case where the FDF rule omits the Xip keyword.
        (RuleSimpleFile,  None, _UNSET,  False,
         'Simple: default Xip (not set)'),
        # RuleSimpleFile: Xip=None.
        # Edge case: if code or a parser bug sets Xip=None, it should be
        # treated as falsy and return False rather than raising a TypeError.
        (RuleSimpleFile,  None, None,    False,
         'Simple: Xip=None'),
    ]

    def test_determine_xip_enabled(self) -> None:
        """Parameterized test for DetermineXipEnabled with Rule objects."""
        for rule_class, section_xip_list, rule_xip, expected, desc in self.TEST_CASES:
            with self.subTest(desc):
                rule = rule_class()
                if section_xip_list is not None:
                    # RuleComplexFile: build SectionList
                    rule.SectionList = []
                    for xip_val in section_xip_list:
                        sect = EfiSection()
                        if xip_val is not self._UNSET:
                            sect.Xip = xip_val
                        rule.SectionList.append(sect)
                elif rule_xip is not self._UNSET:
                    # RuleSimpleFile: set Xip on rule
                    rule.Xip = rule_xip
                self.assertEqual(
                    FfsInfStatement.DetermineXipEnabled(rule), expected
                )


class TestFunctionalBuildXipRebase(unittest.TestCase):
    """Functional tests that build with generated DSC/FDF files to exercise
    all ForceRebase/BaseAddress/Xip combinations.

    Prerequisites:
    - edksetup has been run (sets WORKSPACE and puts build in PATH)
    - BaseTools C binaries built (GenFv.exe, etc.)
    - A working compiler toolchain (VS2022, GCC5, etc.)

    Each test case verifies:
    1. FV INF file has correct ,XIP suffix on EFI_FILE_NAME entries
    2. FV map file shows correct rebase status (Fixed Flash Address)
    3. PE/COFF ImageBase in the FV binary matches expected value
    """

    WORKSPACE = None
    TOOLCHAIN = None
    BUILD_AVAILABLE = False

    # Module names in FV file order (matches INF listing in FDF_TEMPLATE)
    _MODULE_NAMES = ('TestPeim', 'TestPeim2', 'TestDxeDriver')

    # --- File content constants / templates ---

    DEC_CONTENT = """\
[Defines]
  DEC_SPECIFICATION = 0x00010005
  PACKAGE_NAME      = TestXipRebasePkg
  PACKAGE_GUID      = FC530350-34AA-4498-88F5-BF71987785B2
  PACKAGE_VERSION   = 1.0
"""

    PEIM_C_TEMPLATE = """\
#include <PiPei.h>
#include <Library/PeimEntryPoint.h>

EFI_STATUS
EFIAPI
{entry_point} (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{{
  return EFI_SUCCESS;
}}
"""

    PEIM_INF_TEMPLATE = """\
[Defines]
  INF_VERSION    = 0x00010005
  BASE_NAME      = {base_name}
  FILE_GUID      = {file_guid}
  MODULE_TYPE    = PEIM
  VERSION_STRING = 1.0
  ENTRY_POINT    = {entry_point}

[Sources]
  {source_file}

[Packages]
  MdePkg/MdePkg.dec

[LibraryClasses]
  PeimEntryPoint

[Depex]
  TRUE
"""

    DXE_C_SOURCE = """\
#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>

EFI_STATUS
EFIAPI
TestDxeDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
"""

    DXE_INF = """\
[Defines]
  INF_VERSION    = 0x00010005
  BASE_NAME      = TestDxeDriver
  FILE_GUID      = FEC0E1C9-544F-493E-9BD1-91263C7970FC
  MODULE_TYPE    = DXE_DRIVER
  VERSION_STRING = 1.0
  ENTRY_POINT    = TestDxeDriverEntry

[Sources]
  TestDxeDriver.c

[Packages]
  MdePkg/MdePkg.dec

[LibraryClasses]
  UefiDriverEntryPoint

[Depex]
  TRUE
"""

    DSC_CONTENT = """\
[Defines]
  PLATFORM_NAME           = TestXipRebase
  PLATFORM_GUID           = A44E9966-C1A1-47E8-8B21-8B773705DD79
  PLATFORM_VERSION        = 1.0
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/TestXipRebase
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = DEBUG
  SKUID_IDENTIFIER        = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf

[Components]
  TestXipRebasePkg/TestPeim/TestPeim.inf
  TestXipRebasePkg/TestPeim2/TestPeim2.inf
  TestXipRebasePkg/TestDxeDriver/TestDxeDriver.inf {
    <BuildOptions>
      MSFT:*_*_*_DLINK_FLAGS = /ALIGN:4096 /FILEALIGN:4096
      GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000
  }
"""

    PEIM2_RULE_TEMPLATE = """
[Rule.Common.PEIM.PEIM2RULE]
  FILE PEIM = $(NAMED_GUID) {{
    PE32  PE32  Align=Auto{peim2_xip_clause}  $(INF_OUTPUT)/$(MODULE_NAME).efi
  }}
"""

    FDF_TEMPLATE = """\

[FV.{fv_name}]
FvNameGuid = 15942B69-82DC-41DC-9F01-D60162870C4A
{base_line}\
{force_line}\
BlockSize = 0x10000
NumBlocks = 0x10
FvAlignment = 16
ERASE_POLARITY = 1
MEMORY_MAPPED = TRUE
STICKY_WRITE = TRUE
LOCK_CAP = TRUE
LOCK_STATUS = TRUE
WRITE_DISABLED_CAP = TRUE
WRITE_ENABLED_CAP = TRUE
WRITE_STATUS = TRUE
WRITE_LOCK_CAP = TRUE
WRITE_LOCK_STATUS = TRUE
READ_DISABLED_CAP = TRUE
READ_ENABLED_CAP = TRUE
READ_STATUS = TRUE
READ_LOCK_CAP = TRUE
READ_LOCK_STATUS = TRUE

INF  TestXipRebasePkg/TestPeim/TestPeim.inf
{peim2_inf_line}
INF  TestXipRebasePkg/TestDxeDriver/TestDxeDriver.inf

[Rule.Common.PEIM]
  FILE PEIM = $(NAMED_GUID) {{
    PE32  PE32  Align=Auto{peim1_xip_clause}  $(INF_OUTPUT)/$(MODULE_NAME).efi
  }}
{peim2_rule}\
[Rule.Common.DXE_DRIVER]
  FILE DRIVER = $(NAMED_GUID) {{
    PE32  PE32{dxe_xip_clause}  $(INF_OUTPUT)/$(MODULE_NAME).efi
  }}
"""

    @classmethod
    def _detect_toolchain(cls) -> str:
        """Get toolchain from --toolchain command line option (default: VS2022).

        Returns:
            Toolchain tag string (e.g. 'VS2022', 'GCC5').
        """
        for i, arg in enumerate(sys.argv):
            if arg == '--toolchain' and i + 1 < len(sys.argv):
                return sys.argv[i + 1]
            if arg.startswith('--toolchain='):
                return arg.split('=', 1)[1]
        return 'VS2022'

    @classmethod
    def setUpClass(cls) -> None:
        cls.WORKSPACE = os.environ.get('WORKSPACE')
        if cls.WORKSPACE is None:
            return
        cls.TOOLCHAIN = cls._detect_toolchain()
        try:
            result = subprocess.run(
                'build --version', capture_output=True, text=True,
                timeout=30, cwd=cls.WORKSPACE, shell=True
            )
            cls.BUILD_AVAILABLE = result.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass

    @classmethod
    def tearDownClass(cls) -> None:
        """Clean up generated test package directory."""
        if cls.WORKSPACE:
            pkg_dir = Path(cls.WORKSPACE, 'TestXipRebasePkg')
            if pkg_dir.is_dir():
                shutil.rmtree(pkg_dir, ignore_errors=True)

    def setUp(self) -> None:
        if self.WORKSPACE is None:
            self.fail("WORKSPACE environment variable is not set. "
                      "Run edksetup before running tests.")
        if not self.BUILD_AVAILABLE:
            self.fail("edk2 'build' command not found in PATH. "
                      "Run edksetup before running tests.")

    # --- Test package generation ---

    def _create_test_package(self) -> str:
        """Create a minimal test package with two PEIMs and one DXE driver.

        Returns:
            Absolute path to the created package directory.
        """
        pkg = Path(self.WORKSPACE, 'TestXipRebasePkg')
        pkg.mkdir(parents=True, exist_ok=True)
        (pkg / 'TestXipRebasePkg.dec').write_text(self.DEC_CONTENT)

        # Generate PEIM modules from template
        for name, guid, entry in [
            ('TestPeim',  'F43835C3-245F-4951-9D35-8684B98328DA', 'TestPeimEntry'),
            ('TestPeim2', '816C5A1F-23A8-485F-B005-D565FD01E303', 'TestPeim2Entry'),
        ]:
            mod = pkg / name
            mod.mkdir(parents=True, exist_ok=True)
            (mod / f'{name}.c').write_text(
                self.PEIM_C_TEMPLATE.format(entry_point=entry))
            (mod / f'{name}.inf').write_text(
                self.PEIM_INF_TEMPLATE.format(
                    base_name=name, file_guid=guid,
                    entry_point=entry, source_file=f'{name}.c'))

        # DXE driver (different includes/signature, not templated)
        dxe = pkg / 'TestDxeDriver'
        dxe.mkdir(parents=True, exist_ok=True)
        (dxe / 'TestDxeDriver.c').write_text(self.DXE_C_SOURCE)
        (dxe / 'TestDxeDriver.inf').write_text(self.DXE_INF)

        return str(pkg)

    def _generate_fdf(self, pkg_dir: str, fv_name: str,
                      base_address: str | None, force_rebase: str | None,
                      peim1_xip: str | None, peim2_xip: str | None,
                      dxe_xip: str | None) -> str:
        """Generate an FDF file with the specified FV settings.

        Args:
            pkg_dir: Package directory path.
            fv_name: Name for the firmware volume.
            base_address: Hex string (e.g. '0xFFF00000') or None.
            force_rebase: 'TRUE', 'FALSE', or None.
            peim1_xip: 'TRUE', 'FALSE', or None (omit Xip keyword).
            peim2_xip: 'TRUE', 'FALSE', or None.
            dxe_xip:   'TRUE', 'FALSE', or None.

        Returns:
            Absolute path to the generated FDF file.
        """
        def xip_clause(setting):
            return f'  Xip={setting}' if setting is not None else ''

        base_line = f'FvBaseAddress = {base_address}\n' if base_address is not None else ''
        force_line = f'FvForceRebase = {force_rebase}\n' if force_rebase is not None else ''

        # Use RuleOverride when PEIM2 needs a different Xip than PEIM1
        if peim2_xip != peim1_xip:
            peim2_inf_line = 'INF  RuleOverride=PEIM2RULE  TestXipRebasePkg/TestPeim2/TestPeim2.inf'
            peim2_rule = self.PEIM2_RULE_TEMPLATE.format(
                peim2_xip_clause=xip_clause(peim2_xip))
        else:
            peim2_inf_line = 'INF  TestXipRebasePkg/TestPeim2/TestPeim2.inf'
            peim2_rule = ''

        fdf_content = self.FDF_TEMPLATE.format(
            fv_name=fv_name, base_line=base_line, force_line=force_line,
            peim2_inf_line=peim2_inf_line,
            peim1_xip_clause=xip_clause(peim1_xip),
            peim2_rule=peim2_rule,
            dxe_xip_clause=xip_clause(dxe_xip),
        )

        fdf_path = Path(pkg_dir, 'TestXipRebase.fdf')
        fdf_path.write_text(fdf_content)
        return str(fdf_path)

    def _run_build(self, dsc_path: Path, fdf_path: str) -> tuple[int, str, str]:
        """Run the edk2 build command and return (returncode, stdout, stderr).

        When verbose mode is enabled (-v / --verbose), streams build output
        in real-time.

        Args:
            dsc_path: Path to the DSC platform description file.
            fdf_path: Path to the FDF flash description file.

        Returns:
            Tuple of (returncode, stdout, stderr) from the build process.
        """
        rel_dsc = os.path.relpath(dsc_path, self.WORKSPACE)
        rel_fdf = os.path.relpath(fdf_path, self.WORKSPACE)
        cmd = (
            f'build -p {rel_dsc} -f {rel_fdf}'
            f' -a X64 -b DEBUG -t {self.TOOLCHAIN} --quiet'
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
            t.join(timeout=300)
        proc.stdout.close()
        proc.stderr.close()
        proc.wait(timeout=300)
        return proc.returncode, ''.join(stdout_lines), ''.join(stderr_lines)

    # --- Verification helpers ---

    def _fv_output_path(self, filename: str) -> Path:
        """Return the path to a file in the FV output directory.

        Args:
            filename: Name of the file (e.g. 'TESTFV1.Fv', 'TESTFV1.inf').

        Returns:
            Full path to the file under Build/TestXipRebase/DEBUG_<TOOLCHAIN>/FV/.
        """
        return Path(
            self.WORKSPACE, 'Build', 'TestXipRebase',
            f'DEBUG_{self.TOOLCHAIN}', 'FV', filename
        )

    def _read_fv_file(self, filename: str) -> str | None:
        """Read a generated FV output file as text, or None if missing.

        Args:
            filename: Name of the file in the FV output directory.

        Returns:
            File contents as a string, or None if the file does not exist.
        """
        path = self._fv_output_path(filename)
        return path.read_text() if path.is_file() else None

    def _check_module_rebased(self, map_content: str, module_name: str,
                              expect_rebased: bool) -> None:
        """Assert a module's rebase status in the FV map file.

        A rebased module has '(Fixed Flash Address, BaseAddress=0x...' in
        its map entry. A non-rebased module lacks this marker.

        Args:
            map_content: Text content of the FV .map file.
            module_name: Module base name to search for (e.g. 'TestPeim').
            expect_rebased: True if the module should have been rebased.
        """
        self.assertIsNotNone(map_content, "FV map file not found")
        found = bool(re.search(
            rf'{re.escape(module_name)}.*\(Fixed Flash Address',
            map_content, re.IGNORECASE
        ))
        verb = "to be" if expect_rebased else "NOT to be"
        self.assertEqual(
            found, expect_rebased,
            f"Expected {module_name} {verb} rebased.\n"
            f"Map excerpt: {map_content[:500]}"
        )

    def _get_pe_image_bases(self, fv_name: str) -> list[tuple[int, int]] | None:
        """Extract (fv_offset, image_base) for each PE/COFF image in the FV.

        Walks the FV binary using ctypes structures:
        EFI_FIRMWARE_VOLUME_HEADER -> EFI_FFS_FILE_HEADER ->
        EFI_COMMON_SECTION_HEADER -> EFI_IMAGE_DOS_HEADER ->
        EFI_IMAGE_OPTIONAL_HEADER32 / EFI_IMAGE_OPTIONAL_HEADER64.

        Args:
            fv_name: Firmware volume name (e.g. 'TESTFV1').

        Returns:
            Sorted list of (fv_offset, image_base) tuples, or None if the
            FV file does not exist.
        """
        fv_path = self._fv_output_path(f'{fv_name}.Fv')
        if not fv_path.is_file():
            return None

        fv_data = fv_path.read_bytes()
        fv_hdr = EFI_FIRMWARE_VOLUME_HEADER.from_buffer_copy(fv_data)
        ffs_offset = fv_hdr.HeaderLength
        results = []

        # Walk FFS files within the FV
        while ffs_offset + ctypes.sizeof(EFI_FFS_FILE_HEADER) <= len(fv_data):
            ffs_offset = (ffs_offset + 7) & ~7  # FFS 8-byte alignment
            if ffs_offset + ctypes.sizeof(EFI_FFS_FILE_HEADER) > len(fv_data):
                break

            ffs_hdr = EFI_FFS_FILE_HEADER.from_buffer_copy(fv_data, ffs_offset)
            file_size = ffs_hdr.FFS_FILE_SIZE
            if file_size in (0, 0xFFFFFF):
                break  # End of FFS files or pad

            file_end = ffs_offset + file_size
            sect_offset = ffs_offset + ffs_hdr.HeaderLength

            # Walk sections within this FFS file
            while sect_offset + ctypes.sizeof(EFI_COMMON_SECTION_HEADER) <= file_end:
                sect_offset = (sect_offset + 3) & ~3  # Section 4-byte alignment
                if sect_offset + ctypes.sizeof(EFI_COMMON_SECTION_HEADER) > file_end:
                    break

                sect_hdr = EFI_COMMON_SECTION_HEADER.from_buffer_copy(
                    fv_data, sect_offset)
                sect_size = sect_hdr.SECTION_SIZE
                if sect_size == 0:
                    break

                if sect_hdr.Type == EFI_SECTION_PE32:
                    pe_offset = sect_offset + sect_hdr.Common_Header_Size()
                    image_base = self._parse_pe_image_base(
                        fv_data, pe_offset)
                    if image_base is not None:
                        results.append((pe_offset, image_base))

                sect_offset += sect_size

            ffs_offset += file_size

        results.sort(key=lambda x: x[0])
        return results

    @staticmethod
    def _parse_pe_image_base(data: bytes, offset: int) -> int | None:
        """Parse ImageBase from a PE/COFF image at the given offset.

        Uses EFI_IMAGE_DOS_HEADER to locate the PE signature, then reads
        EFI_IMAGE_NT_HEADERS32 or EFI_IMAGE_NT_HEADERS64 to extract ImageBase.

        Args:
            data: Raw bytes of the FV binary.
            offset: Byte offset where the PE/COFF image starts.

        Returns:
            ImageBase value (int), or None if the image cannot be parsed.
        """
        if offset + ctypes.sizeof(EFI_IMAGE_DOS_HEADER) > len(data):
            return None
        dos_hdr = EFI_IMAGE_DOS_HEADER.from_buffer_copy(data, offset)
        if dos_hdr.e_magic != EFI_IMAGE_DOS_SIGNATURE:
            return None

        nt_offset = offset + dos_hdr.e_lfanew

        # Read as NT_HEADERS32 first (smaller); check magic to decide format
        if nt_offset + ctypes.sizeof(EFI_IMAGE_NT_HEADERS32) > len(data):
            return None
        nt32 = EFI_IMAGE_NT_HEADERS32.from_buffer_copy(data, nt_offset)
        if nt32.Signature != EFI_IMAGE_NT_SIGNATURE:
            return None

        if nt32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            # PE32+: re-read with the larger NT_HEADERS64 structure
            if nt_offset + ctypes.sizeof(EFI_IMAGE_NT_HEADERS64) > len(data):
                return None
            nt64 = EFI_IMAGE_NT_HEADERS64.from_buffer_copy(data, nt_offset)
            return nt64.OptionalHeader.ImageBase
        return nt32.OptionalHeader.ImageBase

    def _check_pe_image_base(self, fv_name: str, base_address: str | None,
                             file_index: int, expect_rebased: bool) -> None:
        """Assert that a PE/COFF image in the FV has the correct ImageBase.

        A rebased image has ImageBase = FvBaseAddress + fv_offset.
        A non-rebased image retains its link-time ImageBase of 0.

        Args:
            fv_name: Firmware volume name (e.g. 'TESTFV1').
            base_address: FvBaseAddress hex string or None.
            file_index: Zero-based index of the PE image in FV file order.
            expect_rebased: True if the image should have been rebased.
        """
        pe_images = self._get_pe_image_bases(fv_name)
        self.assertIsNotNone(pe_images, f"Could not read FV for {fv_name}")
        self.assertGreater(
            len(pe_images), file_index,
            f"FV {fv_name} has {len(pe_images)} PE images, need >= {file_index + 1}")

        fv_offset, image_base = pe_images[file_index]
        fv_base = int(base_address, 16) if isinstance(base_address, str) else (base_address or 0)
        expected = (fv_base + fv_offset) if expect_rebased else 0

        self.assertEqual(
            image_base, expected,
            f"File #{file_index} @ FV+0x{fv_offset:X}: "
            f"ImageBase=0x{image_base:X}, expected 0x{expected:X}"
            f"{'' if expect_rebased else ' (not rebased)'}"
        )

    def _build_and_verify(self, fv_name: str, base_address: str | None,
                          force_rebase: str | None, peim1_xip: str | None,
                          peim2_xip: str | None, dxe_xip: str | None,
                          expect_rebase: tuple[bool, bool, bool]) -> None:
        """Build an FV with the given configuration and verify all outputs.

        Args:
            fv_name: Firmware volume name.
            base_address: FvBaseAddress hex string or None.
            force_rebase: FvForceRebase setting ('TRUE', 'FALSE', or None).
            peim1_xip: Xip= keyword for TestPeim ('TRUE', 'FALSE', or None).
            peim2_xip: Xip= keyword for TestPeim2 ('TRUE', 'FALSE', or None).
            dxe_xip: Xip= keyword for TestDxeDriver ('TRUE', 'FALSE', or None).
            expect_rebase: Tuple of 3 bools (peim1, peim2, dxe) indicating
                whether each module should be rebased.
        """
        pkg_dir = self._create_test_package()
        dsc_path = Path(pkg_dir, 'TestXipRebase.dsc')
        dsc_path.write_text(self.DSC_CONTENT)
        fdf_path = self._generate_fdf(
            pkg_dir, fv_name, base_address, force_rebase,
            peim1_xip, peim2_xip, dxe_xip)

        rc, stdout, stderr = self._run_build(dsc_path, fdf_path)
        self.assertEqual(rc, 0,
                         f"Build failed (rc={rc}).\nstdout:\n{stdout}\nstderr:\n{stderr}")

        # Verify ,XIP suffix count in FV INF file
        # (,XIP suffix is present when the FDF rule has Xip=TRUE)
        inf_content = self._read_fv_file(f'{fv_name}.inf')
        self.assertIsNotNone(inf_content, f"FV INF file not found for {fv_name}")
        efi_lines = [l for l in inf_content.splitlines() if 'EFI_FILE_NAME' in l]
        xip_count = sum(1 for l in efi_lines if l.rstrip().endswith(',XIP'))
        expected_xip = sum(x == 'TRUE' for x in (peim1_xip, peim2_xip, dxe_xip))
        self.assertEqual(xip_count, expected_xip,
                         f"Expected {expected_xip} ,XIP lines, got {xip_count}.\n"
                         f"INF content:\n{inf_content}")

        # Verify rebase status in map file and PE/COFF ImageBase in FV binary
        map_content = self._read_fv_file(f'{fv_name}.Fv.map')
        self.assertIsNotNone(map_content, f"FV map file not found for {fv_name}")

        for idx, (name, rebased) in enumerate(
                zip(self._MODULE_NAMES, expect_rebase)):
            self._check_module_rebased(map_content, name, rebased)
            self._check_pe_image_base(fv_name, base_address, idx, rebased)

    # ===================================================================
    # Test case table (matches behavior matrix from test plan header)
    # ===================================================================

    # (fv_name, base_address, force_rebase,
    #  peim1_xip, peim2_xip, dxe_xip,
    #  (expect_peim1_rebase, expect_peim2_rebase, expect_dxe_rebase),
    #  description)
    TEST_CASES = [
        # TC1: ForceRebase not specified, BaseAddress defaults to 0.
        # Early return path: (BaseAddress==0 && ForceRebase==-1).
        # No files are rebased. PEIMs have ,XIP suffix (Xip=TRUE in rule).
        ('TESTFV1', None,         None,    'TRUE', 'TRUE', None,
         (False, False, False), 'TC1: ForceRebase=unset, Base=0 -> no rebase'),
        # TC2: ForceRebase not specified, BaseAddress!=0.
        # Legacy path: (BaseAddress!=0 && ForceRebase==-1).
        # ALL files are rebased regardless of XIP status.
        ('TESTFV2', '0x00800000', None,    'TRUE', 'TRUE', None,
         (True,  True,  True),  'TC2: ForceRebase=unset, Base!=0 -> rebase all (legacy)'),
        # TC3: ForceRebase=FALSE with BaseAddress!=0.
        # Early return path: (ForceRebase==0).
        # No files are rebased even though all have Xip=TRUE.
        ('TESTFV3', '0x00800000', 'FALSE', 'TRUE', 'TRUE', 'TRUE',
         (False, False, False), 'TC3: ForceRebase=FALSE -> no rebase'),
        # TC4: ForceRebase=TRUE, all three files have Xip=TRUE.
        # All files match (ForceRebase==1 && XipFile[i]==TRUE) and are rebased.
        ('TESTFV4', '0x00800000', 'TRUE',  'TRUE', 'TRUE', 'TRUE',
         (True,  True,  True),  'TC4: ForceRebase=TRUE, all Xip=TRUE -> rebase all'),
        # TC5: ForceRebase=TRUE, PEIMs have Xip=TRUE, DXE has no Xip.
        # Selective rebase: PEIMs rebased, DXE skipped.
        ('TESTFV5', '0x00800000', 'TRUE',  'TRUE', 'TRUE', None,
         (True,  True,  False), 'TC5: ForceRebase=TRUE, selective Xip -> rebase Xip only'),
        # TC6: ForceRebase=TRUE, no files have Xip keyword.
        # All files have XipFile==FALSE, so none are rebased.
        ('TESTFV6', '0x00800000', 'TRUE',  None,   None,   None,
         (False, False, False), 'TC6: ForceRebase=TRUE, no Xip -> no rebase'),
        # TC7: ForceRebase=TRUE, PEIM1 has Xip=TRUE, PEIM2 has Xip=FALSE.
        # Mixed XIP within same module type via RuleOverride.
        # Only PEIM1 is rebased; PEIM2 and DXE are skipped.
        ('TESTFV7', '0x00800000', 'TRUE',  'TRUE', 'FALSE', None,
         (True,  False, False), 'TC7: ForceRebase=TRUE, mixed Xip -> rebase Xip=TRUE only'),
        # TC8: ForceRebase=TRUE, BaseAddress=0, PEIMs have Xip=TRUE.
        # ForceRebase=TRUE overrides the (BaseAddress==0) early return.
        # PEIMs are rebased to offset 0+fv_offset; DXE is skipped (no Xip).
        ('TESTFV8', '0x0',        'TRUE',  'TRUE', 'TRUE', None,
         (True,  True,  False), 'TC8: ForceRebase=TRUE, Base=0, Xip -> rebase (force overrides)'),
    ]

    def test_xip_rebase_behavior(self) -> None:
        """Parameterized test covering all ForceRebase/BaseAddress/Xip combos."""
        for (fv_name, base_address, force_rebase,
             peim1_xip, peim2_xip, dxe_xip,
             expect_rebase, description) in self.TEST_CASES:
            with self.subTest(description):
                self._build_and_verify(
                    fv_name, base_address, force_rebase,
                    peim1_xip, peim2_xip, dxe_xip, expect_rebase)


class TestFdfParserPe32KeywordOrder(unittest.TestCase):
    """Test that the FDF parser accepts PE32 section keywords in any order.

    The keywords Align, Xip, and RELOCS_STRIPPED/RELOCS_RETAINED should be
    accepted in any permutation within a PE32 section statement in a [Rule].
    """

    @classmethod
    def setUpClass(cls):
        """Set up environment for FdfParser imports."""
        import tempfile
        cls._tmpdir = tempfile.mkdtemp(prefix='fdf_parser_test_')
        # Set WORKSPACE so the parser doesn't crash
        os.environ.setdefault('WORKSPACE', cls._tmpdir)
        from GenFds.GenFdsGlobalVariable import GenFdsGlobalVariable
        GenFdsGlobalVariable.WorkSpaceDir = cls._tmpdir
        from Common import GlobalData
        GlobalData.gFdfParser = None
        GlobalData.gWorkspace = cls._tmpdir

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls._tmpdir, ignore_errors=True)

    def _parse_rule(self, pe32_section_line):
        """Parse a [Rule] with the given PE32 section line and return the EfiSection."""
        import tempfile
        from GenFds.FdfParser import FdfParser
        from Common import GlobalData

        fdf_content = (
            "[Rule.Common.PEIM]\n"
            "  FILE PEIM = $(NAMED_GUID) {\n"
            "    " + pe32_section_line + "\n"
            "  }\n"
        )
        fdf_path = os.path.join(self._tmpdir, 'test_order.fdf')
        with open(fdf_path, 'w') as f:
            f.write(fdf_content)

        parser = FdfParser(fdf_path)
        # Manually set up parser state for rule parsing
        parser.CurrentLineNumber = 3
        parser.CurrentOffsetWithinLine = 4
        # Re-read the profile to refresh file lines
        parser.Profile.FileLinesList = fdf_content.splitlines(True)

        # Create a RuleComplexFile as the container object
        from GenFds.RuleComplexFile import RuleComplexFile
        obj = RuleComplexFile()
        obj.FvFileType = 'PEIM'
        obj.KeepReloc = None
        obj.SectionList = []

        result = parser._GetEfiSection(obj)
        self.assertTrue(result, f"Parser failed to parse: {pe32_section_line}")
        self.assertEqual(len(obj.SectionList), 1)
        return obj.SectionList[0]

    # (pe32_line, expected_alignment, expected_xip, expected_keep_reloc, description)
    TEST_CASES = [
        # Align before Xip (original supported order)
        ('PE32 PE32 Align=8 Xip=TRUE',
         '8', 'TRUE', None,
         'Align then Xip'),
        # Xip before Align (previously caused stack trace)
        ('PE32 PE32 Xip=TRUE Align=8',
         '8', 'TRUE', None,
         'Xip then Align'),
        # Align before RELOCS_STRIPPED
        ('PE32 PE32 Align=16 RELOCS_STRIPPED',
         '16', None, False,
         'Align then RELOCS_STRIPPED'),
        # RELOCS_STRIPPED before Align
        ('PE32 PE32 RELOCS_STRIPPED Align=16',
         '16', None, False,
         'RELOCS_STRIPPED then Align'),
        # Xip before RELOCS_STRIPPED
        ('PE32 PE32 Xip=TRUE RELOCS_STRIPPED',
         None, 'TRUE', False,
         'Xip then RELOCS_STRIPPED'),
        # RELOCS_STRIPPED before Xip
        ('PE32 PE32 RELOCS_STRIPPED Xip=TRUE',
         None, 'TRUE', False,
         'RELOCS_STRIPPED then Xip'),
        # All three: Align, Xip, RELOCS_STRIPPED
        ('PE32 PE32 Align=16 Xip=TRUE RELOCS_STRIPPED',
         '16', 'TRUE', False,
         'Align then Xip then RELOCS_STRIPPED'),
        # All three: Xip, Align, RELOCS_STRIPPED
        ('PE32 PE32 Xip=TRUE Align=16 RELOCS_STRIPPED',
         '16', 'TRUE', False,
         'Xip then Align then RELOCS_STRIPPED'),
        # All three: RELOCS_STRIPPED, Align, Xip
        ('PE32 PE32 RELOCS_STRIPPED Align=16 Xip=TRUE',
         '16', 'TRUE', False,
         'RELOCS_STRIPPED then Align then Xip'),
        # All three: RELOCS_STRIPPED, Xip, Align
        ('PE32 PE32 RELOCS_STRIPPED Xip=TRUE Align=16',
         '16', 'TRUE', False,
         'RELOCS_STRIPPED then Xip then Align'),
        # All three: Xip, RELOCS_STRIPPED, Align
        ('PE32 PE32 Xip=TRUE RELOCS_STRIPPED Align=16',
         '16', 'TRUE', False,
         'Xip then RELOCS_STRIPPED then Align'),
        # All three: Align, RELOCS_STRIPPED, Xip
        ('PE32 PE32 Align=16 RELOCS_STRIPPED Xip=TRUE',
         '16', 'TRUE', False,
         'Align then RELOCS_STRIPPED then Xip'),
        # Xip=FALSE
        ('PE32 PE32 Xip=FALSE Align=8',
         '8', 'FALSE', None,
         'Xip=FALSE then Align'),
        # RELOCS_RETAINED variant
        ('PE32 PE32 Xip=TRUE RELOCS_RETAINED Align=8',
         '8', 'TRUE', True,
         'Xip then RELOCS_RETAINED then Align'),
        # Only Xip (no Align, no Reloc)
        ('PE32 PE32 Xip=TRUE',
         None, 'TRUE', None,
         'Xip only'),
        # Only Align (no Xip, no Reloc)
        ('PE32 PE32 Align=32',
         '32', None, None,
         'Align only'),
        # Only RELOCS_STRIPPED (no Align, no Xip)
        ('PE32 PE32 RELOCS_STRIPPED',
         None, None, False,
         'RELOCS_STRIPPED only'),
    ]

    def test_pe32_keyword_order(self) -> None:
        """Parameterized test verifying PE32 section keywords in any order."""
        for (pe32_line, exp_align, exp_xip, exp_keep_reloc, desc) in self.TEST_CASES:
            with self.subTest(desc):
                section = self._parse_rule(pe32_line)
                self.assertEqual(section.SectionType, 'PE32')
                if exp_align is not None:
                    self.assertEqual(section.Alignment, exp_align)
                else:
                    self.assertIn(section.Alignment, (None, ''))
                if exp_xip is not None:
                    self.assertEqual(section.Xip, exp_xip)
                else:
                    self.assertFalse(
                        hasattr(section, 'Xip') and section.Xip,
                        f"Expected no Xip but got {getattr(section, 'Xip', None)}"
                    )
                if exp_keep_reloc is not None:
                    self.assertEqual(section.KeepReloc, exp_keep_reloc)
                else:
                    self.assertIsNone(
                        getattr(section, 'KeepReloc', None),
                        f"Expected no KeepReloc but got {section.KeepReloc}"
                    )


class TestFdfParserFvKeywordOrder(unittest.TestCase):
    """Test that the FDF parser accepts [FV] keywords in any order.

    FvForceRebase, FvBaseAddress, FvAlignment, and FV attributes like
    ERASE_POLARITY, MEMORY_MAPPED should be accepted in any order.
    Previously, FvForceRebase between two FV attributes (e.g. between
    ERASE_POLARITY and MEMORY_MAPPED) caused a Python stack trace.
    """

    @classmethod
    def setUpClass(cls):
        """Set up environment for FdfParser imports."""
        import tempfile
        cls._tmpdir = tempfile.mkdtemp(prefix='fdf_fv_parser_test_')
        os.environ.setdefault('WORKSPACE', cls._tmpdir)
        from GenFds.GenFdsGlobalVariable import GenFdsGlobalVariable
        GenFdsGlobalVariable.WorkSpaceDir = cls._tmpdir
        from Common import GlobalData
        GlobalData.gFdfParser = None
        GlobalData.gWorkspace = cls._tmpdir

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls._tmpdir, ignore_errors=True)

    def _parse_fv_section(self, fv_body):
        """Parse an [FV] section and return the FV object."""
        from GenFds.FdfParser import FdfParser
        from Common import GlobalData

        fdf_content = (
            "[FV.TESTFV]\n"
            + fv_body + "\n"
        )
        fdf_path = os.path.join(self._tmpdir, 'test_fv_order.fdf')
        with open(fdf_path, 'w') as f:
            f.write(fdf_content)

        parser = FdfParser(fdf_path)
        parser.Profile.FileLinesList = fdf_content.splitlines(True)
        # Position parser at start of FV body (line 2, offset 0)
        parser.CurrentLineNumber = 2
        parser.CurrentOffsetWithinLine = 0

        # Create FV object and parse the attributes/keywords
        from GenFds.Fv import FV
        fv_obj = FV(Name='TESTFV')

        # Use the same while loop the real parser uses
        while True:
            parser._GetSetStatements(fv_obj)
            if not (parser._GetBlockStatement(fv_obj) or
                    parser._GetFvBaseAddress(fv_obj) or
                    parser._GetFvForceRebase(fv_obj) or
                    parser._GetFvAlignment(fv_obj) or
                    parser._GetFvAttributes(fv_obj) or
                    parser._GetFvNameGuid(fv_obj) or
                    parser._GetFvExtEntryStatement(fv_obj) or
                    parser._GetFvNameString(fv_obj)):
                break

        return fv_obj

    # (fv_body, expected_attrs, expected_force_rebase, description)
    TEST_CASES = [
        # FvForceRebase after all attributes (original working order)
        ("ERASE_POLARITY = 1\nMEMORY_MAPPED = TRUE\nFvForceRebase = TRUE\n",
         {'ERASE_POLARITY': '1', 'MEMORY_MAPPED': 'TRUE'}, True,
         'FvForceRebase after all attributes'),
        # FvForceRebase before all attributes
        ("FvForceRebase = TRUE\nERASE_POLARITY = 1\nMEMORY_MAPPED = TRUE\n",
         {'ERASE_POLARITY': '1', 'MEMORY_MAPPED': 'TRUE'}, True,
         'FvForceRebase before all attributes'),
        # FvForceRebase between ERASE_POLARITY and MEMORY_MAPPED
        ("ERASE_POLARITY = 1\nFvForceRebase = TRUE\nMEMORY_MAPPED = TRUE\n",
         {'ERASE_POLARITY': '1', 'MEMORY_MAPPED': 'TRUE'}, True,
         'FvForceRebase between attributes (previously crashed)'),
        # FvForceRebase=FALSE between attributes
        ("ERASE_POLARITY = 1\nFvForceRebase = FALSE\nMEMORY_MAPPED = TRUE\n",
         {'ERASE_POLARITY': '1', 'MEMORY_MAPPED': 'TRUE'}, False,
         'FvForceRebase=FALSE between attributes'),
        # Multiple attributes, FvForceRebase in the middle
        ("ERASE_POLARITY = 1\nSTICKY_WRITE = TRUE\nFvForceRebase = TRUE\n"
         "MEMORY_MAPPED = TRUE\nLOCK_CAP = TRUE\n",
         {'ERASE_POLARITY': '1', 'STICKY_WRITE': 'TRUE',
          'MEMORY_MAPPED': 'TRUE', 'LOCK_CAP': 'TRUE'}, True,
         'FvForceRebase in middle of many attributes'),
        # FvBaseAddress between attributes
        ("ERASE_POLARITY = 1\nFvBaseAddress = 0x00800000\nMEMORY_MAPPED = TRUE\n",
         {'ERASE_POLARITY': '1', 'MEMORY_MAPPED': 'TRUE'}, None,
         'FvBaseAddress between attributes'),
        # FvAlignment between attributes
        ("ERASE_POLARITY = 1\nFvAlignment = 16\nMEMORY_MAPPED = TRUE\n",
         {'ERASE_POLARITY': '1', 'MEMORY_MAPPED': 'TRUE'}, None,
         'FvAlignment between attributes'),
        # All interleaved: attr, FvForceRebase, attr, FvBaseAddress, attr
        ("ERASE_POLARITY = 1\nFvForceRebase = TRUE\nSTICKY_WRITE = TRUE\n"
         "FvBaseAddress = 0x00800000\nMEMORY_MAPPED = TRUE\n",
         {'ERASE_POLARITY': '1', 'STICKY_WRITE': 'TRUE', 'MEMORY_MAPPED': 'TRUE'}, True,
         'Multiple keywords interleaved with attributes'),
    ]

    def test_fv_keyword_order(self) -> None:
        """Parameterized test verifying FV keywords accepted in any order."""
        for (fv_body, exp_attrs, exp_force_rebase, desc) in self.TEST_CASES:
            with self.subTest(desc):
                fv_obj = self._parse_fv_section(fv_body)
                for attr_name, attr_val in exp_attrs.items():
                    self.assertIn(attr_name, fv_obj.FvAttributeDict,
                                  f"Missing attribute {attr_name}")
                    self.assertEqual(fv_obj.FvAttributeDict[attr_name], attr_val)
                if exp_force_rebase is not None:
                    self.assertEqual(fv_obj.FvForceRebase, exp_force_rebase)


if __name__ == '__main__':
    unittest.main()
