/** @file
  This file declares the Read-only Variable Service PPI, which is required by the framework spec.

  These services provide a lightweight, read-only variant of the full EFI variable services. The
  reason that these services are read-only is to reduce the complexity of flash management. Also,
  some implementation of the PEI may use the same physical flash part for variable and PEIM
  storage. As such, a write command to certain technologies would alter the contents of the entire part,
  making the PEIM execution in the original position not follow the required flow.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This PPI is defined in PEI CIS
  Version 0.91.
**/

#ifndef __PEI_READ_ONLY_VARIABLE_PPI_H__
#define __PEI_READ_ONLY_VARIABLE_PPI_H__

#define EFI_PEI_READ_ONLY_VARIABLE_ACCESS_PPI_GUID \
  { \
    0x3cdc90c6, 0x13fb, 0x4a75, {0x9e, 0x79, 0x59, 0xe9, 0xdd, 0x78, 0xb9, 0xfa } \
  }

typedef struct _EFI_PEI_READ_ONLY_VARIABLE_PPI  EFI_PEI_READ_ONLY_VARIABLE_PPI;

///
/// Variable attributes.
///@{
#define EFI_VARIABLE_NON_VOLATILE       0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x00000004
///
/// Inconsistent with specification here: 
/// In Framework Spec, PeiCis0.91, neither the macro or its value is defined.
/// Keeping this inconsistancy for backward compatibility.
/// 
#define EFI_VARIABLE_READ_ONLY          0x00000008
///@}

/**
  Get Variable value by Name and GUID pair.

  @param[in]  PeiServices    An indirect pointer to the PEI Services Table published 
                             by the PEI Foundation.
  @param[in]  VariableName   A NULL-terminated Unicode string that is the name of the vendor's variable.
  @param[in]  VendorGuid     A unique identifier for the vendor.
  @param[out] Attributes     This OPTIONAL parameter may be either NULL or
                             a pointer to the location in which to return
                             the attributes bitmask for the variable.
  @param[in,out]  DataSize   On input, the size in bytes of the return Data buffer.
                             On output, the size of data returned in Data.
  @param[out] Data           The buffer to return the contents of the variable.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved due to a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_VARIABLE)(
  IN     EFI_PEI_SERVICES   **PeiServices,
  IN     CHAR16             *VariableName,
  IN     EFI_GUID           *VendorGuid,
     OUT UINT32             *Attributes OPTIONAL,
  IN OUT UINTN              *DataSize,
     OUT VOID               *Data
  );

/**
  This function can be called multiple times to retrieve the VariableName
  and VendorGuid of all variables currently available in the system. On each call
  to GetNextVariableName(), the previous results are passed into the interface,
  and on output the interface returns the next variable name data.  When the
  entire variable list has been returned, the error EFI_NOT_FOUND is returned.

  @param[in]  PeiServices       An indirect pointer to the PEI Services Table 
                                published by the PEI Foundation.
  @param[in]  VariableNameSize  The size of the VariableName buffer.
  @param[in]  VariableName      On input, supplies the last VariableName that was
                                returned by GetNextVariableName(). On output, 
                                returns the Null-terminated Unicode string of the 
                                current variable.
  @param[in]  VendorGuid        On input, supplies the last VendorGuid that was
                                returned by GetNextVariableName(). On output, 
                                returns the VendorGuid of the current variable.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the result.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_DEVICE_ERROR      The variable name could not be retrieved due to 
                                a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_NEXT_VARIABLE_NAME)(
  IN EFI_PEI_SERVICES             **PeiServices,
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  );

///
/// This PPI provides a lightweight, read-only variant of the full EFI
/// variable services.  
///
struct _EFI_PEI_READ_ONLY_VARIABLE_PPI {
	///
  /// Inconsistent with specification here: 
  /// In Framework Spec, PeiCis0.91, the field is named as GetVariable and GetNextVariableName.
  /// Keeping this inconsistancy for backward compatibility.
	/// 
  EFI_PEI_GET_VARIABLE            PeiGetVariable;         ///< A service to ascertain a given variable name.
  EFI_PEI_GET_NEXT_VARIABLE_NAME  PeiGetNextVariableName; ///< A service to ascertain a variable based upon a given, known variable
};

extern EFI_GUID gEfiPeiReadOnlyVariablePpiGuid;

#endif	/* __PEI_READ_ONLY_VARIABLE_PPI_H__ */

