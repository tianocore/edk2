# @file
#  Split a file into two pieces at the request offset.
#
#  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

# Import Modules
import unittest
import tempfile
import os
import shutil
import Split.Split as sp
import struct as st


class TestSplit(unittest.TestCase):
    def setUp(self):
        self.WORKSPACE = tempfile.mkdtemp()
        self.binary_file = os.path.join(self.WORKSPACE, "Binary.bin")
        self.create_inputfile()

    def tearDown(self):
        if os.path.exists(self.WORKSPACE):
            shutil.rmtree(self.WORKSPACE)

    def test_splitFile_position(self):
        position = [-1, 0, 256, 512, 700, 1024, 2048]
        result = [(0, 1024), (0, 1024), (256, 768),
                  (512, 512), (700, 324), (1024, 0), (1024, 0)]
        for index, po in enumerate(position):
            try:
                sp.splitFile(self.binary_file, po)
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")

            output1 = os.path.join(self.WORKSPACE, "Binary.bin1")
            output2 = os.path.join(self.WORKSPACE, "Binary.bin2")
            with open(output1, "rb") as f1:
                size1 = len(f1.read())
            with open(output2, "rb") as f2:
                size2 = len(f2.read())

            ex_result = result[index]
            self.assertEqual(size1, ex_result[0])
            self.assertEqual(size2, ex_result[1])

    def create_inputfile(self):
        with open(self.binary_file, "wb") as fout:
            for i in range(512):
                fout.write(st.pack("<H", i))

    def test_splitFile_outputfile(self):
        output = [None, "Binary.bin", "Binary1.bin", r"output/Binary1.bin",
                  os.path.join(self.WORKSPACE, r"output/Binary1.bin")]
        for o in output:
            try:
                sp.splitFile(self.binary_file, 123, outputfile1=o)
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")
            if o is None:
                self.assertTrue(os.path.exists(
                    os.path.join(self.WORKSPACE, "Binary.bin1")))
            else:
                if os.path.isabs(o):
                    self.assertTrue(os.path.exists(o))
                else:
                    self.assertTrue(os.path.exists(
                        os.path.join(self.WORKSPACE, o)))
            self.create_inputfile()

            try:
                sp.splitFile(self.binary_file, 123, outputfile2=o)
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")
            if o is None:
                self.assertTrue(os.path.exists(
                    os.path.join(self.WORKSPACE, "Binary.bin2")))
            else:
                if os.path.isabs(o):
                    self.assertTrue(os.path.exists(o))
                else:
                    self.assertTrue(os.path.exists(
                        os.path.join(self.WORKSPACE, o)))
            self.create_inputfile()

    def test_splitFile_outputfolder(self):
        outputfolder = [None, "output", r"output1/output2",
                        os.path.join(self.WORKSPACE, "output")]
        for o in outputfolder:
            try:
                sp.splitFile(self.binary_file, 123, outputdir=o)
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")

            if o is None:
                self.assertTrue(os.path.exists(
                    os.path.join(self.WORKSPACE, "Binary.bin1")))
            else:
                if os.path.isabs(o):
                    self.assertTrue(os.path.exists(
                        os.path.join(o, "Binary.bin1")))
                else:
                    self.assertTrue(os.path.exists(
                        os.path.join(self.WORKSPACE, o, "Binary.bin1")))


if __name__ == '__main__':
    unittest.main()
