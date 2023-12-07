/*
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * uefi-vars device - API of the virtual device for guest/host communication.
 *
 * copied from qemu.git (include/hw/uefi/var-service-api.h)
 */

#ifndef QEMU_UEFI_VAR_SERVICE_API_H
#define QEMU_UEFI_VAR_SERVICE_API_H

/* isa: io range */
#define UEFI_VARS_IO_BASE  0x520

/* sysbus: fdt node path */
#define UEFI_VARS_FDT_NODE    "qemu-uefi-vars"
#define UEFI_VARS_FDT_COMPAT  "qemu,uefi-vars"

/* registers */
#define UEFI_VARS_REG_MAGIC           0x00         /* 16 bit */
#define UEFI_VARS_REG_CMD_STS         0x02         /* 16 bit */
#define UEFI_VARS_REG_BUFFER_SIZE     0x04         /* 32 bit */
#define UEFI_VARS_REG_BUFFER_ADDR_LO  0x08         /* 32 bit */
#define UEFI_VARS_REG_BUFFER_ADDR_HI  0x0c         /* 32 bit */
#define UEFI_VARS_REGS_SIZE           0x10

/* magic value */
#define UEFI_VARS_MAGIC_VALUE  0xef1

/* command values */
#define UEFI_VARS_CMD_RESET  0x01
#define UEFI_VARS_CMD_MM     0x02

/* status values */
#define UEFI_VARS_STS_SUCCESS              0x00
#define UEFI_VARS_STS_BUSY                 0x01
#define UEFI_VARS_STS_ERR_UNKNOWN          0x10
#define UEFI_VARS_STS_ERR_NOT_SUPPORTED    0x11
#define UEFI_VARS_STS_ERR_BAD_BUFFER_SIZE  0x12

#endif /* QEMU_UEFI_VAR_SERVICE_API_H */
