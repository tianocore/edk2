/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FWVolume.h

Abstract:

  Include file for the module that keeps track of files for the firmware
  volumes.

--*/

#ifndef _FW_VOLUME_H_
#define _FW_VOLUME_H_

//
// class CFirmwareVolume
// {
// public:
//
void
CFVConstructor (
  VOID
  );
void
CFVDestructor (
  VOID
  );

int
CFVAddFVFile (
  char  *Name,
  char  *ComponentType,
  char  *FVs,
  int   ComponentsInstance,
  char  *FFSExt,
  char  *Processor,
  char  *Apriori,
  char  *BaseName,
  char  *Guid
  );

int
CFVSetXRefFileName (
  char    *FileName
  );

int
CFVWriteInfFiles (
  DSC_FILE  *DSC,
  FILE      *MakeFptr
  );

int
NonFFSFVWriteInfFiles (
  DSC_FILE  *DSC,
  char      *FileName
  );

#endif // ifndef _FW_VOLUME_H_
