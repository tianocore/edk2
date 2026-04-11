/*
 * Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

/*
 * Intel ACPI Component Architecture
 * AML Disassembler version 20100528
 *
 * Disassembly of DSDT.dat, Sat Apr 18 15:41:05 2015
 *
 *
 * Original Table Header:
 *     Signature        "DSDT"
 *     Length           0x000008FA (2298)
 *     Revision         0x02
 *     Checksum         0xC4
 *     OEM ID           "BHYVE "
 *     OEM Table ID     "BVDSDT  "
 *     OEM Revision     0x00000001 (1)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20150204 (538247684)
 */
DefinitionBlock ("DSDT.aml", "DSDT", 2, "BHYVE", "BVDSDT", 0x00000001)
{
    Name (_S5, Package (0x02)
    {
        0x05,
        Zero
    })
    Name (PICM, Zero)
    Method (_PIC, 1, NotSerialized)
    {
        Store (Arg0, PICM)
    }

    Scope (_SB)
    {
        Device (PC00)
        {
            Name (_HID, EisaId ("PNP0A03"))
            Name (_ADR, Zero)
            Method (_BBN, 0, NotSerialized)
            {
                Return (Zero)
            }

            Name (_CRS, ResourceTemplate ()
            {
                WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
                    0x0000,             // Granularity
                    0x0000,             // Range Minimum
                    0x00FF,             // Range Maximum
                    0x0000,             // Translation Offset
                    0x0100,             // Length
                    ,, )
                IO (Decode16,
                    0x03C0,             // Range Minimum
                    0x03C0,             // Range Maximum
                    0x00,               // Alignment
                    0x20,               // Length
                    )
                IO (Decode16,
                    0x0CF8,             // Range Minimum
                    0x0CF8,             // Range Maximum
                    0x01,               // Alignment
                    0x08,               // Length
                    )
                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                    0x0000,             // Granularity
                    0x0000,             // Range Minimum
                    0x0CF7,             // Range Maximum
                    0x0000,             // Translation Offset
                    0x0CF8,             // Length
                    ,, , TypeStatic)
                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                    0x0000,             // Granularity
                    0x0D00,             // Range Minimum
                    0xFFFF,             // Range Maximum
                    0x0000,             // Translation Offset
                    0xF300,             // Length
                    ,, , TypeStatic)
                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, Cacheable, ReadWrite,
                    0x00000000,         // Granularity
                    0x000A0000,         // Range Minimum
                    0x000BFFFF,         // Range Maximum
                    0x00000000,         // Translation Offset
                    0x00020000,         // Length
                    ,, , AddressRangeMemory, TypeStatic)
                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                    0x00000000,         // Granularity
                    0xC0000000,         // Range Minimum
                    0xDFFFFFFF,         // Range Maximum
                    0x00000000,         // Translation Offset
                    0x20000000,         // Range Length
                    ,, PW32)
                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                    0x00000000,         // Granularity
                    0xF0000000,         // Range Minimum
                    0xF07FFFFF,         // Range Maximum
                    0x00000000,         // Translation Offset
                    0x00800000,         // Range Length
                    ,, FB32)
                QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                    0x0000000000000000, // Granularity
                    0x000000D000000000, // Range Minimum
                    0x000000D0000FFFFF, // Range Maximum
                    0x0000000000000000, // Translation Offset
                    0x0000000000100000, // Length
                    ,, , AddressRangeMemory, TypeStatic)
            })
            Name (PPRT, Package ()
            {
                Package () { 0x0000FFFF, 0, LPC.LNKF, Zero },
                Package () { 0x0000FFFF, 1, LPC.LNKG, Zero },
                Package () { 0x0000FFFF, 2, LPC.LNKH, Zero },
                Package () { 0x0000FFFF, 3, LPC.LNKA, Zero },

                Package () { 0x0001FFFF, 0, LPC.LNKG, Zero },
                Package () { 0x0001FFFF, 1, LPC.LNKH, Zero },
                Package () { 0x0001FFFF, 2, LPC.LNKA, Zero },
                Package () { 0x0001FFFF, 3, LPC.LNKB, Zero },

                Package () { 0x0002FFFF, 0, LPC.LNKH, Zero },
                Package () { 0x0002FFFF, 1, LPC.LNKA, Zero },
                Package () { 0x0002FFFF, 2, LPC.LNKB, Zero },
                Package () { 0x0002FFFF, 3, LPC.LNKC, Zero },

                Package () { 0x0003FFFF, 0, LPC.LNKA, Zero },
                Package () { 0x0003FFFF, 1, LPC.LNKB, Zero },
                Package () { 0x0003FFFF, 2, LPC.LNKC, Zero },
                Package () { 0x0003FFFF, 3, LPC.LNKD, Zero },

                Package () { 0x0004FFFF, 0, LPC.LNKB, Zero },
                Package () { 0x0004FFFF, 1, LPC.LNKC, Zero },
                Package () { 0x0004FFFF, 2, LPC.LNKD, Zero },
                Package () { 0x0004FFFF, 3, LPC.LNKE, Zero },

                Package () { 0x0005FFFF, 0, LPC.LNKC, Zero },
                Package () { 0x0005FFFF, 1, LPC.LNKD, Zero },
                Package () { 0x0005FFFF, 2, LPC.LNKE, Zero },
                Package () { 0x0005FFFF, 3, LPC.LNKF, Zero },

                Package () { 0x0006FFFF, 0, LPC.LNKD, Zero },
                Package () { 0x0006FFFF, 1, LPC.LNKE, Zero },
                Package () { 0x0006FFFF, 2, LPC.LNKF, Zero },
                Package () { 0x0006FFFF, 3, LPC.LNKG, Zero },

                Package () { 0x0007FFFF, 0, LPC.LNKE, Zero },
                Package () { 0x0007FFFF, 1, LPC.LNKF, Zero },
                Package () { 0x0007FFFF, 2, LPC.LNKG, Zero },
                Package () { 0x0007FFFF, 3, LPC.LNKH, Zero },

                Package () { 0x0008FFFF, 0, LPC.LNKF, Zero },
                Package () { 0x0008FFFF, 1, LPC.LNKG, Zero },
                Package () { 0x0008FFFF, 2, LPC.LNKH, Zero },
                Package () { 0x0008FFFF, 3, LPC.LNKA, Zero },

                Package () { 0x0009FFFF, 0, LPC.LNKG, Zero },
                Package () { 0x0009FFFF, 1, LPC.LNKH, Zero },
                Package () { 0x0009FFFF, 2, LPC.LNKA, Zero },
                Package () { 0x0009FFFF, 3, LPC.LNKB, Zero },

                Package () { 0x000AFFFF, 0, LPC.LNKH, Zero },
                Package () { 0x000AFFFF, 1, LPC.LNKA, Zero },
                Package () { 0x000AFFFF, 2, LPC.LNKB, Zero },
                Package () { 0x000AFFFF, 3, LPC.LNKC, Zero },

                Package () { 0x000BFFFF, 0, LPC.LNKA, Zero },
                Package () { 0x000BFFFF, 1, LPC.LNKB, Zero },
                Package () { 0x000BFFFF, 2, LPC.LNKC, Zero },
                Package () { 0x000BFFFF, 3, LPC.LNKD, Zero },

                Package () { 0x000CFFFF, 0, LPC.LNKB, Zero },
                Package () { 0x000CFFFF, 1, LPC.LNKC, Zero },
                Package () { 0x000CFFFF, 2, LPC.LNKD, Zero },
                Package () { 0x000CFFFF, 3, LPC.LNKE, Zero },

                Package () { 0x000DFFFF, 0, LPC.LNKC, Zero },
                Package () { 0x000DFFFF, 1, LPC.LNKD, Zero },
                Package () { 0x000DFFFF, 2, LPC.LNKE, Zero },
                Package () { 0x000DFFFF, 3, LPC.LNKF, Zero },

                Package () { 0x000EFFFF, 0, LPC.LNKD, Zero },
                Package () { 0x000EFFFF, 1, LPC.LNKE, Zero },
                Package () { 0x000EFFFF, 2, LPC.LNKF, Zero },
                Package () { 0x000EFFFF, 3, LPC.LNKG, Zero },

                Package () { 0x000FFFFF, 0, LPC.LNKE, Zero },
                Package () { 0x000FFFFF, 1, LPC.LNKF, Zero },
                Package () { 0x000FFFFF, 2, LPC.LNKG, Zero },
                Package () { 0x000FFFFF, 3, LPC.LNKH, Zero },

                Package () { 0x0010FFFF, 0, LPC.LNKF, Zero },
                Package () { 0x0010FFFF, 1, LPC.LNKG, Zero },
                Package () { 0x0010FFFF, 2, LPC.LNKH, Zero },
                Package () { 0x0010FFFF, 3, LPC.LNKA, Zero },

                Package () { 0x0011FFFF, 0, LPC.LNKG, Zero },
                Package () { 0x0011FFFF, 1, LPC.LNKH, Zero },
                Package () { 0x0011FFFF, 2, LPC.LNKA, Zero },
                Package () { 0x0011FFFF, 3, LPC.LNKB, Zero },

                Package () { 0x0012FFFF, 0, LPC.LNKH, Zero },
                Package () { 0x0012FFFF, 1, LPC.LNKA, Zero },
                Package () { 0x0012FFFF, 2, LPC.LNKB, Zero },
                Package () { 0x0012FFFF, 3, LPC.LNKC, Zero },

                Package () { 0x0013FFFF, 0, LPC.LNKA, Zero },
                Package () { 0x0013FFFF, 1, LPC.LNKB, Zero },
                Package () { 0x0013FFFF, 2, LPC.LNKC, Zero },
                Package () { 0x0013FFFF, 3, LPC.LNKD, Zero },

                Package () { 0x0014FFFF, 0, LPC.LNKB, Zero },
                Package () { 0x0014FFFF, 1, LPC.LNKC, Zero },
                Package () { 0x0014FFFF, 2, LPC.LNKD, Zero },
                Package () { 0x0014FFFF, 3, LPC.LNKE, Zero },

                Package () { 0x0015FFFF, 0, LPC.LNKC, Zero },
                Package () { 0x0015FFFF, 1, LPC.LNKD, Zero },
                Package () { 0x0015FFFF, 2, LPC.LNKE, Zero },
                Package () { 0x0015FFFF, 3, LPC.LNKF, Zero },

                Package () { 0x0016FFFF, 0, LPC.LNKD, Zero },
                Package () { 0x0016FFFF, 1, LPC.LNKE, Zero },
                Package () { 0x0016FFFF, 2, LPC.LNKF, Zero },
                Package () { 0x0016FFFF, 3, LPC.LNKG, Zero },

                Package () { 0x0017FFFF, 0, LPC.LNKE, Zero },
                Package () { 0x0017FFFF, 1, LPC.LNKF, Zero },
                Package () { 0x0017FFFF, 2, LPC.LNKG, Zero },
                Package () { 0x0017FFFF, 3, LPC.LNKH, Zero },

                Package () { 0x0018FFFF, 0, LPC.LNKF, Zero },
                Package () { 0x0018FFFF, 1, LPC.LNKG, Zero },
                Package () { 0x0018FFFF, 2, LPC.LNKH, Zero },
                Package () { 0x0018FFFF, 3, LPC.LNKA, Zero },

                Package () { 0x0019FFFF, 0, LPC.LNKG, Zero },
                Package () { 0x0019FFFF, 1, LPC.LNKH, Zero },
                Package () { 0x0019FFFF, 2, LPC.LNKA, Zero },
                Package () { 0x0019FFFF, 3, LPC.LNKB, Zero },

                Package () { 0x001AFFFF, 0, LPC.LNKH, Zero },
                Package () { 0x001AFFFF, 1, LPC.LNKA, Zero },
                Package () { 0x001AFFFF, 2, LPC.LNKB, Zero },
                Package () { 0x001AFFFF, 3, LPC.LNKC, Zero },

                Package () { 0x001BFFFF, 0, LPC.LNKA, Zero },
                Package () { 0x001BFFFF, 1, LPC.LNKB, Zero },
                Package () { 0x001BFFFF, 2, LPC.LNKC, Zero },
                Package () { 0x001BFFFF, 3, LPC.LNKD, Zero },

                Package () { 0x001CFFFF, 0, LPC.LNKB, Zero },
                Package () { 0x001CFFFF, 1, LPC.LNKC, Zero },
                Package () { 0x001CFFFF, 2, LPC.LNKD, Zero },
                Package () { 0x001CFFFF, 3, LPC.LNKE, Zero },

                Package () { 0x001DFFFF, 0, LPC.LNKC, Zero },
                Package () { 0x001DFFFF, 1, LPC.LNKD, Zero },
                Package () { 0x001DFFFF, 2, LPC.LNKE, Zero },
                Package () { 0x001DFFFF, 3, LPC.LNKF, Zero },

                Package () { 0x001EFFFF, 0, LPC.LNKD, Zero },
                Package () { 0x001EFFFF, 1, LPC.LNKE, Zero },
                Package () { 0x001EFFFF, 2, LPC.LNKF, Zero },
                Package () { 0x001EFFFF, 3, LPC.LNKG, Zero },

                Package () { 0x001FFFFF, 0, LPC.LNKE, Zero },
                Package () { 0x001FFFFF, 1, LPC.LNKF, Zero },
                Package () { 0x001FFFFF, 2, LPC.LNKG, Zero },
                Package () { 0x001FFFFF, 3, LPC.LNKH, Zero }
            })
            Name (APRT, Package ()
            {
                Package () { 0x0000FFFF, 0, Zero, 0x15 },
                Package () { 0x0000FFFF, 1, Zero, 0x16 },
                Package () { 0x0000FFFF, 2, Zero, 0x17 },
                Package () { 0x0000FFFF, 3, Zero, 0x10 },

                Package () { 0x0001FFFF, 0, Zero, 0x16 },
                Package () { 0x0001FFFF, 1, Zero, 0x17 },
                Package () { 0x0001FFFF, 2, Zero, 0x10 },
                Package () { 0x0001FFFF, 3, Zero, 0x11 },

                Package () { 0x0002FFFF, 0, Zero, 0x17 },
                Package () { 0x0002FFFF, 1, Zero, 0x10 },
                Package () { 0x0002FFFF, 2, Zero, 0x11 },
                Package () { 0x0002FFFF, 3, Zero, 0x12 },

                Package () { 0x0003FFFF, 0, Zero, 0x10 },
                Package () { 0x0003FFFF, 1, Zero, 0x11 },
                Package () { 0x0003FFFF, 2, Zero, 0x12 },
                Package () { 0x0003FFFF, 3, Zero, 0x13 },

                Package () { 0x0004FFFF, 0, Zero, 0x11 },
                Package () { 0x0004FFFF, 1, Zero, 0x12 },
                Package () { 0x0004FFFF, 2, Zero, 0x13 },
                Package () { 0x0004FFFF, 3, Zero, 0x14 },

                Package () { 0x0005FFFF, 0, Zero, 0x12 },
                Package () { 0x0005FFFF, 1, Zero, 0x13 },
                Package () { 0x0005FFFF, 2, Zero, 0x14 },
                Package () { 0x0005FFFF, 3, Zero, 0x15 },

                Package () { 0x0006FFFF, 0, Zero, 0x13 },
                Package () { 0x0006FFFF, 1, Zero, 0x14 },
                Package () { 0x0006FFFF, 2, Zero, 0x15 },
                Package () { 0x0006FFFF, 3, Zero, 0x16 },

                Package () { 0x0007FFFF, 0, Zero, 0x14 },
                Package () { 0x0007FFFF, 1, Zero, 0x15 },
                Package () { 0x0007FFFF, 2, Zero, 0x16 },
                Package () { 0x0007FFFF, 3, Zero, 0x17 },

                Package () { 0x0008FFFF, 0, Zero, 0x15 },
                Package () { 0x0008FFFF, 1, Zero, 0x16 },
                Package () { 0x0008FFFF, 2, Zero, 0x17 },
                Package () { 0x0008FFFF, 3, Zero, 0x10 },

                Package () { 0x0009FFFF, 0, Zero, 0x16 },
                Package () { 0x0009FFFF, 1, Zero, 0x17 },
                Package () { 0x0009FFFF, 2, Zero, 0x10 },
                Package () { 0x0009FFFF, 3, Zero, 0x11 },

                Package () { 0x000AFFFF, 0, Zero, 0x17 },
                Package () { 0x000AFFFF, 1, Zero, 0x10 },
                Package () { 0x000AFFFF, 2, Zero, 0x11 },
                Package () { 0x000AFFFF, 3, Zero, 0x12 },

                Package () { 0x000BFFFF, 0, Zero, 0x10 },
                Package () { 0x000BFFFF, 1, Zero, 0x11 },
                Package () { 0x000BFFFF, 2, Zero, 0x12 },
                Package () { 0x000BFFFF, 3, Zero, 0x13 },

                Package () { 0x000CFFFF, 0, Zero, 0x11 },
                Package () { 0x000CFFFF, 1, Zero, 0x12 },
                Package () { 0x000CFFFF, 2, Zero, 0x13 },
                Package () { 0x000CFFFF, 3, Zero, 0x14 },

                Package () { 0x000DFFFF, 0, Zero, 0x12 },
                Package () { 0x000DFFFF, 1, Zero, 0x13 },
                Package () { 0x000DFFFF, 2, Zero, 0x14 },
                Package () { 0x000DFFFF, 3, Zero, 0x15 },

                Package () { 0x000EFFFF, 0, Zero, 0x13 },
                Package () { 0x000EFFFF, 1, Zero, 0x14 },
                Package () { 0x000EFFFF, 2, Zero, 0x15 },
                Package () { 0x000EFFFF, 3, Zero, 0x16 },

                Package () { 0x000FFFFF, 0, Zero, 0x14 },
                Package () { 0x000FFFFF, 1, Zero, 0x15 },
                Package () { 0x000FFFFF, 2, Zero, 0x16 },
                Package () { 0x000FFFFF, 3, Zero, 0x17 },

                Package () { 0x0010FFFF, 0, Zero, 0x15 },
                Package () { 0x0010FFFF, 1, Zero, 0x16 },
                Package () { 0x0010FFFF, 2, Zero, 0x17 },
                Package () { 0x0010FFFF, 3, Zero, 0x10 },

                Package () { 0x0011FFFF, 0, Zero, 0x16 },
                Package () { 0x0011FFFF, 1, Zero, 0x17 },
                Package () { 0x0011FFFF, 2, Zero, 0x10 },
                Package () { 0x0011FFFF, 3, Zero, 0x11 },

                Package () { 0x0012FFFF, 0, Zero, 0x17 },
                Package () { 0x0012FFFF, 1, Zero, 0x10 },
                Package () { 0x0012FFFF, 2, Zero, 0x11 },
                Package () { 0x0012FFFF, 3, Zero, 0x12 },

                Package () { 0x0013FFFF, 0, Zero, 0x10 },
                Package () { 0x0013FFFF, 1, Zero, 0x11 },
                Package () { 0x0013FFFF, 2, Zero, 0x12 },
                Package () { 0x0013FFFF, 3, Zero, 0x13 },

                Package () { 0x0014FFFF, 0, Zero, 0x11 },
                Package () { 0x0014FFFF, 1, Zero, 0x12 },
                Package () { 0x0014FFFF, 2, Zero, 0x13 },
                Package () { 0x0014FFFF, 3, Zero, 0x14 },

                Package () { 0x0015FFFF, 0, Zero, 0x12 },
                Package () { 0x0015FFFF, 1, Zero, 0x13 },
                Package () { 0x0015FFFF, 2, Zero, 0x14 },
                Package () { 0x0015FFFF, 3, Zero, 0x15 },

                Package () { 0x0016FFFF, 0, Zero, 0x13 },
                Package () { 0x0016FFFF, 1, Zero, 0x14 },
                Package () { 0x0016FFFF, 2, Zero, 0x15 },
                Package () { 0x0016FFFF, 3, Zero, 0x16 },

                Package () { 0x0017FFFF, 0, Zero, 0x14 },
                Package () { 0x0017FFFF, 1, Zero, 0x15 },
                Package () { 0x0017FFFF, 2, Zero, 0x16 },
                Package () { 0x0017FFFF, 3, Zero, 0x17 },

                Package () { 0x0018FFFF, 0, Zero, 0x15 },
                Package () { 0x0018FFFF, 1, Zero, 0x16 },
                Package () { 0x0018FFFF, 2, Zero, 0x17 },
                Package () { 0x0018FFFF, 3, Zero, 0x10 },

                Package () { 0x0019FFFF, 0, Zero, 0x16 },
                Package () { 0x0019FFFF, 1, Zero, 0x17 },
                Package () { 0x0019FFFF, 2, Zero, 0x10 },
                Package () { 0x0019FFFF, 3, Zero, 0x11 },

                Package () { 0x001AFFFF, 0, Zero, 0x17 },
                Package () { 0x001AFFFF, 1, Zero, 0x10 },
                Package () { 0x001AFFFF, 2, Zero, 0x11 },
                Package () { 0x001AFFFF, 3, Zero, 0x12 },

                Package () { 0x001BFFFF, 0, Zero, 0x10 },
                Package () { 0x001BFFFF, 1, Zero, 0x11 },
                Package () { 0x001BFFFF, 2, Zero, 0x12 },
                Package () { 0x001BFFFF, 3, Zero, 0x13 },

                Package () { 0x001CFFFF, 0, Zero, 0x11 },
                Package () { 0x001CFFFF, 1, Zero, 0x12 },
                Package () { 0x001CFFFF, 2, Zero, 0x13 },
                Package () { 0x001CFFFF, 3, Zero, 0x14 },

                Package () { 0x001DFFFF, 0, Zero, 0x12 },
                Package () { 0x001DFFFF, 1, Zero, 0x13 },
                Package () { 0x001DFFFF, 2, Zero, 0x14 },
                Package () { 0x001DFFFF, 3, Zero, 0x15 },

                Package () { 0x001EFFFF, 0, Zero, 0x13 },
                Package () { 0x001EFFFF, 1, Zero, 0x14 },
                Package () { 0x001EFFFF, 2, Zero, 0x15 },
                Package () { 0x001EFFFF, 3, Zero, 0x16 },

                Package () { 0x001FFFFF, 0, Zero, 0x14 },
                Package () { 0x001FFFFF, 1, Zero, 0x15 },
                Package () { 0x001FFFFF, 2, Zero, 0x16 },
                Package () { 0x001FFFFF, 3, Zero, 0x17 }
            })
            Method (_PRT, 0, NotSerialized)
            {
                If (PICM)
                {
                    Return (APRT)
                }
                Else
                {
                    Return (PPRT)
                }
            }

            Device (LPC)
            {
                Name (_ADR, 0x001F0000)
                OperationRegion (LPCR, PCI_Config, Zero, 0x0100)
                Field (LPCR, AnyAcc, NoLock, Preserve)
                {
                    Offset (0x60),
                    PIRA,   8,
                    PIRB,   8,
                    PIRC,   8,
                    PIRD,   8,
                    Offset (0x68),
                    PIRE,   8,
                    PIRF,   8,
                    PIRG,   8,
                    PIRH,   8
                }

                Device (KBD)
                {
                  Name (_HID, EISAID ("PNP0303"))
                  Name (_CID, EISAID ("PNP030B"))
                  Name (_CRS, ResourceTemplate ()
                  {
                    IO (Decode16,
                        0x0060,             // Range Minimum
                        0x0060,             // Range Maximum
                        0x00,               // Alignment
                        0x01,               // Length
                        )
                    IO (Decode16,
                        0x0064,             // Range Minimum
                        0x0064,             // Range Maximum
                        0x00,               // Alignment
                        0x01,               // Length
                        )
                    IRQNoFlags ()
                        {1}
                  })
                }

                Device (MOU)
                {
                  Name (_HID, EISAID ("PNP0F03"))
                  Name (_CID, EISAID ("PNP0F13"))
                  Name (_CRS, ResourceTemplate ()
                  {
                    IRQNoFlags ()
                        {12}
                  })
                }

                Method (PIRV, 1, NotSerialized)
                {
                    If (And (Arg0, 0x80))
                    {
                        Return (Zero)
                    }

                    And (Arg0, 0x0F, Local0)
                    If (LLess (Local0, 0x03))
                    {
                        Return (Zero)
                    }

                    If (LEqual (Local0, 0x08))
                    {
                        Return (Zero)
                    }

                    If (LEqual (Local0, 0x0D))
                    {
                        Return (Zero)
                    }

                    Return (One)
                }

                Device (LNKA)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, One)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRA))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB01, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB01, One, CIRA)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRA, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRA)
                        }
                        Else
                        {
                            Store (Zero, CIRA)
                        }

                        Return (CB01)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRA)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRA)
                        FindSetRightBit (SIRA, Local0)
                        Store (Decrement (Local0), PIRA)
                    }
                }

                Device (LNKB)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x02)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRB))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB02, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB02, One, CIRB)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRB, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRB)
                        }
                        Else
                        {
                            Store (Zero, CIRB)
                        }

                        Return (CB02)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRB)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRB)
                        FindSetRightBit (SIRB, Local0)
                        Store (Decrement (Local0), PIRB)
                    }
                }

                Device (LNKC)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x03)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRC))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB03, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB03, One, CIRC)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRC, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRC)
                        }
                        Else
                        {
                            Store (Zero, CIRC)
                        }

                        Return (CB03)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRC)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRC)
                        FindSetRightBit (SIRC, Local0)
                        Store (Decrement (Local0), PIRC)
                    }
                }

                Device (LNKD)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x04)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRD))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB04, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB04, One, CIRD)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRD, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRD)
                        }
                        Else
                        {
                            Store (Zero, CIRD)
                        }

                        Return (CB04)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRD)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRD)
                        FindSetRightBit (SIRD, Local0)
                        Store (Decrement (Local0), PIRD)
                    }
                }

                Device (LNKE)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x05)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRE))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB05, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB05, One, CIRE)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRE, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRE)
                        }
                        Else
                        {
                            Store (Zero, CIRE)
                        }

                        Return (CB05)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRE)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRE)
                        FindSetRightBit (SIRE, Local0)
                        Store (Decrement (Local0), PIRE)
                    }
                }

                Device (LNKF)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x06)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRF))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB06, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB06, One, CIRF)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRF, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRF)
                        }
                        Else
                        {
                            Store (Zero, CIRF)
                        }

                        Return (CB06)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRF)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRF)
                        FindSetRightBit (SIRF, Local0)
                        Store (Decrement (Local0), PIRF)
                    }
                }

                Device (LNKG)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x07)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRG))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB07, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB07, One, CIRG)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRG, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRG)
                        }
                        Else
                        {
                            Store (Zero, CIRG)
                        }

                        Return (CB07)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRG)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRG)
                        FindSetRightBit (SIRG, Local0)
                        Store (Decrement (Local0), PIRG)
                    }
                }

                Device (LNKH)
                {
                    Name (_HID, EisaId ("PNP0C0F"))
                    Name (_UID, 0x08)
                    Method (_STA, 0, NotSerialized)
                    {
                        If (PIRV (PIRH))
                        {
                            Return (0x0B)
                        }
                        Else
                        {
                            Return (0x09)
                        }
                    }

                    Name (_PRS, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {3,4,5,6,7,9,10,11,12,14,15}
                    })
                    Name (CB08, ResourceTemplate ()
                    {
                        IRQ (Level, ActiveLow, Shared, )
                            {}
                    })
                    CreateWordField (CB08, One, CIRH)
                    Method (_CRS, 0, NotSerialized)
                    {
                        And (PIRH, 0x8F, Local0)
                        If (PIRV (Local0))
                        {
                            ShiftLeft (One, Local0, CIRH)
                        }
                        Else
                        {
                            Store (Zero, CIRH)
                        }

                        Return (CB08)
                    }

                    Method (_DIS, 0, NotSerialized)
                    {
                        Store (0x80, PIRH)
                    }

                    Method (_SRS, 1, NotSerialized)
                    {
                        CreateWordField (Arg0, One, SIRH)
                        FindSetRightBit (SIRH, Local0)
                        Store (Decrement (Local0), PIRH)
                    }
                }

                Device (SIO)
                {
                    Name (_HID, EisaId ("PNP0C02"))
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0220,             // Range Minimum
                            0x0220,             // Range Maximum
                            0x01,               // Alignment
                            0x04,               // Length
                            )
                        IO (Decode16,
                            0x0224,             // Range Minimum
                            0x0224,             // Range Maximum
                            0x01,               // Alignment
                            0x04,               // Length
                            )
                        Memory32Fixed (ReadWrite,
                            0xE0000000,         // Address Base
                            0x10000000,         // Address Length
                            )
                        IO (Decode16,
                            0x04D0,             // Range Minimum
                            0x04D0,             // Range Maximum
                            0x01,               // Alignment
                            0x02,               // Length
                            )
                        IO (Decode16,
                            0x0061,             // Range Minimum
                            0x0061,             // Range Maximum
                            0x01,               // Alignment
                            0x01,               // Length
                            )
                        IO (Decode16,
                            0x0400,             // Range Minimum
                            0x0400,             // Range Maximum
                            0x01,               // Alignment
                            0x08,               // Length
                            )
                        IO (Decode16,
                            0x00B2,             // Range Minimum
                            0x00B2,             // Range Maximum
                            0x01,               // Alignment
                            0x01,               // Length
                            )
                        IO (Decode16,
                            0x0084,             // Range Minimum
                            0x0084,             // Range Maximum
                            0x01,               // Alignment
                            0x01,               // Length
                            )
                        IO (Decode16,
                            0x0072,             // Range Minimum
                            0x0072,             // Range Maximum
                            0x01,               // Alignment
                            0x06,               // Length
                            )
                    })
                }

                Device (COM1)
                {
                    Name (_HID, EisaId ("PNP0501"))
                    Name (_UID, One)
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x03F8,             // Range Minimum
                            0x03F8,             // Range Maximum
                            0x01,               // Alignment
                            0x08,               // Length
                            )
                        IRQNoFlags ()
                            {4}
                    })
                }

                Device (COM2)
                {
                    Name (_HID, EisaId ("PNP0501"))
                    Name (_UID, 0x02)
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x02F8,             // Range Minimum
                            0x02F8,             // Range Maximum
                            0x01,               // Alignment
                            0x08,               // Length
                            )
                        IRQNoFlags ()
                            {3}
                    })
                }

                Device (RTC)
                {
                    Name (_HID, EisaId ("PNP0B00"))
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0070,             // Range Minimum
                            0x0070,             // Range Maximum
                            0x00,               // Alignment
                            0x02,               // Length
                            )
                        IRQNoFlags ()
                            {8}
                        IO (Decode16,
                            0x0072,             // Range Minimum
                            0x0072,             // Range Maximum
                            0x02,               // Alignment
                            0x06,               // Length
                            )
                    })
                }

                Device (PIC)
                {
                    Name (_HID, EisaId ("PNP0000"))
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0020,             // Range Minimum
                            0x0020,             // Range Maximum
                            0x01,               // Alignment
                            0x02,               // Length
                            )
                        IO (Decode16,
                            0x00A0,             // Range Minimum
                            0x00A0,             // Range Maximum
                            0x01,               // Alignment
                            0x02,               // Length
                            )
                        IRQNoFlags ()
                            {2}
                    })
                }

                Device (TIMR)
                {
                    Name (_HID, EisaId ("PNP0100"))
                    Name (_CRS, ResourceTemplate ()
                    {
                        IO (Decode16,
                            0x0040,             // Range Minimum
                            0x0040,             // Range Maximum
                            0x01,               // Alignment
                            0x04,               // Length
                            )
                        IRQNoFlags ()
                            {0}
                    })
                }
            }
        }
    }

    Scope (_SB.PC00)
    {
        Device (HPET)
        {
            Name (_HID, EisaId ("PNP0103"))
            Name (_UID, Zero)
            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite,
                    0xFED00000,         // Address Base
                    0x00000400,         // Address Length
                    )
            })
        }
    }
}

