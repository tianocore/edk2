import os
import logging
import argparse
from datetime import datetime

# Configure logging to output to both file and screen
def ConfigureLogging(LogFile):
    logging.basicConfig(
        format='%(asctime)s DEBUG    %(message)s',
        level=logging.DEBUG,
        datefmt='%m-%d %H:%M:%S',
        handlers=[
            logging.FileHandler(LogFile),
            logging.StreamHandler()
        ]
    )

# Parse the .map file to extract FV sizes
def ParseMapFile(FilePath):
    with open(FilePath, 'r') as File:
        Lines = File.readlines()
        # Check if the file contains EFI_FV_TOTAL_SIZE
        if not any('EFI_FV_TOTAL_SIZE' in Line for Line in Lines):
            return None, None, None
        FvTotalSize = int(Lines[0].split('=')[1].strip(), 16)
        FvTakenSize = int(Lines[1].split('=')[1].strip(), 16)
        FvSpaceSize = int(Lines[2].split('=')[1].strip(), 16)
        return FvTotalSize, FvTakenSize, FvSpaceSize

# Log FV space information for all .map files in the specified directory
def LogFvSpaceInfo(Directory):
    logging.debug('FV Space Information')
    for Filename in os.listdir(Directory):
        if Filename.endswith('.map'):
            FvTotalSize, FvTakenSize, FvSpaceSize = ParseMapFile(os.path.join(Directory, Filename))
            if FvTotalSize is None:
                continue
            FvName = Filename.split('.')[0].upper()
            PercentFull = (FvTakenSize / FvTotalSize) * 100
            logging.debug(f'{FvName} [{PercentFull:.2f}%Full] {FvTotalSize} (0x{FvTotalSize:x}) total, {FvTakenSize} (0x{FvTakenSize:x}) used, {FvSpaceSize} (0x{FvSpaceSize:x}) free')

# Main function
def Main():
    # Parse command-line arguments
    Parser = argparse.ArgumentParser(description='Traverse all .map files in a directory and log FV space information')
    Parser.add_argument('-i', '--Input', required=True, help='Specify the directory path')
    Parser.add_argument('-o', '--Output', required=True, help='Specify the log output file')
    Args = Parser.parse_args()

    # Configure logging
    ConfigureLogging(Args.Output)

    # Log FV space information
    LogFvSpaceInfo(Args.Input)

if __name__ == "__main__":
    Main()

