# @file
#  Split a file into two pieces at the request offset.
#
#  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

# Import Modules
#
import argparse
import os
import io
import shutil
import logging
import sys
import tempfile

parser = argparse.ArgumentParser(description='''
SplitFile creates two Binary files either in the same directory as the current working directory or in the specified directory.
''')
parser.add_argument("-f", "--filename", dest="inputfile",
                    required=True, help="The input file to split tool.")
parser.add_argument("-s", "--split", dest="position",
                    required=True, help="The number of bytes in the first file. The valid format are HEX, Decimal and Decimal[KMG].")
parser.add_argument("-p", "--prefix",  dest="output",
                    help="The output folder.")
parser.add_argument("-o", "--firstfile",  help="The first file name")
parser.add_argument("-t", "--secondfile",  help="The second file name")
parser.add_argument("--version", action="version", version='%(prog)s Version 2.0',
                    help="Print debug information.")

group = parser.add_mutually_exclusive_group()
group.add_argument("-v", "--verbose", action="store_true",
                   help="Print debug information.")
group.add_argument("-q", "--quiet", action="store_true",
                   help="Disable all messages except fatal errors")

SizeDict = {
    "K": 1024,
    "M": 1024*1024,
    "G": 1024*1024*1024
}


def GetPositionValue(position):
    '''
    Parse the string of the argument position and return a decimal number.
    The valid position formats are
    1. HEX
    e.g. 0x1000 or 0X1000
    2. Decimal
    e.g. 100
    3. Decimal[KMG]
    e.g. 100K or 100M or 100G or 100k or 100m or 100g
    '''
    logger = logging.getLogger('Split')
    PosVal = 0
    header = position[:2].upper()
    tailer = position[-1].upper()

    try:
        if tailer in SizeDict:
            PosVal = int(position[:-1]) * SizeDict[tailer]
        else:
            if header == "0X":
                PosVal = int(position, 16)
            else:
                PosVal = int(position)
    except Exception as e:
        logger.error(
            "The parameter %s format is incorrect. The valid format is HEX, Decimal and Decimal[KMG]." % position)
        raise(e)

    return PosVal


def getFileSize(filename):
    '''
    Read the input file and return the file size.
    '''
    logger = logging.getLogger('Split')
    length = 0
    try:
        with open(filename, "rb") as fin:
            fin.seek(0, io.SEEK_END)
            length = fin.tell()
    except Exception as e:
        logger.error("Access file failed: %s", filename)
        raise(e)

    return length

def getoutputfileabs(inputfile, prefix, outputfile,index):
    inputfile = os.path.abspath(inputfile)
    if outputfile is None:
        if prefix is None:
            outputfileabs = os.path.join(os.path.dirname(inputfile), "{}{}".format(os.path.basename(inputfile),index))
        else:
            if os.path.isabs(prefix):
                outputfileabs = os.path.join(prefix, "{}{}".format(os.path.basename(inputfile),index))
            else:
                outputfileabs = os.path.join(os.getcwd(), prefix, "{}{}".format(os.path.basename(inputfile),index))
    elif not os.path.isabs(outputfile):
        if prefix is None:
            outputfileabs = os.path.join(os.getcwd(), outputfile)
        else:
            if os.path.isabs(prefix):
                outputfileabs = os.path.join(prefix, outputfile)
            else:
                outputfileabs = os.path.join(os.getcwd(), prefix, outputfile)
    else:
        outputfileabs = outputfile
    return outputfileabs

def splitFile(inputfile, position, outputdir=None, outputfile1=None, outputfile2=None):
    '''
    Split the inputfile into outputfile1 and outputfile2 from the position.
    '''
    logger = logging.getLogger('Split')

    if not os.path.exists(inputfile):
        logger.error("File Not Found: %s" % inputfile)
        raise(Exception)

    if outputfile1 and outputfile2 and outputfile1 == outputfile2:
        logger.error(
            "The firstfile and the secondfile can't be the same: %s" % outputfile1)
        raise(Exception)

    # Create dir for the output files
    try:

        outputfile1 = getoutputfileabs(inputfile, outputdir, outputfile1,1)
        outputfolder = os.path.dirname(outputfile1)
        if not os.path.exists(outputfolder):
            os.makedirs(outputfolder)

        outputfile2 = getoutputfileabs(inputfile, outputdir, outputfile2,2)
        outputfolder = os.path.dirname(outputfile2)
        if not os.path.exists(outputfolder):
            os.makedirs(outputfolder)

    except Exception as e:
        logger.error("Can't make dir: %s" % outputfolder)
        raise(e)

    if position <= 0:
        if outputfile2 != os.path.abspath(inputfile):
            shutil.copy2(os.path.abspath(inputfile), outputfile2)
        with open(outputfile1, "wb") as fout:
            fout.write(b'')
    else:
        inputfilesize = getFileSize(inputfile)
        if position >= inputfilesize:
            if outputfile1 != os.path.abspath(inputfile):
                shutil.copy2(os.path.abspath(inputfile), outputfile1)
            with open(outputfile2, "wb") as fout:
                fout.write(b'')
        else:
            try:
                tempdir = tempfile.mkdtemp()
                tempfile1 = os.path.join(tempdir, "file1.bin")
                tempfile2 = os.path.join(tempdir, "file2.bin")
                with open(inputfile, "rb") as fin:
                    content1 = fin.read(position)
                    with open(tempfile1, "wb") as fout1:
                        fout1.write(content1)

                    content2 = fin.read(inputfilesize - position)
                    with open(tempfile2, "wb") as fout2:
                        fout2.write(content2)
                shutil.copy2(tempfile1, outputfile1)
                shutil.copy2(tempfile2, outputfile2)
            except Exception as e:
                logger.error("Split file failed")
                raise(e)
            finally:
                if os.path.exists(tempdir):
                    shutil.rmtree(tempdir)


def main():
    args = parser.parse_args()
    status = 0

    logger = logging.getLogger('Split')
    if args.quiet:
        logger.setLevel(logging.CRITICAL)
    if args.verbose:
        logger.setLevel(logging.DEBUG)

    lh = logging.StreamHandler(sys.stdout)
    lf = logging.Formatter("%(levelname)-8s: %(message)s")
    lh.setFormatter(lf)
    logger.addHandler(lh)

    try:
        position = GetPositionValue(args.position)
        splitFile(args.inputfile, position, args.output,
                  args.firstfile, args.secondfile)
    except Exception as e:
        status = 1

    return status


if __name__ == "__main__":
    exit(main())
