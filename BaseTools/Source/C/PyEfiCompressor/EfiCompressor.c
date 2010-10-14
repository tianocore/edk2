/** @file

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Python.h>
#include <Decompress.h>

/*
 UefiDecompress(data_buffer, size, original_size)
*/
STATIC
PyObject*
UefiDecompress(
  PyObject    *Self,
  PyObject    *Args
  )
{
  PyObject      *SrcData;
  UINT32        SrcDataSize;
  UINT32        DstDataSize;
  UINTN         Status;
  UINT8         *SrcBuf;
  UINT8         *DstBuf;
  UINT8         *TmpBuf;
  Py_ssize_t    SegNum;
  Py_ssize_t    Index;

  Status = PyArg_ParseTuple(
            Args,
            "Oi",
            &SrcData,
            &SrcDataSize
            );
  if (Status == 0) {
    return NULL;
  }

  if (SrcData->ob_type->tp_as_buffer == NULL
      || SrcData->ob_type->tp_as_buffer->bf_getreadbuffer == NULL
      || SrcData->ob_type->tp_as_buffer->bf_getsegcount == NULL) {
    PyErr_SetString(PyExc_Exception, "First argument is not a buffer\n");
    return NULL;
  }

  // Because some Python objects which support "buffer" protocol have more than one
  // memory segment, we have to copy them into a contiguous memory.
  SrcBuf = PyMem_Malloc(SrcDataSize);
  if (SrcBuf == NULL) {
    PyErr_SetString(PyExc_Exception, "Not enough memory\n");
    goto ERROR;
  }

  SegNum = SrcData->ob_type->tp_as_buffer->bf_getsegcount((PyObject *)SrcData, NULL);
  TmpBuf = SrcBuf;
  for (Index = 0; Index < SegNum; ++Index) {
    VOID *BufSeg;
    Py_ssize_t Len;

    Len = SrcData->ob_type->tp_as_buffer->bf_getreadbuffer((PyObject *)SrcData, Index, &BufSeg);
    if (Len < 0) {
      PyErr_SetString(PyExc_Exception, "Buffer segment is not available\n");
      goto ERROR;
    }
    memcpy(TmpBuf, BufSeg, Len);
    TmpBuf += Len;
  }

  Status = Extract((VOID *)SrcBuf, SrcDataSize, (VOID **)&DstBuf, &DstDataSize, 1);
  if (Status != EFI_SUCCESS) {
    PyErr_SetString(PyExc_Exception, "Failed to decompress\n");
    goto ERROR;
  }

  return PyBuffer_FromMemory(DstBuf, (Py_ssize_t)DstDataSize);

ERROR:
  if (SrcBuf != NULL) {
    free(SrcBuf);
  }

  if (DstBuf != NULL) {
    free(DstBuf);
  }
  return NULL;
}


STATIC
PyObject*
FrameworkDecompress(
  PyObject    *Self,
  PyObject    *Args
  )
{
  PyObject      *SrcData;
  UINT32        SrcDataSize;
  UINT32        DstDataSize;
  UINTN         Status;
  UINT8         *SrcBuf;
  UINT8         *DstBuf;
  UINT8         *TmpBuf;
  Py_ssize_t    SegNum;
  Py_ssize_t    Index;

  Status = PyArg_ParseTuple(
            Args,
            "Oi",
            &SrcData,
            &SrcDataSize
            );
  if (Status == 0) {
    return NULL;
  }

  if (SrcData->ob_type->tp_as_buffer == NULL
      || SrcData->ob_type->tp_as_buffer->bf_getreadbuffer == NULL
      || SrcData->ob_type->tp_as_buffer->bf_getsegcount == NULL) {
    PyErr_SetString(PyExc_Exception, "First argument is not a buffer\n");
    return NULL;
  }

  // Because some Python objects which support "buffer" protocol have more than one
  // memory segment, we have to copy them into a contiguous memory.
  SrcBuf = PyMem_Malloc(SrcDataSize);
  if (SrcBuf == NULL) {
    PyErr_SetString(PyExc_Exception, "Not enough memory\n");
    goto ERROR;
  }

  SegNum = SrcData->ob_type->tp_as_buffer->bf_getsegcount((PyObject *)SrcData, NULL);
  TmpBuf = SrcBuf;
  for (Index = 0; Index < SegNum; ++Index) {
    VOID *BufSeg;
    Py_ssize_t Len;

    Len = SrcData->ob_type->tp_as_buffer->bf_getreadbuffer((PyObject *)SrcData, Index, &BufSeg);
    if (Len < 0) {
      PyErr_SetString(PyExc_Exception, "Buffer segment is not available\n");
      goto ERROR;
    }
    memcpy(TmpBuf, BufSeg, Len);
    TmpBuf += Len;
  }

  Status = Extract((VOID *)SrcBuf, SrcDataSize, (VOID **)&DstBuf, &DstDataSize, 2);
  if (Status != EFI_SUCCESS) {
    PyErr_SetString(PyExc_Exception, "Failed to decompress\n");
    goto ERROR;
  }

  return PyString_FromStringAndSize((CONST INT8*)DstBuf, (Py_ssize_t)DstDataSize);

ERROR:
  if (SrcBuf != NULL) {
    free(SrcBuf);
  }

  if (DstBuf != NULL) {
    free(DstBuf);
  }
  return NULL;
}


STATIC
PyObject*
UefiCompress(
  PyObject    *Self,
  PyObject    *Args
  )
{
  return NULL;
}


STATIC
PyObject*
FrameworkCompress(
  PyObject    *Self,
  PyObject    *Args
  )
{
  return NULL;
}

STATIC INT8 DecompressDocs[] = "Decompress(): Decompress data using UEFI standard algorithm\n";
STATIC INT8 CompressDocs[] = "Compress(): Compress data using UEFI standard algorithm\n";

STATIC PyMethodDef EfiCompressor_Funcs[] = {
  {"UefiDecompress", (PyCFunction)UefiDecompress, METH_VARARGS, DecompressDocs},
  {"UefiCompress", (PyCFunction)UefiCompress, METH_VARARGS, DecompressDocs},
  {"FrameworkDecompress", (PyCFunction)FrameworkDecompress, METH_VARARGS, DecompressDocs},
  {"FrameworkCompress", (PyCFunction)FrameworkCompress, METH_VARARGS, DecompressDocs},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initEfiCompressor(VOID) {
  Py_InitModule3("EfiCompressor", EfiCompressor_Funcs, "EFI Compression Algorithm Extension Module");
}


