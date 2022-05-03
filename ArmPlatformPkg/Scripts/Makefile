#/* @file
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#*/

EDK2_TOOLCHAIN ?= GCC5
EDK2_ARCH ?= ARM
EDK2_BUILD ?= DEBUG

ifndef EDK2_DSC
  $(error The Makfile macro 'EDK2_DSC' must be defined with an EDK2 DSC file.)
endif
ifeq ("$(EDK2_DSC)","ArmPlatformPkg/ArmVExpressPkg/ArmVExpress-FVP-AArch64.dsc")
  BUILD_FIP=1
  BUILD_PATH=$(WORKSPACE)/Build/ArmVExpress-FVP-AArch64
  UEFI_BIN=FVP_AARCH64_EFI.fd
endif

ifeq ("$(OS)","Windows_NT")
export WORKSPACE?=$(PWD)
export EDK_TOOLS_PATH ?= $(WORKSPACE)\BaseTools
endif

SHELL := /bin/bash
SILENT ?= @
ECHO ?= echo
MAKE ?= make -i -k
RM ?= rm -f

.PHONY: all clean

EDK2_CONF = Conf/BuildEnv.sh Conf/build_rule.txt Conf/target.txt Conf/tools_def.txt

#
# FIP Support
#
ifeq ($(BUILD_FIP),"1")
  ifeq ($(FIP_BIN),"")
    $(info Define location of the FIP to automatically update the package after building UEFI.)
  endif
endif

all: $(EDK2_CONF)
ifeq ("$(OS)","Windows_NT")
	build -a $(EDK2_ARCH) -p $(EDK2_DSC) -t $(EDK2_TOOLCHAIN) -b $(EDK2_BUILD) $(EDK2_MACROS)
else
	. ./edksetup.sh; build -a $(EDK2_ARCH) -p $(EDK2_DSC) -t $(EDK2_TOOLCHAIN) -b $(EDK2_BUILD) $(EDK2_MACROS)
endif
ifeq ("$(BUILD_FIP)","1")
ifneq ($(FIP_BIN),"")
	$(SILENT)which fip_create ; \
	if [ $$? -ne 0 ]; then \
	  $(ECHO) "Warning: 'fip_create' tool is not in the PATH. The UEFI binary will not be added in the Firmware Image Package (FIP)."; \
	else \
	  fip_create --bl33 $(BUILD_PATH)/$(EDK2_BUILD)_$(EDK2_TOOLCHAIN)/FV/$(UEFI_BIN) --dump $(FIP_BIN); \
	fi
endif
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
