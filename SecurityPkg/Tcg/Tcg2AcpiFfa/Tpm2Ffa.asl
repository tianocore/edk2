/** @file

The TPM2 definition block in ACPI table for TCG2 physical presence.

Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
  "Tpm.aml",
  "SSDT",
  2,
  "INTEL ",
  "Tpm2Tabl",
  0x1000
  )
{
  Scope (_SB_) {
    Device(TPM0) {
      Name (_HID, "NNNN0000")
      Name (_CID, "MSFT0101")

      Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate ()
        {
          QWordMemory (
            ResourceConsumer,
            PosDecode,
            MinFixed,
            MaxFixed,
            Cacheable,
            ReadWrite,
            0x0,
            FixedPcdGet64 (PcdTpmBaseAddress),
            FixedPcdGet64 (PcdTpmMaxAddress),
            0x0,
            FixedPcdGet32 (PcdTpmCrbRegionSize))
        })
        Return (RBUF)
      }

      Method (_STR, 0) {
        Return (Unicode ("TPM 2.0 Device"))
      }

      Method (_STA, 0)
      {
        Return (0x0f)
      }

      Name(TPM2, Package (0x02){
        0x0,        // Function Return Code - Success
        0x0         // Pending operation requested by the OS - None
      })

      Name(TPM3, Package (0x03){
        0x0,        // Function Return Code - Success
        0x0,        // Most recent operation request - None
        0x0         // Response to the most recent operation request - Success
      })

      Name(BUFF, Buffer (79){})
      CreateQWordField (BUFF, 0, STAT)   //  In/Out - Status for req/rsp
      CreateQWordField (BUFF, 8, RCID)   //  In - Receiver ID populated by ASL. If zero, OS will populate this based on UUID.
      CreateField (BUFF, 128, 128, UUID) //  UUID of service
      CreateByteField (BUFF, 32, PPIN)   //  Software SMI for Physical Presence Interface
      CreateDWordField (BUFF, 33, PPIP)  //  Used for save physical presence parameter
      CreateDWordField (BUFF, 37, PPRP)  //  Physical Presence request operation response
      CreateDWordField (BUFF, 41, PPRQ)  //  Physical Presence request operation
      CreateDWordField (BUFF, 45, PPRM)  //  Physical Presence request operation parameter
      CreateDWordField (BUFF, 49, LPPR)  //  Last Physical Presence request operation
      CreateDWordField (BUFF, 53, FRET)  //  Physical Presence function return code
      CreateByteField (BUFF, 54, MCIN)   //  Software SMI for Memory Clear Interface
      CreateDWordField (BUFF, 58, MCIP)  //  Used for save the Mor parameter
      CreateDWordField (BUFF, 62, MORD)  //  Memory Overwrite Request Data
      CreateDWordField (BUFF, 66, MRET)  //  Memory Overwrite function return code
      CreateDWordField (BUFF, 70, UCRQ)  //  Physical Presence request operation to Get User Confirmation Status
      CreateDWordField (BUFF, 74, IRQN)  //  IRQ Number for _CRS
      CreateByteField (BUFF, 78, SFRB)   //  Is shortformed Pkglength for resource buffer

      //
      // FFA Direct Req2 Wrapper
      //
      OperationRegion(AFFH, FFixedHw, 2, 144)
      Field(AFFH, BufferAcc, NoLock, Preserve) { AccessAs(BufferAcc, 0x1), FFAC, 1152 }
      Method (FDR2, 0, Serialized) {
        // Service UUID from the input
        Store(ToUUID("3dddfaa6-361b-4eb4-a424-8d10089d1653"), UUID)
        Store(Store(BUFF, \_SB_.TPM0.FFAC), BUFF)
      }

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
            Return ("$PV")
          }
          Case (2)
          {
            //
            // b) Submit TPM Operation Request to Pre-OS Environment
            //
            Store (0, BUFF)

            Store (DerefOf (Index (Arg1, 0x00)), PPRQ)
            Store (0, PPRM)
            Store (0x02, PPIP)

            //
            // Trigger the FFA direct req2
            //
            FDR2 ()
            If(LNotEqual (STAT, 0)) {
              Return (STAT)
            }
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
            Store (0, BUFF)

            Store (0x05, PPIP)

            //
            // Trigger the FFA direct req2
            //
            FDR2 ()
            If(LNotEqual (STAT, 0)) {
              Return (STAT)
            }

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
            Store (0, BUFF)

            Store (7, PPIP)
            Store (DerefOf (Index (Arg1, 0x00)), PPRQ)
            Store (0, PPRM)
            If (LEqual (PPRQ, 23)) {
              Store (DerefOf (Index (Arg1, 0x01)), PPRM)
            }

            //
            // Trigger the FFA direct req2
            //
            FDR2 ()
            If(LNotEqual (STAT, 0)) {
              Return (STAT)
            }
            Return (FRET)
          }
          Case (8)
          {
            //
            // e) Get User Confirmation Status for Operation
            //
            Store (0, BUFF)

            Store (8, PPIP)
            Store (DerefOf (Index (Arg1, 0x00)), UCRQ)

            //
            // Trigger the FFA direct req2
            //
            FDR2 ()
            If(LNotEqual (STAT, 0)) {
              Return (STAT)
            }
            Return (FRET)
          }

          Default {BreakPoint}
        }
        Return (1)
      }


      Method (_DSM, 4, Serialized, 0, UnknownObj, {BuffObj, IntObj, IntObj, PkgObj})
      {
        //
        // TCG Physical Presence Interface
        //
        If(LEqual(Arg0, ToUUID ("3dddfaa6-361b-4eb4-a424-8d10089d1653")))
        {
          Return (TPPI (Arg2, Arg3))
        }

        Return (Buffer () {0})
      }
    }
  } // Scope(_SB_)
}
