/** @file
  Implement the invalid functions to return failures.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <errno.h>
#include <dirent.h>
#include <sysexits.h>

typedef VOID* DIR;

struct dirent *        readdir(DIR *);
int                    closedir(DIR *);
DIR *                  opendir(const char *);
