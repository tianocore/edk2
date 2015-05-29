/*
 * Copyright (c) 2014, Linaro Ltd. All rights reserved.
 *
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */

/*
 * Theory of operation
 * -------------------
 *
 * This code parses a Flattened Device Tree binary (DTB) to find the base of
 * system RAM. It is written in assembly so that it can be executed before a
 * stack has been set up.
 *
 * To find the base of system RAM, we have to traverse the FDT to find a memory
 * node. In the context of this implementation, the first node that has a
 * device_type property with the value 'memory' and a 'reg' property is
 * acceptable, and the name of the node (memory[@xxx]) is ignored, as are any
 * other nodes that match the above constraints.
 *
 * In pseudo code, this implementation does the following:
 *
 * for each node {
 *  have_device_type = false
 *  have_reg = false
 *
 *  for each property {
 *    if property value == 'memory' {
 *      if property name == 'device_type' {
 *        have_device_type = true
 *      }
 *    } else {
 *      if property name == 'reg' {
 *        have_reg = true
 *        membase = property value[0]
 *        memsize = property value[1]
 *      }
 *    }
 *  }
 *  if have_device_type and have_reg {
 *    return membase and memsize
 *  }
 * }
 * return NOT_FOUND
 */

#define FDT_MAGIC    0xedfe0dd0

#define FDT_BEGIN_NODE    0x1
#define FDT_END_NODE    0x2
#define FDT_PROP    0x3
#define FDT_END      0x9

  xMEMSIZE  .req  x0  // recorded system RAM size
  xMEMBASE  .req  x1  // recorded system RAM base

  xLR    .req  x8  // our preserved link register
  xDTP    .req  x9  // pointer to traverse the DT structure
  xSTRTAB    .req  x10  // pointer to the DTB string table
  xMEMNODE  .req  x11  // bit field to record found properties

#define HAVE_REG    0x1
#define HAVE_DEVICE_TYPE  0x2

  .text
  .align  3
_memory:
  .asciz  "memory"
_reg:
  .asciz  "reg"
_device_type:
  .asciz  "device_type"

  /*
   * Compare strings in x4 and x5, return in w7
   */
  .align  3
strcmp:
  ldrb  w2, [x4], #1
  ldrb  w3, [x5], #1
  subs  w7, w2, w3
  cbz  w2, 0f
  cbz  w3, 0f
  beq  strcmp
0:  ret

  .globl  find_memnode
find_memnode:
  // preserve link register
  mov  xLR, x30
  mov  xDTP, x0

  /*
   * Check the DTB magic at offset 0
   */
  movz  w4, #:abs_g0_nc:FDT_MAGIC
  movk  w4, #:abs_g1:FDT_MAGIC
  ldr  w5, [xDTP]
  cmp  w4, w5
  bne  err_invalid_magic

  /*
   * Read the string offset and store it for later use
   */
  ldr  w4, [xDTP, #12]
  rev  w4, w4
  add  xSTRTAB, xDTP, x4

  /*
   * Read the struct offset and add it to the DT pointer
   */
  ldr  w5, [xDTP, #8]
  rev  w5, w5
  add  xDTP, xDTP, x5

  /*
   * Check current tag for FDT_BEGIN_NODE
   */
  ldr  w5, [xDTP]
  rev  w5, w5
  cmp  w5, #FDT_BEGIN_NODE
  bne  err_unexpected_begin_tag

begin_node:
  mov  xMEMNODE, #0
  add  xDTP, xDTP, #4

  /*
   * Advance xDTP past NULL terminated string
   */
0:  ldrb  w4, [xDTP], #1
  cbnz  w4, 0b

next_tag:
  /*
   * Align the DT pointer xDTP to the next 32-bit boundary
   */
  add  xDTP, xDTP, #3
  and  xDTP, xDTP, #~3

  /*
   * Read the next tag, could be BEGIN_NODE, END_NODE, PROP, END
   */
  ldr  w5, [xDTP]
  rev  w5, w5
  cmp  w5, #FDT_BEGIN_NODE
  beq  begin_node
  cmp  w5, #FDT_END_NODE
  beq  end_node
  cmp  w5, #FDT_PROP
  beq  prop_node
  cmp  w5, #FDT_END
  beq  err_end_of_fdt
  b  err_unexpected_tag

prop_node:
  /*
   * If propname == 'reg', record as membase and memsize
   * If propname == 'device_type' and value == 'memory',
   * set the 'is_memnode' flag for this node
   */
  ldr  w6, [xDTP, #4]
  add  xDTP, xDTP, #12
  rev  w6, w6
  mov  x5, xDTP
  adr  x4, _memory
  bl  strcmp

  /*
   * Get handle to property name
   */
  ldr  w5, [xDTP, #-4]
  rev  w5, w5
  add  x5, xSTRTAB, x5

  cbz  w7, check_device_type

  /*
   * Check for 'reg' property
   */
  adr  x4, _reg
  bl  strcmp
  cbnz  w7, inc_and_next_tag

  /*
   * Extract two 64-bit quantities from the 'reg' property. These values
   * will only be used if the node also turns out to have a device_type
   * property with a value of 'memory'.
   *
   * NOTE: xDTP is only guaranteed to be 32 bit aligned, and we are most
   *       likely executing with the MMU off, so we cannot use 64 bit
   *       wide accesses here.
   */
  ldp  w4, w5, [xDTP]
  orr  xMEMBASE, x4, x5, lsl #32
  ldp  w4, w5, [xDTP, #8]
  orr  xMEMSIZE, x4, x5, lsl #32
  rev  xMEMBASE, xMEMBASE
  rev  xMEMSIZE, xMEMSIZE
  orr  xMEMNODE, xMEMNODE, #HAVE_REG
  b  inc_and_next_tag

check_device_type:
  /*
   * Check whether the current property's name is 'device_type'
   */
  adr  x4, _device_type
  bl  strcmp
  cbnz  w7, inc_and_next_tag
  orr  xMEMNODE, xMEMNODE, #HAVE_DEVICE_TYPE

inc_and_next_tag:
  add  xDTP, xDTP, x6
  b  next_tag

end_node:
  /*
   * Check for device_type = memory and reg = xxxx
   * If we have both, we are done
   */
  add  xDTP, xDTP, #4
  cmp  xMEMNODE, #(HAVE_REG | HAVE_DEVICE_TYPE)
  bne  next_tag

  ret  xLR

err_invalid_magic:
err_unexpected_begin_tag:
err_unexpected_tag:
err_end_of_fdt:
  wfi
