## @file
# This file is used to define the functions to operate bios binary file.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from core.FMMTParser import *
from core.FvHandler import *
from utils.FvLayoutPrint import *
from utils.FmmtLogger import FmmtLogger as logger

global Fv_count
Fv_count = 0

# The ROOT_TYPE can be 'ROOT_TREE', 'ROOT_FV_TREE', 'ROOT_FFS_TREE', 'ROOT_SECTION_TREE'
def ViewFile(inputfile: str, ROOT_TYPE: str, layoutfile: str=None, outputfile: str=None) -> None:
    if not os.path.exists(inputfile):
        logger.error("Invalid inputfile, can not open {}.".format(inputfile))
        raise Exception("Process Failed: Invalid inputfile!")
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TYPE)
    # 2. DataTree Create
    logger.debug('Parsing inputfile data......')
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    logger.debug('Done!')
    # 3. Log Output
    InfoDict = FmmtParser.WholeFvTree.ExportTree()
    logger.debug('BinaryTree created, start parsing BinaryTree data......')
    FmmtParser.WholeFvTree.parserTree(InfoDict, FmmtParser.BinaryInfo)
    logger.debug('Done!')
    GetFormatter("").LogPrint(FmmtParser.BinaryInfo)
    if layoutfile:
        if os.path.splitext(layoutfile)[1]:
            layoutfilename = layoutfile
            layoutfileformat = os.path.splitext(layoutfile)[1][1:].lower()
        else:
            layoutfilename = "Layout_{}{}".format(os.path.basename(inputfile),".{}".format(layoutfile.lower()))
            layoutfileformat = layoutfile.lower()
        GetFormatter(layoutfileformat).dump(InfoDict, FmmtParser.BinaryInfo, layoutfilename)
    # 4. Data Encapsulation
    if outputfile:
        logger.debug('Start encapsulating data......')
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)
        logger.debug('Encapsulated data is saved in {}.'.format(outputfile))

def DeleteFfs(inputfile: str, TargetFfs_name: str, outputfile: str, Fv_name: str=None) -> None:
    if not os.path.exists(inputfile):
        logger.error("Invalid inputfile, can not open {}.".format(inputfile))
        raise Exception("Process Failed: Invalid inputfile!")
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    logger.debug('Parsing inputfile data......')
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    logger.debug('Done!')
    # 3. Data Modify
    FmmtParser.WholeFvTree.FindNode(TargetFfs_name, FmmtParser.WholeFvTree.Findlist)
    # Choose the Specfic DeleteFfs with Fv info
    if Fv_name:
        FindNum = len(FmmtParser.WholeFvTree.Findlist)
        for index in range(FindNum-1, -1, -1):
            if FmmtParser.WholeFvTree.Findlist[index].Parent.key != Fv_name and FmmtParser.WholeFvTree.Findlist[index].Parent.Data.Name != Fv_name:
                FmmtParser.WholeFvTree.Findlist.remove(FmmtParser.WholeFvTree.Findlist[index])
    Status = False
    if FmmtParser.WholeFvTree.Findlist != []:
        for Delete_Ffs in FmmtParser.WholeFvTree.Findlist:
            FfsMod = FvHandler(None, Delete_Ffs)
            Status = FfsMod.DeleteFfs()
    else:
        logger.error('Target Ffs not found!!!')
    # 4. Data Encapsulation
    if Status:
        logger.debug('Start encapsulating data......')
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)
        logger.debug('Encapsulated data is saved in {}.'.format(outputfile))

def AddNewFfs(inputfile: str, Fv_name: str, newffsfile: str, outputfile: str) -> None:
    if not os.path.exists(inputfile):
        logger.error("Invalid inputfile, can not open {}.".format(inputfile))
        raise Exception("Process Failed: Invalid inputfile!")
    if not os.path.exists(newffsfile):
        logger.error("Invalid ffsfile, can not open {}.".format(newffsfile))
        raise Exception("Process Failed: Invalid ffs file!")
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    logger.debug('Parsing inputfile data......')
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    logger.debug('Done!')
    # Get Target Fv and Target Ffs_Pad
    FmmtParser.WholeFvTree.FindNode(Fv_name, FmmtParser.WholeFvTree.Findlist)
    # Create new ffs Tree
    with open(newffsfile, "rb") as f:
        new_ffs_data = f.read()
    NewFmmtParser = FMMTParser(newffsfile, ROOT_FFS_TREE)
    Status = False
    # 3. Data Modify
    if FmmtParser.WholeFvTree.Findlist:
        for TargetFv in FmmtParser.WholeFvTree.Findlist:
            TargetFfsPad = TargetFv.Child[-1]
            logger.debug('Parsing newffsfile data......')
            if TargetFfsPad.type == FFS_FREE_SPACE:
                NewFmmtParser.ParserFromRoot(NewFmmtParser.WholeFvTree, new_ffs_data, TargetFfsPad.Data.HOffset)
            else:
                NewFmmtParser.ParserFromRoot(NewFmmtParser.WholeFvTree, new_ffs_data, TargetFfsPad.Data.HOffset+TargetFfsPad.Data.Size)
            logger.debug('Done!')
            FfsMod = FvHandler(NewFmmtParser.WholeFvTree.Child[0], TargetFfsPad)
            Status = FfsMod.AddFfs()
    else:
        logger.error('Target Fv not found!!!')
    # 4. Data Encapsulation
    if Status:
        logger.debug('Start encapsulating data......')
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)
        logger.debug('Encapsulated data is saved in {}.'.format(outputfile))

def ReplaceFfs(inputfile: str, Ffs_name: str, newffsfile: str, outputfile: str, Fv_name: str=None) -> None:
    if not os.path.exists(inputfile):
        logger.error("Invalid inputfile, can not open {}.".format(inputfile))
        raise Exception("Process Failed: Invalid inputfile!")
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    logger.debug('Parsing inputfile data......')
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    logger.debug('Done!')
    with open(newffsfile, "rb") as f:
        new_ffs_data = f.read()
    newFmmtParser = FMMTParser(newffsfile, FV_TREE)
    logger.debug('Parsing newffsfile data......')
    newFmmtParser.ParserFromRoot(newFmmtParser.WholeFvTree, new_ffs_data)
    logger.debug('Done!')
    Status = False
    # 3. Data Modify
    new_ffs = newFmmtParser.WholeFvTree.Child[0]
    new_ffs.Data.PadData = GetPadSize(new_ffs.Data.Size, FFS_COMMON_ALIGNMENT) * b'\xff'
    FmmtParser.WholeFvTree.FindNode(Ffs_name, FmmtParser.WholeFvTree.Findlist)
    if Fv_name:
        FindNum = len(FmmtParser.WholeFvTree.Findlist)
        for index in range(FindNum-1, -1, -1):
            if FmmtParser.WholeFvTree.Findlist[index].Parent.key != Fv_name and FmmtParser.WholeFvTree.Findlist[index].Parent.Data.Name != Fv_name:
                FmmtParser.WholeFvTree.Findlist.remove(FmmtParser.WholeFvTree.Findlist[index])
    if FmmtParser.WholeFvTree.Findlist != []:
        for TargetFfs in FmmtParser.WholeFvTree.Findlist:
            FfsMod = FvHandler(newFmmtParser.WholeFvTree.Child[0], TargetFfs)
            Status = FfsMod.ReplaceFfs()
    else:
        logger.error('Target Ffs not found!!!')
    # 4. Data Encapsulation
    if Status:
        logger.debug('Start encapsulating data......')
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)
        logger.debug('Encapsulated data is saved in {}.'.format(outputfile))

def ExtractFfs(inputfile: str, Ffs_name: str, outputfile: str, Fv_name: str=None) -> None:
    if not os.path.exists(inputfile):
        logger.error("Invalid inputfile, can not open {}.".format(inputfile))
        raise Exception("Process Failed: Invalid inputfile!")
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    logger.debug('Parsing inputfile data......')
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    logger.debug('Done!')
    FmmtParser.WholeFvTree.FindNode(Ffs_name, FmmtParser.WholeFvTree.Findlist)
    if Fv_name:
        FindNum = len(FmmtParser.WholeFvTree.Findlist)
        for index in range(FindNum-1, -1, -1):
            if FmmtParser.WholeFvTree.Findlist[index].Parent.key != Fv_name and FmmtParser.WholeFvTree.Findlist[index].Parent.Data.Name != Fv_name:
                FmmtParser.WholeFvTree.Findlist.remove(FmmtParser.WholeFvTree.Findlist[index])
    if FmmtParser.WholeFvTree.Findlist != []:
        TargetNode = FmmtParser.WholeFvTree.Findlist[0]
        if TargetNode.type == FV_TREE or TargetNode.type == SEC_FV_TREE or TargetNode.type == DATA_FV_TREE:
            FinalData = struct2stream(TargetNode.Data.Header) + TargetNode.Data.Data
            with open(outputfile, "wb") as f:
                f.write(FinalData)
            logger.debug('Extract fv data is saved in {}.'.format(outputfile))
        else:
            TargetFv = TargetNode.Parent
            if TargetFv.Data.Header.Attributes & EFI_FVB2_ERASE_POLARITY:
                TargetNode.Data.Header.State = c_uint8(
                    ~TargetNode.Data.Header.State)
            FinalData = struct2stream(TargetNode.Data.Header) + TargetNode.Data.Data
            with open(outputfile, "wb") as f:
                f.write(FinalData)
            logger.debug('Extract ffs data is saved in {}.'.format(outputfile))
    else:
        logger.error('Target Ffs/Fv not found!!!')

def ShrinkFv(inputfile: str, outputfile: str) -> None:
    if not os.path.exists(inputfile):
        logger.error("Invalid inputfile, can not open {}.".format(inputfile))
        raise Exception("Process Failed: Invalid inputfile!")
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    logger.debug('Parsing inputfile data......')
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    logger.debug('Done!')
    TargetFv = FmmtParser.WholeFvTree.Child[0]
    if TargetFv:
        FvMod = FvHandler(TargetFv)
        Status = FvMod.ShrinkFv()
    else:
        logger.error('Target Fv not found!!!')
    # 4. Data Encapsulation
    if Status:
        logger.debug('Start encapsulating data......')
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)
        logger.debug('Encapsulated data is saved in {}.'.format(outputfile))
