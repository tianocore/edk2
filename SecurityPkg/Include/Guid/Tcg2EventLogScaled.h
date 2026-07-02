/** @file
  Defines the GUID used to signal that the TCG event log has been dynamically
  scaled. Consumers may register a notification callback on this event group
  to react to the scaling event.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TCG2_EVENT_LOG_SCALED_H_
#define TCG2_EVENT_LOG_SCALED_H_

#define TCG2_EVENT_LOG_SCALED_GUID \
  { \
    0x9b4f7c2a, 0x1d3e, 0x4a8b, { 0x9c, 0x6f, 0x5e, 0x8d, 0x2a, 0x1b, 0x4c, 0x7f } \
  }

extern EFI_GUID  gTcg2EventLogScaledGuid;

#endif // TCG2_EVENT_LOG_SCALED_H_
