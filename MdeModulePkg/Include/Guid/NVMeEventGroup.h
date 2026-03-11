/** @file

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

// gNVMeEnableStartEventGroupGuid is used to signal the start of enabling the NVMe controller
extern EFI_GUID  gNVMeEnableStartEventGroupGuid;
// gNVMeEnableCompleteEventGroupGuid is used to signal that the NVMe controller enable has finished
extern EFI_GUID  gNVMeEnableCompleteEventGroupGuid;
