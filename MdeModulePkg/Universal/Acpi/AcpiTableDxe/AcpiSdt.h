/** @file
  ACPI Sdt Protocol Driver

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_SDT_H_
#define _ACPI_SDT_H_

//
// Privacy data structure
//

//
// ACPI Notify Linked List Signature.
//
#define EFI_ACPI_NOTIFY_LIST_SIGNATURE SIGNATURE_32 ('E', 'A', 'N', 'L')

//
// ACPI Notify List Entry definition.
//
//  Signature must be set to EFI_ACPI_NOTIFY_LIST_SIGNATURE
//  Link is the linked list data.
//  Notification is the callback function.
//
typedef struct {
  UINT32                   Signature;
  LIST_ENTRY               Link;
  EFI_ACPI_NOTIFICATION_FN Notification;
} EFI_ACPI_NOTIFY_LIST;

//
// Containment record for ACPI Notify linked list.
//
#define EFI_ACPI_NOTIFY_LIST_FROM_LINK(_link)  CR (_link, EFI_ACPI_NOTIFY_LIST, Link, EFI_ACPI_NOTIFY_LIST_SIGNATURE)

typedef struct _AML_BYTE_ENCODING AML_BYTE_ENCODING;
typedef struct _EFI_AML_NODE_LIST EFI_AML_NODE_LIST;

//
// AML Node Linked List Signature.
//
#define EFI_AML_NODE_LIST_SIGNATURE SIGNATURE_32 ('E', 'A', 'M', 'L')

//
// AML Node Linked List Entry definition.
//
//  Signature must be set to EFI_AML_NODE_LIST_SIGNATURE
//  Link is the linked list data.
//  Name is the ACPI node name.
//         This is listed for PATH finding.
//  Buffer is the ACPI node buffer pointer, the first/second bytes are opcode.
//         This buffer should not be freed.
//  Size is the total size of this ACPI node buffer.
//  Children is the children linked list of this node.
//
#define AML_NAME_SEG_SIZE  4

struct _EFI_AML_NODE_LIST {
  UINT32                  Signature;
  UINT8                   Name[AML_NAME_SEG_SIZE];
  UINT8                   *Buffer;
  UINTN                   Size;
  LIST_ENTRY              Link;
  LIST_ENTRY              Children;
  EFI_AML_NODE_LIST       *Parent;
  AML_BYTE_ENCODING       *AmlByteEncoding;
};

//
// Containment record for AML Node linked list.
//
#define EFI_AML_NODE_LIST_FROM_LINK(_link)  CR (_link, EFI_AML_NODE_LIST, Link, EFI_AML_NODE_LIST_SIGNATURE)

//
// AML Handle Signature.
//
#define EFI_AML_HANDLE_SIGNATURE SIGNATURE_32 ('E', 'A', 'H', 'S')
#define EFI_AML_ROOT_HANDLE_SIGNATURE SIGNATURE_32 ('E', 'A', 'R', 'H')

//
// AML Handle Entry definition.
//
//  Signature must be set to EFI_AML_HANDLE_SIGNATURE or EFI_AML_ROOT_HANDLE_SIGNATURE
//  Buffer is the ACPI node buffer pointer, the first/second bytes are opcode.
//         This buffer should not be freed.
//  Size is the total size of this ACPI node buffer.
//
typedef struct {
  UINT32                  Signature;
  UINT8                   *Buffer;
  UINTN                   Size;
  AML_BYTE_ENCODING       *AmlByteEncoding;
  BOOLEAN                 Modified;
} EFI_AML_HANDLE;

typedef UINT32 AML_OP_PARSE_INDEX;

#define AML_OP_PARSE_INDEX_GET_OPCODE     0
#define AML_OP_PARSE_INDEX_GET_TERM1      1
#define AML_OP_PARSE_INDEX_GET_TERM2      2
#define AML_OP_PARSE_INDEX_GET_TERM3      3
#define AML_OP_PARSE_INDEX_GET_TERM4      4
#define AML_OP_PARSE_INDEX_GET_TERM5      5
#define AML_OP_PARSE_INDEX_GET_TERM6      6
#define AML_OP_PARSE_INDEX_GET_SIZE       (AML_OP_PARSE_INDEX)-1

typedef UINT32 AML_OP_PARSE_FORMAT;
#define AML_NONE         0
#define AML_OPCODE       1
#define AML_UINT8        2
#define AML_UINT16       3
#define AML_UINT32       4
#define AML_UINT64       5
#define AML_NAME         6
#define AML_STRING       7
#define AML_OBJECT       8

typedef UINT32 AML_OP_ATTRIBUTE;
#define AML_HAS_PKG_LENGTH       0x1     // It is ACPI attribute - if OpCode has PkgLength
#define AML_IS_NAME_CHAR         0x2     // It is ACPI attribute - if this is NameChar
#define AML_HAS_CHILD_OBJ        0x4     // it is ACPI attribute - if OpCode has Child Object.
#define AML_IN_NAMESPACE         0x10000 // It is UEFI SDT attribute - if OpCode will be in NameSpace
                                         // NOTE; Not all OBJECT will be in NameSpace
                                         // For example, BankField | CreateBitField | CreateByteField | CreateDWordField |
                                         //   CreateField | CreateQWordField | CreateWordField | Field | IndexField.

struct _AML_BYTE_ENCODING {
  UINT8                      OpCode;
  UINT8                      SubOpCode;
  AML_OP_PARSE_INDEX         MaxIndex;
  AML_OP_PARSE_FORMAT        Format[6];
  AML_OP_ATTRIBUTE           Attribute;
};

//
// AcpiSdt protocol declaration
//

/**
  Returns a requested ACPI table.

  The GetAcpiTable() function returns a pointer to a buffer containing the ACPI table associated
  with the Index that was input. The following structures are not considered elements in the list of
  ACPI tables:
  - Root System Description Pointer (RSD_PTR)
  - Root System Description Table (RSDT)
  - Extended System Description Table (XSDT)
  Version is updated with a bit map containing all the versions of ACPI of which the table is a
  member. For tables installed via the EFI_ACPI_TABLE_PROTOCOL.InstallAcpiTable() interface,
  the function returns the value of EFI_ACPI_STD_PROTOCOL.AcpiVersion.

  @param[in]    Index       The zero-based index of the table to retrieve.
  @param[out]   Table       Pointer for returning the table buffer.
  @param[out]   Version     On return, updated with the ACPI versions to which this table belongs. Type
                            EFI_ACPI_TABLE_VERSION is defined in "Related Definitions" in the
                            EFI_ACPI_SDT_PROTOCOL.
  @param[out]   TableKey    On return, points to the table key for the specified ACPI system definition table.
                            This is identical to the table key used in the EFI_ACPI_TABLE_PROTOCOL.
                            The TableKey can be passed to EFI_ACPI_TABLE_PROTOCOL.UninstallAcpiTable()
                            to uninstall the table.

  @retval EFI_SUCCESS       The function completed successfully.
  @retval EFI_NOT_FOUND     The requested index is too large and a table was not found.
**/
EFI_STATUS
EFIAPI
GetAcpiTable2 (
  IN  UINTN                               Index,
  OUT EFI_ACPI_SDT_HEADER                 **Table,
  OUT EFI_ACPI_TABLE_VERSION              *Version,
  OUT UINTN                               *TableKey
  );

/**
  Register or unregister a callback when an ACPI table is installed.

  This function registers or unregisters a function which will be called whenever a new ACPI table is
  installed.

  @param[in]    Register        If TRUE, then the specified function will be registered. If FALSE, then the specified
                                function will be unregistered.
  @param[in]    Notification    Points to the callback function to be registered or unregistered.

  @retval EFI_SUCCESS           Callback successfully registered or unregistered.
  @retval EFI_INVALID_PARAMETER Notification is NULL
  @retval EFI_INVALID_PARAMETER Register is FALSE and Notification does not match a known registration function.
**/
EFI_STATUS
EFIAPI
RegisterNotify (
  IN BOOLEAN                    Register,
  IN EFI_ACPI_NOTIFICATION_FN   Notification
  );

/**
  Create a handle for the first ACPI opcode in an ACPI system description table.

  @param[in]    TableKey    The table key for the ACPI table, as returned by GetTable().
  @param[out]   Handle      On return, points to the newly created ACPI handle.

  @retval EFI_SUCCESS       Handle created successfully.
  @retval EFI_NOT_FOUND     TableKey does not refer to a valid ACPI table.
**/
EFI_STATUS
EFIAPI
OpenSdt (
  IN    UINTN           TableKey,
  OUT   EFI_ACPI_HANDLE *Handle
  );

/**
  Create a handle from an ACPI opcode

  @param[in]  Buffer                 Points to the ACPI opcode.
  @param[out] Handle                 Upon return, holds the handle.

  @retval   EFI_SUCCESS             Success
  @retval   EFI_INVALID_PARAMETER   Buffer is NULL or Handle is NULL or Buffer points to an
                                    invalid opcode.

**/
EFI_STATUS
EFIAPI
Open (
  IN    VOID            *Buffer,
  OUT   EFI_ACPI_HANDLE *Handle
  );

/**
  Close an ACPI handle.

  @param[in] Handle Returns the handle.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER Handle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
Close (
  IN EFI_ACPI_HANDLE Handle
  );

/**
  Retrieve information about an ACPI object.

  @param[in]    Handle      ACPI object handle.
  @param[in]    Index       Index of the data to retrieve from the object. In general, indexes read from left-to-right
                            in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[out]   DataType    Points to the returned data type or EFI_ACPI_DATA_TYPE_NONE if no data exists
                            for the specified index.
  @param[out]   Data        Upon return, points to the pointer to the data.
  @param[out]   DataSize    Upon return, points to the size of Data.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER Handle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
GetOption (
  IN        EFI_ACPI_HANDLE     Handle,
  IN        UINTN               Index,
  OUT       EFI_ACPI_DATA_TYPE  *DataType,
  OUT CONST VOID                **Data,
  OUT       UINTN               *DataSize
  );

/**
  Change information about an ACPI object.

  @param[in]  Handle    ACPI object handle.
  @param[in]  Index     Index of the data to retrieve from the object. In general, indexes read from left-to-right
                        in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[in]  Data      Points to the data.
  @param[in]  DataSize  The size of the Data.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER Handle is NULL or does not refer to a valid ACPI object.
  @retval EFI_BAD_BUFFER_SIZE   Data cannot be accommodated in the space occupied by
                                the option.

**/
EFI_STATUS
EFIAPI
SetOption (
  IN        EFI_ACPI_HANDLE Handle,
  IN        UINTN           Index,
  IN CONST  VOID            *Data,
  IN        UINTN           DataSize
  );

/**
  Return the child ACPI objects.

  @param[in]        ParentHandle    Parent handle.
  @param[in, out]   Handle          On entry, points to the previously returned handle or NULL to start with the first
                                    handle. On return, points to the next returned ACPI handle or NULL if there are no
                                    child objects.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     ParentHandle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
GetChild (
  IN EFI_ACPI_HANDLE        ParentHandle,
  IN OUT EFI_ACPI_HANDLE    *Handle
  );

/**
  Returns the handle of the ACPI object representing the specified ACPI path

  @param[in]    HandleIn    Points to the handle of the object representing the starting point for the path search.
  @param[in]    AcpiPath    Points to the ACPI path, which conforms to the ACPI encoded path format.
  @param[out]   HandleOut   On return, points to the ACPI object which represents AcpiPath, relative to
                            HandleIn.

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER HandleIn is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
EFIAPI
FindPath (
  IN    EFI_ACPI_HANDLE HandleIn,
  IN    VOID            *AcpiPath,
  OUT   EFI_ACPI_HANDLE *HandleOut
  );

//
// ACPI SDT function
//

/**
  Create a handle from an ACPI opcode

  @param[in]  Buffer                 Points to the ACPI opcode.
  @param[in]  BufferSize             Max buffer size.
  @param[out] Handle                 Upon return, holds the handle.

  @retval   EFI_SUCCESS             Success
  @retval   EFI_INVALID_PARAMETER   Buffer is NULL or Handle is NULL or Buffer points to an
                                    invalid opcode.

**/
EFI_STATUS
SdtOpenEx (
  IN    VOID            *Buffer,
  IN    UINTN           BufferSize,
  OUT   EFI_ACPI_HANDLE *Handle
  );

//
// AML support function
//

/**
  Get AML NameString size.

  @param[in]    Buffer     AML NameString.
  @param[out]   BufferSize AML NameString size

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER Buffer does not refer to a valid AML NameString.
**/
EFI_STATUS
AmlGetNameStringSize (
  IN  UINT8              *Buffer,
  OUT UINTN              *BufferSize
  );

/**
  This function retuns package length from the buffer.

  @param[in]  Buffer    AML buffer
  @param[out] PkgLength The total length of package.

  @return The byte data count to present the package length.
**/
UINTN
AmlGetPkgLength (
  IN UINT8              *Buffer,
  OUT UINTN             *PkgLength
  );

/**
  This function returns AcpiDataType according to AmlType.

  @param[in]  AmlType        AML Type.

  @return AcpiDataType
**/
EFI_ACPI_DATA_TYPE
AmlTypeToAcpiType (
  IN AML_OP_PARSE_FORMAT  AmlType
  );

/**
  This function returns AmlByteEncoding according to OpCode Byte.

  @param[in]  OpByteBuffer        OpCode byte buffer.

  @return AmlByteEncoding
**/
AML_BYTE_ENCODING *
AmlSearchByOpByte (
  IN UINT8                *OpByteBuffer
  );

/**
  Return object size.

  @param[in]    AmlByteEncoding      AML Byte Encoding.
  @param[in]    Buffer               AML object buffer.
  @param[in]    MaxBufferSize        AML object buffer MAX size. The parser can not parse any data exceed this region.

  @return       Size of the object.
**/
UINTN
AmlGetObjectSize (
  IN AML_BYTE_ENCODING   *AmlByteEncoding,
  IN UINT8               *Buffer,
  IN UINTN               MaxBufferSize
  );

/**
  Return object name.

  @param[in]    AmlHandle            AML handle.

  @return       Name of the object.
**/
CHAR8 *
AmlGetObjectName (
  IN EFI_AML_HANDLE      *AmlHandle
  );

/**
  Retrieve information according to AmlHandle

  @param[in]    AmlHandle            AML handle.
  @param[in]    Index                Index of the data to retrieve from the object. In general, indexes read from left-to-right
                                     in the ACPI encoding, with index 0 always being the ACPI opcode.
  @param[out]   DataType             Points to the returned data type or EFI_ACPI_DATA_TYPE_NONE if no data exists
                                     for the specified index.
  @param[out]   Data                 Upon return, points to the pointer to the data.
  @param[out]   DataSize             Upon return, points to the size of Data.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AmlHandle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlParseOptionHandleCommon (
  IN EFI_AML_HANDLE      *AmlHandle,
  IN AML_OP_PARSE_INDEX  Index,
  OUT EFI_ACPI_DATA_TYPE *DataType,
  OUT VOID               **Data,
  OUT UINTN              *DataSize
  );

/**
  Return offset of last option.

  @param[in]    AmlHandle            AML Handle.
  @param[out]   Buffer               Upon return, points to the offset after last option.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AmlHandle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlGetOffsetAfterLastOption (
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT UINT8                 **Buffer
  );

/**
  Return the child ACPI objects from Root Handle.

  @param[in]        AmlParentHandle Parent handle. It is Root Handle.
  @param[in]        AmlHandle       The previously returned handle or NULL to start with the first handle.
  @param[out]       Buffer          On return, points to the next returned ACPI handle or NULL if there are no
                                    child objects.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     ParentHandle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlGetChildFromRoot (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT VOID                  **Buffer
  );

/**
  Return the child ACPI objects from Non-Root Handle.

  @param[in]        AmlParentHandle Parent handle. It is Non-Root Handle.
  @param[in]        AmlHandle       The previously returned handle or NULL to start with the first handle.
  @param[out]       Buffer          On return, points to the next returned ACPI handle or NULL if there are no
                                    child objects.

  @retval EFI_SUCCESS               Success
  @retval EFI_INVALID_PARAMETER     ParentHandle is NULL or does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlGetChildFromNonRoot (
  IN EFI_AML_HANDLE         *AmlParentHandle,
  IN EFI_AML_HANDLE         *AmlHandle,
  OUT VOID                  **Buffer
  );

/**
  Return AML name according to ASL name.
  The caller need free the AmlName returned.

  @param[in]    AslPath     ASL name.

  @return AmlName
**/
UINT8 *
AmlNameFromAslName (
  IN UINT8 *AslPath
  );

/**
  Returns the handle of the ACPI object representing the specified ACPI AML path

  @param[in]    AmlHandle   Points to the handle of the object representing the starting point for the path search.
  @param[in]    AmlPath     Points to the ACPI AML path.
  @param[out]   Buffer      On return, points to the ACPI object which represents AcpiPath, relative to
                            HandleIn.
  @param[in]    FromRoot    TRUE means to find AML path from \ (Root) Node.
                            FALSE means to find AML path from this Node (The HandleIn).

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER HandleIn does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlFindPath (
  IN    EFI_AML_HANDLE  *AmlHandle,
  IN    UINT8           *AmlPath,
  OUT   VOID            **Buffer,
  IN    BOOLEAN         FromRoot
  );

/**
  Print AML NameString.

  @param[in] Buffer AML NameString.
**/
VOID
AmlPrintNameString (
  IN UINT8              *Buffer
  );

/**
  Print AML NameSeg.

  @param[in] Buffer AML NameSeg.
**/
VOID
AmlPrintNameSeg (
  IN UINT8              *Buffer
  );

/**
  Check if it is AML Root name

  @param[in]    Buffer AML path.

  @retval       TRUE  AML path is root.
  @retval       FALSE AML path is not root.
**/
BOOLEAN
AmlIsRootPath (
  IN UINT8              *Buffer
  );

#endif
