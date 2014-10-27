#
#  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

# Define the following variables to specify an alternative toolchain to the one located in your PATH:
# - RVCT_TOOLS_PATH: for RVCT and RVCTLINUX toolchains
# - ARMGCC_TOOLS_PATH: for ARMGCC toolchain
# - ARMLINUXGCC_TOOLS_PATH: for ARMLINUXGCC

EDK2_TOOLCHAIN ?= GCC49
GCC49_AARCH64_PREFIX ?= aarch64-none-elf-
EDK2_ARCH ?= AARCH64
EDK2_BUILD ?= DEBUG
EDK2_DSC = ArmPlatformPkg/ArmJunoPkg/ArmJuno.dsc
DEST_BIN_ROOT ?=

ifeq ($(EDK2_DSC),"")
  $(error The Makefile macro 'EDK2_DSC' must be defined with an EDK2 DSC file.)
endif

ifeq ("$(OS)","Windows_NT")
export WORKSPACE?=$(PWD)
export EDK_TOOLS_PATH ?= $(WORKSPACE)\BaseTools
else
export WORKSPACE?=$(PWD)
endif

# Define the destination of the Firmware Image Package (FIP) if not defined
ifndef JUNO_FIP
  ifdef DEST_BIN_ROOT
    JUNO_FIP=$(DEST_BIN_ROOT)/fip.bin
  else
    JUNO_FIP=fip.bin
  endif
endif

SHELL := /bin/bash
SILENT ?= @
ECHO ?= echo
MAKE ?= make -i -k
RM ?= rm -f
CP ?= cp

.PHONY: all clean

EDK2_CONF = Conf/BuildEnv.sh Conf/build_rule.txt Conf/target.txt Conf/tools_def.txt

all: $(EDK2_CONF)
ifeq ("$(OS)","Windows_NT")
	build -a $(EDK2_ARCH) -p $(EDK2_DSC) -t $(EDK2_TOOLCHAIN) -b $(EDK2_BUILD) $(EDK2_MACROS)
else
	. ./edksetup.sh; GCC49_AARCH64_PREFIX=$(GCC49_AARCH64_PREFIX) build -a $(EDK2_ARCH) -p $(EDK2_DSC) -t $(EDK2_TOOLCHAIN) -b $(EDK2_BUILD) $(EDK2_MACROS)
endif
ifeq ("$(OS)","Windows_NT")
	$(SILENT)$(ECHO) "Warning: The UEFI Firmware must be added to the Firmware Image Package (FIP)."
else
	$(SILENT)which fip_create ; \
	if [ $$? -ne 0 ]; then \
		$(ECHO) "Warning: 'fip_create' tool is not in the PATH. The UEFI binary will not be added in the Firmware Image Package (FIP)."; \
	else \
		fip_create --bl33 $(WORKSPACE)/Build/ArmJuno/$(EDK2_BUILD)_$(EDK2_TOOLCHAIN)/FV/BL33_AP_UEFI.fd --dump $(JUNO_FIP); \
	fi
endif

$(EDK2_CONF):
ifeq ("$(OS)","Windows_NT")
	copy $(EDK_TOOLS_PATH)\Conf\build_rule.template Conf\build_rule.txt
	copy $(EDK_TOOLS_PATH)\Conf\FrameworkDatabase.template Conf\FrameworkDatabase.txt
	copy $(EDK_TOOLS_PATH)\Conf\target.template Conf\target.txt
	copy $(EDK_TOOLS_PATH)\Conf\tools_def.template Conf\tools_def.txt
else
	. ./edksetup.sh; $(MAKE) -C BaseTools
endif

clean:
ifeq ("$(OS)","Windows_NT")
	build -a $(EDK2_ARCH) -p $(EDK2_DSC) -t $(EDK2_TOOLCHAIN) -b $(EDK2_BUILD) $(EDK2_MACROS) cleanall
else
	. ./edksetup.sh; build -a $(EDK2_ARCH) -p $(EDK2_DSC) -t $(EDK2_TOOLCHAIN) -b $(EDK2_BUILD) $(EDK2_MACROS) cleanall; \
	rm -Rf $(EDK2_CONF) Conf/.cache
endif
