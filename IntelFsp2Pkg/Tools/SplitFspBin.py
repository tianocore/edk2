## @ SplitFspBin.py
#
# Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import sys
import uuid
import copy
import struct
import argparse
from   ctypes import *
from functools import reduce
from PeInfo import *

"""
This utility supports some operations for Intel FSP 1.x/2.x image.
It supports:
    - Display FSP 1.x/2.x information header
    - Split FSP 2.x image into individual FSP-T/M/S/O component
    - Rebase FSP 1.x/2.x components to a different base address
    - Generate FSP 1.x/2.x mapping C header file
"""

CopyRightHeaderFile = """/*
 *
 * Automatically generated file; DO NOT EDIT.
 * FSP mapping file
 *
 */
"""

def IsIntegerType (val):
    if sys.version_info[0] < 3:
        if type(val) in (int, long):
            return True
    else:
        if type(val) is int:
            return True
    return False

def IsStrType (val):
    if sys.version_info[0] < 3:
        if type(val) is str:
            return True
    else:
        if type(val) is bytes:
            return True
    return False

def HandleNameStr (val):
    if sys.version_info[0] < 3:
        rep = "0x%X ('%s')" % (Bytes2Val (bytearray (val)), val)
    else:
        rep = "0x%X ('%s')" % (Bytes2Val (bytearray (val)), str (val, 'utf-8'))
    return rep

def OutputStruct (obj, indent = 0, plen = 0):
    if indent:
        body = ''
    else:
        body = ('  ' * indent + '<%s>:\n') % obj.__class__.__name__

    if plen == 0:
        plen = sizeof(obj)

    max_key_len = 26
    pstr = ('  ' * (indent + 1) + '{0:<%d} = {1}\n') % max_key_len

    for field in obj._fields_:
        key = field[0]
        val = getattr(obj, key)
        rep = ''
        if not isinstance(val, c_uint24) and isinstance(val, Structure):
            body += pstr.format(key, val.__class__.__name__)
            body += OutputStruct (val, indent + 1)
            plen -= sizeof(val)
        else:
            if IsStrType (val):
                rep = HandleNameStr (val)
            elif IsIntegerType (val):
                if (key == 'ImageRevision'):
                    FspImageRevisionMajor       = ((val >> 24) & 0xFF)
                    FspImageRevisionMinor       = ((val >> 16) & 0xFF)
                    FspImageRevisionRevision    = ((val >> 8) & 0xFF)
                    FspImageRevisionBuildNumber = (val & 0xFF)
                    rep = '0x%08X' % val
                elif (key == 'ExtendedImageRevision'):
                    FspImageRevisionRevision    |= (val & 0xFF00)
                    FspImageRevisionBuildNumber |= ((val << 8) & 0xFF00)
                    rep = "0x%04X ('%02X.%02X.%04X.%04X')" % (val, FspImageRevisionMajor, FspImageRevisionMinor, FspImageRevisionRevision, FspImageRevisionBuildNumber)
                elif field[1] == c_uint64:
                    rep = '0x%016X' % val
                elif field[1] == c_uint32:
                    rep = '0x%08X' % val
                elif field[1] == c_uint16:
                    rep = '0x%04X' % val
                elif field[1] == c_uint8:
                    rep = '0x%02X' % val
                else:
                    rep = '0x%X' % val
            elif isinstance(val, c_uint24):
                rep = '0x%X' % val.get_value()
            elif 'c_ubyte_Array' in str(type(val)):
                if sizeof(val) == 16:
                    if sys.version_info[0] < 3:
                        rep = str(bytearray(val))
                    else:
                        rep = bytes(val)
                    rep = str(uuid.UUID(bytes_le = rep)).upper()
                else:
                    res = ['0x%02X'%i for i in bytearray(val)]
                    rep = '[%s]' % (','.join(res))
            else:
                rep = str(val)
            plen -= sizeof(field[1])
            body += pstr.format(key, rep)
        if plen <= 0:
            break
    return body

class Section:
    def __init__(self, offset, secdata):
        self.SecHdr   = EFI_COMMON_SECTION_HEADER.from_buffer (secdata, 0)
        self.SecData  = secdata[0:int(self.SecHdr.Size)]
        self.Offset   = offset

class FirmwareFile:
    def __init__(self, offset, filedata):
        self.FfsHdr   = EFI_FFS_FILE_HEADER.from_buffer (filedata, 0)
        self.FfsData  = filedata[0:int(self.FfsHdr.Size)]
        self.Offset   = offset
        self.SecList  = []

    def ParseFfs(self):
        ffssize = len(self.FfsData)
        offset  = sizeof(self.FfsHdr)
        if self.FfsHdr.Name != '\xff' * 16:
            while offset < (ffssize - sizeof (EFI_COMMON_SECTION_HEADER)):
                sechdr = EFI_COMMON_SECTION_HEADER.from_buffer (self.FfsData, offset)
                sec = Section (offset, self.FfsData[offset:offset + int(sechdr.Size)])
                self.SecList.append(sec)
                offset += int(sechdr.Size)
                offset  = AlignPtr(offset, 4)

class FirmwareVolume:
    def __init__(self, offset, fvdata):
        self.FvHdr    = EFI_FIRMWARE_VOLUME_HEADER.from_buffer (fvdata, 0)
        self.FvData   = fvdata[0 : self.FvHdr.FvLength]
        self.Offset   = offset
        if self.FvHdr.ExtHeaderOffset > 0:
            self.FvExtHdr = EFI_FIRMWARE_VOLUME_EXT_HEADER.from_buffer (self.FvData, self.FvHdr.ExtHeaderOffset)
        else:
            self.FvExtHdr = None
        self.FfsList  = []
        self.ChildFvList  = []

    def ParseFv(self):
        fvsize = len(self.FvData)
        if self.FvExtHdr:
            offset = self.FvHdr.ExtHeaderOffset + self.FvExtHdr.ExtHeaderSize
        else:
            offset = self.FvHdr.HeaderLength
        offset = AlignPtr(offset)
        while offset < (fvsize - sizeof (EFI_FFS_FILE_HEADER)):
            ffshdr = EFI_FFS_FILE_HEADER.from_buffer (self.FvData, offset)
            if (ffshdr.Name == '\xff' * 16) and (int(ffshdr.Size) == 0xFFFFFF):
                offset = fvsize
            else:
                ffs = FirmwareFile (offset, self.FvData[offset:offset + int(ffshdr.Size)])
                # check if there is child fv
                childfvfound = 0
                if (ffs.FfsHdr.Type == EFI_FV_FILETYPE.FIRMWARE_VOLUME_IMAGE):
                    csoffset = offset + sizeof (EFI_FFS_FILE_HEADER)
                    csoffset = AlignPtr(csoffset, 4)
                    # find fv section
                    while csoffset < (offset + int(ffs.FfsHdr.Size)):
                        cshdr = EFI_COMMON_SECTION_HEADER.from_buffer (self.FvData, csoffset)
                        if (cshdr.Type == EFI_SECTION_TYPE.FIRMWARE_VOLUME_IMAGE):
                            childfvfound = 1
                            break
                        else:
                            # check next section
                            csoffset += int(cshdr.Size)
                            csoffset = AlignPtr(csoffset, 4)
                if (childfvfound):
                    childfvoffset = csoffset + sizeof (EFI_COMMON_SECTION_HEADER)
                    childfvhdr = EFI_FIRMWARE_VOLUME_HEADER.from_buffer (self.FvData, childfvoffset)
                    childfv = FirmwareVolume (childfvoffset, self.FvData[childfvoffset:childfvoffset + int(childfvhdr.FvLength)])
                    childfv.ParseFv ()
                    self.ChildFvList.append(childfv)
                else:
                    ffs.ParseFfs()
                    self.FfsList.append(ffs)
                offset += int(ffshdr.Size)
                offset = AlignPtr(offset)

class FspImage:
    def __init__(self, offset, fih, fihoff, patch):
        self.Fih       = fih
        self.FihOffset = fihoff
        self.Offset    = offset
        self.FvIdxList = []
        self.Type      = "XTMSIXXXOXXXXXXX"[(fih.ComponentAttribute >> 12) & 0x0F]
        self.PatchList = patch
        self.PatchList.append(fihoff + 0x1C)

    def AppendFv(self, FvIdx):
        self.FvIdxList.append(FvIdx)

    def Patch(self, delta, fdbin):
        count   = 0
        applied = 0
        for idx, patch in enumerate(self.PatchList):
            ptype = (patch>>24) & 0x0F
            if ptype not in [0x00, 0x0F]:
                raise Exception('ERROR: Invalid patch type %d !' % ptype)
            if patch & 0x80000000:
                patch = self.Fih.ImageSize - (0x1000000 - (patch & 0xFFFFFF))
            else:
                patch = patch & 0xFFFFFF
            if (patch < self.Fih.ImageSize) and (patch + sizeof(c_uint32) <= self.Fih.ImageSize):
                offset = patch + self.Offset
                value  = Bytes2Val(fdbin[offset:offset+sizeof(c_uint32)])
                value += delta
                fdbin[offset:offset+sizeof(c_uint32)] = Val2Bytes(value, sizeof(c_uint32))
                applied += 1
            count += 1
        # Don't count the FSP base address patch entry appended at the end
        if count != 0:
            count   -= 1
            applied -= 1
        return (count, applied)

class FirmwareDevice:
    def __init__(self, offset, fdfile):
        self.FvList  = []
        self.FspList = []
        self.FdFile = fdfile
        self.Offset = 0
        hfsp = open (self.FdFile, 'rb')
        self.FdData = bytearray(hfsp.read())
        hfsp.close()

    def ParseFd(self):
        offset = 0
        fdsize = len(self.FdData)
        self.FvList  = []
        while offset < (fdsize - sizeof (EFI_FIRMWARE_VOLUME_HEADER)):
            fvh = EFI_FIRMWARE_VOLUME_HEADER.from_buffer (self.FdData, offset)
            if b'_FVH' != fvh.Signature:
                raise Exception("ERROR: Invalid FV header !")
            fv = FirmwareVolume (offset, self.FdData[offset:offset + fvh.FvLength])
            fv.ParseFv ()
            self.FvList.append(fv)
            offset += fv.FvHdr.FvLength

    def CheckFsp (self):
        if len(self.FspList) == 0:
            return

        fih = None
        for fsp in self.FspList:
            if not fih:
                fih = fsp.Fih
            else:
                newfih = fsp.Fih
                if (newfih.ImageId != fih.ImageId) or (newfih.ImageRevision != fih.ImageRevision):
                    raise Exception("ERROR: Inconsistent FSP ImageId or ImageRevision detected !")

    def ParseFsp(self):
        flen = 0
        for idx, fv in enumerate(self.FvList):
            # Check if this FV contains FSP header
            if flen == 0:
                if len(fv.FfsList) == 0:
                    continue
                ffs = fv.FfsList[0]
                if len(ffs.SecList) == 0:
                    continue
                sec = ffs.SecList[0]
                if sec.SecHdr.Type != EFI_SECTION_TYPE.RAW:
                    continue
                fihoffset = ffs.Offset + sec.Offset + sizeof(sec.SecHdr)
                fspoffset = fv.Offset
                offset    = fspoffset + fihoffset
                fih = FSP_INFORMATION_HEADER.from_buffer (self.FdData, offset)
                if b'FSPH' != fih.Signature:
                    continue

                offset += fih.HeaderLength
                offset = AlignPtr(offset, 4)
                plist  = []
                while True:
                    fch = FSP_COMMON_HEADER.from_buffer (self.FdData, offset)
                    if b'FSPP' != fch.Signature:
                        offset += fch.HeaderLength
                        offset = AlignPtr(offset, 4)
                    else:
                        fspp = FSP_PATCH_TABLE.from_buffer (self.FdData, offset)
                        offset += sizeof(fspp)
                        pdata  = (c_uint32 * fspp.PatchEntryNum).from_buffer(self.FdData, offset)
                        plist  = list(pdata)
                        break

                fsp  = FspImage (fspoffset, fih, fihoffset, plist)
                fsp.AppendFv (idx)
                self.FspList.append(fsp)
                flen = fsp.Fih.ImageSize - fv.FvHdr.FvLength
            else:
                fsp.AppendFv (idx)
                flen -= fv.FvHdr.FvLength
                if flen < 0:
                    raise Exception("ERROR: Incorrect FV size in image !")
        self.CheckFsp ()



def ShowFspInfo (fspfile):
    fd = FirmwareDevice(0, fspfile)
    fd.ParseFd  ()
    fd.ParseFsp ()

    print ("\nFound the following %d Firmware Volumes in FSP binary:" % (len(fd.FvList)))
    for idx, fv in enumerate(fd.FvList):
        name = fv.FvExtHdr.FvName
        if not name:
            name = '\xff' * 16
        else:
            if sys.version_info[0] < 3:
                name = str(bytearray(name))
            else:
                name = bytes(name)
        guid = uuid.UUID(bytes_le = name)
        print ("FV%d:" % idx)
        print ("  GUID   : %s" % str(guid).upper())
        print ("  Offset : 0x%08X" %  fv.Offset)
        print ("  Length : 0x%08X" % fv.FvHdr.FvLength)
    print ("\n")

    for fsp in fd.FspList:
        fvlist = map(lambda x : 'FV%d' % x, fsp.FvIdxList)
        print ("FSP_%s contains %s" % (fsp.Type, ','.join(fvlist)))
        print ("%s" % (OutputStruct(fsp.Fih, 0, fsp.Fih.HeaderLength)))

def GenFspHdr (fspfile, outdir, hfile):
    fd = FirmwareDevice(0, fspfile)
    fd.ParseFd  ()
    fd.ParseFsp ()

    if not hfile:
        hfile = os.path.splitext(os.path.basename(fspfile))[0] + '.h'
    fspname, ext = os.path.splitext(os.path.basename(hfile))
    filename = os.path.join(outdir, fspname + ext)
    hfsp   = open(filename, 'w')
    hfsp.write ('%s\n\n' % CopyRightHeaderFile)

    firstfv = True
    for fsp in fd.FspList:
        fih = fsp.Fih
        if firstfv:
            if sys.version_info[0] < 3:
                hfsp.write("#define  FSP_IMAGE_ID    0x%016X    /* '%s' */\n" % (Bytes2Val(bytearray(fih.ImageId)), fih.ImageId))
            else:
                hfsp.write("#define  FSP_IMAGE_ID    0x%016X    /* '%s' */\n" % (Bytes2Val(bytearray(fih.ImageId)), str (fih.ImageId, 'utf-8')))
            hfsp.write("#define  FSP_IMAGE_REV   0x%08X \n\n" % fih.ImageRevision)
            firstfv = False
        fv = fd.FvList[fsp.FvIdxList[0]]
        hfsp.write ('#define  FSP%s_BASE       0x%08X\n'   % (fsp.Type, fih.ImageBase))
        hfsp.write ('#define  FSP%s_OFFSET     0x%08X\n'   % (fsp.Type, fv.Offset))
        hfsp.write ('#define  FSP%s_LENGTH     0x%08X\n\n' % (fsp.Type, fih.ImageSize))

    hfsp.close()

def SplitFspBin (fspfile, outdir, nametemplate):
    fd = FirmwareDevice(0, fspfile)
    fd.ParseFd  ()
    fd.ParseFsp ()

    for fsp in fd.FspList:
        if fsp.Fih.HeaderRevision < 3:
            raise Exception("ERROR: FSP 1.x is not supported by the split command !")
        ftype = fsp.Type
        if not nametemplate:
            nametemplate = fspfile
        fspname, ext = os.path.splitext(os.path.basename(nametemplate))
        filename = os.path.join(outdir, fspname + '_' + fsp.Type + ext)
        hfsp = open(filename, 'wb')
        print ("Create FSP component file '%s'" % filename)
        for fvidx in fsp.FvIdxList:
            fv = fd.FvList[fvidx]
            hfsp.write(fv.FvData)
        hfsp.close()

def GetImageFromFv (fd, parentfvoffset, fv, imglist):
    for ffs in fv.FfsList:
        for sec in ffs.SecList:
            if sec.SecHdr.Type in [EFI_SECTION_TYPE.TE, EFI_SECTION_TYPE.PE32]:   # TE or PE32
                offset = fd.Offset + parentfvoffset + fv.Offset + ffs.Offset + sec.Offset + sizeof(sec.SecHdr)
                imglist.append ((offset, len(sec.SecData) - sizeof(sec.SecHdr)))

def RebaseFspBin (FspBinary, FspComponent, FspBase, OutputDir, OutputFile):
    fd = FirmwareDevice(0, FspBinary)
    fd.ParseFd  ()
    fd.ParseFsp ()

    numcomp  = len(FspComponent)
    baselist = FspBase
    if numcomp != len(baselist):
        print ("ERROR: Required number of base does not match number of FSP component !")
        return

    newfspbin = fd.FdData[:]

    for idx, fspcomp in enumerate(FspComponent):

        found = False
        for fsp in fd.FspList:
            # Is this FSP 1.x single binary?
            if fsp.Fih.HeaderRevision < 3:
                found = True
                ftype = 'X'
                break
            ftype = fsp.Type.lower()
            if ftype == fspcomp:
                found = True
                break

        if not found:
            print ("ERROR: Could not find FSP_%c component to rebase !" % fspcomp.upper())
            return

        fspbase = baselist[idx]
        if fspbase.startswith('0x'):
            newbase = int(fspbase, 16)
        else:
            newbase = int(fspbase)
        oldbase = fsp.Fih.ImageBase
        delta = newbase - oldbase
        print ("Rebase FSP-%c from 0x%08X to 0x%08X:" % (ftype.upper(),oldbase,newbase))

        imglist = []
        for fvidx in fsp.FvIdxList:
            fv = fd.FvList[fvidx]
            GetImageFromFv (fd, 0, fv, imglist)
            # get image from child fv
            for childfv in fv.ChildFvList:
                print ("Get image from child fv of fv%d, parent fv offset: 0x%x" % (fvidx, fv.Offset))
                GetImageFromFv (fd, fv.Offset, childfv, imglist)

        fcount  = 0
        pcount  = 0
        for (offset, length) in imglist:
            img = PeTeImage(offset, fd.FdData[offset:offset + length])
            img.ParseReloc()
            pcount += img.Rebase(delta, newfspbin)
            fcount += 1

        print ("  Patched %d entries in %d TE/PE32 images." % (pcount, fcount))

        (count, applied) = fsp.Patch(delta, newfspbin)
        print ("  Patched %d entries using FSP patch table." % applied)
        if count != applied:
            print ("  %d invalid entries are ignored !" % (count - applied))

    if OutputFile == '':
        filename = os.path.basename(FspBinary)
        base, ext  = os.path.splitext(filename)
        OutputFile = base + "_%08X" % newbase + ext

    fspname, ext = os.path.splitext(os.path.basename(OutputFile))
    filename = os.path.join(OutputDir, fspname + ext)
    fd = open(filename, "wb")
    fd.write(newfspbin)
    fd.close()

def main ():
    parser     = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(title='commands', dest="which")

    parser_rebase  = subparsers.add_parser('rebase',  help='rebase a FSP into a new base address')
    parser_rebase.set_defaults(which='rebase')
    parser_rebase.add_argument('-f',  '--fspbin' , dest='FspBinary',  type=str, help='FSP binary file path', required = True)
    parser_rebase.add_argument('-c',  '--fspcomp', choices=['t','m','s','o','i'],  nargs='+', dest='FspComponent', type=str, help='FSP component to rebase', default = "['t']", required = True)
    parser_rebase.add_argument('-b',  '--newbase', dest='FspBase', nargs='+', type=str, help='Rebased FSP binary file name', default = '', required = True)
    parser_rebase.add_argument('-o',  '--outdir' , dest='OutputDir',  type=str, help='Output directory path', default = '.')
    parser_rebase.add_argument('-n',  '--outfile', dest='OutputFile', type=str, help='Rebased FSP binary file name', default = '')

    parser_split  = subparsers.add_parser('split',  help='split a FSP into multiple components')
    parser_split.set_defaults(which='split')
    parser_split.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)
    parser_split.add_argument('-o',  '--outdir' , dest='OutputDir', type=str, help='Output directory path',   default = '.')
    parser_split.add_argument('-n',  '--nametpl', dest='NameTemplate', type=str, help='Output name template', default = '')

    parser_genhdr = subparsers.add_parser('genhdr',  help='generate a header file for FSP binary')
    parser_genhdr.set_defaults(which='genhdr')
    parser_genhdr.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)
    parser_genhdr.add_argument('-o',  '--outdir' , dest='OutputDir', type=str, help='Output directory path',   default = '.')
    parser_genhdr.add_argument('-n',  '--hfile',   dest='HFileName', type=str, help='Output header file name', default = '')

    parser_info = subparsers.add_parser('info',  help='display FSP information')
    parser_info.set_defaults(which='info')
    parser_info.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)

    args = parser.parse_args()
    if args.which in ['rebase', 'split', 'genhdr', 'info']:
        if not os.path.exists(args.FspBinary):
            raise Exception ("ERROR: Could not locate FSP binary file '%s' !" % args.FspBinary)
        if hasattr(args, 'OutputDir') and not os.path.exists(args.OutputDir):
            raise Exception ("ERROR: Invalid output directory '%s' !" % args.OutputDir)

    if args.which == 'rebase':
        RebaseFspBin (args.FspBinary, args.FspComponent, args.FspBase, args.OutputDir, args.OutputFile)
    elif args.which == 'split':
        SplitFspBin (args.FspBinary, args.OutputDir, args.NameTemplate)
    elif args.which == 'genhdr':
        GenFspHdr (args.FspBinary, args.OutputDir, args.HFileName)
    elif args.which == 'info':
        ShowFspInfo (args.FspBinary)
    else:
        parser.print_help()

    return 0

if __name__ == '__main__':
    sys.exit(main())
