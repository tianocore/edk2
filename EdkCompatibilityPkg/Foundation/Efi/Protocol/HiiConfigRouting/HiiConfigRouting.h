/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  HiiConfigRouting.h

Abstract:

  EFI_HII_CONFIG_ROUTING_PROTOCOL as defined in UEFI 2.1 spec.

--*/

#ifndef __EFI_HII_CONFIG_ROUTING_H__
#define __EFI_HII_CONFIG_ROUTING_H__

#include "EfiHii.h"

#define EFI_HII_CONFIG_ROUTING_PROTOCOL_GUID \
  { \
    0x587e72d7, 0xcc50, 0x4f79, {0x82, 0x09, 0xca, 0x29, 0x1f, 0xc1, 0xa1, 0x0f} \
  }

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_HII_CONFIG_ROUTING_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_HII_EXTRACT_CONFIG) (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
/*++

  Routine Description:
    This function allows a caller to extract the current configuration 
    for one or more named elements from one or more drivers.
    
  Arguments:          
    This                   - A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL instance.
    Request                - A null-terminated Unicode string in <MultiConfigRequest> format.
    Progress               - On return, points to a character in the Request string. Points to the string's null    
                             terminator if request was successful. Points to the most recent '&' before the first  
                             failing name / value pair (or the beginning of the string if the failure is in the first
                             name / value pair) if the request was not successful.
    Results                - Null-terminated Unicode string in <MultiConfigAltResp> format which has all               
                             values filled in for the names in the Request string. String to be allocated by the called
                             function.                                                                                                         
                        
  Returns:              
    EFI_SUCCESS            - The Results string is filled with the values
                             corresponding to all requested names.       
    EFI_OUT_OF_RESOURCES   - Not enough memory to store the parts of the results
                             that must be stored awaiting possible future       
                             protocols.                                                                      
    EFI_NOT_FOUND          - Routing data doesn't match any known driver.        
                             Progress set to the 'G' in "GUID" of the routing 
                             header that doesn't match. Note: There is no        
                             requirement that all routing data be validated before
                             any configuration extraction.                            
    EFI_INVALID_PARAMETER  - For example, passing in a NULL for the Request   
                             parameter would result in this type of error. The
                             Progress parameter is set to NULL.               
                             
    EFI_INVALID_PARAMETER  - Illegal syntax. Progress set to most recent & before
                             the error or the beginning of the string.               
        
    EFI_INVALID_PARAMETER  - Unknown name. Progress points to the & before
                             the name in question.                        
                             
--*/    
;
  
typedef
EFI_STATUS 
(EFIAPI *EFI_HII_EXPORT_CONFIG) (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  OUT EFI_STRING                             *Results
  )
/*++

  Routine Description:
    This function allows the caller to request the current configuration for the 
    entirety of the current HII database and returns the data in a null-terminated Unicode string.        
    
  Arguments:          
    This                   - A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL instance.    
    Results                - Null-terminated Unicode string in <MultiConfigAltResp> format which has all               
                             values filled in for the names in the Request string. String to be allocated by the called
                             function. De-allocation is up to the caller.                                                        
                        
  Returns:              
    EFI_SUCCESS            - The Results string is filled with the values
                             corresponding to all requested names.       
    EFI_OUT_OF_RESOURCES   - Not enough memory to store the parts of the results
                             that must be stored awaiting possible future       
                             protocols.                                                                      
    EFI_INVALID_PARAMETER  - For example, passing in a NULL for the Results
                             parameter would result in this type of error.    
                             
--*/    
;  

typedef
EFI_STATUS
(EFIAPI *EFI_HII_ROUTE_CONFIG) (
  IN  EFI_HII_CONFIG_ROUTING_PROTOCOL        *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
/*++

  Routine Description:
    This function processes the results of processing forms and routes it to the 
    appropriate handlers or storage.
    
  Arguments:          
    This                   - A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL instance.    
    Configuration          - A null-terminated Unicode string in <MulltiConfigResp> format.
    Progress               - A pointer to a string filled in with the offset of the most recent "&" before the first
                             failing name / value pair (or the beginning of the string if the failure is in the first 
                             name / value pair) or the terminating NULL if all was successful.                                                                                  
                        
  Returns:              
    EFI_SUCCESS            - The results have been distributed or are awaiting
                             distribution.                                    
    EFI_OUT_OF_RESOURCES   - Not enough memory to store the parts of the results
                             that must be stored awaiting possible future       
                             protocols.                                                                      
    EFI_INVALID_PARAMETER  - Passing in a NULL for the Configuration
                             parameter would result in this type of error.    
    EFI_NOT_FOUND          - Target for the specified routing data was not found.        
                             
--*/    
;  
  

typedef
EFI_STATUS 
(EFIAPI *EFI_HII_BLOCK_TO_CONFIG) (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       ConfigRequest,
  IN  CONST UINT8                            *Block,
  IN  CONST UINTN                            BlockSize,
  OUT EFI_STRING                             *Config,
  OUT EFI_STRING                             *Progress
  )
/*++

  Routine Description:
    This helper function is to be called by drivers to map configuration data stored
    in byte array ("block") formats such as UEFI Variables into current configuration strings.
    
  Arguments:          
    This                   - A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL instance.    
    ConfigRequest          - A null-terminated Unicode string in <ConfigRequest> format.
    Block                  - Array of bytes defining the block's configuration.
    BlockSize              - Length in bytes of Block.
    Config                 - Filled-in configuration string. String allocated by the function. 
                              Returned only if call is successful.                                                                               
    Progress               - A pointer to a string filled in with the offset of the most recent "&" before the first
                             failing name / value pair (or the beginning of the string if the failure is in the first 
                             name / value pair) or the terminating NULL if all was successful.                             
                        
  Returns:              
    EFI_SUCCESS            - The request succeeded. Progress points to the null
                             terminator at the end of the ConfigRequest        
                             string.                                                                        
    EFI_OUT_OF_RESOURCES   - Not enough memory to allocate Config.    
                             Progress points to the first character of
                             ConfigRequest.                           
    EFI_INVALID_PARAMETER  - Passing in a NULL for the ConfigRequest or      
                             Block parameter would result in this type of    
                             error. Progress points to the first character of
                             ConfigRequest.                                   
    EFI_NOT_FOUND          - Target for the specified routing data was not found.
                             Progress points to the "G" in "GUID" of the     
                             errant routing data.                                
    EFI_DEVICE_ERROR       - Block not large enough. Progress undefined.
    EFI_INVALID_PARAMETER  - Encountered non <BlockName> formatted string.         
                             Block is left updated and Progress points at the "&"
                             preceding the first non-<BlockName>.                  
                                                          
--*/      
;

typedef
EFI_STATUS 
(EFIAPI *EFI_HII_CONFIG_TO_BLOCK) (
  IN     CONST EFI_HII_CONFIG_ROUTING_PROTOCOL *This,
  IN     CONST EFI_STRING                      ConfigResp,
  IN OUT UINT8                                 *Block,
  IN OUT UINTN                                 *BlockSize,
  OUT    EFI_STRING                            *Progress
  )
/*++

  Routine Description:
    This helper function is to be called by drivers to map configuration strings 
    to configurations stored in byte array ("block") formats such as UEFI Variables.
    
  Arguments:          
    This                   - A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL instance.    
    ConfigResp             - A null-terminated Unicode string in <ConfigResp> format.
    Block                  - A possibly null array of bytes representing the current block.  
                             Only bytes referenced in the ConfigResp string in the block are modified. 
                             If this parameter is null or if the *BlockSize parameter is (on input) 
                             shorter than required by the Configuration string, only the BlockSize 
                             parameter is updated and an appropriate status (see below) is returned.            
                             
    BlockSize              - The length of the Block in units of UINT8. On input, this is the size of the Block.
                             On output, if successful, contains the index of the last modified byte in the Block.
                             
    Progress               - On return, points to an element of the ConfigResp string filled in with the offset of      
                             the most recent "&" before the first failing name / value pair (or the beginning of the  
                             string if the failure is in the first name / value pair) or the terminating NULL if all was
                             successful.                                                                                
  Returns:              
    EFI_SUCCESS            - The request succeeded. Progress points to the null
                             terminator at the end of the ConfigResp
                             string.                                           
    EFI_OUT_OF_RESOURCES   - Not enough memory to allocate Config.    
                             Progress points to the first character of
                             ConfigResp.                           
    EFI_INVALID_PARAMETER  - Passing in a NULL for the ConfigResp or         
                             Block parameter would result in this type of error.
                             Progress points to the first character of          
                             ConfigResp.                                                                  
    EFI_NOT_FOUND          - Target for the specified routing data was not found.
                             Progress points to the "G" in "GUID" of the     
                             errant routing data.                                
    EFI_INVALID_PARAMETER  - Encountered non <BlockName> formatted name /    
                             value pair. Block is left updated and           
                             Progress points at the "&" preceding the first
                             non-<BlockName>.                                
                             
--*/                         
;

typedef
EFI_STATUS 
(EFIAPI * EFI_HII_GET_ALT_CFG) (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL    *This, 
  IN  CONST EFI_STRING                         Configuration, 
  IN  CONST EFI_GUID                           *Guid, 
  IN  CONST EFI_STRING                         Name, 
  IN  CONST EFI_DEVICE_PATH_PROTOCOL           *DevicePath,  
  IN  CONST UINT16                             *AltCfgId,
  OUT EFI_STRING                               *AltCfgResp 
  )
/*++

  Routine Description:
    This helper function is to be called by drivers to extract portions of 
    a larger configuration string.
    
  Arguments:          
    This                   - A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL instance.    
    Configuration          - A null-terminated Unicode string in <MultiConfigAltResp> format.
    Guid                   - A pointer to the GUID value to search for in the 
                             routing portion of the ConfigResp string when retrieving 
                             the requested data. If Guid is NULL, then all GUID 
                             values will be searched for.
    Name                   - A pointer to the NAME value to search for in the 
                             routing portion of the ConfigResp string when retrieving 
                             the requested data. If Name is NULL, then all Name 
                             values will be searched for.                         
    DevicePath             - A pointer to the PATH value to search for in the 
                             routing portion of the ConfigResp string when retrieving 
                             the requested data. If DevicePath is NULL, then all 
                             DevicePath values will be searched for.             
    AltCfgId               - A pointer to the ALTCFG value to search for in the 
                             routing portion of the ConfigResp string when retrieving 
                             the requested data.  If this parameter is NULL, 
                             then the current setting will be retrieved.
    AltCfgResp             - A pointer to a buffer which will be allocated by the 
                             function which contains the retrieved string as requested.  
                             This buffer is only allocated if the call was successful. 
    
  Returns:              
    EFI_SUCCESS            - The request succeeded. The requested data was extracted 
                             and placed in the newly allocated AltCfgResp buffer.
    EFI_OUT_OF_RESOURCES   - Not enough memory to allocate AltCfgResp.    
    EFI_INVALID_PARAMETER  - Any parameter is invalid.
    EFI_NOT_FOUND          - Target for the specified routing data was not found.
                             
--*/        
;


struct _EFI_HII_CONFIG_ROUTING_PROTOCOL {
  EFI_HII_EXTRACT_CONFIG    ExtractConfig;
  EFI_HII_EXPORT_CONFIG     ExportConfig;
  EFI_HII_ROUTE_CONFIG      RouteConfig;
  EFI_HII_BLOCK_TO_CONFIG   BlockToConfig;
  EFI_HII_CONFIG_TO_BLOCK   ConfigToBlock;
  EFI_HII_GET_ALT_CFG       GetAltConfig;
};

extern EFI_GUID gEfiHiiConfigRoutingProtocolGuid;

#endif
