/** @file
  DXE Dispatcher.

  Step #1 - When a FV protocol is added to the system every driver in the FV
            is added to the mDiscoveredList. The SOR, Before, and After Depex are
            pre-processed as drivers are added to the mDiscoveredList. If an Apriori
            file exists in the FV those drivers are addeded to the
            mScheduledQueue. The mFvHandleList is used to make sure a
            FV is only processed once.

  Step #2 - Dispatch. Remove driver from the mScheduledQueue and load and
            start it. After mScheduledQueue is drained check the
            mDiscoveredList to see if any item has a Depex that is ready to
            be placed on the mScheduledQueue.

  Step #3 - Adding to the mScheduledQueue requires that you process Before
            and After dependencies. This is done recursively as the call to add
            to the mScheduledQueue checks for Before and recursively adds
            all Befores. It then addes the item that was passed in and then
            processess the After dependecies by recursively calling the routine.

  Dispatcher Rules:
  The rules for the dispatcher are in chapter 10 of the DXE CIS. Figure 10-3
  is the state diagram for the DXE dispatcher

  Depex - Dependency Expresion.
  SOR   - Schedule On Request - Don't schedule if this bit is set.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"

//
// The Driver List contains one copy of every driver that has been discovered.
// Items are never removed from the driver list. List of EFI_CORE_DRIVER_ENTRY
//
LIST_ENTRY  mDiscoveredList = INITIALIZE_LIST_HEAD_VARIABLE (mDiscoveredList);

//
// Queue of drivers that are ready to dispatch. This queue is a subset of the
// mDiscoveredList.list of EFI_CORE_DRIVER_ENTRY.
//
LIST_ENTRY  mScheduledQueue = INITIALIZE_LIST_HEAD_VARIABLE (mScheduledQueue);

//
// List of handles who's Fv's have been parsed and added to the mFwDriverList.
//
LIST_ENTRY  mFvHandleList = INITIALIZE_LIST_HEAD_VARIABLE (mFvHandleList);           // list of KNOWN_HANDLE

//
// Lock for mDiscoveredList, mScheduledQueue, gDispatcherRunning.
//
EFI_LOCK  mDispatcherLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL);


//
// Flag for the DXE Dispacher.  TRUE if dispatcher is execuing.
//
BOOLEAN  gDispatcherRunning = FALSE;

//
// Module globals to manage the FwVol registration notification event
//
EFI_EVENT       mFwVolEvent;
VOID            *mFwVolEventRegistration;

//
// List of file types supported by dispatcher
//
EFI_FV_FILETYPE mDxeFileTypes[] = {
  EFI_FV_FILETYPE_DRIVER,
  EFI_FV_FILETYPE_COMBINED_SMM_DXE,
  EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER,
  EFI_FV_FILETYPE_DXE_CORE,
  EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
};

typedef struct {
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   File;
  EFI_DEVICE_PATH_PROTOCOL            End;
} FV_FILEPATH_DEVICE_PATH;

FV_FILEPATH_DEVICE_PATH mFvDevicePath;

//
// Function Prototypes
//
/**
  Insert InsertedDriverEntry onto the mScheduledQueue. To do this you
  must add any driver with a before dependency on InsertedDriverEntry first.
  You do this by recursively calling this routine. After all the Befores are
  processed you can add InsertedDriverEntry to the mScheduledQueue.
  Then you can add any driver with an After dependency on InsertedDriverEntry
  by recursively calling this routine.

  @param  InsertedDriverEntry   The driver to insert on the ScheduledLink Queue

**/
VOID
CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (
  IN  EFI_CORE_DRIVER_ENTRY   *InsertedDriverEntry
  );

/**
  Event notification that is fired every time a FV dispatch protocol is added.
  More than one protocol may have been added when this event is fired, so you
  must loop on CoreLocateHandle () to see how many protocols were added and
  do the following to each FV:
  If the Fv has already been processed, skip it. If the Fv has not been
  processed then mark it as being processed, as we are about to process it.
  Read the Fv and add any driver in the Fv to the mDiscoveredList.The
  mDiscoveredList is never free'ed and contains variables that define
  the other states the DXE driver transitions to..
  While you are at it read the A Priori file into memory.
  Place drivers in the A Priori list onto the mScheduledQueue.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
CoreFwVolEventProtocolNotify (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  );

/**
  Convert FvHandle and DriverName into an EFI device path

  @param  Fv                    Fv protocol, needed to read Depex info out of
                                FLASH.
  @param  FvHandle              Handle for Fv, needed in the
                                EFI_CORE_DRIVER_ENTRY so that the PE image can be
                                read out of the FV at a later time.
  @param  DriverName            Name of driver to add to mDiscoveredList.

  @return Pointer to device path constructed from FvHandle and DriverName

**/
EFI_DEVICE_PATH_PROTOCOL *
CoreFvToDevicePath (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  );

/**
  Add an entry to the mDiscoveredList. Allocate memory to store the DriverEntry,
  and initilize any state variables. Read the Depex from the FV and store it
  in DriverEntry. Pre-process the Depex to set the SOR, Before and After state.
  The Discovered list is never free'ed and contains booleans that represent the
  other possible DXE driver states.

  @param  Fv                    Fv protocol, needed to read Depex info out of
                                FLASH.
  @param  FvHandle              Handle for Fv, needed in the
                                EFI_CORE_DRIVER_ENTRY so that the PE image can be
                                read out of the FV at a later time.
  @param  DriverName            Name of driver to add to mDiscoveredList.
  @param  Type                  Fv File Type of file to add to mDiscoveredList.

  @retval EFI_SUCCESS           If driver was added to the mDiscoveredList.
  @retval EFI_ALREADY_STARTED   The driver has already been started. Only one
                                DriverName may be active in the system at any one
                                time.

**/
EFI_STATUS
CoreAddToDriverList (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName,
  IN  EFI_FV_FILETYPE                 Type
  );

/**
  Get the driver from the FV through driver name, and produce a FVB protocol on FvHandle.

  @param  Fv                    The FIRMWARE_VOLUME protocol installed on the FV.
  @param  FvHandle              The handle which FVB protocol installed on.
  @param  DriverName            The driver guid specified.

  @retval EFI_OUT_OF_RESOURCES  No enough memory or other resource.
  @retval EFI_VOLUME_CORRUPTED  Corrupted volume.
  @retval EFI_SUCCESS           Function successfully returned.

**/
EFI_STATUS
CoreProcessFvImageFile (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  );


/**
  Enter critical section by gaining lock on mDispatcherLock.

**/
VOID
CoreAcquireDispatcherLock (
  VOID
  )
{
  CoreAcquireLock (&mDispatcherLock);
}


/**
  Exit critical section by releasing lock on mDispatcherLock.

**/
VOID
CoreReleaseDispatcherLock (
  VOID
  )
{
  CoreReleaseLock (&mDispatcherLock);
}


/**
  Read Depex and pre-process the Depex for Before and After. If Section Extraction
  protocol returns an error via ReadSection defer the reading of the Depex.

  @param  DriverEntry           Driver to work on.

  @retval EFI_SUCCESS           Depex read and preprossesed
  @retval EFI_PROTOCOL_ERROR    The section extraction protocol returned an error
                                and  Depex reading needs to be retried.
  @retval Error                 DEPEX not found.

**/
EFI_STATUS
CoreGetDepexSectionAndPreProccess (
  IN  EFI_CORE_DRIVER_ENTRY   *DriverEntry
  )
{
  EFI_STATUS                    Status;
  EFI_SECTION_TYPE              SectionType;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;


  Fv = DriverEntry->Fv;

  //
  // Grab Depex info, it will never be free'ed.
  //
  SectionType         = EFI_SECTION_DXE_DEPEX;
  Status = Fv->ReadSection (
                DriverEntry->Fv,
                &DriverEntry->FileName,
                SectionType,
                0,
                &DriverEntry->Depex,
                (UINTN *)&DriverEntry->DepexSize,
                &AuthenticationStatus
                );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_PROTOCOL_ERROR) {
      //
      // The section extraction protocol failed so set protocol error flag
      //
      DriverEntry->DepexProtocolError = TRUE;
    } else {
      //
      // If no Depex assume UEFI 2.0 driver model
      //
      DriverEntry->Depex = NULL;
      DriverEntry->Dependent = TRUE;
      DriverEntry->DepexProtocolError = FALSE;
    }
  } else {
    //
    // Set Before, After, and Unrequested state information based on Depex
    // Driver will be put in Dependent or Unrequested state
    //
    CorePreProcessDepex (DriverEntry);
    DriverEntry->DepexProtocolError = FALSE;
  }

  return Status;
}


/**
  Check every driver and locate a matching one. If the driver is found, the Unrequested
  state flag is cleared.

  @param  FirmwareVolumeHandle  The handle of the Firmware Volume that contains
                                the firmware  file specified by DriverName.
  @param  DriverName            The Driver name to put in the Dependent state.

  @retval EFI_SUCCESS           The DriverName was found and it's SOR bit was
                                cleared
  @retval EFI_NOT_FOUND         The DriverName does not exist or it's SOR bit was
                                not set.

**/
EFI_STATUS
EFIAPI
CoreSchedule (
  IN  EFI_HANDLE  FirmwareVolumeHandle,
  IN  EFI_GUID    *DriverName
  )
{
  LIST_ENTRY            *Link;
  EFI_CORE_DRIVER_ENTRY *DriverEntry;

  //
  // Check every driver
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->FvHandle == FirmwareVolumeHandle &&
        DriverEntry->Unrequested &&
        CompareGuid (DriverName, &DriverEntry->FileName)) {
      //
      // Move the driver from the Unrequested to the Dependent state
      //
      CoreAcquireDispatcherLock ();
      DriverEntry->Unrequested  = FALSE;
      DriverEntry->Dependent    = TRUE;
      CoreReleaseDispatcherLock ();

      DEBUG ((DEBUG_DISPATCH, "Schedule FFS(%g) - EFI_SUCCESS\n", DriverName));
      
      return EFI_SUCCESS;
    }
  }
  
  DEBUG ((DEBUG_DISPATCH, "Schedule FFS(%g) - EFI_NOT_FOUND\n", DriverName));
  
  return EFI_NOT_FOUND;
}



/**
  Convert a driver from the Untrused back to the Scheduled state.

  @param  FirmwareVolumeHandle  The handle of the Firmware Volume that contains
                                the firmware  file specified by DriverName.
  @param  DriverName            The Driver name to put in the Scheduled state

  @retval EFI_SUCCESS           The file was found in the untrusted state, and it
                                was promoted  to the trusted state.
  @retval EFI_NOT_FOUND         The file was not found in the untrusted state.

**/
EFI_STATUS
EFIAPI
CoreTrust (
  IN  EFI_HANDLE  FirmwareVolumeHandle,
  IN  EFI_GUID    *DriverName
  )
{
  LIST_ENTRY            *Link;
  EFI_CORE_DRIVER_ENTRY *DriverEntry;

  //
  // Check every driver
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->FvHandle == FirmwareVolumeHandle &&
        DriverEntry->Untrusted &&
        CompareGuid (DriverName, &DriverEntry->FileName)) {
      //
      // Transition driver from Untrusted to Scheduled state.
      //
      CoreAcquireDispatcherLock ();
      DriverEntry->Untrusted = FALSE;
      DriverEntry->Scheduled = TRUE;
      InsertTailList (&mScheduledQueue, &DriverEntry->ScheduledLink);
      CoreReleaseDispatcherLock ();

      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

/**
  This is the main Dispatcher for DXE and it exits when there are no more
  drivers to run. Drain the mScheduledQueue and load and start a PE
  image for each driver. Search the mDiscoveredList to see if any driver can
  be placed on the mScheduledQueue. If no drivers are placed on the
  mScheduledQueue exit the function. On exit it is assumed the Bds()
  will be called, and when the Bds() exits the Dispatcher will be called
  again.

  @retval EFI_ALREADY_STARTED   The DXE Dispatcher is already running
  @retval EFI_NOT_FOUND         No DXE Drivers were dispatched
  @retval EFI_SUCCESS           One or more DXE Drivers were dispatched

**/
EFI_STATUS
EFIAPI
CoreDispatcher (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      ReturnStatus;
  LIST_ENTRY                      *Link;
  EFI_CORE_DRIVER_ENTRY           *DriverEntry;
  BOOLEAN                         ReadyToRun;
  EFI_EVENT                       DxeDispatchEvent;
  

  if (gDispatcherRunning) {
    //
    // If the dispatcher is running don't let it be restarted.
    //
    return EFI_ALREADY_STARTED;
  }

  gDispatcherRunning = TRUE;

  Status = CoreCreateEventEx (
             EVT_NOTIFY_SIGNAL,
             TPL_NOTIFY,
             EfiEventEmptyFunction,
             NULL,
             &gEfiEventDxeDispatchGuid,
             &DxeDispatchEvent
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ReturnStatus = EFI_NOT_FOUND;
  do {
    //
    // Drain the Scheduled Queue
    //
    while (!IsListEmpty (&mScheduledQueue)) {
      DriverEntry = CR (
                      mScheduledQueue.ForwardLink,
                      EFI_CORE_DRIVER_ENTRY,
                      ScheduledLink,
                      EFI_CORE_DRIVER_ENTRY_SIGNATURE
                      );

      //
      // Load the DXE Driver image into memory. If the Driver was transitioned from
      // Untrused to Scheduled it would have already been loaded so we may need to
      // skip the LoadImage
      //
      if (DriverEntry->ImageHandle == NULL && !DriverEntry->IsFvImage) {
        DEBUG ((DEBUG_INFO, "Loading driver %g\n", &DriverEntry->FileName));
        Status = CoreLoadImage (
                        FALSE,
                        gDxeCoreImageHandle,
                        DriverEntry->FvFileDevicePath,
                        NULL,
                        0,
                        &DriverEntry->ImageHandle
                        );

        //
        // Update the driver state to reflect that it's been loaded
        //
        if (EFI_ERROR (Status)) {
          CoreAcquireDispatcherLock ();

          if (Status == EFI_SECURITY_VIOLATION) {
            //
            // Take driver from Scheduled to Untrused state
            //
            DriverEntry->Untrusted = TRUE;
          } else {
            //
            // The DXE Driver could not be loaded, and do not attempt to load or start it again.
            // Take driver from Scheduled to Initialized.
            //
            // This case include the Never Trusted state if EFI_ACCESS_DENIED is returned
            //
            DriverEntry->Initialized  = TRUE;
          }

          DriverEntry->Scheduled = FALSE;
          RemoveEntryList (&DriverEntry->ScheduledLink);

          CoreReleaseDispatcherLock ();

          //
          // If it's an error don't try the StartImage
          //
          continue;
        }
      }

      CoreAcquireDispatcherLock ();

      DriverEntry->Scheduled    = FALSE;
      DriverEntry->Initialized  = TRUE;
      RemoveEntryList (&DriverEntry->ScheduledLink);

      CoreReleaseDispatcherLock ();

 
      if (DriverEntry->IsFvImage) {
        //
        // Produce a firmware volume block protocol for FvImage so it gets dispatched from. 
        //
        Status = CoreProcessFvImageFile (DriverEntry->Fv, DriverEntry->FvHandle, &DriverEntry->FileName);
      } else {
        REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
          EFI_PROGRESS_CODE,
          (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_BEGIN),
          &DriverEntry->ImageHandle,
          sizeof (DriverEntry->ImageHandle)
          );
        ASSERT (DriverEntry->ImageHandle != NULL);
  
        Status = CoreStartImage (DriverEntry->ImageHandle, NULL, NULL);
  
        REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
          EFI_PROGRESS_CODE,
          (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_END),
          &DriverEntry->ImageHandle,
          sizeof (DriverEntry->ImageHandle)
          );
      }

      ReturnStatus = EFI_SUCCESS;
    }

    //
    // Now DXE Dispatcher finished one round of dispatch, signal an event group
    // so that SMM Dispatcher get chance to dispatch SMM Drivers which depend
    // on UEFI protocols
    //
    if (!EFI_ERROR (ReturnStatus)) {
      CoreSignalEvent (DxeDispatchEvent);
    }

    //
    // Search DriverList for items to place on Scheduled Queue
    //
    ReadyToRun = FALSE;
    for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
      DriverEntry = CR (Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);

      if (DriverEntry->DepexProtocolError){
        //
        // If Section Extraction Protocol did not let the Depex be read before retry the read
        //
        Status = CoreGetDepexSectionAndPreProccess (DriverEntry);
      }

      if (DriverEntry->Dependent) {
        if (CoreIsSchedulable (DriverEntry)) {
          CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (DriverEntry);
          ReadyToRun = TRUE;
        }
      } else {
        if (DriverEntry->Unrequested) {
          DEBUG ((DEBUG_DISPATCH, "Evaluate DXE DEPEX for FFS(%g)\n", &DriverEntry->FileName));
          DEBUG ((DEBUG_DISPATCH, "  SOR                                             = Not Requested\n"));
          DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE\n"));
        }
      }
    }
  } while (ReadyToRun);

  //
  // Close DXE dispatch Event
  //
  CoreCloseEvent (DxeDispatchEvent);

  gDispatcherRunning = FALSE;

  return ReturnStatus;
}


/**
  Insert InsertedDriverEntry onto the mScheduledQueue. To do this you
  must add any driver with a before dependency on InsertedDriverEntry first.
  You do this by recursively calling this routine. After all the Befores are
  processed you can add InsertedDriverEntry to the mScheduledQueue.
  Then you can add any driver with an After dependency on InsertedDriverEntry
  by recursively calling this routine.

  @param  InsertedDriverEntry   The driver to insert on the ScheduledLink Queue

**/
VOID
CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (
  IN  EFI_CORE_DRIVER_ENTRY   *InsertedDriverEntry
  )
{
  LIST_ENTRY            *Link;
  EFI_CORE_DRIVER_ENTRY *DriverEntry;

  //
  // Process Before Dependency
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->Before && DriverEntry->Dependent && DriverEntry != InsertedDriverEntry) {
      DEBUG ((DEBUG_DISPATCH, "Evaluate DXE DEPEX for FFS(%g)\n", &DriverEntry->FileName));
      DEBUG ((DEBUG_DISPATCH, "  BEFORE FFS(%g) = ", &DriverEntry->BeforeAfterGuid));
      if (CompareGuid (&InsertedDriverEntry->FileName, &DriverEntry->BeforeAfterGuid)) {
        //
        // Recursively process BEFORE
        //
        DEBUG ((DEBUG_DISPATCH, "TRUE\n  END\n  RESULT = TRUE\n"));
        CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (DriverEntry);
      } else {
        DEBUG ((DEBUG_DISPATCH, "FALSE\n  END\n  RESULT = FALSE\n"));
      }
    }
  }

  //
  // Convert driver from Dependent to Scheduled state
  //
  CoreAcquireDispatcherLock ();

  InsertedDriverEntry->Dependent = FALSE;
  InsertedDriverEntry->Scheduled = TRUE;
  InsertTailList (&mScheduledQueue, &InsertedDriverEntry->ScheduledLink);

  CoreReleaseDispatcherLock ();

  //
  // Process After Dependency
  //
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->After && DriverEntry->Dependent && DriverEntry != InsertedDriverEntry) {
      DEBUG ((DEBUG_DISPATCH, "Evaluate DXE DEPEX for FFS(%g)\n", &DriverEntry->FileName));
      DEBUG ((DEBUG_DISPATCH, "  AFTER FFS(%g) = ", &DriverEntry->BeforeAfterGuid));
      if (CompareGuid (&InsertedDriverEntry->FileName, &DriverEntry->BeforeAfterGuid)) {
        //
        // Recursively process AFTER
        //
        DEBUG ((DEBUG_DISPATCH, "TRUE\n  END\n  RESULT = TRUE\n"));
        CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter (DriverEntry);
      } else {
        DEBUG ((DEBUG_DISPATCH, "FALSE\n  END\n  RESULT = FALSE\n"));
      }
    }
  }
}


/**
  Return TRUE if the Fv has been processed, FALSE if not.

  @param  FvHandle              The handle of a FV that's being tested

  @retval TRUE                  Fv protocol on FvHandle has been processed
  @retval FALSE                 Fv protocol on FvHandle has not yet been processed

**/
BOOLEAN
FvHasBeenProcessed (
  IN  EFI_HANDLE      FvHandle
  )
{
  LIST_ENTRY      *Link;
  KNOWN_HANDLE    *KnownHandle;

  for (Link = mFvHandleList.ForwardLink; Link != &mFvHandleList; Link = Link->ForwardLink) {
    KnownHandle = CR(Link, KNOWN_HANDLE, Link, KNOWN_HANDLE_SIGNATURE);
    if (KnownHandle->Handle == FvHandle) {
      return TRUE;
    }
  }
  return FALSE;
}


/**
  Remember that Fv protocol on FvHandle has had it's drivers placed on the
  mDiscoveredList. This fucntion adds entries on the mFvHandleList if new 
  entry is different from one in mFvHandleList by checking FvImage Guid.
  Items are never removed/freed from the mFvHandleList.

  @param  FvHandle              The handle of a FV that has been processed

  @return A point to new added FvHandle entry. If FvHandle with the same FvImage guid
          has been added, NULL will return. 

**/
KNOWN_HANDLE * 
FvIsBeingProcesssed (
  IN  EFI_HANDLE    FvHandle
  )
{
  EFI_STATUS                            Status;
  EFI_GUID                              FvNameGuid;
  BOOLEAN                               FvNameGuidIsFound;
  UINT32                                ExtHeaderOffset;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_FV_BLOCK_MAP_ENTRY                *BlockMap;
  UINTN                                 LbaOffset;
  UINTN                                 Index;
  EFI_LBA                               LbaIndex;
  LIST_ENTRY                            *Link;
  KNOWN_HANDLE                          *KnownHandle;

  FwVolHeader = NULL;

  //
  // Get the FirmwareVolumeBlock protocol on that handle
  //
  FvNameGuidIsFound = FALSE;
  Status = CoreHandleProtocol (FvHandle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **)&Fvb);
  if (!EFI_ERROR (Status)) {
    //
    // Get the full FV header based on FVB protocol.
    //
    ASSERT (Fvb != NULL);
    Status = GetFwVolHeader (Fvb, &FwVolHeader);
    if (!EFI_ERROR (Status)) {
      ASSERT (FwVolHeader != NULL);
      if (VerifyFvHeaderChecksum (FwVolHeader) && FwVolHeader->ExtHeaderOffset != 0) {
        ExtHeaderOffset = (UINT32) FwVolHeader->ExtHeaderOffset;
        BlockMap  = FwVolHeader->BlockMap;
        LbaIndex  = 0;
        LbaOffset = 0;
        //
        // Find LbaIndex and LbaOffset for FV extension header based on BlockMap.
        //
        while ((BlockMap->NumBlocks != 0) || (BlockMap->Length != 0)) {
          for (Index = 0; Index < BlockMap->NumBlocks && ExtHeaderOffset >= BlockMap->Length; Index ++) {
            ExtHeaderOffset -= BlockMap->Length;
            LbaIndex ++;
          }
          //
          // Check whether FvExtHeader is crossing the multi block range.
          //
          if (Index < BlockMap->NumBlocks) {
            LbaOffset = ExtHeaderOffset;
            break;
          }
          BlockMap++;
        }
        //
        // Read FvNameGuid from FV extension header.
        //
        Status = ReadFvbData (Fvb, &LbaIndex, &LbaOffset, sizeof (FvNameGuid), (UINT8 *) &FvNameGuid);
        if (!EFI_ERROR (Status)) {
          FvNameGuidIsFound = TRUE;
        }
      }
      CoreFreePool (FwVolHeader);
    }
  }

  if (FvNameGuidIsFound) {
    //
    // Check whether the FV image with the found FvNameGuid has been processed.
    //
    for (Link = mFvHandleList.ForwardLink; Link != &mFvHandleList; Link = Link->ForwardLink) {
      KnownHandle = CR(Link, KNOWN_HANDLE, Link, KNOWN_HANDLE_SIGNATURE);
      if (CompareGuid (&FvNameGuid, &KnownHandle->FvNameGuid)) {
        DEBUG ((EFI_D_ERROR, "FvImage on FvHandle %p and %p has the same FvNameGuid %g.\n", FvHandle, KnownHandle->Handle, &FvNameGuid));
        return NULL;
      }
    }
  }

  KnownHandle = AllocateZeroPool (sizeof (KNOWN_HANDLE));
  ASSERT (KnownHandle != NULL);

  KnownHandle->Signature = KNOWN_HANDLE_SIGNATURE;
  KnownHandle->Handle = FvHandle;
  if (FvNameGuidIsFound) {
    CopyGuid (&KnownHandle->FvNameGuid, &FvNameGuid);
  }
  InsertTailList (&mFvHandleList, &KnownHandle->Link);
  return KnownHandle;
}




/**
  Convert FvHandle and DriverName into an EFI device path

  @param  Fv                    Fv protocol, needed to read Depex info out of
                                FLASH.
  @param  FvHandle              Handle for Fv, needed in the
                                EFI_CORE_DRIVER_ENTRY so that the PE image can be
                                read out of the FV at a later time.
  @param  DriverName            Name of driver to add to mDiscoveredList.

  @return Pointer to device path constructed from FvHandle and DriverName

**/
EFI_DEVICE_PATH_PROTOCOL *
CoreFvToDevicePath (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *FvDevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *FileNameDevicePath;

  //
  // Remember the device path of the FV
  //
  Status = CoreHandleProtocol (FvHandle, &gEfiDevicePathProtocolGuid, (VOID **)&FvDevicePath);
  if (EFI_ERROR (Status)) {
    FileNameDevicePath = NULL;
  } else {
    //
    // Build a device path to the file in the FV to pass into gBS->LoadImage
    //
    EfiInitializeFwVolDevicepathNode (&mFvDevicePath.File, DriverName);
    SetDevicePathEndNode (&mFvDevicePath.End);

    FileNameDevicePath = AppendDevicePath (
                            FvDevicePath,
                            (EFI_DEVICE_PATH_PROTOCOL *)&mFvDevicePath
                            );
  }

  return FileNameDevicePath;
}



/**
  Add an entry to the mDiscoveredList. Allocate memory to store the DriverEntry,
  and initilize any state variables. Read the Depex from the FV and store it
  in DriverEntry. Pre-process the Depex to set the SOR, Before and After state.
  The Discovered list is never free'ed and contains booleans that represent the
  other possible DXE driver states.

  @param  Fv                    Fv protocol, needed to read Depex info out of
                                FLASH.
  @param  FvHandle              Handle for Fv, needed in the
                                EFI_CORE_DRIVER_ENTRY so that the PE image can be
                                read out of the FV at a later time.
  @param  DriverName            Name of driver to add to mDiscoveredList.
  @param  Type                  Fv File Type of file to add to mDiscoveredList.

  @retval EFI_SUCCESS           If driver was added to the mDiscoveredList.
  @retval EFI_ALREADY_STARTED   The driver has already been started. Only one
                                DriverName may be active in the system at any one
                                time.

**/
EFI_STATUS
CoreAddToDriverList (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName,
  IN  EFI_FV_FILETYPE                 Type
  )
{
  EFI_CORE_DRIVER_ENTRY               *DriverEntry;


  //
  // Create the Driver Entry for the list. ZeroPool initializes lots of variables to
  // NULL or FALSE.
  //
  DriverEntry = AllocateZeroPool (sizeof (EFI_CORE_DRIVER_ENTRY));
  ASSERT (DriverEntry != NULL);
  if (Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
    DriverEntry->IsFvImage = TRUE;
  }

  DriverEntry->Signature        = EFI_CORE_DRIVER_ENTRY_SIGNATURE;
  CopyGuid (&DriverEntry->FileName, DriverName);
  DriverEntry->FvHandle         = FvHandle;
  DriverEntry->Fv               = Fv;
  DriverEntry->FvFileDevicePath = CoreFvToDevicePath (Fv, FvHandle, DriverName);

  CoreGetDepexSectionAndPreProccess (DriverEntry);

  CoreAcquireDispatcherLock ();

  InsertTailList (&mDiscoveredList, &DriverEntry->Link);

  CoreReleaseDispatcherLock ();

  return EFI_SUCCESS;
}


/**
  Check if a FV Image type file (EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) is
  described by a EFI_HOB_FIRMWARE_VOLUME2 Hob.

  @param  FvNameGuid            The FV image guid specified.
  @param  DriverName            The driver guid specified.

  @retval TRUE                  This file is found in a EFI_HOB_FIRMWARE_VOLUME2
                                Hob.
  @retval FALSE                 Not found.

**/
BOOLEAN
FvFoundInHobFv2 (
  IN  CONST EFI_GUID                  *FvNameGuid,
  IN  CONST EFI_GUID                  *DriverName
  )
{
  EFI_PEI_HOB_POINTERS                HobFv2;

  HobFv2.Raw = GetHobList ();

  while ((HobFv2.Raw = GetNextHob (EFI_HOB_TYPE_FV2, HobFv2.Raw)) != NULL) {
    //
    // Compare parent FvNameGuid and FileGuid both.
    //
    if (CompareGuid (DriverName, &HobFv2.FirmwareVolume2->FileName) &&
        CompareGuid (FvNameGuid, &HobFv2.FirmwareVolume2->FvName)) {
      return TRUE;
    }
    HobFv2.Raw = GET_NEXT_HOB (HobFv2);
  }

  return FALSE;
}

/**
  Find USED_SIZE FV_EXT_TYPE entry in FV extension header and get the FV used size.

  @param[in]  FvHeader      Pointer to FV header.
  @param[out] FvUsedSize    Pointer to FV used size returned,
                            only valid if USED_SIZE FV_EXT_TYPE entry is found.
  @param[out] EraseByte     Pointer to erase byte returned,
                            only valid if USED_SIZE FV_EXT_TYPE entry is found.

  @retval TRUE              USED_SIZE FV_EXT_TYPE entry is found,
                            FV used size and erase byte are returned.
  @retval FALSE             No USED_SIZE FV_EXT_TYPE entry found.

**/
BOOLEAN
GetFvUsedSize (
  IN EFI_FIRMWARE_VOLUME_HEADER     *FvHeader,
  OUT UINT32                        *FvUsedSize,
  OUT UINT8                         *EraseByte
  )
{
  UINT16                                        ExtHeaderOffset;
  EFI_FIRMWARE_VOLUME_EXT_HEADER                *ExtHeader;
  EFI_FIRMWARE_VOLUME_EXT_ENTRY                 *ExtEntryList;
  EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE  *ExtEntryUsedSize;

  ExtHeaderOffset = ReadUnaligned16 (&FvHeader->ExtHeaderOffset);
  if (ExtHeaderOffset != 0) {
    ExtHeader    = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) ((UINT8 *) FvHeader + ExtHeaderOffset);
    ExtEntryList = (EFI_FIRMWARE_VOLUME_EXT_ENTRY *) (ExtHeader + 1);
    while ((UINTN) ExtEntryList < ((UINTN) ExtHeader + ReadUnaligned32 (&ExtHeader->ExtHeaderSize))) {
      if (ReadUnaligned16 (&ExtEntryList->ExtEntryType) == EFI_FV_EXT_TYPE_USED_SIZE_TYPE) {
        //
        // USED_SIZE FV_EXT_TYPE entry is found.
        //
        ExtEntryUsedSize = (EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE *) ExtEntryList;
        *FvUsedSize = ReadUnaligned32 (&ExtEntryUsedSize->UsedSize);
        if ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_ERASE_POLARITY) != 0) {
          *EraseByte = 0xFF;
        } else {
          *EraseByte = 0;
        }
        DEBUG ((
          DEBUG_INFO,
          "FV at 0x%x has 0x%x used size, and erase byte is 0x%02x\n",
          FvHeader,
          *FvUsedSize,
          *EraseByte
          ));
        return TRUE;
      }
      ExtEntryList = (EFI_FIRMWARE_VOLUME_EXT_ENTRY *)
                     ((UINT8 *) ExtEntryList + ReadUnaligned16 (&ExtEntryList->ExtEntrySize));
    }
  }

  //
  // No USED_SIZE FV_EXT_TYPE entry found.
  //
  return FALSE;
}

/**
  Get the driver from the FV through driver name, and produce a FVB protocol on FvHandle.

  @param  Fv                    The FIRMWARE_VOLUME protocol installed on the FV.
  @param  FvHandle              The handle which FVB protocol installed on.
  @param  DriverName            The driver guid specified.

  @retval EFI_OUT_OF_RESOURCES  No enough memory or other resource.
  @retval EFI_VOLUME_CORRUPTED  Corrupted volume.
  @retval EFI_SUCCESS           Function successfully returned.

**/
EFI_STATUS
CoreProcessFvImageFile (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv,
  IN  EFI_HANDLE                      FvHandle,
  IN  EFI_GUID                        *DriverName
  )
{
  EFI_STATUS                          Status;
  EFI_SECTION_TYPE                    SectionType;
  UINT32                              AuthenticationStatus;
  VOID                                *Buffer;
  VOID                                *AlignedBuffer;
  UINTN                               BufferSize;
  EFI_FIRMWARE_VOLUME_HEADER          *FvHeader;
  UINT32                              FvAlignment;
  EFI_DEVICE_PATH_PROTOCOL            *FvFileDevicePath;
  UINT32                              FvUsedSize;
  UINT8                               EraseByte;

  //
  // Read the first (and only the first) firmware volume section
  //
  SectionType   = EFI_SECTION_FIRMWARE_VOLUME_IMAGE;
  FvHeader      = NULL;
  FvAlignment   = 0;
  Buffer        = NULL;
  BufferSize    = 0;
  AlignedBuffer = NULL;
  Status = Fv->ReadSection (
                 Fv,
                 DriverName,
                 SectionType,
                 0,
                 &Buffer,
                 &BufferSize,
                 &AuthenticationStatus
                 );
  if (!EFI_ERROR (Status)) {
     //
    // Evaluate the authentication status of the Firmware Volume through
    // Security Architectural Protocol
    //
    if (gSecurity != NULL) {
      FvFileDevicePath = CoreFvToDevicePath (Fv, FvHandle, DriverName);
      Status = gSecurity->FileAuthenticationState (
                            gSecurity,
                            AuthenticationStatus,
                            FvFileDevicePath
                            );
      if (FvFileDevicePath != NULL) {
        FreePool (FvFileDevicePath);
      }

      if (Status != EFI_SUCCESS) {
        //
        // Security check failed. The firmware volume should not be used for any purpose.
        //
        if (Buffer != NULL) {
          FreePool (Buffer);
        }
        return Status;
      }
    }

    //
    // FvImage should be at its required alignment.
    //
    FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) Buffer;
    //
    // If EFI_FVB2_WEAK_ALIGNMENT is set in the volume header then the first byte of the volume
    // can be aligned on any power-of-two boundary. A weakly aligned volume can not be moved from
    // its initial linked location and maintain its alignment.
    //
    if ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_WEAK_ALIGNMENT) != EFI_FVB2_WEAK_ALIGNMENT) {
      //
      // Get FvHeader alignment
      //
      FvAlignment = 1 << ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_ALIGNMENT) >> 16);
      //
      // FvAlignment must be greater than or equal to 8 bytes of the minimum FFS alignment value.
      //
      if (FvAlignment < 8) {
        FvAlignment = 8;
      }

      DEBUG ((
        DEBUG_INFO,
        "%a() FV at 0x%x, FvAlignment required is 0x%x\n",
        __FUNCTION__,
        FvHeader,
        FvAlignment
        ));

      //
      // Check FvImage alignment.
      //
      if ((UINTN) FvHeader % FvAlignment != 0) {
        //
        // Allocate the aligned buffer for the FvImage.
        //
        AlignedBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), (UINTN) FvAlignment);
        if (AlignedBuffer == NULL) {
          FreePool (Buffer);
          return EFI_OUT_OF_RESOURCES;
        } else {
          //
          // Move FvImage into the aligned buffer and release the original buffer.
          //
          if (GetFvUsedSize (FvHeader, &FvUsedSize, &EraseByte)) {
            //
            // Copy the used bytes and fill the rest with the erase value.
            //
            CopyMem (AlignedBuffer, FvHeader, (UINTN) FvUsedSize);
            SetMem (
              (UINT8 *) AlignedBuffer + FvUsedSize,
              (UINTN) (BufferSize - FvUsedSize),
              EraseByte
              );
          } else {
            CopyMem (AlignedBuffer, Buffer, BufferSize);
          }
          FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) AlignedBuffer;
          CoreFreePool (Buffer);
          Buffer = NULL;
        }
      }
    }
    //
    // Produce a FVB protocol for the file
    //
    Status = ProduceFVBProtocolOnBuffer (
              (EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader,
              (UINT64)BufferSize,
              FvHandle,
              AuthenticationStatus,
              NULL
              );
  }

  if (EFI_ERROR (Status)) {
    //
    // ReadSection or Produce FVB failed, Free data buffer
    //
    if (Buffer != NULL) {
      FreePool (Buffer);
    }

    if (AlignedBuffer != NULL) {
      FreeAlignedPages (AlignedBuffer, EFI_SIZE_TO_PAGES (BufferSize));
    }
  }

  return Status;
}


/**
  Event notification that is fired every time a FV dispatch protocol is added.
  More than one protocol may have been added when this event is fired, so you
  must loop on CoreLocateHandle () to see how many protocols were added and
  do the following to each FV:
  If the Fv has already been processed, skip it. If the Fv has not been
  processed then mark it as being processed, as we are about to process it.
  Read the Fv and add any driver in the Fv to the mDiscoveredList.The
  mDiscoveredList is never free'ed and contains variables that define
  the other states the DXE driver transitions to..
  While you are at it read the A Priori file into memory.
  Place drivers in the A Priori list onto the mScheduledQueue.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
CoreFwVolEventProtocolNotify (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    GetNextFileStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  EFI_DEVICE_PATH_PROTOCOL      *FvDevicePath;
  EFI_HANDLE                    FvHandle;
  UINTN                         BufferSize;
  EFI_GUID                      NameGuid;
  UINTN                         Key;
  EFI_FV_FILETYPE               Type;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  EFI_CORE_DRIVER_ENTRY         *DriverEntry;
  EFI_GUID                      *AprioriFile;
  UINTN                         AprioriEntryCount;
  UINTN                         Index;
  LIST_ENTRY                    *Link;
  UINT32                        AuthenticationStatus;
  UINTN                         SizeOfBuffer;
  VOID                          *DepexBuffer;
  KNOWN_HANDLE                  *KnownHandle;

  FvHandle = NULL;

  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = CoreLocateHandle (
               ByRegisterNotify,
               NULL,
               mFwVolEventRegistration,
               &BufferSize,
               &FvHandle
               );
    if (EFI_ERROR (Status)) {
      //
      // If no more notification events exit
      //
      return;
    }

    if (FvHasBeenProcessed (FvHandle)) {
      //
      // This Fv has already been processed so lets skip it!
      //
      continue;
    }

    //
    // Since we are about to process this Fv mark it as processed.
    //
    KnownHandle = FvIsBeingProcesssed (FvHandle);
    if (KnownHandle == NULL) {
      //
      // The FV with the same FV name guid has already been processed. 
      // So lets skip it!
      //
      continue;
    }

    Status = CoreHandleProtocol (FvHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv);
    if (EFI_ERROR (Status) || Fv == NULL) {
      //
      // FvHandle must have Firmware Volume2 protocol thus we should never get here.
      //
      ASSERT (FALSE);
      continue;
    }

    Status = CoreHandleProtocol (FvHandle, &gEfiDevicePathProtocolGuid, (VOID **)&FvDevicePath);
    if (EFI_ERROR (Status)) {
      //
      // The Firmware volume doesn't have device path, can't be dispatched.
      //
      continue;
    }

    //
    // Discover Drivers in FV and add them to the Discovered Driver List.
    // Process EFI_FV_FILETYPE_DRIVER type and then EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER
    //  EFI_FV_FILETYPE_DXE_CORE is processed to produce a Loaded Image protocol for the core
    //  EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE is processed to create a Fvb
    //
    for (Index = 0; Index < sizeof (mDxeFileTypes) / sizeof (EFI_FV_FILETYPE); Index++) {
      //
      // Initialize the search key
      //
      Key = 0;
      do {
        Type = mDxeFileTypes[Index];
        GetNextFileStatus = Fv->GetNextFile (
                                  Fv,
                                  &Key,
                                  &Type,
                                  &NameGuid,
                                  &Attributes,
                                  &Size
                                  );
        if (!EFI_ERROR (GetNextFileStatus)) {
          if (Type == EFI_FV_FILETYPE_DXE_CORE) {
            //
            // If this is the DXE core fill in it's DevicePath & DeviceHandle
            //
            if (gDxeCoreLoadedImage->FilePath == NULL) {
              if (CompareGuid (&NameGuid, gDxeCoreFileName)) {
                //
                // Maybe One specail Fv cantains only one DXE_CORE module, so its device path must
                // be initialized completely.
                //
                EfiInitializeFwVolDevicepathNode (&mFvDevicePath.File, &NameGuid);
                SetDevicePathEndNode (&mFvDevicePath.End);

                gDxeCoreLoadedImage->FilePath = DuplicateDevicePath (
                                                  (EFI_DEVICE_PATH_PROTOCOL *)&mFvDevicePath
                                                  );
                gDxeCoreLoadedImage->DeviceHandle = FvHandle;
              }
            }
          } else if (Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
            //
            // Check if this EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE file has already
            // been extracted.
            //
            if (FvFoundInHobFv2 (&KnownHandle->FvNameGuid, &NameGuid)) {
              continue;
            }

            //
            // Check if this EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE file has SMM depex section.
            //
            DepexBuffer  = NULL;
            SizeOfBuffer = 0;
            Status = Fv->ReadSection (
                           Fv,
                           &NameGuid,
                           EFI_SECTION_SMM_DEPEX,
                           0,
                           &DepexBuffer,
                           &SizeOfBuffer,
                           &AuthenticationStatus
                           );
            if (!EFI_ERROR (Status)) {
              //
              // If SMM depex section is found, this FV image is invalid to be supported.
              // ASSERT FALSE to report this FV image.  
              //
              FreePool (DepexBuffer);
              ASSERT (FALSE);
            }

            //
            // Check if this EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE file has DXE depex section.
            //
            DepexBuffer  = NULL;
            SizeOfBuffer = 0;
            Status = Fv->ReadSection (
                           Fv,
                           &NameGuid,
                           EFI_SECTION_DXE_DEPEX,
                           0,
                           &DepexBuffer,
                           &SizeOfBuffer,
                           &AuthenticationStatus
                           );
            if (EFI_ERROR (Status)) {
              //
              // If no depex section, produce a firmware volume block protocol for it so it gets dispatched from. 
              //
              CoreProcessFvImageFile (Fv, FvHandle, &NameGuid);
            } else {
              //
              // If depex section is found, this FV image will be dispatched until its depex is evaluated to TRUE.
              //
              FreePool (DepexBuffer);
              CoreAddToDriverList (Fv, FvHandle, &NameGuid, Type);
            }
          } else {
            //
            // Transition driver from Undiscovered to Discovered state
            //
            CoreAddToDriverList (Fv, FvHandle, &NameGuid, Type);
          }
        }
      } while (!EFI_ERROR (GetNextFileStatus));
    }

    //
    // Read the array of GUIDs from the Apriori file if it is present in the firmware volume
    //
    AprioriFile = NULL;
    Status = Fv->ReadSection (
                  Fv,
                  &gAprioriGuid,
                  EFI_SECTION_RAW,
                  0,
                  (VOID **)&AprioriFile,
                  &SizeOfBuffer,
                  &AuthenticationStatus
                  );
    if (!EFI_ERROR (Status)) {
      AprioriEntryCount = SizeOfBuffer / sizeof (EFI_GUID);
    } else {
      AprioriEntryCount = 0;
    }

    //
    // Put drivers on Apriori List on the Scheduled queue. The Discovered List includes
    // drivers not in the current FV and these must be skipped since the a priori list
    // is only valid for the FV that it resided in.
    //

    for (Index = 0; Index < AprioriEntryCount; Index++) {
      for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
        DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
        if (CompareGuid (&DriverEntry->FileName, &AprioriFile[Index]) &&
            (FvHandle == DriverEntry->FvHandle)) {
          CoreAcquireDispatcherLock ();
          DriverEntry->Dependent = FALSE;
          DriverEntry->Scheduled = TRUE;
          InsertTailList (&mScheduledQueue, &DriverEntry->ScheduledLink);
          CoreReleaseDispatcherLock ();
          DEBUG ((DEBUG_DISPATCH, "Evaluate DXE DEPEX for FFS(%g)\n", &DriverEntry->FileName));
          DEBUG ((DEBUG_DISPATCH, "  RESULT = TRUE (Apriori)\n"));
          break;
        }
      }
    }

    //
    // Free data allocated by Fv->ReadSection ()
    //
    CoreFreePool (AprioriFile);
  }
}



/**
  Initialize the dispatcher. Initialize the notification function that runs when
  an FV2 protocol is added to the system.

**/
VOID
CoreInitializeDispatcher (
  VOID
  )
{
  mFwVolEvent = EfiCreateProtocolNotifyEvent (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  TPL_CALLBACK,
                  CoreFwVolEventProtocolNotify,
                  NULL,
                  &mFwVolEventRegistration
                  );
}

//
// Function only used in debug builds
//

/**
  Traverse the discovered list for any drivers that were discovered but not loaded
  because the dependency experessions evaluated to false.

**/
VOID
CoreDisplayDiscoveredNotDispatched (
  VOID
  )
{
  LIST_ENTRY                    *Link;
  EFI_CORE_DRIVER_ENTRY         *DriverEntry;

  for (Link = mDiscoveredList.ForwardLink;Link !=&mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry = CR(Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry->Dependent) {
      DEBUG ((DEBUG_LOAD, "Driver %g was discovered but not loaded!!\n", &DriverEntry->FileName));
    }
  }
}
