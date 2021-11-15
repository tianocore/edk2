## @file
# This file is used to define the functions to operate bios binary file.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from core.FMMTParser import *
from core.FvHandler import *
from utils.FvLayoutPrint import *

global Fv_count
Fv_count = 0

# The ROOT_TYPE can be 'ROOT_TREE', 'ROOT_FV_TREE', 'ROOT_FFS_TREE', 'ROOT_SECTION_TREE'
def ParserFile(inputfile: str, ROOT_TYPE: str, logfile: str=None, outputfile: str=None) -> None:
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TYPE)
    # 2. DataTree Create
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    # 3. Log Output
    InfoDict = FmmtParser.WholeFvTree.ExportTree()
    FmmtParser.WholeFvTree.parserTree(InfoDict, FmmtParser.BinaryInfo)
    GetFormatter("").LogPrint(FmmtParser.BinaryInfo)
    if logfile:
        GetFormatter(logfile.lower()).dump(InfoDict, FmmtParser.BinaryInfo,"Parser_Log_{}{}".format(os.path.basename(inputfile),".{}".format(logfile.lower())))
    # 4. Data Encapsultion
    if outputfile:
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)

def DeleteFfs(inputfile: str, TargetFfs_name: str, outputfile: str, Fv_name: str=None) -> None:
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    # 3. Data Modify
    FmmtParser.WholeFvTree.FindNode(TargetFfs_name, FmmtParser.WholeFvTree.Findlist)
    # Choose the Specfic DeleteFfs with Fv info
    if Fv_name:
        for item in FmmtParser.WholeFvTree.Findlist:
            if item.Parent.key != Fv_name and item.Parent.Data.Name != Fv_name:
                FmmtParser.WholeFvTree.Findlist.remove(item)
    if FmmtParser.WholeFvTree.Findlist != []:
        for Delete_Ffs in FmmtParser.WholeFvTree.Findlist:
            FfsMod = FvHandler(None, Delete_Ffs)
            Status = FfsMod.DeleteFfs()
    else:
        print('Target Ffs not found!!!')
    # 4. Data Encapsultion
    if Status:
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)

def AddNewFfs(inputfile: str, Fv_name: str, newffsfile: str, outputfile: str) -> None:
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
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
            if TargetFfsPad.type == FFS_FREE_SPACE:
                NewFmmtParser.ParserFromRoot(NewFmmtParser.WholeFvTree, new_ffs_data, TargetFfsPad.Data.HOffset)
            else:
                NewFmmtParser.ParserFromRoot(NewFmmtParser.WholeFvTree, new_ffs_data, TargetFfsPad.Data.HOffset+TargetFfsPad.Data.Size)
            FfsMod = FvHandler(NewFmmtParser.WholeFvTree.Child[0], TargetFfsPad)
            Status = FfsMod.AddFfs()
    else:
        print('Target Fv not found!!!')
    # 4. Data Encapsultion
    if Status:
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)

def ReplaceFfs(inputfile: str, Ffs_name: str, newffsfile: str, outputfile: str, Fv_name: str=None) -> None:
    # 1. Data Prepare
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    # 2. DataTree Create
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    with open(newffsfile, "rb") as f:
        new_ffs_data = f.read()
    newFmmtParser = FMMTParser(newffsfile, FV_TREE)
    newFmmtParser.ParserFromRoot(newFmmtParser.WholeFvTree, new_ffs_data)
    Status = False
    # 3. Data Modify
    new_ffs = newFmmtParser.WholeFvTree.Child[0]
    new_ffs.Data.PadData = GetPadSize(new_ffs.Data.Size, 8) * b'\xff'
    FmmtParser.WholeFvTree.FindNode(Ffs_name, FmmtParser.WholeFvTree.Findlist)
    if Fv_name:
        for item in FmmtParser.WholeFvTree.Findlist:
            if item.Parent.key != Fv_name and item.Parent.Data.Name != Fv_name:
                FmmtParser.WholeFvTree.Findlist.remove(item)
    if FmmtParser.WholeFvTree.Findlist != []:
        for TargetFfs in FmmtParser.WholeFvTree.Findlist:
            FfsMod = FvHandler(newFmmtParser.WholeFvTree.Child[0], TargetFfs)
            Status = FfsMod.ReplaceFfs()
    else:
        print('Target Ffs not found!!!')
    # 4. Data Encapsultion
    if Status:
        FmmtParser.Encapsulation(FmmtParser.WholeFvTree, False)
        with open(outputfile, "wb") as f:
            f.write(FmmtParser.FinalData)

def ExtractFfs(inputfile: str, Ffs_name: str, outputfile: str) -> None:
    with open(inputfile, "rb") as f:
        whole_data = f.read()
    FmmtParser = FMMTParser(inputfile, ROOT_TREE)
    FmmtParser.ParserFromRoot(FmmtParser.WholeFvTree, whole_data)
    FmmtParser.WholeFvTree.FindNode(Ffs_name, FmmtParser.WholeFvTree.Findlist)
    if FmmtParser.WholeFvTree.Findlist != []:
        TargetNode = FmmtParser.WholeFvTree.Findlist[0]
        TargetFv = TargetNode.Parent
        if TargetFv.Data.Header.Attributes & EFI_FVB2_ERASE_POLARITY:
            TargetNode.Data.Header.State = c_uint8(
                ~TargetNode.Data.Header.State)
        FinalData = struct2stream(TargetNode.Data.Header) + TargetNode.Data.Data
        with open(outputfile, "wb") as f:
            f.write(FinalData)
    else:
        print('Target Ffs not found!!!')