/** @file
Prototypes and defines for the QNC SMM Dispatcher.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef QNC_SMM_H
#define QNC_SMM_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmmRegisters.h"

extern EFI_HANDLE  mQNCSmmDispatcherImageHandle;


//
// /////////////////////////////////////////////////////////////////////////////
// SUPPORTED PROTOCOLS
//

//
// Define an enumeration for all the supported protocols
//
typedef enum {
  // UsbType,    DELETE:on QuarkNcSocId, there is no usb smi supported
  SxType,
  SwType,
  GpiType,
  QNCnType,
  PowerButtonType,
  PeriodicTimerType,
  NUM_PROTOCOLS
} QNC_SMM_PROTOCOL_TYPE;

//
// /////////////////////////////////////////////////////////////////////////////
// SPECIFYING A REGISTER
// We want a general way of referring to addresses.  For this case, we'll only
// need addresses in the ACPI table (and the TCO entries within the ACPI table).
// However, it's interesting to consider what it would take to support other types
// of addresses.  To address Will's concern, I think it prudent to accommodate it
// early on in the design.
//
// Addresses we need to consider:
//
//  Type:                           Required:
//  I/O                             Yes
//    ACPI (special case of I/O)    Only if we want to
//    TCO  (special case of ACPI)   Only if we want to
//  Memory (or Memory Mapped I/O)   Only if we want to
//  PCI                             Yes, for BiosWp
//
typedef enum {
  //
  //  IO_ADDR_TYPE, // unimplemented
  //
  ACPI_ADDR_TYPE,
  GPE_ADDR_TYPE,
  //
  //  MEMORY_ADDR_TYPE, // unimplemented
  //
  MEMORY_MAPPED_IO_ADDRESS_TYPE,
  PCI_ADDR_TYPE,
  NUM_ADDR_TYPES,                     // count of items in this enum
  QNC_SMM_ADDR_TYPE_NULL        = -1  // sentinel to indicate NULL or to signal end of arrays
} ADDR_TYPE;

//
// Assumption: 32-bits -- enum's evaluate to integer
// Assumption: This code will only run on IA-32.  Justification: IA-64 doesn't have SMIs.
// We don't have to worry about 64-bit addresses.
// Typedef the size of addresses in case the numbers I'm using are wrong or in case
// this changes.  This is a good idea because PCI_ADDR will change, for example, when
// we add support for PciExpress.
//
typedef UINT16 IO_ADDR;
typedef IO_ADDR ACPI_ADDR;  // can omit
typedef IO_ADDR GPE_ADDR;  // can omit
typedef IO_ADDR TCO_ADDR;   // can omit
typedef VOID *MEM_ADDR;
typedef MEM_ADDR MEMORY_MAPPED_IO_ADDRESS;
typedef union {
  UINT32  Raw;
  struct {
    UINT8 Reg;
    UINT8 Fnc;
    UINT8 Dev;
    UINT8 Bus;
  } Fields;
} PCI_ADDR;

typedef struct {
  ADDR_TYPE Type;
  union {
    //
    // used to initialize during declaration/definition
    //
    UINTN                     raw;

    //
    // used to access useful data
    //
    IO_ADDR                   io;
    ACPI_ADDR                 acpi;
    GPE_ADDR                  gpe;
    TCO_ADDR                  tco;
    MEM_ADDR                  mem;
    MEMORY_MAPPED_IO_ADDRESS  Mmio;
    PCI_ADDR                  pci;

  } Data;

} QNC_SMM_ADDRESS;
//
// Assumption: total size is 64 bits (32 for type and 32 for data) or 8 bytes
//
#define EFI_PCI_ADDRESS_PORT  0xcf8
#define EFI_PCI_DATA_PORT     0xcfc

//
// /////////////////////////////////////////////////////////////////////////////
// SPECIFYING BITS WITHIN A REGISTER
// Here's a struct that helps us specify a source or enable bit.
//
typedef struct {
  QNC_SMM_ADDRESS Reg;
  UINT8           SizeInBytes;  // of the register
  UINT8           Bit;
} QNC_SMM_BIT_DESC;

//
// Sometimes, we'll have bit descriptions that are unused.  It'd be great to have a
// way to easily identify them:
//
#define IS_BIT_DESC_NULL(BitDesc)   ((BitDesc).Reg.Type == QNC_SMM_ADDR_TYPE_NULL)  // "returns" true when BitDesc is NULL
#define NULL_THIS_BIT_DESC(BitDesc) ((BitDesc).Reg.Type = QNC_SMM_ADDR_TYPE_NULL)   // will "return" an integer w/ value of 0
#define NULL_BIT_DESC_INITIALIZER \
  { \
    { \
      QNC_SMM_ADDR_TYPE_NULL, \
      { \
        0 \
      } \
    }, \
    0, 0 \
  }
//
// I'd like a type to specify the callback's Sts & En bits because they'll
// be commonly used together:
//
#define NUM_EN_BITS   2
#define NUM_STS_BITS  1

//
// Flags
//
typedef UINT8 QNC_SMM_SOURCE_FLAGS;

//
// Flags required today
//
#define QNC_SMM_NO_FLAGS               0
#define QNC_SMM_SCI_EN_DEPENDENT      (BIT0)
#define QNC_SMM_CLEAR_WITH_ZERO       (BIT6)

//
// Flags that might be required tomorrow
// #define QNC_SMM_CLEAR_WITH_ONE 2 // may need to support bits that clear by writing 0
// #define QNC_SMM_MULTIBIT_FIELD 3 // may need to support status/enable fields 2 bits wide
//
typedef struct {
  QNC_SMM_SOURCE_FLAGS  Flags;
  QNC_SMM_BIT_DESC      En[NUM_EN_BITS];
  QNC_SMM_BIT_DESC      Sts[NUM_STS_BITS];
} QNC_SMM_SOURCE_DESC;
//
// 31 bytes, I think
//
#define NULL_SOURCE_DESC_INITIALIZER \
  { \
    QNC_SMM_NO_FLAGS, \
    { \
      NULL_BIT_DESC_INITIALIZER, NULL_BIT_DESC_INITIALIZER \
    }, \
    { \
      NULL_BIT_DESC_INITIALIZER \
    } \
  }

//
// /////////////////////////////////////////////////////////////////////////////
// CHILD CONTEXTS
// To keep consistent w/ the architecture, we'll need to provide the context
// to the child when we call its callback function.  After talking with Will,
// we agreed that we'll need functions to "dig" the context out of the hardware
// in many cases (Sx, Trap, Gpi, etc), and we'll need a function to compare those
// contexts to prevent unnecessary dispatches.  I'd like a general type for these
// "GetContext" functions, so I'll need a union of all the protocol contexts for
// our internal use:
//
typedef union {
  //
  // (in no particular order)
  //
  EFI_SMM_ICHN_REGISTER_CONTEXT           QNCn;
  EFI_SMM_SX_REGISTER_CONTEXT             Sx;
  EFI_SMM_PERIODIC_TIMER_REGISTER_CONTEXT PeriodicTimer;
  EFI_SMM_SW_REGISTER_CONTEXT             Sw;
  EFI_SMM_POWER_BUTTON_REGISTER_CONTEXT   PowerButton;
  // EFI_SMM_USB_REGISTER_CONTEXT            Usb; DELETE:on QuarkNcSocId, there is no usb smi supported
  EFI_SMM_GPI_REGISTER_CONTEXT            Gpi;
} QNC_SMM_CONTEXT;

typedef union {
  //
  // (in no particular order)
  //
  EFI_SMM_SW_CONTEXT                      Sw;
  EFI_SMM_PERIODIC_TIMER_CONTEXT          PeriodicTimer;
} QNC_SMM_BUFFER;

//
// Assumption: PeriodicTimer largest at 3x64-bits or 24 bytes
//
typedef struct _DATABASE_RECORD DATABASE_RECORD;

typedef
VOID
(EFIAPI *GET_CONTEXT) (
  IN  DATABASE_RECORD    * Record,
  OUT QNC_SMM_CONTEXT    * Context
  );
//
// Assumption: the GET_CONTEXT function will be as small and simple as possible.
// Assumption: We don't need to pass in an enumeration for the protocol because each
//    GET_CONTEXT function is written for only one protocol.
// We also need a function to compare contexts to see if the child should be dispatched
//
typedef
BOOLEAN
(EFIAPI *CMP_CONTEXT) (
  IN QNC_SMM_CONTEXT     * Context1,
  IN QNC_SMM_CONTEXT     * Context2
  );

/*
    Returns: True when contexts are equivalent; False otherwise
*/

//
// This function is used to get the content of CommBuffer that will be passed
// to Callback function
//
typedef
VOID
(EFIAPI *GET_BUFFER) (
  IN  DATABASE_RECORD     * Record
  );

//
// Finally, every protocol will require a "Get Context", "Compare Context"
// and "Get CommBuffer" call, so we may as well wrap that up in a table, too.
//
typedef struct {
  GET_CONTEXT GetContext;
  CMP_CONTEXT CmpContext;
  GET_BUFFER  GetBuffer;
} CONTEXT_FUNCTIONS;

extern CONTEXT_FUNCTIONS          ContextFunctions[NUM_PROTOCOLS];

//
// /////////////////////////////////////////////////////////////////////////////
// MAPPING CONTEXT TO BIT DESCRIPTIONS
// I'd like to have a general approach to mapping contexts to bit descriptions.
// Sometimes, we'll find that we can use table lookups or CONSTant assignments;
// other times, we'll find that we'll need to use a function to perform the mapping.
// If we define a macro to mask that process, we'll never have to change the code.
// I don't know if this is desirable or not -- if it isn't, then we can get rid
// of the macros and just use function calls or variable assignments.  Doesn't matter
// to me.
// Mapping complex contexts requires a function
//
// DELETE:on QuarkNcSocId, there is no usb smi supported
//EFI_STATUS
//EFIAPI
//MapUsbToSrcDesc (
//  IN  QNC_SMM_CONTEXT                                          *RegisterContext,
//  OUT QNC_SMM_SOURCE_DESC                                      *SrcDesc
//  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  RegisterContext - GC_TODO: add argument description
  SrcDesc         - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
MapPeriodicTimerToSrcDesc (
  IN  QNC_SMM_CONTEXT                                          *RegisterContext,
  OUT QNC_SMM_SOURCE_DESC                                     *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  RegisterContext - GC_TODO: add argument description
  SrcDesc         - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

//
// Mapping simple contexts can be done by assignment or lookup table
//
extern CONST QNC_SMM_SOURCE_DESC  SW_SOURCE_DESC;
extern CONST QNC_SMM_SOURCE_DESC  SX_SOURCE_DESC;

//
// With the changes we've made to the protocols, we can now use table
// lookups for the following protocols:
//
extern CONST QNC_SMM_SOURCE_DESC  GPI_SOURCE_DESC;

extern QNC_SMM_SOURCE_DESC        QNCN_SOURCE_DESCS[NUM_ICHN_TYPES];


//
// For QNCx, APMC is UINT8 port, so the MAX SWI Value is 0xFF.
//
#define MAXIMUM_SWI_VALUE   0xFF


//
// Open: Need to make sure this kind of type cast will actually work.
//   May need an intermediate form w/ two VOID* arguments.  I'll figure
//   that out when I start compiling.

///////////////////////////////////////////////////////////////////////////////
//
typedef
VOID
(EFIAPI *QNC_SMM_CLEAR_SOURCE) (
  QNC_SMM_SOURCE_DESC * SrcDesc
  );

//
// /////////////////////////////////////////////////////////////////////////////
// "DATABASE" RECORD
// Linked list data structures
//
#define DATABASE_RECORD_SIGNATURE SIGNATURE_32 ('D', 'B', 'R', 'C')

struct _DATABASE_RECORD {
  UINT32                Signature;
  LIST_ENTRY            Link;

  BOOLEAN               Processed;

  //
  // Status and Enable bit description
  //
  QNC_SMM_SOURCE_DESC   SrcDesc;

  //
  // Callback function
  //
  EFI_SMM_HANDLER_ENTRY_POINT2      Callback;
  QNC_SMM_CONTEXT                   ChildContext;
  VOID                              *CallbackContext;
  QNC_SMM_BUFFER                    CommBuffer;
  UINTN                             BufferSize;

  //
  // Special handling hooks -- init them to NULL if unused/unneeded
  //
  QNC_SMM_CLEAR_SOURCE  ClearSource;  // needed for SWSMI timer
  // Functions required to make callback code general
  //
  CONTEXT_FUNCTIONS     ContextFunctions;

  //
  // The protocol that this record dispatches
  //
  QNC_SMM_PROTOCOL_TYPE ProtocolType;

};

#define DATABASE_RECORD_FROM_LINK(_record)  CR (_record, DATABASE_RECORD, Link, DATABASE_RECORD_SIGNATURE)
#define DATABASE_RECORD_FROM_CONTEXT(_record)  CR (_record, DATABASE_RECORD, ChildContext, DATABASE_RECORD_SIGNATURE)

//
// /////////////////////////////////////////////////////////////////////////////
// HOOKING INTO THE ARCHITECTURE
//
typedef
EFI_STATUS
(EFIAPI *QNC_SMM_GENERIC_REGISTER) (
  IN  VOID                                    **This,
  IN  VOID                                    *DispatchFunction,
  IN  VOID                                    *RegisterContext,
  OUT EFI_HANDLE                              * DispatchHandle
  );
typedef
EFI_STATUS
(EFIAPI *QNC_SMM_GENERIC_UNREGISTER) (
  IN  VOID                                    **This,
  IN  EFI_HANDLE                              DispatchHandle
  );

//
// Define a memory "stamp" equivalent in size and function to most of the protocols
//
typedef struct {
  QNC_SMM_GENERIC_REGISTER    Register;
  QNC_SMM_GENERIC_UNREGISTER  Unregister;
  UINTN                       Extra1;
  UINTN                       Extra2; // may not need this one
} QNC_SMM_GENERIC_PROTOCOL;

EFI_STATUS
QNCSmmCoreRegister (
  IN  QNC_SMM_GENERIC_PROTOCOL                          *This,
  IN  EFI_SMM_HANDLER_ENTRY_POINT2                      DispatchFunction,
  IN  QNC_SMM_CONTEXT                                    *RegisterContext,
  OUT EFI_HANDLE                                        *DispatchHandle
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This              - GC_TODO: add argument description
  DispatchFunction  - GC_TODO: add argument description
  RegisterContext   - GC_TODO: add argument description
  DispatchHandle    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;
EFI_STATUS
QNCSmmCoreUnRegister (
  IN  QNC_SMM_GENERIC_PROTOCOL                         *This,
  IN EFI_HANDLE                                        DispatchHandle
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This            - GC_TODO: add argument description
  DispatchHandle  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

typedef union {
  QNC_SMM_GENERIC_PROTOCOL                  Generic;

  // EFI_SMM_USB_DISPATCH2_PROTOCOL             Usb;  DELETE:on QuarkNcSocId, there is no usb smi supported
  EFI_SMM_SX_DISPATCH2_PROTOCOL              Sx;
  EFI_SMM_SW_DISPATCH2_PROTOCOL              Sw;
  EFI_SMM_GPI_DISPATCH2_PROTOCOL             Gpi;
  EFI_SMM_ICHN_DISPATCH2_PROTOCOL            QNCn;
  EFI_SMM_POWER_BUTTON_DISPATCH2_PROTOCOL    PowerButton;
  EFI_SMM_PERIODIC_TIMER_DISPATCH2_PROTOCOL  PeriodicTimer;
} QNC_SMM_PROTOCOL;

//
// Define a structure to help us identify the generic protocol
//
#define PROTOCOL_SIGNATURE  SIGNATURE_32 ('P', 'R', 'O', 'T')

typedef struct {
  UINTN                 Signature;

  QNC_SMM_PROTOCOL_TYPE Type;
  EFI_GUID              *Guid;
  QNC_SMM_PROTOCOL      Protocols;
} QNC_SMM_QUALIFIED_PROTOCOL;

#define QUALIFIED_PROTOCOL_FROM_GENERIC(_generic) \
  CR (_generic, \
      QNC_SMM_QUALIFIED_PROTOCOL, \
      Protocols, \
      PROTOCOL_SIGNATURE \
      )

//
// Create private data for the protocols that we'll publish
//
typedef struct {
  LIST_ENTRY                  CallbackDataBase;
  EFI_HANDLE                  SmiHandle;
  EFI_HANDLE                  InstallMultProtHandle;
  QNC_SMM_QUALIFIED_PROTOCOL  Protocols[NUM_PROTOCOLS];
} PRIVATE_DATA;

extern PRIVATE_DATA           mPrivateData;

//
// /////////////////////////////////////////////////////////////////////////////
//
VOID
EFIAPI
SwGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT QNC_SMM_CONTEXT    *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Record  - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
EFIAPI
SwCmpContext (
  IN QNC_SMM_CONTEXT     *Context1,
  IN QNC_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context1  - GC_TODO: add argument description
  Context2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
SwGetBuffer (
  IN  DATABASE_RECORD     * Record
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Record  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
EFIAPI
SxGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT QNC_SMM_CONTEXT    *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Record  - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
EFIAPI
SxCmpContext (
  IN QNC_SMM_CONTEXT     *Context1,
  IN QNC_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context1  - GC_TODO: add argument description
  Context2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
EFIAPI
PeriodicTimerGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT QNC_SMM_CONTEXT    *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Record  - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
EFIAPI
PeriodicTimerCmpContext (
  IN QNC_SMM_CONTEXT     *Context1,
  IN QNC_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context1  - GC_TODO: add argument description
  Context2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
PeriodicTimerGetBuffer (
  IN  DATABASE_RECORD     * Record
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Record  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
EFIAPI
PowerButtonGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT QNC_SMM_CONTEXT     *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Record  - GC_TODO: add argument description
  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
EFIAPI
PowerButtonCmpContext (
  IN QNC_SMM_CONTEXT     *Context1,
  IN QNC_SMM_CONTEXT     *Context2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context1  - GC_TODO: add argument description
  Context2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

//
// /////////////////////////////////////////////////////////////////////////////
//
VOID
EFIAPI
QNCSmmPeriodicTimerClearSource (
  QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
QNCSmmPeriodicTimerDispatchGetNextShorterInterval (
  IN CONST EFI_SMM_PERIODIC_TIMER_DISPATCH2_PROTOCOL    *This,
  IN OUT UINT64                                         **SmiTickInterval
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This            - GC_TODO: add argument description
  SmiTickInterval - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmSxGoToSleep (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

VOID
EFIAPI
QNCSmmQNCnClearSource (
  QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
