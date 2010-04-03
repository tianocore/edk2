;%HEADER%
;/** @file
;  Macros to work around lack of Apple support for LDR register, =expr
;
;  Copyright (c) 2009, Apple, Inc.  All rights reserved.
;
;**/


  MACRO 
  MmioWrite32Macro $Address, $Data 
  ldr  r1, = ($Address)                
  ldr  r0, = ($Data)                 
  str  r0, [r1]  
  MEND
    
  MACRO 
  MmioOr32Macro $Address, $OrData 
  ldr  r1, =($Address)               
  ldr  r2, =($OrData)                
  ldr  r0, [r1]                      
  orr  r0, r0, r2                    
  str  r0, [r1] 
  MEND

  MACRO 
  MmioAnd32Macro $Address, $AndData 
  ldr  r1, =($Address)                 
  ldr  r2, =($AndData)                 
  ldr  r0, [r1]                        
  and  r0, r0, r2                      
  str  r0, [r1] 
  MEND

  MACRO 
  MmioAndThenOr32Macro $Address, $AndData, $OrData 
  ldr  r1, =($Address)                         
  ldr  r0, [r1]                                        
  ldr  r2, =($AndData)                                 
  and  r0, r0, r2                                      
  ldr  r2, =($OrData)                                  
  orr  r0, r0, r2                                      
  str  r0, [r1]          
  MEND

  MACRO 
  MmioWriteFromReg32Macro $Address, $Reg 
  ldr  r1, =($Address)   
  str  $Reg, [r1]        
  MEND

  MACRO 
  MmioRead32Macro $Address   
  ldr  r1, =($Address)         
  ldr  r0, [r1]   
  MEND

  MACRO 
  MmioReadToReg32Macro $Address, $Reg 
  ldr  r1, =($Address)                   
  ldr  $Reg, [r1]        
  MEND

  MACRO 
  LoadConstantMacro $Data 
  ldr  r0, =($Data) 
  MEND

  MACRO 
  LoadConstantToRegMacro $Data, $Reg 
  ldr  $Reg, =($Data) 
  MEND 

  END
