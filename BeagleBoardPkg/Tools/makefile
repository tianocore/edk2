#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

all: GenerateImage replace

GenerateImage: generate_image.c
		$(CC) $(CCFLAGS) $(LDFLAGS) -o GenerateImage.exe generate_image.c

replace: replace.c
		$(CC) $(CCFLAGS) $(LDFLAGS) -o replace.exe replace.c

clean:
	del GenerateImage.exe generate_image.obj replace.exe replace.obj
