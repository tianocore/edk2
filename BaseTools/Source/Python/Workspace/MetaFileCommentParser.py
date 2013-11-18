## @file
# This file is used to check format of comments
#
# Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

from CommonDataClass.DataClass import (
    MODEL_PCD_PATCHABLE_IN_MODULE,
    MODEL_PCD_DYNAMIC_EX,
    MODEL_PCD_DYNAMIC,
    MODEL_EFI_GUID,
    MODEL_EFI_PPI,
    MODEL_EFI_PROTOCOL
)
from Common.BuildToolError import FORMAT_INVALID
import Common.EdkLogger as EdkLogger

UsageList = ("PRODUCES", "PRODUCED", "ALWAYS_PRODUCES", "ALWAYS_PRODUCED", "SOMETIMES_PRODUCES",
             "SOMETIMES_PRODUCED", "CONSUMES", "CONSUMED", "ALWAYS_CONSUMES", "ALWAYS_CONSUMED",
             "SOMETIMES_CONSUMES", "SOMETIMES_CONSUMED", "SOMETIME_CONSUMES")

