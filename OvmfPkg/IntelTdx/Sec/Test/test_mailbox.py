#!/usr/bin/env python3
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
"""Execute the SEC AP mailbox assembly without requiring TDX hardware."""

import pathlib
import struct
import subprocess
import tempfile
import unittest

from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_R8,
    UC_X86_REG_R9,
    UC_X86_REG_RAX,
    UC_X86_REG_RBX,
    UC_X86_REG_RCX,
    UC_X86_REG_RDX,
    UC_X86_REG_RIP,
    UC_X86_REG_RSP,
)


ROOT = pathlib.Path(__file__).resolve().parents[3]
CODE = 0x1000000
SCRATCH_START = 0x800000
SCRATCH_END = 0x820000
MAILBOX = 0x80C000
STACK = 0x2000000
TARGET = CODE + 0x8000
COMMAND, APIC_ID, VECTOR = 0, 4, 8
ARRIVING, EXITING = 0x900, 0xA00


class MailboxTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        symbols = (
            "ParkAp",
            "do_wait_loop",
            "do_wait_loop.check_command",
            "do_wait_loop.check_exiting_cnt",
            "do_wait_loop.invalid_target",
        )
        wrapper = f"""
BITS 64
ORG {CODE}
%define MAILBOX_SCRATCH_START {SCRATCH_START}
%define MAILBOX_SCRATCH_END {SCRATCH_END}
%macro tdcall 0
    db 0x66, 0x0f, 0x01, 0xcc
%endmacro
%include "IntelTdxAPs.nasm"
dq {", ".join(symbols)}
"""
        with tempfile.TemporaryDirectory() as directory:
            source = pathlib.Path(directory) / "mailbox.nasm"
            binary = pathlib.Path(directory) / "mailbox.bin"
            source.write_text(wrapper, encoding="ascii")
            subprocess.run(
                [
                    "nasm", "-f", "bin",
                    "-I", str(ROOT / "Include") + "/",
                    "-I", str(ROOT / "IntelTdx/Sec/X64") + "/",
                    "-o", str(binary), str(source),
                ],
                check=True,
            )
            data = binary.read_bytes()
        table_size = len(symbols) * 8
        cls.code = data[:-table_size]
        cls.symbols = dict(zip(symbols, struct.unpack("<5Q", data[-table_size:])))

    def setUp(self):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_64)
        self.uc.mem_map(CODE, 0x10000)
        self.uc.mem_write(CODE, self.code)
        self.uc.mem_write(TARGET, b"\xc3")  # Memory-accept callback: ret.
        self.uc.mem_map(SCRATCH_START - 0x1000, 0x22000)
        self.uc.mem_map(0, 0x1000)
        self.uc.mem_map(STACK, 0x10000)
        self.uc.reg_write(UC_X86_REG_RIP, self.symbols["ParkAp"])
        self.uc.reg_write(UC_X86_REG_RSP, MAILBOX)
        self.uc.reg_write(UC_X86_REG_R8, 2)
        self.uc.reg_write(UC_X86_REG_R9, 1)
        self.stop_at = None
        self.stopped_at = None
        self.halted = False
        self.tdcalls = 0
        self.uc.hook_add(UC_HOOK_CODE, self.instruction)

    def instruction(self, uc, address, size, _context):
        if address == self.stop_at:
            self.stopped_at = address
            uc.emu_stop()
        elif bytes(uc.mem_read(address, 4)) == b"\x66\x0f\x01\xcc":
            self.assertEqual(uc.reg_read(UC_X86_REG_RAX), 1)
            self.tdcalls += 1
            uc.reg_write(UC_X86_REG_R8, 2)
            uc.reg_write(UC_X86_REG_R9, 1)
            uc.reg_write(UC_X86_REG_RIP, address + 4)
        elif bytes(uc.mem_read(address, 1)) == b"\xf4":
            self.halted = True
            uc.emu_stop()

    def write32(self, offset, value):
        self.uc.mem_write(MAILBOX + offset, struct.pack("<I", value))

    def write64(self, offset, value):
        self.uc.mem_write(MAILBOX + offset, struct.pack("<Q", value))

    def read32(self, offset):
        return struct.unpack("<I", self.uc.mem_read(MAILBOX + offset, 4))[0]

    def execute(self, stop_at=None, count=500):
        self.stop_at = stop_at
        self.stopped_at = None
        self.uc.emu_start(self.uc.reg_read(UC_X86_REG_RIP), CODE + len(self.code), count=count)
        if stop_at is not None:
            self.assertEqual(self.stopped_at, stop_at)

    def publish(self, command, target):
        self.write64(VECTOR, target)
        self.write32(COMMAND, command)

    def initialize(self):
        self.write32(ARRIVING, 1)
        self.execute(self.symbols["do_wait_loop.check_command"])

    def test_initial_headers_are_cleared_before_first_read(self):
        for command in (1, 2):
            with self.subTest(command=command):
                self.setUp()
                self.uc.mem_write(MAILBOX, b"\xa5" * 0x1000)
                self.write32(COMMAND, command)
                self.write32(APIC_ID, 0xFFFFFFFE)
                self.write64(VECTOR, (0xA5A5A5A5 << 32) | TARGET)
                self.write32(ARRIVING, 1)
                self.write32(EXITING, 0x12345678)
                self.execute(self.symbols["do_wait_loop.check_command"])
                self.assertEqual(bytes(self.uc.mem_read(MAILBOX, 16)),
                                 struct.pack("<IIQ", 0, 0xFFFFFFFF, 0))
                self.assertEqual(self.read32(ARRIVING), 2)
                self.assertEqual(self.read32(EXITING), 0x12345678)
                self.assertEqual(bytes(self.uc.mem_read(MAILBOX + 0x800, 32)), b"\xa5" * 32)
                self.execute()
                self.assertEqual(self.read32(COMMAND), 0)
                self.assertFalse(self.halted)

    def test_zero_and_scratch_targets_halt_before_stack_switch(self):
        for command in (1, 2):
            for target in (0, SCRATCH_START, MAILBOX + 0x100, SCRATCH_END - 1):
                with self.subTest(command=command, target=hex(target)):
                    self.setUp()
                    self.initialize()
                    self.write64(0x810, STACK + 0x1000)
                    self.write32(0x818, 0x1000)
                    self.publish(command, target)
                    self.execute()
                    self.assertTrue(self.halted)
                    self.assertEqual(self.uc.reg_read(UC_X86_REG_RSP), MAILBOX)

    def test_valid_target_boundaries_and_wakeup_arguments(self):
        for command in (1, 2):
            for target in (SCRATCH_START - 1, SCRATCH_END, TARGET):
                with self.subTest(command=command, target=hex(target)):
                    self.setUp()
                    self.initialize()
                    self.write64(0x800, 0x4000000)
                    self.write64(0x808, 0x6000000)
                    self.write64(0x810, STACK)
                    self.write32(0x818, 0x1000)
                    self.publish(command, target)
                    self.execute(target)
                    self.assertFalse(self.halted)
                    if command == 1:
                        self.assertEqual(self.uc.reg_read(UC_X86_REG_RBX), 0x4000000)
                        self.assertEqual(self.uc.reg_read(UC_X86_REG_RSP), MAILBOX)
                    else:
                        self.assertEqual(self.uc.reg_read(UC_X86_REG_RCX), 1)
                        self.assertEqual(self.uc.reg_read(UC_X86_REG_RDX), 0x4000000)
                        self.assertEqual(self.uc.reg_read(UC_X86_REG_R8), 0x6000000)
                        self.assertEqual(self.uc.reg_read(UC_X86_REG_RSP), STACK + 0x1000 - 16)

    def test_accept_target_check_uses_full_64_bit_address(self):
        self.initialize()
        target = 0x100000000 + MAILBOX
        self.uc.mem_map(target, 0x1000)
        self.write64(0x810, STACK + 0x1000)
        self.publish(2, target)
        self.execute(target)
        self.assertFalse(self.halted)

    def test_accept_rounds_do_not_repeat_initialization(self):
        self.initialize()
        for _round in range(2):
            self.write32(EXITING, 2)
            self.write64(0x810, STACK)
            self.write32(0x818, 0x1000)
            self.publish(2, TARGET)
            self.execute(self.symbols["do_wait_loop.check_exiting_cnt"])
            self.assertEqual(self.read32(EXITING), 1)
            self.assertEqual(self.uc.reg_read(UC_X86_REG_RSP), MAILBOX)
            # BSP's MpSerializeEnd resets the command and counters.
            self.write32(COMMAND, 0)
            self.write32(ARRIVING, 0)
            self.write32(EXITING, 0)
            self.execute(self.symbols["do_wait_loop"])
            self.execute()
            self.assertEqual(self.read32(ARRIVING), 1)
            self.assertEqual(struct.unpack("<Q", self.uc.mem_read(MAILBOX + VECTOR, 8))[0], TARGET)
            self.write32(ARRIVING, 2)
            self.execute(self.symbols["do_wait_loop.check_command"])
        self.assertEqual(self.tdcalls, 2)

    def test_delayed_aps_wait_for_bsp_release(self):
        for cpus in (2, 4, 8):
            with self.subTest(cpus=cpus):
                shared = bytes(0x1000)
                aps = []
                for index in reversed(range(1, cpus)):
                    self.setUp()
                    self.uc.mem_write(MAILBOX, shared)
                    self.uc.reg_write(UC_X86_REG_R8, cpus)
                    self.uc.reg_write(UC_X86_REG_R9, index)
                    self.execute()
                    shared = bytes(self.uc.mem_read(MAILBOX, 0x1000))
                    self.assertEqual(self.read32(COMMAND), 0)
                    self.assertEqual(self.read32(ARRIVING), len(aps) + 1)
                    aps.append(self.uc)
                # BSP releases the arrival gate before publishing the command.
                for ap in aps:
                    self.uc = ap
                    self.uc.mem_write(MAILBOX, shared)
                    self.write32(ARRIVING, cpus)
                    self.publish(1, TARGET)
                    self.execute(TARGET)
                    self.assertEqual(self.read32(COMMAND), 1)
                    self.assertFalse(self.halted)


if __name__ == "__main__":
    unittest.main(verbosity=2)
