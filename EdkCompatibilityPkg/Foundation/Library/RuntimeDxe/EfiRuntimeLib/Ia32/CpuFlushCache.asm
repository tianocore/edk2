;/*++
;
;Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
;This program and the accompanying materials                          
;are licensed and made available under the terms and conditions of the BSD License         
;which accompanies this distribution.  The full text of the license may be found at        
;http://opensource.org/licenses/bsd-license.php                                            
                                                                                          ;
;THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;
;Module Name:
;
    ;CpuFlushCache.c
;
;Abstract:
;
 ;Cpu Flush Cache Function.
;
;--*/
;---------------------------------------------------------------------------
    .586p
    .model  flat,C
    .code

;---------------------------------------------------------------------------
;
;//
;// Cache Flush Routine.
;//
;EFI_STATUS
;EfiCpuFlushCache (
  ;IN EFI_PHYSICAL_ADDRESS          Start,
;  IN UINT64                        Length
  ;)
;/*++
;
;Routine Description:
;
  ;Flush cache with specified range.
;
;Arguments:
;
  ;Start   - Start address
;  Length  - Length in bytes
;
;Returns:
;
  ;Status code
;  
  ;EFI_SUCCESS - success
;
;--*/
EfiCpuFlushCache	PROC
    wbinvd
	xor			eax, eax
	ret
EfiCpuFlushCache	ENDP
	
	END
	
	