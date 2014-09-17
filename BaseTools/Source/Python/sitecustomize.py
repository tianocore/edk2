## @file
#
#
# Copyright (c) 2009 - 2014, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
import sys
import locale

if sys.platform == "darwin":
  DefaultLocal = locale.getdefaultlocale()[1]
  if DefaultLocal is None:
    DefaultLocal = 'UTF8'  
  sys.setdefaultencoding(DefaultLocal)

