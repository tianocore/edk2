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
import struct as st
import filecmp

import os, sys
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
import FspDscBsf2Yaml

YamlHeaderLineLength = 10
HdrFileHeaderLineLength = 32
BsfFileHeaderLineLength = 19

def GenFileWithoutHdr(inputfile, numLineToStrip):
    yaml_file = open(inputfile, "r")
    lines = yaml_file.readlines()
    yaml_file.close()
    del lines[:numLineToStrip]

    noHdrOutputFileName = "no-header-" + inputfile
    stripped_file = open(noHdrOutputFileName, "w")
    for line in lines:
        stripped_file.write(line)
    stripped_file.close()
    return noHdrOutputFileName

class TestFspScripts(unittest.TestCase):
    def test_generateFspHeader_fromDsc(self):
        # Generate HEADER
        cmd = '{} {} HEADER {} {} {}'.format(
            'python',
            '..\GenCfgOpt.py',
            'QemuFspPkg.dsc',
            '.',
            "")
        os.system(cmd)
        noHdrOutputFileName = GenFileWithoutHdr("FspUpd.h", HdrFileHeaderLineLength)
        self.assertTrue(filecmp.cmp(noHdrOutputFileName,
                  'ExpectedFspUpd.h'))

    def test_generateFspsHeader_fromDsc(self):
        noHdrOutputFileName = GenFileWithoutHdr("FspsUpd.h", HdrFileHeaderLineLength)
        self.assertTrue(filecmp.cmp(noHdrOutputFileName,
                  'ExpectedFspsUpd.h'))

    def test_generateFsptHeader_fromDsc(self):
        noHdrOutputFileName = GenFileWithoutHdr("FsptUpd.h", HdrFileHeaderLineLength)
        self.assertTrue(filecmp.cmp(noHdrOutputFileName,
                  'ExpectedFsptUpd.h'))

    def test_generateFspmHeader_fromDsc(self):
        noHdrOutputFileName = GenFileWithoutHdr("FspmUpd.h", HdrFileHeaderLineLength)
        self.assertTrue(filecmp.cmp(noHdrOutputFileName,
                  'ExpectedFspmUpd.h'))

    def test_generateBsf_fromDsc(self):
        # Generate BSF
        cmd = '{} {} GENBSF {} {} {}'.format(
            'python',
            '..\GenCfgOpt.py',
            'QemuFspPkg.dsc',
            '.',
            "Output.bsf")
        os.system(cmd)
        noHdrOutputFileName = GenFileWithoutHdr("Output.bsf", BsfFileHeaderLineLength)
        self.assertTrue(filecmp.cmp(noHdrOutputFileName,
                  'ExpectedOutput.bsf'))

    def test_generateYaml_fromDsc(self):
        # Generate YAML
        cmd = '{} {} {} {}'.format(
            'python',
            '..\FspDscBsf2Yaml.py',
            'QemuFspPkg.dsc',
            "Output.yaml")
        os.system(cmd)
        noHdrOutputFileName = GenFileWithoutHdr("Output.yaml", YamlHeaderLineLength)
        self.assertTrue(filecmp.cmp(noHdrOutputFileName,
                  'ExpectedOutput.yaml'))

if __name__ == '__main__':
    unittest.main()
