/** @file
  Universal Payload general definitions.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_H_
#define UNIVERSAL_PAYLOAD_H_

#pragma pack(1)

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
