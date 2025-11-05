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
        self.tmpdir = tempfile.mkdtemp()
        self.binary_file = os.path.join(self.tmpdir, "Binary.bin")
        self.create_inputfile()

    def tearDown(self):
        if os.path.exists(self.tmpdir):
            shutil.rmtree(self.tmpdir)

    def test_splitFile_position(self):
        position = [-1, 0, 256, 512, 700, 1024, 2048]
        result = [(0, 1024), (0, 1024), (256, 768),
                  (512, 512), (700, 324), (1024, 0), (1024, 0)]
        outputfolder = self.tmpdir
        for index, po in enumerate(position):
            try:
                sp.splitFile(self.binary_file, po)
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")

            output1 = os.path.join(outputfolder, "Binary.bin1")
            output2 = os.path.join(outputfolder, "Binary.bin2")
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
        output = [
            None,
            "Binary.bin",
            "Binary1.bin",
            r"output/Binary1.bin",
            os.path.abspath( r"output/Binary1.bin")
            ]
        expected_output = [
            os.path.join(os.path.dirname(self.binary_file),"Binary.bin1" ),
            os.path.join(os.getcwd(),"Binary.bin"),
            os.path.join(os.getcwd(),"Binary1.bin"),
            os.path.join(os.getcwd(),r"output/Binary1.bin"),
            os.path.join(os.path.abspath( r"output/Binary1.bin"))
            ]
        for index, o in enumerate(output):
            try:
                sp.splitFile(self.binary_file, 123, outputfile1=o)
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")

            self.assertTrue(os.path.exists(expected_output[index]))
            self.create_inputfile()

    def test_splitFile_outputfolder(self):
        outputfolder = [
            None,
            "output",
            r"output1/output2",
            os.path.abspath("output"),
            "output"
            ]
        output = [
            None,
            None,
            "Binary1.bin",
            r"output/Binary1.bin",
            os.path.abspath( r"output_1/Binary1.bin")
            ]

        expected_output = [
            os.path.join(os.path.dirname(self.binary_file),"Binary.bin1" ),
            os.path.join(os.getcwd(),"output", "Binary.bin1"),
            os.path.join(os.getcwd(), r"output1/output2" , "Binary1.bin"),
            os.path.join(os.getcwd(),r"output", "output/Binary1.bin"),
            os.path.join(os.path.abspath( r"output/Binary1.bin"))
            ]

        for index, o in enumerate(outputfolder):
            try:
                sp.splitFile(self.binary_file, 123, outputdir=o,outputfile1=output[index])
            except Exception as e:
                self.assertTrue(False, msg="splitFile function error")

            self.assertTrue(os.path.exists(expected_output[index]))
            self.create_inputfile()


if __name__ == '__main__':
    unittest.main()
