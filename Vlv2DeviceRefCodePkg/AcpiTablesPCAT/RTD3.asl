/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
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
DefinitionBlock (
  "Rtd3.aml",
  "SSDT",
  1,
  "AcpiRef",
  "Msg_Rtd3",
  0x1000
)
{
  External(RTD3)             //flag if RTD3 is enabled

  If(LEqual(RTD3,1))
  {
    Scope (\_SB)
    {
      Name(OSCI, 0)  // \_SB._OSC DWORD2 input
      Name(OSCO, 0)  // \_SB._OSC DWORD2 output

      //Arg0 -- A buffer containing UUID
      //Arg1 -- An Interger containing a Revision ID of the buffer format
      //Arg2 -- An interger containing a count of entries in Arg3
      //Arg3 -- A buffer containing a list of DWORD capacities
      Method(_OSC, 4, NotSerialized)
      {
        // Check for proper UUID
        If(LEqual(Arg0, ToUUID("0811B06E-4A27-44F9-8D60-3CBBC22E7B48")))
        {
          CreateDWordField(Arg3,0,CDW1)     //bit1,2 is always clear
          CreateDWordField(Arg3,4,CDW2)     //Table 6-147 from ACPI spec

          Store(CDW2, OSCI)                 // Save DWord2
          Or(OSCI, 0x4, OSCO)               // Only allow _PR3 support

          If(LNotEqual(Arg1,One))
          {
            Or(CDW1,0x08,CDW1)            // Unknown revision
          }

          If(LNotEqual(OSCI, OSCO))
          {
            Or(CDW1,0x10,CDW1)            // Capabilities bits were masked
          }

          Store(OSCO, CDW2)                 // Replace DWord2
          Return(Arg3)
        } Else
        {
          Or(CDW1,4,CDW1)                   // Unrecognized UUID
          Return(Arg3)
        }
      }// End _OSC
    }
  }//end of RTD3 condition


  //USB RTD3 code
  If(LEqual(RTD3,1))
  {
    Scope(\_SB.PCI0.EHC1.HUBN.PR01.PR13)
    {
      Name(_PR0, Package() {\PR34})
      Name(_PR3, Package() {\PR34})

      Method(_S0W, 0)
      {
        If(And(\_SB.OSCO, 0x04))              // PMEs can be genrated from D3cold
        {
          Return(4)                         // OS comprehends D3cold, as described via \_SB._OSC
        } Else
        {
          Return(3)
        }
      } // End _S0W
    }

    Scope(\_SB.PCI0.EHC1.HUBN.PR01.PR14)
    {
      Name(_PR0, Package() {\PR34})
      Name(_PR3, Package() {\PR34})

      Method(_S0W, 0)
      {
        If(And(\_SB.OSCO, 0x04))
        {
          Return(4)
        } Else
        {
          Return(3)
        }
      } // End _S0W
    }


    Scope(\_SB.PCI0.EHC1.HUBN.PR01.PR15)
    {
      Name(_PR0, Package() {\PR56})
      Name(_PR3, Package() {\PR56})

      Method(_S0W, 0)
      {
        If(And(\_SB.OSCO, 0x04))
        {
          Return(4)
        } Else
        {
          Return(3)
        }
      } // End _S0W
    }

    Scope(\_SB.PCI0.EHC1.HUBN.PR01.PR16)
    {
      Name(_PR0, Package() {\PR56})
      Name(_PR3, Package() {\PR56})

      Method(_S0W, 0)
      {
        If(And(\_SB.OSCO, 0x04))
        {
          Return(4)
        } Else
        {
          Return(3)
        }
      } // End _S0W
    }

    Scope(\_SB.PCI0.XHC1)                              // XHCI host only controller
    {

      Method(_PS0,0,Serialized)                      // set device into D0 state
      {
      }

      Method(_PS3,0,Serialized)                      // place device into D3H state
      {
        //write to PMCSR
      }

      Method(_DSW, 3,Serialized)                     // enable or disable the device’s ability to wake a sleeping system.
      {
      }
    }

    Scope(\_SB.PCI0.XHC1.RHUB.HS01)
    {

    }

    Scope(\_SB.PCI0.XHC1.RHUB.SSP1)
    {

    }

    Scope(\_SB.PCI0.XHC2)                              // OTG
    {

      Method(_PS0,0,Serialized)                      // set device into D0 state
      {
      }

      Method(_PS3,0,Serialized)                      // place device into D3H state
      {
        //write to PMCSR
      }

      Method(_DSW, 3,Serialized)                      // enable or disable the device’s ability to wake a sleeping system.
      {
      }
    }

    Scope(\_SB.PCI0.XHC2.RHUB.HS01)
    {

    }

    Scope(\_SB.PCI0.XHC2.RHUB.SSP1)
    {

    }
  } //If(LEqual(RTD3,1)) USB

}//end of SSDT
