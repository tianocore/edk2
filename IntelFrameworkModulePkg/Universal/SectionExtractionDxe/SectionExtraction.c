/** @file
  Section Extraction Protocol implementation.
  
  Stream database is implemented as a linked list of section streams,
  where each stream contains a linked list of children, which may be leaves or
  encapsulations.  
  
  Children that are encapsulations generate new stream entries
  when they are created.  Streams can also be created by calls to 
  SEP->OpenSectionStream().
  
  The database is only created far enough to return the requested data from
  any given stream, or to determine that the requested data is not found.
  
  If a GUIDed encapsulation is encountered, there are three possiblilites.
  
  1) A support protocol is found, in which the stream is simply processed with
     the support protocol.
     
  2) A support protocol is not found, but the data is available to be read
     without processing.  In this case, the database is built up through the
     recursions to return the data, and a RPN event is set that will enable
     the stream in question to be refreshed if and when the required section
     extraction protocol is published.This insures the AuthenticationStatus 
     does not become stale in the cache.
     
  3) A support protocol is not found, and the data is not available to be read
     without it.  This results in EFI_PROTOCOL_ERROR.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <FrameworkDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Protocol/Decompress.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <Protocol/SectionExtraction.h>

//
// Local defines and typedefs
//
#define FRAMEWORK_SECTION_CHILD_SIGNATURE  SIGNATURE_32('S','X','F','S')
#define CHILD_SECTION_NODE_FROM_LINK(Node) \
  CR (Node, FRAMEWORK_SECTION_CHILD_NODE, Link, FRAMEWORK_SECTION_CHILD_SIGNATURE)

typedef struct {
  UINT32                      Signature;
  LIST_ENTRY                  Link;
  UINT32                      Type;
  UINT32                      Size;
  //
  // StreamBase + OffsetInStream == pointer to section header in stream.  The
  // stream base is always known when walking the sections within.
  //
  UINT32                      OffsetInStream;
  //
  // Then EncapsulatedStreamHandle below is always 0 if the section is NOT an
  // encapsulating section.  Otherwise, it contains the stream handle
  // of the encapsulated stream.  This handle is ALWAYS produced any time an
  // encapsulating child is encountered, irrespective of whether the
  // encapsulated stream is processed further.
  //
  UINTN                       EncapsulatedStreamHandle;
  EFI_GUID                    *EncapsulationGuid;
  //
  // If the section REQUIRES an extraction protocol, register for RPN 
  // when the required GUIDed extraction protocol becomes available.
  //
  EFI_EVENT                   Event;
} FRAMEWORK_SECTION_CHILD_NODE;

#define FRAMEWORK_SECTION_STREAM_SIGNATURE SIGNATURE_32('S','X','S','S')
#define STREAM_NODE_FROM_LINK(Node) \
  CR (Node, FRAMEWORK_SECTION_STREAM_NODE, Link, FRAMEWORK_SECTION_STREAM_SIGNATURE)

typedef struct {
  UINT32                      Signature;
  LIST_ENTRY                  Link;
  UINTN                       StreamHandle;
  UINT8                       *StreamBuffer;
  UINTN                       StreamLength;
  LIST_ENTRY                  Children;
  //
  // Authentication status is from GUIDed encapsulations.
  //
  UINT32                      AuthenticationStatus;
} FRAMEWORK_SECTION_STREAM_NODE;

#define NULL_STREAM_HANDLE    0

typedef struct {
  FRAMEWORK_SECTION_CHILD_NODE     *ChildNode;
  FRAMEWORK_SECTION_STREAM_NODE    *ParentStream;
  VOID                             *Registration;
} RPN_EVENT_CONTEXT;

/**
  SEP member function.  This function creates and returns a new section stream
  handle to represent the new section stream.

  @param This                 Indicates the calling context.
  @param SectionStreamLength  Size in bytes of the section stream.
  @param SectionStream        Buffer containing the new section stream.
  @param SectionStreamHandle  A pointer to a caller allocated UINTN that on output
                              contains the new section stream handle.

  @retval EFI_SUCCESS           Section wase opened successfully.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_INVALID_PARAMETER Section stream does not end concident with end of
                                last section.

**/
EFI_STATUS
EFIAPI
OpenSectionStream (
  IN     EFI_SECTION_EXTRACTION_PROTOCOL           *This,
  IN     UINTN                                     SectionStreamLength,
  IN     VOID                                      *SectionStream,
     OUT UINTN                                     *SectionStreamHandle
  )
;

/**
  SEP member function.  Retrieves requested section from section stream.

  @param This                  Pointer to SEP instance.
  @param SectionStreamHandle   The section stream from which to extract the requested
                               section.
  @param SectionType           A pointer to the type of section to search for.
  @param SectionDefinitionGuid If the section type is EFI_SECTION_GUID_DEFINED, then
                               SectionDefinitionGuid indicates which of these types
                               of sections to search for.
  @param SectionInstance       Indicates which instance of the requested section to
                               return.
  @param Buffer                Double indirection to buffer.  If *Buffer is non-null on
                               input, then the buffer is caller allocated.  If
                               *Buffer is NULL, then the buffer is callee allocated.
                               In either case, the requried buffer size is returned
                               in *BufferSize.
  @param BufferSize            On input, indicates the size of *Buffer if *Buffer is
                               non-null on input.  On output, indicates the required
                               size (allocated size if callee allocated) of *Buffer.
  @param AuthenticationStatus  Indicates the authentication status of the retrieved
                               section.

 
  @retval EFI_SUCCESS           Section was retrieved successfully
  @retval EFI_PROTOCOL_ERROR    A GUID defined section was encountered in the section 
                                stream with its EFI_GUIDED_SECTION_PROCESSING_REQUIRED
                                bit set, but there was no corresponding GUIDed Section 
                                Extraction Protocol in the handle database.  *Buffer is 
                                unmodified.
  @retval EFI_NOT_FOUND         An error was encountered when parsing the SectionStream.
                                This indicates the SectionStream  is not correctly 
                                formatted.
  @retval EFI_NOT_FOUND         The requested section does not exist.
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient resources to process the 
                                request.
  @retval EFI_INVALID_PARAMETER The SectionStreamHandle does not exist.
  @retval EFI_WARN_TOO_SMALL    The size of the caller allocated input buffer is 
                                insufficient to contain the requested section.  The 
                                input buffer is filled and contents are section contents
                                are truncated.

**/
EFI_STATUS
EFIAPI
GetSection (
  IN EFI_SECTION_EXTRACTION_PROTOCOL                    *This,
  IN UINTN                                              SectionStreamHandle,
  IN EFI_SECTION_TYPE                                   *SectionType,
  IN EFI_GUID                                           *SectionDefinitionGuid,
  IN UINTN                                              SectionInstance,
  IN VOID                                               **Buffer,
  IN OUT UINTN                                          *BufferSize,
  OUT UINT32                                            *AuthenticationStatus
  )
;

/**
  SEP member function.  Deletes an existing section stream

  @param This                   Indicates the calling context.
  @param StreamHandleToClose    Indicates the stream to close

  @retval EFI_SUCCESS           Section stream was closed successfully.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_INVALID_PARAMETER Section stream does not end concident with end of
                                last section.

**/
EFI_STATUS
EFIAPI
CloseSectionStream (
  IN  EFI_SECTION_EXTRACTION_PROTOCOL           *This,
  IN  UINTN                                     StreamHandleToClose
  )
;

//
// Module globals
//
LIST_ENTRY mStreamRoot = INITIALIZE_LIST_HEAD_VARIABLE (mStreamRoot);

EFI_HANDLE mSectionExtractionHandle = NULL;

EFI_SECTION_EXTRACTION_PROTOCOL mSectionExtraction = { 
  OpenSectionStream, 
  GetSection, 
  CloseSectionStream
};

/**
  Entry point of the section extraction code. Initializes an instance of the 
  section extraction interface and installs it on a new handle.

  @param ImageHandle             A handle for the image that is initializing this driver
  @param SystemTable             A pointer to the EFI system table        

  @retval EFI_SUCCESS            Driver initialized successfully
  @retval EFI_OUT_OF_RESOURCES   Could not allocate needed resources

**/
EFI_STATUS
EFIAPI
SectionExtractionEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                         Status;

  //
  // Install SEP to a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &mSectionExtractionHandle,
                  &gEfiSectionExtractionProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSectionExtraction
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**

  Check if a stream is valid.

  @param SectionStream         The section stream to be checked
  @param SectionStreamLength   The length of section stream

  @return A boolean value indicating the validness of the section stream.

**/
BOOLEAN
IsValidSectionStream (
  IN  VOID              *SectionStream,
  IN  UINTN             SectionStreamLength
  )
{
  UINTN                       TotalLength;
  UINTN                       SectionLength;
  EFI_COMMON_SECTION_HEADER   *SectionHeader;
  EFI_COMMON_SECTION_HEADER   *NextSectionHeader;

  TotalLength = 0;
  SectionHeader = (EFI_COMMON_SECTION_HEADER *)SectionStream;
  
  while (TotalLength < SectionStreamLength) {
    if (IS_SECTION2 (SectionHeader)) {
      SectionLength = SECTION2_SIZE (SectionHeader);
    } else {
      SectionLength = SECTION_SIZE (SectionHeader);
    }
    TotalLength += SectionLength;

    if (TotalLength == SectionStreamLength) {
      return TRUE;    
    }

    //
    // Move to the next byte following the section...
    //
    SectionHeader = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) SectionHeader + SectionLength);
    
    //
    // Figure out where the next section begins
    //
    NextSectionHeader = ALIGN_POINTER(SectionHeader, 4);
    TotalLength += (UINTN) NextSectionHeader - (UINTN) SectionHeader;
    SectionHeader = NextSectionHeader;
  }

  ASSERT (FALSE);
  return FALSE;
}

/**
  Worker function.  Constructor for section streams.

  @param SectionStreamLength   Size in bytes of the section stream.
  @param SectionStream         Buffer containing the new section stream.
  @param AllocateBuffer        Indicates whether the stream buffer is to be copied
                               or the input buffer is to be used in place.
  @param AuthenticationStatus  Indicates the default authentication status for the
                               new stream.
  @param SectionStreamHandle   A pointer to a caller allocated section stream handle.

  @retval EFI_SUCCESS           Stream was added to stream database.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

**/
EFI_STATUS
OpenSectionStreamEx (
  IN     UINTN                                     SectionStreamLength,
  IN     VOID                                      *SectionStream,
  IN     BOOLEAN                                   AllocateBuffer,
  IN     UINT32                                    AuthenticationStatus,   
     OUT UINTN                                     *SectionStreamHandle
  )
{
  FRAMEWORK_SECTION_STREAM_NODE    *NewStream;
  EFI_TPL                          OldTpl;
  
  //
  // Allocate a new stream
  //
  NewStream = AllocatePool (sizeof (FRAMEWORK_SECTION_STREAM_NODE));
  if (NewStream == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  if (AllocateBuffer) { 
    //
    // if we're here, we're double buffering, allocate the buffer and copy the
    // data in
    //
    if (SectionStreamLength > 0) {
      NewStream->StreamBuffer = AllocatePool (SectionStreamLength); 
      if (NewStream->StreamBuffer == NULL) {
        FreePool (NewStream);
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Copy in stream data
      //
      CopyMem (NewStream->StreamBuffer, SectionStream, SectionStreamLength);
    } else {
      //
      // It's possible to have a zero length section stream.
      //
      NewStream->StreamBuffer = NULL;
    }
  } else {
    //
    // If were here, the caller has supplied the buffer (it's an internal call)
    // so just assign the buffer.  This happens when we open section streams
    // as a result of expanding an encapsulating section.
    //
    NewStream->StreamBuffer = SectionStream;
  }
  
  //
  // Initialize the rest of the section stream
  //
  NewStream->Signature = FRAMEWORK_SECTION_STREAM_SIGNATURE;
  NewStream->StreamHandle = (UINTN) NewStream;
  NewStream->StreamLength = SectionStreamLength;
  InitializeListHead (&NewStream->Children);
  NewStream->AuthenticationStatus = AuthenticationStatus;
  
  //
  // Add new stream to stream list
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&mStreamRoot, &NewStream->Link);
  gBS->RestoreTPL (OldTpl);

  *SectionStreamHandle = NewStream->StreamHandle;
  
  return EFI_SUCCESS;
}

/**
  SEP member function.  This function creates and returns a new section stream
  handle to represent the new section stream.

  @param This                 Indicates the calling context.
  @param SectionStreamLength  Size in bytes of the section stream.
  @param SectionStream        Buffer containing the new section stream.
  @param SectionStreamHandle  A pointer to a caller allocated UINTN that on output
                              contains the new section stream handle.

  @retval EFI_SUCCESS           Section wase opened successfully.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_INVALID_PARAMETER Section stream does not end concident with end of
                                last section.

**/
EFI_STATUS
EFIAPI
OpenSectionStream (
  IN     EFI_SECTION_EXTRACTION_PROTOCOL           *This,
  IN     UINTN                                     SectionStreamLength,
  IN     VOID                                      *SectionStream,
     OUT UINTN                                     *SectionStreamHandle
  )
{
  //
  // Check to see section stream looks good...
  //
  if (!IsValidSectionStream (SectionStream, SectionStreamLength)) {
    return EFI_INVALID_PARAMETER;
  }
  
  return OpenSectionStreamEx ( 
          SectionStreamLength, 
          SectionStream,
          TRUE,
          0,
          SectionStreamHandle
          );
}

/**
  Worker function.  Determine if the input stream:child matches the input type.

  @param Stream                 Indicates the section stream associated with the child
  @param Child                  Indicates the child to check
  @param SearchType             Indicates the type of section to check against for
  @param SectionDefinitionGuid  Indicates the GUID to check against if the type is
                                EFI_SECTION_GUID_DEFINED

  @retval TRUE                  The child matches
  @retval FALSE                 The child doesn't match

**/
BOOLEAN
ChildIsType (
  IN FRAMEWORK_SECTION_STREAM_NODE *Stream,
  IN FRAMEWORK_SECTION_CHILD_NODE  *Child,
  IN EFI_SECTION_TYPE              SearchType,
  IN EFI_GUID                      *SectionDefinitionGuid
  )
{
  EFI_GUID_DEFINED_SECTION    *GuidedSection;
  
  if (SearchType == EFI_SECTION_ALL) {
    return TRUE;
  }
  if (Child->Type != SearchType) {
    return FALSE;
  }
  if ((SearchType != EFI_SECTION_GUID_DEFINED) || (SectionDefinitionGuid == NULL)) {
    return TRUE;
  }
  GuidedSection = (EFI_GUID_DEFINED_SECTION * )(Stream->StreamBuffer + Child->OffsetInStream);
  if (IS_SECTION2 (GuidedSection)) {
    return CompareGuid (&(((EFI_GUID_DEFINED_SECTION2 *) GuidedSection)->SectionDefinitionGuid), SectionDefinitionGuid);
  } else {
    return CompareGuid (&GuidedSection->SectionDefinitionGuid, SectionDefinitionGuid);
  }
}

/**
  Create a protocol notification event and return it.

  @param ProtocolGuid    Protocol to register notification event on.
  @param NotifyTpl       Maximum TPL to signal the NotifyFunction.
  @param NotifyFunction  EFI notification routine.
  @param NotifyContext   Context passed into Event when it is created.
  @param Registration    Registration key returned from RegisterProtocolNotify().
  @param SignalFlag      Boolean value to decide whether kick the event after register or not.

  @return The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
           is added to the system.

**/
EFI_EVENT
CreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration,
  IN  BOOLEAN             SignalFlag
  )
{
  EFI_STATUS              Status;
  EFI_EVENT               Event;

  //
  // Create the event
  //

  Status = gBS->CreateEvent (
            EVT_NOTIFY_SIGNAL,
            NotifyTpl,
            NotifyFunction,
            NotifyContext,
           &Event
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifactions on this event
  //

  Status = gBS->RegisterProtocolNotify (
            ProtocolGuid,
            Event,
            Registration
            );
  ASSERT_EFI_ERROR (Status);

  if (SignalFlag) {
    //
    // Kick the event so we will perform an initial pass of
    // current installed drivers
    //
    gBS->SignalEvent (Event);
  }

  return Event;
}

/**
  Verify the Guided Section GUID by checking if there is the Guided Section GUID configuration table recorded the GUID itself.

  @param GuidedSectionGuid          The Guided Section GUID.
  @param GuidedSectionExtraction    A pointer to the pointer to the supported Guided Section Extraction Protocol
                                    for the Guided Section.

  @return TRUE      The GuidedSectionGuid could be identified, and the pointer to
                    the Guided Section Extraction Protocol will be returned to *GuidedSectionExtraction.
  @return FALSE     The GuidedSectionGuid could not be identified, or 
                    the Guided Section Extraction Protocol has not been installed yet.

**/
BOOLEAN
VerifyGuidedSectionGuid (
  IN  EFI_GUID                                  *GuidedSectionGuid,
  OUT EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL    **GuidedSectionExtraction
  )
{
  EFI_GUID              *GuidRecorded;
  VOID                  *Interface;
  EFI_STATUS            Status;

  //
  // Check if there is the Guided Section GUID configuration table recorded the GUID itself.
  //
  Status = EfiGetSystemConfigurationTable (GuidedSectionGuid, (VOID **) &GuidRecorded);
  if (Status == EFI_SUCCESS) {
    if (CompareGuid (GuidRecorded, GuidedSectionGuid)) {
      //
      // Found the recorded GuidedSectionGuid.
      //
      Status = gBS->LocateProtocol (GuidedSectionGuid, NULL, (VOID **) &Interface);
      if (!EFI_ERROR (Status) && Interface != NULL) {
        //
        // Found the supported Guided Section Extraction Porotocol for the Guided Section.
        //
        *GuidedSectionExtraction = (EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL *) Interface;
        return TRUE;
      }
      return FALSE;
    }
  }

  return FALSE;
}

/**
  RPN callback function.  
  1. Initialize the section stream when the GUIDED_SECTION_EXTRACTION_PROTOCOL is installed.
  2. Removes a stale section stream and re-initializes it with an updated AuthenticationStatus.

  @param Event               The event that fired
  @param RpnContext          A pointer to the context that allows us to identify
                             the relevent encapsulation.

**/
VOID
EFIAPI
NotifyGuidedExtraction (
  IN   EFI_EVENT   Event,
  IN   VOID        *RpnContext
  )
{
  EFI_STATUS                              Status;
  EFI_GUID_DEFINED_SECTION                *GuidedHeader;
  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL  *GuidedExtraction;
  VOID                                    *NewStreamBuffer;
  UINTN                                   NewStreamBufferSize;
  UINT32                                  AuthenticationStatus;
  RPN_EVENT_CONTEXT                       *Context;
  
  Context = RpnContext;
  Status = EFI_SUCCESS;
  if (Context->ChildNode->EncapsulatedStreamHandle != NULL_STREAM_HANDLE) {
    Status = CloseSectionStream (&mSectionExtraction, Context->ChildNode->EncapsulatedStreamHandle);
  }
  if (!EFI_ERROR (Status)) {
    //
    // The stream is not initialized, open it. 
    // Or the stream closed successfully, so re-open the stream with correct AuthenticationStatus.
    //
  
    GuidedHeader = (EFI_GUID_DEFINED_SECTION *) 
      (Context->ParentStream->StreamBuffer + Context->ChildNode->OffsetInStream);
    ASSERT (GuidedHeader->CommonHeader.Type == EFI_SECTION_GUID_DEFINED);
    
    if (!VerifyGuidedSectionGuid (Context->ChildNode->EncapsulationGuid, &GuidedExtraction)) {
      return;
    }

    Status = GuidedExtraction->ExtractSection (
                                 GuidedExtraction,
                                 GuidedHeader,
                                 &NewStreamBuffer,
                                 &NewStreamBufferSize,
                                 &AuthenticationStatus
                                 );
    ASSERT_EFI_ERROR (Status);
    //
    // OR in the parent stream's aggregate status.
    //
    AuthenticationStatus |= Context->ParentStream->AuthenticationStatus & EFI_AGGREGATE_AUTH_STATUS_ALL;
    Status = OpenSectionStreamEx (
               NewStreamBufferSize,
               NewStreamBuffer,
               FALSE,
               AuthenticationStatus,
               &Context->ChildNode->EncapsulatedStreamHandle
               );
    ASSERT_EFI_ERROR (Status);
  }

  //
  //  If above, the stream did not close successfully, it indicates it's
  //  already been closed by someone, so just destroy the event and be done with
  //  it.
  //
  
  gBS->CloseEvent (Event);
  Context->ChildNode->Event = NULL;
  FreePool (Context);
}  

/**
  Worker function.  Constructor for RPN event if needed to keep AuthenticationStatus
  cache correct when a missing GUIDED_SECTION_EXTRACTION_PROTOCOL appears...

  @param ParentStream        Indicates the parent of the ecnapsulation section (child)
  @param ChildNode           Indicates the child node that is the encapsulation section.

**/
VOID
CreateGuidedExtractionRpnEvent (
  IN FRAMEWORK_SECTION_STREAM_NODE       *ParentStream,
  IN FRAMEWORK_SECTION_CHILD_NODE        *ChildNode
  )
{
  RPN_EVENT_CONTEXT *Context;
  
  //
  // Allocate new event structure and context
  //
  Context = AllocatePool (sizeof (RPN_EVENT_CONTEXT));
  ASSERT (Context != NULL);
  
  Context->ChildNode = ChildNode;
  Context->ParentStream = ParentStream;
 
  Context->ChildNode->Event = CreateProtocolNotifyEvent (
                                Context->ChildNode->EncapsulationGuid,
                                TPL_NOTIFY,
                                NotifyGuidedExtraction,
                                Context,
                                &Context->Registration,
                                FALSE
                                );
}

/**
  Worker function.  Constructor for new child nodes.

  @param Stream                Indicates the section stream in which to add the child.
  @param ChildOffset           Indicates the offset in Stream that is the beginning
                               of the child section.
  @param ChildNode             Indicates the Callee allocated and initialized child.

  @retval EFI_SUCCESS          Child node was found and returned.
  @retval EFI_OUT_OF_RESOURCES Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR   Encapsulation sections produce new stream handles when
                               the child node is created.  If the section type is GUID
                               defined, and the extraction GUID does not exist, and
                               producing the stream requires the GUID, then a protocol
                               error is generated and no child is produced.
                               Values returned by OpenSectionStreamEx.

**/
EFI_STATUS
CreateChildNode (
  IN     FRAMEWORK_SECTION_STREAM_NODE              *Stream,
  IN     UINT32                                     ChildOffset,
  OUT    FRAMEWORK_SECTION_CHILD_NODE               **ChildNode
  )
{
  EFI_STATUS                                   Status;
  EFI_COMMON_SECTION_HEADER                    *SectionHeader;
  EFI_COMPRESSION_SECTION                      *CompressionHeader;
  EFI_GUID_DEFINED_SECTION                     *GuidedHeader;
  EFI_DECOMPRESS_PROTOCOL                      *Decompress;
  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL       *GuidedExtraction;
  VOID                                         *NewStreamBuffer;
  VOID                                         *ScratchBuffer;
  UINT32                                       ScratchSize;
  UINTN                                        NewStreamBufferSize;
  UINT32                                       AuthenticationStatus;
  VOID                                         *CompressionSource;
  UINT32                                       CompressionSourceSize;
  UINT32                                       UncompressedLength;
  UINT8                                        CompressionType;
  UINT16                                       GuidedSectionAttributes;
    
  FRAMEWORK_SECTION_CHILD_NODE                      *Node;

  SectionHeader = (EFI_COMMON_SECTION_HEADER *) (Stream->StreamBuffer + ChildOffset);

  //
  // Allocate a new node
  //
  *ChildNode = AllocateZeroPool (sizeof (FRAMEWORK_SECTION_CHILD_NODE));
  Node = *ChildNode;
  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Now initialize it
  //
  Node->Signature = FRAMEWORK_SECTION_CHILD_SIGNATURE;
  Node->Type = SectionHeader->Type;
  if (IS_SECTION2 (SectionHeader)) {
    Node->Size = SECTION2_SIZE (SectionHeader);
  } else {
    Node->Size = SECTION_SIZE (SectionHeader);
  }
  Node->OffsetInStream = ChildOffset;
  Node->EncapsulatedStreamHandle = NULL_STREAM_HANDLE;
  Node->EncapsulationGuid = NULL;
  
  //
  // If it's an encapsulating section, then create the new section stream also
  //
  switch (Node->Type) {
    case EFI_SECTION_COMPRESSION:
      //
      // Get the CompressionSectionHeader
      //
      if (Node->Size < sizeof (EFI_COMPRESSION_SECTION)) {
        FreePool (Node);
        return EFI_NOT_FOUND;
      }

      CompressionHeader = (EFI_COMPRESSION_SECTION *) SectionHeader;

      if (IS_SECTION2 (CompressionHeader)) {
        CompressionSource = (VOID *) ((UINT8 *) CompressionHeader + sizeof (EFI_COMPRESSION_SECTION2));
        CompressionSourceSize = (UINT32) (SECTION2_SIZE (CompressionHeader) - sizeof (EFI_COMPRESSION_SECTION2));
        UncompressedLength = ((EFI_COMPRESSION_SECTION2 *) CompressionHeader)->UncompressedLength;
        CompressionType = ((EFI_COMPRESSION_SECTION2 *) CompressionHeader)->CompressionType;
      } else {
        CompressionSource = (VOID *) ((UINT8 *) CompressionHeader + sizeof (EFI_COMPRESSION_SECTION));
        CompressionSourceSize = (UINT32) (SECTION_SIZE (CompressionHeader) - sizeof (EFI_COMPRESSION_SECTION));
        UncompressedLength = CompressionHeader->UncompressedLength;
        CompressionType = CompressionHeader->CompressionType;
      }

      //
      // Allocate space for the new stream
      //
      if (UncompressedLength > 0) {
        NewStreamBufferSize = UncompressedLength;
        NewStreamBuffer = AllocatePool (NewStreamBufferSize);
        if (NewStreamBuffer == NULL) {
          FreePool (Node);
          return EFI_OUT_OF_RESOURCES;
        }
        
        if (CompressionType == EFI_NOT_COMPRESSED) {
          //
          // stream is not actually compressed, just encapsulated.  So just copy it.
          //
          CopyMem (NewStreamBuffer, CompressionSource, NewStreamBufferSize);
        } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
          //
          // Only support the EFI_SATNDARD_COMPRESSION algorithm.
          // 

          //
          // Decompress the stream
          //
          Status = gBS->LocateProtocol (&gEfiDecompressProtocolGuid, NULL, (VOID **)&Decompress);
          
          ASSERT_EFI_ERROR (Status);
          
          Status = Decompress->GetInfo (
                                 Decompress,
                                 CompressionSource,
                                 CompressionSourceSize,
                                 (UINT32 *)&NewStreamBufferSize,
                                 &ScratchSize
                                 );
          if (EFI_ERROR (Status) || (NewStreamBufferSize != UncompressedLength)) {
            FreePool (Node);
            FreePool (NewStreamBuffer);
            if (!EFI_ERROR (Status)) {
              Status = EFI_BAD_BUFFER_SIZE;
            }
            return Status;
          }

          ScratchBuffer = AllocatePool (ScratchSize);
          if (ScratchBuffer == NULL) {
            FreePool (Node);
            FreePool (NewStreamBuffer);
            return EFI_OUT_OF_RESOURCES;
          }

          Status = Decompress->Decompress (
                                 Decompress,
                                 CompressionSource,
                                 CompressionSourceSize,
                                 NewStreamBuffer,
                                 (UINT32)NewStreamBufferSize,
                                 ScratchBuffer,
                                 ScratchSize
                                 );
          FreePool (ScratchBuffer); 
          if (EFI_ERROR (Status)) {
            FreePool (Node);
            FreePool (NewStreamBuffer);
            return Status;
          }
        }
      } else {
        NewStreamBuffer = NULL;
        NewStreamBufferSize = 0;
      }
      
      Status = OpenSectionStreamEx (
                 NewStreamBufferSize,
                 NewStreamBuffer,
                 FALSE,
                 Stream->AuthenticationStatus,
                 &Node->EncapsulatedStreamHandle
                 );
      if (EFI_ERROR (Status)) {
        FreePool (Node);
        FreePool (NewStreamBuffer);
        return Status;
      }
      break;

    case EFI_SECTION_GUID_DEFINED:
      GuidedHeader = (EFI_GUID_DEFINED_SECTION *) SectionHeader;
      if (IS_SECTION2 (GuidedHeader)) {
        Node->EncapsulationGuid = &(((EFI_GUID_DEFINED_SECTION2 *) GuidedHeader)->SectionDefinitionGuid);
        GuidedSectionAttributes = ((EFI_GUID_DEFINED_SECTION2 *) GuidedHeader)->Attributes;
      } else {
        Node->EncapsulationGuid = &GuidedHeader->SectionDefinitionGuid;
        GuidedSectionAttributes = GuidedHeader->Attributes;
      }
      if (VerifyGuidedSectionGuid (Node->EncapsulationGuid, &GuidedExtraction)) {
        //
        // NewStreamBuffer is always allocated by ExtractSection... No caller
        // allocation here.
        //
        Status = GuidedExtraction->ExtractSection (
                                     GuidedExtraction,
                                     GuidedHeader,
                                     &NewStreamBuffer,
                                     &NewStreamBufferSize,
                                     &AuthenticationStatus
                                     );
        if (EFI_ERROR (Status)) {
          FreePool (*ChildNode);
          return EFI_PROTOCOL_ERROR;
        }
        
        //
        // Make sure we initialize the new stream with the correct 
        // authentication status for both aggregate and local status fields.
        //
        if ((GuidedSectionAttributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) == EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
          //
          // OR in the parent stream's aggregate status.
          //
          AuthenticationStatus |= Stream->AuthenticationStatus & EFI_AGGREGATE_AUTH_STATUS_ALL;
        } else {
          //
          // since there's no authentication data contributed by the section,
          // just inherit the full value from our immediate parent.
          //
          AuthenticationStatus = Stream->AuthenticationStatus;
        }
        
        Status = OpenSectionStreamEx (
                   NewStreamBufferSize,
                   NewStreamBuffer,
                   FALSE,
                   AuthenticationStatus,
                   &Node->EncapsulatedStreamHandle
                   );
        if (EFI_ERROR (Status)) {
          FreePool (*ChildNode);
          FreePool (NewStreamBuffer);
          return Status;
        }
      } else {
        //
        // There's no GUIDed section extraction protocol available.
        //
        if ((GuidedSectionAttributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
          //
          // If the section REQUIRES an extraction protocol, register for RPN 
          // when the required GUIDed extraction protocol becomes available. 
          //
          AuthenticationStatus = 0;
          CreateGuidedExtractionRpnEvent (Stream, Node);
        } else {
          //
          // Figure out the proper authentication status
          //
          AuthenticationStatus = Stream->AuthenticationStatus;
          if ((GuidedSectionAttributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) == EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
            //
            //  The local status of the new stream is contained in 
            //  AuthenticaionStatus.  This value needs to be ORed into the
            //  Aggregate bits also...
            //
            
            //
            // Clear out and initialize the local status
            //
            AuthenticationStatus &= ~EFI_LOCAL_AUTH_STATUS_ALL;
            AuthenticationStatus |= EFI_LOCAL_AUTH_STATUS_IMAGE_SIGNED | EFI_LOCAL_AUTH_STATUS_NOT_TESTED;
            //
            // OR local status into aggregate status
            //
            AuthenticationStatus |= AuthenticationStatus >> 16;
          }
          
          if (IS_SECTION2 (GuidedHeader)) {
            Status = OpenSectionStreamEx (
                       SECTION2_SIZE (GuidedHeader) - ((EFI_GUID_DEFINED_SECTION2 *) GuidedHeader)->DataOffset,
                       (UINT8 *) GuidedHeader + ((EFI_GUID_DEFINED_SECTION2 *) GuidedHeader)->DataOffset,
                       TRUE,
                       AuthenticationStatus,
                       &Node->EncapsulatedStreamHandle
                       );
          } else {
            Status = OpenSectionStreamEx (
                       SECTION_SIZE (GuidedHeader) - ((EFI_GUID_DEFINED_SECTION *) GuidedHeader)->DataOffset,
                       (UINT8 *) GuidedHeader + ((EFI_GUID_DEFINED_SECTION *) GuidedHeader)->DataOffset,
                       TRUE,
                       AuthenticationStatus,
                       &Node->EncapsulatedStreamHandle
                       );
          }
          if (EFI_ERROR (Status)) {
            FreePool (Node);
            return Status;
          }
        }
      }
      
      if ((AuthenticationStatus & EFI_LOCAL_AUTH_STATUS_ALL) == 
            (EFI_LOCAL_AUTH_STATUS_IMAGE_SIGNED | EFI_LOCAL_AUTH_STATUS_NOT_TESTED)) {
        //
        // Need to register for RPN for when the required GUIDed extraction
        // protocol becomes available.  This will enable us to refresh the
        // AuthenticationStatus cached in the Stream if it's ever requested
        // again.
        //
        CreateGuidedExtractionRpnEvent (Stream, Node);
      }
      
      break;

    default:
      
      //
      // Nothing to do if it's a leaf
      //
      break;
  }
  
  //
  // Last, add the new child node to the stream
  //
  InsertTailList (&Stream->Children, &Node->Link);

  return EFI_SUCCESS;
}

/**
  Worker function  Recursively searches / builds section stream database
  looking for requested section.


  @param SourceStream          Indicates the section stream in which to do the search.
  @param SearchType            Indicates the type of section to search for.
  @param SectionInstance       Indicates which instance of section to find.  This is
                               an in/out parameter to deal with recursions.
  @param SectionDefinitionGuid Guid of section definition
  @param FoundChild            Output indicating the child node that is found.
  @param FoundStream           Output indicating which section stream the child was
                               found in.  If this stream was generated as a result of
                               an encapsulation section, the streamhandle is visible
                               within the SEP driver only.
  @param AuthenticationStatus  Indicates the authentication status of the found section.

  @retval EFI_SUCCESS          Child node was found and returned.
  @retval EFI_OUT_OF_RESOURCES Memory allocation failed.
  @retval EFI_NOT_FOUND        Requested child node does not exist.
  @retval EFI_PROTOCOL_ERROR   A required GUIDED section extraction protocol does not
                               exist

**/
EFI_STATUS
FindChildNode (
  IN     FRAMEWORK_SECTION_STREAM_NODE                   *SourceStream,
  IN     EFI_SECTION_TYPE                           SearchType,
  IN OUT UINTN                                      *SectionInstance,
  IN     EFI_GUID                                   *SectionDefinitionGuid,
  OUT    FRAMEWORK_SECTION_CHILD_NODE                    **FoundChild,
  OUT    FRAMEWORK_SECTION_STREAM_NODE                   **FoundStream,
  OUT    UINT32                                     *AuthenticationStatus
  )
{
  FRAMEWORK_SECTION_CHILD_NODE                       *CurrentChildNode;
  FRAMEWORK_SECTION_CHILD_NODE                       *RecursedChildNode;
  FRAMEWORK_SECTION_STREAM_NODE                      *RecursedFoundStream;
  UINT32                                        NextChildOffset;
  EFI_STATUS                                    ErrorStatus;
  EFI_STATUS                                    Status;
  
  CurrentChildNode = NULL;
  ErrorStatus = EFI_NOT_FOUND;
  
  if (SourceStream->StreamLength == 0) {
    return EFI_NOT_FOUND;
  }
  
  if (IsListEmpty (&SourceStream->Children) && 
                   SourceStream->StreamLength >= sizeof (EFI_COMMON_SECTION_HEADER)) {
    //
    // This occurs when a section stream exists, but no child sections
    // have been parsed out yet.  Therefore, extract the first child and add it
    // to the list of children so we can get started.
    // Section stream may contain an array of zero or more bytes.
    // So, its size should be >= the size of commen section header.
    //
    Status = CreateChildNode (SourceStream, 0, &CurrentChildNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  
  //
  // At least one child has been parsed out of the section stream.  So, walk
  // through the sections that have already been parsed out looking for the
  // requested section, if necessary, continue parsing section stream and
  // adding children until either the requested section is found, or we run
  // out of data
  //
  CurrentChildNode = CHILD_SECTION_NODE_FROM_LINK (GetFirstNode(&SourceStream->Children));

  for (;;) {
    ASSERT (CurrentChildNode != NULL);
    if (ChildIsType (SourceStream, CurrentChildNode, SearchType, SectionDefinitionGuid)) {
      //
      // The type matches, so check the instance count to see if it's the one we want
      //
      (*SectionInstance)--;
      if (*SectionInstance == 0) {
        //
        // Got it!
        //
        *FoundChild = CurrentChildNode;
        *FoundStream = SourceStream;
        *AuthenticationStatus = SourceStream->AuthenticationStatus;
        return EFI_SUCCESS;
      }
    }
    
    if (CurrentChildNode->EncapsulatedStreamHandle != NULL_STREAM_HANDLE) {
      //
      // If the current node is an encapsulating node, recurse into it...
      //
      Status = FindChildNode (
                (FRAMEWORK_SECTION_STREAM_NODE *)CurrentChildNode->EncapsulatedStreamHandle,
                SearchType,
                SectionInstance,
                SectionDefinitionGuid,
                &RecursedChildNode,
                &RecursedFoundStream,
                AuthenticationStatus
                );
      //
      // If the status is not EFI_SUCCESS, just save the error code and continue
      // to find the request child node in the rest stream.
      //
      if (*SectionInstance == 0) {
        ASSERT_EFI_ERROR (Status);
        *FoundChild = RecursedChildNode;
        *FoundStream = RecursedFoundStream;
        return EFI_SUCCESS;
      } else {
        ErrorStatus = Status;
      }
    } else if ((CurrentChildNode->Type == EFI_SECTION_GUID_DEFINED) && (SearchType != EFI_SECTION_GUID_DEFINED)) {
      //
      // When Node Type is GUIDED section, but Node has no encapsulated data, Node data should not be parsed
      // because a required GUIDED section extraction protocol does not exist.
      // If SearchType is not GUIDED section, EFI_PROTOCOL_ERROR should return.
      //
      ErrorStatus = EFI_PROTOCOL_ERROR;
    }
    
    if (!IsNodeAtEnd (&SourceStream->Children, &CurrentChildNode->Link)) {
      //
      // We haven't found the child node we're interested in yet, but there's
      // still more nodes that have already been parsed so get the next one
      // and continue searching..
      //
      CurrentChildNode = CHILD_SECTION_NODE_FROM_LINK (GetNextNode (&SourceStream->Children, &CurrentChildNode->Link));
    } else {
      //
      // We've exhausted children that have already been parsed, so see if
      // there's any more data and continue parsing out more children if there
      // is.
      //
      NextChildOffset = CurrentChildNode->OffsetInStream + CurrentChildNode->Size;
      //
      // Round up to 4 byte boundary
      //
      NextChildOffset += 3;
      NextChildOffset &= ~(UINTN)3;
      if (NextChildOffset <= SourceStream->StreamLength - sizeof (EFI_COMMON_SECTION_HEADER)) {
        //
        // There's an unparsed child remaining in the stream, so create a new child node
        //
        Status = CreateChildNode (SourceStream, NextChildOffset, &CurrentChildNode);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      } else {
        ASSERT (EFI_ERROR (ErrorStatus));
        return ErrorStatus;
      }
    }
  }
}

/**
  Worker function.  Search stream database for requested stream handle.

  @param SearchHandle        Indicates which stream to look for.
  @param FoundStream         Output pointer to the found stream.

  @retval EFI_SUCCESS        StreamHandle was found and *FoundStream contains
                             the stream node.
  @retval EFI_NOT_FOUND      SearchHandle was not found in the stream database.

**/
EFI_STATUS
FindStreamNode (
  IN  UINTN                                     SearchHandle,
  OUT FRAMEWORK_SECTION_STREAM_NODE                  **FoundStream
  )
{  
  FRAMEWORK_SECTION_STREAM_NODE                      *StreamNode;
  
  if (!IsListEmpty (&mStreamRoot)) {
    StreamNode = STREAM_NODE_FROM_LINK (GetFirstNode (&mStreamRoot));
    for (;;) {
      if (StreamNode->StreamHandle == SearchHandle) {
        *FoundStream = StreamNode;
        return EFI_SUCCESS;
      } else if (IsNodeAtEnd (&mStreamRoot, &StreamNode->Link)) {
        break;
      } else {
        StreamNode = STREAM_NODE_FROM_LINK (GetNextNode (&mStreamRoot, &StreamNode->Link));
      }
    }
  }
  
  return EFI_NOT_FOUND;
}
  
/**
  SEP member function.  Retrieves requested section from section stream.

  @param This                  Pointer to SEP instance.
  @param SectionStreamHandle   The section stream from which to extract the requested
                               section.
  @param SectionType           A pointer to the type of section to search for.
  @param SectionDefinitionGuid If the section type is EFI_SECTION_GUID_DEFINED, then
                               SectionDefinitionGuid indicates which of these types
                               of sections to search for.
  @param SectionInstance       Indicates which instance of the requested section to
                               return.
  @param Buffer                Double indirection to buffer.  If *Buffer is non-null on
                               input, then the buffer is caller allocated.  If
                               *Buffer is NULL, then the buffer is callee allocated.
                               In either case, the requried buffer size is returned
                               in *BufferSize.
  @param BufferSize            On input, indicates the size of *Buffer if *Buffer is
                               non-null on input.  On output, indicates the required
                               size (allocated size if callee allocated) of *Buffer.
  @param AuthenticationStatus  Indicates the authentication status of the retrieved
                               section.

 
  @retval EFI_SUCCESS           Section was retrieved successfully
  @retval EFI_PROTOCOL_ERROR    A GUID defined section was encountered in the section 
                                stream with its EFI_GUIDED_SECTION_PROCESSING_REQUIRED
                                bit set, but there was no corresponding GUIDed Section 
                                Extraction Protocol in the handle database.  *Buffer is 
                                unmodified.
  @retval EFI_NOT_FOUND         An error was encountered when parsing the SectionStream.
                                This indicates the SectionStream  is not correctly 
                                formatted.
  @retval EFI_NOT_FOUND         The requested section does not exist.
  @retval EFI_OUT_OF_RESOURCES  The system has insufficient resources to process the 
                                request.
  @retval EFI_INVALID_PARAMETER The SectionStreamHandle does not exist.
  @retval EFI_WARN_TOO_SMALL    The size of the caller allocated input buffer is 
                                insufficient to contain the requested section.  The 
                                input buffer is filled and contents are section contents
                                are truncated.

**/
EFI_STATUS
EFIAPI
GetSection (
  IN EFI_SECTION_EXTRACTION_PROTOCOL                    *This,
  IN UINTN                                              SectionStreamHandle,
  IN EFI_SECTION_TYPE                                   *SectionType,
  IN EFI_GUID                                           *SectionDefinitionGuid,
  IN UINTN                                              SectionInstance,
  IN VOID                                               **Buffer,
  IN OUT UINTN                                          *BufferSize,
  OUT UINT32                                            *AuthenticationStatus
  )
{
  FRAMEWORK_SECTION_STREAM_NODE                         *StreamNode;
  EFI_TPL                                               OldTpl;
  EFI_STATUS                                            Status;
  FRAMEWORK_SECTION_CHILD_NODE                          *ChildNode;
  FRAMEWORK_SECTION_STREAM_NODE                         *ChildStreamNode;
  UINTN                                                 CopySize;
  UINT32                                                ExtractedAuthenticationStatus;
  UINTN                                                 Instance;
  UINT8                                                 *CopyBuffer;
  UINTN                                                 SectionSize;
  EFI_COMMON_SECTION_HEADER                             *Section;
  

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  Instance = SectionInstance + 1;
  ChildStreamNode = NULL;
  
  //
  // Locate target stream
  //
  Status = FindStreamNode (SectionStreamHandle, &StreamNode);
  if (EFI_ERROR (Status)) {
    Status = EFI_INVALID_PARAMETER;
    goto GetSection_Done;
  }
  
  //
  // Found the stream, now locate and return the appropriate section
  //
  if (SectionType == NULL) {
    //
    // SectionType == NULL means return the WHOLE section stream...
    //
    CopySize = StreamNode->StreamLength;
    CopyBuffer = StreamNode->StreamBuffer;
    *AuthenticationStatus = StreamNode->AuthenticationStatus;
  } else {
    //
    // There's a requested section type, so go find it and return it...
    //
    Status = FindChildNode (
                      StreamNode, 
                      *SectionType, 
                      &Instance, 
                      SectionDefinitionGuid,
                      &ChildNode,
                      &ChildStreamNode, 
                      &ExtractedAuthenticationStatus
                      );
    if (EFI_ERROR (Status)) {
      goto GetSection_Done;
    }
    ASSERT (ChildNode != NULL);
    ASSERT (ChildStreamNode != NULL);
    Section = (EFI_COMMON_SECTION_HEADER *) (ChildStreamNode->StreamBuffer + ChildNode->OffsetInStream);

    if (IS_SECTION2 (Section)) {
      CopySize = SECTION2_SIZE (Section) - sizeof (EFI_COMMON_SECTION_HEADER2);
      CopyBuffer = (UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER2);
    } else {
      CopySize = SECTION_SIZE (Section) - sizeof (EFI_COMMON_SECTION_HEADER);
      CopyBuffer = (UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER);
    }
    *AuthenticationStatus = ExtractedAuthenticationStatus;
  }   
    
  SectionSize = CopySize;  
  if (*Buffer != NULL) {
    //
    // Caller allocated buffer.  Fill to size and return required size...
    //
    if (*BufferSize < CopySize) {
      Status = EFI_WARN_BUFFER_TOO_SMALL;
      CopySize = *BufferSize;
    }
  } else {
    //
    // Callee allocated buffer.  Allocate buffer and return size.
    //
    *Buffer = AllocatePool (CopySize);
    if (*Buffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto GetSection_Done;
    }
  }
  CopyMem (*Buffer, CopyBuffer, CopySize);
  *BufferSize = SectionSize;
  
GetSection_Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Worker function.  Destructor for child nodes.

  @param ChildNode           Indicates the node to destroy

**/
VOID
FreeChildNode (
  IN  FRAMEWORK_SECTION_CHILD_NODE                   *ChildNode
  )
{
  ASSERT (ChildNode->Signature == FRAMEWORK_SECTION_CHILD_SIGNATURE);
  //
  // Remove the child from it's list
  //
  RemoveEntryList (&ChildNode->Link);
  
  if (ChildNode->EncapsulatedStreamHandle != NULL_STREAM_HANDLE) {
    //
    // If it's an encapsulating section, we close the resulting section stream.
    // CloseSectionStream will free all memory associated with the stream.
    //
    CloseSectionStream (&mSectionExtraction, ChildNode->EncapsulatedStreamHandle);
  }

  if (ChildNode->Event != NULL) {
    gBS->CloseEvent (ChildNode->Event);
  }

  //
  // Last, free the child node itself
  //
  FreePool (ChildNode);
}  

/**
  SEP member function.  Deletes an existing section stream

  @param This                   Indicates the calling context.
  @param StreamHandleToClose    Indicates the stream to close

  @retval EFI_SUCCESS           Section stream was closed successfully.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_INVALID_PARAMETER Section stream does not end concident with end of
                                last section.

**/
EFI_STATUS
EFIAPI
CloseSectionStream (
  IN  EFI_SECTION_EXTRACTION_PROTOCOL           *This,
  IN  UINTN                                     StreamHandleToClose
  )
{
  FRAMEWORK_SECTION_STREAM_NODE                      *StreamNode;
  EFI_TPL                                       OldTpl;
  EFI_STATUS                                    Status;
  LIST_ENTRY                                    *Link;
  FRAMEWORK_SECTION_CHILD_NODE                       *ChildNode;
  
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  //
  // Locate target stream
  //
  Status = FindStreamNode (StreamHandleToClose, &StreamNode);
  if (!EFI_ERROR (Status)) {
    //
    // Found the stream, so close it
    //
    RemoveEntryList (&StreamNode->Link);
    while (!IsListEmpty (&StreamNode->Children)) {
      Link = GetFirstNode (&StreamNode->Children);
      ChildNode = CHILD_SECTION_NODE_FROM_LINK (Link);
      FreeChildNode (ChildNode);
    }
    FreePool (StreamNode->StreamBuffer);
    FreePool (StreamNode);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  gBS->RestoreTPL (OldTpl);
  return Status;
}
