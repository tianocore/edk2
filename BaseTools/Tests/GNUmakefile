## @file
# GNU/Linux makefile for 'Tests' module build.
#
# Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

all: test

test:
	@if command -v python3 >/dev/null 2>&1; then python3 RunTests.py; else echo "Error: Please install a python 3 tool!"; fi

clean:
	find . -name '*.pyc' -exec rm '{}' ';'

