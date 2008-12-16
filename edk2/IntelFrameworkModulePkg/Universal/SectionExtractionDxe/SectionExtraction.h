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

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SECION_EXTRACTION_H_
#define _SECION_EXTRACTION_H_

#include <FrameworkDxe.h>

#include <Protocol/SectionExtraction.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/Decompress.h>
#include <Protocol/GuidedSectionExtraction.h>

//
// Local defines and typedefs
//
#define CORE_SECTION_CHILD_SIGNATURE  SIGNATURE_32('S','X','C','S')
#define CHILD_SECTION_NODE_FROM_LINK(Node) \
  CR (Node, CORE_SECTION_CHILD_NODE, Link, CORE_SECTION_CHILD_SIGNATURE)

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
} CORE_SECTION_CHILD_NODE;

#define CORE_SECTION_STREAM_SIGNATURE SIGNATURE_32('S','X','S','S')
#define STREAM_NODE_FROM_LINK(Node) \
  CR (Node, CORE_SECTION_STREAM_NODE, Link, CORE_SECTION_STREAM_SIGNATURE)

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
} CORE_SECTION_STREAM_NODE;

#define NULL_STREAM_HANDLE    0

typedef struct {
  CORE_SECTION_CHILD_NODE     *ChildNode;
  CORE_SECTION_STREAM_NODE    *ParentStream;
  VOID                        *Registration;
  EFI_EVENT                   Event;
} RPN_EVENT_CONTEXT;
  
  
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
CoreCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration,
  IN  BOOLEAN             SignalFlag
  );

//
// Local prototypes
//

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
  IN CORE_SECTION_STREAM_NODE *Stream,
  IN CORE_SECTION_CHILD_NODE  *Child,
  IN EFI_SECTION_TYPE         SearchType,
  IN EFI_GUID                 *SectionDefinitionGuid
  );

/**
  RPN callback function.  Removes a stale section stream and re-initializes it
  with an updated AuthenticationStatus.

  @param Event               The event that fired
  @param RpnContext          A pointer to the context that allows us to identify
                             the relevent encapsulation.

**/
VOID
EFIAPI
NotifyGuidedExtraction (
  IN   EFI_EVENT   Event,
  IN   VOID        *RpnContext
  );

/**
  Worker function.  Constructor for RPN event if needed to keep AuthenticationStatus
  cache correct when a missing GUIDED_SECTION_EXTRACTION_PROTOCOL appears...

  @param ParentStream        Indicates the parent of the ecnapsulation section (child)
  @param ChildNode           Indicates the child node that is the encapsulation section.

**/
VOID
CreateGuidedExtractionRpnEvent (
  IN CORE_SECTION_STREAM_NODE       *ParentStream,
  IN CORE_SECTION_CHILD_NODE        *ChildNode
  );

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
  );
  
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
  );
  
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
  );
  
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
  OUT CORE_SECTION_STREAM_NODE                  **FoundStream
  );
  
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
  IN     CORE_SECTION_STREAM_NODE                   *SourceStream,
  IN     EFI_SECTION_TYPE                           SearchType,
  IN OUT UINTN                                      *SectionInstance,
  IN     EFI_GUID                                   *SectionDefinitionGuid,
  OUT    CORE_SECTION_CHILD_NODE                    **FoundChild,
  OUT    CORE_SECTION_STREAM_NODE                   **FoundStream,
  OUT    UINT32                                     *AuthenticationStatus
  );
  
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
  IN     CORE_SECTION_STREAM_NODE              *Stream,
  IN     UINT32                                ChildOffset,
     OUT CORE_SECTION_CHILD_NODE               **ChildNode
  );
  
/**
  Worker function.  Destructor for child nodes.

  @param ChildNode           Indicates the node to destroy

**/
VOID
FreeChildNode (
  IN  CORE_SECTION_CHILD_NODE                   *ChildNode
  );
  
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
  );
  
/**

  Check if a stream is valid.

  @param SectionStream         The section stream to be checked
  @param SectionStreamLength   The length of section stream

  @return The validness of a stream.

**/
BOOLEAN
IsValidSectionStream (
  IN  VOID              *SectionStream,
  IN  UINTN             SectionStreamLength
  );

#endif  // _SECTION_EXTRACTION_H_
