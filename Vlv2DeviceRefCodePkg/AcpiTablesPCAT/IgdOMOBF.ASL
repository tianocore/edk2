/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  IgdOMOBF.ASL

Abstract:

  IGD OpRegion/Software SCI Reference Code for the Baytrail Family.
  This file contains ASL code with the purpose of handling events
  i.e. hotkeys and other system interrupts.

--*/


// Notes:
// 1. The following routines are to be called from the appropriate event
//    handlers.
// 2. This code cannot comprehend the exact implementation in the OEM's BIOS.
//    Therefore, an OEM must call these methods from the existing event
//    handler infrastructure.  Details on when/why to call each method is
//    included in the method header under the "usage" section.


/************************************************************************;
;* ACPI Notification Methods
;************************************************************************/


/************************************************************************;
;*
;* Name:        PDRD
;*
;* Description: Check if the graphics driver is ready to process
;*              notifications and video extensions.
;*
;* Usage:       This method is to be called prior to performing any
;*              notifications or handling video extensions.
;*              Ex: If (PDRD()) {Return (FAIL)}
;*
;* Input:       None
;*
;* Output:      None
;*
;* References:  DRDY (Driver ready status), ASLP (Driver recommended
;*              sleep timeout value).
;*
;************************************************************************/

Method(PDRD)
{
  If(LNot(DRDY))
  {

    // Sleep for ASLP milliseconds if the driver is not ready.

    Sleep(ASLP)
  }

  // If DRDY is clear, the driver is not ready.  If the return value is
  // !=0, do not perform any notifications or video extension handling.

  Return(LNot(DRDY))
}


/************************************************************************;
;*
;* Name:        PSTS
;*
;* Description: Check if the graphics driver has completed the previous
;*              "notify" command.
;*
;* Usage:       This method is called before every "notify" command.  A
;*              "notify" should only be set if the driver has completed the
;*              previous command.  Else, ignore the event and exit the parent
;*              method.
;*              Ex: If (PSTS()) {Return (FAIL)}
;*
;* Input:       None
;*
;* Output:      None
;*
;* References:  CSTS (Notification status), ASLP (Driver recommended sleep
;*              timeout value).
;*
;************************************************************************/

Method(PSTS)
{
  If(LGreater(CSTS, 2))
  {
    // Sleep for ASLP milliseconds if the status is not "success,
    // failure, or pending"
    //
    Sleep(ASLP)
  }

  Return(LEqual(CSTS, 3))         // Return True if still Dispatched
}


/************************************************************************;
;*
;* Name:        GNOT
;*
;* Description: Call the appropriate methods to query the graphics driver
;*              status.  If all methods return success, do a notification of
;*              the graphics device.
;*
;* Usage:       This method is to be called when a graphics device
;*              notification is required (display switch hotkey, etc).
;*
;* Input:       Arg0 = Current event type:
;*                      1 = display switch
;*                      2 = lid
;*                      3 = dock
;*              Arg1 = Notification type:
;*                      0 = Re-enumeration
;*                      0x80 = Display switch
;*
;* Output:      Returns 0 = success, 1 = failure
;*
;* References:  PDRD and PSTS methods.  OSYS (OS version)
;*
;************************************************************************/

Method(GNOT, 2)
{
  // Check for 1. Driver loaded, 2. Driver ready.
  // If any of these cases is not met, skip this event and return failure.
  //
  If(PDRD())
  {
    Return(0x1)             // Return failure if driver not loaded.
  }

  Store(Arg0, CEVT)               // Set up the current event value
  Store(3, CSTS)                  // CSTS=BIOS dispatched an event

  If(LAnd(LEqual(CHPD, 0), LEqual(Arg1, 0)))      // Do not re-enum if driver supports hotplug
  {
    If(LOr(LGreater(OSYS, 2000), LLess(OSYS, 2006)))
    {
      //
      // WINXP requires that the entire PCI Bridge be re-enumerated.
      //
      Notify(\_SB.PCI0, Arg1)
    }
    Else
    {
      //
      // Re-enumerate the Graphics Device for non-XP operating systems.
      //
      Notify(\_SB.PCI0.GFX0, Arg1)
    }
  }

  Notify(\_SB.PCI0.GFX0,0x80)


  Return(0x0)                     // Return success
}


/************************************************************************;
;*
;* Name:        GHDS
;*
;* Description: Handle a hotkey display switching event (performs a
;*              Notify(GFX0, 0).
;*
;* Usage:       This method must be called when a hotkey event occurs and the
;*              purpose of that hotkey is to do a display switch.
;*
;* Input:       Arg0 = Toggle table number.
;*
;* Output:      Returns 0 = success, 1 = failure.
;*              CEVT and TIDX are indirect outputs.
;*
;* References:  TIDX, GNOT
;*
;************************************************************************/

Method(GHDS, 1)
{
  Store(Arg0, TIDX)               // Store the table number

  // Call GNOT for CEVT = 1 = hotkey, notify value = 0

  Return(GNOT(1, 0))              // Return stats from GNOT
}


/************************************************************************;
;*
;* Name:        GLID
;*
;* Description: Handle a lid event (performs the Notify(GFX0, 0), but not the
;*              lid notify).
;*
;* Usage:       This method must be called when a lid event occurs.  A
;*              Notify(LID0, 0x80) must follow the call to this method.
;*
;* Input:       Arg0 = Lid state:
;*                      0 = All closed
;*                      1 = internal LFP lid open
;*                      2 = external lid open
;*                      3 = both external and internal open
;*
;* Output:      Returns 0=success, 1=failure.
;*              CLID and CEVT are indirect outputs.
;*
;* References:  CLID, GNOT
;*
;************************************************************************/

Method(GLID, 1)
{
  Store(Arg0, CLID)               // Store the current lid state

  // Call GNOT for CEVT=2=Lid, notify value = 0

  Return(GNOT(2, 0))              // Return stats from GNOT
}


/************************************************************************;
;*
;* Name:        GDCK
;*
;* Description: Handle a docking event by updating the current docking status
;*              and doing a notification.
;*
;* Usage:       This method must be called when a docking event occurs.
;*
;* Input:       Arg0 = Docking state:
;*                      0 = Undocked
;*                      1 = Docked
;*
;* Output:      Returns 0=success, 1=failure.
;*              CDCK and CEVT are indirect outputs.
;*
;* References:  CDCK, GNOT
;*
;************************************************************************/

Method(GDCK, 1)
{
  Store(Arg0, CDCK)               // Store the current dock state

  // Call GNOT for CEVT=4=Dock, notify value = 0

  Return(GNOT(4, 0))              // Return stats from GNOT
}


/************************************************************************;
;* ASLE Interrupt Methods
;************************************************************************/


/************************************************************************;
;*
;* Name:        PARD
;*
;* Description: Check if the driver is ready to handle ASLE interrupts
;*              generate by the system BIOS.
;*
;* Usage:       This method must be called before generating each ASLE
;*              interrupt.
;*
;* Input:       None
;*
;* Output:      Returns 0 = success, 1 = failure.
;*
;* References:  ARDY (Driver readiness), ASLP (Driver recommended sleep
;*              timeout value)
;*
;************************************************************************/

Method(PARD)
{
  If(LNot(ARDY))
  {

    // Sleep for ASLP milliseconds if the driver is not ready.

    Sleep(ASLP)
  }

  // If ARDY is clear, the driver is not ready.  If the return value is
  // !=0, do not generate the ASLE interrupt.

  Return(LNot(ARDY))
}


/************************************************************************;
;*
;* Name:        AINT
;*
;* Description: Call the appropriate methods to generate an ASLE interrupt.
;*              This process includes ensuring the graphics driver is ready
;*              to process the interrupt, ensuring the driver supports the
;*              interrupt of interest, and passing information about the event
;*              to the graphics driver.
;*
;* Usage:       This method must called to generate an ASLE interrupt.
;*
;* Input:       Arg0 = ASLE command function code:
;*                      0 = Set ALS illuminance
;*                      1 = Set backlight brightness
;*                      2 = Do Panel Fitting
;*              Arg1 = If Arg0 = 0, current ALS reading:
;*                      0 = Reading below sensor range
;*                      1-0xFFFE = Current sensor reading
;*                      0xFFFF = Reading above sensor range
;*              Arg1 = If Arg0 = 1, requested backlight percentage
;*
;* Output:      Returns 0 = success, 1 = failure
;*
;* References:  PARD method.
;*
;************************************************************************/

Method(AINT, 2)
{

  // Return failure if the requested feature is not supported by the
  // driver.

  If(LNot(And(TCHE, ShiftLeft(1, Arg0))))
  {
    Return(0x1)
  }

  // Return failure if the driver is not ready to handle an ASLE
  // interrupt.

  If(PARD())
  {
    Return(0x1)
  }

  // Evaluate the first argument (Panel fitting, backlight brightness, or ALS).

  If(LEqual(Arg0, 2))             // Arg0 = 2, so request a panel fitting mode change.
  {
    If(CPFM)                                        // If current mode field is non-zero use it.
    {
      And(CPFM, 0x0F, Local0)                 // Create variables without reserved
      And(EPFM, 0x0F, Local1)                 // or valid bits.

      If(LEqual(Local0, 1))                   // If current mode is centered,
      {
        If(And(Local1, 6))              // and if stretched is enabled,
        {
          Store(6, PFIT)          // request stretched.
        }
        Else                            // Otherwise,
        {
          If(And(Local1, 8))      // if aspect ratio is enabled,
          {
            Store(8, PFIT)  // request aspect ratio.
          }
          Else                    // Only centered mode is enabled
          {
            Store(1, PFIT)  // so request centered. (No change.)
          }
        }
      }
      If(LEqual(Local0, 6))                   // If current mode is stretched,
      {
        If(And(Local1, 8))              // and if aspect ratio is enabled,
        {
          Store(8, PFIT)          // request aspect ratio.
        }
        Else                            // Otherwise,
        {
          If(And(Local1, 1))      // if centered is enabled,
          {
            Store(1, PFIT)  // request centered.
          }
          Else                    // Only stretched mode is enabled
          {
            Store(6, PFIT)  // so request stretched. (No change.)
          }
        }
      }
      If(LEqual(Local0, 8))                   // If current mode is aspect ratio,
      {
        If(And(Local1, 1))              // and if centered is enabled,
        {
          Store(1, PFIT)          // request centered.
        }
        Else                            // Otherwise,
        {
          If(And(Local1, 6))      // if stretched is enabled,
          {
            Store(6, PFIT)  // request stretched.
          }
          Else                    // Only aspect ratio mode is enabled
          {
            Store(8, PFIT)  // so request aspect ratio. (No change.)
          }
        }
      }
    }

    // The following code for panel fitting (within the Else condition) is retained for backward compatiblity.

    Else                            // If CFPM field is zero use PFIT and toggle the
    {
      Xor(PFIT,7,PFIT)        // mode setting between stretched and centered only.
    }

    Or(PFIT,0x80000000,PFIT)        // Set the valid bit for all cases.

    Store(4, ASLC)                  // Store "Panel fitting event" to ASLC[31:1]
  }
  Else
  {
    If(LEqual(Arg0, 1))             // Arg0=1, so set the backlight brightness.
    {
      Store(Divide(Multiply(Arg1, 255), 100), BCLP) // Convert from percent to 0-255.

      Or(BCLP, 0x80000000, BCLP)      // Set the valid bit.

      Store(2, ASLC)                  // Store "Backlight control event" to ASLC[31:1]
    }
    Else
    {
      If(LEqual(Arg0, 0))             // Arg0=0, so set the ALS illuminace
      {
        Store(Arg1, ALSI)

        Store(1, ASLC)          // Store "ALS event" to ASLC[31:1]
      }
      Else
      {
        Return(0x1) // Unsupported function
      }
    }
  }

  Store(0x01, ASLE)               // Generate ASLE interrupt
  Return(0x0)                     // Return success
}


/************************************************************************;
;*
;* Name:        SCIP
;*
;* Description: Checks the presence of the OpRegion and SCI
;*
;* Usage:       This method is called before other OpRegion methods. The
;*              former "GSMI True/False is not always valid.  This method
;*              checks if the OpRegion Version is non-zero and if non-zero,
;*              (present and readable) then checks the GSMI flag.
;*
;* Input:       None
;*
;* Output:      Boolean True = SCI present.
;*
;* References:  None
;*
;************************************************************************/

Method(SCIP)
{
  If(LNotEqual(OVER,0))           // If OpRegion Version not 0.
  {
    Return(LNot(GSMI))      // Return True if SCI.
  }

  Return(0)                       // Else Return False.
}
