/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved   *;
;
; This program and the accompanying materials are licensed and made available under
; the terms and conditions of the BSD License that accompanies this distribution.
; The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/


// THERMAL.ASL represents a Thermal Zone to be used for testing on the
// Customer Reference Boards.

External(\_SB.DPTF.CTOK, MethodObj)

Scope(\_TZ)
{

  // Thermal Zone 1 = DTS Thermal Zone.

  ThermalZone(TZ01)
  {
    // Return the temperature at which the OS must perform a Critcal
    // Shutdown.

    Method(_CRT,0,Serialized)
    {
      Return(Add(2732,Multiply(CRTT,10)))
    }

    // Notifies ASL Code the current cooling mode.
    //      0 - Active cooling
    //      1 - Passive cooling

    Method(_SCP,1,Serialized)
    {
      Store(Arg0,CTYP)
    }

    // _TMP (Temperature)
    //
    // Return the highest of the CPU temperatures to the OS.
    //
    // Arguments: (0)
    //   None
    // Return Value:
    //   An Integer containing the current temperature of the thermal zone (in tenths of degrees Kelvin)
    //
    Method(_TMP,0,Serialized)
    {
      If(DTSE)
      {
        If(LGreater(DTS2, DTS1))
        {
          Store(DTS2,Local0)
        } else
        {
          Store(DTS1,Local0)
        }
        Return(Add(2732,Multiply(Local0,10)))
        //
        // Else return a static value if both EC and DTS are unavailable.
        //
      } Else
      {
        Return(3000) // (3000-2732)/10 = 26.8 degree C
      }
    }

    // Return the Processor(s) used for Passive Cooling.

    Method(_PSL,0,Serialized)
    {
      If(LEqual(MPEN, 4))
      {
        //  CMP - Throttling controls all four logical CPUs.
        Return(Package() {\_PR.CPU0,\_PR.CPU1,\_PR.CPU2,\_PR.CPU3})
      }

      If(MPEN)
      {
        //  CMP - Throttling controls both CPUs.

        Return(Package() {\_PR.CPU0,\_PR.CPU1})
      }

      Return(Package() {\_PR.CPU0})
    }

    // Returns the temperature at which the OS initiates CPU throttling.

    Method(_PSV,0,Serialized)
    {
      Return(Add(2732,Multiply(PSVT,10)))
    }

    // Returns TC1 value used in the passive cooling formula.

    Method(_TC1,0,Serialized)
    {
      Return(TC1V)
    }

    // Returns TC2 value used in the passive cooling formula.

    Method(_TC2,0,Serialized)
    {
      Return(TC2V)
    }

    // Returns the sampling period used in the passive cooling formula.

    Method(_TSP,0,Serialized)
    {
      Return(TSPV)
    }

    // Returns Hot Temperature

    Method(_HOT,0,Serialized)
    {
      Subtract(CRTT, 5, Local0)
      Return(Add(2732,Multiply(Local0,10)))
    }
  }
}
