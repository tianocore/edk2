/** @file

  The definition of CFormPkg's member function

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "stdio.h"
#include "assert.h"
#include "VfrFormPkg.h"

/*
 * The definition of CFormPkg's member function
 */

SPendingAssign::SPendingAssign (
  IN CHAR8  *Key,
  IN VOID   *Addr,
  IN UINT32 Len,
  IN UINT32 LineNo,
  IN CONST CHAR8  *Msg
  )
{
  mKey    = NULL;
  mAddr   = Addr;
  mLen    = Len;
  mFlag   = PENDING;
  mLineNo = LineNo;
  mMsg    = NULL;
  mNext   = NULL;
  if (Key != NULL) {
    mKey = new CHAR8[strlen (Key) + 1];
    if (mKey != NULL) {
      strcpy (mKey, Key);
    }
  }

  if (Msg != NULL) {
    mMsg = new CHAR8[strlen (Msg) + 1];
    if (mMsg != NULL) {
      strcpy (mMsg, Msg);
    }
  }
}

SPendingAssign::~SPendingAssign (
  VOID
  )
{
  if (mKey != NULL) {
    delete[] mKey;
  }
  mAddr   = NULL;
  mLen    = 0;
  mLineNo = 0;
  if (mMsg != NULL) {
    delete[] mMsg;
  }
  mNext   = NULL;
}

VOID
SPendingAssign::SetAddrAndLen (
  IN VOID   *Addr,
  IN UINT32 LineNo
  )
{
  mAddr   = Addr;
  mLineNo = LineNo;
}

VOID
SPendingAssign::AssignValue (
  IN VOID   *Addr,
  IN UINT32 Len
  )
{
  memmove (mAddr, Addr, (mLen < Len ? mLen : Len));
  mFlag = ASSIGNED;
}

CHAR8 *
SPendingAssign::GetKey (
  VOID
  )
{
  return mKey;
}

CFormPkg::CFormPkg (
  IN UINT32 BufferSize
  )
{
  CHAR8       *BufferStart;
  CHAR8       *BufferEnd;
  SBufferNode *Node;

  mPkgLength           = 0;
  mBufferSize          = 0;
  mBufferNodeQueueHead = NULL;
  mBufferNodeQueueTail = NULL;
  mCurrBufferNode      = NULL;
  mReadBufferNode      = NULL;
  mReadBufferOffset    = 0;
  PendingAssignList    = NULL;

  Node = new SBufferNode;
  if (Node == NULL) {
    return ;
  }
  BufferStart = new CHAR8[BufferSize];
  if (BufferStart == NULL) {
    delete Node;
    return;
  }
  BufferEnd   = BufferStart + BufferSize;

  memset (BufferStart, 0, BufferSize);
  Node->mBufferStart   = BufferStart;
  Node->mBufferEnd     = BufferEnd;
  Node->mBufferFree    = BufferStart;
  Node->mNext          = NULL;

  mBufferSize          = BufferSize;
  mBufferNodeQueueHead = Node;
  mBufferNodeQueueTail = Node;
  mCurrBufferNode      = Node;
}

CFormPkg::~CFormPkg ()
{
  SBufferNode    *pBNode;
  SPendingAssign *pPNode;

  while (mBufferNodeQueueHead != NULL) {
    pBNode = mBufferNodeQueueHead;
    mBufferNodeQueueHead = mBufferNodeQueueHead->mNext;
    if (pBNode->mBufferStart != NULL) {
      delete[] pBNode->mBufferStart;
      delete pBNode;
    }
  }
  mBufferNodeQueueTail = NULL;
  mCurrBufferNode      = NULL;

  while (PendingAssignList != NULL) {
    pPNode = PendingAssignList;
    PendingAssignList = PendingAssignList->mNext;
    delete pPNode;
  }
  PendingAssignList = NULL;
}

SBufferNode *
CFormPkg::CreateNewNode (
  VOID
  )
{
  SBufferNode *Node;

  Node = new SBufferNode;
  if (Node == NULL) {
    return NULL;
  }

  Node->mBufferStart = new CHAR8[mBufferSize];
  if (Node->mBufferStart == NULL) {
    delete Node;
    return NULL;
  } else {
    memset (Node->mBufferStart, 0, mBufferSize);
    Node->mBufferEnd  = Node->mBufferStart + mBufferSize;
    Node->mBufferFree = Node->mBufferStart;
    Node->mNext       = NULL;
  }

  return Node;
}

CHAR8 *
CFormPkg::IfrBinBufferGet (
  IN UINT32 Len
  )
{
  CHAR8       *BinBuffer = NULL;
  SBufferNode *Node      = NULL;

  if ((Len == 0) || (Len > mBufferSize)) {
    return NULL;
  }

  if ((mCurrBufferNode->mBufferFree + Len) <= mCurrBufferNode->mBufferEnd) {
    BinBuffer = mCurrBufferNode->mBufferFree;
    mCurrBufferNode->mBufferFree += Len;
  } else {
    Node = CreateNewNode ();
    if (Node == NULL) {
      return NULL;
    }

    if (mBufferNodeQueueTail == NULL) {
      mBufferNodeQueueHead = mBufferNodeQueueTail = Node;
    } else {
      mBufferNodeQueueTail->mNext = Node;
      mBufferNodeQueueTail = Node;
    }
    mCurrBufferNode = Node;

    //
    // Now try again.
    //
    BinBuffer = mCurrBufferNode->mBufferFree;
    mCurrBufferNode->mBufferFree += Len;
  }

  mPkgLength += Len;

  return BinBuffer;
}

inline
UINT32
CFormPkg::GetPkgLength (
  VOID
  )
{
  return mPkgLength;
}

VOID
CFormPkg::Open (
  VOID
  )
{
  mReadBufferNode   = mBufferNodeQueueHead;
  mReadBufferOffset = 0;
}

VOID
CFormPkg::Close (
  VOID
  )
{
  mReadBufferNode   = NULL;
  mReadBufferOffset = 0;
}

UINT32
CFormPkg::Read (
  IN CHAR8     *Buffer,
  IN UINT32    Size
  )
{
  UINT32       Index;

  if ((Size == 0) || (Buffer == NULL)) {
    return 0;
  }

  if (mReadBufferNode == NULL) {
    return 0;
  }

  for (Index = 0; Index < Size; Index++) {
    if ((mReadBufferNode->mBufferStart + mReadBufferOffset) < mReadBufferNode->mBufferFree) {
      Buffer[Index] = mReadBufferNode->mBufferStart[mReadBufferOffset++];
    } else {
      if ((mReadBufferNode = mReadBufferNode->mNext) == NULL) {
        return Index;
      } else {
        mReadBufferOffset = 0;
        Index --;
      }
    }
  }

  return Size;
}

EFI_VFR_RETURN_CODE
CFormPkg::BuildPkgHdr (
  OUT EFI_HII_PACKAGE_HEADER **PkgHdr
  )
{
  if (PkgHdr == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if (((*PkgHdr) = new EFI_HII_PACKAGE_HEADER) == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  (*PkgHdr)->Type = EFI_HII_PACKAGE_FORM;
  (*PkgHdr)->Length = mPkgLength + sizeof (EFI_HII_PACKAGE_HEADER);

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CFormPkg::BuildPkg (
  OUT PACKAGE_DATA &TBuffer
  )
{

  CHAR8  *Temp;
  UINT32 Size;
  CHAR8  Buffer[1024];

  if (TBuffer.Buffer != NULL) {
    delete TBuffer.Buffer;
  }

  TBuffer.Size = mPkgLength;
  TBuffer.Buffer = NULL;
  if (TBuffer.Size != 0) {
    TBuffer.Buffer = new CHAR8[TBuffer.Size];
  } else {
    return VFR_RETURN_SUCCESS;
  }

  Temp = TBuffer.Buffer;
  Open ();
  while ((Size = Read (Buffer, 1024)) != 0) {
    memcpy (Temp, Buffer, Size);
    Temp += Size;
  }
  Close ();
  return VFR_RETURN_SUCCESS;
}


EFI_VFR_RETURN_CODE
CFormPkg::BuildPkg (
  IN FILE  *Output,
  IN PACKAGE_DATA *PkgData
  )
{
  EFI_VFR_RETURN_CODE     Ret;
  CHAR8                   Buffer[1024];
  UINT32                  Size;
  EFI_HII_PACKAGE_HEADER  *PkgHdr;

  if (Output == NULL) {
    return VFR_RETURN_FATAL_ERROR;
  }

  if ((Ret = BuildPkgHdr(&PkgHdr)) != VFR_RETURN_SUCCESS) {
    return Ret;
  }
  fwrite (PkgHdr, sizeof (EFI_HII_PACKAGE_HEADER), 1, Output);
  delete PkgHdr;

  if (PkgData == NULL) {
    Open ();
    while ((Size = Read (Buffer, 1024)) != 0) {
      fwrite (Buffer, Size, 1, Output);
    }
    Close ();
  } else {
    fwrite (PkgData->Buffer, PkgData->Size, 1, Output);
  }

  return VFR_RETURN_SUCCESS;
}

VOID
CFormPkg::_WRITE_PKG_LINE (
  IN FILE         *pFile,
  IN UINT32       LineBytes,
  IN CONST CHAR8  *LineHeader,
  IN CHAR8        *BlkBuf,
  IN UINT32       BlkSize
  )
{
  UINT32    Index;

  if ((pFile == NULL) || (LineHeader == NULL) || (BlkBuf == NULL)) {
    return;
  }

  for (Index = 0; Index < BlkSize; Index++) {
    if ((Index % LineBytes) == 0) {
      fprintf (pFile, "\n%s", LineHeader);
    }
    fprintf (pFile, "0x%02X,  ", (UINT8)BlkBuf[Index]);
  }
}

VOID
CFormPkg::_WRITE_PKG_END (
  IN FILE         *pFile,
  IN UINT32       LineBytes,
  IN CONST CHAR8  *LineHeader,
  IN CHAR8        *BlkBuf,
  IN UINT32       BlkSize
  )
{
  UINT32    Index;

  if ((BlkSize == 0) || (pFile == NULL) || (LineHeader == NULL) || (BlkBuf == NULL)) {
    return;
  }

  for (Index = 0; Index < BlkSize - 1; Index++) {
    if ((Index % LineBytes) == 0) {
      fprintf (pFile, "\n%s", LineHeader);
    }
    fprintf (pFile, "0x%02X,  ", (UINT8)BlkBuf[Index]);
  }

  if ((Index % LineBytes) == 0) {
    fprintf (pFile, "\n%s", LineHeader);
  }
  fprintf (pFile, "0x%02X\n", (UINT8)BlkBuf[Index]);
}

#define BYTES_PRE_LINE 0x10
UINT32   gAdjustOpcodeOffset = 0;
BOOLEAN  gNeedAdjustOpcode   = FALSE;
UINT32   gAdjustOpcodeLen    = 0;

EFI_VFR_RETURN_CODE
CFormPkg::GenCFile (
  IN CHAR8 *BaseName,
  IN FILE *pFile,
  IN PACKAGE_DATA *PkgData
  )
{
  EFI_VFR_RETURN_CODE          Ret;
  CHAR8                        Buffer[BYTES_PRE_LINE * 8];
  EFI_HII_PACKAGE_HEADER       *PkgHdr;
  UINT32                       PkgLength  = 0;
  UINT32                       ReadSize   = 0;

  if ((BaseName == NULL) || (pFile == NULL)) {
    return VFR_RETURN_FATAL_ERROR;
  }

  fprintf (pFile, "\nunsigned char %sBin[] = {\n", BaseName);

  if ((Ret = BuildPkgHdr(&PkgHdr)) != VFR_RETURN_SUCCESS) {
    return Ret;
  }


  fprintf (pFile, "  // ARRAY LENGTH\n");
  PkgLength = PkgHdr->Length + sizeof (UINT32);
  _WRITE_PKG_LINE(pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)&PkgLength, sizeof (UINT32));

  fprintf (pFile, "\n\n  // PACKAGE HEADER\n");
  _WRITE_PKG_LINE(pFile, BYTES_PRE_LINE, "  ", (CHAR8 *)PkgHdr, sizeof (EFI_HII_PACKAGE_HEADER));
  PkgLength = sizeof (EFI_HII_PACKAGE_HEADER);

  fprintf (pFile, "\n\n  // PACKAGE DATA\n");

  if (PkgData == NULL) {
    Open ();
    while ((ReadSize = Read ((CHAR8 *)Buffer, BYTES_PRE_LINE * 8)) != 0) {
      PkgLength += ReadSize;
      if (PkgLength < PkgHdr->Length) {
        _WRITE_PKG_LINE (pFile, BYTES_PRE_LINE, "  ", Buffer, ReadSize);
      } else {
        _WRITE_PKG_END (pFile, BYTES_PRE_LINE, "  ", Buffer, ReadSize);
      }
    }
    Close ();
  } else {
    if (PkgData->Size % BYTES_PRE_LINE != 0) {
      PkgLength = PkgData->Size - (PkgData->Size % BYTES_PRE_LINE);
      _WRITE_PKG_LINE (pFile, BYTES_PRE_LINE, "  ", PkgData->Buffer, PkgLength);
      _WRITE_PKG_END (pFile, BYTES_PRE_LINE, "  ", PkgData->Buffer + PkgLength, PkgData->Size % BYTES_PRE_LINE);
    } else {
      PkgLength = PkgData->Size - BYTES_PRE_LINE;
      _WRITE_PKG_LINE (pFile, BYTES_PRE_LINE, "  ", PkgData->Buffer, PkgLength);
      _WRITE_PKG_END (pFile, BYTES_PRE_LINE, "  ", PkgData->Buffer + PkgLength, BYTES_PRE_LINE);
    }
  }

  delete PkgHdr;
  fprintf (pFile, "\n};\n");

  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CFormPkg::AssignPending (
  IN CHAR8  *Key,
  IN VOID   *ValAddr,
  IN UINT32 ValLen,
  IN UINT32 LineNo,
  IN CONST CHAR8  *Msg
  )
{
  SPendingAssign *pNew;

  pNew = new SPendingAssign (Key, ValAddr, ValLen, LineNo, Msg);
  if (pNew == NULL) {
    return VFR_RETURN_OUT_FOR_RESOURCES;
  }

  pNew->mNext       = PendingAssignList;
  PendingAssignList = pNew;
  return VFR_RETURN_SUCCESS;
}

VOID
CFormPkg::DoPendingAssign (
  IN CHAR8  *Key,
  IN VOID   *ValAddr,
  IN UINT32 ValLen
  )
{
  SPendingAssign *pNode;

  if ((Key == NULL) || (ValAddr == NULL)) {
    return;
  }

  for (pNode = PendingAssignList; pNode != NULL; pNode = pNode->mNext) {
    if (strcmp (pNode->mKey, Key) == 0) {
      pNode->AssignValue (ValAddr, ValLen);
    }
  }
}

bool
CFormPkg::HavePendingUnassigned (
  VOID
  )
{
  SPendingAssign *pNode;

  for (pNode = PendingAssignList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mFlag == PENDING) {
      return TRUE;
    }
  }

  return FALSE;
}

VOID
CFormPkg::PendingAssignPrintAll (
  VOID
  )
{
  SPendingAssign *pNode;

  for (pNode = PendingAssignList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mFlag == PENDING) {
      gCVfrErrorHandle.PrintMsg (pNode->mLineNo, pNode->mKey, "Error", pNode->mMsg);
    }
  }
}

SBufferNode *
CFormPkg::GetBinBufferNodeForAddr (
  IN CHAR8              *BinBuffAddr
  )
{
  SBufferNode *TmpNode;

  TmpNode = mBufferNodeQueueHead;

  while (TmpNode != NULL) {
    if (TmpNode->mBufferStart <= BinBuffAddr && TmpNode->mBufferFree >= BinBuffAddr) {
      return TmpNode;
    }

    TmpNode = TmpNode->mNext;
  }

  return NULL;
}

SBufferNode *
CFormPkg::GetNodeBefore(
  IN SBufferNode *CurrentNode
  )
{
  SBufferNode *FirstNode   = mBufferNodeQueueHead;
  SBufferNode *LastNode    = mBufferNodeQueueHead;

  while (FirstNode != NULL) {
    if (FirstNode == CurrentNode) {
      break;
    }

    LastNode    = FirstNode;
    FirstNode   = FirstNode->mNext;
  }

  if (FirstNode == NULL) {
    LastNode = NULL;
  }

  return LastNode;
}

EFI_VFR_RETURN_CODE
CFormPkg::InsertNodeBefore(
  IN SBufferNode *CurrentNode,
  IN SBufferNode *NewNode
  )
{
  SBufferNode *LastNode = GetNodeBefore (CurrentNode);

  if (LastNode == NULL) {
    return VFR_RETURN_MISMATCHED;
  }

  NewNode->mNext = LastNode->mNext;
  LastNode->mNext = NewNode;

  return VFR_RETURN_SUCCESS;
}

CHAR8 *
CFormPkg::GetBufAddrBaseOnOffset (
  IN UINT32      Offset
  )
{
  SBufferNode *TmpNode;
  UINT32      TotalBufLen;
  UINT32      CurrentBufLen;

  TotalBufLen = 0;

  for (TmpNode = mBufferNodeQueueHead; TmpNode != NULL; TmpNode = TmpNode->mNext) {
    CurrentBufLen = TmpNode->mBufferFree - TmpNode->mBufferStart;
    if (Offset >= TotalBufLen && Offset < TotalBufLen + CurrentBufLen) {
      return TmpNode->mBufferStart + (Offset - TotalBufLen);
    }

    TotalBufLen += CurrentBufLen;
  }

  return NULL;
}

EFI_VFR_RETURN_CODE
CFormPkg::AdjustDynamicInsertOpcode (
  IN CHAR8              *InserPositionAddr,
  IN CHAR8              *InsertOpcodeAddr,
  IN BOOLEAN            CreateOpcodeAfterParsingVfr
  )
{
  SBufferNode *InserPositionNode;
  SBufferNode *InsertOpcodeNode;
  SBufferNode *NewRestoreNodeBegin;
  SBufferNode *NewRestoreNodeEnd;
  SBufferNode *NewLastEndNode;
  SBufferNode *TmpNode;
  UINT32      NeedRestoreCodeLen;

  NewRestoreNodeEnd = NULL;

  InserPositionNode  = GetBinBufferNodeForAddr(InserPositionAddr);
  InsertOpcodeNode = GetBinBufferNodeForAddr(InsertOpcodeAddr);
  assert (InserPositionNode != NULL);
  assert (InsertOpcodeNode  != NULL);

  if (InserPositionNode == InsertOpcodeNode) {
    //
    // Create New Node to save the restore opcode.
    //
    NeedRestoreCodeLen = InsertOpcodeAddr - InserPositionAddr;
    gAdjustOpcodeLen   = NeedRestoreCodeLen;
    NewRestoreNodeBegin = CreateNewNode ();
    if (NewRestoreNodeBegin == NULL) {
      return VFR_RETURN_OUT_FOR_RESOURCES;
    }
    memcpy (NewRestoreNodeBegin->mBufferFree, InserPositionAddr, NeedRestoreCodeLen);
    NewRestoreNodeBegin->mBufferFree += NeedRestoreCodeLen;

    //
    // Override the restore buffer data.
    //
    memmove (InserPositionAddr, InsertOpcodeAddr, InsertOpcodeNode->mBufferFree - InsertOpcodeAddr);
    InsertOpcodeNode->mBufferFree -= NeedRestoreCodeLen;
    memset (InsertOpcodeNode->mBufferFree, 0, NeedRestoreCodeLen);
  } else {
    //
    // Create New Node to save the restore opcode.
    //
    NeedRestoreCodeLen = InserPositionNode->mBufferFree - InserPositionAddr;
    gAdjustOpcodeLen   = NeedRestoreCodeLen;
    NewRestoreNodeBegin = CreateNewNode ();
    if (NewRestoreNodeBegin == NULL) {
      return VFR_RETURN_OUT_FOR_RESOURCES;
    }
    memcpy (NewRestoreNodeBegin->mBufferFree, InserPositionAddr, NeedRestoreCodeLen);
    NewRestoreNodeBegin->mBufferFree += NeedRestoreCodeLen;
    //
    // Override the restore buffer data.
    //
    InserPositionNode->mBufferFree -= NeedRestoreCodeLen;
    //
    // Link the restore data to new node.
    //
    NewRestoreNodeBegin->mNext = InserPositionNode->mNext;

    //
    // Count the Adjust opcode len.
    //
    TmpNode = InserPositionNode->mNext;
    while (TmpNode != InsertOpcodeNode) {
      gAdjustOpcodeLen += TmpNode->mBufferFree - TmpNode->mBufferStart;
      TmpNode = TmpNode->mNext;
    }

    //
    // Create New Node to save the last node of restore opcode.
    //
    NeedRestoreCodeLen = InsertOpcodeAddr - InsertOpcodeNode->mBufferStart;
    gAdjustOpcodeLen  += NeedRestoreCodeLen;
    if (NeedRestoreCodeLen > 0) {
      NewRestoreNodeEnd = CreateNewNode ();
      if (NewRestoreNodeEnd == NULL) {
        return VFR_RETURN_OUT_FOR_RESOURCES;
      }
      memcpy (NewRestoreNodeEnd->mBufferFree, InsertOpcodeNode->mBufferStart, NeedRestoreCodeLen);
      NewRestoreNodeEnd->mBufferFree += NeedRestoreCodeLen;
      //
      // Override the restore buffer data.
      //
      memmove (InsertOpcodeNode->mBufferStart, InsertOpcodeAddr, InsertOpcodeNode->mBufferFree - InsertOpcodeAddr);
      InsertOpcodeNode->mBufferFree -= InsertOpcodeAddr - InsertOpcodeNode->mBufferStart;

      //
      // Insert the last restore data node.
      //
      TmpNode = GetNodeBefore (InsertOpcodeNode);
      assert (TmpNode != NULL);

      if (TmpNode == InserPositionNode) {
        NewRestoreNodeBegin->mNext = NewRestoreNodeEnd;
      } else {
        TmpNode->mNext = NewRestoreNodeEnd;
      }
      //
      // Connect the dynamic opcode node to the node after InserPositionNode.
      //
      InserPositionNode->mNext = InsertOpcodeNode;
    }
  }

  if (CreateOpcodeAfterParsingVfr) {
    //
    // Th new opcodes were created after Parsing Vfr file,
    // so the content in mBufferNodeQueueTail must be the new created opcodes.
    // So connet the  NewRestoreNodeBegin to the tail and update the tail node.
    //
    mBufferNodeQueueTail->mNext = NewRestoreNodeBegin;
    if (NewRestoreNodeEnd != NULL) {
      mBufferNodeQueueTail = NewRestoreNodeEnd;
    } else {
      mBufferNodeQueueTail = NewRestoreNodeBegin;
    }
  } else {
    if (mBufferNodeQueueTail->mBufferFree - mBufferNodeQueueTail->mBufferStart > 2) {
      //
      // End form set opcode all in the mBufferNodeQueueTail node.
      //
      NewLastEndNode = CreateNewNode ();
      if (NewLastEndNode == NULL) {
        return VFR_RETURN_OUT_FOR_RESOURCES;
      }
      NewLastEndNode->mBufferStart[0] = 0x29;
      NewLastEndNode->mBufferStart[1] = 0x02;
      NewLastEndNode->mBufferFree += 2;

      mBufferNodeQueueTail->mBufferFree -= 2;

      mBufferNodeQueueTail->mNext = NewRestoreNodeBegin;
      if (NewRestoreNodeEnd != NULL) {
        NewRestoreNodeEnd->mNext = NewLastEndNode;
      } else {
        NewRestoreNodeBegin->mNext = NewLastEndNode;
      }

      mBufferNodeQueueTail = NewLastEndNode;
    } else if (mBufferNodeQueueTail->mBufferFree - mBufferNodeQueueTail->mBufferStart == 2) {
      TmpNode = GetNodeBefore(mBufferNodeQueueTail);
      assert (TmpNode != NULL);

      TmpNode->mNext = NewRestoreNodeBegin;
      if (NewRestoreNodeEnd != NULL) {
        NewRestoreNodeEnd->mNext = mBufferNodeQueueTail;
      } else {
        NewRestoreNodeBegin->mNext = mBufferNodeQueueTail;
      }
    }
  }
  mCurrBufferNode = mBufferNodeQueueTail;
  return VFR_RETURN_SUCCESS;
}

EFI_VFR_RETURN_CODE
CFormPkg::DeclarePendingQuestion (
  IN CVfrVarDataTypeDB   &lCVfrVarDataTypeDB,
  IN CVfrDataStorage     &lCVfrDataStorage,
  IN CVfrQuestionDB      &lCVfrQuestionDB,
  IN EFI_GUID            *LocalFormSetGuid,
  IN UINT32              LineNo,
  OUT CHAR8              **InsertOpcodeAddr
  )
{
  SPendingAssign *pNode;
  CHAR8          *VarStr;
  UINT32         ArrayIdx;
  CHAR8          FName[MAX_NAME_LEN];
  CHAR8          *SName;
  CHAR8          *NewStr;
  UINT32         ShrinkSize = 0;
  EFI_VFR_RETURN_CODE  ReturnCode;
  EFI_VFR_VARSTORE_TYPE VarStoreType  = EFI_VFR_VARSTORE_INVALID;
  UINT8    LFlags;
  UINT32   MaxValue;
  CIfrGuid *GuidObj = NULL;

  //
  // Declare all questions as Numeric in DisableIf True
  //
  // DisableIf
  CIfrDisableIf DIObj;
  DIObj.SetLineNo (LineNo);
  *InsertOpcodeAddr = DIObj.GetObjBinAddr<CHAR8>();

  //TrueOpcode
  CIfrTrue TObj (LineNo);

  // Declare Numeric qeustion for each undefined question.
  for (pNode = PendingAssignList; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mFlag == PENDING) {
      EFI_VARSTORE_INFO Info;
      EFI_QUESTION_ID   QId   = EFI_QUESTION_ID_INVALID;
      //
      // Register this question, assume it is normal question, not date or time question
      //
      VarStr = pNode->mKey;
      ReturnCode = lCVfrQuestionDB.RegisterQuestion (NULL, VarStr, QId);
      if (ReturnCode != VFR_RETURN_SUCCESS) {
        gCVfrErrorHandle.HandleError (ReturnCode, pNode->mLineNo, pNode->mKey);
        return ReturnCode;
      }

#ifdef VFREXP_DEBUG
      printf ("Undefined Question name is %s and Id is 0x%x\n", VarStr, QId);
#endif
      //
      // Get Question Info, framework vfr VarName == StructName
      //
      ReturnCode = lCVfrVarDataTypeDB.ExtractFieldNameAndArrary (VarStr, FName, ArrayIdx);
      if (ReturnCode != VFR_RETURN_SUCCESS) {
        gCVfrErrorHandle.PrintMsg (pNode->mLineNo, pNode->mKey, "Error", "Var string is not the valid C variable");
        return ReturnCode;
      }
      //
      // Get VarStoreType
      //
      ReturnCode = lCVfrDataStorage.GetVarStoreId (FName, &Info.mVarStoreId);
      if (ReturnCode != VFR_RETURN_SUCCESS) {
        gCVfrErrorHandle.PrintMsg (pNode->mLineNo, FName, "Error", "Var Store Type is not defined");
        return ReturnCode;
      }
      VarStoreType = lCVfrDataStorage.GetVarStoreType (Info.mVarStoreId);

      if (*VarStr == '\0' && ArrayIdx != INVALID_ARRAY_INDEX) {
        ReturnCode = lCVfrDataStorage.GetNameVarStoreInfo (&Info, ArrayIdx);
      } else {
        if (VarStoreType == EFI_VFR_VARSTORE_EFI) {
          ReturnCode = lCVfrDataStorage.GetEfiVarStoreInfo (&Info);
        } else if (VarStoreType == EFI_VFR_VARSTORE_BUFFER || VarStoreType == EFI_VFR_VARSTORE_BUFFER_BITS) {
          VarStr = pNode->mKey;
          //convert VarStr with store name to VarStr with structure name
          ReturnCode = lCVfrDataStorage.GetBufferVarStoreDataTypeName (Info.mVarStoreId, &SName);
          if (ReturnCode == VFR_RETURN_SUCCESS) {
            NewStr = new CHAR8[strlen (VarStr) + strlen (SName) + 1];
            NewStr[0] = '\0';
            strcpy (NewStr, SName);
            strcat (NewStr, VarStr + strlen (FName));
            ReturnCode = lCVfrVarDataTypeDB.GetDataFieldInfo (NewStr, Info.mInfo.mVarOffset, Info.mVarType, Info.mVarTotalSize, Info.mIsBitVar);
            delete[] NewStr;
          }
        } else {
          ReturnCode = VFR_RETURN_UNSUPPORTED;
        }
      }
      if (ReturnCode != VFR_RETURN_SUCCESS) {
        gCVfrErrorHandle.HandleError (ReturnCode, pNode->mLineNo, pNode->mKey);
        return ReturnCode;
      }
      //
      // If the storage is bit fields, create Guid opcode to wrap the numeric opcode.
      //
      if (Info.mIsBitVar) {
        GuidObj = new CIfrGuid(0);
        GuidObj->SetGuid (&gEdkiiIfrBitVarGuid);
        GuidObj->SetLineNo(LineNo);
      }

      CIfrNumeric CNObj;
      CNObj.SetLineNo (LineNo);
      CNObj.SetPrompt (0x0);
      CNObj.SetHelp (0x0);
      CNObj.SetQuestionId (QId);
      CNObj.SetVarStoreInfo (&Info);

      //
      // Set Min/Max/Step Data and flags for the question with bit fields.Min/Max/Step Data are saved as UINT32 type for bit question.
      //
      if (Info.mIsBitVar) {
        MaxValue = (1 << Info.mVarTotalSize) -1;
        CNObj.SetMinMaxStepData ((UINT32) 0, MaxValue, (UINT32) 0);
        ShrinkSize = 12;
        LFlags = (EDKII_IFR_NUMERIC_SIZE_BIT & Info.mVarTotalSize);
        CNObj.SetFlagsForBitField (0, LFlags);
      } else {
        //
        // Numeric doesn't support BOOLEAN data type.
        // BOOLEAN type has the same data size to UINT8.
        //
        if (Info.mVarType == EFI_IFR_TYPE_BOOLEAN) {
          Info.mVarType = EFI_IFR_TYPE_NUM_SIZE_8;
        }
        CNObj.SetFlags (0, Info.mVarType);
        //
        // Use maximum value not to limit the valid value for the undefined question.
        //
        switch (Info.mVarType) {
        case EFI_IFR_TYPE_NUM_SIZE_64:
          CNObj.SetMinMaxStepData ((UINT64) 0, (UINT64) -1 , (UINT64) 0);
          ShrinkSize = 0;
          break;
        case EFI_IFR_TYPE_NUM_SIZE_32:
          CNObj.SetMinMaxStepData ((UINT32) 0, (UINT32) -1 , (UINT32) 0);
          ShrinkSize = 12;
          break;
        case EFI_IFR_TYPE_NUM_SIZE_16:
          CNObj.SetMinMaxStepData ((UINT16) 0, (UINT16) -1 , (UINT16) 0);
          ShrinkSize = 18;
          break;
        case EFI_IFR_TYPE_NUM_SIZE_8:
          CNObj.SetMinMaxStepData ((UINT8) 0, (UINT8) -1 , (UINT8) 0);
          ShrinkSize = 21;
          break;
        default:
          break;
        }
      }
      CNObj.ShrinkBinSize (ShrinkSize);

      //
      // For undefined Efi VarStore type question
      // Append the extended guided opcode to contain VarName
      //
      if (VarStoreType == EFI_VFR_VARSTORE_EFI) {
        CIfrVarEqName CVNObj (QId, Info.mInfo.mVarName);
        CVNObj.SetLineNo (LineNo);
      }

      //
      // End for Numeric
      //
      CIfrEnd CEObj;
      CEObj.SetLineNo (LineNo);
      //
      // End for Guided opcode
      //
      if (GuidObj != NULL) {
        CIfrEnd CEObjGuid;
        CEObjGuid.SetLineNo (LineNo);
        GuidObj->SetScope(1);
        delete GuidObj;
        GuidObj = NULL;
      }
    }
  }

  //
  // End for DisableIf
  //
  CIfrEnd SEObj;
  SEObj.SetLineNo (LineNo);

  return VFR_RETURN_SUCCESS;
}

CFormPkg gCFormPkg;

SIfrRecord::SIfrRecord (
  VOID
  )
{
  mIfrBinBuf = NULL;
  mBinBufLen = 0;
  mLineNo    = 0xFFFFFFFF;
  mOffset    = 0xFFFFFFFF;
  mNext      = NULL;
}

SIfrRecord::~SIfrRecord (
  VOID
  )
{
  if (mIfrBinBuf != NULL) {
    //delete mIfrBinBuf;
    mIfrBinBuf = NULL;
  }
  mLineNo      = 0xFFFFFFFF;
  mOffset      = 0xFFFFFFFF;
  mBinBufLen   = 0;
  mNext        = NULL;
}

CIfrRecordInfoDB::CIfrRecordInfoDB (
  VOID
  )
{
  mSwitch            = TRUE;
  mRecordCount       = EFI_IFR_RECORDINFO_IDX_START;
  mIfrRecordListHead = NULL;
  mIfrRecordListTail = NULL;
  mAllDefaultTypeCount = 0;
  for (UINT8 i = 0; i < EFI_HII_MAX_SUPPORT_DEFAULT_TYPE; i++) {
    mAllDefaultIdArray[i] = 0xffff;
  }
}

CIfrRecordInfoDB::~CIfrRecordInfoDB (
  VOID
  )
{
  SIfrRecord *pNode;

  while (mIfrRecordListHead != NULL) {
    pNode = mIfrRecordListHead;
    mIfrRecordListHead = mIfrRecordListHead->mNext;
    delete pNode;
  }
}

SIfrRecord *
CIfrRecordInfoDB::GetRecordInfoFromIdx (
  IN UINT32 RecordIdx
  )
{
  UINT32     Idx;
  SIfrRecord *pNode = NULL;

  if (RecordIdx == EFI_IFR_RECORDINFO_IDX_INVALUD) {
    return NULL;
  }

  for (Idx = (EFI_IFR_RECORDINFO_IDX_START + 1), pNode = mIfrRecordListHead;
       (Idx != RecordIdx) && (pNode != NULL);
       Idx++, pNode = pNode->mNext)
  ;

  return pNode;
}

UINT32
CIfrRecordInfoDB::IfrRecordRegister (
  IN UINT32 LineNo,
  IN CHAR8  *IfrBinBuf,
  IN UINT8  BinBufLen,
  IN UINT32 Offset
  )
{
  SIfrRecord *pNew;

  if (mSwitch == FALSE) {
    return EFI_IFR_RECORDINFO_IDX_INVALUD;
  }

  if ((pNew = new SIfrRecord) == NULL) {
    return EFI_IFR_RECORDINFO_IDX_INVALUD;
  }

  if (mIfrRecordListHead == NULL) {
    mIfrRecordListHead = pNew;
    mIfrRecordListTail = pNew;
  } else {
    mIfrRecordListTail->mNext = pNew;
    mIfrRecordListTail = pNew;
  }
  mRecordCount++;

  return mRecordCount;
}

VOID
CIfrRecordInfoDB::IfrRecordInfoUpdate (
  IN UINT32 RecordIdx,
  IN UINT32 LineNo,
  IN CHAR8  *BinBuf,
  IN UINT8  BinBufLen,
  IN UINT32 Offset
  )
{
  SIfrRecord *pNode;
  SIfrRecord *Prev;

  if ((pNode = GetRecordInfoFromIdx (RecordIdx)) == NULL) {
    return;
  }

  if (LineNo == 0) {
    //
    // Line number is not specified explicitly, try to use line number of previous opcode
    //
    Prev = GetRecordInfoFromIdx (RecordIdx - 1);
    if (Prev != NULL) {
      LineNo = Prev->mLineNo;
    }
  }

  pNode->mLineNo    = LineNo;
  pNode->mOffset    = Offset;
  pNode->mBinBufLen = BinBufLen;
  pNode->mIfrBinBuf = BinBuf;

}

VOID
CIfrRecordInfoDB::IfrRecordOutput (
  OUT PACKAGE_DATA &TBuffer
  )
{
  CHAR8      *Temp;
  SIfrRecord *pNode;

  if (TBuffer.Buffer != NULL) {
    delete[] TBuffer.Buffer;
  }

  TBuffer.Size = 0;
  TBuffer.Buffer = NULL;


  if (mSwitch == FALSE) {
    return;
  }

  for (pNode = mIfrRecordListHead; pNode != NULL; pNode = pNode->mNext) {
    TBuffer.Size += pNode->mBinBufLen;
  }

  if (TBuffer.Size != 0) {
    TBuffer.Buffer = new CHAR8[TBuffer.Size];
  } else {
    return;
  }

  Temp = TBuffer.Buffer;

  for (pNode = mIfrRecordListHead; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mIfrBinBuf != NULL) {
      memcpy (Temp, pNode->mIfrBinBuf, pNode->mBinBufLen);
      Temp += pNode->mBinBufLen;
    }
  }

  return;
}

VOID
CIfrRecordInfoDB::IfrRecordOutput (
  IN FILE   *File,
  IN UINT32 LineNo
  )
{
  SIfrRecord *pNode;
  UINT8      Index;
  UINT32     TotalSize;

  if (mSwitch == FALSE) {
    return;
  }

  if (File == NULL) {
    return;
  }

  TotalSize = 0;

  for (pNode = mIfrRecordListHead; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mLineNo == LineNo || LineNo == 0) {
      fprintf (File, ">%08X: ", pNode->mOffset);
      TotalSize += pNode->mBinBufLen;
      if (pNode->mIfrBinBuf != NULL) {
        for (Index = 0; Index < pNode->mBinBufLen; Index++) {
          fprintf (File, "%02X ", (UINT8)(pNode->mIfrBinBuf[Index]));
        }
      }
      fprintf (File, "\n");
    }
  }

  if (LineNo == 0) {
    fprintf (File, "\nTotal Size of all record is 0x%08X\n", TotalSize);
  }
}

//
// for framework vfr file
// adjust opcode sequence for uefi IFR format
// adjust inconsistent and varstore into the right position.
//
BOOLEAN
CIfrRecordInfoDB::CheckQuestionOpCode (
  IN UINT8 OpCode
  )
{
  switch (OpCode) {
  case EFI_IFR_CHECKBOX_OP:
  case EFI_IFR_NUMERIC_OP:
  case EFI_IFR_PASSWORD_OP:
  case EFI_IFR_ONE_OF_OP:
  case EFI_IFR_ACTION_OP:
  case EFI_IFR_STRING_OP:
  case EFI_IFR_DATE_OP:
  case EFI_IFR_TIME_OP:
  case EFI_IFR_ORDERED_LIST_OP:
  case EFI_IFR_REF_OP:
    return TRUE;
  default:
    return FALSE;
  }
}

BOOLEAN
CIfrRecordInfoDB::CheckIdOpCode (
  IN UINT8 OpCode
  )
{
  switch (OpCode) {
  case EFI_IFR_EQ_ID_VAL_OP:
  case EFI_IFR_EQ_ID_ID_OP:
  case EFI_IFR_EQ_ID_VAL_LIST_OP:
  case EFI_IFR_QUESTION_REF1_OP:
    return TRUE;
  default:
    return FALSE;
  }
}

EFI_QUESTION_ID
CIfrRecordInfoDB::GetOpcodeQuestionId (
  IN EFI_IFR_OP_HEADER *OpHead
  )
{
  EFI_IFR_QUESTION_HEADER *QuestionHead;

  QuestionHead = (EFI_IFR_QUESTION_HEADER *) (OpHead + 1);

  return QuestionHead->QuestionId;
}

SIfrRecord *
CIfrRecordInfoDB::GetRecordInfoFromOffset (
  IN UINT32 Offset
  )
{
  SIfrRecord *pNode = NULL;

  for (pNode = mIfrRecordListHead; pNode != NULL; pNode = pNode->mNext) {
    if (pNode->mOffset == Offset) {
      return pNode;
    }
  }

  return pNode;
}

/**
  Add just the op code position.

  Case1 (CreateOpcodeAfterParsingVfr == FALSE): The dynamic opcodes were created before the formset opcode,
  so pDynamicOpcodeNodes is before mIfrRecordListTail.

  From

  |mIfrRecordListHead + ...+ pAdjustNode + pDynamicOpcodeNodes + mIfrRecordListTail|

  To

  |mIfrRecordListHead + ...+ pDynamicOpcodeNodes + pAdjustNode + mIfrRecordListTail|

  Case2 (CreateOpcodeAfterParsingVfr == TRUE): The dynamic opcodes were created after paring the vfr file,
  so new records are appennded to the end of OriginalIfrRecordListTail.

  From

  |mIfrRecordListHead + ...+ pAdjustNode +  ... + OriginalIfrRecordListTail + pDynamicOpcodeNodes|

  To

  |mIfrRecordListHead + ...+ pDynamicOpcodeNodes + pAdjustNode +  ... + OriginalIfrRecordListTail|


  @param CreateOpcodeAfterParsingVfr     Whether create the dynamic opcode after parsing the VFR file.

**/
BOOLEAN
CIfrRecordInfoDB::IfrAdjustDynamicOpcodeInRecords (
  IN BOOLEAN  CreateOpcodeAfterParsingVfr
  )
{
  UINT32             OpcodeOffset;
  SIfrRecord         *pNode, *pPreNode;
  SIfrRecord         *pAdjustNode, *pNodeBeforeAdjust;
  SIfrRecord         *pNodeBeforeDynamic;

  pPreNode            = NULL;
  pAdjustNode         = NULL;
  pNodeBeforeDynamic  = NULL;
  OpcodeOffset        = 0;

  //
  // Base on the gAdjustOpcodeOffset and gAdjustOpcodeLen to find the pAdjustNod, the node before pAdjustNode,
  // and the node before pDynamicOpcodeNode.
  //
  for (pNode = mIfrRecordListHead; pNode!= NULL; pNode = pNode->mNext) {
    if (OpcodeOffset == gAdjustOpcodeOffset) {
      pAdjustNode       = pNode;
      pNodeBeforeAdjust = pPreNode;
    } else if (OpcodeOffset == gAdjustOpcodeOffset + gAdjustOpcodeLen) {
      pNodeBeforeDynamic = pPreNode;
    }
    if (pNode->mNext != NULL) {
      pPreNode = pNode;
    }
    OpcodeOffset += pNode->mBinBufLen;
  }

  //
  // Check the nodes whether exist.
  //
  if (pNodeBeforeDynamic == NULL || pAdjustNode == NULL || pNodeBeforeAdjust == NULL) {
    return FALSE;
  }

  //
  // Adjust the node. pPreNode save the Node before mIfrRecordListTail
  //
  pNodeBeforeAdjust->mNext = pNodeBeforeDynamic->mNext;
  if (CreateOpcodeAfterParsingVfr) {
    //
    // mIfrRecordListTail is the end of pDynamicNode (Case2).
    //
    mIfrRecordListTail->mNext = pAdjustNode;
    mIfrRecordListTail = pNodeBeforeDynamic;
    mIfrRecordListTail->mNext = NULL;
  } else {
    //
    //pPreNode is the end of pDynamicNode(Case1).
    //
    pPreNode->mNext = pAdjustNode;
    pNodeBeforeDynamic->mNext = mIfrRecordListTail;
  }

  return TRUE;
}

/**
  Update the record info(the position in the record list, offset and mIfrBinBuf) for new created record.

  @param CreateOpcodeAfterParsingVfr     Whether create the dynamic opcode after parsing the VFR file.

**/
VOID
CIfrRecordInfoDB::IfrUpdateRecordInfoForDynamicOpcode (
  IN BOOLEAN  CreateOpcodeAfterParsingVfr
  )
{
  SIfrRecord          *pRecord;

  //
  // Base on the original offset info to update the record list.
  //
  if (!IfrAdjustDynamicOpcodeInRecords(CreateOpcodeAfterParsingVfr)) {
    gCVfrErrorHandle.PrintMsg (0, (CHAR8 *)"Error", (CHAR8 *)"Can not find the adjust offset in the record.");
  }

  //
  // Base on the opcode binary length to recalculate the offset for each opcode.
  //
  IfrAdjustOffsetForRecord();

  //
  // Base on the offset to find the binary address.
  //
  pRecord = GetRecordInfoFromOffset(gAdjustOpcodeOffset);
  while (pRecord != NULL) {
    pRecord->mIfrBinBuf = gCFormPkg.GetBufAddrBaseOnOffset(pRecord->mOffset);
    pRecord = pRecord->mNext;
  }
}


VOID
CIfrRecordInfoDB::IfrAdjustOffsetForRecord (
  VOID
  )
{
  UINT32             OpcodeOffset;
  SIfrRecord         *pNode;

  OpcodeOffset = 0;
  for (pNode = mIfrRecordListHead; pNode != NULL; pNode = pNode->mNext) {
    pNode->mOffset = OpcodeOffset;
    OpcodeOffset += pNode->mBinBufLen;
  }
}

EFI_VFR_RETURN_CODE
CIfrRecordInfoDB::IfrRecordAdjust (
  VOID
  )
{
  SIfrRecord *pNode, *preNode;
  SIfrRecord *uNode, *tNode;
  EFI_IFR_OP_HEADER  *OpHead, *tOpHead;
  EFI_QUESTION_ID    QuestionId;
  UINT32             StackCount;
  UINT32             QuestionScope;
  CHAR8              ErrorMsg[MAX_STRING_LEN] = {0, };
  EFI_VFR_RETURN_CODE  Status;

  //
  // Init local variable
  //
  Status = VFR_RETURN_SUCCESS;
  pNode = mIfrRecordListHead;
  preNode = pNode;
  QuestionScope = 0;
  while (pNode != NULL) {
    OpHead = (EFI_IFR_OP_HEADER *) pNode->mIfrBinBuf;

    //
    // make sure the inconsistent opcode in question scope
    //
    if (QuestionScope > 0) {
      QuestionScope += OpHead->Scope;
      if (OpHead->OpCode == EFI_IFR_END_OP) {
        QuestionScope --;
      }
    }

    if (CheckQuestionOpCode (OpHead->OpCode)) {
      QuestionScope = 1;
    }
    //
    // for the inconsistent opcode not in question scope, adjust it
    //
    if (OpHead->OpCode == EFI_IFR_INCONSISTENT_IF_OP && QuestionScope == 0) {
      //
      // for inconsistent opcode not in question scope
      //

      //
      // Count inconsistent opcode Scope
      //
      StackCount = OpHead->Scope;
      QuestionId = EFI_QUESTION_ID_INVALID;
      tNode = pNode;
      while (tNode != NULL && StackCount > 0) {
        tNode = tNode->mNext;
        tOpHead = (EFI_IFR_OP_HEADER *) tNode->mIfrBinBuf;
        //
        // Calculate Scope Number
        //
        StackCount += tOpHead->Scope;
        if (tOpHead->OpCode == EFI_IFR_END_OP) {
          StackCount --;
        }
        //
        // by IdEqual opcode to get QuestionId
        //
        if (QuestionId == EFI_QUESTION_ID_INVALID &&
            CheckIdOpCode (tOpHead->OpCode)) {
          QuestionId = *(EFI_QUESTION_ID *) (tOpHead + 1);
        }
      }
      if (tNode == NULL || QuestionId == EFI_QUESTION_ID_INVALID) {
        //
        // report error; not found
        //
        sprintf (ErrorMsg, "Inconsistent OpCode Record list invalid QuestionId is 0x%X", QuestionId);
        gCVfrErrorHandle.PrintMsg (0, NULL, "Error", ErrorMsg);
        Status = VFR_RETURN_MISMATCHED;
        break;
      }
      //
      // extract inconsistent opcode list
      // pNode is Inconsistent opcode, tNode is End Opcode
      //

      //
      // insert inconsistent opcode list into the right question scope by questionid
      //
      for (uNode = mIfrRecordListHead; uNode != NULL; uNode = uNode->mNext) {
        tOpHead = (EFI_IFR_OP_HEADER *) uNode->mIfrBinBuf;
        if (CheckQuestionOpCode (tOpHead->OpCode) &&
            (QuestionId == GetOpcodeQuestionId (tOpHead))) {
          break;
        }
      }
      //
      // insert inconsistent opcode list and check LATE_CHECK flag
      //
      if (uNode != NULL) {
        if ((((EFI_IFR_QUESTION_HEADER *)(tOpHead + 1))->Flags & 0x20) != 0) {
          //
          // if LATE_CHECK flag is set, change inconsistent to nosumbit
          //
          OpHead->OpCode = EFI_IFR_NO_SUBMIT_IF_OP;
        }

        //
        // skip the default storage for Date and Time
        //
        if ((uNode->mNext != NULL) && (*uNode->mNext->mIfrBinBuf == EFI_IFR_DEFAULT_OP)) {
          uNode = uNode->mNext;
        }

        preNode->mNext = tNode->mNext;
        tNode->mNext = uNode->mNext;
        uNode->mNext = pNode;
        //
        // reset pNode to head list, scan the whole list again.
        //
        pNode = mIfrRecordListHead;
        preNode = pNode;
        QuestionScope = 0;
        continue;
      } else {
        //
        // not found matched question id, report error
        //
        sprintf (ErrorMsg, "QuestionId required by Inconsistent OpCode is not found. QuestionId is 0x%X", QuestionId);
        gCVfrErrorHandle.PrintMsg (0, NULL, "Error", ErrorMsg);
        Status = VFR_RETURN_MISMATCHED;
        break;
      }
    } else if (OpHead->OpCode == EFI_IFR_VARSTORE_OP ||
               OpHead->OpCode == EFI_IFR_VARSTORE_EFI_OP) {
      //
      // for new added group of varstore opcode
      //
      tNode = pNode;
      while (tNode->mNext != NULL) {
        tOpHead = (EFI_IFR_OP_HEADER *) tNode->mNext->mIfrBinBuf;
        if (tOpHead->OpCode != EFI_IFR_VARSTORE_OP &&
            tOpHead->OpCode != EFI_IFR_VARSTORE_EFI_OP) {
          break;
        }
        tNode = tNode->mNext;
      }

      if (tNode->mNext == NULL) {
        //
        // invalid IfrCode, IfrCode end by EndOpCode
        //
        gCVfrErrorHandle.PrintMsg (0, NULL, "Error", "No found End Opcode in the end");
        Status = VFR_RETURN_MISMATCHED;
        break;
      }

      if (tOpHead->OpCode != EFI_IFR_END_OP) {
          //
          // not new added varstore, which are not needed to be adjust.
          //
          preNode = tNode;
          pNode   = tNode->mNext;
          continue;
      } else {
        //
        // move new added varstore opcode to the position befor form opcode
        // varstore opcode between pNode and tNode
        //

        //
        // search form opcode from begin
        //
        for (uNode = mIfrRecordListHead; uNode->mNext != NULL; uNode = uNode->mNext) {
          tOpHead = (EFI_IFR_OP_HEADER *) uNode->mNext->mIfrBinBuf;
          if (tOpHead->OpCode == EFI_IFR_FORM_OP) {
            break;
          }
        }
        //
        // Insert varstore opcode beform form opcode if form opcode is found
        //
        if (uNode->mNext != NULL) {
          preNode->mNext = tNode->mNext;
          tNode->mNext = uNode->mNext;
          uNode->mNext = pNode;
          //
          // reset pNode to head list, scan the whole list again.
          //
          pNode = mIfrRecordListHead;
          preNode = pNode;
          QuestionScope = 0;
          continue;
        } else {
          //
          // not found form, continue scan IfrRecord list
          //
          preNode = tNode;
          pNode   = tNode->mNext;
          continue;
        }
      }
    }
    //
    // next node
    //
    preNode = pNode;
    pNode = pNode->mNext;
  }

  //
  // Update Ifr Opcode Offset
  //
  if (Status == VFR_RETURN_SUCCESS) {
    IfrAdjustOffsetForRecord ();
  }
  return Status;
}

/**
  When the Varstore of the question is EFI_VFR_VARSTORE_BUFFER and the default value is not
  given by expression, should save the default info for the Buffer VarStore.

  @param  DefaultId           The default id.
  @param  pQuestionNode       Point to the question opcode node.
  @param  Value               The default value.
**/
VOID
CIfrRecordInfoDB::IfrAddDefaultToBufferConfig (
  IN  UINT16                  DefaultId,
  IN  SIfrRecord              *pQuestionNode,
  IN  EFI_IFR_TYPE_VALUE      Value
  )
{
  CHAR8                   *VarStoreName = NULL;
  EFI_VFR_VARSTORE_TYPE    VarStoreType  = EFI_VFR_VARSTORE_INVALID;
  EFI_GUID                 *VarGuid      = NULL;
  EFI_VARSTORE_INFO        VarInfo;
  EFI_IFR_QUESTION_HEADER  *QuestionHead;
  EFI_IFR_OP_HEADER        *pQuestionOpHead;

  pQuestionOpHead = (EFI_IFR_OP_HEADER *) pQuestionNode->mIfrBinBuf;
  QuestionHead    = (EFI_IFR_QUESTION_HEADER *) (pQuestionOpHead + 1);

  //
  // Get the Var Store name and type.
  //
  gCVfrDataStorage.GetVarStoreName (QuestionHead->VarStoreId, &VarStoreName);
  VarGuid= gCVfrDataStorage.GetVarStoreGuid (QuestionHead->VarStoreId);
  VarStoreType = gCVfrDataStorage.GetVarStoreType (QuestionHead->VarStoreId);

  //
  // Only for Buffer storage need to save the default info in the storage.
  // Other type storage, just return.
  //
  if (VarStoreType != EFI_VFR_VARSTORE_BUFFER) {
    return;
  } else {
    VarInfo.mInfo.mVarOffset = QuestionHead->VarStoreInfo.VarOffset;
    VarInfo.mVarStoreId = QuestionHead->VarStoreId;
  }

  //
  // Get the buffer storage info about this question.
  //
  gCVfrDataStorage.GetBufferVarStoreFieldInfo (&VarInfo);

  //
  // Add action.
  //
  gCVfrDefaultStore.BufferVarStoreAltConfigAdd (
    DefaultId,
    VarInfo,
    VarStoreName,
    VarGuid,
    VarInfo.mVarType,
    Value
    );
}

/**
  Record the number and default id of all defaultstore opcode.

**/
VOID
CIfrRecordInfoDB::IfrGetDefaultStoreInfo (
  VOID
  )
{
  SIfrRecord             *pNode;
  EFI_IFR_OP_HEADER      *pOpHead;
  EFI_IFR_DEFAULTSTORE   *DefaultStore;

  pNode                = mIfrRecordListHead;
  mAllDefaultTypeCount = 0;

  while (pNode != NULL) {
    pOpHead = (EFI_IFR_OP_HEADER *) pNode->mIfrBinBuf;

    if (pOpHead->OpCode == EFI_IFR_DEFAULTSTORE_OP){
      DefaultStore = (EFI_IFR_DEFAULTSTORE *) pNode->mIfrBinBuf;
      mAllDefaultIdArray[mAllDefaultTypeCount++] = DefaultStore->DefaultId;
    }
    pNode = pNode->mNext;
  }
}

/**
  Create new default opcode record.

  @param    Size            The new default opcode size.
  @param    DefaultId       The new default id.
  @param    Type            The new default type.
  @param    LineNo          The line number of the new record.
  @param    Value           The new default value.

**/
VOID
CIfrRecordInfoDB::IfrCreateDefaultRecord(
  IN UINT8               Size,
  IN UINT16              DefaultId,
  IN UINT8               Type,
  IN UINT32              LineNo,
  IN EFI_IFR_TYPE_VALUE  Value
  )
{
  CIfrDefault   *DObj;
  CIfrDefault2  *DObj2;

  DObj  = NULL;
  DObj2 = NULL;

  if (Type == EFI_IFR_TYPE_OTHER) {
    DObj2 = new CIfrDefault2 (Size);
    DObj2->SetDefaultId(DefaultId);
    DObj2->SetType(Type);
    DObj2->SetLineNo(LineNo);
    DObj2->SetScope (1);
    delete DObj2;
  } else {
    DObj = new CIfrDefault (Size);
    DObj->SetDefaultId(DefaultId);
    DObj->SetType(Type);
    DObj->SetLineNo(LineNo);
    DObj->SetValue (Value);
    delete DObj;
  }
}

/**
  Create new default opcode for question base on the QuestionDefaultInfo.

  @param  pQuestionNode              Point to the question opcode Node.
  @param  QuestionDefaultInfo        Point to the QuestionDefaultInfo for current question.

**/
VOID
CIfrRecordInfoDB::IfrCreateDefaultForQuestion (
  IN  SIfrRecord              *pQuestionNode,
  IN  QuestionDefaultRecord   *QuestionDefaultInfo
  )
{
  EFI_IFR_OP_HEADER      *pOpHead;
  EFI_IFR_DEFAULT        *Default;
  SIfrRecord             *pSNode;
  SIfrRecord             *pENode;
  SIfrRecord             *pDefaultNode;
  CIfrObj                *Obj;
  CHAR8                  *ObjBinBuf;
  UINT8                  ScopeCount;
  UINT8                  OpcodeNumber;
  UINT8                  OpcodeCount;
  UINT8                  DefaultSize;
  EFI_IFR_ONE_OF_OPTION  *DefaultOptionOpcode;
  EFI_IFR_TYPE_VALUE     CheckBoxDefaultValue;

  CheckBoxDefaultValue.b = 1;
  pOpHead                = (EFI_IFR_OP_HEADER *) pQuestionNode->mIfrBinBuf;
  ScopeCount             = 0;
  OpcodeCount            = 0;
  Obj                    = NULL;

  //
  // Record the offset of node which need to be adjust, will move the new created default opcode to this offset.
  //
  gAdjustOpcodeOffset = pQuestionNode->mNext->mOffset;
  //
  // Case 1:
  // For oneof, the default with smallest default id is given by the option flag.
  // So create the missing defaults base on the oneof option value(mDefaultValueRecord).
  //
  if (pOpHead->OpCode == EFI_IFR_ONE_OF_OP && !QuestionDefaultInfo->mIsDefaultOpcode) {
    DefaultOptionOpcode = (EFI_IFR_ONE_OF_OPTION *)QuestionDefaultInfo->mDefaultValueRecord->mIfrBinBuf;
    DefaultSize = QuestionDefaultInfo->mDefaultValueRecord->mBinBufLen - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value);
    DefaultSize += OFFSET_OF (EFI_IFR_DEFAULT, Value);
    for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
      if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
        IfrCreateDefaultRecord (DefaultSize, mAllDefaultIdArray[i], DefaultOptionOpcode->Type, pQuestionNode->mLineNo, DefaultOptionOpcode->Value);
        //
        // Save the new created default in the buffer storage.
        //
        IfrAddDefaultToBufferConfig (mAllDefaultIdArray[i], pQuestionNode, DefaultOptionOpcode->Value);
      }
    }
    return;
  }

  //
  // Case2:
  // For checkbox, the default with smallest default id is given by the question flag.
  // And create the missing defaults with true value.
  //
  if (pOpHead-> OpCode == EFI_IFR_CHECKBOX_OP && !QuestionDefaultInfo->mIsDefaultOpcode) {
    DefaultSize = OFFSET_OF (EFI_IFR_DEFAULT, Value) + sizeof (BOOLEAN);
    for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
      if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
        IfrCreateDefaultRecord (DefaultSize, mAllDefaultIdArray[i], EFI_IFR_TYPE_BOOLEAN, pQuestionNode->mLineNo, CheckBoxDefaultValue);
        //
        // Save the new created default.
        //
        IfrAddDefaultToBufferConfig (mAllDefaultIdArray[i], pQuestionNode, CheckBoxDefaultValue);
      }
    }
    return;
  }

  //
  // Case3:
  // The default with smallest default id is given by the default opcode.
  // So create the missing defaults base on the value in the default opcode.
  //

  //
  // pDefaultNode point to the mDefaultValueRecord in QuestionDefaultInfo.
  //
  pDefaultNode = QuestionDefaultInfo->mDefaultValueRecord;
  Default = (EFI_IFR_DEFAULT *)pDefaultNode->mIfrBinBuf;
  //
  // Record the offset of node which need to be adjust, will move the new created default opcode to this offset.
  //
  gAdjustOpcodeOffset = pDefaultNode->mNext->mOffset;

  if (Default->Type == EFI_IFR_TYPE_OTHER) {
    //
    // EFI_IFR_DEFAULT_2 opcode.
    //
    // Point to the first expression opcode.
    //
    pSNode = pDefaultNode->mNext;
    pENode = NULL;
    ScopeCount++;
    //
    // Get opcode number behind the EFI_IFR_DEFAULT_2 until reach its END opcode (including the END opcode of EFI_IFR_DEFAULT_2)
    //
    while (pSNode != NULL && pSNode->mNext != NULL && ScopeCount != 0) {
      pOpHead = (EFI_IFR_OP_HEADER *) pSNode->mIfrBinBuf;
      if (pOpHead->Scope == 1) {
        ScopeCount++;
      }
      if (pOpHead->OpCode == EFI_IFR_END_OP) {
        ScopeCount--;
      }
      pENode = pSNode;
      pSNode = pSNode->mNext;
      OpcodeCount++;
    }

    assert (pSNode);
    assert (pENode);

    //
    // Record the offset of node which need to be adjust, will move the new created default opcode to this offset.
    //
    gAdjustOpcodeOffset = pSNode->mOffset;
    //
    // Create new default opcode node for missing default.
    //
    for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
      OpcodeNumber = OpcodeCount;
      if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
        IfrCreateDefaultRecord (Default->Header.Length, mAllDefaultIdArray[i], Default->Type, pENode->mLineNo, Default->Value);
        //
        // Point to the first expression opcode node.
        //
        pSNode = pDefaultNode->mNext;
        //
        // Create the expression opcode and end opcode for the new created EFI_IFR_DEFAULT_2 opcode.
        //
        while (pSNode != NULL && pSNode->mNext != NULL && OpcodeNumber-- != 0) {
          pOpHead = (EFI_IFR_OP_HEADER *) pSNode->mIfrBinBuf;
          Obj = new CIfrObj (pOpHead->OpCode, NULL, pSNode->mBinBufLen, FALSE);
          assert (Obj != NULL);
          Obj->SetLineNo (pSNode->mLineNo);
          ObjBinBuf = Obj->GetObjBinAddr<CHAR8>();
          memcpy (ObjBinBuf, pSNode->mIfrBinBuf, (UINTN)pSNode->mBinBufLen);
          delete Obj;
          pSNode = pSNode->mNext;
        }
      }
    }
  } else {
    //
    // EFI_IFR_DEFAULT opcode.
    //
    // Create new default opcode node for missing default.
    //
    for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
      if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
        IfrCreateDefaultRecord (Default->Header.Length, mAllDefaultIdArray[i], Default->Type, pDefaultNode->mLineNo, Default->Value);
        //
        // Save the new created default in the buffer storage..
        //
        IfrAddDefaultToBufferConfig (mAllDefaultIdArray[i], pQuestionNode, Default->Value);
      }
    }
  }
}

/**
  Parse the default information in a question, get the QuestionDefaultInfo.

  @param  pQuestionNode          Point to the question record Node.
  @param  QuestionDefaultInfo    On return, point to the QuestionDefaultInfo.
**/
VOID
CIfrRecordInfoDB::IfrParseDefaulInfoInQuestion(
  IN  SIfrRecord              *pQuestionNode,
  OUT QuestionDefaultRecord   *QuestionDefaultInfo
  )
{
  SIfrRecord              *pSNode;
  EFI_IFR_ONE_OF_OPTION   *OneofOptionOpcode;
  EFI_IFR_OP_HEADER       *pSOpHead;
  EFI_IFR_CHECKBOX        *CheckBoxOpcode;
  EFI_IFR_DEFAULT         *DefaultOpcode;
  BOOLEAN                 IsOneOfOpcode;
  UINT16                  SmallestDefaultId;
  UINT8                   ScopeCount;

  SmallestDefaultId  = 0xffff;
  IsOneOfOpcode      = FALSE;
  ScopeCount         = 0;
  pSNode             = pQuestionNode;

  //
  // Parse all the opcodes in the Question.
  //
  while (pSNode != NULL) {
    pSOpHead = (EFI_IFR_OP_HEADER *) pSNode->mIfrBinBuf;
    //
    // For a question, its scope bit must be set, the scope exists until it reaches a corresponding EFI_IFR_END_OP.
    // Scopes may be nested within other scopes.
    // When finishing parsing a question, the scope count must be zero.
    //
    if (pSOpHead->Scope == 1) {
      ScopeCount++;
    }
    if (pSOpHead->OpCode == EFI_IFR_END_OP) {
      ScopeCount--;
    }
    //
    // Check whether finishing parsing a question.
    //
    if (ScopeCount == 0) {
      break;
    }

    //
    // Record the default information in the question.
    //
    switch (pSOpHead->OpCode) {
    case EFI_IFR_ONE_OF_OP:
      IsOneOfOpcode = TRUE;
      break;
    case EFI_IFR_CHECKBOX_OP:
      //
      // The default info of check box may be given by flag.
      // So need to check the flag of check box.
      //
      CheckBoxOpcode = (EFI_IFR_CHECKBOX *)pSNode->mIfrBinBuf;
      if ((CheckBoxOpcode->Flags & EFI_IFR_CHECKBOX_DEFAULT) != 0) {
        //
        // Check whether need to update the smallest default id.
        //
        if (SmallestDefaultId > EFI_HII_DEFAULT_CLASS_STANDARD) {
          SmallestDefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
        }
        //
        // Update the QuestionDefaultInfo.
        //
        for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
          if (mAllDefaultIdArray[i] == EFI_HII_DEFAULT_CLASS_STANDARD) {
            if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
              QuestionDefaultInfo->mDefaultNumber ++;
              QuestionDefaultInfo->mIsDefaultIdExist[i] = TRUE;
            }
            break;
          }
        }
      }
      if ((CheckBoxOpcode->Flags & EFI_IFR_CHECKBOX_DEFAULT_MFG) != 0) {
        //
        // Check whether need to update the smallest default id.
        //
        if (SmallestDefaultId > EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
          SmallestDefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
        }
        //
        // Update the QuestionDefaultInfo.
        //
        for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
          if (mAllDefaultIdArray[i] == EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
            if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
              QuestionDefaultInfo->mDefaultNumber ++;
              QuestionDefaultInfo->mIsDefaultIdExist[i] = TRUE;
            }
            break;
          }
        }
      }
      break;
    case EFI_IFR_ONE_OF_OPTION_OP:
      if (!IsOneOfOpcode) {
        //
        // Only check the option in oneof.
        //
        break;
      }
      OneofOptionOpcode = (EFI_IFR_ONE_OF_OPTION *)pSNode->mIfrBinBuf;
      if ((OneofOptionOpcode->Flags & EFI_IFR_OPTION_DEFAULT) != 0) {
        //
        // The option is used as the standard default.
        // Check whether need to update the smallest default id and QuestionDefaultInfo.
        //
        if (SmallestDefaultId > EFI_HII_DEFAULT_CLASS_STANDARD) {
          SmallestDefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
          QuestionDefaultInfo->mDefaultValueRecord = pSNode;
        }
        //
        // Update the IsDefaultIdExist array in QuestionDefaultInfo.
        //
        for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
          if (mAllDefaultIdArray[i] == EFI_HII_DEFAULT_CLASS_STANDARD) {
            if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
              QuestionDefaultInfo->mDefaultNumber ++;
              QuestionDefaultInfo->mIsDefaultIdExist[i] = TRUE;
            }
            break;
          }
        }
      }
      if ((OneofOptionOpcode->Flags & EFI_IFR_OPTION_DEFAULT_MFG) != 0) {
        //
        // This option is used as the manufacture default.
        // Check whether need to update the smallest default id and QuestionDefaultInfo.
        //
        if (SmallestDefaultId > EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
          SmallestDefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
          QuestionDefaultInfo->mDefaultValueRecord = pSNode;
        }
        //
        // Update the QuestionDefaultInfo.
        //
        for (UINT8 i = 0; i < mAllDefaultTypeCount; i++) {
          if (mAllDefaultIdArray[i] == EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
            if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
              QuestionDefaultInfo->mDefaultNumber ++;
              QuestionDefaultInfo->mIsDefaultIdExist[i] = TRUE;
            }
            break;
          }
        }
      }
      break;
    case EFI_IFR_DEFAULT_OP:
      DefaultOpcode = (EFI_IFR_DEFAULT *) pSNode->mIfrBinBuf;
      //
      // Check whether need to update the smallest default id and QuestionDefaultInfo.
      //
      if (SmallestDefaultId >= DefaultOpcode->DefaultId ) {
        SmallestDefaultId = DefaultOpcode->DefaultId;
        QuestionDefaultInfo->mDefaultValueRecord= pSNode;
        QuestionDefaultInfo->mIsDefaultOpcode= TRUE;
      }
      //
      // Update the QuestionDefaultInfo.
      //
      for (UINT8 i = 0; i < mAllDefaultTypeCount; i++){
        if (mAllDefaultIdArray[i] == ((EFI_IFR_DEFAULT *)pSNode->mIfrBinBuf)->DefaultId) {
          if (!QuestionDefaultInfo->mIsDefaultIdExist[i]) {
            QuestionDefaultInfo->mDefaultNumber ++;
            QuestionDefaultInfo->mIsDefaultIdExist[i] = TRUE;
          }
          break;
        }
      }
      break;
    default:
      break;
    }
    //
    // Parse next opcode in this question.
    //
    pSNode = pSNode->mNext;
  }
}

/**
  Check or add default for question if need.

  This function will check the default info for question.
  If the question has default, but the default number < defaultstore opcode number.
  will do following two action :

  1. if (AutoDefault) will add default for question to support all kinds of defaults.
  2. if (CheckDefault) will generate an error to tell user the question misses some default value.

  We assume that the two options can not be TRUE at same time.
  If they are TRUE at same time, only do the action corresponding to AutoDefault option.

  @param  AutoDefault          Add default for question if needed
  @param  CheckDefault         Check the default info, if missing default, generates an error.

**/
VOID
CIfrRecordInfoDB::IfrCheckAddDefaultRecord (
  BOOLEAN  AutoDefault,
  BOOLEAN  CheckDefault
  )
{
  SIfrRecord            *pNode;
  SIfrRecord            *pTailNode;
  SIfrRecord            *pStartAdjustNode;
  EFI_IFR_OP_HEADER     *pOpHead;
  QuestionDefaultRecord  QuestionDefaultInfo;
  UINT8                  MissingDefaultCount;
  CHAR8                  Msg[MAX_STRING_LEN] = {0, };

  pNode               = mIfrRecordListHead;

  //
  // Record the number and default id of all defaultstore opcode.
  //
  IfrGetDefaultStoreInfo ();

  while (pNode != NULL) {
    pOpHead = (EFI_IFR_OP_HEADER *) pNode->mIfrBinBuf;
    //
    // Check whether is question opcode.
    //
    if (CheckQuestionOpCode (pOpHead->OpCode)) {
      //
      // Initialize some local variables here, because they vary with question.
      // Record the mIfrRecordListTail for each question, because may create default node for question after mIfrRecordListTail.
      //
      memset (&QuestionDefaultInfo, 0, sizeof (QuestionDefaultRecord));
      pTailNode = mIfrRecordListTail;
      //
      // Get the QuestionDefaultInfo for current question.
      //
      IfrParseDefaulInfoInQuestion (pNode, &QuestionDefaultInfo);

      if (QuestionDefaultInfo.mDefaultNumber != mAllDefaultTypeCount && QuestionDefaultInfo.mDefaultNumber != 0) {
        if (AutoDefault) {
          //
          // Create default for question which misses default.
          //
          IfrCreateDefaultForQuestion (pNode, &QuestionDefaultInfo);

          //
          // Adjust the buffer content.
          // pStartAdjustNode->mIfrBinBuf points to the insert position.
          // pTailNode->mNext->mIfrBinBuf points to the inset opcodes.
          //
          pStartAdjustNode =GetRecordInfoFromOffset (gAdjustOpcodeOffset);
          gCFormPkg.AdjustDynamicInsertOpcode (pStartAdjustNode->mIfrBinBuf, pTailNode->mNext->mIfrBinBuf, TRUE);

          //
          // Update the record info.
          //
          IfrUpdateRecordInfoForDynamicOpcode (TRUE);
        } else if (CheckDefault) {
          //
          // Generate an error for question which misses default.
          //
          MissingDefaultCount = mAllDefaultTypeCount - QuestionDefaultInfo.mDefaultNumber;
          sprintf (Msg, "The question misses %d default, the question's opcode is %d", MissingDefaultCount, pOpHead->OpCode);
          gCVfrErrorHandle.PrintMsg (pNode->mLineNo, NULL, "Error", Msg);
        }
      }
    }
    //
    // parse next opcode.
    //
    pNode = pNode->mNext;
  }
}

CIfrRecordInfoDB gCIfrRecordInfoDB;

VOID
CIfrObj::_EMIT_PENDING_OBJ (
  VOID
  )
{
  CHAR8  *ObjBinBuf = NULL;

  //
  // do nothing
  //
  if (!mDelayEmit || !gCreateOp) {
    return;
  }

  mPkgOffset = gCFormPkg.GetPkgLength ();
  //
  // update data buffer to package data
  //
  ObjBinBuf  = gCFormPkg.IfrBinBufferGet (mObjBinLen);
  if (ObjBinBuf != NULL) {
    memmove (ObjBinBuf, mObjBinBuf, mObjBinLen);
  }

  //
  // update bin buffer to package data buffer
  //
  if (mObjBinBuf != NULL) {
    delete[] mObjBinBuf;
    mObjBinBuf = ObjBinBuf;
  }

  mDelayEmit = FALSE;
}

/*
 * The definition of CIfrObj's member function
 */
static struct {
  UINT8  mSize;
  UINT8  mScope;
} gOpcodeSizesScopeTable[] = {
  { 0, 0 },                                    // EFI_IFR_INVALID - 0x00
  { sizeof (EFI_IFR_FORM), 1 },                // EFI_IFR_FORM_OP
  { sizeof (EFI_IFR_SUBTITLE), 1 },            // EFI_IFR_SUBTITLE_OP
  { sizeof (EFI_IFR_TEXT), 0 },                // EFI_IFR_TEXT_OP
  { sizeof (EFI_IFR_IMAGE), 0 },               // EFI_IFR_IMAGE_OP
  { sizeof (EFI_IFR_ONE_OF), 1 },              // EFI_IFR_ONE_OF_OP - 0x05
  { sizeof (EFI_IFR_CHECKBOX), 1},             // EFI_IFR_CHECKBOX_OP
  { sizeof (EFI_IFR_NUMERIC), 1 },             // EFI_IFR_NUMERIC_OP
  { sizeof (EFI_IFR_PASSWORD), 1 },            // EFI_IFR_PASSWORD_OP
  { sizeof (EFI_IFR_ONE_OF_OPTION), 0 },       // EFI_IFR_ONE_OF_OPTION_OP
  { sizeof (EFI_IFR_SUPPRESS_IF), 1 },         // EFI_IFR_SUPPRESS_IF - 0x0A
  { sizeof (EFI_IFR_LOCKED), 0 },              // EFI_IFR_LOCKED_OP
  { sizeof (EFI_IFR_ACTION), 1 },              // EFI_IFR_ACTION_OP
  { sizeof (EFI_IFR_RESET_BUTTON), 1 },        // EFI_IFR_RESET_BUTTON_OP
  { sizeof (EFI_IFR_FORM_SET), 1 },            // EFI_IFR_FORM_SET_OP -0xE
  { sizeof (EFI_IFR_REF), 0 },                 // EFI_IFR_REF_OP
  { sizeof (EFI_IFR_NO_SUBMIT_IF), 1},         // EFI_IFR_NO_SUBMIT_IF_OP -0x10
  { sizeof (EFI_IFR_INCONSISTENT_IF), 1 },     // EFI_IFR_INCONSISTENT_IF_OP
  { sizeof (EFI_IFR_EQ_ID_VAL), 0 },           // EFI_IFR_EQ_ID_VAL_OP
  { sizeof (EFI_IFR_EQ_ID_ID), 0 },            // EFI_IFR_EQ_ID_ID_OP
  { sizeof (EFI_IFR_EQ_ID_VAL_LIST), 0 },      // EFI_IFR_EQ_ID_LIST_OP - 0x14
  { sizeof (EFI_IFR_AND), 0 },                 // EFI_IFR_AND_OP
  { sizeof (EFI_IFR_OR), 0 },                  // EFI_IFR_OR_OP
  { sizeof (EFI_IFR_NOT), 0 },                 // EFI_IFR_NOT_OP
  { sizeof (EFI_IFR_RULE), 1 },                // EFI_IFR_RULE_OP
  { sizeof (EFI_IFR_GRAY_OUT_IF), 1 },         // EFI_IFR_GRAYOUT_IF_OP - 0x19
  { sizeof (EFI_IFR_DATE), 1 },                // EFI_IFR_DATE_OP
  { sizeof (EFI_IFR_TIME), 1 },                // EFI_IFR_TIME_OP
  { sizeof (EFI_IFR_STRING), 1 },              // EFI_IFR_STRING_OP
  { sizeof (EFI_IFR_REFRESH), 0 },             // EFI_IFR_REFRESH_OP
  { sizeof (EFI_IFR_DISABLE_IF), 1 },          // EFI_IFR_DISABLE_IF_OP - 0x1E
  { 0, 0 },                                    // 0x1F
  { sizeof (EFI_IFR_TO_LOWER), 0 },            // EFI_IFR_TO_LOWER_OP - 0x20
  { sizeof (EFI_IFR_TO_UPPER), 0 },            // EFI_IFR_TO_UPPER_OP - 0x21
  { sizeof (EFI_IFR_MAP), 1 },                 // EFI_IFR_MAP - 0x22
  { sizeof (EFI_IFR_ORDERED_LIST), 1 },        // EFI_IFR_ORDERED_LIST_OP - 0x23
  { sizeof (EFI_IFR_VARSTORE), 0 },            // EFI_IFR_VARSTORE_OP
  { sizeof (EFI_IFR_VARSTORE_NAME_VALUE), 0 }, // EFI_IFR_VARSTORE_NAME_VALUE_OP
  { sizeof (EFI_IFR_VARSTORE_EFI), 0 },        // EFI_IFR_VARSTORE_EFI_OP
  { sizeof (EFI_IFR_VARSTORE_DEVICE), 1 },     // EFI_IFR_VARSTORE_DEVICE_OP
  { sizeof (EFI_IFR_VERSION), 0 },             // EFI_IFR_VERSION_OP - 0x28
  { sizeof (EFI_IFR_END), 0 },                 // EFI_IFR_END_OP
  { sizeof (EFI_IFR_MATCH), 0 },               // EFI_IFR_MATCH_OP - 0x2A
  { sizeof (EFI_IFR_GET), 0 },                 // EFI_IFR_GET - 0x2B
  { sizeof (EFI_IFR_SET), 0 },                 // EFI_IFR_SET - 0x2C
  { sizeof (EFI_IFR_READ), 0 },                // EFI_IFR_READ - 0x2D
  { sizeof (EFI_IFR_WRITE), 0 },               // EFI_IFR_WRITE - 0x2E
  { sizeof (EFI_IFR_EQUAL), 0 },               // EFI_IFR_EQUAL_OP - 0x2F
  { sizeof (EFI_IFR_NOT_EQUAL), 0 },           // EFI_IFR_NOT_EQUAL_OP
  { sizeof (EFI_IFR_GREATER_THAN), 0 },        // EFI_IFR_GREATER_THAN_OP
  { sizeof (EFI_IFR_GREATER_EQUAL), 0 },       // EFI_IFR_GREATER_EQUAL_OP
  { sizeof (EFI_IFR_LESS_THAN), 0 },           // EFI_IFR_LESS_THAN_OP
  { sizeof (EFI_IFR_LESS_EQUAL), 0 },          // EFI_IFR_LESS_EQUAL_OP - 0x34
  { sizeof (EFI_IFR_BITWISE_AND), 0 },         // EFI_IFR_BITWISE_AND_OP
  { sizeof (EFI_IFR_BITWISE_OR), 0 },          // EFI_IFR_BITWISE_OR_OP
  { sizeof (EFI_IFR_BITWISE_NOT), 0 },         // EFI_IFR_BITWISE_NOT_OP
  { sizeof (EFI_IFR_SHIFT_LEFT), 0 },          // EFI_IFR_SHIFT_LEFT_OP
  { sizeof (EFI_IFR_SHIFT_RIGHT), 0 },         // EFI_IFR_SHIFT_RIGHT_OP
  { sizeof (EFI_IFR_ADD), 0 },                 // EFI_IFR_ADD_OP - 0x3A
  { sizeof (EFI_IFR_SUBTRACT), 0 },            // EFI_IFR_SUBTRACT_OP
  { sizeof (EFI_IFR_MULTIPLY), 0 },            // EFI_IFR_MULTIPLY_OP
  { sizeof (EFI_IFR_DIVIDE), 0 },              // EFI_IFR_DIVIDE_OP
  { sizeof (EFI_IFR_MODULO), 0 },              // EFI_IFR_MODULO_OP - 0x3E
  { sizeof (EFI_IFR_RULE_REF), 0 },            // EFI_IFR_RULE_REF_OP
  { sizeof (EFI_IFR_QUESTION_REF1), 0 },       // EFI_IFR_QUESTION_REF1_OP
  { sizeof (EFI_IFR_QUESTION_REF2), 0 },       // EFI_IFR_QUESTION_REF2_OP - 0x41
  { sizeof (EFI_IFR_UINT8), 0},                // EFI_IFR_UINT8
  { sizeof (EFI_IFR_UINT16), 0},               // EFI_IFR_UINT16
  { sizeof (EFI_IFR_UINT32), 0},               // EFI_IFR_UINT32
  { sizeof (EFI_IFR_UINT64), 0},               // EFI_IFR_UTNT64
  { sizeof (EFI_IFR_TRUE), 0 },                // EFI_IFR_TRUE_OP - 0x46
  { sizeof (EFI_IFR_FALSE), 0 },               // EFI_IFR_FALSE_OP
  { sizeof (EFI_IFR_TO_UINT), 0 },             // EFI_IFR_TO_UINT_OP
  { sizeof (EFI_IFR_TO_STRING), 0 },           // EFI_IFR_TO_STRING_OP
  { sizeof (EFI_IFR_TO_BOOLEAN), 0 },          // EFI_IFR_TO_BOOLEAN_OP
  { sizeof (EFI_IFR_MID), 0 },                 // EFI_IFR_MID_OP
  { sizeof (EFI_IFR_FIND), 0 },                // EFI_IFR_FIND_OP
  { sizeof (EFI_IFR_TOKEN), 0 },               // EFI_IFR_TOKEN_OP
  { sizeof (EFI_IFR_STRING_REF1), 0 },         // EFI_IFR_STRING_REF1_OP - 0x4E
  { sizeof (EFI_IFR_STRING_REF2), 0 },         // EFI_IFR_STRING_REF2_OP
  { sizeof (EFI_IFR_CONDITIONAL), 0 },         // EFI_IFR_CONDITIONAL_OP
  { sizeof (EFI_IFR_QUESTION_REF3), 0 },       // EFI_IFR_QUESTION_REF3_OP
  { sizeof (EFI_IFR_ZERO), 0 },                // EFI_IFR_ZERO_OP
  { sizeof (EFI_IFR_ONE), 0 },                 // EFI_IFR_ONE_OP
  { sizeof (EFI_IFR_ONES), 0 },                // EFI_IFR_ONES_OP
  { sizeof (EFI_IFR_UNDEFINED), 0 },           // EFI_IFR_UNDEFINED_OP
  { sizeof (EFI_IFR_LENGTH), 0 },              // EFI_IFR_LENGTH_OP
  { sizeof (EFI_IFR_DUP), 0 },                 // EFI_IFR_DUP_OP - 0x57
  { sizeof (EFI_IFR_THIS), 0 },                // EFI_IFR_THIS_OP
  { sizeof (EFI_IFR_SPAN), 0 },                // EFI_IFR_SPAN_OP
  { sizeof (EFI_IFR_VALUE), 1 },               // EFI_IFR_VALUE_OP
  { sizeof (EFI_IFR_DEFAULT), 0 },             // EFI_IFR_DEFAULT_OP
  { sizeof (EFI_IFR_DEFAULTSTORE), 0 },        // EFI_IFR_DEFAULTSTORE_OP - 0x5C
  { sizeof (EFI_IFR_FORM_MAP), 1},             // EFI_IFR_FORM_MAP_OP - 0x5D
  { sizeof (EFI_IFR_CATENATE), 0 },            // EFI_IFR_CATENATE_OP
  { sizeof (EFI_IFR_GUID), 0 },                // EFI_IFR_GUID_OP
  { sizeof (EFI_IFR_SECURITY), 0 },            // EFI_IFR_SECURITY_OP - 0x60
  { sizeof (EFI_IFR_MODAL_TAG), 0},            // EFI_IFR_MODAL_TAG_OP - 0x61
  { sizeof (EFI_IFR_REFRESH_ID), 0},           // EFI_IFR_REFRESH_ID_OP - 0x62
  { sizeof (EFI_IFR_WARNING_IF), 1},           // EFI_IFR_WARNING_IF_OP - 0x63
  { sizeof (EFI_IFR_MATCH2), 0 },              // EFI_IFR_MATCH2_OP - 0x64
};

#ifdef CIFROBJ_DEUBG
static struct {
  CHAR8 *mIfrName;
} gIfrObjPrintDebugTable[] = {
  "EFI_IFR_INVALID",    "EFI_IFR_FORM",                 "EFI_IFR_SUBTITLE",      "EFI_IFR_TEXT",            "EFI_IFR_IMAGE",         "EFI_IFR_ONE_OF",
  "EFI_IFR_CHECKBOX",   "EFI_IFR_NUMERIC",              "EFI_IFR_PASSWORD",      "EFI_IFR_ONE_OF_OPTION",   "EFI_IFR_SUPPRESS_IF",   "EFI_IFR_LOCKED",
  "EFI_IFR_ACTION",     "EFI_IFR_RESET_BUTTON",         "EFI_IFR_FORM_SET",      "EFI_IFR_REF",             "EFI_IFR_NO_SUBMIT_IF",  "EFI_IFR_INCONSISTENT_IF",
  "EFI_IFR_EQ_ID_VAL",  "EFI_IFR_EQ_ID_ID",             "EFI_IFR_EQ_ID_LIST",    "EFI_IFR_AND",             "EFI_IFR_OR",            "EFI_IFR_NOT",
  "EFI_IFR_RULE",       "EFI_IFR_GRAY_OUT_IF",          "EFI_IFR_DATE",          "EFI_IFR_TIME",            "EFI_IFR_STRING",        "EFI_IFR_REFRESH",
  "EFI_IFR_DISABLE_IF", "EFI_IFR_INVALID",              "EFI_IFR_TO_LOWER",      "EFI_IFR_TO_UPPER",        "EFI_IFR_MAP",           "EFI_IFR_ORDERED_LIST",
  "EFI_IFR_VARSTORE",   "EFI_IFR_VARSTORE_NAME_VALUE",  "EFI_IFR_VARSTORE_EFI",  "EFI_IFR_VARSTORE_DEVICE", "EFI_IFR_VERSION",       "EFI_IFR_END",
  "EFI_IFR_MATCH",      "EFI_IFR_GET",                  "EFI_IFR_SET",           "EFI_IFR_READ",            "EFI_IFR_WRITE",         "EFI_IFR_EQUAL",
  "EFI_IFR_NOT_EQUAL",  "EFI_IFR_GREATER_THAN",         "EFI_IFR_GREATER_EQUAL", "EFI_IFR_LESS_THAN",       "EFI_IFR_LESS_EQUAL",    "EFI_IFR_BITWISE_AND",
  "EFI_IFR_BITWISE_OR", "EFI_IFR_BITWISE_NOT",          "EFI_IFR_SHIFT_LEFT",    "EFI_IFR_SHIFT_RIGHT",     "EFI_IFR_ADD",           "EFI_IFR_SUBTRACT",
  "EFI_IFR_MULTIPLY",   "EFI_IFR_DIVIDE",               "EFI_IFR_MODULO",        "EFI_IFR_RULE_REF",        "EFI_IFR_QUESTION_REF1", "EFI_IFR_QUESTION_REF2",
  "EFI_IFR_UINT8",      "EFI_IFR_UINT16",               "EFI_IFR_UINT32",        "EFI_IFR_UINT64",          "EFI_IFR_TRUE",          "EFI_IFR_FALSE",
  "EFI_IFR_TO_UINT",    "EFI_IFR_TO_STRING",            "EFI_IFR_TO_BOOLEAN",    "EFI_IFR_MID",             "EFI_IFR_FIND",          "EFI_IFR_TOKEN",
  "EFI_IFR_STRING_REF1","EFI_IFR_STRING_REF2",          "EFI_IFR_CONDITIONAL",   "EFI_IFR_QUESTION_REF3",   "EFI_IFR_ZERO",          "EFI_IFR_ONE",
  "EFI_IFR_ONES",       "EFI_IFR_UNDEFINED",            "EFI_IFR_LENGTH",        "EFI_IFR_DUP",             "EFI_IFR_THIS",          "EFI_IFR_SPAN",
  "EFI_IFR_VALUE",      "EFI_IFR_DEFAULT",              "EFI_IFR_DEFAULTSTORE",  "EFI_IFR_FORM_MAP",        "EFI_IFR_CATENATE",      "EFI_IFR_GUID",
  "EFI_IFR_SECURITY",   "EFI_IFR_MODAL_TAG",            "EFI_IFR_REFRESH_ID",    "EFI_IFR_WARNING_IF",      "EFI_IFR_MATCH2",
};

VOID
CIFROBJ_DEBUG_PRINT (
  IN UINT8 OpCode
  )
{
  printf ("======Create IFR [%s]\n", gIfrObjPrintDebugTable[OpCode].mIfrName);
}
#else

#define CIFROBJ_DEBUG_PRINT(OpCode)

#endif

BOOLEAN gCreateOp = TRUE;

CIfrObj::CIfrObj (
  IN  UINT8   OpCode,
  OUT CHAR8   **IfrObj,
  IN  UINT8   ObjBinLen,
  IN  BOOLEAN DelayEmit
  )
{
  mDelayEmit   = DelayEmit;
  mPkgOffset   = gCFormPkg.GetPkgLength ();
  mObjBinLen   = (ObjBinLen == 0) ? gOpcodeSizesScopeTable[OpCode].mSize : ObjBinLen;
  mObjBinBuf   = ((DelayEmit == FALSE) && (gCreateOp == TRUE)) ? gCFormPkg.IfrBinBufferGet (mObjBinLen) : new CHAR8[EFI_IFR_MAX_LENGTH];
  mRecordIdx   = (gCreateOp == TRUE) ? gCIfrRecordInfoDB.IfrRecordRegister (0xFFFFFFFF, mObjBinBuf, mObjBinLen, mPkgOffset) : EFI_IFR_RECORDINFO_IDX_INVALUD;
  mLineNo      = 0;

  assert (mObjBinBuf != NULL);

  if (IfrObj != NULL) {
    *IfrObj    = mObjBinBuf;
  }

  CIFROBJ_DEBUG_PRINT (OpCode);
}

CIfrObj::~CIfrObj (
  VOID
  )
{
  if ((mDelayEmit == TRUE) && ((gCreateOp == TRUE))) {
    _EMIT_PENDING_OBJ ();
  }

  gCIfrRecordInfoDB.IfrRecordInfoUpdate (mRecordIdx, mLineNo, mObjBinBuf, mObjBinLen, mPkgOffset);
}

/*
 * The definition of CIfrObj's member function
 */
UINT8 gScopeCount = 0;

CIfrOpHeader::CIfrOpHeader (
  IN UINT8 OpCode,
  IN VOID *StartAddr,
  IN UINT8 Length
  ) : mHeader ((EFI_IFR_OP_HEADER *)StartAddr)
{
  mHeader->OpCode = OpCode;
  mHeader->Length = (Length == 0) ? gOpcodeSizesScopeTable[OpCode].mSize : Length;
  mHeader->Scope  = (gOpcodeSizesScopeTable[OpCode].mScope + gScopeCount > 0) ? 1 : 0;
}

CIfrOpHeader::CIfrOpHeader (
  IN CIfrOpHeader &OpHdr
  )
{
  mHeader = OpHdr.mHeader;
}

UINT32 CIfrFormId::FormIdBitMap[EFI_FREE_FORM_ID_BITMAP_SIZE] = {0, };
