## @file
# Convert an XML-based FPD file to a text-based DSC file.
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os, re, sys, xml.dom.minidom  #XmlRoutines, EdkIIWorkspace
from LoadFpd import LoadFpd
from StoreDsc import StoreDsc
from optparse import OptionParser

# Version and Copyright
__version_number__ = "1.0"
__version__ = "%prog Version " + __version_number__
__copyright__ = "Copyright (c) 2007, Intel Corporation  All rights reserved."

## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
# @retval Options   A optparse.Values object containing the parsed options
# @retval Args      All the arguments got from the command line
#
def MyOptionParser():
    """ Argument Parser """
    usage = "%prog [options] input_filename"
    parser = OptionParser(usage=usage,description=__copyright__,version="%prog " + str(__version_number__))
    parser.add_option("-o", "--output", dest="outfile", help="Specific Name of the DSC file to create, otherwise it is the FPD filename with the extension repalced.")
    parser.add_option("-a", "--auto", action="store_true", dest="autowrite", default=False, help="Automatically create output files and write the DSC file")
    parser.add_option("-q", "--quiet", action="store_const", const=0, dest="verbose", help="Do not print any messages, just return either 0 for succes or 1 for failure")
    parser.add_option("-v", "--verbose", action="count", dest="verbose", help="Do not print any messages, just return either 0 for succes or 1 for failure")
    parser.add_option("-d", "--debug", action="store_true", dest="debug", default=False, help="Enable printing of debug messages.")
    parser.add_option("-w", "--workspace", dest="workspace", default=str(os.environ.get('WORKSPACE')), help="Specify workspace directory.")
    (options, args) = parser.parse_args(sys.argv[1:])

    return options,args

## Entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @retval 0     Tool was successful
# @retval 1     Tool failed
#
def Main():
    global Options
    global Args
    global WorkSpace
    Options,Args = MyOptionParser()

    WorkSpace = ""
    #print Options.workspace
    if (Options.workspace == None):
        print "ERROR: E0000: WORKSPACE not defined.\n  Please set the WORKSPACE environment variable to the location of the EDK II install directory."
        sys.exit(1)
    else:
        WorkSpace = Options.workspace
        if (Options.debug):
            print "Using Workspace:", WorkSpace
    try:
        Options.verbose +=1
    except:
        Options.verbose = 1
        pass

    InputFile = ""
    if Args == []:
        print "usage:" "%prog [options] input_filename"
    else:
        InputFile = Args[0]
        #print InputFile
    if InputFile != "":
        FileName = InputFile
        if ((Options.verbose > 1) | (Options.autowrite)):
            print "FileName:",InputFile
    else:
        print "ERROR: E0001 - You must specify an input filename"
        sys.exit(1)

    if (Options.outfile):
        OutputFile = Options.outfile
    else:
       OutputFile = FileName.replace('.fpd', '.dsc')

    if ((Options.verbose > 2) or (Options.debug)):
        print "Output Filename:", OutputFile
        
    try:
        Platform = LoadFpd(FileName)
        StoreDsc(OutputFile, Platform)
        return 0
    except Exception, e:
        print e
        return 1

if __name__ == '__main__':
    sys.exit(Main())
    #pass
    #global Options
    #global Args
    #Options,Args = MyOptionParser()
    
    #Main()
    #sys.exit(0)