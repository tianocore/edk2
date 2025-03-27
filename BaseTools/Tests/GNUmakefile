## @file
# GNU/Linux makefile for 'Tests' module build.
#
# Copyright (c) 2009 - 2024, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

all: test

ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
test:
	$(shell if defined PYTHON_COMMAND $(PYTHON_COMMAND) RunTests.py)
	$(shell if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe RunTests.py)
clean:
	del /s /q *.pyc
else
test:
	@if command -v $(PYTHON_COMMAND) >/dev/null 1; then $(PYTHON_COMMAND) RunTests.py; else python RunTests.py; fi
clean:
	find . -name '*.pyc' -exec rm '{}' ';'
endif
