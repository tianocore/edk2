/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MIPI_SYST_H_
#define MIPI_SYST_H_

#include <Uefi.h>

/**
 * SyS-T API version information
**/
#define MIPI_SYST_VERSION_MAJOR  1   // Major version, incremented if API changes
#define MIPI_SYST_VERSION_MINOR  0   // Minor version, incremented on compatible extensions
#define MIPI_SYST_VERSION_PATCH  0   // Patch for existing major, minor, usually 0

/**
 * Compute SYS-T version value
 *
 * Used to compare SYS-T Major.Minor.patch versions numerically at runtime.
 *
 * @param ma major version number
 * @param mi minor version number
 * @param p patch version number
**/
#define MIPI_SYST_MAKE_VERSION_CODE(ma, mi, p)  (((ma) << 16) | ((mi)<<8) | (p))

/**
 * Numeric SYS-T version code
**/
#define MIPI_SYST_VERSION_CODE  MIPI_SYST_MAKE_VERSION_CODE(\
  MIPI_SYST_VERSION_MAJOR,\
  MIPI_SYST_VERSION_MINOR,\
  MIPI_SYST_VERSION_PATCH)

/**
 * Maximum size of printf payload in bytes.
 * Adjust this value if larger strings shall be supported by the library.
 * The buffer space is located in stack memory when calling one of the printf
 * style APIs.
**/
#define MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE  1024

/**
 * Enable message data CRC32 generation.
**/
#define MIPI_SYST_PCFG_ENABLE_CHECKSUM

#define MIPI_SYST_SCATTER_PROG_LEN  10    // maximum needed scatter prog size

typedef enum {
  TraceHubDebugType = 0,
  TraceHubCatalogType
} TRACEHUB_PRINTTYPE;

typedef struct {
  UINT32    EtType      : 4;    // SyS-T message type ID
  UINT32    EtSeverity  : 3;    // severity level of message
  UINT32    EtRes7      : 1;    // reserved for future use
  UINT32    EtLocation  : 1;    // indicate location information
  UINT32    EtLength    : 1;    // indicate length field
  UINT32    EtChksum    : 1;    // indicate 32-bit CRC
  UINT32    EtTimestamp : 1;    // indicate 64-bit timestamp
  UINT32    EtModunit   : 11;   // unit for GUID or module:unit
  UINT32    EtGuid      : 1;    // 128-bit GUID present
  UINT32    EtSubtype   : 6;    // type dependent sub category
  UINT32    EtRes30     : 1;    // reserved for future use
  UINT32    EtRes31     : 1;    // reserved for future use
} MIPI_SYST_MSG_TAG;

typedef union {
  UINT32    CatId32;
  UINT64    CatId64;
} CAT_ID;

typedef union {
  struct {
    UINT64        Id;
    CONST VOID    *Text;
  } Version;

  struct {
    CAT_ID    Id;
    UINT32    *Param;
  } CatId;

  UINT64        DataClock[2];
  CONST VOID    *DataVar;   // variable length payload
} ED_PLD;

/**
 * SyS-T message descriptor
 *
 *  This structure stores a SyS-T message in "logical" memory format.
 *  Logical means that all optional fields are present but not necessarily
 *  used. Variable length payloads are addressed through a pointer and
 *  are not copied into the structure.
**/
typedef struct {
  MIPI_SYST_MSG_TAG    EdTag;      // 32-bit message tag  (mandatory)
  EFI_GUID             EdGuid;     // origin GUID    (optional)
  UINT64               EdTs;       // protocol embedded time stamp
  UINT16               EdLen;      // variable payload length (optional)
  ED_PLD               EdPld;
  UINT32               EdChk;      // message checksum (optional)
} MIPI_SYST_MSGDSC;

typedef enum {
  MipiSystScatterOpSkip  = 0x00,  // skip sso_length bytes
  MipiSystScatterOp8bit  = 0x01,  // write sso_length times 8 bit
  MipiSystScatterOp16bit = 0x02,  // write sso_length times 16 bit
  MipiSystScatterOp32bit = 0x04,  // write sso_length times 32 bit
  MipiSystScatterOp64bit = 0x08,  // write sso_length times 64 bit
  MipiSystScatterOpBlob  = 0x10,  // write sso_length bytes that are accessed through a pointer
  MipiSystScatterOpEnd   = 0xFF   // end of scatter writer program
} U_SYST_SCATTER_OP;

/**
  message scatter write instruction definition
**/
typedef struct {
  UINT8     SsoOpcode; // scatter write operation
  UINT8     SsoOffset; // data offset in message descriptor
  UINT16    SsoLength; // repeat count for sso_opcode
} MIPI_SYST_SCATTER_PROG;

/**
 *
 * A Pre-defined Scatter Op index table which mapping to mScatterOps
 *
**/
typedef enum {
  ScatterOpGuid,
  ScatterOpLength,
  ScatterOpPayldVar,
  ScatterOpChecksum,
  ScatterOpCatId32,
  ScatterOpCatId64,
  ScatterOpCatIdArgs,
  ScatterOpClock,
  ScatterOpTs,
  ScatterOpVerId,
  ScatterOpVerTxt,
  ScatterOpEnd
} SYST_SCATTER_OPS;

/**
 * MIPI_SYST_TYPE_DEBUG_STRING Sub-Types
**/
typedef enum {
  MipiSystStringGeneric       = 1,    // string generic debug
  MipiSystStringFunctionEnter = 2,    // string is function name
  MipiSystStringFunctionExit  = 3,    // string is function name
  MipiSystStringInvalidParam  = 5,    // invalid SyS-T APIcall
  MipiSystStringAssert        = 7,    // Software Assert: failure
  MipiSystStringPrintf32      = 11,   // printf with 32-bit packing
  MipiSystStringPrintf64      = 12,   // printf with 64-bit packing
  MipiSystStringMax
} MIPI_SYST_SUBTYPE_STRING;

/**
 * Major Message Types
**/
typedef enum {
  MipiSystTypeBuild   = 0,          // client build id message
  MipiSystTypeShort32 = 1,          // value only message
  MipiSystTypeString  = 2,          // text message output
  MipiSystTypeCatalog = 3,          // catalog message output
  MipiSystTypeRaw     = 6,          // raw binary data
  MipiSystTypeShort64 = 7,          // value only message
  MipiSystTypeClock   = 8,          // clock sync message
  MipiSystTypeMax
} MIPI_SYST_MSGTYPE;

/**
 * MipiSystTypeCatalog Sub-Types
**/
typedef enum {
  MipiSystCatalogId32P32 = 1,   // 32-bit catalog ID, 32-bit packing
  MipiSystCatalogId64P32 = 2,   // 64-bit catalog ID, 32-bit packing
  MipiSystCatalogId32P64 = 5,   // 32-bit catalog ID, 64-bit packing
  MipiSystCatalogId64P64 = 6,   // 64-bit catalog ID, 64-bit packing
  MipiSystCatalogMax
} MIPI_SYST_SUBTYPE_CATALOG;

/**
 * Message severity level enumeration
**/
typedef enum {
  MipiSystSeverityMax     = 0,    // no assigned severity
  MipiSystSeverityFatal   = 1,    // critical error level
  MipiSystSeverityError   = 2,    // error message level
  MipiSystSeverityWarning = 3,    // warning message level
  MipiSystSeverityInfo    = 4,    // information message level
  MipiSystSeverityUser1   = 5,    // user defined level 5
  MipiSystSeverityUser2   = 6,    // user defined level 6
  MipiSystSeverityDebug   = 7     // debug information level
} MIPI_SYST_SEVERITY;

/** SyS-T connection handle state structure
 *
 * This structure connects the instrumentation API with the underlying SyS-T
 * infrastructure. It plays a similar role to a FILE * in traditional
 * C file IO.
**/
typedef struct MIPI_SYST_HANDLE {
  UINT32               SysthVersion;                 // SyS-T version ID
  MIPI_SYST_MSG_TAG    SystHandleTag;                // tag flags
  EFI_GUID             SystHandleGuid;               // module GUID
  UINT32               SystHandleParamCount;         // number of parameters
  UINT32               SystHandleParam[6];           // catalog msg parameters
  UINT8                *ThDebugMmioAddress;          // Trace Hub MMIO address
} MIPI_SYST_HANDLE;

#endif //MIPI_SYST_H_
