/*-----------------------------------------------------------------------------
-------------------------------------------------------------------------------


 Intel Silvermont Processor Power Management BIOS Reference Code

 Copyright (c) 2006 - 2014, Intel Corporation

  SPDX-License-Identifier: BSD-2-Clause-Patent


 Filename:      CPU0TST.ASL

 Revision:      Refer to Readme

 Date:          Refer to Readme

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
        efficiency ratings) based upon power source changes, end user
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

DefinitionBlock(
        "CPU0TST.aml",
        "SSDT",
        0x01,
        "PmRef",
        "Cpu0Tst",
        0x3000
        )
{
        External(\_PR.CPU0, DeviceObj)
        External(PDC0)
        External(CFGD)
        External(_PSS)

        Scope(\_PR.CPU0)
        {
                Name(_TPC, 0)   // All T-States are available

                //
                // T-State Control/Status interface
                //
                Method(_PTC, 0)
                {
                        //
                        // IF OSPM is capable of direct access to MSR
                        //    Report MSR interface
                        // ELSE
                        //    Report I/O interface
                        //
                        //  PDCx[2] = OSPM is capable of direct access to On
                        //              Demand throttling MSR
                        //
                        If(And(PDC0, 0x0004)) {
                                Return(Package() {
                                        ResourceTemplate(){Register(FFixedHW, 0, 0, 0)},
                                        ResourceTemplate(){Register(FFixedHW, 0, 0, 0)}
                                })
                        }

                }

                // _TSS package for I/O port based T-State control
                // "Power" fields are replaced with real values by the first
                // call of _TSS method.
                //
                Name(TSSI, Package() {
                                Package(){100, 1000, 0, 0x00, 0},
                                Package(){ 88,  875, 0, 0x0F, 0},
                                Package(){ 75,  750, 0, 0x0E, 0},
                                Package(){ 63,  625, 0, 0x0D, 0},
                                Package(){ 50,  500, 0, 0x0C, 0},
                                Package(){ 38,  375, 0, 0x0B, 0},
                                Package(){ 25,  250, 0, 0x0A, 0},
                                Package(){ 13,  125, 0, 0x09, 0}
                })

                // _TSS package for MSR based T-State control
                // "Power" fields are replaced with real values by the first
                // call of _TSS method.
                //
                Name(TSSM, Package() {
                                Package(){100, 1000, 0, 0x00, 0},
                                Package(){ 88,  875, 0, 0x1E, 0},
                                Package(){ 75,  750, 0, 0x1C, 0},
                                Package(){ 63,  625, 0, 0x1A, 0},
                                Package(){ 50,  500, 0, 0x18, 0},
                                Package(){ 38,  375, 0, 0x16, 0},
                                Package(){ 25,  250, 0, 0x14, 0},
                                Package(){ 13,  125, 0, 0x12, 0}
                })

                Name(TSSF, 0)   // Flag for TSSI/TSSM initialization

                Method(_TSS, 0)
                {
                        // Update "Power" fields of TSSI/TSSM with the LFM
                        // power data IF _PSS is available
                        //
                        IF (LAnd(LNot(TSSF),CondRefOf(_PSS)))
                        {
                                Store(_PSS, Local0)
                                Store(SizeOf(Local0), Local1)   // _PSS size
                                Decrement(Local1)               // Index of LFM
                                Store(DerefOf(Index(DerefOf(Index(Local0,Local1)),1)), Local2)  // LFM Power

                                Store(0, Local3)
                                While(LLess(Local3, SizeOf(TSSI)))
                                {
                                        Store(Divide(Multiply(Local2, Subtract(8, Local3)), 8),
                                              Local4)           // Power for this TSSI/TSSM entry
                                        Store(Local4,Index(DerefOf(Index(TSSI,Local3)),1))
                                        Store(Local4,Index(DerefOf(Index(TSSM,Local3)),1))
                                        Increment(Local3)
                                }
                                Store(Ones, TSSF)               // TSSI/TSSM are updated
                        }
                        //
                        // IF OSPM is capable of direct access to MSR
                        //    Report TSSM
                        // ELSE
                        //    Report TSSI
                        //
                        If(And(PDC0, 0x0004))
                        {
                                Return(TSSM)
                        }
                        Return(TSSI)
                }

              Method(_TDL, 0)
              {
                Store ("Cpu0: _TDL Called", Debug)
                Name ( LFMI, 0)
                Store (SizeOf(TSSM), LFMI)
                Decrement(LFMI)    // Index of LFM entry in TSSM
                Return(LFMI)
              }

                //
                // T-State Dependency
                //
                Method(_TSD, 0)
                {
                        //
      // IF four cores are supported/enabled && !(direct access to MSR)
                        //    Report 4 processors and SW_ANY as the coordination type
      // ELSE IF two cores are supported/enabled && !(direct access to MSR)
                        //    Report 2 processors and SW_ANY as the coordination type
                        // ELSE
                        //   Report 1 processor and SW_ALL as the coordination type
                        //
                        //  CFGD[23] = Four cores enabled
                        //  CFGD[24] = Two or more cores enabled
                        //  PDCx[2] = OSPM is capable of direct access to On
                        //              Demand throttling MSR
                        //
                        If(LAnd(And(CFGD,0x0800000),LNot(And(PDC0,4))))
                        {
                                Return(Package(){       // SW_ANY
                                        Package(){
                                                5,                // # entries.
                                                0,                // Revision.
                                                0,                // Domain #.
                                                0xFD,           // Coord Type- SW_ANY
                                                4                   // # processors.
                                        }
                                })
                        }
                        If(LAnd(And(CFGD,0x1000000),LNot(And(PDC0,4))))
                        {
                                Return(Package(){       // SW_ANY
                                        Package(){
                                                5,                // # entries.
                                                0,                // Revision.
                                                0,                // Domain #.
                                                0xFD,           // Coord Type- SW_ANY
                                                2                   // # processors.
                                        }
                                })
                        }
                        Return(Package(){               // SW_ALL
                                Package(){
                                        5,                        // # entries.
                                        0,                        // Revision.
                                        0,                        // Domain #.
                                        0xFC,                   // Coord Type- SW_ALL
                                        1                           // # processors.
                                }
                        })
                }
        }
} // End of Definition Block

