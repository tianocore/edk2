## @file
# Windows makefile for 'GenFw' module build.
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
!INCLUDE ..\Makefiles\ms.common

APPNAME = GenFw

LIBS = $(LIB_PATH)\Common.lib

OBJECTS = GenFw.obj ElfConvert.obj Elf32Convert.obj Elf64Convert.obj

#CFLAGS = $(CFLAGS) /nodefaultlib:libc.lib

!INCLUDE ..\Makefiles\ms.app

