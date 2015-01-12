/*-----------------------------------------------------------------------------
-------------------------------------------------------------------------------


 Intel Silvermont Processor Power Management BIOS Reference Code

 Copyright (c) 2006 - 2014, Intel Corporation

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


 Filename:    CPU0IST.ASL

 Revision:    Refer to Readme

 Date:        Refer to Readme

--------------------------------------------------------------------------------
-------------------------------------------------------------------------------

 This Processor Power Management BIOS Source Code is furnished under license
 and may only be used or copied in accordance with the terms of the license.
 The information in this document is furnished for informational use only, is
 subject to change without notice, and should not be construed as a commitment
 by Intel Corporation. Intel Corporation assumes no responsibility or liability
 for any errors or inaccuracies that may appear in this document or any
 software that may be provided in association with this document.

 Except as permitted by such license, no part of this document may be
 reproduced, stored in a retrieval system, or transmitted in any form or by
 any means without the express written consent of Intel Corporation.

 WARNING: You are authorized and licensed to install and use this BIOS code
 ONLY on an IST PC. This utility may damage any system that does not
 meet these requirements.

    An IST PC is a computer which
    (1) Is capable of seamlessly and automatically transitioning among
    multiple performance states (potentially operating at different
    efficiency ratings) based upon power source changes, END user
    preference, processor performance demand, and thermal conditions; and
    (2) Includes an Intel Pentium II processors, Intel Pentium III
    processor, Mobile Intel Pentium III Processor-M, Mobile Intel Pentium 4
    Processor-M, Intel Pentium M Processor, or any other future Intel
    processors that incorporates the capability to transition between
    different performance states by altering some, or any combination of,
    the following processor attributes: core voltage, core frequency, bus
    frequency, number of processor cores available, or any other attribute
    that changes the efficiency (instructions/unit time-power) at which the
    processor operates.

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

NOTES:
    (1) <TODO> - IF the trap range and port definitions do not match those
    specified by this reference code, this file must be modified IAW the
    individual implmentation.

--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/


DefinitionBlock (
    "CPU0IST.aml",
    "SSDT",
    0x01,
    "PmRef",
    "Cpu0Ist",
    0x3000
    )
{
    External (\_PR.CPU0, DeviceObj)
    External (PDC0)
    External (CFGD)

    Scope(\_PR.CPU0)
    {
        //OperationRegion (DEB0, SystemIO, 0x80, 1)    //DBG
        //Field (DEB0, ByteAcc,NoLock,Preserve)        //DBG
        //{ DBG8, 8,}                                  //DBG

        Name(_PPC, 0)        // Initialize as All States Available.

        // NOTE:  For CMP systems; this table is not loaded unless
        //      the required driver support is present.
        //      So, we do not check for those cases here.
        //
        //   CFGD[0] = GV3 Capable/Enabled
        //   PDCx[0]  = OS Capable of Hardware P-State control
        //
        Method(_PCT,0)
        {
            If(LAnd(And(CFGD,0x0001), And(PDC0,0x0001)))
            {
                //Store(0xA0,DBG8) //DBG
                Return(Package()    // Native Mode
                {
                    ResourceTemplate(){Register(FfixedHW, 0, 0, 0)},
                    ResourceTemplate(){Register(FfixedHW, 0, 0, 0)}
                })
            }
            // @NOTE: IO Trap is not supported. Therefore should not expose any IO interface for _PCT
            // For all other cases, report control through the
            // SMI interface.  (The port used for SMM control is fixed up
            // by the initialization code.)
            //
            Return(Package()        // SMM Mode
            {
               ResourceTemplate(){Register(FfixedHW, 0, 0, 0)},
               ResourceTemplate(){Register(FfixedHW, 0, 0, 0)}
            })
        }


        // NOTE:  For CMP systems; this table is not loaded if MP
        //      driver support is not present or P-State are disabled.
        //
        Method(_PSS,0)
        {
            //
            // Report NSPP if:
            //   (1) GV3 capable (Not checked, see above.)
            //   (2) Driver support direct hardware control
            //   (3) MP driver support present (Not checked, see above.)
            // else;
            //   Report SPSS
            //
            //   PDCx[0]  = OS Capable of Hardware P-State control
            //
            If(And(PDC0,0x0001)){
                //Store(0xB0,DBG8) //DBG
                Return(NPSS)
            }
            //Store(0xBF,DBG8) //DBG
            // Otherwise, report SMM mode
            //
            Return(SPSS)

        }

        Name(SPSS,Package()
        {
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000}
        })

        Name(NPSS,Package()
        {
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000}
        })

        // The _PSD object provides information to the OSPM related
        // to P-State coordination between processors in a multi-processor
        // configurations.
        //
        Method(_PSD,0)
        {
            //
            // IF CMP is supported/enabled
            //   IF quad core processor
            //     IF PDC[11]
            //         Report 4 processors and HW_ALL as the coordination type
            //     ELSE
            //         Report 4 processors and SW_ALL as the coordination type
            //   ELSE
            //     IF PDC[11]
            //         Report 2 processors and HW_ALL as the coordination type
            //     ELSE
            //         Report 2 processors and SW_ALL as the coordination type
            // ELSE
            //    Report 1 processor and SW_ALL as the coordination type
            //    (Domain 0)
            //
            //   CFGD[24] = Two or more cores enabled
            //   CFGD[23] = Four cores enabled
            //   PDCx[11] = Hardware coordination with hardware feedback
            //

            If(And(CFGD,0x1000000))    // CMP Enabled.
            {
              If(And(CFGD,0x800000))    // 2 or 4 process.
                {
                  If(And(PDC0,0x0800))
                  {
                      Return(Package(){    // HW_ALL
                        Package(){
                            5,              // # entries.
                            0,              // Revision.
                            0,              // Domain #.
                            0xFE,           // Coord Type- HW_ALL.
                            4               // # processors.
                        }
                      })
                  } // If(And(PDC0,0x0800))
                   Return(Package(){        // SW_ALL
                     Package(){
                        5,                  // # entries.
                        0,                  // Revision.
                        0,                  // Domain #.
                        0xFC,               // Coord Type- SW_ALL.
                        4                   // # processors.
                     }
                    })
                } else {
                  Return(Package(){        // HW_ALL
                      Package(){
                          5,                  // # entries.
                          0,                  // Revision.
                          0,                  // Domain #.
                          0xFE,               // Coord Type- HW_ALL.
                          2                   // # processors.
                      }
                  })
                }
            }    // If(And(CFGD,0x1000000))    // CMP Enabled.

            Return(Package(){              // SW_ALL
                Package(){
                    5,                        // # entries.
                    0,                        // Revision.
                    0,                        // Domain #.
                    0xFC,                     // Coord Type- SW_ALL.
                    1                         // # processors.
                }
            })
        } // Method(_PSD,0)
    } // Scope(\_PR.CPU0)
} // End of Definition Block


