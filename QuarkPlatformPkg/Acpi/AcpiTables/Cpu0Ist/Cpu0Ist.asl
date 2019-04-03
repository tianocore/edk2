/** @file
CPU EIST control methods

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
    "CPU0IST.aml",
    "SSDT",
    0x01,
    "SsgPmm",
    "Cpu0Ist",
    0x0012
    )
{
    External (PDC0, IntObj)
    External (CFGD, FieldUnitObj)
    External(\_PR.CPU0, DeviceObj)

    Scope(\_PR.CPU0)
    {
        Method(_PPC,0)
        {
            Return(ZERO)   // Return All States Available.
        }

        Method(_PCT,0)
        {
            //
            // If GV3 is supported and OSPM is capable of direct access to
            // performance state MSR, we use MSR method
            //
            //
            // PDCx[0] = Indicates whether OSPM is capable of direct access to
            // performance state MSR.
            //
            If(LAnd(And(CFGD,0x0001), And(PDC0,0x0001)))
            {
                Return(Package()    // MSR Method
                {
                    ResourceTemplate(){Register(FFixedHW, 0, 0, 0)},
                    ResourceTemplate(){Register(FFixedHW, 0, 0, 0)}
                })

            }

            //
            // Otherwise, we use smi method
            //
            Return(Package()    // SMI Method
                {
                  ResourceTemplate(){Register(SystemIO,16,0,0xB2)},
                  ResourceTemplate(){Register(SystemIO, 8,0,0xB3)}
                })
        }

        Method(_PSS,0)
        {
            //
            // If OSPM is capable of direct access to performance state MSR,
            // we report NPSS, otherwise, we report SPSS.
            If (And(PDC0,0x0001))
            {
                Return(NPSS)
            }

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
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000}
        })

        Method(_PSD,0)
        {
          //
          // If CMP is suppored, we report the dependency with two processors
          //
          If(And(CFGD,0x1000000))
          {
            //
            // If OSPM is capable of hardware coordination of P-states, we report
            // the dependency with hardware coordination.
            //
            // PDCx[11] = Indicates whether OSPM is capable of hardware coordination of P-states
            //
            If(And(PDC0,0x0800))
            {
              Return(Package(){
                Package(){
                  5,  // # entries.
                  0,  // Revision.
                  0,  // Domain #.
                  0xFE,  // Coord Type- HW_ALL.
                  2  // # processors.
                }
              })
            }

            //
            // Otherwise, the dependency with OSPM coordination
            //
            Return(Package(){
              Package(){
                5,    // # entries.
                0,    // Revision.
                0,    // Domain #.
                0xFC, // Coord Type- SW_ALL.
                2     // # processors.
              }
            })
          }

          //
          //  Otherwise, we report the dependency with one processor
          //
          Return(Package(){
            Package(){
              5,      // # entries.
              0,      // Revision.
              0,      // Domain #.
              0xFC,   // Coord Type- SW_ALL.
              1       // # processors.
            }
          })
        }
    }
}
