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

CC     = gcc
CFLAGS = -g

generate_image: generate_image.c
		$(CC) $(CCFLAGS) $(LDFLAGS) -o generate_image generate_image.c

clean:
	rm -f generate_image generate_image.exe
