## @file
#
#
# Copyright (c) 2009 - 2014, Apple Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
import sys
import locale

if sys.platform == "darwin":
  DefaultLocal = locale.getdefaultlocale()[1]
  if DefaultLocal is None:
    DefaultLocal = 'UTF8'
  sys.setdefaultencoding(DefaultLocal)

