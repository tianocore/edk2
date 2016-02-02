#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

all: GenerateImage replace

GenerateImage: generate_image.c
		$(CC) $(CCFLAGS) $(LDFLAGS) -o GenerateImage.exe generate_image.c

replace: replace.c
		$(CC) $(CCFLAGS) $(LDFLAGS) -o replace.exe replace.c

clean:
	del GenerateImage.exe generate_image.obj replace.exe replace.obj
