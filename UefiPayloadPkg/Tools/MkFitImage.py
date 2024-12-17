## @file
# This file is a script to build fit image.
# It generate a dtb header and combine a binary file after this header.
#
# Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from os.path import exists
import libfdt
from ctypes import *
import time
import os

class FIT_IMAGE_INFO_HEADER:
    """Class for user setting data to use MakeFitImage()
    """
    _pack_ = 1
    _fields_ = [
        ('Compatible',    str),
        ('UplVersion',    int),
        ('Description',   str),
        ('Type',          str),
        ('Arch',          str),
        ('Compression',   str),
        ('Revision',      int),
        ('BuildType',     str),
        ('Capabilities',  str),
        ('Producer',      str),
        ('ImageId',       str),
        ('DataOffset',    int),
        ('DataSize',      int),
        ('RelocStart',    int),
        ('LoadAddr',      int),
        ('Entry',         int),
        ('Binary',        str),
        ('TargetPath',    str),
        ('UefifvPath',    str),
        ('BdsfvPath',     str),
        ('NetworkfvPath', str),
        ('Project',       str),
        ]

    def __init__(self):
        self.Compatible     = 'universal-payload'
        self.UplVersion     = 0x0100
        self.TargetPath     = 'mkimage.fit'

def CreatFdt(Fdt):
    FdtEmptyTree = libfdt.fdt_create_empty_tree(Fdt, len(Fdt))
    if FdtEmptyTree != 0:
        print('\n- Failed - Create Fdt failed!')
        return False
    return True

def BuildConfNode(Fdt, ParentNode, MultiImage):
    ConfNode1     = libfdt.fdt_add_subnode(Fdt, ParentNode, 'conf-1')

    libfdt.fdt_setprop(Fdt, ConfNode1, 'require-fit', b'', 0)
    libfdt.fdt_setprop(Fdt, ConfNode1, 'firmware', bytes('tianocore', 'utf-8'), len('tianocore') + 1)

def BuildFvImageNode(Fdt, InfoHeader, ParentNode, DataOffset, DataSize, Description, Arch):
    libfdt.fdt_setprop_u32(Fdt, ParentNode, 'data-size', DataSize)
    libfdt.fdt_setprop_u32(Fdt, ParentNode, 'data-offset', DataOffset)
    libfdt.fdt_setprop(Fdt, ParentNode, 'compression', bytes('none',                'utf-8'), len('none') + 1)
    libfdt.fdt_setprop(Fdt, ParentNode, 'project',    bytes('tianocore',           'utf-8'), len('tianocore') + 1)
    libfdt.fdt_setprop(Fdt, ParentNode, 'arch',        bytes(Arch,                  'utf-8'), len(Arch) + 1)
    libfdt.fdt_setprop(Fdt, ParentNode, 'type',        bytes('flat-binary',         'utf-8'), len('flat-binary') + 1)
    libfdt.fdt_setprop(Fdt, ParentNode, 'description', bytes(Description,           'utf-8'), len(Description) + 1)

def BuildTianoImageNode(Fdt, InfoHeader, ParentNode, DataOffset, DataSize, Description, Arch):
    #
    # Set 'load' and 'data-offset' to reserve the memory first.
    # They would be set again when Fdt completes or this function parses target binary file.
    #
    if InfoHeader.LoadAddr is not None:
        libfdt.fdt_setprop_u64(Fdt, ParentNode, 'load', InfoHeader.LoadAddr)
    if InfoHeader.Entry is not None:
        libfdt.fdt_setprop_u64(Fdt, ParentNode, 'entry', InfoHeader.Entry)
    if InfoHeader.RelocStart is not None:
        libfdt.fdt_setprop_u32(Fdt, ParentNode, 'reloc-start', InfoHeader.RelocStart)
    if InfoHeader.DataSize is not None:
       libfdt.fdt_setprop_u32(Fdt, ParentNode, 'data-size', DataSize)
    if InfoHeader.DataOffset is not None:
        libfdt.fdt_setprop_u32(Fdt, ParentNode, 'data-offset', DataOffset)
    if InfoHeader.Producer is not None:
        libfdt.fdt_setprop(Fdt, ParentNode, 'producer', bytes(InfoHeader.Producer, 'utf-8'), len(InfoHeader.Producer) + 1)
    if InfoHeader.Capabilities is not None:
        CapStrs = ','.join(InfoHeader.Capabilities)
        libfdt.fdt_setprop(Fdt, ParentNode, 'capabilities', bytes(CapStrs, 'utf-8'), len(CapStrs) + 1)
    if InfoHeader.Type is not None:
        libfdt.fdt_setprop(Fdt, ParentNode, 'type', bytes(InfoHeader.Type, 'utf-8'), len(InfoHeader.Type) + 1)
    if InfoHeader.Arch is not None:
        libfdt.fdt_setprop(Fdt, ParentNode, 'arch', bytes(InfoHeader.Arch, 'utf-8'), len(InfoHeader.Arch) + 1)
    if InfoHeader.Project is not None:
        libfdt.fdt_setprop(Fdt, ParentNode, 'project', bytes(InfoHeader.Project, 'utf-8'), len(InfoHeader.Project) + 1)
    if InfoHeader.Description is not None:
        libfdt.fdt_setprop(Fdt, ParentNode, 'description', bytes(Description, 'utf-8'), len(Description) + 1)

#
# The subnode would be inserted from bottom to top of structure block.
#
def BuildFitImage(Fdt, InfoHeader, Arch):
    MultiImage = [
        ["tianocore",   InfoHeader.Binary,        BuildTianoImageNode , InfoHeader.Description,     None, 0 ],
        ["uefi-fv",     InfoHeader.UefifvPath,    BuildFvImageNode,     "UEFI Firmware Volume",     None, 0 ],
        ["bds-fv",      InfoHeader.BdsfvPath,     BuildFvImageNode ,    "BDS Firmware Volume",      None, 0 ],
        ["sec-fv",      InfoHeader.SecfvPath,     BuildFvImageNode ,    "Security Firmware Volume", None, 0 ],
        ["network-fv",  InfoHeader.NetworkfvPath, BuildFvImageNode ,    "Network Firmware Volume",  None, 0 ],
    ]

    #
    # Set basic information
    #
    libfdt.fdt_setprop_u32(Fdt, 0, '#address-cells', 2)
    libfdt.fdt_setprop_u32(Fdt, 0, 'build-revision', InfoHeader.Revision)
    libfdt.fdt_setprop_u32(Fdt, 0, 'spec-version', InfoHeader.UplVersion)

    #
    # Build configurations node
    #
    ConfNode  = libfdt.fdt_add_subnode(Fdt, 0, 'configurations')
    BuildConfNode(Fdt, ConfNode, MultiImage)

    # Build image
    DataOffset = InfoHeader.DataOffset
    for Index in range (0, len (MultiImage)):
        _, Path, _, _, _, _ = MultiImage[Index]
        if exists(Path) == 1:
            TempBinary = open(Path, 'rb')
            BinaryData = TempBinary.read()
            TempBinary.close()
            MultiImage[Index][-2] = BinaryData
            MultiImage[Index][-1] = DataOffset
            DataOffset += len (BinaryData)
    libfdt.fdt_setprop_u32(Fdt, 0, 'size', DataOffset)
    posix_time = int(time.time())
    libfdt.fdt_setprop_u32(Fdt, 0, 'timestamp', posix_time)
    DescriptionFit = 'Uefi OS Loader'
    libfdt.fdt_setprop(Fdt, 0, 'description', bytes(DescriptionFit, 'utf-8'), len(DescriptionFit) + 1)

    ImageNode = libfdt.fdt_add_subnode(Fdt, 0, 'images')
    for Item in reversed (MultiImage):
        Name, Path, BuildFvNode, Description, BinaryData, DataOffset = Item
        if os.path.exists (Item[1]) == False:
            continue
        FvNode = libfdt.fdt_add_subnode(Fdt, ImageNode, Name)
        BuildFvNode (Fdt, InfoHeader, FvNode, DataOffset, len(BinaryData), Description, Arch)

    #
    # Create new image file and combine all binary.
    #
    DtbFile = open(InfoHeader.TargetPath, "wb")
    DtbFile.truncate()
    DtbFile.write(Fdt)
    for Item in MultiImage:
        _, FilePath, _, _, BinaryData, _ = Item
        if os.path.exists (Item[1]) == False:
            continue
        DtbFile.write(BinaryData)
    DtbFile.close()

    return True

def MakeFitImage(InfoHeader, Arch):
    #
    # Allocate fdt byte array.
    #
    Fdt = bytearray(InfoHeader.DataOffset)

    #
    # Create fdt empty tree.
    #
    if CreatFdt(Fdt) is False:
        return False

    #
    # Parse args to build fit image.
    #
    return BuildFitImage(Fdt, InfoHeader, Arch)

def ReplaceFv (UplBinary, SectionFvFile, SectionName, Arch):
    try:
        #
        # Get Original Multi Fv
        #
        with open (UplBinary, "rb") as File:
            Dtb = File.read ()
        Fit          = libfdt.Fdt (Dtb)
        NewFitHeader = bytearray(Dtb[0:Fit.totalsize()])
        FitSize      = len(Dtb)

        LoadablesList = []
        ImagesNode    = libfdt.fdt_subnode_offset(NewFitHeader, 0, 'images')
        FvNode        = libfdt.fdt_subnode_offset(NewFitHeader, ImagesNode, 'uefi-fv')
        NodeDepth     = libfdt.fdt_node_depth (NewFitHeader, ImagesNode)
        node_name     = libfdt.fdt_get_name(NewFitHeader, FvNode)
        FvNode        = libfdt.fdt_next_node(NewFitHeader, FvNode, NodeDepth)

        while node_name[0][-2:] == 'fv':
            LoadablesList.append (node_name[0])
            node_name = libfdt.fdt_get_name(NewFitHeader, FvNode[0])
            FvNode = libfdt.fdt_next_node(NewFitHeader, FvNode[0], NodeDepth)
        #
        # Get current Fit Binary FV data
        #
        MultiFvList = []
        for Item in LoadablesList:
            ImageNode    = libfdt.fdt_subnode_offset(NewFitHeader, ImagesNode, Item)
            ImageOffset  = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-offset')[0], 'big')
            ImageSize    = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-size')[0], 'big')
            MultiFvList.append ([Item, Dtb[ImageOffset:ImageOffset + ImageSize]])

        IsFvExist = False
        for Index in range (0, len (MultiFvList)):
            if MultiFvList[Index][0] == SectionName:
                with open (SectionFvFile, 'rb') as File:
                    MultiFvList[Index][1] = File.read ()
                ImageNode     = libfdt.fdt_subnode_offset(NewFitHeader, ImagesNode, SectionName)
                ImageSize     = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-size')[0], 'big')
                ReplaceOffset = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-offset')[0], 'big')
                OffsetDelta   = len(MultiFvList[Index][1]) - ImageSize
                FitSize      += OffsetDelta
                IsFvExist     = True
                libfdt.fdt_setprop_u32(NewFitHeader, ImageNode, 'data-size', len(MultiFvList[Index][1]))

        #
        # Update new fit header
        #
        ImagesNode = libfdt.fdt_subnode_offset(NewFitHeader, 0, 'images')
        if (IsFvExist == False):
            with open (SectionFvFile, 'rb') as File:
                SectionFvFileBinary = File.read ()
            MultiFvList.append ([SectionName, SectionFvFileBinary])
            FvNode = libfdt.fdt_add_subnode(NewFitHeader, ImagesNode, SectionName)
            BuildFvImageNode (NewFitHeader, None, FvNode, FitSize, len(SectionFvFileBinary), SectionName + " Firmware Volume", Arch)
            FitSize += len(SectionFvFileBinary)
        else:
            for Index in range (0, len (MultiFvList)):
                ImageNode    = libfdt.fdt_subnode_offset(NewFitHeader, ImagesNode, MultiFvList[Index][0])
                ImageOffset  = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-offset')[0], 'big')
                if ImageOffset > ReplaceOffset:
                    libfdt.fdt_setprop_u32(NewFitHeader, ImageNode, 'data-offset', ImageOffset + OffsetDelta)

        ConfNodes     = libfdt.fdt_subnode_offset(NewFitHeader, 0, 'configurations')
        libfdt.fdt_setprop(NewFitHeader, ConfNodes, 'default', bytes('conf-1', 'utf-8'), len('conf-1') + 1)
        ConfNode      = libfdt.fdt_subnode_offset(NewFitHeader, ConfNodes, 'conf-1')

        libfdt.fdt_setprop_u32(NewFitHeader, 0, 'size', FitSize)

        #
        # Generate new fit image
        #
        ImagesNode    = libfdt.fdt_subnode_offset(NewFitHeader, 0, 'images')
        TianoNode     = libfdt.fdt_subnode_offset(NewFitHeader, ImagesNode, 'tianocore')
        TianoOffset   = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, TianoNode, 'data-offset')[0], 'big')
        TianoSize     = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, TianoNode, 'data-size')[0], 'big')
        TianoBinary   = Dtb[TianoOffset:TianoOffset + TianoSize]

        print("\nGenerate new fit image:")
        NewUplBinary = bytearray(FitSize)
        print("Update fit header\t to 0x0\t\t ~ " + str(hex(len(NewFitHeader))))
        NewUplBinary[:len(NewFitHeader)] = NewFitHeader
        print("Update tiano image\t to " + str(hex(len(NewFitHeader))) + "\t ~ " + str(hex(len(NewFitHeader) + len(TianoBinary))))
        NewUplBinary[len(NewFitHeader):len(NewFitHeader) + len(TianoBinary)] = TianoBinary
        for Index in range (0, len (MultiFvList)):
            ImageNode   = libfdt.fdt_subnode_offset(NewFitHeader, ImagesNode, MultiFvList[Index][0])
            ImageOffset = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-offset')[0], 'big')
            ImageSize   = int.from_bytes (libfdt.fdt_getprop (NewFitHeader, ImageNode, 'data-size')[0], 'big')
            NewUplBinary[ImageOffset:ImageOffset + ImageSize] = MultiFvList[Index][1]
            print("Update " + MultiFvList[Index][0] + "\t\t to " + str(hex(ImageOffset)) + "\t ~ " + str(hex(ImageOffset + ImageSize)))

        with open (UplBinary, "wb") as File:
            File.write (NewUplBinary)

        return 0
    except Exception as Ex:
        print(Ex)
        return 1
