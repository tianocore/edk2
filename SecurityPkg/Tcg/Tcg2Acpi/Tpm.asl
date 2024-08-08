/** @file
  The TPM2 definition block in ACPI table for TCG2 physical presence
  and MemoryClear.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(c)Copyright 2016 HP Development Company, L.P.<BR>
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
  Scope (\_SB)
  {
    Device (TPM)
    {
      //
      // TCG2
      //

      //
      //  TAG for patching TPM2.0 _HID
      //
      Name (_HID, "NNNN0000")

      Name (_CID, "MSFT0101")

      //
      // Readable name of this device, don't know if this way is correct yet
      //
      Name (_STR, Unicode ("TPM 2.0 Device"))

      //
      // Operational region for Smi port access
      //
      OperationRegion (SMIP, SystemIO, FixedPcdGet16 (PcdSmiCommandIoPort), 1)
      Field (SMIP, ByteAcc, NoLock, Preserve)
      {
          IOPN, 8
      }

      //
      // Operational region for TPM access
      //
      OperationRegion (TPMR, SystemMemory, 0xfed40000, 0x5000)
      Field (TPMR, AnyAcc, NoLock, Preserve)
      {
        ACC0, 8,  // TPM_ACCESS_0
        Offset(0x8),
        INTE, 32, // TPM_INT_ENABLE_0
        INTV, 8,  // TPM_INT_VECTOR_0
        Offset(0x10),
        INTS, 32, // TPM_INT_STATUS_0
        INTF, 32, // TPM_INTF_CAPABILITY_0
        STS0, 32, // TPM_STS_0
        Offset(0x24),
        FIFO, 32, // TPM_DATA_FIFO_0
        Offset(0x30),
        TID0, 32, // TPM_INTERFACE_ID_0
                  // ignore the rest
      }

      //
      // Operational region for TPM support, TPM Physical Presence and TPM Memory Clear
      // Region Offset 0xFFFF0000 and Length 0xF0 will be fixed in C code.
      //
      OperationRegion (TNVS, SystemMemory, 0xFFFF0000, 0xF0)
      Field (TNVS, AnyAcc, NoLock, Preserve)
      {
        PPIN,   8,  //   Software SMI for Physical Presence Interface
        PPIP,   32, //   Used for save physical presence parameter
        PPRP,   32, //   Physical Presence request operation response
        PPRQ,   32, //   Physical Presence request operation
        PPRM,   32, //   Physical Presence request operation parameter
        LPPR,   32, //   Last Physical Presence request operation
        FRET,   32, //   Physical Presence function return code
        MCIN,   8,  //   Software SMI for Memory Clear Interface
        MCIP,   32, //   Used for save the Mor parameter
        MORD,   32, //   Memory Overwrite Request Data
        MRET,   32, //   Memory Overwrite function return code
        UCRQ,   32, //   Physical Presence request operation to Get User Confirmation Status
        IRQN,   32, //   IRQ Number for _CRS
        SFRB,   8   //   Is shortformed Pkglength for resource buffer
      }

      //
      // Possible resource settings returned by  _PRS method
      //   RESS : ResourceTemplate with PkgLength <=63
      //   RESL : ResourceTemplate with PkgLength > 63
      //
      // The format of the data has to follow the same format as
      // _CRS (according to ACPI spec).
      //
      Name (RESS, ResourceTemplate() {
        Memory32Fixed (ReadWrite, 0xfed40000, 0x5000)
        Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , ) {1,2,3,4,5,6,7,8,9,10}
      })

      Name (RESL, ResourceTemplate() {
        Memory32Fixed (ReadWrite, 0xfed40000, 0x5000)
        Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , ) {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
      })

      //
      // Current resource settings for _CRS method
      //
      Name(RES0, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xfed40000, 0x5000, REG0)
        Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , INTR) {12}
      })

      Name(RES1, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xfed40000, 0x5000, REG1)
      })


      //
      // Return the resource consumed by TPM device.
      //
      Method(_CRS,0,Serialized)
      {
        //
        // IRQNum = 0 means disable IRQ support
        //
        If (LEqual(IRQN, 0)) {
          Return (RES1)
        }
        Else
        {
          CreateDWordField(RES0, ^INTR._INT, LIRQ)
          Store(IRQN, LIRQ)
          Return (RES0)
        }
      }

      //
      // Set resources consumed by the TPM device. This is used to
      // assign an interrupt number to the device. The input byte stream
      // has to be the same as returned by _CRS (according to ACPI spec).
      //
      // Platform may choose to override this function with specific interrupt
      // programing logic to replace FIFO/TIS SIRQ registers programing
      //
      Method(_SRS,1,Serialized)
      {
        //
        // Do not configure Interrupt if IRQ Num is configured 0 by default
        //
        If (LNotEqual(IRQN, 0)) {
          //
          // Update resource descriptor
          // Use the field name to identify the offsets in the argument
          // buffer and RES0 buffer.
          //
          CreateDWordField(Arg0, ^INTR._INT, IRQ0)
          CreateDWordField(RES0, ^INTR._INT, LIRQ)
          Store(IRQ0, LIRQ)
          Store(IRQ0, IRQN)

          CreateBitField(Arg0, ^INTR._HE, ITRG)
          CreateBitField(RES0, ^INTR._HE, LTRG)
          Store(ITRG, LTRG)

          CreateBitField(Arg0, ^INTR._LL, ILVL)
          CreateBitField(RES0, ^INTR._LL, LLVL)
          Store(ILVL, LLVL)

          //
          // Update TPM FIFO PTP/TIS interface only, identified by TPM_INTERFACE_ID_x lowest
          // nibble.
          // 0000 - FIFO interface as defined in PTP for TPM 2.0 is active
          // 1111 - FIFO interface as defined in TIS1.3 is active
          //
          If (LOr(LEqual (And (TID0, 0x0F), 0x00), LEqual (And (TID0, 0x0F), 0x0F))) {
            //
            // If FIFO interface, interrupt vector register is
            // available. TCG PTP specification allows only
            // values 1..15 in this field. For other interrupts
            // the field should stay 0.
            //
            If (LLess (IRQ0, 16)) {
              Store (And(IRQ0, 0xF), INTV)
            }
            //
            // Interrupt enable register (TPM_INT_ENABLE_x) bits 3:4
            // contains settings for interrupt polarity.
            // The other bits of the byte enable individual interrupts.
            // They should be all be zero, but to avoid changing the
            // configuration, the other bits are be preserved.
            // 00 - high level
            // 01 - low level
            // 10 - rising edge
            // 11 - falling edge
            //
            // ACPI spec definitions:
            // _HE: '1' is Edge, '0' is Level
            // _LL: '1' is ActiveHigh, '0' is ActiveLow (inverted from TCG spec)
            //
            If (LEqual (ITRG, 1)) {
              Or(INTE, 0x00000010, INTE)
            } Else {
              And(INTE, 0xFFFFFFEF, INTE)
            }
            if (LEqual (ILVL, 0)) {
              Or(INTE, 0x00000008, INTE)
            } Else {
              And(INTE, 0xFFFFFFF7, INTE)
            }
          }
        }
      }

      Method(_PRS,0,Serialized)
      {
        //
        // IRQNum = 0 means disable IRQ support
        //
        If (LEqual(IRQN, 0)) {
          Return (RES1)
        } ElseIf(LEqual(SFRB, 0)) {
          //
          // Long format. Possible resources PkgLength > 63
          //
          Return (RESL)
        } Else {
          //
          // Short format. Possible resources PkgLength <=63
          //
          Return (RESS)
        }
      }

      Method (PTS, 1, Serialized)
      {
        //
        // _PTS is deprecated for being security deficient
        // this implementation simply returns to maintain
        // compatibility with older OSes using it.
        //
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
            Name(TPMV, Package () {0x01, Package () {0x2, 0x0}})
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
            Return ("$PV")
          }
          Case (2)
          {
            //
            // b) Submit TPM Operation Request to Pre-OS Environment
            //

            Store (DerefOf (Index (Arg1, 0x00)), PPRQ)
            Store (0, PPRM)
            Store (0x02, PPIP)

            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOPN)
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
            Store (PPIN, IOPN)

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
            Store (0, PPRM)
            If (LEqual (PPRQ, 23)) {
              Store (DerefOf (Index (Arg1, 0x01)), PPRM)
            }

            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOPN)
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
            Store (PPIN, IOPN)

            Return (FRET)
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
        // _DSM Memory Clear is deprecated, so not called
        //

        Return (Buffer () {0})
      }
    }
  }
}
