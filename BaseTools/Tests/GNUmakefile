## @file
# GNU/Linux makefile for 'Tests' module build.
#
# Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

all: test

test:
	@if command -v "${PYTHON_COMMAND}" >/dev/null 2>&1; then ${PYTHON_COMMAND} RunTests.py; else python RunTests.py; fi

clean:
	find . -name '*.pyc' -exec rm '{}' ';'

