## @file
#  Intel Binary Product Data Generation Tool (Intel BPDG).
#  This tool provide a simple process for the creation of a binary file containing read-only 
#  configuration data for EDK II platforms that contain Dynamic and DynamicEx PCDs described 
#  in VPD sections. It also provide an option for specifying an alternate name for a mapping 
#  file of PCD layout for use during the build when the platform integrator selects to use 
#  automatic offset calculation.
#
#  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Common.LongFilePathOs as os
import sys
import encodings.ascii

from optparse import OptionParser
from Common import EdkLogger
from Common.BuildToolError import *
from Common.BuildVersion import gBUILD_VERSION

import StringTable as st
import GenVpd

PROJECT_NAME       = st.LBL_BPDG_LONG_UNI
VERSION            = (st.LBL_BPDG_VERSION + " " + gBUILD_VERSION)

## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
#   @retval 0     Tool was successful
#   @retval 1     Tool failed
#
def main():
    global Options, Args
    
    # Initialize log system
    EdkLogger.Initialize()          
    Options, Args = MyOptionParser()
    
    ReturnCode = 0
    
    if Options.opt_verbose:
        EdkLogger.SetLevel(EdkLogger.VERBOSE)
    elif Options.opt_quiet:
        EdkLogger.SetLevel(EdkLogger.QUIET)
    elif Options.debug_level != None:
        EdkLogger.SetLevel(Options.debug_level + 1) 
    else:
        EdkLogger.SetLevel(EdkLogger.INFO)
                  
    if Options.bin_filename == None:
        EdkLogger.error("BPDG", ATTRIBUTE_NOT_AVAILABLE, "Please use the -o option to specify the file name for the VPD binary file")  
    if Options.filename == None:
        EdkLogger.error("BPDG", ATTRIBUTE_NOT_AVAILABLE, "Please use the -m option to specify the file name for the mapping file")  

    Force = False
    if Options.opt_force != None:
        Force = True

    if (Args[0] != None) :
        StartBpdg(Args[0], Options.filename, Options.bin_filename, Force)
    else :
        EdkLogger.error("BPDG", ATTRIBUTE_NOT_AVAILABLE, "Please specify the file which contain the VPD pcd info.",
                        None)         
    
    return ReturnCode


## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
#   @retval options   A optparse.Values object containing the parsed options
#   @retval args      Target of BPDG command
#            
def MyOptionParser():   
    #
    # Process command line firstly.
    #
    parser = OptionParser(version="%s - Version %s\n" % (PROJECT_NAME, VERSION),
                          description='',
                          prog='BPDG',
                          usage=st.LBL_BPDG_USAGE
                          )
    parser.add_option('-d', '--debug', action='store', type="int", dest='debug_level',
                      help=st.MSG_OPTION_DEBUG_LEVEL)
    parser.add_option('-v', '--verbose', action='store_true', dest='opt_verbose',
                      help=st.MSG_OPTION_VERBOSE)
    parser.add_option('-q', '--quiet', action='store_true', dest='opt_quiet', default=False,
                      help=st.MSG_OPTION_QUIET)
    parser.add_option('-o', '--vpd-filename', action='store', dest='bin_filename',
                      help=st.MSG_OPTION_VPD_FILENAME)
    parser.add_option('-m', '--map-filename', action='store', dest='filename',
                      help=st.MSG_OPTION_MAP_FILENAME)   
    parser.add_option('-f', '--force', action='store_true', dest='opt_force',
                      help=st.MSG_OPTION_FORCE)     
    
    (options, args) = parser.parse_args()
    if len(args) == 0:
        EdkLogger.info("Please specify the filename.txt file which contain the VPD pcd info!")
        EdkLogger.info(parser.usage)
        sys.exit(1)
    return options, args


## Start BPDG and call the main functions 
#
# This method mainly focus on call GenVPD class member functions to complete
# BPDG's target. It will process VpdFile override, and provide the interface file
# information.
#
#   @Param      InputFileName   The filename include the vpd type pcd information
#   @param      MapFileName     The filename of map file that stores vpd type pcd information.
#                               This file will be generated by the BPDG tool after fix the offset
#                               and adjust the offset to make the pcd data aligned.
#   @param      VpdFileName     The filename of Vpd file that hold vpd pcd information.
#   @param      Force           Override the exist Vpdfile or not.
#
def StartBpdg(InputFileName, MapFileName, VpdFileName, Force):
    if os.path.exists(VpdFileName) and not Force:
        print "\nFile %s already exist, Overwrite(Yes/No)?[Y]: " % VpdFileName
        choice = sys.stdin.readline()
        if choice.strip().lower() not in ['y', 'yes', '']:
            return
        
    GenVPD = GenVpd.GenVPD (InputFileName, MapFileName, VpdFileName)
    
    EdkLogger.info('%-24s = %s' % ("VPD input data file: ", InputFileName))  
    EdkLogger.info('%-24s = %s' % ("VPD output map file: ", MapFileName))
    EdkLogger.info('%-24s = %s' % ("VPD output binary file: ", VpdFileName))  
          
    GenVPD.ParserInputFile()
    GenVPD.FormatFileLine()
    GenVPD.FixVpdOffset()
    GenVPD.GenerateVpdFile(MapFileName, VpdFileName)
    
    EdkLogger.info("- Vpd pcd fixed done! -")    

if __name__ == '__main__':
    r = main()
    ## 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127: r = 1
    sys.exit(r)

    
