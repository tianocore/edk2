## @file
# Sample makefile for PREBUILD or POSTBUILD action.
#
# Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

all: show
	@echo $@
genc: show
	@echo $@
genmake: show
	@echo $@
modules: show
	@echo $@
libraries: show
	@echo $@
fds: show
	@echo $@
clean: show
	@echo $@
cleanall: show
	@echo $@
cleanlib: show
	@echo $@
run: show
	@echo $@

show:
	@echo WORKSPACE........ $(WORKSPACE)
	@echo PACKAGES_PATH.... $(PACKAGES_PATH)
	@echo ACTIVE_PLATFORM.. $(ACTIVE_PLATFORM)
	@echo TARGET_ARCH...... $(TARGET_ARCH)
	@echo TOOL_CHAIN_TAG... $(TOOL_CHAIN_TAG)
	@echo CONF_DIRECTORY... $(CONF_DIRECTORY)
	@echo TARGET........... $(TARGET)
	@echo EXTRA_FLAGS...... $(EXTRA_FLAGS)
