## @file
# Unit tests for the Firmware Module Management Tool.
#
# Copyright (c) Microsoft Corporation.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import logging
import os
import sys
import tempfile
import unittest
import uuid
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import TestTools


_TESTS_DIR = Path(__file__).resolve().parent
_PYTHON_SOURCE = _TESTS_DIR.parent / "Source" / "Python"
_FMMT_SOURCE = _PYTHON_SOURCE / "FMMT"
sys.path.insert(0, str(_FMMT_SOURCE))
sys.path.insert(0, str(_PYTHON_SOURCE))
os.environ.setdefault("FmmtConfPath", "")

# FMMT opens a build log when imported. Unit tests do not need a persistent log.
with mock.patch("logging.FileHandler", return_value=logging.NullHandler()):
    import core.FMMTOperation as FMMTOperation
    from core.BiosTree import BIOSTREE
    from core.BiosTree import FFS_FREE_SPACE
    from core.BiosTree import FFS_TREE
    from core.BiosTree import SEC_FV_TREE
    from core.BiosTreeNode import FreeSpaceNode
    from core.FvHandler import FvHandler
    from FirmwareStorageFormat.Common import ModifyGuidFormat
    from FirmwareStorageFormat.FfsFileHeader import EFI_FFS_FILE_HEADER


class _FirmwareVolumeData:
    def __init__(self, size, free_space=0, block_size=0x40):
        self.Data = b""
        self.Free_Space = free_space
        self.Header = SimpleNamespace(
            Attributes=0,
            BlockMap=[SimpleNamespace(Length=block_size)],
            ExtHeaderOffset=0,
            FileSystemGuid=ModifyGuidFormat(
                "00000000-0000-0000-0000-000000000000"
            ),
            FvLength=size,
        )
        self.Size = size

    def ModCheckSum(self):
        pass

    def ModExtHeaderData(self):
        pass

    def ModFvExt(self):
        pass

    def ModFvSize(self):
        pass


class _TestFvHandler(FvHandler):
    def CompressData(self, target_tree):
        self.Status = True

    def ModifyTest(self, parent_tree, needed_space):
        self.Status = True


class _TestRecursiveFvHandler(FvHandler):
    def CompressData(self, target_tree):
        self.Status = True


def _create_ffs(name, size, data=b""):
    node = BIOSTREE(name)
    node.type = FFS_TREE
    header = EFI_FFS_FILE_HEADER()
    header.Name = ModifyGuidFormat(str(name))
    header.Size[0] = size & 0xFF
    header.Size[1] = (size >> 8) & 0xFF
    header.Size[2] = (size >> 16) & 0xFF
    node.Data = SimpleNamespace(
        Data=data,
        Header=header,
        Name=name,
        PadData=b"",
        Size=size,
    )
    return node


class FmmtReplaceTests(unittest.TestCase):
    def test_replace_multiple_matches_uses_independent_ffs_trees(self):
        target_guid = uuid.UUID("11111111-2222-3333-4444-555555555555")
        targets = [SimpleNamespace(), SimpleNamespace()]
        replacement = SimpleNamespace(
            Data=SimpleNamespace(PadData=b"", Size=0x20),
            Parent=None,
        )

        class _Root:
            def __init__(self):
                self.Findlist = []

            def FindNode(self, name, matches):
                matches.extend(targets)

        class _Parser:
            def __init__(self, root):
                self.FinalData = b"output"
                self.WholeFvTree = root

            def Encapsulation(self, root, compress_status):
                pass

            def ParserFromRoot(self, root, data):
                pass

        input_parser = _Parser(_Root())
        replacement_parser = _Parser(SimpleNamespace(Child=[replacement]))
        replacements = []

        class _Handler:
            def __init__(self, new_ffs, target_ffs):
                replacements.append(new_ffs)
                new_ffs.Parent = target_ffs

            def ReplaceFfs(self):
                return True

        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "input.fd"
            new_ffs_path = Path(directory) / "new.ffs"
            output_path = Path(directory) / "output.fd"
            input_path.write_bytes(b"input")
            new_ffs_path.write_bytes(b"replacement")

            with mock.patch.object(
                FMMTOperation,
                "FMMTParser",
                side_effect=[input_parser, replacement_parser],
            ), mock.patch.object(FMMTOperation, "FvHandler", _Handler):
                FMMTOperation.ReplaceFfs(
                    str(input_path),
                    target_guid,
                    str(new_ffs_path),
                    str(output_path),
                )

        self.assertEqual(2, len(replacements))
        self.assertIsNot(replacements[0], replacements[1])
        self.assertIs(targets[0], replacements[0].Parent)
        self.assertIs(targets[1], replacements[1].Parent)

    def test_smaller_replacement_creates_free_space_node(self):
        prefix = _create_ffs(
            uuid.UUID("10000000-0000-0000-0000-000000000001"), 0x18
        )
        target = _create_ffs(
            uuid.UUID("20000000-0000-0000-0000-000000000002"), 0x40
        )
        replacement = _create_ffs(target.Data.Name, 0x18)
        volume = BIOSTREE("FV")
        volume.type = SEC_FV_TREE
        volume.Data = _FirmwareVolumeData(0x80)
        volume.insertChild(prefix)
        volume.insertChild(target)

        self.assertTrue(_TestFvHandler(replacement, target).ReplaceFfs())

        self.assertIs(replacement, volume.Child[1])
        free_space = volume.Child[-1]
        self.assertEqual(FFS_FREE_SPACE, free_space.type)
        self.assertIsInstance(free_space.Data, FreeSpaceNode)
        self.assertEqual(b"\xFF" * 0x28, free_space.Data.Data)

    def test_growing_nested_fv_preserves_final_ffs(self):
        prefix = _create_ffs(
            uuid.UUID("10000000-0000-0000-0000-000000000001"), 0x18, b"prefix"
        )
        target = _create_ffs(
            uuid.UUID("20000000-0000-0000-0000-000000000002"), 0x20, b"target"
        )
        trailing = _create_ffs(
            uuid.UUID("30000000-0000-0000-0000-000000000003"),
            0x18,
            b"trailing",
        )
        replacement = _create_ffs(target.Data.Name, 0x30, b"replacement")
        volume = BIOSTREE("FV")
        volume.type = SEC_FV_TREE
        volume.Data = _FirmwareVolumeData(0x80)
        volume.insertChild(prefix)
        volume.insertChild(target)
        volume.insertChild(trailing)

        self.assertTrue(_TestFvHandler(replacement, target).ReplaceFfs())

        self.assertEqual(b"trailing", trailing.Data.Data)
        self.assertIn(trailing, volume.Child)
        self.assertIs(replacement, volume.Child[1])
        free_space = volume.Child[-1]
        self.assertEqual(FFS_FREE_SPACE, free_space.type)
        self.assertIsInstance(free_space.Data, FreeSpaceNode)
        self.assertEqual(b"\xFF" * 0x30, free_space.Data.Data)

    def test_block_aligned_growth_of_only_ffs_does_not_crash(self):
        target = _create_ffs(
            uuid.UUID("20000000-0000-0000-0000-000000000002"), 0x20, b"target"
        )
        replacement = _create_ffs(target.Data.Name, 0x60, b"replacement")
        volume = BIOSTREE("FV")
        volume.type = SEC_FV_TREE
        volume.Data = _FirmwareVolumeData(0x40)
        volume.insertChild(target)

        self.assertTrue(_TestFvHandler(replacement, target).ReplaceFfs())

        self.assertEqual([replacement], volume.Child)
        self.assertEqual(0, volume.Data.Free_Space)

    def test_block_aligned_growth_preserves_ffs_order(self):
        prefix = _create_ffs(
            uuid.UUID("10000000-0000-0000-0000-000000000001"), 0x18, b"prefix"
        )
        target = _create_ffs(
            uuid.UUID("20000000-0000-0000-0000-000000000002"), 0x20, b"target"
        )
        trailing = _create_ffs(
            uuid.UUID("30000000-0000-0000-0000-000000000003"),
            0x18,
            b"trailing",
        )
        replacement = _create_ffs(target.Data.Name, 0x60, b"replacement")
        volume = BIOSTREE("FV")
        volume.type = SEC_FV_TREE
        volume.Data = _FirmwareVolumeData(0x80)
        volume.insertChild(prefix)
        volume.insertChild(target)
        volume.insertChild(trailing)

        self.assertTrue(_TestFvHandler(replacement, target).ReplaceFfs())

        self.assertEqual([prefix, replacement, trailing], volume.Child)
        self.assertEqual(0, volume.Data.Free_Space)

    def test_growing_ancestor_fv_preserves_final_ffs(self):
        prefix = _create_ffs(
            uuid.UUID("10000000-0000-0000-0000-000000000001"), 0x18, b"prefix"
        )
        trailing = _create_ffs(
            uuid.UUID("30000000-0000-0000-0000-000000000003"),
            0x18,
            b"trailing",
        )
        volume = BIOSTREE("FV")
        volume.type = SEC_FV_TREE
        volume.Data = _FirmwareVolumeData(0x80)
        volume.insertChild(prefix)
        volume.insertChild(trailing)

        parent = BIOSTREE("ROOT")
        parent.type = FMMTOperation.ROOT_TREE
        parent.insertChild(volume)

        _TestRecursiveFvHandler(None).ModifyTest(volume, 0x10)

        self.assertEqual(b"trailing", trailing.Data.Data)
        self.assertIn(trailing, volume.Child)
        free_space = volume.Child[-1]
        self.assertEqual(FFS_FREE_SPACE, free_space.type)
        self.assertIsInstance(free_space.Data, FreeSpaceNode)
        self.assertEqual(b"\xFF" * 0x30, free_space.Data.Data)


TheTestSuite = TestTools.MakeTheTestSuite(locals())


if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(TheTestSuite())
