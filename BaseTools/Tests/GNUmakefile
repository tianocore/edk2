## @file
# GNU/Linux makefile for 'Tests' module build.
#
# Copyright (c) 2009 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

all: test

ifndef PYTHON_COMMAND
  ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
    SHELL := cmd.exe
    MAKEROOT := $(shell echo %CD%)
    SEP:=$(shell echo \)
    #
    # Try using the Python Launcher for Windows to find an interperter.
    #
    CHECK_PY := $(shell where py.exe || echo NotFound)
    ifeq ($(CHECK_PY),NotFound)
      #
      # PYTHON_HOME is the old method of specifying a Python interperter on Windows.
      # Check if an interperter can be found using PYTHON_HOME.
      #
      ifdef PYTHON_HOME
        ifndef (,$(wildcard $(PYTHON_HOME)$(SEP)python.exe)) # Make sure the file exists
          PYTHON_COMMAND := $(PYTHON_HOME)$(SEP)python.exe
        else
          $(error Unable to find a Python interperter, if one is installed, set the PYTHON_COMMAND environment variable!)
        endif
      endif
    else
      PYTHON_COMMAND := $(shell py -3 -c "import sys; print(sys.executable)")
      ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
        $(error Unable to find a Python interperter, if one is installed, set the PYTHON_COMMAND environment variable!)
      endif
    endif
    undefine CHECK_PY
  else # UNIX
    MAKEROOT := .
    SEP:=/
    PYTHON_COMMAND := $(shell /usr/bin/env python3 -c "import sys; print(sys.executable)")
    ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
      PYTHON_COMMAND := $(shell /usr/bin/env python -c "import sys; print(sys.executable)")
      ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
        undefine PYTHON_COMMAND
      endif
    endif
    ifndef PYTHON_COMMAND
      $(error Unable to find a Python interpreter, if one is installed, set the PYTHON_COMMAND environment variable!)
    endif
  endif
endif

test:
	$(PYTHON_COMMAND) RunTests.py
clean:
	$(PYTHON_COMMAND) $(MAKEROOT)$(SEP)..$(SEP)Source$(SEP)C$(SEP)Makefiles$(SEP)GnuMakeUtils.py rm_pyc_files .
