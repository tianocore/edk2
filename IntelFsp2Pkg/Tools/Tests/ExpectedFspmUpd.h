#ifndef __FSPMUPD_H__
#define __FSPMUPD_H__

#include <FspUpd.h>

#pragma pack(1)


/** Fsp M Configuration
**/
typedef struct {

/** Offset 0x00C8 - Debug Serial Port Base address
  Debug serial port base address. This option will be used only when the 'Serial Port
  Debug Device' option is set to 'External Device'. 0x00000000(Default).
**/
  UINT32                      SerialDebugPortAddress;

/** Offset 0x00CC - Debug Serial Port Type
  16550 compatible debug serial port resource type. NONE means no serial port support.
  0x02:MMIO(Default).
  0:NONE, 1:I/O, 2:MMIO
**/
  UINT8                       SerialDebugPortType;

/** Offset 0x00CD - Serial Port Debug Device
  Select active serial port device for debug.For SOC UART devices,'Debug Serial Port
  Base' options will be ignored. 0x02:SOC UART2(Default).
  0:SOC UART0, 1:SOC UART1, 2:SOC UART2, 3:External Device
**/
  UINT8                       SerialDebugPortDevice;

/** Offset 0x00CE - Debug Serial Port Stride Size
  Debug serial port register map stride size in bytes. 0x00:1, 0x02:4(Default).
  0:1, 2:4
**/
  UINT8                       SerialDebugPortStrideSize;

/** Offset 0x00CF
**/
  UINT8                       UnusedUpdSpace2[1];

/** Offset 0x00D0
**/
  UINT8                       ReservedFspmUpd[4];
} FSP_M_CONFIG;

/** Fsp M UPD Configuration
**/
typedef struct {

/** Offset 0x0000
**/
  FSP_UPD_HEADER              FspUpdHeader;

/** Offset 0x00A8
**/
  FSPM_ARCH_UPD               FspmArchUpd;

/** Offset 0x00C8
**/
  FSP_M_CONFIG                FspmConfig;

/** Offset 0x00D4
**/
  UINT8                       UnusedUpdSpace3[2];

/** Offset 0x00D6
**/
  UINT16                      UpdTerminator;
} FSPM_UPD;

#pragma pack()

#endif
