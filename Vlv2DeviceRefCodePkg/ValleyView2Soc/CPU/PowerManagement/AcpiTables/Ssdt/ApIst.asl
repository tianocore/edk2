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


 Filename:      APIST.ASL

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
        "APIST.aml",
        "SSDT",
        1,
        "PmRef",
        "ApIst",
        0x3000
        )
{
        External(\_PR.CPU0._PSS, MethodObj)
        External(\_PR.CPU0._PCT, MethodObj)
        External(\_PR.CPU0._PPC, IntObj)
        External(\_PR.CPU0._PSD, MethodObj)
        External(\_PR.CPU1, DeviceObj)
        External(\_PR.CPU2, DeviceObj)
        External(\_PR.CPU3, DeviceObj)
        External (CFGD)
        External (PDC0)

        Scope(\_PR.CPU1)
        {
                Method(_PPC,0)
                {
                        Return(\_PR.CPU0._PPC)  // Return P0 _PPC value.
                }

                Method(_PCT,0)
                {
                        Return(\_PR.CPU0._PCT)  // Return P0 _PCT.
                }

                Method(_PSS,0)
                {
                        //Return the same table as CPU0 for CMP cases.
                        Return(\_PR.CPU0._PSS)
                }

                // The _PSD object provides information to the OSPM related
                // to P-State coordination between processors in a multi-processor
                // configurations.
                //
                Method(_PSD,0)
                {
                        Return(\_PR.CPU0._PSD)  // Return P0 _PSD.
                }
        }

        Scope(\_PR.CPU2)
        {
                Method(_PPC,0)
                {
                        Return(\_PR.CPU0._PPC)  // Return P0 _PPC value.
                }

                Method(_PCT,0)
                {
                        Return(\_PR.CPU0._PCT)  // Return P0 _PCT.
                }

                Method(_PSS,0)
                {
                        //Return the same table as CPU0 for CMP cases.
                        Return(\_PR.CPU0._PSS)
                }

                // The _PSD object provides information to the OSPM related
                // to P-State coordination between processors in a multi-processor
                // configurations.
                //
                Method(_PSD,0)
                {
                        Return(\_PR.CPU0._PSD)  // Return P0 _PSD.
                }
        }

        Scope(\_PR.CPU3)
        {
                Method(_PPC,0)
                {
                        Return(\_PR.CPU0._PPC)  // Return P0 _PPC value.
                }

                Method(_PCT,0)
                {
                        Return(\_PR.CPU0._PCT)  // Return P0 _PCT.
                }

                Method(_PSS,0)
                {
                        //Return the same table as CPU0 for CMP cases.
                        Return(\_PR.CPU0._PSS)
                }

                // The _PSD object provides information to the OSPM related
                // to P-State coordination between processors in a multi-processor
                // configurations.
                //
                Method(_PSD,0)
                {
                        Return(\_PR.CPU0._PSD)  // Return P0 _PSD.
                }
        }
} // End of Definition Block
