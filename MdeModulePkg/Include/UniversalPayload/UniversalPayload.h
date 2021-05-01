/** @file
  Universal Payload general definitions.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_H_
#define UNIVERSAL_PAYLOAD_H_

/**
  Main entry point to Universal Payload.

  @param HobList  Pointer to the beginning of the HOB List from boot loader.
**/
typedef  VOID   (EFIAPI *UNIVERSAL_PAYLOAD_ENTRY) (VOID *HobList);

#define UNIVERSAL_PAYLOAD_IDENTIFIER                   SIGNATURE_32('U', 'P', 'L', 'D')
#define UNIVERSAL_PAYLOAD_INFO_SEC_NAME                ".upld_info"
#define UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX        ".upld."
#define UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX_LENGTH (sizeof (UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX) - 1)

#pragma pack(1)

typedef struct {
  UINT32                          Identifier;
  UINT32                          HeaderLength;
  UINT16                          SpecRevision;
  UINT8                           Reserved[2];
  UINT32                          Revision;
  UINT32                          Attribute;
  UINT32                          Capability;
  CHAR8                           ProducerId[16];
  CHAR8                           ImageId[16];
} UNIVERSAL_PAYLOAD_INFO_HEADER;

typedef struct {
  UINT8                Revision;
  UINT8                Reserved;
  UINT16               Length;
} UNIVERSAL_PAYLOAD_GENERIC_HEADER;

#pragma pack()

/**
  Returns the size of a structure of known type, up through and including a specified field.

  @param   TYPE     The name of the data structure that contains the field specified by Field.
  @param   Field    The name of the field in the data structure.

  @return  size, in bytes.

**/
#define UNIVERSAL_PAYLOAD_SIZEOF_THROUGH_FIELD(TYPE, Field) (OFFSET_OF(TYPE, Field) + sizeof (((TYPE *) 0)->Field))

#endif // UNIVERSAL_PAYLOAD_H_
