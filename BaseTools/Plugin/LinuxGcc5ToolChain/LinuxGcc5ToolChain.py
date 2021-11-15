# @file LinuxGcc5ToolChain.py
# Plugin to configures paths for GCC5 ARM/AARCH64 Toolchain
##
# This plugin works in conjuncture with the tools_def
#
# Copyright (c) Microsoft Corporation
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
# Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext.environment import shell_environment


class LinuxGcc5ToolChain(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        return 0

    def do_pre_build(self, thebuilder):
        self.Logger = logging.getLogger("LinuxGcc5ToolChain")

        #
        # GCC5 - The ARM and AARCH64 compilers need their paths set if available
        if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "GCC5":

            # Start with AARACH64 compiler
            ret = self._check_aarch64()
            if ret != 0:
                self.Logger.critical("Failed in check aarch64")
                return ret

            # Check arm compiler
            ret = self._check_arm()
            if ret != 0:
                self.Logger.critical("Failed in check arm")
                return ret

            # Check RISCV64 compiler
            ret = self._check_riscv64()
            if ret != 0:
                self.Logger.critical("Failed in check riscv64")
                return ret

            # Check LoongArch64 compiler
            ret = self._check_loongarch64()
            if ret != 0:
                self.Logger.critical("Failed in check loongarch64")
                return ret

        return 0

    def _check_arm(self):
        # check to see if full path already configured
        if shell_environment.GetEnvironment().get_shell_var("GCC5_ARM_PREFIX") is not None:
            self.Logger.info("GCC5_ARM_PREFIX is already set.")

        else:
            # now check for install dir.  If set then set the Prefix
            install_path = shell_environment.GetEnvironment().get_shell_var("GCC5_ARM_INSTALL")
            if install_path is None:
                return 0

            # make GCC5_ARM_PREFIX to align with tools_def.txt
            prefix = os.path.join(install_path, "bin", "arm-none-linux-gnueabihf-")
            shell_environment.GetEnvironment().set_shell_var("GCC5_ARM_PREFIX", prefix)

        # now confirm it exists
        if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("GCC5_ARM_PREFIX") + "gcc"):
            self.Logger.error("Path for GCC5_ARM_PREFIX toolchain is invalid")
            return -2

        return 0

    def _check_aarch64(self):
        # check to see if full path already configured
        if shell_environment.GetEnvironment().get_shell_var("GCC5_AARCH64_PREFIX") is not None:
            self.Logger.info("GCC5_AARCH64_PREFIX is already set.")

        else:
            # now check for install dir.  If set then set the Prefix
            install_path = shell_environment.GetEnvironment(
            ).get_shell_var("GCC5_AARCH64_INSTALL")
            if install_path is None:
                return 0

            # make GCC5_AARCH64_PREFIX to align with tools_def.txt
            prefix = os.path.join(install_path, "bin", "aarch64-none-linux-gnu-")
            shell_environment.GetEnvironment().set_shell_var("GCC5_AARCH64_PREFIX", prefix)

        # now confirm it exists
        if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("GCC5_AARCH64_PREFIX") + "gcc"):
            self.Logger.error(
                "Path for GCC5_AARCH64_PREFIX toolchain is invalid")
            return -2

        return 0

    def _check_riscv64(self):
        # now check for install dir.  If set then set the Prefix
        install_path = shell_environment.GetEnvironment(
        ).get_shell_var("GCC5_RISCV64_INSTALL")
        if install_path is None:
            return 0

        # check to see if full path already configured
        if shell_environment.GetEnvironment().get_shell_var("GCC5_RISCV64_PREFIX") is not None:
            self.Logger.info("GCC5_RISCV64_PREFIX is already set.")

        else:
            # make GCC5_RISCV64_PREFIX to align with tools_def.txt
            prefix = os.path.join(install_path, "bin", "riscv64-unknown-elf-")
            shell_environment.GetEnvironment().set_shell_var("GCC5_RISCV64_PREFIX", prefix)

        # now confirm it exists
        if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("GCC5_RISCV64_PREFIX") + "gcc"):
            self.Logger.error(
                "Path for GCC5_RISCV64_PREFIX toolchain is invalid")
            return -2

        # Check if LD_LIBRARY_PATH is set for the libraries of RISC-V GCC toolchain
        if shell_environment.GetEnvironment().get_shell_var("LD_LIBRARY_PATH") is not None:
            self.Logger.info("LD_LIBRARY_PATH is already set.")

        prefix = os.path.join(install_path, "lib")
        shell_environment.GetEnvironment().set_shell_var("LD_LIBRARY_PATH", prefix)

        return 0

    def _check_loongarch64(self):
        # check to see if full path already configured
        if shell_environment.GetEnvironment().get_shell_var("GCC5_LOONGARCH64_PREFIX") is not None:
            self.Logger.info("GCC5_LOONGARCH64_PREFIX is already set.")

        else:
            # now check for install dir.  If set then set the Prefix
            install_path = shell_environment.GetEnvironment(
            ).get_shell_var("GCC5_LOONGARCH64_INSTALL")
            if install_path is None:
                return 0

            # make GCC5_LOONGARCH64_PREFIX to align with tools_def.txt
            prefix = os.path.join(install_path, "bin", "loongarch64-unknown-linux-gnu-")
            shell_environment.GetEnvironment().set_shell_var("GCC5_LOONGARCH64_PREFIX", prefix)

        # now confirm it exists
        if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("GCC5_LOONGARCH64_PREFIX") + "gcc"):
            self.Logger.error(
                "Path for GCC5_LOONGARCH64_PREFIX toolchain is invalid")
            return -2

        return 0
