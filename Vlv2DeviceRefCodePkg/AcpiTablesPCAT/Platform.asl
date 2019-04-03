/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/


// Define the following External variables to prevent a WARNING when
// using ASL.EXE and an ERROR when using IASL.EXE.

External(PDC0)
External(PDC1)
External(PDC2)
External(PDC3)
External(CFGD)
External(\_PR.CPU0._PPC, IntObj)
External(\_SB.PCI0.LPCB.TPM.PTS, MethodObj)
External(\_SB.STR3, DeviceObj)
External(\_SB.I2C1.BATC, DeviceObj)
External(\_SB.DPTF, DeviceObj)
External(\_SB.TCHG, DeviceObj)
External(\_SB.IAOE.PTSL)
External(\_SB.IAOE.WKRS)

//
// Create a Global MUTEX.
//
Mutex(MUTX,0)



// Port 80h Update:
//              Update 8 bits of the 32-bit Port 80h.
//
//      Arguments:
//              Arg0:   0 = Write Port 80h, Bits 7:0 Only.
//                      1 = Write Port 80h, Bits 15:8 Only.
//                      2 = Write Port 80h, Bits 23:16 Only.
//                      3 = Write Port 80h, Bits 31:24 Only.
//              Arg1:   8-bit Value to write
//
//      Return Value:
//              None

Method(P8XH,2,Serialized)
{
  If(LEqual(Arg0,0))            // Write Port 80h, Bits 7:0.
  {
    Store(Or(And(P80D,0xFFFFFF00),Arg1),P80D)
  }

  If(LEqual(Arg0,1))            // Write Port 80h, Bits 15:8.
  {
    Store(Or(And(P80D,0xFFFF00FF),ShiftLeft(Arg1,8)),P80D)
  }

  If(LEqual(Arg0,2))            // Write Port 80h, Bits 23:16.
  {
    Store(Or(And(P80D,0xFF00FFFF),ShiftLeft(Arg1,16)),P80D)
  }

  If(LEqual(Arg0,3))            // Write Port 80h, Bits 31:24.
  {
    Store(Or(And(P80D,0x00FFFFFF),ShiftLeft(Arg1,24)),P80D)
  }

}

//
// Define SW SMI port as an ACPI Operating Region to use for generate SW SMI.
//
OperationRegion (SPRT, SystemIO, 0xB2, 2)
Field (SPRT, ByteAcc, Lock, Preserve)
{
  SSMP, 8
}

// The _PIC Control Method is optional for ACPI design.  It allows the
// OS to inform the ASL code which interrupt controller is being used,
// the 8259 or APIC.  The reference code in this document will address
// PCI IRQ Routing and resource allocation for both cases.
//
// The values passed into _PIC are:
//       0 = 8259
//       1 = IOAPIC

Method(\_PIC,1)
{
  Store(Arg0,GPIC)
  Store(Arg0,PICM)
}

OperationRegion(SWC0, SystemIO, 0x610, 0x0F)
Field(SWC0, ByteAcc, NoLock, Preserve)
{
  G1S, 8,      //SWC GPE1_STS
  Offset(0x4),
  G1E, 8,
  Offset(0xA),
  G1S2, 8,     //SWC GPE1_STS_2
  G1S3, 8      //SWC GPE1_STS_3
}

OperationRegion (SWC1, SystemIO, \PMBS, 0x2C)
Field(SWC1, DWordAcc, NoLock, Preserve)
{
  Offset(0x20),
  G0S, 32,      //GPE0_STS
  Offset(0x28),
  G0EN, 32      //GPE0_EN
}

// Prepare to Sleep.  The hook is called when the OS is about to
// enter a sleep state.  The argument passed is the numeric value of
// the Sx state.

Method(_PTS,1)
{
  Store(0,P80D)         // Zero out the entire Port 80h DWord.
  P8XH(0,Arg0)          // Output Sleep State to Port 80h, Byte 0.

  //clear the 3 SWC status bits
  Store(Ones, G1S3)
  Store(Ones, G1S2)
  Store(1, G1S)

  //set SWC GPE1_EN
  Store(1,G1E)

  //clear GPE0_STS
  Store(Ones, G0S)


  If(LEqual(Arg0,3))   // If S3 Suspend
  {
    //
    // Disable Digital Thermal Sensor function when doing S3 suspend
    //
    If(CondRefOf(DTSE))
    {
      If(LGreaterEqual(DTSE, 0x01))
      {
        Store(30, DTSF) // DISABLE_UPDATE_DTS_EVERY_SMI
        Store(0xD0, SSMP) // DTS SW SMI
      }
    }
  }
}

// Wake.  This hook is called when the OS is about to wake from a
// sleep state.  The argument passed is the numeric value of the
// sleep state the system is waking from.
Method(_WAK,1,Serialized)
{
  P8XH(1,0xAB) // Beginning of _WAK.

  Notify(\_SB.PWRB,0x02)

  If(NEXP)
  {
    // Reinitialize the Native PCI Express after resume
    If(And(OSCC,0x02))
    {
      \_SB.PCI0.NHPG()
    }

    If(And(OSCC,0x04))   // PME control granted?
    {
      \_SB.PCI0.NPME()
    }
  }

  If(LOr(LEqual(Arg0,3), LEqual(Arg0,4)))   // If S3 or S4 Resume
  {


    // If CMP is enabled, we may need to restore the C-State and/or
    // P-State configuration, as it may have been saved before the
    // configuration was finalized based on OS/driver support.
    //
    //   CFGD[24]  = Two or more cores enabled
    //
    If(And(CFGD,0x01000000))
    {
      //
      // If CMP and the OSYS is WinXP SP1, we will enable C1-SMI if
      // C-States are enabled.
      //
      //   CFGD[7:4] = C4, C3, C2, C1 Capable/Enabled
      //
      //
    }

    // Windows XP SP2 does not properly restore the P-State
    // upon resume from S4 or S3 with degrade modes enabled.
    // Use the existing _PPC methods to cycle the available
    // P-States such that the processor ends up running at
    // the proper P-State.
    //
    // Note:  For S4, another possible W/A is to always boot
    // the system in LFM.
    //
    If(LEqual(OSYS,2002))
    {
      If(And(CFGD,0x01))
      {
        If(LGreater(\_PR.CPU0._PPC,0))
        {
          Subtract(\_PR.CPU0._PPC,1,\_PR.CPU0._PPC)
          PNOT()
          Add(\_PR.CPU0._PPC,1,\_PR.CPU0._PPC)
          PNOT()
        }
        Else
        {
          Add(\_PR.CPU0._PPC,1,\_PR.CPU0._PPC)
          PNOT()
          Subtract(\_PR.CPU0._PPC,1,\_PR.CPU0._PPC)
          PNOT()
        }
      }
    }
  }
  Return(Package() {0,0})
}

// Power Notification:
//              Perform all needed OS notifications during a
//              Power Switch.
//
//      Arguments:
//              None
//
//      Return Value:
//              None

Method(PNOT,0,Serialized)
{
  // If MP enabled and driver support is present, notify all
  // processors.

  If(MPEN)
  {
    If(And(PDC0,0x0008))
    {
      Notify(\_PR.CPU0,0x80)    // Eval CPU0 _PPC.

      If(And(PDC0,0x0010))
      {
        Sleep(100)
        Notify(\_PR.CPU0,0x81)  // Eval _CST.
      }
    }

    If(And(PDC1,0x0008))
    {
      Notify(\_PR.CPU1,0x80)    // Eval CPU1 _PPC.

      If(And(PDC1,0x0010))
      {
        Sleep(100)
        Notify(\_PR.CPU1,0x81)  // Eval _CST.
      }
    }

    If(And(PDC2,0x0008))
    {
      Notify(\_PR.CPU2,0x80)    // Eval CPU2 _PPC.

      If(And(PDC2,0x0010))
      {
        Sleep(100)
        Notify(\_PR.CPU2,0x81)  // Eval _CST.
      }
    }

    If(And(PDC3,0x0008))
    {
      Notify(\_PR.CPU3,0x80)    // Eval CPU3 _PPC.

      If(And(PDC3,0x0010))
      {
        Sleep(100)
        Notify(\_PR.CPU3,0x81)  // Eval _CST.
      }
    }
  }
  Else
  {
    Notify(\_PR.CPU0,0x80)      // Eval _PPC.
    Sleep(100)
    Notify(\_PR.CPU0,0x81)      // Eval _CST
  }
}

//
// System Bus
//
Scope(\_SB)
{
  Name(CRTT, 110) // Processor critical temperature
  Name(ACTT, 77)  // Active temperature limit for processor participant
  Name(GCR0, 70)  // Critical temperature for Generic participant 0 in degree celsius
  Name(GCR1, 70)  // Critical temperature for Generic participant 1 in degree celsius
  Name(GCR2, 70)  // Critical temperature for Generic participant 2 in degree celsius
  Name(GCR3, 70)  // Critical temperature for Generic participant 3 in degree celsius
  Name(GCR4, 70)  // Critical temperature for Generic participant 4 in degree celsius
  Name(GCR5, 70)  // Critical temperature for Generic participant 5 in degree celsius
  Name(GCR6, 70)  // Critical temperature for Generic participant 6 in degree celsius
  Name(PST0, 60)  // Passive temperature limit for Generic Participant 0 in degree celsius
  Name(PST1, 60)  // Passive temperature limit for Generic Participant 1 in degree celsius
  Name(PST2, 60)  // Passive temperature limit for Generic Participant 2 in degree celsius
  Name(PST3, 60)  // Passive temperature limit for Generic Participant 3 in degree celsius
  Name(PST4, 60)  // Passive temperature limit for Generic Participant 4 in degree celsius
  Name(PST5, 60)  // Passive temperature limit for Generic Participant 5 in degree celsius
  Name(PST6, 60)  // Passive temperature limit for Generic Participant 6 in degree celsius
  Name(LPMV, 3)
  Name(PDBG, 0)   // DPTF Super debug option
  Name(PDPM, 1)   // DPTF DPPM enable
  Name(PDBP, 1)   // DPTF DBPT enable (dynamic battery protection technology)
  Name(DLPO, Package()
  {
    0x1, // Revision
    0x1, // LPO Enable
    0x1, // LPO StartPState
    25,  // LPO StepSize
    0x1, //
    0x1, //
  })
  Name(BRQD, 0x00) // This is used to determine if DPTF display participant requested Brightness level change
  // or it is from Graphics driver. Value of 1 is for DPTF else it is 0

  Method(_INI,0)
  {
    // NVS has stale DTS data.  Get and update the values
    // with current temperatures.   Note that this will also
    // re-arm any AP Thermal Interrupts.
    // Read temperature settings from global NVS
    Store(DPCT, CRTT)
    Store(Subtract(DPPT, 8), ACTT)                      // Active Trip point = Passive trip point - 8
    Store(DGC0, GCR0)
    Store(DGC0, GCR1)
    Store(DGC1, GCR2)
    Store(DGC1, GCR3)
    Store(DGC1, GCR4)
    Store(DGC2, GCR5)
    Store(DGC2, GCR6)
    Store(DGP0, PST0)
    Store(DGP0, PST1)
    Store(DGP1, PST2)
    Store(DGP1, PST3)
    Store(DGP1, PST4)
    Store(DGP2, PST5)
    Store(DGP2, PST6)
    // Read Current low power mode setting from global NVS
    Store(DLPM, LPMV)


    // Update DPTF Super Debug option
    Store(DDBG, PDBG)


    // Update DPTF LPO Options
    Store(LPOE, Index(DLPO,1))
    Store(LPPS, Index(DLPO,2))
    Store(LPST, Index(DLPO,3))
    Store(LPPC, Index(DLPO,4))
    Store(LPPF, Index(DLPO,5))
    Store(DPME, PDPM)
  }

  // Define a (Control Method) Power Button.
  Device(PWRB)
  {
    Name(_HID,EISAID("PNP0C0C"))

    // GPI_SUS0 = GPE16 = Waketime SCI.  The PRW isn't working when
    // placed in any of the logical locations ( PS2K, PS2M),
    // so a Power Button Device was created specifically
    // for the WAKETIME_SCI PRW.

    Name(_PRW, Package() {16,4})
  }

  Device(SLPB)
  {
    Name(_HID, EISAID("PNP0C0E"))
  } // END SLPB

  Scope(PCI0)
  {
    Method(_INI,0)
    {
      // Determine the OS and store the value, where:
      //
      //   OSYS = 2009 = Windows 7 and Windows Server 2008 R2.
      //   OSYS = 2012 = Windows 8 and Windows Server 2012.
      //
      // Assume Windows 7 at a minimum.

      Store(2009,OSYS)

      // Check for a specific OS which supports _OSI.

      If(CondRefOf(\_OSI,Local0))
      {
        // Linux returns _OSI = TRUE for numerous Windows
        // strings so that it is fully compatible with
        // BIOSes available in the market today.  There are
        // currently 2 known exceptions to this model:
        //      1) Video Repost - Linux supports S3 without
        //              requireing a Driver, meaning a Video
        //              Repost will be required.
        //      2) On-Screen Branding - a full CMT Logo
        //              is limited to the WIN2K and WINXP
        //              Operating Systems only.

        // Use OSYS for Windows Compatibility.
        If(\_OSI("Windows 2009"))   // Windows 7 or Windows Server 2008 R2
        {
          Store(2009,OSYS)
        }
        If(\_OSI("Windows 2012"))   // Windows 8 or Windows Server 2012
        {
          Store(2012,OSYS)
        }
        If(\_OSI("Windows 2013"))   //Windows Blue
        {
          Store(2013,OSYS)
        }

        //
        // If CMP is enabled, enable SMM C-State
        // coordination.  SMM C-State coordination
        // will be disabled in _PDC if driver support
        // for independent C-States deeper than C1
        // is indicated.
      }
    }

    Method(NHPG,0,Serialized)
    {

    }

    Method(NPME,0,Serialized)
    {

    }
  } // end Scope(PCI0)

  Device (GPED)   //virtual GPIO device for ASL based AC/Battery/Expection notification
  {
    Name (_ADR, 0)
    Name (_HID, "INT0002")
    Name (_CID, "INT0002")
    Name (_DDN, "Virtual GPIO controller" )
    Name (_UID, 1)

    Method (_CRS, 0x0, Serialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, ) {0x9} // Was 9
      })
      Return (RBUF)
    }

    Method (_STA, 0x0, NotSerialized)
    {
      Return(0x0)
    }

    Method (_AEI, 0x0, Serialized)
    {
      Name(RBUF, ResourceTemplate()
      {
        GpioInt(Edge, ActiveHigh, ExclusiveAndWake, PullDown,,"\\_SB.GPED",) {2} //pin 2
      })
      Return(RBUF)
    }

    Method(_E02)   // _Exx method will be called when interrupt is raised
    {
      If (LEqual (PWBS, 1))
      {
        Store (1, PWBS)      //Clear PowerButton Status
      }
      If (LEqual (PMEB, 1))
      {
        Store (1, PMEB)      //Clear PME_B0_STS
      }
      If (LEqual (\_SB.PCI0.SATA.PMES, 1))
      {
        Store (1, \_SB.PCI0.SATA.PMES)
        Notify (\_SB.PCI0.SATA, 0x02)
      }
      //
      // eMMC 4.41
      //
      If (LAnd(LEqual (\_SB.PCI0.EM41.PMES, 1), LEqual(PCIM, 1)))
      {
        Store (1, \_SB.PCI0.EM41.PMES)
        Notify (\_SB.PCI0.EM41, 0x02)
      }

      //
      // eMMC 4.5
      //
      If (LAnd(LEqual (\_SB.PCI0.EM45.PMES, 1), LEqual(PCIM, 1)))
      {
        Store (1, \_SB.PCI0.EM45.PMES)
        Notify (\_SB.PCI0.EM45, 0x02)
      }

      If (LEqual(HDAD, 0))
      {
        If (LEqual (\_SB.PCI0.HDEF.PMES, 1))
        {
          Store (1, \_SB.PCI0.HDEF.PMES)
          Notify (\_SB.PCI0.HDEF, 0x02)
        }
      }

      If (LEqual (\_SB.PCI0.EHC1.PMES, 1))
      {
        Store (1, \_SB.PCI0.EHC1.PMES)
        Notify (\_SB.PCI0.EHC1, 0x02)
      }
      If (LEqual (\_SB.PCI0.XHC1.PMES, 1))
      {
        Store (1, \_SB.PCI0.XHC1.PMES)
        Notify (\_SB.PCI0.XHC1, 0x02)
      }
      If (LEqual (\_SB.PCI0.SEC0.PMES, 1))
      {
        Or (\_SB.PCI0.SEC0.PMES, Zero, \_SB.PCI0.SEC0.PMES)
        Notify (\_SB.PCI0.SEC0, 0x02)
      }
    }
  } //  Device (GPED)

  //--------------------
  //  GPIO
  //--------------------
  Device (GPO0)
  {
    Name (_ADR, 0)
    Name (_HID, "INT33FC")
    Name (_CID, "INT33B2")
    Name (_DDN, "ValleyView2 General Purpose Input/Output (GPIO) controller" )
    Name (_UID, 1)
    Method (_CRS, 0x0, Serialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x0FED0C000, 0x00001000)
        Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , ) {49}

      })
      Return (RBUF)
    }

    Method (_STA, 0x0, NotSerialized)
    {
      //
      // GPO driver will report present if any of below New IO bus exist
      //
      If (LOr(LEqual(L11D, 0), LEqual(L12D, 0))) // LPIO1 PWM #1 or #2 exist
      { Return(0xF) }
      If (LOr(LEqual(L13D, 0), LEqual(L14D, 0))) // LPIO1 HS-UART #1 or #2 exist
      { Return(0xF) }
      If (LOr(LEqual(L15D, 0), LEqual(SD1D, 0))) // LPIO1 SPI or SCC SDIO #1 exist
      { Return(0xF) }
      If (LOr(LEqual(SD2D, 0), LEqual(SD3D, 0))) // SCC SDIO #2 or #3 exist
      { Return(0xF) }
      If (LOr(LEqual(L21D, 0), LEqual(L22D, 0))) // LPIO2 I2C #1 or #2 exist
      { Return(0xF) }
      If (LOr(LEqual(L23D, 0), LEqual(L24D, 0))) // LPIO2 I2C #3 or #4 exist
      { Return(0xF) }
      If (LOr(LEqual(L25D, 0), LEqual(L26D, 0))) // LPIO2 I2C #5 or #6 exist
      { Return(0xF) }
      If (LEqual(L27D, 0))                       // LPIO2 I2C #7 exist
      { Return(0xF) }

      Return(0x0)
    }

    // Track status of GPIO OpRegion availability for this controller
    Name(AVBL, 0)
    Method(_REG,2)
    {
      If (Lequal(Arg0, 8))
      {
        Store(Arg1, ^AVBL)
      }
    }

    OperationRegion(GPOP, SystemIo, \GPBS, 0x50)
      Field(GPOP, ByteAcc, NoLock, Preserve) {
      Offset(0x28), // cfio_ioreg_SC_GP_LVL_63_32_ - [GPIO_BASE_ADDRESS] + 28h
          ,  21,
      BTD3,  1,     //This field is not used. Pin not defined in schematics. Closest is GPIO_S5_35 - COMBO_BT_WAKEUP
      Offset(0x48), // cfio_ioreg_SC_GP_LVL_95_64_ - [GPIO_BASE_ADDRESS] + 48h
          ,  30,
      SHD3,  1      //GPIO_S0_SC_95 - SENS_HUB_RST_N
    }



  }   //  Device (GPO0)

  Device (GPO1)
  {
    Name (_ADR, 0)
    Name (_HID, "INT33FC")
    Name (_CID, "INT33B2")
    Name (_DDN, "ValleyView2 GPNCORE controller" )
    Name (_UID, 2)
    Method (_CRS, 0x0, Serialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x0FED0D000, 0x00001000)
        Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , ) {48}
      })
      Return (RBUF)
    }

    Method (_STA, 0x0, NotSerialized)
    {
      Return(\_SB.GPO0._STA)
    }
  }   //  Device (GPO1)

  Device (GPO2)
  {
    Name (_ADR, 0)
    Name (_HID, "INT33FC")
    Name (_CID, "INT33B2")
    Name (_DDN, "ValleyView2 GPSUS controller" )
    Name (_UID, 3)
    Method (_CRS, 0x0, Serialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x0FED0E000, 0x00001000)
        Interrupt(ResourceConsumer, Level, ActiveLow, Shared, , , ) {50}
      })
      Return (RBUF)
    }

    Method (_STA, 0x0, NotSerialized)
    {
      Return(^^GPO0._STA)
    }

    // Track status of GPIO OpRegion availability for this controller
    Name(AVBL, 0)
    Method(_REG,2)
    {
      If (Lequal(Arg0, 8))
      {
        Store(Arg1, ^AVBL)
      }
    }
    //Manipulate GPIO line using GPIO operation regions.
    Name (GMOD, ResourceTemplate ()     //One method of creating a Connection for OpRegion accesses in Field definitions
    {
      //is creating a named object that refers to the connection attributes
      GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionOutputOnly, "\\_SB.GPO2") {21}  //sus 21+128 BT+WLAN_ENABLE
    })

  OperationRegion(GPOP, SystemIo, \GPBS, 0x100)
  Field(GPOP, ByteAcc, NoLock, Preserve) {
      Offset(0x88),  // cfio_ioreg_SUS_GP_LVL_31_0_ - [GPIO_BASE_ADDRESS] + 88h
          ,  20,
      WFD3,  1
    }


  }   //  Device (GPO2)
  include ("PchScc.asl")
  include ("PchLpss.asl")

         Scope(I2C7)
  {

  } //End Scope(I2C7)

} // end Scope(\_SB)

Name(PICM, 0)   // Global Name, returns current Interrupt controller mode; updated from _PIC control method

