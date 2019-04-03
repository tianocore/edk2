/** @file
CPU T-state control methods

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
    "CPU0TST.aml",
    "SSDT",
    0x01,
    "SsgPmm",
    "Cpu0Tst",
    0x0013
    )
{
    External (PDC0, IntObj)
    External (CFGD, FieldUnitObj)
    External(\_PR.CPU0, DeviceObj)
    External(_PSS)

    Scope(\_PR.CPU0)
    {
        Method(_TPC,0)
        {
            Return(ZERO)   // Return All States Available.
        }

        Name(TPTC, ResourceTemplate()
        {
            Memory32Fixed(ReadOnly, 0, 0, FIX1) // IO APIC
        })

        //
        // If OSPM is capable of direct access to on demand throttling MSR,
        // we use MSR method;otherwise we use IO method.
        //
        //
        // PDCx[2] = Indicates whether OSPM is capable of direct access to
        // on demand throttling MSR.
        //
        Method(_PTC, 0)
        {
          If(And(PDC0, 0x0004))
          {
            Return(Package() // MSR Method
            {
              ResourceTemplate(){Register(FFixedHW, 0, 0, 0)},
              ResourceTemplate(){Register(FFixedHW, 0, 0, 0)}
            }
            )
          }
          Return(Package() // IO Method
          {
            //
            // PM IO base ("PMBALVL0" will be updated at runtime)
            //
            ResourceTemplate(){Register(SystemIO, 4, 1, 0x304C564C41424D50)},
            ResourceTemplate(){Register(SystemIO, 4, 1, 0x304C564C41424D50)}
          }
          )
        }

        //
        // _TSS returned package for IO Method
        //
        Name(TSSI, Package()
        {
          Package(){100, 1000, 0, 0x00, 0}
        }
        )
        //
        // _TSS returned package for MSR Method
        //
        Name(TSSM, Package()
        {
          Package(){100, 1000, 0, 0x00, 0}
        }
        )

        Method(_TSS, 0)
        {
          //
          // If OSPM is capable of direct access to on demand throttling MSR,
          // we report TSSM;otherwise report TSSI.
          //
          If(And(PDC0, 0x0004))
          {
            Return(TSSM)
          }
          Return(TSSI)
        }

        Method(_TSD, 0)
        {
          //
          // If CMP is suppored, we report the dependency with two processors
          //
          If(LAnd(And(CFGD, 0x1000000), LNot(And(PDC0, 4))))
          {
            Return(Package()
            {
              Package()
              {
                5,    // # entries.
                0,    // Revision.
                0,    // Domain #.
                0xFD, // Coord Type- SW_ANY
                2     // # processors.
              }
            }
            )
          }
          //
          // Otherwise, we report the dependency with one processor
          //
          Return(Package()
          {
            Package()
            {
              5,        // # entries.
              0,        // Revision.
              0,        // Domain #.
              0xFC,     // Coord Type- SW_ALL
              1         // # processors.
            }
          }
          )
        }
    }
}
