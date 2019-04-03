#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

CC     = gcc
CFLAGS = -g

generate_image: generate_image.c
		$(CC) $(CCFLAGS) $(LDFLAGS) -o generate_image generate_image.c

clean:
	rm -f generate_image generate_image.exe
