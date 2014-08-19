/** @file
  Debug Agent library implementition with empty functions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "GdbDebugAgent.h"


UINTN     gMaxProcessorIndex = 0;

//
// Buffers for basic gdb communication
//
CHAR8 gInBuffer[MAX_BUF_SIZE];
CHAR8 gOutBuffer[MAX_BUF_SIZE];


//
// Globals for returning XML from qXfer:libraries:read packet
//
UINTN                             gPacketqXferLibraryOffset = 0;
UINTN                             gEfiDebugImageTableEntry = 0;
CHAR8                             gXferLibraryBuffer[2000];

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mHexToStr[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};


// add-symbol-file c:/work/edk2/Build/BeagleBoard/DEBUG_ARMGCC/ARM/BeagleBoardPkg/Sec/Sec/DEBUG/BeagleBoardSec.dll 0x80008360
CHAR8  *qXferHack = "<library name=\"c:/work/edk2/Build/BeagleBoard/DEBUG_ARMGCC/ARM/BeagleBoardPkg/Sec/Sec/DEBUG/BeagleBoardSec.dll\"><segment address=\"0x80008360\"/></library>";

UINTN
gXferObjectReadResponse (
  IN  CHAR8         Type,
  IN  CHAR8         *Str
  )
{
  CHAR8   *OutBufPtr;             // pointer to the output buffer
  CHAR8   Char;
  UINTN   Count;

  // responce starts with 'm' or 'l' if it is the end
  OutBufPtr = gOutBuffer;
  *OutBufPtr++ = Type;
  Count = 1;

  // Binary data encoding
  OutBufPtr = gOutBuffer;
  while (*Str != '\0') {
    Char = *Str++;
    if ((Char == 0x7d) || (Char == 0x23) || (Char == 0x24) || (Char == 0x2a)) {
      // escape character
      *OutBufPtr++ = 0x7d;

      Char ^= 0x20;
    }
    *OutBufPtr++ = Char;
    Count++;
  }

  *OutBufPtr = '\0' ;  // the end of the buffer
  SendPacket (gOutBuffer);

  return Count;
}

/**
  Process "qXfer:object:read:annex:offset,length" request.

  Returns an XML document that contains loaded libraries. In our case it is
  infomration in the EFI Debug Inmage Table converted into an XML document.

  GDB will call with an arbitrary length (it can't know the real length and
  will reply with chunks of XML that are easy for us to deal with. Gdb will
  keep calling until we say we are done. XML doc looks like:

  <library-list>
    <library name="/a/a/c/d.dSYM"><segment address="0x10000000"/></library>
    <library name="/a/m/e/e.pdb"><segment address="0x20000000"/></library>
    <library name="/a/l/f/f.dll"><segment address="0x30000000"/></library>
  </library-list>

  Since we can not allocate memory in interupt context this module has
  assumptions about how it will get called:
  1) Length will generally be max remote packet size (big enough)
  2) First Offset of an XML document read needs to be 0
  3) This code will return back small chunks of the XML document on every read.
     Each subseqent call will ask for the next availble part of the document.

  Note: The only variable size element in the XML is:
  "  <library name=\"%s\"><segment address=\"%p\"/></library>\n" and it is
  based on the file path and name of the symbol file. If the symbol file name
  is bigger than the max gdb remote packet size we could update this code
  to respond back in chunks.

 @param Offset  offset into special data area
 @param Length  number of bytes to read starting at Offset

 **/
VOID
QxferLibrary (
  IN  UINTN   Offset,
  IN  UINTN   Length
  )
{
  gPacketqXferLibraryOffset += gXferObjectReadResponse ('m', "<library-list>\n");
  gPacketqXferLibraryOffset += gXferObjectReadResponse ('m', qXferHack);
  gXferObjectReadResponse ('l', "</library-list>\n");
  gPacketqXferLibraryOffset = 0;
}

/**
 Transfer length bytes of input buffer, starting at Address, to memory.

 @param     length                  the number of the bytes to be transferred/written
 @param     *address                the start address of the transferring/writing the memory
 @param     *new_data               the new data to be written to memory
 **/

VOID
TransferFromInBufToMem (
  IN    UINTN                       Length,
  IN    unsigned char               *Address,
  IN    CHAR8                       *NewData
  )
{
  CHAR8 c1;
  CHAR8 c2;

  while (Length-- > 0) {
    c1 = (CHAR8)HexCharToInt (*NewData++);
    c2 = (CHAR8)HexCharToInt (*NewData++);

    if ((c1 < 0) || (c2 < 0)) {
      SendError (GDB_EBADMEMDATA);
      return;
    }
    *Address++ = (UINT8)((c1 << 4) + c2);
  }

  SendSuccess();
}


/**
 Transfer Length bytes of memory starting at Address to an output buffer, OutBuffer. This function will finally send the buffer
 as a packet.

 @param     Length                  the number of the bytes to be transferred/read
 @param     *address                pointer to the start address of the transferring/reading the memory
 **/

VOID
TransferFromMemToOutBufAndSend (
  IN    UINTN                       Length,
  IN    unsigned char               *Address
  )
{
  // there are Length bytes and every byte is represented as 2 hex chars
  CHAR8   OutBuffer[MAX_BUF_SIZE];
  CHAR8   *OutBufPtr;             // pointer to the output buffer
  CHAR8   Char;

  OutBufPtr = OutBuffer;
  while (Length > 0) {

    Char = mHexToStr[*Address >> 4];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }
    *OutBufPtr++ = Char;

    Char = mHexToStr[*Address & 0x0f];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }
    *OutBufPtr++ = Char;

    Address++;
    Length--;
  }

  *OutBufPtr = '\0' ;  // the end of the buffer
  SendPacket (OutBuffer);
}



/**
  Send a GDB Remote Serial Protocol Packet

  $PacketData#checksum PacketData is passed in and this function adds the packet prefix '$',
  the packet teminating character '#' and the two digit checksum.

  If an ack '+' is not sent resend the packet, but timeout eventually so we don't end up
  in an infinit loop. This is so if you unplug the debugger code just keeps running

  @param PacketData   Payload data for the packet


  @retval             Number of bytes of packet data sent.

**/
UINTN
SendPacket (
  IN  CHAR8 *PacketData
  )
{
  UINT8 CheckSum;
  UINTN Timeout;
  CHAR8 *Ptr;
  CHAR8 TestChar;
  UINTN Count;

  Timeout = PcdGet32 (PcdGdbMaxPacketRetryCount);

  Count = 0;
  do {

    Ptr = PacketData;

    if (Timeout-- == 0) {
      // Only try a finite number of times so we don't get stuck in the loop
      return Count;
    }

    // Packet prefix
    GdbPutChar ('$');

    for (CheckSum = 0, Count =0 ; *Ptr != '\0'; Ptr++, Count++) {
      GdbPutChar (*Ptr);
      CheckSum = CheckSum + *Ptr;
    }

    // Packet terminating character and checksum
    GdbPutChar ('#');
    GdbPutChar (mHexToStr[CheckSum >> 4]);
    GdbPutChar (mHexToStr[CheckSum & 0x0F]);

    TestChar =  GdbGetChar ();
  } while (TestChar != '+');

  return Count;
}

/**
  Receive a GDB Remote Serial Protocol Packet

  $PacketData#checksum PacketData is passed in and this function adds the packet prefix '$',
  the packet teminating character '#' and the two digit checksum.

  If host re-starts sending a packet without ending the previous packet, only the last valid packet is proccessed.
  (In other words, if received packet is '$12345$12345$123456#checksum', only '$123456#checksum' will be processed.)

  If an ack '+' is not sent resend the packet

  @param PacketData   Payload data for the packet

  @retval             Number of bytes of packet data received.

**/
UINTN
ReceivePacket (
  OUT  CHAR8 *PacketData,
  IN   UINTN PacketDataSize
 )
{
  UINT8 CheckSum;
  UINTN Index;
  CHAR8 Char;
  CHAR8 SumString[3];
  CHAR8 TestChar;

  ZeroMem (PacketData, PacketDataSize);

  for (;;) {
      // wait for the start of a packet
    TestChar = GdbGetChar ();
    while (TestChar != '$') {
      TestChar = GdbGetChar ();
    };

  retry:
    for (Index = 0, CheckSum = 0; Index < (PacketDataSize - 1); Index++) {
      Char = GdbGetChar ();
      if (Char == '$') {
        goto retry;
      }
      if (Char == '#') {
        break;
      }

      PacketData[Index] = Char;
      CheckSum = CheckSum + Char;
    }
    PacketData[Index] = '\0';

    if (Index == PacketDataSize) {
      continue;
    }

    SumString[0] = GdbGetChar ();
    SumString[1] = GdbGetChar ();
    SumString[2] = '\0';

    if (AsciiStrHexToUintn (SumString) == CheckSum) {
      // Ack: Success
      GdbPutChar ('+');

      // Null terminate the callers string
      PacketData[Index] = '\0';
      return Index;
    } else {
      // Ack: Failure
      GdbPutChar ('-');
    }
  }

  //return 0;
}


/**
 Empties the given buffer
 @param   Buf          pointer to the first element in buffer to be emptied
 **/
VOID
EmptyBuffer (
  IN  CHAR8           *Buf
  )
{
  *Buf = '\0';
}


/**
 Converts an 8-bit Hex Char into a INTN.

 @param   Char the hex character to be converted into UINTN
 @retval  a INTN, from 0 to 15, that corressponds to Char
 -1 if Char is not a hex character
 **/
INTN
HexCharToInt (
  IN  CHAR8           Char
  )
{
  if ((Char >= 'A') && (Char <= 'F')) {
    return Char - 'A' + 10;
  } else if ((Char >= 'a') && (Char <= 'f')) {
    return Char - 'a' + 10;
  } else if ((Char >= '0') && (Char <= '9')) {
    return Char - '0';
  } else { // if not a hex value, return a negative value
    return -1;
  }
}

  // 'E' + the biggest error number is 255, so its 2 hex digits + buffer end
CHAR8 *gError = "E__";

/** 'E NN'
 Send an error with the given error number after converting to hex.
 The error number is put into the buffer in hex. '255' is the biggest errno we can send.
 ex: 162 will be sent as A2.

 @param   errno           the error number that will be sent
 **/
VOID
EFIAPI
SendError (
  IN  UINT8              ErrorNum
  )
{
  //
  // Replace _, or old data, with current errno
  //
  gError[1] = mHexToStr [ErrorNum >> 4];
  gError[2] = mHexToStr [ErrorNum & 0x0f];

  SendPacket (gError); // send buffer
}



/**
 Send 'OK' when the function is done executing successfully.
 **/
VOID
EFIAPI
SendSuccess (
  VOID
  )
{
  SendPacket ("OK"); // send buffer
}


/**
 Send empty packet to specify that particular command/functionality is not supported.
 **/
VOID
EFIAPI
SendNotSupported (
  VOID
  )
{
  SendPacket ("");
}





/**
 Translates the EFI mapping to GDB mapping

 @param   EFIExceptionType    EFI Exception that is being processed
 @retval  UINTN that corresponds to EFIExceptionType's GDB exception type number
 **/
UINT8
ConvertEFItoGDBtype (
  IN  EFI_EXCEPTION_TYPE      EFIExceptionType
  )
{
  UINTN i;

  for (i=0; i < MaxEfiException() ; i++) {
    if (gExceptionType[i].Exception == EFIExceptionType) {
      return gExceptionType[i].SignalNo;
    }
  }
  return GDB_SIGTRAP; // this is a GDB trap
}


/** "m addr,length"
 Find the Length of the area to read and the start addres. Finally, pass them to
 another function, TransferFromMemToOutBufAndSend, that will read from that memory space and
 send it as a packet.
 **/

VOID
EFIAPI
ReadFromMemory (
  CHAR8 *PacketData
  )
{
  UINTN Address;
  UINTN Length;
  CHAR8 AddressBuffer[MAX_ADDR_SIZE]; // the buffer that will hold the address in hex chars
  CHAR8 *AddrBufPtr; // pointer to the address buffer
  CHAR8 *InBufPtr; /// pointer to the input buffer

  AddrBufPtr = AddressBuffer;
  InBufPtr = &PacketData[1];
  while (*InBufPtr != ',') {
    *AddrBufPtr++ = *InBufPtr++;
  }
  *AddrBufPtr = '\0';

  InBufPtr++; // this skips ',' in the buffer

  /* Error checking */
  if (AsciiStrLen(AddressBuffer) >= MAX_ADDR_SIZE) {
    SendError (GDB_EBADMEMADDRBUFSIZE);
    return;
  }

  // 2 = 'm' + ','
  if (AsciiStrLen(PacketData) - AsciiStrLen(AddressBuffer) - 2 >= MAX_LENGTH_SIZE) {
    SendError (GDB_EBADMEMLENGTH);
    return;
  }

  Address = AsciiStrHexToUintn (AddressBuffer);
  Length = AsciiStrHexToUintn (InBufPtr);

  TransferFromMemToOutBufAndSend (Length, (unsigned char *)Address);
}


/** "M addr,length :XX..."
 Find the Length of the area in bytes to write and the start addres. Finally, pass them to
 another function, TransferFromInBufToMem, that will write to that memory space the info in
 the input buffer.
 **/
VOID
EFIAPI
WriteToMemory (
  IN CHAR8 *PacketData
  )
{
  UINTN Address;
  UINTN Length;
  UINTN MessageLength;
  CHAR8 AddressBuffer[MAX_ADDR_SIZE]; // the buffer that will hold the Address in hex chars
  CHAR8 LengthBuffer[MAX_LENGTH_SIZE]; // the buffer that will hold the Length in hex chars
  CHAR8 *AddrBufPtr; // pointer to the Address buffer
  CHAR8 *LengthBufPtr; // pointer to the Length buffer
  CHAR8 *InBufPtr; /// pointer to the input buffer

  AddrBufPtr = AddressBuffer;
  LengthBufPtr = LengthBuffer;
  InBufPtr = &PacketData[1];

  while (*InBufPtr != ',') {
    *AddrBufPtr++ = *InBufPtr++;
  }
  *AddrBufPtr = '\0';

  InBufPtr++; // this skips ',' in the buffer

  while (*InBufPtr != ':') {
    *LengthBufPtr++ = *InBufPtr++;
  }
  *LengthBufPtr = '\0';

  InBufPtr++; // this skips ':' in the buffer

  Address = AsciiStrHexToUintn (AddressBuffer);
  Length = AsciiStrHexToUintn (LengthBuffer);

  /* Error checking */

  //Check if Address is not too long.
  if (AsciiStrLen(AddressBuffer) >= MAX_ADDR_SIZE) {
    SendError (GDB_EBADMEMADDRBUFSIZE);
    return;
  }

  //Check if message length is not too long
  if (AsciiStrLen(LengthBuffer) >= MAX_LENGTH_SIZE) {
    SendError (GDB_EBADMEMLENGBUFSIZE);
    return;
  }

  // Check if Message is not too long/short.
  // 3 = 'M' + ',' + ':'
  MessageLength = (AsciiStrLen(PacketData) - AsciiStrLen(AddressBuffer) - AsciiStrLen(LengthBuffer) - 3);
  if (MessageLength != (2*Length)) {
    //Message too long/short. New data is not the right size.
    SendError (GDB_EBADMEMDATASIZE);
    return;
  }
  TransferFromInBufToMem (Length, (unsigned char *)Address, InBufPtr);
}

/**
  Parses breakpoint packet data and captures Breakpoint type, Address and length.
  In case of an error, function returns particular error code. Returning 0 meaning
  no error.

  @param  PacketData  Pointer to the payload data for the packet.
  @param  Type        Breakpoint type
  @param  Address     Breakpoint address
  @param  Length      Breakpoint length in Bytes (1 byte, 2 byte, 4 byte)

  @retval 1           Success
  @retval {other}     Particular error code

**/
UINTN
ParseBreakpointPacket (
  IN  CHAR8 *PacketData,
  OUT UINTN *Type,
  OUT UINTN *Address,
  OUT UINTN *Length
  )
{
  CHAR8 AddressBuffer[MAX_ADDR_SIZE];
  CHAR8 *AddressBufferPtr;
  CHAR8 *PacketDataPtr;

  PacketDataPtr = &PacketData[1];
  AddressBufferPtr = AddressBuffer;

  *Type = AsciiStrHexToUintn (PacketDataPtr);

  //Breakpoint/watchpoint type should be between 0 to 4
  if (*Type > 4) {
    return 22; //EINVAL: Invalid argument.
  }

  //Skip ',' in the buffer.
  while (*PacketDataPtr++ != ',');

  //Parse Address information
  while (*PacketDataPtr != ',') {
    *AddressBufferPtr++ = *PacketDataPtr++;
  }
  *AddressBufferPtr = '\0';

  //Check if Address is not too long.
  if (AsciiStrLen(AddressBuffer) >= MAX_ADDR_SIZE) {
    return 40; //EMSGSIZE: Message size too long.
  }

  *Address = AsciiStrHexToUintn (AddressBuffer);

  PacketDataPtr++; //This skips , in the buffer

  //Parse Length information
  *Length = AsciiStrHexToUintn (PacketDataPtr);

  //Length should be 1, 2 or 4 bytes
  if (*Length > 4) {
    return 22; //EINVAL: Invalid argument
  }

  return 0; //0 = No error
}



/**
 Send the T signal with the given exception type (in gdb order) and possibly with n:r pairs related to the watchpoints

 @param  SystemContext        Register content at time of the exception
 @param  GdbExceptionType     GDB exception type
 **/
VOID
GdbSendTSignal (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINT8               GdbExceptionType
  )
{
  CHAR8 TSignalBuffer[128];
  CHAR8 *TSignalPtr;

  TSignalPtr = &TSignalBuffer[0];

  //Construct TSignal packet
  *TSignalPtr++ = 'T';

  //
  // replace _, or previous value, with Exception type
  //
  *TSignalPtr++ = mHexToStr [GdbExceptionType >> 4];
  *TSignalPtr++ = mHexToStr [GdbExceptionType & 0x0f];

  ProcessorSendTSignal (SystemContext, GdbExceptionType, TSignalPtr, sizeof (TSignalBuffer) - 2);

  SendPacket (TSignalBuffer);
}

VOID
GdbFWrite (
  IN  UINTN Fd,
  IN  CHAR8 *Data,
  IN  UINTN DataSize
  )
{
  CHAR8 Buffer[128];

  AsciiSPrint (Buffer, sizeof (Buffer), "Fwrite,%x,%x,%x", Fd, Data, DataSize);
  SendPacket (Buffer);

  for( ; ; ) {
    ReceivePacket (gInBuffer, MAX_BUF_SIZE);

    switch (gInBuffer[0]) {
    case 'm':
      ReadFromMemory (gInBuffer);
      break;

    case 'M':
      WriteToMemory (gInBuffer);
      break;

    case 'F':
      return;
    }
  }
}


VOID
GdbFPutString (
  IN CHAR8  *String
  )
{
  UINTN Len = AsciiStrSize (String);

  GdbFWrite (2, String, Len);
}


/**
 Exception Hanldler for GDB. It will be called for all exceptions
 registered via the gExceptionType[] array.

 @param ExceptionType     Exception that is being processed
 @param SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
GdbExceptionHandler (
  IN  EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT     SystemContext
  )
{
  UINT8   GdbExceptionType;
  CHAR8   *Ptr;

  if (ProcessorControlC (ExceptionType, SystemContext)) {
    // We tried to process a control C handler and there is nothing to do
    return;
  }

  GdbExceptionType = ConvertEFItoGDBtype (ExceptionType);
  GdbSendTSignal (SystemContext, GdbExceptionType);

  for( ; ; ) {
    ReceivePacket (gInBuffer, MAX_BUF_SIZE);

    switch (gInBuffer[0]) {
      case '?':
        GdbSendTSignal (SystemContext, GdbExceptionType);
        break;

      case 'c':
        ContinueAtAddress (SystemContext, gInBuffer);
        return;

      case 'D':
        // gdb wants to disconnect so return "OK" packet since.
        SendSuccess ();
        return;

      case 'g':
        ReadGeneralRegisters (SystemContext);
        break;

      case 'G':
        WriteGeneralRegisters (SystemContext, gInBuffer);
        break;

      case 'H':
        //Return "OK" packet since we don't have more than one thread.
        SendSuccess ();
        break;

      case 'm':
        ReadFromMemory (gInBuffer);
        break;

      case 'M':
        WriteToMemory (gInBuffer);
        break;

      case 'P':
        WriteNthRegister (SystemContext, gInBuffer);
        break;

      //
      // Still debugging this code. Not used in Darwin
      //
      case 'q':
        // General Query Packets
        if (AsciiStrnCmp (gInBuffer, "qSupported", 10) == 0) {
          // return what we currently support, we don't parse what gdb suports
          AsciiSPrint (gOutBuffer, MAX_BUF_SIZE, "qXfer:libraries:read+;PacketSize=%d", MAX_BUF_SIZE);
          SendPacket (gOutBuffer);
        } else if (AsciiStrnCmp (gInBuffer, "qXfer:libraries:read::", 22) == 0) {
          // ‘qXfer:libraries:read::offset,length
          // gInBuffer[22] is offset string, ++Ptr is length string’
          for (Ptr = &gInBuffer[22]; *Ptr != ','; Ptr++);

          // Not sure if multi-radix support is required. Currently only support decimal
          QxferLibrary (AsciiStrHexToUintn (&gInBuffer[22]), AsciiStrHexToUintn (++Ptr));
        } else if (AsciiStrnCmp (gInBuffer, "qOffsets", 8) == 0) {
          AsciiSPrint (gOutBuffer, MAX_BUF_SIZE, "Text=1000;Data=f000;Bss=f000");
          SendPacket (gOutBuffer);
        } else if (AsciiStrnCmp (gInBuffer, "qAttached", 9) == 0) {
          // remote server attached to an existing process
          SendPacket ("1");
        } else {
          //Send empty packet
          SendNotSupported ();
        }
        break;

      case 's':
        SingleStep (SystemContext, gInBuffer);
        return;

      case 'z':
        RemoveBreakPoint (SystemContext, gInBuffer);
        break;

      case 'Z':
        InsertBreakPoint (SystemContext, gInBuffer);
        break;

      default:
        //Send empty packet
        SendNotSupported ();
        break;
    }
  }
}





