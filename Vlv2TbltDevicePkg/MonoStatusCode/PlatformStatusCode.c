/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  PlatformStatusCode.c

Abstract:

  Contains Platform specific implementations required to use status codes.

--*/

#include "PlatformStatusCode.h"
#include <PchRegs.h>
#include <PlatformBaseAddresses.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>

typedef struct {
  EFI_STATUS_CODE_DATA  DataHeader;
  EFI_HANDLE            Handle;
} PEIM_FILE_HANDLE_EXTENDED_DATA;

#define CONFIG_PORT0    0x4E
#define PCI_IDX         0xCF8
#define PCI_DAT         0xCFC

#define PCI_LPC_BASE    (0x8000F800)
#define PCI_LPC_REG(x)  (PCI_LPC_BASE + (x))

//
// Function implementations
//
BOOLEAN
PeiCodeTypeToPostCode (
  IN  EFI_STATUS_CODE_TYPE    CodeType,
  IN  EFI_STATUS_CODE_VALUE   Value,
  OUT UINT8                   *PostCode
  );

/**
  Provide a port 80 status code

  @param Same as ReportStatusCode PPI

  @retval EFI_SUCCESS   Always returns success.

**/
EFI_STATUS
EFIAPI
Port80ReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 *CallerId,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )

{
  EFI_STATUS               Status;
  EFI_FV_FILE_INFO         FvFileInfo;
  UINT16                   Port80Code = 0;

  //
  // Progress or error code, Output Port 80h card.
  //
  if (!PeiCodeTypeToPostCode (CodeType, Value, (UINT8 *)&Port80Code)) {
    if ((Data != NULL) && (Value ==(EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN))){
      Status = PeiServicesFfsGetFileInfo (
                 ((PEIM_FILE_HANDLE_EXTENDED_DATA *) (Data + 1))->Handle,
                 &FvFileInfo
                 );
      if (!EFI_ERROR (Status)) {
        Port80Code = (FvFileInfo.FileName.Data4[6]<<8) + (FvFileInfo.FileName.Data4[7]);
      }
    }
  }
  if (Port80Code != 0){
    IoWrite16 (0x80, (UINT16) Port80Code);
    DEBUG ((EFI_D_ERROR, "POSTCODE=<%04x>\n", Port80Code));
  }
  return  EFI_SUCCESS;
}

/**
  Provide a serial status code

  @param Same as ReportStatusCode PPI

  @retval EFI_SUCCESS   Always returns success.

**/
EFI_STATUS
EFIAPI
SerialReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 * CallerId,
  IN CONST EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
{
  CHAR8           *Filename;
  CHAR8           *Description;
  CHAR8           *Format;
  CHAR8           Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];
  UINT32          ErrorLevel;
  UINT32          LineNumber;
  UINTN           CharCount;
  BASE_LIST       Marker;

  Buffer[0] = '\0';

  if (Data != NULL &&
      ReportStatusCodeExtractAssertInfo (CodeType, Value, Data, &Filename, &Description, &LineNumber)) {
    //
    // Print ASSERT() information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "\n\rPEI_ASSERT!: %a (%d): %a\n\r",
                  Filename,
                  LineNumber,
                  Description
                  );
  } else if (Data != NULL &&
             ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
    //
    // Print DEBUG() information into output buffer.
    //
    CharCount = AsciiBSPrint (
                  Buffer,
                  sizeof (Buffer),
                  Format,
                  Marker
                  );
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
    //
    // Print ERROR information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "ERROR: C%x:V%x I%x",
                  CodeType,
                  Value,
                  Instance
                  );

    ASSERT(CharCount > 0);

    if (CallerId != NULL) {
      CharCount += AsciiSPrint (
                     &Buffer[CharCount],
                     (sizeof (Buffer) - (sizeof (Buffer[0]) * CharCount)),
                     " %g",
                     CallerId
                     );
    }

    if (Data != NULL) {
      CharCount += AsciiSPrint (
                     &Buffer[CharCount],
                     (sizeof (Buffer) - (sizeof (Buffer[0]) * CharCount)),
                     " %x",
                     Data
                     );
    }

    CharCount += AsciiSPrint (
                   &Buffer[CharCount],
                   (sizeof (Buffer) - (sizeof (Buffer[0]) * CharCount)),
                   "\n\r"
                   );
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
    //
    // Print PROGRESS information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "PROGRESS CODE: V%x I%x\n\r",
                  Value,
                  Instance
                  );
  } else if (Data != NULL &&
             CompareGuid (&Data->Type, &gEfiStatusCodeDataTypeStringGuid) &&
             ((EFI_STATUS_CODE_STRING_DATA *) Data)->StringType == EfiStringAscii) {
    //
    // EFI_STATUS_CODE_STRING_DATA
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "%a\n\r",
                  ((EFI_STATUS_CODE_STRING_DATA *) Data)->String.Ascii
                  );
  } else {
    //
    // Code type is not defined.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "Undefined: C%x:V%x I%x\n\r",
                  CodeType,
                  Value,
                  Instance
                  );
  }

  //
  // Call SerialPort Lib function to do print.
  //
  SerialPortWrite ((UINT8 *) Buffer, CharCount);

  return EFI_SUCCESS;
}

/**

  Call all status code listeners in the MonoStatusCode.

  @param PeiServices    The PEI core services table.
  @param CodeType       Type of Status Code.
  @param Value          Value to output for Status Code.
  @param Instance       Instance Number of this status code.
  @param CallerId       ID of the caller of this status code.
  @param Data           Optional data associated with this status code.

  @retval EFI_SUCCESS              If status code is successfully reported.
  @retval EFI_NOT_AVAILABLE_YET    If StatusCodePpi has not been installed.

**/
EFI_STATUS
EFIAPI
PlatformReportStatusCode (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 * CallerId,
  IN CONST EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
{
  //
  // If we are in debug mode, we will allow serial status codes
  //
  SerialReportStatusCode (PeiServices, CodeType, Value, Instance, CallerId, Data);

  Port80ReportStatusCode (PeiServices, CodeType, Value, Instance, CallerId, Data);

  return EFI_SUCCESS;
}

/**
  Install the PEIM.  Initialize listeners, publish the PPI and HOB for PEI and
  DXE use respectively.

  @param FfsHeader      FV this PEIM was loaded from.
  @param PeiServices    General purpose services available to every PEIM.

  @retval EFI_SUCCESS   The function always returns success.

**/
EFI_STATUS
EFIAPI
InstallMonoStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{

  //
  // Initialize all listeners
  //
  InitializeMonoStatusCode (FfsHeader, PeiServices);

  //
  // Publish the listener in a HOB for DXE use.
  //
  InitializeDxeReportStatusCode (PeiServices);

  return EFI_SUCCESS;
}

#define V_PCH_ILB_IRQE_UARTIRQEN_IRQ3             BIT3 // UART IRQ3 Enable
#define V_PCH_ILB_IRQE_UARTIRQEN_IRQ4             BIT4 // UART IRQ4 Enable
#define PCIEX_BASE_ADDRESS                        0xE0000000
#define PciD31F0RegBase                           PCIEX_BASE_ADDRESS + (UINT32) (31 << 15)
#define SB_RCBA                                   0xfed1c000

extern PCH_STEPPING EFIAPI PchStepping (VOID);

VOID
RamDebugInit (
  VOID
  );

/**
  Enable legacy decoding on ICH6

  @param none

  @retval EFI_SUCCESS     Always returns success.

**/
EFI_STATUS
EnableInternalUart(
  VOID
  )
{

  //
  // Program and enable PMC Base.
  //
  IoWrite32 (PCI_IDX,  PCI_LPC_REG(R_PCH_LPC_PMC_BASE));
  IoWrite32 (PCI_DAT,  (PMC_BASE_ADDRESS | B_PCH_LPC_PMC_BASE_EN));

  //
  // Enable COM1 for debug message output.
  //
  MmioAndThenOr32 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1, (UINT32) (~(B_PCH_PMC_GEN_PMCON_SUS_PWR_FLR + B_PCH_PMC_GEN_PMCON_PWROK_FLR)), BIT24);

  //
  // Silicon Steppings
  //
  if (PchStepping()>= PchB0)
    MmioOr8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) V_PCH_ILB_IRQE_UARTIRQEN_IRQ4);
  else
    MmioOr8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) V_PCH_ILB_IRQE_UARTIRQEN_IRQ3);
  MmioAnd32(IO_BASE_ADDRESS + 0x0520, (UINT32)~(0x00000187));
  MmioOr32 (IO_BASE_ADDRESS + 0x0520, (UINT32)0x81); // UART3_RXD-L
  MmioAnd32(IO_BASE_ADDRESS + 0x0530, (UINT32)~(0x00000007));
  MmioOr32 (IO_BASE_ADDRESS + 0x0530, (UINT32)0x1); // UART3_RXD-L
  MmioOr8 (PciD31F0RegBase + R_PCH_LPC_UART_CTRL, (UINT8) B_PCH_LPC_UART_CTRL_COM1_EN);

  return  EFI_SUCCESS;
}

/**
  INIT the SIO. Ported this code and I don't undertand the comments either.

  @param FfsHeader    FV this PEIM was loaded from.
  @param PeiServices  General purpose services available to every PEIM.

  None

**/
VOID
EFIAPI
PlatformInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{

  //
  // Enable internal COM1 on South Cluster.
  //
	EnableInternalUart();


  //
  // Initialize additional debug status code listeners.
  //
   SerialPortInitialize();

}
//#endif //EFI_DEBUG

