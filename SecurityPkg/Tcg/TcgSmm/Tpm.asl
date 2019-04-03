/** @file
  The TPM definition block in ACPI table for physical presence
  and MemoryClear.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
  "Tpm.aml",
  "SSDT",
  2,
  "INTEL ",
  "TcgTable",
  0x1000
  )
{
  Scope (\_SB)
  {
    Device (TPM)
    {
      //
      // Define _HID, "PNP0C31" is defined in
      // "Secure Startup-FVE and TPM Admin BIOS and Platform Requirements"
      //
      Name (_HID, EISAID ("PNP0C31"))

      //
      // Readable name of this device, don't know if this way is correct yet
      //
      Name (_STR, Unicode ("TPM 1.2 Device"))

      //
      // Return the resource consumed by TPM device
      //
      Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xfed40000, 0x5000)
      })

      //
      // Operational region for Smi port access
      //
      OperationRegion (SMIP, SystemIO, 0xB2, 1)
      Field (SMIP, ByteAcc, NoLock, Preserve)
      {
          IOB2, 8
      }

      //
      // Operational region for TPM access
      //
      OperationRegion (TPMR, SystemMemory, 0xfed40000, 0x5000)
      Field (TPMR, AnyAcc, NoLock, Preserve)
      {
        ACC0, 8,
      }

      //
      // Operational region for TPM support, TPM Physical Presence and TPM Memory Clear
      // Region Offset 0xFFFF0000 and Length 0xF0 will be fixed in C code.
      //
      OperationRegion (TNVS, SystemMemory, 0xFFFF0000, 0xF0)
      Field (TNVS, AnyAcc, NoLock, Preserve)
      {
        PPIN,   8,  //   Software SMI for Physical Presence Interface
        PPIP,   32, //   Used for save physical presence paramter
        PPRP,   32, //   Physical Presence request operation response
        PPRQ,   32, //   Physical Presence request operation
        LPPR,   32, //   Last Physical Presence request operation
        FRET,   32, //   Physical Presence function return code
        MCIN,   8,  //   Software SMI for Memory Clear Interface
        MCIP,   32, //   Used for save the Mor paramter
        MORD,   32, //   Memory Overwrite Request Data
        MRET,   32, //   Memory Overwrite function return code
        UCRQ,   32  //   Phyical Presence request operation to Get User Confirmation Status
      }

      Method (PTS, 1, Serialized)
      {
        //
        // Detect Sx state for MOR, only S4, S5 need to handle
        //
        If (LAnd (LLess (Arg0, 6), LGreater (Arg0, 3)))
        {
          //
          // Bit4 -- DisableAutoDetect. 0 -- Firmware MAY autodetect.
          //
          If (LNot (And (MORD, 0x10)))
          {
            //
            // Trigger the SMI through ACPI _PTS method.
            //
            Store (0x02, MCIP)

            //
            // Trigger the SMI interrupt
            //
            Store (MCIN, IOB2)
          }
        }
        Return (0)
      }

      Method (_STA, 0)
      {
        if (LEqual (ACC0, 0xff))
        {
            Return (0)
        }
        Return (0x0f)
      }

      //
      // TCG Hardware Information
      //
      Method (HINF, 1, Serialized, 0, {BuffObj, PkgObj}, {UnknownObj}) // IntObj
      {
        //
        // Switch by function index
        //
        Switch (ToInteger(Arg0))
        {
          Case (0)
          {
            //
            // Standard query
            //
            Return (Buffer () {0x03})
          }
          Case (1)
          {
            //
            // Return failure if no TPM present
            //
            Name(TPMV, Package () {0x01, Package () {0x1, 0x20}})
            if (LEqual (_STA (), 0x00))
            {
              Return (Package () {0x00})
            }

            //
            // Return TPM version
            //
            Return (TPMV)
          }
          Default {BreakPoint}
        }
        Return (Buffer () {0})
      }

      Name(TPM2, Package (0x02){
        Zero,
        Zero
      })

      Name(TPM3, Package (0x03){
        Zero,
        Zero,
        Zero
      })

      //
      // TCG Physical Presence Interface
      //
      Method (TPPI, 2, Serialized, 0, {BuffObj, PkgObj, IntObj, StrObj}, {UnknownObj, UnknownObj}) // IntObj, PkgObj
      {
        //
        // Switch by function index
        //
        Switch (ToInteger(Arg0))
        {
          Case (0)
          {
            //
            // Standard query, supports function 1-8
            //
            Return (Buffer () {0xFF, 0x01})
          }
          Case (1)
          {
            //
            // a) Get Physical Presence Interface Version
            //
            Return ("1.2")
          }
          Case (2)
          {
            //
            // b) Submit TPM Operation Request to Pre-OS Environment
            //

            Store (DerefOf (Index (Arg1, 0x00)), PPRQ)
            Store (0x02, PPIP)

            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB2)
            Return (FRET)


          }
          Case (3)
          {
            //
            // c) Get Pending TPM Operation Requested By the OS
            //

            Store (PPRQ, Index (TPM2, 0x01))
            Return (TPM2)
          }
          Case (4)
          {
            //
            // d) Get Platform-Specific Action to Transition to Pre-OS Environment
            //
            Return (2)
          }
          Case (5)
          {
            //
            // e) Return TPM Operation Response to OS Environment
            //
            Store (0x05, PPIP)

            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB2)

            Store (LPPR, Index (TPM3, 0x01))
            Store (PPRP, Index (TPM3, 0x02))

            Return (TPM3)
          }
          Case (6)
          {

            //
            // f) Submit preferred user language (Not implemented)
            //

            Return (3)

          }
          Case (7)
          {
            //
            // g) Submit TPM Operation Request to Pre-OS Environment 2
            //
            Store (7, PPIP)
            Store (DerefOf (Index (Arg1, 0x00)), PPRQ)

            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB2)
            Return (FRET)
          }
          Case (8)
          {
            //
            // e) Get User Confirmation Status for Operation
            //
            Store (8, PPIP)
            Store (DerefOf (Index (Arg1, 0x00)), UCRQ)

            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB2)

            Return (FRET)
          }

          Default {BreakPoint}
        }
        Return (1)
      }

      Method (TMCI, 2, Serialized, 0, IntObj, {UnknownObj, UnknownObj}) // IntObj, PkgObj
      {
        //
        // Switch by function index
        //
        Switch (ToInteger (Arg0))
        {
          Case (0)
          {
            //
            // Standard query, supports function 1-1
            //
            Return (Buffer () {0x03})
          }
          Case (1)
          {
            //
            // Save the Operation Value of the Request to MORD (reserved memory)
            //
            Store (DerefOf (Index (Arg1, 0x00)), MORD)

            //
            // Trigger the SMI through ACPI _DSM method.
            //
            Store (0x01, MCIP)

            //
            // Trigger the SMI interrupt
            //
            Store (MCIN, IOB2)
            Return (MRET)
          }
          Default {BreakPoint}
        }
        Return (1)
      }

      Method (_DSM, 4, Serialized, 0, UnknownObj, {BuffObj, IntObj, IntObj, PkgObj})
      {

        //
        // TCG Hardware Information
        //
        If(LEqual(Arg0, ToUUID ("cf8e16a5-c1e8-4e25-b712-4f54a96702c8")))
        {
          Return (HINF (Arg2))
        }

        //
        // TCG Physical Presence Interface
        //
        If(LEqual(Arg0, ToUUID ("3dddfaa6-361b-4eb4-a424-8d10089d1653")))
        {
          Return (TPPI (Arg2, Arg3))
        }

        //
        // TCG Memory Clear Interface
        //
        If(LEqual(Arg0, ToUUID ("376054ed-cc13-4675-901c-4756d7f2d45d")))
        {
          Return (TMCI (Arg2, Arg3))
        }

        Return (Buffer () {0})
      }
    }
  }
}
