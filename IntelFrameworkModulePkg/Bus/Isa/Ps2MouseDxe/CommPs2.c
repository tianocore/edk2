/** @file
  PS2 Mouse Communication Interface.
  
Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ps2Mouse.h"
#include "CommPs2.h"

UINT8 SampleRateTbl[MaxSampleRate]   = { 0xa, 0x14, 0x28, 0x3c, 0x50, 0x64, 0xc8 };

UINT8 ResolutionTbl[MaxResolution]  = { 0, 1, 2, 3 };

/**
  Issue self test command via IsaIo interface.
  
  @param IsaIo Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return EFI_SUCCESS  Success to do keyboard self testing.
  @return others       Fail to do keyboard self testing.
**/
EFI_STATUS
KbcSelfTest (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Keyboard controller self test
  //
  Status = Out8042Command (IsaIo, SELF_TEST);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Read return code
  //
  Status = In8042Data (IsaIo, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Data != 0x55) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set system flag
  //
  Status = Out8042Command (IsaIo, READ_CMD_BYTE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = In8042Data (IsaIo, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Out8042Command (IsaIo, WRITE_CMD_BYTE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data |= CMD_SYS_FLAG;
  Status = Out8042Data (IsaIo, Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Issue command to enable keyboard AUX functionality.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return Status of command issuing.
**/
EFI_STATUS
KbcEnableAux (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  //
  // Send 8042 enable mouse command
  //
  return Out8042Command (IsaIo, ENABLE_AUX);
}

/**
  Issue command to disable keyboard AUX functionality.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return Status of command issuing.
**/
EFI_STATUS
KbcDisableAux (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  //
  // Send 8042 disable mouse command
  //
  return Out8042Command (IsaIo, DISABLE_AUX);
}

/**
  Issue command to enable keyboard.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return Status of command issuing.
**/
EFI_STATUS
KbcEnableKb (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  //
  // Send 8042 enable keyboard command
  //
  return Out8042Command (IsaIo, ENABLE_KB);
}

/**
  Issue command to disable keyboard.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return Status of command issuing.
**/
EFI_STATUS
KbcDisableKb (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  //
  // Send 8042 disable keyboard command
  //
  return Out8042Command (IsaIo, DISABLE_KB);
}

/**
  Issue command to check keyboard status.
  
  @param IsaIo          Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param KeyboardEnable return whether keyboard is enable.
  
  @return Status of command issuing.
**/
EFI_STATUS
CheckKbStatus (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  OUT BOOLEAN                             *KeyboardEnable
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Send command to read KBC command byte
  //
  Status = Out8042Command (IsaIo, READ_CMD_BYTE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = In8042Data (IsaIo, &Data);
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
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseReset (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  Status = Out8042AuxCommand (IsaIo, RESET_CMD, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = In8042AuxData (IsaIo, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check BAT Complete Code
  //
  if (Data != PS2MOUSE_BAT1) {
    return EFI_DEVICE_ERROR;
  }

  Status = In8042AuxData (IsaIo, &Data);
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
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param SampleRate value of sample rate 
  
  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetSampleRate (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN MOUSE_SR                             SampleRate
  )
{
  EFI_STATUS  Status;

  //
  // Send auxiliary command to set mouse sample rate
  //
  Status = Out8042AuxCommand (IsaIo, SETSR_CMD, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Out8042AuxData (IsaIo, SampleRateTbl[SampleRate]);

  return Status;
}

/**
  Issue command to set mouse's resolution.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Resolution value of resolution
  
  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetResolution (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN MOUSE_RE                             Resolution
  )
{
  EFI_STATUS  Status;

  //
  // Send auxiliary command to set mouse resolution
  //
  Status = Out8042AuxCommand (IsaIo, SETRE_CMD, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Out8042AuxData (IsaIo, ResolutionTbl[Resolution]);

  return Status;
}

/**
  Issue command to set mouse's scaling.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Scaling value of scaling
  
  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetScaling (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN MOUSE_SF                             Scaling
  )
{
  UINT8 Command;

  Command = (UINT8) (Scaling == Scaling1 ? SETSF1_CMD : SETSF2_CMD);

  //
  // Send auxiliary command to set mouse scaling data
  //
  return Out8042AuxCommand (IsaIo, Command, FALSE);
}

/**
  Issue command to enable Ps2 mouse.
  
  @param IsaIo  Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseEnable (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  //
  // Send auxiliary command to enable mouse
  //
  return Out8042AuxCommand (IsaIo, ENABLE_CMD, FALSE);
}

/**
  Get mouse packet . Only care first 3 bytes

  @param MouseDev  Pointer of PS2 Mouse Private Data Structure 

  @retval EFI_NOT_READY  Mouse Device not ready to input data packet, or some error happened during getting the packet
  @retval EFI_SUCCESS    The data packet is gotten successfully.

**/
EFI_STATUS
PS2MouseGetPacket (
  PS2_MOUSE_DEV     *MouseDev
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

  KeyboardEnable  = FALSE;
  Count           = 1;
  State           = PS2_READ_BYTE_ONE;

  //
  // State machine to get mouse packet
  //
  while (1) {

    switch (State) {
    case PS2_READ_BYTE_ONE:
      //
      // Read mouse first byte data, if failed, immediately return
      //
      KbcDisableAux (MouseDev->IsaIo);
      Status = PS2MouseRead (MouseDev->IsaIo, &Data, &Count, State);
      if (EFI_ERROR (Status)) {
        KbcEnableAux (MouseDev->IsaIo);
        return EFI_NOT_READY;
      }

      if (Count != 1) {
        KbcEnableAux (MouseDev->IsaIo);
        return EFI_NOT_READY;
      }

      if (IS_PS2_SYNC_BYTE (Data)) {
        Packet[0] = Data;
        State     = PS2_READ_DATA_BYTE;

        CheckKbStatus (MouseDev->IsaIo, &KeyboardEnable);
        KbcDisableKb (MouseDev->IsaIo);
        KbcEnableAux (MouseDev->IsaIo);
      }
      break;

    case PS2_READ_DATA_BYTE:
      Count   = 2;
      Status  = PS2MouseRead (MouseDev->IsaIo, (Packet + 1), &Count, State);
      if (EFI_ERROR (Status)) {
        if (KeyboardEnable) {
          KbcEnableKb (MouseDev->IsaIo);
        }

        return EFI_NOT_READY;
      }

      if (Count != 2) {
        if (KeyboardEnable) {
          KbcEnableKb (MouseDev->IsaIo);
        }

        return EFI_NOT_READY;
      }

      State = PS2_PROCESS_PACKET;
      break;

    case PS2_PROCESS_PACKET:
      if (KeyboardEnable) {
        KbcEnableKb (MouseDev->IsaIo);
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
      RelativeMovementX = (INT16) (RelativeMovementX & 0xFF); 
      RelativeMovementY = (INT16) (RelativeMovementY & 0xFF); 
      //
      // Second, if the 9-bit signed twos complement integer is negative, set the high 8 bit 0xff
      //
      if ((Packet[0] & 0x10) != 0) {
        RelativeMovementX = (INT16) (RelativeMovementX | 0xFF00);
      }
      if ((Packet[0] & 0x20) != 0) {
        RelativeMovementY = (INT16) (RelativeMovementY | 0xFF00);
      }

      
      RButton           = (UINT8) (Packet[0] & 0x2);
      LButton           = (UINT8) (Packet[0] & 0x1);

      //
      // Update mouse state
      //
      MouseDev->State.RelativeMovementX += RelativeMovementX;
      MouseDev->State.RelativeMovementY -= RelativeMovementY;
      MouseDev->State.RightButton = (UINT8) (RButton ? TRUE : FALSE);
      MouseDev->State.LeftButton  = (UINT8) (LButton ? TRUE : FALSE);
      MouseDev->StateChanged      = TRUE;

      return EFI_SUCCESS;
    }
  }
}

/**
  Read data via IsaIo protocol with given number.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Buffer  Buffer receive data of mouse
  @param BufSize The size of buffer
  @param State   Check input or read data
  
  @return status of reading mouse data.
**/
EFI_STATUS
PS2MouseRead (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  OUT VOID                                *Buffer,
  IN OUT UINTN                            *BufSize,
  IN  UINTN                               State
  )
{
  EFI_STATUS  Status;
  UINTN       BytesRead;

  Status    = EFI_SUCCESS;
  BytesRead = 0;

  if (State == PS2_READ_BYTE_ONE) {
    //
    // Check input for mouse
    //
    Status = CheckForInput (IsaIo);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  while (BytesRead < *BufSize) {

    Status = WaitOutputFull (IsaIo, TIMEOUT);
    if (EFI_ERROR (Status)) {
      break;
    }

    IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, Buffer);

    BytesRead++;
    Buffer = (UINT8 *) Buffer + 1;
  }
  //
  // Verify the correct number of bytes read
  //
  if (BytesRead == 0 || BytesRead != *BufSize) {
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
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Command I/O command.
  
  @retval EFI_SUCCESS Success to excute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042Command (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Command
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Send command
  //
  Data = Command;
  IsaIo->Io.Write (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow of outing 8042 data.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Data    Data value
  
  @retval EFI_SUCCESS Success to excute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Data
  )
{
  EFI_STATUS  Status;
  UINT8       Temp;
  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Temp = Data;
  IsaIo->Io.Write (IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, &Temp);

  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow of in 8042 data.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Data    Data value
  
  @retval EFI_SUCCESS Success to excute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  )
{
  UINTN Delay;
  UINT8 Temp;

  Delay = TIMEOUT / 50;

  do {
    IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Temp);

    //
    // Check keyboard controller status bit 0(output buffer status)
    //
    if ((Temp & KBC_OUTB) == KBC_OUTB) {
      break;
    }

    gBS->Stall (50);
    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, Data);

  return EFI_SUCCESS;
}

/**
  I/O work flow of outing 8042 Aux command.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Command Aux I/O command
  @param Resend  Whether need resend the Aux command.
  
  @retval EFI_SUCCESS Success to excute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042AuxCommand (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Command,
  IN BOOLEAN                              Resend
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Send write to auxiliary device command
  //
  Data = WRITE_AUX_DEV;
  IsaIo->Io.Write (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Send auxiliary device command
  //
  IsaIo->Io.Write (IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, &Command);

  //
  // Read return code
  //
  Status = In8042AuxData (IsaIo, &Data);
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
    Status = Out8042AuxCommand (IsaIo, Command, TRUE);
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
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Data    Buffer holding return value
  
  @retval EFI_SUCCESS Success to excute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042AuxData (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Data
  )
{
  EFI_STATUS  Status;
  UINT8       Temp;
  //
  // Wait keyboard controller input buffer empty
  //
  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Send write to auxiliary device command
  //
  Temp = WRITE_AUX_DEV;
  IsaIo->Io.Write (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Temp);

  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Temp = Data;
  IsaIo->Io.Write (IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, &Temp);

  Status = WaitInputEmpty (IsaIo, TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  I/O work flow of in 8042 Aux data.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Data    Buffer holding return value.
  
  @retval EFI_SUCCESS Success to excute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042AuxData (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  )
{
  EFI_STATUS  Status;

  //
  // wait for output data
  //
  Status = WaitOutputFull (IsaIo, BAT_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, Data);

  return EFI_SUCCESS;
}


/**
  Check keyboard controller status, if it is output buffer full and for auxiliary device.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  
  @retval EFI_SUCCESS   Keyboard controller is ready
  @retval EFI_NOT_READY Keyboard controller is not ready
**/
EFI_STATUS
CheckForInput (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
{
  UINT8 Data;

  IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

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
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Timeout Wating time.
  
  @retval EFI_TIMEOUT if input is still not empty in given time.
  @retval EFI_SUCCESS input is empty.
**/
EFI_STATUS
WaitInputEmpty (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINTN                                Timeout
  )
{
  UINTN Delay;
  UINT8 Data;

  Delay = Timeout / 50;

  do {
    IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

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
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Timeout given time
  
  @retval EFI_TIMEOUT  output is not full in given time
  @retval EFI_SUCCESS  output is full in given time.
**/
EFI_STATUS
WaitOutputFull (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINTN                                Timeout
  )
{
  UINTN Delay;
  UINT8 Data;

  Delay = Timeout / 50;

  do {
    IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

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
