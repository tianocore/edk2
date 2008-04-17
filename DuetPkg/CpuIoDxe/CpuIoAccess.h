/*++
# 
# Copyright (c) 2004, Intel Corporation                                                         
# All rights reserved. This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

  CpuIoAccess.h

Abstract:

--*/

#ifndef _CPU_IO_ACCESS_H
#define _CPU_IO_ACCESS_H


#define IA32API __cdecl

UINT8
IA32API
CpuIoRead8 (
  IN  UINT16  Port
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Port  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/*++                                                                                                                               
Routine Description:                                                
  Cpu I/O read port
Arguments:                
   Port: - Port number to read                                                          
Returns:                                                            
   Return read 8 bit value                                                
--*/
UINT16
IA32API
CpuIoRead16 (
  IN  UINT16  Port
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Port  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/*++                                                                                                                               
Routine Description:                                                
  Cpu I/O read port
Arguments:                
   Port: - Port number to read                                                          
Returns:                                                            
   Return read 16 bit value                                                
--*/
UINT32
IA32API
CpuIoRead32 (
  IN  UINT16  Port
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Port  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/*++                                                                                                                               
Routine Description:                                                
  Cpu I/O read port
Arguments:                
   Port: - Port number to read                                                          
Returns:                                                            
   Return read 32 bit value                                                
--*/
VOID
IA32API
CpuIoWrite8 (
  IN  UINT16  Port,
  IN  UINT32  Data
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Port  - GC_TODO: add argument description
  Data  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/*++                                                                                                                               
Routine Description:                                                
  Cpu I/O write 8 bit data to port
Arguments:                
   Port: - Port number to read  
   Data: - Data to write to the Port                                                        
Returns:                                                            
   None                                                
--*/
VOID
IA32API
CpuIoWrite16 (
  IN  UINT16  Port,
  IN  UINT32  Data
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Port  - GC_TODO: add argument description
  Data  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/*++                                                                                                                               
Routine Description:                                                
  Cpu I/O write 16 bit data to port
Arguments:                
   Port: - Port number to read  
   Data: - Data to write to the Port                                                        
Returns:                                                            
   None                                                
--*/
VOID
IA32API
CpuIoWrite32 (
  IN  UINT16  Port,
  IN  UINT32  Data
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Port  - GC_TODO: add argument description
  Data  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

/*++                                                                                                                               
Routine Description:                                                
  Cpu I/O write 32 bit data to port
Arguments:                
   Port: - Port number to read  
   Data: - Data to write to the Port                                                        
Returns:                                                            
   None                                                
--*/
#endif
