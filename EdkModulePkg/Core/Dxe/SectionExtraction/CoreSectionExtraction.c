/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CoreSectionExtraction.c
  
Abstract:

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
  
--*/

#include <DxeMain.h>

//
// Local defines and typedefs
//
#define CORE_SECTION_CHILD_SIGNATURE  EFI_SIGNATURE_32('S','X','C','S')
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

#define CORE_SECTION_STREAM_SIGNATURE EFI_SIGNATURE_32('S','X','S','S')
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
  
  

//
// Local prototypes
//

STATIC
BOOLEAN
ChildIsType (
  IN CORE_SECTION_STREAM_NODE                 *Stream,
  IN CORE_SECTION_CHILD_NODE                  *Child,
  IN EFI_SECTION_TYPE                         SearchType,
  IN EFI_GUID                                 *SectionDefinitionGuid
  );

STATIC
VOID
EFIAPI
NotifyGuidedExtraction (
  IN  EFI_EVENT                             Event,
  IN  VOID                                  *RpnContext
  );

STATIC
VOID
CreateGuidedExtractionRpnEvent (
  IN CORE_SECTION_STREAM_NODE                 *ParentStream,
  IN CORE_SECTION_CHILD_NODE                  *ChildNode
  );

STATIC
EFI_STATUS
EFIAPI
OpenSectionStream (
  IN     EFI_SECTION_EXTRACTION_PROTOCOL      *This,
  IN     UINTN                                SectionStreamLength,
  IN     VOID                                 *SectionStream,
     OUT UINTN                                *SectionStreamHandle
  );
  
STATIC
EFI_STATUS
EFIAPI
GetSection (
  IN EFI_SECTION_EXTRACTION_PROTOCOL          *This,
  IN UINTN                                    SectionStreamHandle,
  IN EFI_SECTION_TYPE                         *SectionType,
  IN EFI_GUID                                 *SectionDefinitionGuid,
  IN UINTN                                    SectionInstance,
  IN VOID                                     **Buffer,
  IN OUT UINTN                                *BufferSize,
  OUT UINT32                                  *AuthenticationStatus
  );
  
STATIC
EFI_STATUS
EFIAPI
CloseSectionStream (
  IN  EFI_SECTION_EXTRACTION_PROTOCOL         *This,
  IN  UINTN                                   StreamHandleToClose
  );
  
STATIC
EFI_STATUS
FindStreamNode (
  IN  UINTN                                   SearchHandle,
  OUT CORE_SECTION_STREAM_NODE                **FoundStream
  );
  
STATIC
EFI_STATUS
FindChildNode (
  IN     CORE_SECTION_STREAM_NODE             *SourceStream,
  IN     EFI_SECTION_TYPE                     SearchType,
  IN     UINTN                                *SectionInstance,
  IN     EFI_GUID                             *SectionDefinitionGuid,
  OUT    CORE_SECTION_CHILD_NODE              **FoundChild,
  OUT    CORE_SECTION_STREAM_NODE             **FoundStream,
  OUT    UINT32                               *AuthenticationStatus
  );
  
STATIC
EFI_STATUS
CreateChildNode (
  IN     CORE_SECTION_STREAM_NODE             *Stream,
  IN     UINT32                               ChildOffset,
  OUT    CORE_SECTION_CHILD_NODE              **ChildNode
  );
  
STATIC
VOID
FreeChildNode (
  IN  CORE_SECTION_CHILD_NODE                 *ChildNode
  );
  
STATIC
EFI_STATUS
OpenSectionStreamEx (
  IN     UINTN                                SectionStreamLength,
  IN     VOID                                 *SectionStream,
  IN     BOOLEAN                              AllocateBuffer,
  IN     UINT32                               AuthenticationStatus,   
     OUT UINTN                                *SectionStreamHandle
  );
  
STATIC
BOOLEAN
IsValidSectionStream (
  IN  VOID                                    *SectionStream,
  IN  UINTN                                   SectionStreamLength
  );
  
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

                                             
EFI_STATUS
EFIAPI
InitializeSectionExtraction (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
/*++

Routine Description: 
  Entry point of the section extraction code. Initializes an instance of the 
  section extraction interface and installs it on a new handle.

Arguments:  
  ImageHandle   EFI_HANDLE: A handle for the image that is initializing this driver
  SystemTable   EFI_SYSTEM_TABLE: A pointer to the EFI system table        

Returns:  
  EFI_SUCCESS:  Driver initialized successfully
  EFI_OUT_OF_RESOURCES:   Could not allocate needed resources

--*/
{
  EFI_STATUS                         Status;

  //
  // Install SEP to a new handle
  //
  Status = CoreInstallProtocolInterface (
            &mSectionExtractionHandle,
            &gEfiSectionExtractionProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &mSectionExtraction
            );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OpenSectionStream (
  IN     EFI_SECTION_EXTRACTION_PROTOCOL           *This,
  IN     UINTN                                     SectionStreamLength,
  IN     VOID                                      *SectionStream,
     OUT UINTN                                     *SectionStreamHandle
  )
/*++

Routine Description:
  SEP member function.  This function creates and returns a new section stream
  handle to represent the new section stream.

Arguments:
  This                - Indicates the calling context.
  SectionStreamLength - Size in bytes of the section stream.
  SectionStream       - Buffer containing the new section stream.
  SectionStreamHandle - A pointer to a caller allocated UINTN that on output
                        contains the new section stream handle.

Returns:
  EFI_SUCCESS
  EFI_OUT_OF_RESOURCES - memory allocation failed.
  EFI_INVALID_PARAMETER - section stream does not end concident with end of
                          last section.

--*/
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
  
STATIC
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
/*++

Routine Description:
  SEP member function.  Retrieves requested section from section stream.

Arguments:  
  This:                 Pointer to SEP instance.
  SectionStreamHandle:  The section stream from which to extract the requested
                          section.
  SectionType:         A pointer to the type of section to search for.
  SectionDefinitionGuid: If the section type is EFI_SECTION_GUID_DEFINED, then
                        SectionDefinitionGuid indicates which of these types
                          of sections to search for.
  SectionInstance:      Indicates which instance of the requested section to
                          return.
  Buffer:               Double indirection to buffer.  If *Buffer is non-null on
                          input, then the buffer is caller allocated.  If
                          *Buffer is NULL, then the buffer is callee allocated.
                          In either case, the requried buffer size is returned
                          in *BufferSize.
  BufferSize:           On input, indicates the size of *Buffer if *Buffer is
                          non-null on input.  On output, indicates the required
                          size (allocated size if callee allocated) of *Buffer.
  AuthenticationStatus: Indicates the authentication status of the retrieved
                          section.

Returns:  
  EFI_SUCCESS:        Section was retrieved successfully
  EFI_PROTOCOL_ERROR: A GUID defined section was encountered in the section 
                        stream with its EFI_GUIDED_SECTION_PROCESSING_REQUIRED
                        bit set, but there was no corresponding GUIDed Section 
                        Extraction Protocol in the handle database.  *Buffer is 
                        unmodified.
  EFI_NOT_FOUND:      An error was encountered when parsing the SectionStream.
                        This indicates the SectionStream  is not correctly 
                        formatted.
  EFI_NOT_FOUND:      The requested section does not exist.
  EFI_OUT_OF_RESOURCES: The system has insufficient resources to process the 
                        request.
  EFI_INVALID_PARAMETER: The SectionStreamHandle does not exist.
  EFI_WARN_TOO_SMALL: The size of the caller allocated input buffer is 
                        insufficient to contain the requested section.  The 
                        input buffer is filled and contents are section contents
                        are truncated.

--*/
{
  CORE_SECTION_STREAM_NODE                              *StreamNode;
  EFI_TPL                                               OldTpl;
  EFI_STATUS                                            Status;
  CORE_SECTION_CHILD_NODE                               *ChildNode;
  CORE_SECTION_STREAM_NODE                              *ChildStreamNode;
  UINTN                                                 CopySize;
  UINT32                                                ExtractedAuthenticationStatus;
  UINTN                                                 Instance;
  UINT8                                                 *CopyBuffer;
  UINTN                                                 SectionSize;
  

  OldTpl = CoreRaiseTpl (TPL_NOTIFY);
  Instance = SectionInstance + 1;
  
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
    CopySize = ChildNode->Size - sizeof (EFI_COMMON_SECTION_HEADER);
    CopyBuffer = ChildStreamNode->StreamBuffer + ChildNode->OffsetInStream + sizeof (EFI_COMMON_SECTION_HEADER);
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
    *Buffer = CoreAllocateBootServicesPool (CopySize);
    if (*Buffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto GetSection_Done;
    }
  }
  CopyMem (*Buffer, CopyBuffer, CopySize);
  *BufferSize = SectionSize;
  
GetSection_Done:
  CoreRestoreTpl (OldTpl);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
CloseSectionStream (
  IN  EFI_SECTION_EXTRACTION_PROTOCOL           *This,
  IN  UINTN                                     StreamHandleToClose
  )
/*++

Routine Description:
  SEP member function.  Deletes an existing section stream

Arguments:
  This                - Indicates the calling context.
  StreamHandleToClose - Indicates the stream to close

Returns:
  EFI_SUCCESS
  EFI_OUT_OF_RESOURCES - memory allocation failed.
  EFI_INVALID_PARAMETER - section stream does not end concident with end of
                          last section.

--*/
{
  CORE_SECTION_STREAM_NODE                      *StreamNode;
  EFI_TPL                                       OldTpl;
  EFI_STATUS                                    Status;
  LIST_ENTRY                                    *Link;
  CORE_SECTION_CHILD_NODE                       *ChildNode;
  
  OldTpl = CoreRaiseTpl (TPL_NOTIFY);
  
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
    CoreFreePool (StreamNode->StreamBuffer);
    CoreFreePool (StreamNode);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }
  
  CoreRestoreTpl (OldTpl);
  return Status;
}


STATIC
BOOLEAN
ChildIsType (
  IN CORE_SECTION_STREAM_NODE *Stream,
  IN CORE_SECTION_CHILD_NODE  *Child,
  IN EFI_SECTION_TYPE         SearchType,
  IN EFI_GUID                 *SectionDefinitionGuid
  )
/*++

Routine Description:
  Worker function.  Determine if the input stream:child matches the input type.

Arguments:
  Stream              - Indicates the section stream associated with the child
  Child               - Indicates the child to check
  SearchType          - Indicates the type of section to check against for
  SectionDefinitionGuid - Indicates the GUID to check against if the type is
                        EFI_SECTION_GUID_DEFINED
Returns:
  TRUE                - The child matches
  FALSE               - The child doesn't match

--*/
{
  EFI_GUID_DEFINED_SECTION    *GuidedSection;
  
  if (SearchType == EFI_SECTION_ALL) {
    return TRUE;
  }
  if (Child->Type != SearchType) {
    return FALSE;
  }
  if (SearchType != EFI_SECTION_GUID_DEFINED) {
    return TRUE;
  }
  GuidedSection = (EFI_GUID_DEFINED_SECTION * )(Stream->StreamBuffer + Child->OffsetInStream);
  return CompareGuid (&GuidedSection->SectionDefinitionGuid, SectionDefinitionGuid);
}


STATIC
EFI_STATUS
FindChildNode (
  IN     CORE_SECTION_STREAM_NODE                   *SourceStream,
  IN     EFI_SECTION_TYPE                           SearchType,
  IN OUT UINTN                                      *SectionInstance,
  IN     EFI_GUID                                   *SectionDefinitionGuid,
  OUT    CORE_SECTION_CHILD_NODE                    **FoundChild,
  OUT    CORE_SECTION_STREAM_NODE                   **FoundStream,
  OUT    UINT32                                     *AuthenticationStatus
  )
/*++

Routine Description:
  Worker function  Recursively searches / builds section stream database
  looking for requested section.

Arguments:
  SourceStream        - Indicates the section stream in which to do the search.
  SearchType          - Indicates the type of section to search for.
  SectionInstance     - Indicates which instance of section to find.  This is
                        an in/out parameter to deal with recursions.
  SectionDefinitionGuid  - Guid of section definition
  FoundChild          - Output indicating the child node that is found.
  FoundStream         - Output indicating which section stream the child was
                        found in.  If this stream was generated as a result of
                        an encapsulation section, the streamhandle is visible
                        within the SEP driver only.
  AuthenticationStatus- Indicates the authentication status of the found section.

Returns:
  EFI_SUCCESS         - Child node was found and returned.
  EFI_OUT_OF_RESOURCES- Memory allocation failed.
  EFI_NOT_FOUND       - Requested child node does not exist.
  EFI_PROTOCOL_ERROR  - a required GUIDED section extraction protocol does not
                        exist

--*/
{
  CORE_SECTION_CHILD_NODE                       *CurrentChildNode;
  CORE_SECTION_CHILD_NODE                       *RecursedChildNode;
  CORE_SECTION_STREAM_NODE                      *RecursedFoundStream;
  UINT32                                        NextChildOffset;
  EFI_STATUS                                    ErrorStatus;
  EFI_STATUS                                    Status;
  
  CurrentChildNode = NULL;
  ErrorStatus = EFI_NOT_FOUND;
  
  if (SourceStream->StreamLength == 0) {
    return EFI_NOT_FOUND;
  }
  
  if (IsListEmpty (&SourceStream->Children) && 
                   SourceStream->StreamLength > sizeof (EFI_COMMON_SECTION_HEADER)) {
    //
    // This occurs when a section stream exists, but no child sections
    // have been parsed out yet.  Therefore, extract the first child and add it
    // to the list of children so we can get started.
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
                (CORE_SECTION_STREAM_NODE *)CurrentChildNode->EncapsulatedStreamHandle,
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


STATIC
EFI_STATUS
CreateChildNode (
  IN     CORE_SECTION_STREAM_NODE              *Stream,
  IN     UINT32                                ChildOffset,
     OUT CORE_SECTION_CHILD_NODE               **ChildNode
  )
/*++

Routine Description:
  Worker function.  Constructor for new child nodes.

Arguments:
  Stream              - Indicates the section stream in which to add the child.
  ChildOffset         - Indicates the offset in Stream that is the beginning
                        of the child section.
  ChildNode           - Indicates the Callee allocated and initialized child.

Returns:
  EFI_SUCCESS         - Child node was found and returned.
  EFI_OUT_OF_RESOURCES- Memory allocation failed.
  EFI_PROTOCOL_ERROR  - Encapsulation sections produce new stream handles when
                        the child node is created.  If the section type is GUID
                        defined, and the extraction GUID does not exist, and
                        producing the stream requires the GUID, then a protocol
                        error is generated and no child is produced.
  Values returned by OpenSectionStreamEx.

--*/
{
  EFI_STATUS                                   Status;
  EFI_COMMON_SECTION_HEADER                    *SectionHeader;
  EFI_COMPRESSION_SECTION                      *CompressionHeader;
  EFI_GUID_DEFINED_SECTION                     *GuidedHeader;
  EFI_TIANO_DECOMPRESS_PROTOCOL                *Decompress;
  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL       *GuidedExtraction;
  VOID                                         *NewStreamBuffer;
  VOID                                         *ScratchBuffer;
  UINT32                                       ScratchSize;
  UINTN                                        NewStreamBufferSize;
  UINT32                                       AuthenticationStatus;
  UINT32                                       SectionLength;
    
  CORE_SECTION_CHILD_NODE                      *Node;

  SectionHeader = (EFI_COMMON_SECTION_HEADER *) (Stream->StreamBuffer + ChildOffset);

  //
  // Allocate a new node
  //
  *ChildNode = CoreAllocateBootServicesPool (sizeof (CORE_SECTION_CHILD_NODE));
  Node = *ChildNode;
  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Now initialize it
  //
  Node->Signature = CORE_SECTION_CHILD_SIGNATURE;
  Node->Type = SectionHeader->Type;
  Node->Size = SECTION_SIZE (SectionHeader);
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
      ASSERT (Node->Size >= sizeof (EFI_COMPRESSION_SECTION));
      
      CompressionHeader = (EFI_COMPRESSION_SECTION *) SectionHeader;
      
      //
      // Allocate space for the new stream
      //
      if (CompressionHeader->UncompressedLength > 0) {
        NewStreamBufferSize = CompressionHeader->UncompressedLength;
        NewStreamBuffer = CoreAllocateBootServicesPool (NewStreamBufferSize);
        if (NewStreamBuffer == NULL) {
          CoreFreePool (Node);
          return EFI_OUT_OF_RESOURCES;
        }
        
        if (CompressionHeader->CompressionType == EFI_NOT_COMPRESSED) {
          //
          // stream is not actually compressed, just encapsulated.  So just copy it.
          //
          CopyMem (NewStreamBuffer, CompressionHeader + 1, NewStreamBufferSize);
        } else if (CompressionHeader->CompressionType == EFI_STANDARD_COMPRESSION ||
                   CompressionHeader->CompressionType == EFI_CUSTOMIZED_COMPRESSION) {
          //
          // Decompress the stream
          //
          if (CompressionHeader->CompressionType == EFI_STANDARD_COMPRESSION) {
            Status = CoreLocateProtocol (&gEfiDecompressProtocolGuid, NULL, (VOID **)&Decompress);
          } else {
            Status = CoreLocateProtocol (&gEfiCustomizedDecompressProtocolGuid, NULL, (VOID **)&Decompress);
          }
          
          ASSERT_EFI_ERROR (Status);
          
          Status = Decompress->GetInfo (
                                 Decompress,
                                 CompressionHeader + 1,
                                 Node->Size - sizeof (EFI_COMPRESSION_SECTION),
                                 (UINT32 *)&NewStreamBufferSize,
                                 &ScratchSize
                                 );
          ASSERT_EFI_ERROR (Status);
          ASSERT (NewStreamBufferSize == CompressionHeader->UncompressedLength);

          ScratchBuffer = CoreAllocateBootServicesPool (ScratchSize);
          if (ScratchBuffer == NULL) {
            CoreFreePool (Node);
            CoreFreePool (NewStreamBuffer);
            return EFI_OUT_OF_RESOURCES;
          }

          Status = Decompress->Decompress (
                                 Decompress,
                                 CompressionHeader + 1,
                                 Node->Size - sizeof (EFI_COMPRESSION_SECTION),
                                 NewStreamBuffer,
                                 (UINT32)NewStreamBufferSize,
                                 ScratchBuffer,
                                 ScratchSize
                                 );
          ASSERT_EFI_ERROR (Status);
          CoreFreePool (ScratchBuffer);                                           
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
        CoreFreePool (Node);
        CoreFreePool (NewStreamBuffer);
        return Status;
      }
      break;

    case EFI_SECTION_GUID_DEFINED:
      GuidedHeader = (EFI_GUID_DEFINED_SECTION *) SectionHeader;
      Node->EncapsulationGuid = &GuidedHeader->SectionDefinitionGuid;
      Status = CoreLocateProtocol (Node->EncapsulationGuid, NULL, (VOID **)&GuidedExtraction);
      if (!EFI_ERROR (Status)) {
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
          CoreFreePool (*ChildNode);
          return EFI_PROTOCOL_ERROR;
        }
        
        //
        // Make sure we initialize the new stream with the correct 
        // authentication status for both aggregate and local status fields.
        //
        if (GuidedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
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
          CoreFreePool (*ChildNode);
          CoreFreePool (NewStreamBuffer);
          return Status;
        }
      } else {
        //
        // There's no GUIDed section extraction protocol available.
        //
        if (GuidedHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
          //
          // If the section REQUIRES an extraction protocol, then we're toast
          //
          CoreFreePool (*ChildNode);
          return EFI_PROTOCOL_ERROR;
        }
        
        //
        // Figure out the proper authentication status
        //
        AuthenticationStatus = Stream->AuthenticationStatus;
        if (GuidedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
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
        
        SectionLength = SECTION_SIZE (GuidedHeader);
        Status = OpenSectionStreamEx (
                   SectionLength - GuidedHeader->DataOffset,
                   (UINT8 *) GuidedHeader + GuidedHeader->DataOffset,
                   TRUE,
                   AuthenticationStatus,
                   &Node->EncapsulatedStreamHandle
                   );
        if (EFI_ERROR (Status)) {
          CoreFreePool (Node);
          return Status;
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


STATIC
VOID
CreateGuidedExtractionRpnEvent (
  IN CORE_SECTION_STREAM_NODE       *ParentStream,
  IN CORE_SECTION_CHILD_NODE        *ChildNode
  )
/*++

Routine Description:
  Worker function.  Constructor for RPN event if needed to keep AuthenticationStatus
  cache correct when a missing GUIDED_SECTION_EXTRACTION_PROTOCOL appears...

Arguments:
  ParentStream        - Indicates the parent of the ecnapsulation section (child)
  ChildNode           - Indicates the child node that is the encapsulation section.

Returns:
  None

--*/
{
  RPN_EVENT_CONTEXT *Context;
  
  //
  // Allocate new event structure and context
  //
  Context = CoreAllocateBootServicesPool (sizeof (RPN_EVENT_CONTEXT));
  ASSERT (Context != NULL);
  
  Context->ChildNode = ChildNode;
  Context->ParentStream = ParentStream;
 
  Context->Event = CoreCreateProtocolNotifyEvent (
                    Context->ChildNode->EncapsulationGuid,
                    TPL_NOTIFY,
                    NotifyGuidedExtraction,
                    Context,
                    &Context->Registration,
                    FALSE
                    );
}
  
  
STATIC
VOID
EFIAPI
NotifyGuidedExtraction (
  IN   EFI_EVENT   Event,
  IN   VOID        *RpnContext
  )
/*++

Routine Description:
  RPN callback function.  Removes a stale section stream and re-initializes it
  with an updated AuthenticationStatus.

Arguments:
  Event               - The event that fired
  RpnContext          - A pointer to the context that allows us to identify
                        the relevent encapsulation...

Returns:
  None

--*/
{
  EFI_STATUS                              Status;
  EFI_GUID_DEFINED_SECTION                *GuidedHeader;
  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL  *GuidedExtraction;
  VOID                                    *NewStreamBuffer;
  UINTN                                   NewStreamBufferSize;
  UINT32                                  AuthenticationStatus;
  RPN_EVENT_CONTEXT                       *Context;
  
  Context = RpnContext;
  
  Status = CloseSectionStream (&mSectionExtraction, Context->ChildNode->EncapsulatedStreamHandle);
  if (!EFI_ERROR (Status)) {
    //
    // The stream closed successfully, so re-open the stream with correct AuthenticationStatus
    //
  
    GuidedHeader = (EFI_GUID_DEFINED_SECTION *) 
      (Context->ParentStream->StreamBuffer + Context->ChildNode->OffsetInStream);
    ASSERT (GuidedHeader->CommonHeader.Type == EFI_SECTION_GUID_DEFINED);
    
    Status = CoreLocateProtocol (Context->ChildNode->EncapsulationGuid, NULL, (VOID **)&GuidedExtraction);
    ASSERT_EFI_ERROR (Status);

    
    Status = GuidedExtraction->ExtractSection (
                                 GuidedExtraction,
                                 GuidedHeader,
                                 &NewStreamBuffer,
                                 &NewStreamBufferSize,
                                 &AuthenticationStatus
                                 );
    ASSERT_EFI_ERROR (Status);
    //
    // OR in the parent stream's aggregagate status.
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
  //  If above, the stream  did not close successfully, it indicates it's
  //  alread been closed by someone, so just destroy the event and be done with
  //  it.
  //
  
  CoreCloseEvent (Event);
  CoreFreePool (Context);
}  
  

STATIC
VOID
FreeChildNode (
  IN  CORE_SECTION_CHILD_NODE                   *ChildNode
  )
/*++

Routine Description:
  Worker function.  Destructor for child nodes.

Arguments:
  ChildNode           - Indicates the node to destroy

Returns:
  none

--*/
{
  ASSERT (ChildNode->Signature == CORE_SECTION_CHILD_SIGNATURE);
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
  //
  // Last, free the child node itself
  //
  CoreFreePool (ChildNode);
}  


STATIC
EFI_STATUS
OpenSectionStreamEx (
  IN     UINTN                                     SectionStreamLength,
  IN     VOID                                      *SectionStream,
  IN     BOOLEAN                                   AllocateBuffer,
  IN     UINT32                                    AuthenticationStatus,   
     OUT UINTN                                     *SectionStreamHandle
  )
/*++

  Routine Description:
    Worker function.  Constructor for section streams.

  Arguments:
    SectionStreamLength - Size in bytes of the section stream.
    SectionStream       - Buffer containing the new section stream.
    AllocateBuffer      - Indicates whether the stream buffer is to be copied
                          or the input buffer is to be used in place.
    AuthenticationStatus- Indicates the default authentication status for the
                          new stream.
    SectionStreamHandle - A pointer to a caller allocated section stream handle.

  Returns:
    EFI_SUCCESS         - Stream was added to stream database.
    EFI_OUT_OF_RESOURCES - memory allocation failed.

--*/
{
  CORE_SECTION_STREAM_NODE    *NewStream;
  EFI_TPL                     OldTpl;
  
  //
  // Allocate a new stream
  //
  NewStream = CoreAllocateBootServicesPool (sizeof (CORE_SECTION_STREAM_NODE));
  if (NewStream == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  if (AllocateBuffer) { 
    //
    // if we're here, we're double buffering, allocate the buffer and copy the
    // data in
    //
    if (SectionStreamLength > 0) {
      NewStream->StreamBuffer = CoreAllocateBootServicesPool (SectionStreamLength); 
      if (NewStream->StreamBuffer == NULL) {
        CoreFreePool (NewStream);
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
  NewStream->Signature = CORE_SECTION_STREAM_SIGNATURE;
  NewStream->StreamHandle = (UINTN) NewStream;
  NewStream->StreamLength = SectionStreamLength;
  InitializeListHead (&NewStream->Children);
  NewStream->AuthenticationStatus = AuthenticationStatus;
  
  //
  // Add new stream to stream list
  //
  OldTpl = CoreRaiseTpl (TPL_NOTIFY);
  InsertTailList (&mStreamRoot, &NewStream->Link);
  CoreRestoreTpl (OldTpl);

  *SectionStreamHandle = NewStream->StreamHandle;
  
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FindStreamNode (
  IN  UINTN                                     SearchHandle,
  OUT CORE_SECTION_STREAM_NODE                  **FoundStream
  )
/*++

  Routine Description:
    Worker function.  Search stream database for requested stream handle.

  Arguments:
    SearchHandle        - Indicates which stream to look for.
    FoundStream         - Output pointer to the found stream.

  Returns:
    EFI_SUCCESS         - StreamHandle was found and *FoundStream contains
                          the stream node.
    EFI_NOT_FOUND       - SearchHandle was not found in the stream database.

--*/
{  
  CORE_SECTION_STREAM_NODE                      *StreamNode;
  
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


STATIC
BOOLEAN
IsValidSectionStream (
  IN  VOID              *SectionStream,
  IN  UINTN             SectionStreamLength
  )
/*++

Routine Description:
  Check if a stream is valid.

Arguments:
  SectionStream         - The section stream to be checked
  SectionStreamLength   - The length of section stream

Returns:
  TRUE
  FALSE

--*/
{
  UINTN                       TotalLength;
  UINTN                       SectionLength;
  EFI_COMMON_SECTION_HEADER   *SectionHeader;
  EFI_COMMON_SECTION_HEADER   *NextSectionHeader;

  TotalLength = 0;
  SectionHeader = (EFI_COMMON_SECTION_HEADER *)SectionStream;
  
  while (TotalLength < SectionStreamLength) {
    SectionLength = SECTION_SIZE (SectionHeader);
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
    NextSectionHeader = (EFI_COMMON_SECTION_HEADER *) ((UINTN) SectionHeader + 3);
    NextSectionHeader = (EFI_COMMON_SECTION_HEADER *) ((UINTN) NextSectionHeader & ~(UINTN)3);
    TotalLength += (UINTN) NextSectionHeader - (UINTN) SectionHeader;
    SectionHeader = NextSectionHeader;
  }

  ASSERT (FALSE);
  return FALSE;
}
