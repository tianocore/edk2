/** @file
Contains root level name space objects for the platform

Copyright (c) 2013-2019 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// OS TYPE DEFINITION
//
#define WINDOWS_XP          0x01
#define WINDOWS_XP_SP1      0x02
#define WINDOWS_XP_SP2      0x04
#define WINDOWS_2003        0x08
#define WINDOWS_Vista       0x10
#define WINDOWS_WIN7        0x11
#define WINDOWS_WIN8        0x12
#define WINDOWS_WIN8_1      0x13
#define LINUX               0xF0

//
// GPIO Interrupt Connection Resource Descriptor (GpioInt) usage.
// GpioInt() descriptors maybe used in this file and included .asi files.
//
// The mapping below was provided by the first OS user that requested
// GpioInt() support.
// Other OS users that need GpioInt() support must use the following mapping.
//
#define QUARK_GPIO8_MAPPING         0x00
#define QUARK_GPIO9_MAPPING         0x01
#define QUARK_GPIO_SUS0_MAPPING     0x02
#define QUARK_GPIO_SUS1_MAPPING     0x03
#define QUARK_GPIO_SUS2_MAPPING     0x04
#define QUARK_GPIO_SUS3_MAPPING     0x05
#define QUARK_GPIO_SUS4_MAPPING     0x06
#define QUARK_GPIO_SUS5_MAPPING     0x07
#define QUARK_GPIO0_MAPPING         0x08
#define QUARK_GPIO1_MAPPING         0x09
#define QUARK_GPIO2_MAPPING         0x0A
#define QUARK_GPIO3_MAPPING         0x0B
#define QUARK_GPIO4_MAPPING         0x0C
#define QUARK_GPIO5_MAPPING         0x0D
#define QUARK_GPIO6_MAPPING         0x0E
#define QUARK_GPIO7_MAPPING         0x0F

DefinitionBlock (
  "Platform.aml",
  "DSDT",
  1,
  "INTEL ",
  "QuarkNcS",
  3)
{
    //
    // Global Variables
    //
    Name(\GPIC, 0x0)

    //
    // Port 80
    //
    OperationRegion (DBG0, SystemIO, 0x80, 1)
    Field (DBG0, ByteAcc, NoLock, Preserve)
    { IO80,8 }

    //
    // Access CMOS range
    //
    OperationRegion (ACMS, SystemIO, 0x72, 2)
    Field (ACMS, ByteAcc, NoLock, Preserve)
    { INDX, 8, DATA, 8 }

    //
    // Global NVS Memory Block
    //
    OperationRegion (MNVS, SystemMemory, 0xFFFF0000, 512)
    Field (MNVS, ByteAcc, NoLock, Preserve)
    {
      OSTP, 32,
      CFGD, 32,
      HPEA, 32,  // HPET Enabled ?

      P1BB, 32,  // Pm1blkIoBaseAddress;
      PBAB, 32,  // PmbaIoBaseAddress;
      GP0B, 32,  // Gpe0blkIoBaseAddress;
      GPAB, 32,  // GbaIoBaseAddress;

      SMBB, 32,  // SmbaIoBaseAddress;
      NRV1, 32,  // GNVS reserved field 1.
      WDTB, 32,  // WdtbaIoBaseAddress;

      HPTB, 32,  // HpetBaseAddress;
      HPTS, 32,  // HpetSize;
      PEXB, 32,  // PciExpressBaseAddress;
      PEXS, 32,  // PciExpressSize;

      RCBB, 32,  // RcbaMmioBaseAddress;
      RCBS, 32,  // RcbaMmioSize;
      APCB, 32,  // IoApicBaseAddress;
      APCS, 32,  // IoApicSize;

      TPMP, 32,  // TpmPresent ?
      DBGP, 32,  // DBG2 Present?
      PTYP, 32,  // Set to one of EFI_PLATFORM_TYPE enums.
      ALTS, 32,  // Use alternate I2c SLA addresses.
    }

    OperationRegion (GPEB, SystemIO, 0x1100, 0x40)  //GPE Block
    Field (GPEB, AnyAcc, NoLock, Preserve)
    {
      Offset(0x10),
      SMIE, 32,                 // SMI Enable
      SMIS, 32,                 // SMI Status
    }

    //
    //  Processor Objects
    //
    Scope(\_PR) {
        //
        // IO base will be updated at runtime with search key "PRIO"
        //
        Processor (CPU0, 0x01, 0x4F495250, 0x06) {}
    }

    //
    // System Sleep States
    //
    Name (\_S0,Package (){0,0,0,0})
    Name (\_S3,Package (){5,0,0,0})
    Name (\_S4,Package (){6,0,0,0})
    Name (\_S5,Package (){7,0,0,0})

    //
    //  General Purpose Event
    //
    Scope(\_GPE)
    {
        //
        // EGPE generated GPE
        //
        Method(_L0D, 0x0, NotSerialized)
        {
            //
            // Check EGPE for this wake event
            //
            Notify (\_SB.SLPB, 0x02)

        }

        //
        // GPIO generated GPE
        //
        Method(_L0E, 0x0, NotSerialized)
        {
            //
            // Check GPIO for this wake event
            //
            Notify (\_SB.PWRB, 0x02)

        }

        //
        // SCLT generated GPE
        //
        Method(_L0F, 0x0, NotSerialized)
        {
            //
            // Check SCLT for this wake event
            //
            Notify (\_SB.PCI0.SDIO, 0x02)
            Notify (\_SB.PCI0.URT0, 0x02)
            Notify (\_SB.PCI0.USBD, 0x02)
            Notify (\_SB.PCI0.EHCI, 0x02)
            Notify (\_SB.PCI0.OHCI, 0x02)
            Notify (\_SB.PCI0.URT1, 0x02)
            Notify (\_SB.PCI0.ENT0, 0x02)
            Notify (\_SB.PCI0.ENT1, 0x02)
            Notify (\_SB.PCI0.SPI0, 0x02)
            Notify (\_SB.PCI0.SPI1, 0x02)
            Notify (\_SB.PCI0.GIP0, 0x02)

        }

        //
        // Remote Management Unit generated GPE
        //
        Method(_L10, 0x0, NotSerialized)
        {
            //
            // Check Remote Management Unit for this wake event.
            //
        }

        //
        // PCIE generated GPE
        //
        Method(_L11, 0x0, NotSerialized)
        {
            //
            // Check PCIE for this wake event
            //
            Notify (\_SB.PCI0.PEX0, 0x02)
            Notify (\_SB.PCI0.PEX1, 0x02)
        }
    }

    //
    // define Sleeping button as mentioned in ACPI spec 2.0
    //
    Device (\_SB.SLPB)
    {
        Name (_HID, EISAID ("PNP0C0E"))
        Method (_PRW, 0, NotSerialized)
        {
            Return (Package (0x02) {0x0D,0x04})
        }
    }

    //
    // define Power Button
    //
     Device (\_SB.PWRB)
    {
        Name (_HID, EISAID ("PNP0C0C"))
        Method (_PRW, 0, NotSerialized)
        {
            Return (Package (0x02) {0x0E,0x04})
        }
    }
    //
    // System Wake up
    //
    Method(_WAK, 1, Serialized)
    {
       // Do nothing here
       Return (0)
    }

    //
    // System sleep down
    //
    Method (_PTS, 1, NotSerialized)
    {
        // Get ready for S3 sleep
        if (Lequal(Arg0,3))
        {
                Store(0xffffffff,SMIS)     // clear SMI status
                Store(SMIE, Local0)        // SMI Enable
                Or(Local0,0x4,SMIE)        // Generate SMI on sleep
        }
    }

    //
    // Determing PIC mode
    //
    Method(\_PIC, 1, NotSerialized)
    {
        Store(Arg0,\GPIC)
    }

    //
    //  System Bus
    //
    Scope(\_SB)
    {
        Device(PCI0)
        {
            Name(_HID,EISAID ("PNP0A08"))          // PCI Express Root Bridge
            Name(_CID,EISAID ("PNP0A03"))          // Compatible PCI Root Bridge

            Name(_ADR,0x00000000)                  // Device (HI WORD)=0, Func (LO WORD)=0
            Method (_INI)
            {
                Store(LINUX, OSTP)                 // Set the default os is Linux
                If (CondRefOf (_OSI))
                {
                    //
                    //_OSI is supported, so it is WinXp or Win2003Server
                    //
                    If (\_OSI("Windows 2001"))
                    {
                        Store (WINDOWS_XP, OSTP)
                    }
                    If (\_OSI("Windows 2001 SP1"))
                    {
                        Store (WINDOWS_XP_SP1, OSTP)
                    }
                    If (\_OSI("Windows 2001 SP2"))
                    {
                        Store (WINDOWS_XP_SP2, OSTP)
                    }
                    If (\_OSI("Windows 2001.1"))
                    {
                        Store (WINDOWS_2003, OSTP)
                    }
                    If (\_OSI("Windows 2006"))
                    {
                        Store (WINDOWS_Vista, OSTP)
                    }
                    If (\_OSI("Windows 2009"))
                    {
                        Store (WINDOWS_WIN7, OSTP)
                    }
                    If (\_OSI("Windows 2012"))
                    {
                        Store (WINDOWS_WIN8, OSTP)
                    }
                    If (\_OSI("Windows 2013"))
                    {
                        Store (WINDOWS_WIN8_1, OSTP)
                    }
                    If (\_OSI("Linux"))
                    {
                      Store (LINUX, OSTP)
                    }
                }
            }

            Include ("PciHostBridge.asi")     // PCI0 Host bridge
            Include ("QNC.asi")               // QNC miscellaneous
            Include ("PcieExpansionPrt.asi")  // PCIe expansion bridges/devices
            Include ("QuarkSouthCluster.asi") // Quark South Cluster devices
            Include ("QNCLpc.asi")            // LPC bridge device
            Include ("QNCApic.asi")           // QNC I/O Apic device

        }

        //
        // Include asi files for I2C and SPI onboard devices.
        // Devices placed here instead of below relevant controllers.
        // Hardware topology information is maintained by the
        // ResourceSource arg to the I2CSerialBus/SPISerialBus macros
        // within the device asi files.
        //
        Include ("Tpm.asi")          // TPM device.
        Include ("CY8C9540A.asi")    // CY8C9540A 40Bit I/O Expander & EEPROM
        Include ("PCAL9555A.asi")    // NXP PCAL9555A I/O expander.
        Include ("PCA9685.asi")      // NXP PCA9685 PWM/LED controller.
        Include ("CAT24C08.asi")     // ONSEMI CAT24C08 I2C 8KB EEPROM.
        Include ("AD7298.asi")       // Analog devices AD7298 ADC.
        Include ("ADC108S102.asi")   // TI ADC108S102 ADC.
        Include ("GpioClient.asi")   // Software device to expose GPIO
    }
}
