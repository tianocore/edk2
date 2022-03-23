/** @file
  Hardware information parser library.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HW_INFO_PARSER_LIB_H_
#define HW_INFO_PARSER_LIB_H_

#include <ConfigurationManagerObject.h>

/** A handle to the HwInfoParser instance.
*/
typedef VOID *HW_INFO_PARSER_HANDLE;

/** Function pointer called by the parser to add information.

  Callback function that the parser can use to add new
  CmObj. This function must copy the CmObj data and not rely on
  the parser preserving the CmObj memory.
  This function is responsible of the Token allocation.

  @param  [in]  ParserHandle  A handle to the parser instance.
  @param  [in]  Context       A pointer to the caller's context provided in
                              HwInfoParserInit ().
  @param  [in]  CmObjDesc     CM_OBJ_DESCRIPTOR containing the CmObj(s) to add.
  @param  [out] Token         If provided and success, contain the token
                              generated for the CmObj.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
typedef
EFI_STATUS
(EFIAPI *HW_INFO_ADD_OBJECT)(
  IN        HW_INFO_PARSER_HANDLE   ParserHandle,
  IN        VOID                  *Context,
  IN  CONST CM_OBJ_DESCRIPTOR     *CmObjDesc,
  OUT       CM_OBJECT_TOKEN       *Token OPTIONAL
  );

/** Initialise the HwInfoParser.

  The HwInfoParser shall use the information provided by the HwDataSource
  to initialise the internal state of the parser or to index the data. This
  internal state shall be linked to the ParserHandle using an implementation
  defined mechanism.

  @param [in]   HwDataSource    Pointer to the blob containing the hardware
                                information. It can be a pointer to a Device
                                Tree, an XML file, etc. or any other data
                                structure defined by the HwInfoParser.
  @param [in]   Context         A pointer to the caller's context.
  @param [in]   HwInfoAdd       Function pointer called by the parser when
                                adding information.
  @param [out]  ParserHandle    A handle to the parser instance.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
HwInfoParserInit (
  IN    VOID                   *HwDataSource,
  IN    VOID                   *Context,
  IN    HW_INFO_ADD_OBJECT     HwInfoAdd,
  OUT   HW_INFO_PARSER_HANDLE  *ParserHandle
  );

/** Parse the data provided by the HwDataSource.

  @param [in]  ParserHandle    A handle to the parser instance.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
EFI_STATUS
EFIAPI
HwInfoParse (
  IN  HW_INFO_PARSER_HANDLE  ParserHandle
  );

/** Cleanup any internal state and resources that were allocated
    by the the HwInfoParser.

  @param [in]  ParserHandle    A handle to the parser instance.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
HwInfoParserShutdown (
  IN  HW_INFO_PARSER_HANDLE  ParserHandle
  );

#endif // HW_INFO_PARSER_LIB_H_
