/** @file
  PS2 Mouse Communication Interface.

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ps2Mouse.h"
#include "CommPs2.h"

UINT8  SampleRateTbl[MaxSampleRate] = { 0xa, 0x14, 0x28, 0x3c, 0x50, 0x64, 0xc8 };

UINT8  ResolutionTbl[MaxResolution] = { 0, 1, 2, 3 };

/**
  Issue self test command via IsaIo interface.

  @return EFI_SUCCESS  Success to do keyboard self testing.
  @return others       Fail to do keyboard self testing.
**/
EFI_STATUS
KbcSelfTest (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Keyboard controller self test
  //
  Status = Out8042Command (SELF_TEST);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read return code
  //
  Status = In8042Data (&Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Data != 0x55) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Set system flag
  //
  Status = Out8042Command (READ_CMD_BYTE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = In8042Data (&Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Out8042Command (WRITE_CMD_BYTE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data  |= CMD_SYS_FLAG;
  Status = Out8042Data (Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Issue command to enable keyboard AUX functionality.

  @return Status of command issuing.
**/
EFI_STATUS
KbcEnableAux (
  VOID
  )
{
  //
  // Send 8042 enable mouse command
  //
  return Out8042Command (ENABLE_AUX);
}

/**
  Issue command to disable keyboard AUX functionality.

  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL

  @return Status of command issuing.
**/
EFI_STATUS
KbcDisableAux (
  VOID
  )
{
  //
  // Send 8042 disable mouse command
  //
  return Out8042Command (DISABLE_AUX);
}

/**
  Issue command to enable keyboard.

  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL

  @return Status of command issuing.
**/
EFI_STATUS
KbcEnableKb (
  VOID
  )
{
  //
  // Send 8042 enable keyboard command
  //
  return Out8042Command (ENABLE_KB);
}

/**
  Issue command to disable keyboard.

  @return Status of command issuing.
**/
EFI_STATUS
KbcDisableKb (
  VOID
  )
{
  //
  // Send 8042 disable keyboard command
  //
  return Out8042Command (DISABLE_KB);
}

/**
  Issue command to check keyboard status.

  @param KeyboardEnable return whether keyboard is enable.

  @return Status of command issuing.
**/
EFI_STATUS
CheckKbStatus (
  OUT BOOLEAN  *KeyboardEnable
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Send command to read KBC command byte
  //
  Status = Out8042Command (READ_CMD_BYTE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = In8042Data (&Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check keyboard enable or not
  //
  if ((Data & CMD_KB_STS) == CMD_KB_DIS) {
    *KeyboardEnable = FALSE;
  } else {
    *KeyboardEnable = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Issue command to reset keyboard.

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseReset (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  Status = Out8042AuxCommand (RESET_CMD, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = In8042AuxData (&Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check BAT Complete Code
  //
  if (Data != PS2MOUSE_BAT1) {
    return EFI_DEVICE_ERROR;
  }

  Status = In8042AuxData (&Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check BAT Complete Code
  //
  if (Data != PS2MOUSE_BAT2) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Issue command to set mouse's sample rate

  @param SampleRate value of sample rate

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetSampleRate (
  IN MOUSE_SR  SampleRate
  )
{
  EFI_STATUS  Status;

  //
  // Send auxiliary command to set mouse sample rate
  //
  Status = Out8042AuxCommand (SETSR_CMD, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Out8042AuxData (SampleRateTbl[SampleRate]);

  return Status;
}

/**
  Issue command to set mouse's resolution.

  @param Resolution value of resolution

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetResolution (
  IN MOUSE_RE  Resolution
  )
{
  EFI_STATUS  Status;

  //
  // Send auxiliary command to set mouse resolution
  //
  Status = Out8042AuxCommand (SETRE_CMD, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Out8042AuxData (ResolutionTbl[Resolution]);

  return Status;
}

/**
  Issue command to set mouse's scaling.

  @param Scaling value of scaling

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetScaling (
  IN MOUSE_SF  Scaling
  )
{
  //
  // Send auxiliary command to set mouse scaling data
  //
  return Out8042AuxCommand (Scaling == Scaling1 ? SETSF1_CMD : SETSF2_CMD, FALSE);
}

/**
  Issue command to enable Ps2 mouse.

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseEnable (
  VOID
  )
{
  //
  // Send auxiliary command to enable mouse
  //
  return Out8042AuxCommand (ENABLE_CMD, FALSE);
}

/**
  Get mouse packet . Only care first 3 bytes

  @param MouseDev  Pointer of PS2 Mouse Private Data Structure

  @retval EFI_NOT_READY  Mouse Device not ready to input data packet, or some error happened during getting the packet
  @retval EFI_SUCCESS    The data packet is gotten successfully.

**/
EFI_STATUS
PS2MouseGetPacket (
  PS2_MOUSE_DEV  *MouseDev
  )

{
  EFI_STATUS  Status;
  BOOLEAN     KeyboardEnable;
  UINT8       Packet[PS2_PACKET_LENGTH];
  UINT8       Data;
  UINTN       Count;
  UINTN       State;
  INT16       RelativeMovementX;
  INT16       RelativeMovementY;
  BOOLEAN     LButton;
  BOOLEAN     RButton;

  KeyboardEnable = FALSE;
  State          = PS2_READ_BYTE_ONE;

  //
  // State machine to get mouse packet
  //
  while (1) {
    switch (State) {
      case PS2_READ_BYTE_ONE:
        //
        // Read mouse first byte data, if failed, immediately return
        //
        KbcDisableAux ();
        Count  = 1;
        Status = PS2MouseRead (&Data, &Count, State);
        if (EFI_ERROR (Status)) {
          KbcEnableAux ();
          return EFI_NOT_READY;
        }

        if (Count != 1) {
          KbcEnableAux ();
          return EFI_NOT_READY;
        }

        if (IS_PS2_SYNC_BYTE (Data)) {
          Packet[0] = Data;
          State     = PS2_READ_DATA_BYTE;

          CheckKbStatus (&KeyboardEnable);
          KbcDisableKb ();
          KbcEnableAux ();
        }

        break;

      case PS2_READ_DATA_BYTE:
        Count  = 2;
        Status = PS2MouseRead ((Packet + 1), &Count, State);
        if (EFI_ERROR (Status)) {
          if (KeyboardEnable) {
            KbcEnableKb ();
          }

          return EFI_NOT_READY;
        }

        if (Count != 2) {
          if (KeyboardEnable) {
            KbcEnableKb ();
          }

          return EFI_NOT_READY;
        }

        State = PS2_PROCESS_PACKET;
        break;

      case PS2_PROCESS_PACKET:
        if (KeyboardEnable) {
          KbcEnableKb ();
        }

        //
        // Decode the packet
        //
        RelativeMovementX = Packet[1];
        RelativeMovementY = Packet[2];
        //
        //               Bit 7   |    Bit 6   |    Bit 5   |   Bit 4    |   Bit 3  |   Bit 2    |   Bit 1   |   Bit 0
        //  Byte 0  | Y overflow | X overflow | Y sign bit | X sign bit | Always 1 | Middle Btn | Right Btn | Left Btn
        //  Byte 1  |                                           8 bit X Movement
        //  Byte 2  |                                           8 bit Y Movement
        //
        // X sign bit + 8 bit X Movement : 9-bit signed twos complement integer that presents the relative displacement of the device in the X direction since the last data transmission.
        // Y sign bit + 8 bit Y Movement : Same as X sign bit + 8 bit X Movement.
        //
        //
        // First, Clear X and Y high 8 bits
        //
        RelativeMovementX = (INT16)(RelativeMovementX & 0xFF);
        RelativeMovementY = (INT16)(RelativeMovementY & 0xFF);
        //
        // Second, if the 9-bit signed twos complement integer is negative, set the high 8 bit 0xff
        //
        if ((Packet[0] & 0x10) != 0) {
          RelativeMovementX = (INT16)(RelativeMovementX | 0xFF00);
        }

        if ((Packet[0] & 0x20) != 0) {
          RelativeMovementY = (INT16)(RelativeMovementY | 0xFF00);
        }

        RButton = (UINT8)(Packet[0] & 0x2);
        LButton = (UINT8)(Packet[0] & 0x1);

        //
        // Update mouse state
        //
        MouseDev->State.RelativeMovementX += RelativeMovementX;
        MouseDev->State.RelativeMovementY -= RelativeMovementY;
        MouseDev->State.RightButton        = (UINT8)(RButton ? TRUE : FALSE);
        MouseDev->State.LeftButton         = (UINT8)(LButton ? TRUE : FALSE);
        MouseDev->StateChanged             = TRUE;

        return EFI_SUCCESS;
    }
  }
}

/**
  Read data via IsaIo protocol with given number.

  @param Buffer  Buffer receive data of mouse
  @param BufSize The size of buffer
  @param State   Check input or read data

  @return status of reading mouse data.
**/
EFI_STATUS
PS2MouseRead (
  OUT UINT8     *Buffer,
  IN OUT UINTN  *BufSize,
  IN  UINTN     State
  )
{
  EFI_STATUS  Status;
  UINTN       BytesRead;

  Status = EFI_SUCCESS;

  if (State == PS2_READ_BYTE_ONE) {
    //
    // Check input for mouse
    //
    Status = CheckForInput ();

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  for (BytesRead = 0; BytesRead < *BufSize; BytesRead++) {
    Status = WaitOutputFull (TIMEOUT);
    if (EFI_ERROR (Status)) {
      break;
    }

    Buffer[BytesRead] = IoRead8 (KBC_DATA_PORT);
  }

  //
  // Verify the correct number of bytes read
  //
  if ((BytesRead == 0) || (BytesRead != *BufSize)) {
    Status = EFI_NOT_FOUND;
  }

  *BufSize = BytesRead;
  return Status;
}

//
// 8042 I/O function
//

/**
  I/O work flow of outing 8042 command.

  @param Command I/O command.

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042Command (
  IN UINT8  Command
  )
{
  EFI_STATUS  Status;

  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send command
  //
  IoWrite8 (KBC_CMD_STS_PORT, Command);

  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow of outing 8042 data.

  @param Data    Data value

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042Data (
  IN UINT8  Data
  )
{
  EFI_STATUS  Status;

  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (KBC_DATA_PORT, Data);
  return WaitInputEmpty (TIMEOUT);
}

/**
  I/O work flow of in 8042 data.

  @param Data    Data value

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042Data (
  IN OUT UINT8  *Data
  )
{
  UINTN  Delay;

  Delay = TIMEOUT / 50;

  do {
    //
    // Check keyboard controller status bit 0(output buffer status)
    //
    if ((IoRead8 (KBC_CMD_STS_PORT) & KBC_OUTB) == KBC_OUTB) {
      break;
    }

    gBS->Stall (50);
    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  *Data = IoRead8 (KBC_DATA_PORT);

  return EFI_SUCCESS;
}

/**
  I/O work flow of outing 8042 Aux command.

  @param Command Aux I/O command
  @param Resend  Whether need resend the Aux command.

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042AuxCommand (
  IN UINT8    Command,
  IN BOOLEAN  Resend
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send write to auxiliary device command
  //
  IoWrite8 (KBC_CMD_STS_PORT, WRITE_AUX_DEV);

  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send auxiliary device command
  //
  IoWrite8 (KBC_DATA_PORT, Command);

  //
  // Read return code
  //
  Status = In8042AuxData (&Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Data == PS2_ACK) {
    //
    // Receive mouse acknowledge, command send success
    //
    return EFI_SUCCESS;
  } else if (Resend) {
    //
    // Resend fail
    //
    return EFI_DEVICE_ERROR;
  } else if (Data == PS2_RESEND) {
    //
    // Resend command
    //
    Status = Out8042AuxCommand (Command, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // Invalid return code
    //
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow of outing 8042 Aux data.

  @param Data    Buffer holding return value

  @retval EFI_SUCCESS Success to execute I/O work flow.
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042AuxData (
  IN UINT8  Data
  )
{
  EFI_STATUS  Status;

  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Send write to auxiliary device command
  //
  IoWrite8 (KBC_CMD_STS_PORT, WRITE_AUX_DEV);

  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (KBC_DATA_PORT, Data);

  Status = WaitInputEmpty (TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow of in 8042 Aux data.

  @param Data    Buffer holding return value.

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042AuxData (
  IN OUT UINT8  *Data
  )
{
  EFI_STATUS  Status;

  //
  // wait for output data
  //
  Status = WaitOutputFull (BAT_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Data = IoRead8 (KBC_DATA_PORT);

  return EFI_SUCCESS;
}

/**
  Check keyboard controller status, if it is output buffer full and for auxiliary device.

  @retval EFI_SUCCESS   Keyboard controller is ready
  @retval EFI_NOT_READY Keyboard controller is not ready
**/
EFI_STATUS
CheckForInput (
  VOID
  )
{
  UINT8  Data;

  Data = IoRead8 (KBC_CMD_STS_PORT);

  //
  // Check keyboard controller status, if it is output buffer full and for auxiliary device
  //
  if ((Data & (KBC_OUTB | KBC_AUXB)) != (KBC_OUTB | KBC_AUXB)) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow to wait input buffer empty in given time.

  @param Timeout Waiting time.

  @retval EFI_TIMEOUT if input is still not empty in given time.
  @retval EFI_SUCCESS input is empty.
**/
EFI_STATUS
WaitInputEmpty (
  IN UINTN  Timeout
  )
{
  UINTN  Delay;
  UINT8  Data;

  Delay = Timeout / 50;

  do {
    Data = IoRead8 (KBC_CMD_STS_PORT);

    //
    // Check keyboard controller status bit 1(input buffer status)
    //
    if ((Data & KBC_INPB) == 0) {
      break;
    }

    gBS->Stall (50);
    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow to wait output buffer full in given time.

  @param Timeout given time

  @retval EFI_TIMEOUT  output is not full in given time
  @retval EFI_SUCCESS  output is full in given time.
**/
EFI_STATUS
WaitOutputFull (
  IN UINTN  Timeout
  )
{
  UINTN  Delay;
  UINT8  Data;

  Delay = Timeout / 50;

  do {
    Data = IoRead8 (KBC_CMD_STS_PORT);

    //
    // Check keyboard controller status bit 0(output buffer status)
    //  & bit5(output buffer for auxiliary device)
    //
    if ((Data & (KBC_OUTB | KBC_AUXB)) == (KBC_OUTB | KBC_AUXB)) {
      break;
    }

    gBS->Stall (50);
    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
