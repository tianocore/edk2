/** @file
  I/O FIFO routines

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 

#ifndef _IO_FIFO_H_INCLUDED_
#define _IO_FIFO_H_INCLUDED_

/**
  Reads an 8-bit I/O port fifo into a block of memory.

  Reads the 8-bit I/O fifo port specified by Port.

  The port is read Count times, and the read data is
  stored in the provided Buffer.

  This function must guarantee that all I/O read and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to read.
  @param  Count   The number of times to read I/O port.
  @param  Buffer  The buffer to store the read data into.

**/
VOID
EFIAPI
IoReadFifo8 (
  IN      UINTN                     Port,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  );

/**
  Reads a 16-bit I/O port fifo into a block of memory.

  Reads the 16-bit I/O fifo port specified by Port.

  The port is read Count times, and the read data is
  stored in the provided Buffer.

  This function must guarantee that all I/O read and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to read.
  @param  Count   The number of times to read I/O port.
  @param  Buffer  The buffer to store the read data into.

**/
VOID
EFIAPI
IoReadFifo16 (
  IN      UINTN                     Port,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  );

/**
  Reads a 32-bit I/O port fifo into a block of memory.

  Reads the 32-bit I/O fifo port specified by Port.

  The port is read Count times, and the read data is
  stored in the provided Buffer.

  This function must guarantee that all I/O read and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to read.
  @param  Count   The number of times to read I/O port.
  @param  Buffer  The buffer to store the read data into.

**/
VOID
EFIAPI
IoReadFifo32 (
  IN      UINTN                     Port,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  );

/**
  Writes a block of memory into an 8-bit I/O port fifo.

  Writes the 8-bit I/O fifo port specified by Port.

  The port is written Count times, and the write data is
  retrieved from the provided Buffer.

  This function must guarantee that all I/O write and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to write.
  @param  Count   The number of times to write I/O port.
  @param  Buffer  The buffer to store the write data into.

**/
VOID
EFIAPI
IoWriteFifo8 (
  IN      UINTN                     Port,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  );

/**
  Writes a block of memory into a 16-bit I/O port fifo.

  Writes the 16-bit I/O fifo port specified by Port.

  The port is written Count times, and the write data is
  retrieved from the provided Buffer.

  This function must guarantee that all I/O write and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to write.
  @param  Count   The number of times to write I/O port.
  @param  Buffer  The buffer to store the write data into.

**/
VOID
EFIAPI
IoWriteFifo16 (
  IN      UINTN                     Port,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  );

/**
  Writes a block of memory into a 32-bit I/O port fifo.

  Writes the 32-bit I/O fifo port specified by Port.

  The port is written Count times, and the write data is
  retrieved from the provided Buffer.

  This function must guarantee that all I/O write and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to write.
  @param  Count   The number of times to write I/O port.
  @param  Buffer  The buffer to store the write data into.

**/
VOID
EFIAPI
IoWriteFifo32 (
  IN      UINTN                     Port,
  IN      UINTN                     Count,
  OUT     VOID                      *Buffer
  );

#endif

