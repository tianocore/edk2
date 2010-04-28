#/*++
#
# Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
#--*/
#
# EDK Common Library for PEI usage (32bit)
#

#
# Libraries common to PEI and DXE
#
$(EDK_PREFIX)Foundation\Efi\Guid\EfiGuidLib.inf
$(EDK_PREFIX)Foundation\Framework\Guid\EdkFrameworkGuidLib.inf
$(EDK_PREFIX)Foundation\Guid\EdkGuidLib.inf
$(EDK_PREFIX)Foundation\Library\EfiCommonLib\EfiCommonLib.inf
$(EDK_PREFIX)Foundation\Cpu\Pentium\CpuIA32Lib\CpuIA32Lib.inf
$(EDK_PREFIX)Foundation\Cpu\Itanium\CpuIA64Lib\CpuIA64Lib.inf
$(EDK_PREFIX)Foundation\Library\CustomizedDecompress\CustomizedDecompress.inf
$(EDK_PREFIX)Foundation\Library\CompilerStub\CompilerStubLib.inf

#
# PEI libraries
#
$(EDK_PREFIX)Foundation\Framework\Ppi\EdkFrameworkPpiLib.inf
$(EDK_PREFIX)Foundation\Ppi\EdkPpiLib.inf
$(EDK_PREFIX)Foundation\Library\Pei\PeiLib\PeiLib.inf
$(EDK_PREFIX)Foundation\Library\Pei\Hob\PeiHobLib.inf
