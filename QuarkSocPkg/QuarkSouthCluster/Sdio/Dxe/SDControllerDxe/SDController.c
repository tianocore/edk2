/** @file

The SD host controller driver model and HC protocol routines.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#include "SDController.h"


EFI_DRIVER_BINDING_PROTOCOL gSDControllerDriverBinding = {
  SDControllerSupported,
  SDControllerStart,
  SDControllerStop,
  0x20,
  NULL,
  NULL
};


EFI_SD_HOST_IO_PROTOCOL  mSDHostIo = {
  EFI_SD_HOST_IO_PROTOCOL_REVISION_01,
  {
    0, // HighSpeedSupport
    0, // V18Support
    0, // V30Support
    0, // V33Support
    0, // Reserved0
    0, // BusWidth4
    0, // BusWidth8
    0, // Reserved1
    0, // Reserved1
    (512 * 1024) //BoundarySize
  },
  SendCommand,
  SetClockFrequency,
  SetBusWidth,
  SetHostVoltage,
  ResetSDHost,
  EnableAutoStopCmd,
  DetectCardAndInitHost,
  SetBlockLength,
  SetHighSpeedMode,
  SetDDRMode
};

/**
  Find sdclk_freq_sel and upr_sdclk_freq_sel bits
  for Clock Control Register (CLK_CTL)Offset 2Ch when using 8bit or 10bit
  divided clock mode.

  @param  BaseClockFreg        Base Clock Frequency in Hz For SD Clock in the
                               Capabilities register.
  @param  TargetFreq           Target Frequency in Hz to reach.
  @param  Is8BitMode           True if 8-bit Divided Clock Mode else 10bit mode.
  @param  Bits                 sdclk_freq_sel and upr_sdclk_freq_sel bits for
                               TargetFreq.

  @return EFI_SUCCESS          // Bits setup.
  @return EFI_UNSUPPORTED      // Cannot divide base clock to reach target clock.
**/
EFI_STATUS
DividedClockModeBits (
  IN CONST UINTN                          BaseClockFreg,
  IN CONST UINTN                          TargetFreq,
  IN CONST BOOLEAN                        Is8BitMode,
  OUT UINT16                              *Bits
  )
{
  UINTN                             N;
  UINTN                             CurrFreq;

 *Bits = 0;
  CurrFreq = BaseClockFreg;
  N = 0;
  //
  // N == 0 same for 8bit & 10bit mode i.e. BaseClockFreg of controller.
  //
  if (TargetFreq < CurrFreq) {
    if (Is8BitMode) {
      N = 1;
      do {
        //
        // N values for 8bit mode when N > 0.
        //  Bit[15:8] SDCLK Frequency Select at offset 2Ch
        //    80h - base clock divided by 256
        //    40h - base clock divided by 128
        //    20h - base clock divided by 64
        //    10h - base clock divided by 32
        //    08h - base clock divided by 16
        //    04h - base clock divided by 8
        //    02h - base clock divided by 4
        //    01h - base clock divided by 2
        //
        CurrFreq = BaseClockFreg / (2 * N);
        if (TargetFreq >= CurrFreq) {
          break;
        }
        N *= 2;
        if (N > V_MMIO_CLKCTL_MAX_8BIT_FREQ_SEL) {
          return EFI_UNSUPPORTED;
        }
      } while (TRUE);
    } else {
      N = 1;
      CurrFreq = BaseClockFreg / (2 * N);
      //
      // (try N = 0 or 1 first since don't want divide by 0).
      //
      if (TargetFreq < CurrFreq) {
        //
        // If still no match then calculate it for 10bit.
        // N values for 10bit mode.
        // N 1/2N Divided Clock (Duty 50%).
        // from Spec "The length of divider is extended to 10 bits and all
        // divider values shall be supported.
        //
        N = (BaseClockFreg / TargetFreq) / 2;

        //
        // Can only be N or N+1;
        //
        CurrFreq = BaseClockFreg / (2 * N);
        if (TargetFreq < CurrFreq) {
          N++;
          CurrFreq = BaseClockFreg / (2 * N);
        }

        if (N > V_MMIO_CLKCTL_MAX_10BIT_FREQ_SEL) {
          return EFI_UNSUPPORTED;
        }

        //
        // Set upper bits of SDCLK Frequency Select (bits 7:6 of reg 0x2c).
        //
        *Bits |= ((UINT16) ((N >> 2) & B_MMIO_CLKCTL_UPR_SDCLK_FREQ_SEL_MASK));
      }
    }
  }

  //
  // Set lower bits of SDCLK Frequency Select (bits 15:8 of reg 0x2c).
  //
  *Bits |= ((UINT16) ((UINT8) N) << 8);
  DEBUG (
    (EFI_D_INFO,
    "SDIO:DividedClockModeBits: %dbit mode Want %dHz Got %dHz bits = %04x\r\n",
    (Is8BitMode) ? 8 : 10,
    TargetFreq,
    CurrFreq,
    (UINTN) *Bits
     ));

  return EFI_SUCCESS;
}

/**
  Print type of error and command index

  @param  CommandIndex         Command index to set the command index field of command register.
  @param  ErrorCode            Error interrupt status read from host controller

  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT
  @return EFI_CRC_ERROR

**/
EFI_STATUS
GetErrorReason (
  IN  UINT16    CommandIndex,
  IN  UINT16    ErrorCode
  )
{
  EFI_STATUS    Status;

  Status = EFI_DEVICE_ERROR;

  DEBUG((EFI_D_ERROR, "[%2d] -- ", CommandIndex));

  if (ErrorCode & BIT0) {
    Status = EFI_TIMEOUT;
    DEBUG((EFI_D_ERROR, "Command Timeout Erro"));
  }

  if (ErrorCode & BIT1) {
    Status = EFI_CRC_ERROR;
    DEBUG((EFI_D_ERROR, "Command CRC Error"));
  }

  if (ErrorCode & BIT2) {
    DEBUG((EFI_D_ERROR, "Command End Bit Error"));
  }

  if (ErrorCode & BIT3) {
    DEBUG((EFI_D_ERROR, "Command Index Error"));
  }
  if (ErrorCode & BIT4) {
    Status = EFI_TIMEOUT;
    DEBUG((EFI_D_ERROR, "Data Timeout Error"));
  }

  if (ErrorCode & BIT5) {
    Status = EFI_CRC_ERROR;
    DEBUG((EFI_D_ERROR, "Data CRC Error"));
  }

  if (ErrorCode & BIT6) {
    DEBUG((EFI_D_ERROR, "Data End Bit Error"));
  }

  if (ErrorCode & BIT7) {
    DEBUG((EFI_D_ERROR, "Current Limit Error"));
  }

  if (ErrorCode & BIT8) {
    DEBUG((EFI_D_ERROR, "Auto CMD12 Error"));
  }

  if (ErrorCode & BIT9) {
    DEBUG((EFI_D_ERROR, "ADMA Error"));
  }

  DEBUG((EFI_D_ERROR, "\n"));

  return Status;
}
/**
  Enable/Disable High Speed transfer mode

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Enable                TRUE to Enable, FALSE to Disable

  @return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
SetHighSpeedMode (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  )
{
  UINT32                 Data;
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;

  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_HOSTCTL,
               1,
               &Data
               );

  if (Enable) {
    if (PcdGetBool(PcdSdHciQuirkNoHiSpd)) {
      DEBUG ((EFI_D_INFO, "SDIO: Quirk never set High Speed Enable bit\r\n"));
      return EFI_SUCCESS;
    }
    DEBUG ((EFI_D_INFO, "Enable High Speed transfer mode ... \r\n"));
    Data |= BIT2;
  } else {
    Data &= ~BIT2;
  }
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint8,
               0,
              (UINT64)MMIO_HOSTCTL,
               1,
               &Data
              );
  return EFI_SUCCESS;
}
EFI_STATUS
EFIAPI
SetDDRMode (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  )
{
  UINT16                 Data;
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;
  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_HOSTCTL2,
               1,
               &Data
               );
  Data &= 0xFFF0;
  if (Enable) {
    Data |= 0x0004; // Enable DDR50 by default, later should enable other mode like HS200/400
    Data |= BIT3;   // Enable 1.8V Signaling
  }
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
              (UINT64)MMIO_HOSTCTL2,
               1,
               &Data
              );
  return EFI_SUCCESS;
}
/**
  Power on/off the LED associated with the slot

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Enable                TRUE to set LED on, FALSE to set LED off

  @return EFI_SUCCESS
**/
EFI_STATUS
HostLEDEnable (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  )
{
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT32                 Data;

  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_HOSTCTL,
               1,
               &Data
               );

  if (Enable) {
    //
    //LED On
    //
    Data |= BIT0;
  } else {
    //
    //LED Off
    //
    Data &= ~BIT0;
  }

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_HOSTCTL,
               1,
               &Data
               );

  return EFI_SUCCESS;
}


/**
  The main function used to send the command to the card inserted into the SD host slot.
  It will assemble the arguments to set the command register and wait for the command
  and transfer completed until timeout. Then it will read the response register to fill
  the ResponseData.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  CommandIndex          The command index to set the command index field of command register.
  @param  Argument              Command argument to set the argument field of command register.
  @param  DataType              TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                Contains the data read from / write to the device.
  @param  BufferSize            The size of the buffer.
  @param  ResponseType          RESPONSE_TYPE.
  @param  TimeOut               Time out value in 1 ms unit.
  @param  ResponseData          Depending on the ResponseType, such as CSD or card status.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_TIMEOUT
  @retval EFI_DEVICE_ERROR

**/

EFI_STATUS
EFIAPI
SendCommand (
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData OPTIONAL
  )
/*++

  Routine Description:
    The main function used to send the command to the card inserted into the SD host
    slot.
    It will assemble the arguments to set the command register and wait for the command
    and transfer completed until timeout. Then it will read the response register to fill
    the ResponseData

  Arguments:
    This           - Pointer to EFI_SD_HOST_IO_PROTOCOL
    CommandIndex   - The command index to set the command index field of command register
    Argument       - Command argument to set the argument field of command register
    DataType       - TRANSFER_TYPE, indicates no data, data in or data out
    Buffer         - Contains the data read from / write to the device
    BufferSize     - The size of the buffer
    ResponseType   - RESPONSE_TYPE
    TimeOut        - Time out value in 1 ms unit
    ResponseData   - Depending on the ResponseType, such as CSD or card status

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_OUT_OF_RESOURCES
    EFI_TIMEOUT
    EFI_DEVICE_ERROR

--*/
{
  EFI_STATUS            Status;
  SDHOST_DATA           *SDHostData;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT32                ResponseDataCount;
  UINT32                Data;
  UINT64                Data64;
  UINT8                 Index;
  INTN                  TimeOut2;
  BOOLEAN               AutoCMD12Enable = FALSE;


  Status             = EFI_SUCCESS;
  ResponseDataCount  = 1;
  SDHostData         = SDHOST_DATA_FROM_THIS (This);
  PciIo              = SDHostData->PciIo;
  AutoCMD12Enable    =  (CommandIndex & AUTO_CMD12_ENABLE) ? TRUE : FALSE;
  CommandIndex       = CommandIndex & CMD_INDEX_MASK;

  if (Buffer != NULL && DataType == NoData) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "SendCommand: invalid parameter \r\n"));
    goto Exit;
  }

  if (((UINTN)Buffer & (This->HostCapability.BoundarySize - 1)) != (UINTN)NULL) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "SendCommand: invalid parameter \r\n"));
    goto Exit;
  }

  DEBUG ((EFI_D_INFO, "SendCommand: Command Index = %d \r\n", CommandIndex));
  //
  TimeOut2 = 1000; // 10 ms
  do {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint32,
                 0,
                 (UINT64)MMIO_PSTATE,
                 1,
                 &Data
                 );
     gBS->Stall (10);
  }while ((TimeOut2-- > 0) && (Data & BIT0));
  TimeOut2 = 1000; // 10 ms
  do {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint32,
                 0,
                 (UINT64)MMIO_PSTATE,
                 1,
                 &Data
                 );
    gBS->Stall (10);
  }while ((TimeOut2-- > 0) && (Data & BIT1));
  //Clear status bits
  //
  Data = 0xFFFF;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_NINTSTS,
               1,
               &Data
               );

  Data = 0xFFFF;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_ERINTSTS,
               1,
               &Data
               );


  if (Buffer != NULL) {
     PciIo->Mem.Write (
                  PciIo,
                  EfiPciIoWidthUint32,
                  0,
                  (UINT64)MMIO_DMAADR,
                  1,
                  &Buffer
                  );

     PciIo->Mem.Read (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0,
                  (UINT64)MMIO_BLKSZ,
                  1,
                  &Data
                  );
     Data &= ~(0xFFF);
     if (BufferSize <= SDHostData->BlockLength) {
       Data |= (BufferSize | 0x7000);
     } else {
       Data |= (SDHostData->BlockLength | 0x7000);
     }


     PciIo->Mem.Write (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0,
                  (UINT64)MMIO_BLKSZ,
                  1,
                  &Data
                  );
     if (BufferSize <= SDHostData->BlockLength) {
       Data = 1;
     } else {
       Data = BufferSize / SDHostData->BlockLength;
     }
     PciIo->Mem.Write (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0,
                  (UINT64)MMIO_BLKCNT,
                  1,
                  &Data
                  );

  } else {
    Data = 0;
    PciIo->Mem.Write (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0,
                  (UINT64)MMIO_BLKSZ,
                  1,
                  &Data
                  );
    PciIo->Mem.Write (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0,
                  (UINT64)MMIO_BLKCNT,
                  1,
                  &Data
                  );
  }

  //
  //Argument
  //
  Data = Argument;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               0,
               (UINT64)MMIO_CMDARG,
               1,
               &Data
               );


  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_XFRMODE,
               1,
               &Data
               );


  DEBUG ((EFI_D_INFO, "Transfer mode read  = 0x%x \r\n", (Data & 0xFFFF)));
  //
  //BIT0 - DMA Enable
  //BIT2 - Auto Cmd12
  //
  if (DataType == InData) {
    Data |= BIT4 | BIT0;
  } else if (DataType == OutData){
    Data &= ~BIT4;
    Data |= BIT0;
  } else {
    Data &= ~(BIT4 | BIT0);
  }

  if (BufferSize <= SDHostData->BlockLength) {
    Data &= ~ (BIT5 | BIT1 | BIT2);
    Data |= BIT1; // Enable block count always
  } else {
     if (SDHostData->IsAutoStopCmd && AutoCMD12Enable) {
      Data |= (BIT5 | BIT1 | BIT2);
     } else {
      Data |= (BIT5 | BIT1);
     }
  }

  DEBUG ((EFI_D_INFO, "Transfer mode write = 0x%x \r\n", (Data & 0xffff)));
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_XFRMODE,
               1,
               &Data
               );
  //
  //Command
  //
  //ResponseTypeSelect    IndexCheck    CRCCheck    ResponseType
  //  00                     0            0           NoResponse
  //  01                     0            1           R2
  //  10                     0            0           R3, R4
  //  10                     1            1           R1, R5, R6, R7
  //  11                     1            1           R1b, R5b
  //
  switch (ResponseType) {
    case ResponseNo:
      Data = (CommandIndex << 8);
      ResponseDataCount = 0;
      break;

    case ResponseR1:
    case ResponseR5:
    case ResponseR6:
    case ResponseR7:
      Data = (CommandIndex << 8) | BIT1 | BIT4| BIT3;
      ResponseDataCount = 1;
      break;

    case ResponseR1b:
    case ResponseR5b:
      Data = (CommandIndex << 8) | BIT0 | BIT1 | BIT4| BIT3;
      ResponseDataCount = 1;
      break;

    case ResponseR2:
      Data = (CommandIndex << 8) | BIT0 | BIT3;
      ResponseDataCount = 4;
      break;

    case ResponseR3:
    case ResponseR4:
      Data = (CommandIndex << 8) | BIT1;
      ResponseDataCount = 1;
      break;

    default:
      ASSERT (0);
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((EFI_D_ERROR, "SendCommand: invalid parameter \r\n"));
      goto Exit;
  }

  if (DataType != NoData) {
    Data |= BIT5;
  }

  HostLEDEnable (This, TRUE);


  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_SDCMD,
               1,
               &Data
               );


  Data = 0;
  do {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint16,
                 0,
                 (UINT64)MMIO_ERINTSTS,
                 1,
                 &Data
                 );

    if ((Data & 0x07FF) != 0) {
      Status = GetErrorReason (CommandIndex, (UINT16)Data);
      DEBUG ((EFI_D_ERROR, "SendCommand: Error happens \r\n"));
      goto Exit;
    }

    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint16,
                 0,
                 (UINT64)MMIO_NINTSTS,
                 1,
                 &Data
                 );

    if ((Data & BIT0) == BIT0) {
       //
       //Command completed, can read response
       //
       if (DataType == NoData) {
         break;
       } else {
         //
         //Transfer completed
         //
         if ((Data & BIT1) == BIT1) {
           break;
         }
       }
    }

    gBS->Stall (1 * 1000);

    TimeOut --;

  } while (TimeOut > 0);

  if (TimeOut == 0) {
    Status = EFI_TIMEOUT;
    DEBUG ((EFI_D_ERROR, "SendCommand: Time out \r\n"));
    goto Exit;
  }

  if (ResponseData != NULL) {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint32,
                 0,
                 (UINT64)MMIO_RESP,
                 ResponseDataCount,
                 ResponseData
                 );
    if (ResponseType == ResponseR2) {
      //
      // Adjustment for R2 response
      //
      Data = 1;
      for (Index = 0; Index < ResponseDataCount; Index++) {
        Data64 = LShiftU64(*ResponseData, 8);
        *ResponseData = (UINT32)((Data64 & 0xFFFFFFFF) | Data);
        Data =  (UINT32)RShiftU64 (Data64, 32);
        ResponseData++;
      }
    }
  }

Exit:
  HostLEDEnable (This, FALSE);
  return Status;
}

/**
  Set max clock frequency of the host, the actual frequency may not be the same as MaxFrequency.
  It depends on the max frequency the host can support, divider, and host speed mode.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  MaxFrequency          Max frequency in HZ.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
SetClockFrequency (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     MaxFrequency
  )
{
  UINT32                 Data;
  UINT16                 FreqSelBits;
  EFI_STATUS             Status;
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT32                 TimeOutCount;
  UINT32                 Revision;

  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;
  Data = 0;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &Data
               );

  PciIo->Mem.Read (
                PciIo,
                EfiPciIoWidthUint8,
                0,
                (UINT64)MMIO_CTRLRVER,
                1,
                &Revision
                );
  Revision &= 0x000000FF;

  Status = DividedClockModeBits (
             SDHostData->BaseClockInMHz * 1000 * 1000,
             MaxFrequency,
             (Revision < SDHCI_SPEC_300),
             &FreqSelBits
             );

  if (EFI_ERROR (Status)) {
    //
    // Cannot reach MaxFrequency with SDHostData->BaseClockInMHz.
    //
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Data = 0;

  //
  //Enable internal clock and Stop Clock Enable
  //
  Data = BIT0;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &Data
               );

  TimeOutCount = TIME_OUT_1S;
  do {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint16,
                 0,
                 (UINT64)MMIO_CLKCTL,
                 1,
                 &Data
                 );
    gBS->Stall (1 * 1000);
    TimeOutCount --;
    if (TimeOutCount == 0) {
      DEBUG ((EFI_D_ERROR, "SetClockFrequency: Time out \r\n"));
      return EFI_TIMEOUT;
    }
  } while ((Data & BIT1) != BIT1);

  DEBUG ((EFI_D_INFO, "Base Clock In MHz: %d\r\n", SDHostData->BaseClockInMHz));

  Data = (BIT0 | ((UINT32) FreqSelBits));
  DEBUG ((EFI_D_INFO, "Data write to MMIO_CLKCTL: 0x%04x \r\n", Data));
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &Data
               );

  TimeOutCount = TIME_OUT_1S;
  do {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint16,
                 0,
                 (UINT64)MMIO_CLKCTL,
                 1,
                 &Data
                 );
    gBS->Stall (1 * 1000);
    TimeOutCount --;
    if (TimeOutCount == 0) {
      DEBUG ((EFI_D_ERROR, "SetClockFrequency: Time out \r\n"));
      return EFI_TIMEOUT;
    }
  } while ((Data & BIT1) != BIT1);
  gBS->Stall (20 * 1000);
  Data |= BIT2;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &Data
               );

  return EFI_SUCCESS;
}

/**
  Set bus width of the host controller

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  BusWidth              Bus width in 1, 4, 8 bits.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
SetBusWidth (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BusWidth
  )
{
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT8                  Data;

  SDHostData = SDHOST_DATA_FROM_THIS (This);


  if ((BusWidth != 1) && (BusWidth != 4) && (BusWidth != 8)) {
    DEBUG ((EFI_D_ERROR, "SetBusWidth: Invalid parameter \r\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((SDHostData->SDHostIo.HostCapability.BusWidth8 == FALSE) && (BusWidth == 8)) {
     DEBUG ((EFI_D_ERROR, "SetBusWidth: Invalid parameter \r\n"));
     return EFI_INVALID_PARAMETER;
  }

  PciIo      = SDHostData->PciIo;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_HOSTCTL,
               1,
               &Data
               );
  //
  // BIT5 8-bit MMC Support (MMC8):
  // If set, IOH supports 8-bit MMC. When cleared, IOH does not support this feature
  //
  if (BusWidth == 8) {
    DEBUG ((EFI_D_INFO, "Bus Width is 8-bit ... \r\n"));
    Data |= BIT5;
  } else if (BusWidth == 4) {
    DEBUG ((EFI_D_INFO, "Bus Width is 4-bit ... \r\n"));
    Data &= ~BIT5;
    Data |= BIT1;
  } else {
    DEBUG ((EFI_D_INFO, "Bus Width is 1-bit ... \r\n"));
    Data &= ~BIT5;
    Data &= ~BIT1;
  }

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_HOSTCTL,
               1,
               &Data
               );

  return EFI_SUCCESS;
}


/**
  Set voltage which could supported by the host controller.
  Support 0(Power off the host), 1.8V, 3.0V, 3.3V

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Voltage               Units in 0.1 V.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
SetHostVoltage (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     Voltage
  )
{
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT8                  Data;
  EFI_STATUS             Status;

  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;
  Status     = EFI_SUCCESS;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_PWRCTL,
               1,
               &Data
               );

  if (Voltage == 0) {
    //
    //Power Off the host
    //
    Data &= ~BIT0;
  } else if (Voltage <= 18 && This->HostCapability.V18Support) {
     //
     //1.8V
     //
     Data |= (BIT1 | BIT3 | BIT0);
  } else if (Voltage > 18 &&  Voltage <= 30 && This->HostCapability.V30Support) {
     //
     //3.0V
     //
     Data |= (BIT2 | BIT3 | BIT0);
  } else if (Voltage > 30 && Voltage <= 33 && This->HostCapability.V33Support) {
     //
     //3.3V
     //
     Data |= (BIT1 | BIT2 | BIT3 | BIT0);
  } else {
     Status = EFI_UNSUPPORTED;
     goto Exit;
  }

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_PWRCTL,
               1,
               &Data
               );
  gBS->Stall (10 * 1000);

Exit:
  return Status;
}



/**
  Reset the host controller.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  ResetAll              TRUE to reset all.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
ResetSDHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  RESET_TYPE                 ResetType
  )
{
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT32                 Data;
  UINT16                 ErrStatus;
  UINT32                 Mask;
  UINT32                 TimeOutCount;
  UINT16                 SaveClkCtl;
  UINT16                 ZeroClkCtl;

  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;
  Mask       = 0;
  ErrStatus  = 0;

  if (ResetType == Reset_Auto) {
    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint16,
                 0,
                 (UINT64)MMIO_ERINTSTS,
                 1,
                 &ErrStatus
                 );
    if ((ErrStatus & 0xF) != 0) {
      //
      //Command Line
      //
      Mask |= BIT1;
    }
    if ((ErrStatus & 0x70) != 0) {
      //
      //Data Line
      //
      Mask |= BIT2;
    }
  }


  if (ResetType == Reset_DAT || ResetType == Reset_DAT_CMD) {
    Mask |= BIT2;
  }
  if (ResetType == Reset_CMD || ResetType == Reset_DAT_CMD) {
    Mask |= BIT1;
  }
  if (ResetType == Reset_All) {
    Mask = BIT0;
  }

  if (Mask == 0) {
    return EFI_SUCCESS;
  }

  //
  // To improve SD stability, we zero the MMIO_CLKCTL register and
  // stall for 50 microseconds before resetting the controller.  We
  // restore the register setting following the reset operation.
  //
  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &SaveClkCtl
               );

  ZeroClkCtl = (UINT16) 0;
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &ZeroClkCtl
               );

  gBS->Stall (50);

  //
  // Reset the SD host controller
  //
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_SWRST,
               1,
               &Mask
               );

  Data          = 0;
  TimeOutCount  = TIME_OUT_1S;
  do {

    gBS->Stall (1 * 1000);

    TimeOutCount --;

    PciIo->Mem.Read (
                 PciIo,
                 EfiPciIoWidthUint8,
                 0,
                 (UINT64)MMIO_SWRST,
                 1,
                 &Data
                 );
    if ((Data & Mask) == 0) {
      break;
    }
  } while (TimeOutCount > 0);

  //
  // We now restore the MMIO_CLKCTL register which we set to 0 above.
  //
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_CLKCTL,
               1,
               &SaveClkCtl
               );

  if (TimeOutCount == 0) {
    DEBUG ((EFI_D_ERROR, "ResetSDHost: Time out \r\n"));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


/**
  Enable auto stop on the host controller.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Enable                TRUE to enable, FALSE to disable.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
EnableAutoStopCmd (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  )
{
  SDHOST_DATA            *SDHostData;

  SDHostData = SDHOST_DATA_FROM_THIS (This);

  SDHostData->IsAutoStopCmd = Enable;

  return EFI_SUCCESS;
}

/**
  Set the Block length on the host controller.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  BlockLength           card supportes block length.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
SetBlockLength (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BlockLength
  )
{
  SDHOST_DATA            *SDHostData;

  SDHostData = SDHOST_DATA_FROM_THIS (This);

  DEBUG ((EFI_D_INFO, "Block length on the host controller: %d \r\n", BlockLength));
  SDHostData->BlockLength = BlockLength;

  return EFI_SUCCESS;
}


/**
  Find whether these is a card inserted into the slot. If so init the host.
  If not, return EFI_NOT_FOUND.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND

**/
EFI_STATUS
EFIAPI
DetectCardAndInitHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  )
{
  SDHOST_DATA            *SDHostData;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT32                 Data;
  EFI_STATUS             Status;
  UINT8                  Voltages[] = { 33, 30, 18 };
  UINTN                  Loop;

  SDHostData = SDHOST_DATA_FROM_THIS (This);
  PciIo      = SDHostData->PciIo;
  Status     = EFI_NOT_FOUND;

  Data = 0;
  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               0,
               (UINT64)MMIO_PSTATE,
               1,
               &Data
               );

  if ((Data & (BIT16 | BIT17 | BIT18)) != (BIT16 | BIT17 | BIT18)) {
    //
    // Has no card inserted
    //
    DEBUG ((EFI_D_INFO, "DetectCardAndInitHost: No Cards \r\n"));
    Status =  EFI_NOT_FOUND;
    goto Exit;
  }
  DEBUG ((EFI_D_INFO, "DetectCardAndInitHost: Find Cards \r\n"));

  Status =  EFI_NOT_FOUND;
  for (Loop = 0; Loop < sizeof (Voltages); Loop++) {
    DEBUG ((
      EFI_D_INFO,
      "DetectCardAndInitHost: SetHostVoltage %d.%dV \r\n",
      Voltages[Loop] / 10,
      Voltages[Loop] % 10
      ));
    Status = SetHostVoltage (This, Voltages[Loop]);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "DetectCardAndInitHost set voltages: [failed]\n"));
    } else {
      DEBUG ((EFI_D_INFO, "DetectCardAndInitHost set voltages: [success]\n"));
      break;
    }
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: Fail to set voltage \r\n"));
    goto Exit;
  }

  Status = SetClockFrequency (This, FREQUENCY_OD);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "DetectCardAndInitHost: Fail to set frequency \r\n"));
    goto Exit;
  }
  SetBusWidth (This, 1);

  //
  //Enable normal status change
  //

  Data = (BIT0 | BIT1);

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_NINTEN,
               1,
               &Data
               );

  //
  //Enable error status change
  //
  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_ERINTEN,
               1,
               &Data
               );

  Data |= (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7 | BIT8);

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint16,
               0,
               (UINT64)MMIO_ERINTEN,
               1,
               &Data
               );

  //
  //Data transfer Timeout control
  //
  Data = 0x0E;

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint8,
               0,
               (UINT64)MMIO_TOCTL,
               1,
               &Data
               );
  //
  //Set Default Bus width as 1 bit
  //

Exit:
  return Status;

}

/**
  Entry point for EFI drivers.

  @param  ImageHandle      EFI_HANDLE.
  @param  SystemTable      EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS      Driver is successfully loaded.
  @return Others           Failed.

**/
EFI_STATUS
EFIAPI
InitializeSDController (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gSDControllerDriverBinding,
           ImageHandle,
           &gSDControllerName,
           &gSDControllerName2
           );
}


/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has SDHostIoProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SDControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS            OpenStatus;
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  PCI_CLASSC            PciClass;
  EFI_SD_HOST_IO_PROTOCOL   *SdHostIo;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSDHostIoProtocolGuid,
                  (VOID **)&SdHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG (( DEBUG_INFO, "SdHost controller is already started\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (PCI_CLASSC) / sizeof (UINT8),
                        &PciClass
                        );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to SD type
  //
  if ((PciClass.BaseCode != PCI_CLASS_SYSTEM_PERIPHERAL) ||
      (PciClass.SubClassCode != PCI_SUBCLASS_SD_HOST_CONTROLLER) ||
      ((PciClass.PI != PCI_IF_STANDARD_HOST_NO_DMA) && (PciClass.PI != PCI_IF_STANDARD_HOST_SUPPORT_DMA))
      ) {

    Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}
/**
  Starting the SD Host Controller Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval EFI_UNSUPPORTED      This driver does not support this device.
  @retval EFI_DEVICE_ERROR     This driver cannot be started due to device Error.
                               EFI_OUT_OF_RESOURCES- Failed due to resource shortage.

**/
EFI_STATUS
EFIAPI
SDControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  SDHOST_DATA           *SDHostData;
  UINT32                Data;


  SDHostData = NULL;
  Data       = 0;

  //
  // Open PCI I/O Protocol and save pointer to open protocol
  // in private data area.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Enable the SD Host Controller MMIO space
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }


  SDHostData = (SDHOST_DATA*)AllocateZeroPool(sizeof (SDHOST_DATA));
  if (SDHostData == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  SDHostData->Signature   = SDHOST_DATA_SIGNATURE;
  SDHostData->PciIo       = PciIo;

  CopyMem (&SDHostData->SDHostIo, &mSDHostIo, sizeof (EFI_SD_HOST_IO_PROTOCOL));

  ResetSDHost (&SDHostData->SDHostIo, Reset_All);

  PciIo->Mem.Read (
              PciIo,
              EfiPciIoWidthUint16,
              0,
              (UINT64)MMIO_CTRLRVER,
              1,
              &Data
              );
  SDHostData->SDHostIo.HostCapability.HostVersion = Data & 0xFF;
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: HostVersion 0x%x \r\n", SDHostData->SDHostIo.HostCapability.HostVersion));

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               0,
               (UINT64)MMIO_CAP,
               1,
               &Data
               );
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: MMIO_CAP 0x%x \r\n", Data));
  if ((Data & BIT18) != 0) {
    SDHostData->SDHostIo.HostCapability.BusWidth8 = TRUE;
  }

  if ((Data & BIT21) != 0) {
    SDHostData->SDHostIo.HostCapability.HighSpeedSupport = TRUE;
  }

  if ((Data & BIT24) != 0) {
    SDHostData->SDHostIo.HostCapability.V33Support = TRUE;
  }

  if ((Data & BIT25) != 0) {
    SDHostData->SDHostIo.HostCapability.V30Support = TRUE;
  }

  if ((Data & BIT26) != 0) {
    SDHostData->SDHostIo.HostCapability.V18Support = TRUE;
  }

  SDHostData->SDHostIo.HostCapability.BusWidth4 = TRUE;

  if(SDHostData->SDHostIo.HostCapability.HostVersion < SDHCI_SPEC_300) {



      SDHostData->BaseClockInMHz = (Data >> 8) & 0x3F;
   }
   else {
      SDHostData->BaseClockInMHz = (Data >> 8) & 0xFF;

   }

  SDHostData->BlockLength = 512 << ((Data >> 16) & 0x03);
  DEBUG ((EFI_D_INFO, "SdHostDriverBindingStart: BlockLength 0x%x \r\n", SDHostData->BlockLength));
  SDHostData->IsAutoStopCmd  = TRUE;

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiSDHostIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &SDHostData->SDHostIo
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Install the component name protocol
  //
  SDHostData->ControllerNameTable = NULL;

  AddUnicodeString2 (
    "eng",
    gSDControllerName.SupportedLanguages,
    &SDHostData->ControllerNameTable,
    L"SD Host Controller",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gSDControllerName2.SupportedLanguages,
    &SDHostData->ControllerNameTable,
    L"SD Host Controller",
    FALSE
    );

Exit:
  if (EFI_ERROR (Status)) {
    if (SDHostData != NULL) {
      FreePool (SDHostData);
    }
  }

  return Status;
}


/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
EFIAPI
SDControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS               Status;
  EFI_SD_HOST_IO_PROTOCOL  *SDHostIo;
  SDHOST_DATA              *SDHostData;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSDHostIoProtocolGuid,
                  (VOID **) &SDHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetHostVoltage (SDHostIo, 0);

  SDHostData  = SDHOST_DATA_FROM_THIS(SDHostIo);

  //
  // Uninstall Block I/O protocol from the device handle
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSDHostIoProtocolGuid,
                  SDHostIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FreeUnicodeStringTable (SDHostData->ControllerNameTable);

  FreePool (SDHostData);

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}



