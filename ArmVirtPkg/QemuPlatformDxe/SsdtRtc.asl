/** @file
  SSDT exposing the PL031 RTC to the OS as an ACPI Time and Alarm Device
  (TAD, ACPI 6.6 section 9.17).

  The PL031 holds a 32-bit counter counting seconds since 1970-01-01 00:00:00,
  read via RTCDR (offset 0x0) and loaded via RTCLR (offset 0x8). Because _GRT
  and _SRT only perform single 32-bit accesses to those registers, they cannot
  interfere with the firmware's GetTime/SetTime runtime services.

  Copyright (c) 2026, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

DefinitionBlock ("SsdtRtc.aml", "SSDT", 2, "ARMLTD", "PL031RTC", 1)
{
  Scope (\_SB)
  {
    Device (RTC0)
    {
      Name (_HID, "ACPI000E")
      Name (_UID, Zero)

      //
      // The base address placeholder is patched from the FDT 'arm,pl031'
      // node before this SSDT is installed.
      //
      OperationRegion (RTCR, SystemMemory, 0xDEADCAFE00000000, 0x1000)
      Field (RTCR, DWordAcc, NoLock, WriteAsZeros)
      {
        RTDR, 32,
        Offset (0x08),
        RTLR, 32
      }

      Method (_GCP, 0, NotSerialized)
      {
        Return (0x04)  // Get/set real time features implemented
      }

      Method (_GRT, 0, Serialized)
      {
        //
        // Valid is 1, TimeZone is 2047 (unspecified), and Milliseconds and
        // Daylight are 0.
        //
        Local6 = Buffer (0x10)
        {
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
          0x00, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00
        }
        CreateWordField (Local6, 0x00, YEAR)
        CreateByteField (Local6, 0x02, MNTH)
        CreateByteField (Local6, 0x03, MDAY)
        CreateByteField (Local6, 0x04, HOUR)
        CreateByteField (Local6, 0x05, MINS)
        CreateByteField (Local6, 0x06, SECS)

        Divide (RTDR, 86400, Local1, Local0)
        Divide (Local1, 60, SECS, Local1)
        Divide (Local1, 60, MINS, HOUR)

        //
        // Convert days since 1970-01-01 (Local0) to a calendar date using
        // civil_from_days() (400-year eras of 146097 days from 0000-03-01).
        //
        Local0 += 719468
        Divide (Local0, 146097, Local3, Local2)
        Local4 = (Local3 - Local3 / 1460 + Local3 / 36524 - Local3 / 146096) / 365
        Local3 -= Local4 * 365 + Local4 / 4 - Local4 / 100
        Local5 = (Local3 * 5 + 2) / 153
        MDAY = Local3 + 1 - (Local5 * 153 + 2) / 5
        Local2 = Local2 * 400 + Local4
        If (Local5 < 10)
        {
          MNTH = Local5 + 3
        }
        Else
        {
          MNTH = Local5 - 9
          Local2++
        }

        YEAR = Local2
        Return (Local6)
      }

      Method (_SRT, 1, Serialized)
      {
        CreateWordField (Arg0, 0x00, YEAR)
        CreateByteField (Arg0, 0x02, MNTH)
        CreateByteField (Arg0, 0x03, MDAY)
        CreateByteField (Arg0, 0x04, HOUR)
        CreateByteField (Arg0, 0x05, MINS)
        CreateByteField (Arg0, 0x06, SECS)

        If ((YEAR < 1970) || (MNTH < 1) || (MNTH > 12) || (MDAY < 1) ||
            (MDAY > 31) || (HOUR > 23) || (MINS > 59) || (SECS > 59))
        {
          Return (0xFFFFFFFF)
        }

        Local0 = YEAR
        Local1 = MNTH
        If (Local1 < 3)
        {
          Local0--
          Local1 += 12
        }

        Local2 = MDAY + (Local1 * 153 - 457) / 5 + Local0 * 365 +
                 Local0 / 4 - Local0 / 100 + Local0 / 400 - 719469
        Local2 = ((Local2 * 24 + HOUR) * 60 + MINS) * 60 + SECS
        If (Local2 > 0xFFFFFFFF)
        {
          Return (0xFFFFFFFF)
        }

        RTLR = Local2
        Return (Zero)
      }
    }
  }
}
