/** @file
  Virtio Serial Device specific type and macro definitions.

  Copyright (C) 2013-2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_SERIAL_H_
#define _VIRTIO_SERIAL_H_

#include <IndustryStandard/Virtio095.h>
#include <IndustryStandard/Virtio10.h>

//
// Device Configuration
//
typedef struct {
  UINT16    Cols;
  UINT16    Rows;
  UINT32    MaxPorts;
  UINT32    EmergWrite;
} VIRTIO_SERIAL_CONFIG;

//
// Control Queue
//
typedef struct {
  UINT32    Id;
  UINT16    Event;
  UINT16    Value;
} VIRTIO_SERIAL_CONTROL;

//
// Queue Identifiers
//
#define VIRTIO_SERIAL_Q_RX_PORT0  0
#define VIRTIO_SERIAL_Q_TX_PORT0  1
#define VIRTIO_SERIAL_Q_RX_CTRL   2
#define VIRTIO_SERIAL_Q_TX_CTRL   3
#define VIRTIO_SERIAL_Q_RX_BASE   4
#define VIRTIO_SERIAL_Q_TX_BASE   5

//
// Feature Bits
//
#define VIRTIO_SERIAL_F_SIZE         BIT0
#define VIRTIO_SERIAL_F_MULTIPORT    BIT1
#define VIRTIO_SERIAL_F_EMERG_WRITE  BIT2

//
// Events
//
#define VIRTIO_SERIAL_DEVICE_READY   0
#define VIRTIO_SERIAL_DEVICE_ADD     1
#define VIRTIO_SERIAL_DEVICE_REMOVE  2
#define VIRTIO_SERIAL_PORT_READY     3
#define VIRTIO_SERIAL_CONSOLE_PORT   4
#define VIRTIO_SERIAL_RESIZE         5
#define VIRTIO_SERIAL_PORT_OPEN      6
#define VIRTIO_SERIAL_PORT_NAME      7

#endif /* _VIRTIO_SERIAL_H_ */
