# @file
#
# Copyright (c) Microsoft Corporation.
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
# Copyright (c) 2020 - 2021, ARM Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
import sys
from edk2toolext.environment import shell_environment
from edk2toolext.invocables.edk2_ci_build import CiBuildSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from pathlib import Path


try:
    # Temporarily needed until edk2 can update to the latest edk2-pytools
    # that has the CodeQL helpers.
    #
    # May not be present until submodules are populated.
    #
    root = Path(__file__).parent.parent.resolve()
    sys.path.append(str(root/'BaseTools'/'Plugin'/'CodeQL'/'integration'))
    import stuart_codeql as codeql_helpers
except ImportError:
    pass


class Settings(CiBuildSettingsManager, UpdateSettingsManager, SetupSettingsManager, PrEvalSettingsManager):

    def __init__(self):
        self.ActualPackages = []
        self.ActualTargets = []
        self.ActualArchitectures = []
        self.ActualToolChainTag = ""
        self.ActualScopes = None

    # ####################################################################################### #
    #                             Extra CmdLine configuration                                 #
    # ####################################################################################### #

    def AddCommandLineOptions(self, parserObj):
        try:
            codeql_helpers.add_command_line_option(parserObj)
        except NameError:
            pass

    def RetrieveCommandLineOptions(self, args):
        super().RetrieveCommandLineOptions(args)

        try:
            self.codeql = codeql_helpers.is_codeql_enabled_on_command_line(args)
        except NameError:
            pass

    # ####################################################################################### #
    #                        Default Support for this Ci Build                                #
    # ####################################################################################### #

    def GetPackagesSupported(self):
        ''' return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths '''

        return ("ArmPkg",
                "ArmPlatformPkg",
                "ArmVirtPkg",
                "DynamicTablesPkg",
                "EmbeddedPkg",
                "EmulatorPkg",
                "IntelFsp2Pkg",
                "IntelFsp2WrapperPkg",
                "MdePkg",
                "MdeModulePkg",
                "NetworkPkg",
                "PcAtChipsetPkg",
                "SecurityPkg",
                "UefiCpuPkg",
                "FmpDevicePkg",
                "ShellPkg",
                "SignedCapsulePkg",
                "StandaloneMmPkg",
                "FatPkg",
                "CryptoPkg",
                "PrmPkg",
                "UnitTestFrameworkPkg",
                "OvmfPkg",
                "RedfishPkg",
                "SourceLevelDebugPkg",
                "UefiPayloadPkg"
                )

    def GetArchitecturesSupported(self):
        ''' return iterable of edk2 architectures supported by this build '''
        return (
                "IA32",
                "X64",
                "ARM",
                "AARCH64",
                "RISCV64",
                "LOONGARCH64")

    def GetTargetsSupported(self):
        ''' return iterable of edk2 target tags supported by this build '''
        return ("DEBUG", "RELEASE", "NO-TARGET", "NOOPT")

    # ####################################################################################### #
    #                     Verify and Save requested Ci Build Config                           #
    # ####################################################################################### #

    def SetPackages(self, list_of_requested_packages):
        ''' Confirm the requested package list is valid and configure SettingsManager
        to build the requested packages.

        Raise UnsupportedException if a requested_package is not supported
        '''
        unsupported = set(list_of_requested_packages) - \
            set(self.GetPackagesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Package Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Package Requested: " +
                            " ".join(unsupported))
        self.ActualPackages = list_of_requested_packages

    def SetArchitectures(self, list_of_requested_architectures):
        ''' Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise Exception if a list_of_requested_architectures is not supported
        '''
        unsupported = set(list_of_requested_architectures) - \
            set(self.GetArchitecturesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
            raise Exception(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
        self.ActualArchitectures = list_of_requested_architectures

    def SetTargets(self, list_of_requested_target):
        ''' Confirm the request target list is valid and configure SettingsManager
        to run only the requested targets.

        Raise UnsupportedException if a requested_target is not supported
        '''
        unsupported = set(list_of_requested_target) - \
            set(self.GetTargetsSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Targets Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Targets Requested: " +
                            " ".join(unsupported))
        self.ActualTargets = list_of_requested_target

    # ####################################################################################### #
    #                         Actual Configuration for Ci Build                               #
    # ####################################################################################### #

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        if self.ActualScopes is None:
            scopes = ("cibuild", "edk2-build", "host-based-test")

            self.ActualToolChainTag = shell_environment.GetBuildVars().GetValue("TOOL_CHAIN_TAG", "")

            try:
                scopes += codeql_helpers.get_scopes(self.codeql)

                if self.codeql:
                    shell_environment.GetBuildVars().SetValue(
                        "STUART_CODEQL_AUDIT_ONLY",
                        "TRUE",
                        "Set in CISettings.py")
                    shell_environment.GetBuildVars().SetValue(
                        "STUART_CODEQL_FILTER_FILES",
                        os.path.join(self.GetWorkspaceRoot(),
                                     "CodeQlFilters.yml"),
                        "Set in CISettings.py")
            except NameError:
                pass

            self.ActualScopes = scopes
        return self.ActualScopes

    def GetRequiredSubmodules(self):
        ''' return iterable containing RequiredSubmodule objects.
        If no RequiredSubmodules return an empty iterable
        '''
        rs = []
        rs.append(RequiredSubmodule(
            "CryptoPkg/Library/OpensslLib/openssl", False))
        rs.append(RequiredSubmodule(
            "UnitTestFrameworkPkg/Library/CmockaLib/cmocka", False))
        rs.append(RequiredSubmodule(
            "UnitTestFrameworkPkg/Library/GoogleTestLib/googletest", False))
        rs.append(RequiredSubmodule(
            "MdeModulePkg/Universal/RegularExpressionDxe/oniguruma", False))
        rs.append(RequiredSubmodule(
            "MdeModulePkg/Library/BrotliCustomDecompressLib/brotli", False))
        rs.append(RequiredSubmodule(
            "BaseTools/Source/C/BrotliCompress/brotli", False))
        rs.append(RequiredSubmodule(
            "RedfishPkg/Library/JsonLib/jansson", False))
        rs.append(RequiredSubmodule(
            "UnitTestFrameworkPkg/Library/SubhookLib/subhook", False))
        rs.append(RequiredSubmodule(
            "MdePkg/Library/BaseFdtLib/libfdt", False))
        rs.append(RequiredSubmodule(
            "MdePkg/Library/MipiSysTLib/mipisyst", False))
        rs.append(RequiredSubmodule(
            "CryptoPkg/Library/MbedTlsLib/mbedtls", False))
        rs.append(RequiredSubmodule(
            "SecurityPkg/DeviceSecurity/SpdmLib/libspdm", False))
        return rs

    def GetName(self):
        return "Edk2"

    def GetDependencies(self):
        return [
        ]

    def GetPackagesPath(self):
        return ()

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    def FilterPackagesToTest(self, changedFilesList: list, potentialPackagesList: list) -> list:
        ''' Filter potential packages to test based on changed files. '''
        build_these_packages = []
        possible_packages = potentialPackagesList.copy()
        for f in changedFilesList:
            # split each part of path for comparison later
            nodes = f.split("/")

            # python file change in .pytool folder causes building all
            if f.endswith(".py") and ".pytool" in nodes:
                build_these_packages = possible_packages
                break

            # BaseTools files that might change the build
            if "BaseTools" in nodes:
                if os.path.splitext(f) not in [".txt", ".md"]:
                    build_these_packages = possible_packages
                    break
        return build_these_packages
