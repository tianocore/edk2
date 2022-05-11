# @ ConfigEditor.py
#
# Copyright(c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import sys
import marshal
import tkinter
import tkinter.ttk as ttk
import tkinter.messagebox as messagebox
import tkinter.filedialog as filedialog

from pickle import FALSE, TRUE
from pathlib import Path
from GenYamlCfg import CGenYamlCfg, bytes_to_value, \
      bytes_to_bracket_str, value_to_bytes, array_str_to_value
from ctypes import sizeof, Structure, ARRAY, c_uint8, c_uint64, c_char, \
     c_uint32, c_uint16
from functools import reduce

sys.path.insert(0, '..')
from FspDscBsf2Yaml import bsf_to_dsc, dsc_to_yaml  # noqa


sys.dont_write_bytecode = True


class create_tool_tip(object):
    '''
    create a tooltip for a given widget
    '''
    in_progress = False

    def __init__(self, widget, text=''):
        self.top_win = None
        self.widget = widget
        self.text = text
        self.widget.bind("<Enter>", self.enter)
        self.widget.bind("<Leave>", self.leave)

    def enter(self, event=None):
        if self.in_progress:
            return
        if self.widget.winfo_class() == 'Treeview':
            # Only show help when cursor is on row header.
            rowid = self.widget.identify_row(event.y)
            if rowid != '':
                return
        else:
            x, y, cx, cy = self.widget.bbox("insert")

        cursor = self.widget.winfo_pointerxy()
        x = self.widget.winfo_rootx() + 35
        y = self.widget.winfo_rooty() + 20
        if cursor[1] > y and cursor[1] < y + 20:
            y += 20

        # creates a toplevel window
        self.top_win = tkinter.Toplevel(self.widget)
        # Leaves only the label and removes the app window
        self.top_win.wm_overrideredirect(True)
        self.top_win.wm_geometry("+%d+%d" % (x, y))
        label = tkinter.Message(self.top_win,
                                text=self.text,
                                justify='left',
                                background='bisque',
                                relief='solid',
                                borderwidth=1,
                                font=("times", "10", "normal"))
        label.pack(ipadx=1)
        self.in_progress = True

    def leave(self, event=None):
        if self.top_win:
            self.top_win.destroy()
            self.in_progress = False


class validating_entry(tkinter.Entry):
    def __init__(self, master, **kw):
        tkinter.Entry.__init__(*(self, master), **kw)
        self.parent = master
        self.old_value = ''
        self.last_value = ''
        self.variable = tkinter.StringVar()
        self.variable.trace("w", self.callback)
        self.config(textvariable=self.variable)
        self.config({"background": "#c0c0c0"})
        self.bind("<Return>", self.move_next)
        self.bind("<Tab>", self.move_next)
        self.bind("<Escape>", self.cancel)
        for each in ['BackSpace', 'Delete']:
            self.bind("<%s>" % each, self.ignore)
        self.display(None)

    def ignore(self, even):
        return "break"

    def move_next(self, event):
        if self.row < 0:
            return
        row, col = self.row, self.col
        txt, row_id, col_id = self.parent.get_next_cell(row, col)
        self.display(txt, row_id, col_id)
        return "break"

    def cancel(self, event):
        self.variable.set(self.old_value)
        self.display(None)

    def display(self, txt, row_id='', col_id=''):
        if txt is None:
            self.row = -1
            self.col = -1
            self.place_forget()
        else:
            row = int('0x' + row_id[1:], 0) - 1
            col = int(col_id[1:]) - 1
            self.row = row
            self.col = col
            self.old_value = txt
            self.last_value = txt
            x, y, width, height = self.parent.bbox(row_id, col)
            self.place(x=x, y=y, w=width)
            self.variable.set(txt)
            self.focus_set()
            self.icursor(0)

    def callback(self, *Args):
        cur_val = self.variable.get()
        new_val = self.validate(cur_val)
        if new_val is not None and self.row >= 0:
            self.last_value = new_val
            self.parent.set_cell(self.row, self.col, new_val)
        self.variable.set(self.last_value)

    def validate(self, value):
        if len(value) > 0:
            try:
                int(value, 16)
            except Exception:
                return None

        # Normalize the cell format
        self.update()
        cell_width = self.winfo_width()
        max_len = custom_table.to_byte_length(cell_width) * 2
        cur_pos = self.index("insert")
        if cur_pos == max_len + 1:
            value = value[-max_len:]
        else:
            value = value[:max_len]
        if value == '':
            value = '0'
        fmt = '%%0%dX' % max_len
        return fmt % int(value, 16)


class custom_table(ttk.Treeview):
    _Padding = 20
    _Char_width = 6

    def __init__(self, parent, col_hdr, bins):
        cols = len(col_hdr)

        col_byte_len = []
        for col in range(cols):  # Columns
            col_byte_len.append(int(col_hdr[col].split(':')[1]))

        byte_len = sum(col_byte_len)
        rows = (len(bins) + byte_len - 1) // byte_len

        self.rows = rows
        self.cols = cols
        self.col_byte_len = col_byte_len
        self.col_hdr = col_hdr

        self.size = len(bins)
        self.last_dir = ''

        style = ttk.Style()
        style.configure("Custom.Treeview.Heading",
                        font=('calibri', 10, 'bold'),
                        foreground="blue")
        ttk.Treeview.__init__(self, parent, height=rows,
                              columns=[''] + col_hdr, show='headings',
                              style="Custom.Treeview",
                              selectmode='none')
        self.bind("<Button-1>", self.click)
        self.bind("<FocusOut>", self.focus_out)
        self.entry = validating_entry(self, width=4,  justify=tkinter.CENTER)

        self.heading(0, text='LOAD')
        self.column(0, width=60, stretch=0, anchor=tkinter.CENTER)

        for col in range(cols):  # Columns
            text = col_hdr[col].split(':')[0]
            byte_len = int(col_hdr[col].split(':')[1])
            self.heading(col+1, text=text)
            self.column(col+1, width=self.to_cell_width(byte_len),
                        stretch=0, anchor=tkinter.CENTER)
        idx = 0
        for row in range(rows):  # Rows
            text = '%04X' % (row * len(col_hdr))
            vals = ['%04X:' % (cols * row)]
            for col in range(cols):  # Columns
                if idx >= len(bins):
                    break
                byte_len = int(col_hdr[col].split(':')[1])
                value = bytes_to_value(bins[idx:idx+byte_len])
                hex = ("%%0%dX" % (byte_len * 2)) % value
                vals.append(hex)
                idx += byte_len
            self.insert('', 'end', values=tuple(vals))
            if idx >= len(bins):
                break

    @staticmethod
    def to_cell_width(byte_len):
        return byte_len * 2 * custom_table._Char_width + custom_table._Padding

    @staticmethod
    def to_byte_length(cell_width):
        return(cell_width - custom_table._Padding) \
               // (2 * custom_table._Char_width)

    def focus_out(self, event):
        self.entry.display(None)

    def refresh_bin(self, bins):
        if not bins:
            return

        # Reload binary into widget
        bin_len = len(bins)
        for row in range(self.rows):
            iid = self.get_children()[row]
            for col in range(self.cols):
                idx = row * sum(self.col_byte_len) + \
                      sum(self.col_byte_len[:col])
                byte_len = self.col_byte_len[col]
                if idx + byte_len <= self.size:
                    byte_len = int(self.col_hdr[col].split(':')[1])
                    if idx + byte_len > bin_len:
                        val = 0
                    else:
                        val = bytes_to_value(bins[idx:idx+byte_len])
                    hex_val = ("%%0%dX" % (byte_len * 2)) % val
                    self.set(iid, col + 1, hex_val)

    def get_cell(self, row, col):
        iid = self.get_children()[row]
        txt = self.item(iid, 'values')[col]
        return txt

    def get_next_cell(self, row, col):
        rows = self.get_children()
        col += 1
        if col > self.cols:
            col = 1
            row += 1
        cnt = row * sum(self.col_byte_len) + sum(self.col_byte_len[:col])
        if cnt > self.size:
            # Reached the last cell, so roll back to beginning
            row = 0
            col = 1

        txt = self.get_cell(row, col)
        row_id = rows[row]
        col_id = '#%d' % (col + 1)
        return(txt, row_id, col_id)

    def set_cell(self, row, col, val):
        iid = self.get_children()[row]
        self.set(iid, col, val)

    def load_bin(self):
        # Load binary from file
        path = filedialog.askopenfilename(
            initialdir=self.last_dir,
            title="Load binary file",
            filetypes=(("Binary files", "*.bin"), (
                "binary files", "*.bin")))
        if path:
            self.last_dir = os.path.dirname(path)
            fd = open(path, 'rb')
            bins = bytearray(fd.read())[:self.size]
            fd.close()
            bins.extend(b'\x00' * (self.size - len(bins)))
            return bins

        return None

    def click(self, event):
        row_id = self.identify_row(event.y)
        col_id = self.identify_column(event.x)
        if row_id == '' and col_id == '#1':
            # Clicked on "LOAD" cell
            bins = self.load_bin()
            self.refresh_bin(bins)
            return

        if col_id == '#1':
            # Clicked on column 1(Offset column)
            return

        item = self.identify('item', event.x, event.y)
        if not item or not col_id:
            # Not clicked on valid cell
            return

        # Clicked cell
        row = int('0x' + row_id[1:], 0) - 1
        col = int(col_id[1:]) - 1
        if row * self.cols + col > self.size:
            return

        vals = self.item(item, 'values')
        if col < len(vals):
            txt = self.item(item, 'values')[col]
            self.entry.display(txt, row_id, col_id)

    def get(self):
        bins = bytearray()
        row_ids = self.get_children()
        for row_id in row_ids:
            row = int('0x' + row_id[1:], 0) - 1
            for col in range(self.cols):
                idx = row * sum(self.col_byte_len) + \
                      sum(self.col_byte_len[:col])
                byte_len = self.col_byte_len[col]
                if idx + byte_len > self.size:
                    break
                hex = self.item(row_id, 'values')[col + 1]
                values = value_to_bytes(int(hex, 16)
                                        & ((1 << byte_len * 8) - 1), byte_len)
                bins.extend(values)
        return bins


class c_uint24(Structure):
    """Little-Endian 24-bit Unsigned Integer"""
    _pack_ = 1
    _fields_ = [('Data', (c_uint8 * 3))]

    def __init__(self, val=0):
        self.set_value(val)

    def __str__(self, indent=0):
        return '0x%.6x' % self.value

    def __int__(self):
        return self.get_value()

    def set_value(self, val):
        self.Data[0:3] = Val2Bytes(val, 3)

    def get_value(self):
        return Bytes2Val(self.Data[0:3])

    value = property(get_value, set_value)


class EFI_FIRMWARE_VOLUME_HEADER(Structure):
    _fields_ = [
        ('ZeroVector',           ARRAY(c_uint8, 16)),
        ('FileSystemGuid',       ARRAY(c_uint8, 16)),
        ('FvLength',             c_uint64),
        ('Signature',            ARRAY(c_char, 4)),
        ('Attributes',           c_uint32),
        ('HeaderLength',         c_uint16),
        ('Checksum',             c_uint16),
        ('ExtHeaderOffset',      c_uint16),
        ('Reserved',             c_uint8),
        ('Revision',             c_uint8)
        ]


class EFI_FIRMWARE_VOLUME_EXT_HEADER(Structure):
    _fields_ = [
        ('FvName',               ARRAY(c_uint8, 16)),
        ('ExtHeaderSize',        c_uint32)
        ]


class EFI_FFS_INTEGRITY_CHECK(Structure):
    _fields_ = [
        ('Header',               c_uint8),
        ('File',                 c_uint8)
        ]


class EFI_FFS_FILE_HEADER(Structure):
    _fields_ = [
        ('Name',                 ARRAY(c_uint8, 16)),
        ('IntegrityCheck',       EFI_FFS_INTEGRITY_CHECK),
        ('Type',                 c_uint8),
        ('Attributes',           c_uint8),
        ('Size',                 c_uint24),
        ('State',                c_uint8)
        ]


class EFI_COMMON_SECTION_HEADER(Structure):
    _fields_ = [
        ('Size',                 c_uint24),
        ('Type',                 c_uint8)
        ]


class EFI_SECTION_TYPE:
    """Enumeration of all valid firmware file section types."""
    ALL = 0x00
    COMPRESSION = 0x01
    GUID_DEFINED = 0x02
    DISPOSABLE = 0x03
    PE32 = 0x10
    PIC = 0x11
    TE = 0x12
    DXE_DEPEX = 0x13
    VERSION = 0x14
    USER_INTERFACE = 0x15
    COMPATIBILITY16 = 0x16
    FIRMWARE_VOLUME_IMAGE = 0x17
    FREEFORM_SUBTYPE_GUID = 0x18
    RAW = 0x19
    PEI_DEPEX = 0x1b
    SMM_DEPEX = 0x1c


class FSP_COMMON_HEADER(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32)
        ]


class FSP_INFORMATION_HEADER(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32),
        ('Reserved1',            c_uint16),
        ('SpecVersion',          c_uint8),
        ('HeaderRevision',       c_uint8),
        ('ImageRevision',        c_uint32),
        ('ImageId',              ARRAY(c_char, 8)),
        ('ImageSize',            c_uint32),
        ('ImageBase',            c_uint32),
        ('ImageAttribute',       c_uint16),
        ('ComponentAttribute',   c_uint16),
        ('CfgRegionOffset',      c_uint32),
        ('CfgRegionSize',        c_uint32),
        ('Reserved2',            c_uint32),
        ('TempRamInitEntryOffset',     c_uint32),
        ('Reserved3',                  c_uint32),
        ('NotifyPhaseEntryOffset',     c_uint32),
        ('FspMemoryInitEntryOffset',   c_uint32),
        ('TempRamExitEntryOffset',     c_uint32),
        ('FspSiliconInitEntryOffset',  c_uint32),
        ('FspMultiPhaseSiInitEntryOffset', c_uint32),
        ('ExtendedImageRevision',  c_uint16),
        ('Reserved4',  c_uint16)
        ]


class FSP_EXTENDED_HEADER(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32),
        ('Revision',             c_uint8),
        ('Reserved',             c_uint8),
        ('FspProducerId',        ARRAY(c_char, 6)),
        ('FspProducerRevision',  c_uint32),
        ('FspProducerDataSize',  c_uint32)
        ]


class FSP_PATCH_TABLE(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint16),
        ('HeaderRevision',       c_uint8),
        ('Reserved',             c_uint8),
        ('PatchEntryNum',        c_uint32)
        ]


class Section:
    def __init__(self, offset, secdata):
        self.SecHdr = EFI_COMMON_SECTION_HEADER.from_buffer(secdata, 0)
        self.SecData = secdata[0:int(self.SecHdr.Size)]
        self.Offset = offset


def AlignPtr(offset, alignment=8):
    return (offset + alignment - 1) & ~(alignment - 1)


def Bytes2Val(bytes):
    return reduce(lambda x, y: (x << 8) | y, bytes[:: -1])


def Val2Bytes(value, blen):
    return [(value >> (i*8) & 0xff) for i in range(blen)]


class FirmwareFile:
    def __init__(self, offset, filedata):
        self.FfsHdr = EFI_FFS_FILE_HEADER.from_buffer(filedata, 0)
        self.FfsData = filedata[0:int(self.FfsHdr.Size)]
        self.Offset = offset
        self.SecList = []

    def ParseFfs(self):
        ffssize = len(self.FfsData)
        offset = sizeof(self.FfsHdr)
        if self.FfsHdr.Name != '\xff' * 16:
            while offset < (ffssize - sizeof(EFI_COMMON_SECTION_HEADER)):
                sechdr = EFI_COMMON_SECTION_HEADER.from_buffer(
                    self.FfsData, offset)
                sec = Section(
                    offset, self.FfsData[offset:offset + int(sechdr.Size)])
                self.SecList.append(sec)
                offset += int(sechdr.Size)
                offset = AlignPtr(offset, 4)


class FirmwareVolume:
    def __init__(self, offset, fvdata):
        self.FvHdr = EFI_FIRMWARE_VOLUME_HEADER.from_buffer(fvdata, 0)
        self.FvData = fvdata[0: self.FvHdr.FvLength]
        self.Offset = offset
        if self.FvHdr.ExtHeaderOffset > 0:
            self.FvExtHdr = EFI_FIRMWARE_VOLUME_EXT_HEADER.from_buffer(
                self.FvData, self.FvHdr.ExtHeaderOffset)
        else:
            self.FvExtHdr = None
        self.FfsList = []

    def ParseFv(self):
        fvsize = len(self.FvData)
        if self.FvExtHdr:
            offset = self.FvHdr.ExtHeaderOffset + self.FvExtHdr.ExtHeaderSize
        else:
            offset = self.FvHdr.HeaderLength
        offset = AlignPtr(offset)
        while offset < (fvsize - sizeof(EFI_FFS_FILE_HEADER)):
            ffshdr = EFI_FFS_FILE_HEADER.from_buffer(self.FvData, offset)
            if (ffshdr.Name == '\xff' * 16) and \
                    (int(ffshdr.Size) == 0xFFFFFF):
                offset = fvsize
            else:
                ffs = FirmwareFile(
                    offset, self.FvData[offset:offset + int(ffshdr.Size)])
                ffs.ParseFfs()
                self.FfsList.append(ffs)
                offset += int(ffshdr.Size)
                offset = AlignPtr(offset)


class FspImage:
    def __init__(self, offset, fih, fihoff, patch):
        self.Fih = fih
        self.FihOffset = fihoff
        self.Offset = offset
        self.FvIdxList = []
        self.Type = "XTMSXXXXOXXXXXXX"[(fih.ComponentAttribute >> 12) & 0x0F]
        self.PatchList = patch
        self.PatchList.append(fihoff + 0x1C)

    def AppendFv(self, FvIdx):
        self.FvIdxList.append(FvIdx)

    def Patch(self, delta, fdbin):
        count = 0
        applied = 0
        for idx, patch in enumerate(self.PatchList):
            ptype = (patch >> 24) & 0x0F
            if ptype not in [0x00, 0x0F]:
                raise Exception('ERROR: Invalid patch type %d !' % ptype)
            if patch & 0x80000000:
                patch = self.Fih.ImageSize - (0x1000000 - (patch & 0xFFFFFF))
            else:
                patch = patch & 0xFFFFFF
            if (patch < self.Fih.ImageSize) and \
                    (patch + sizeof(c_uint32) <= self.Fih.ImageSize):
                offset = patch + self.Offset
                value = Bytes2Val(fdbin[offset:offset+sizeof(c_uint32)])
                value += delta
                fdbin[offset:offset+sizeof(c_uint32)] = Val2Bytes(
                    value, sizeof(c_uint32))
                applied += 1
            count += 1
        # Don't count the FSP base address patch entry appended at the end
        if count != 0:
            count -= 1
            applied -= 1
        return (count, applied)


class FirmwareDevice:
    def __init__(self, offset, FdData):
        self.FvList = []
        self.FspList = []
        self.FspExtList = []
        self.FihList = []
        self.BuildList = []
        self.OutputText = ""
        self.Offset = 0
        self.FdData = FdData

    def ParseFd(self):
        offset = 0
        fdsize = len(self.FdData)
        self.FvList = []
        while offset < (fdsize - sizeof(EFI_FIRMWARE_VOLUME_HEADER)):
            fvh = EFI_FIRMWARE_VOLUME_HEADER.from_buffer(self.FdData, offset)
            if b'_FVH' != fvh.Signature:
                raise Exception("ERROR: Invalid FV header !")
            fv = FirmwareVolume(
                offset, self.FdData[offset:offset + fvh.FvLength])
            fv.ParseFv()
            self.FvList.append(fv)
            offset += fv.FvHdr.FvLength

    def CheckFsp(self):
        if len(self.FspList) == 0:
            return

        fih = None
        for fsp in self.FspList:
            if not fih:
                fih = fsp.Fih
            else:
                newfih = fsp.Fih
                if (newfih.ImageId != fih.ImageId) or \
                        (newfih.ImageRevision != fih.ImageRevision):
                    raise Exception(
                        "ERROR: Inconsistent FSP ImageId or "
                        "ImageRevision detected !")

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
                offset = fspoffset + fihoffset
                fih = FSP_INFORMATION_HEADER.from_buffer(self.FdData, offset)
                self.FihList.append(fih)
                if b'FSPH' != fih.Signature:
                    continue

                offset += fih.HeaderLength

                offset = AlignPtr(offset, 2)
                Extfih = FSP_EXTENDED_HEADER.from_buffer(self.FdData, offset)
                self.FspExtList.append(Extfih)
                offset = AlignPtr(offset, 4)
                plist = []
                while True:
                    fch = FSP_COMMON_HEADER.from_buffer(self.FdData, offset)
                    if b'FSPP' != fch.Signature:
                        offset += fch.HeaderLength
                        offset = AlignPtr(offset, 4)
                    else:
                        fspp = FSP_PATCH_TABLE.from_buffer(
                            self.FdData, offset)
                        offset += sizeof(fspp)
                        start_offset = offset + 32
                        end_offset = offset + 32
                        while True:
                            end_offset += 1
                            if(self.FdData[
                                    end_offset: end_offset + 1] == b'\xff'):
                                break
                        self.BuildList.append(
                            self.FdData[start_offset:end_offset])
                        pdata = (c_uint32 * fspp.PatchEntryNum).from_buffer(
                            self.FdData, offset)
                        plist = list(pdata)
                        break

                fsp = FspImage(fspoffset, fih, fihoffset, plist)
                fsp.AppendFv(idx)
                self.FspList.append(fsp)
                flen = fsp.Fih.ImageSize - fv.FvHdr.FvLength
            else:
                fsp.AppendFv(idx)
                flen -= fv.FvHdr.FvLength
                if flen < 0:
                    raise Exception("ERROR: Incorrect FV size in image !")
        self.CheckFsp()

    def IsIntegerType(self, val):
        if sys.version_info[0] < 3:
            if type(val) in (int, long):
                return True
        else:
            if type(val) is int:
                return True
        return False

    def ConvertRevisionString(self, obj):
        for field in obj._fields_:
            key = field[0]
            val = getattr(obj, key)
            rep = ''

            if self.IsIntegerType(val):
                if (key == 'ImageRevision'):
                    FspImageRevisionMajor = ((val >> 24) & 0xFF)
                    FspImageRevisionMinor = ((val >> 16) & 0xFF)
                    FspImageRevisionRevision = ((val >> 8) & 0xFF)
                    FspImageRevisionBuildNumber = (val & 0xFF)
                    rep = '0x%08X' % val
                elif (key == 'ExtendedImageRevision'):
                    FspImageRevisionRevision |= (val & 0xFF00)
                    FspImageRevisionBuildNumber |= ((val << 8) & 0xFF00)
                    rep = "0x%04X ('%02X.%02X.%04X.%04X')" % (val, FspImageRevisionMajor, FspImageRevisionMinor, FspImageRevisionRevision, FspImageRevisionBuildNumber)
                    return rep

    def OutputFsp(self):
        def copy_text_to_clipboard():
            window.clipboard_clear()
            window.clipboard_append(self.OutputText)

        window = tkinter.Tk()
        window.title("Fsp Headers")
        window.resizable(0, 0)
        # Window Size
        window.geometry("300x400+350+150")
        frame = tkinter.Frame(window)
        frame.pack(side=tkinter.BOTTOM)
        # Vertical (y) Scroll Bar
        scroll = tkinter.Scrollbar(window)
        scroll.pack(side=tkinter.RIGHT, fill=tkinter.Y)
        text = tkinter.Text(window,
                            wrap=tkinter.NONE, yscrollcommand=scroll.set)
        i = 0
        self.OutputText = self.OutputText + "Fsp Header Details \n\n"
        while i < len(self.FihList):
            try:
                # self.OutputText += str(self.BuildList[i].decode()) + "\n"
                self.OutputText += str(self.BuildList[i]) + "\n"
            except Exception:
                self.OutputText += "No description found\n"
            self.OutputText += "FSP Header :\n "
            self.OutputText += "Signature : " + \
                str(self.FihList[i].Signature.decode('utf-8')) + "\n "
            self.OutputText += "Header Length : " + \
                str(hex(self.FihList[i].HeaderLength)) + "\n "
            self.OutputText += "Reserved1 : " + \
                str(hex(self.FihList[i].Reserved1)) + "\n "
            self.OutputText += "Header Revision : " + \
                str(hex(self.FihList[i].HeaderRevision)) + "\n "
            self.OutputText += "Spec Version : " + \
                str(hex(self.FihList[i].SpecVersion)) + "\n "
            self.OutputText += "Image Revision : " + \
                str(hex(self.FihList[i].ImageRevision)) + "\n "
            self.OutputText += "Image Id : " + \
                str(self.FihList[i].ImageId.decode('utf-8')) + "\n "
            self.OutputText += "Image Size : " + \
                str(hex(self.FihList[i].ImageSize)) + "\n "
            self.OutputText += "Image Base : " + \
                str(hex(self.FihList[i].ImageBase)) + "\n "
            self.OutputText += "Image Attribute : " + \
                str(hex(self.FihList[i].ImageAttribute)) + "\n "
            self.OutputText += "Component Attribute : " + \
                str(hex(self.FihList[i].ComponentAttribute)) + "\n "
            self.OutputText += "Cfg Region Offset : " + \
                str(hex(self.FihList[i].CfgRegionOffset)) + "\n "
            self.OutputText += "Cfg Region Size : " + \
                str(hex(self.FihList[i].CfgRegionSize)) + "\n "
            self.OutputText += "Reserved2 : " + \
                str(hex(self.FihList[i].Reserved2)) + "\n "
            self.OutputText += "Temp Ram Init Entry : " + \
                str(hex(self.FihList[i].TempRamInitEntryOffset)) + "\n "
            self.OutputText += "Reserved3 : " + \
                str(hex(self.FihList[i].Reserved3)) + "\n "
            self.OutputText += "Notify Phase Entry : " + \
                str(hex(self.FihList[i].NotifyPhaseEntryOffset)) + "\n "
            self.OutputText += "Fsp Memory Init Entry : " + \
                str(hex(self.FihList[i].FspMemoryInitEntryOffset)) + "\n "
            self.OutputText += "Temp Ram Exit Entry : " + \
                str(hex(self.FihList[i].TempRamExitEntryOffset)) + "\n "
            self.OutputText += "Fsp Silicon Init Entry : " + \
                str(hex(self.FihList[i].FspSiliconInitEntryOffset)) + "\n "
            self.OutputText += "Fsp Multi Phase Si Init Entry : " + \
                str(hex(self.FihList[i].FspMultiPhaseSiInitEntryOffset)) + "\n "

            # display ExtendedImageRevision & Reserved4 if HeaderRevision >= 6
            for fsp in self.FihList:
                if fsp.HeaderRevision >= 6:
                    Display_ExtndImgRev = TRUE
                else:
                    Display_ExtndImgRev = FALSE
                    self.OutputText += "\n"
            if  Display_ExtndImgRev == TRUE:
                self.OutputText += "ExtendedImageRevision : " + \
                    str(self.ConvertRevisionString(self.FihList[i])) + "\n "
                self.OutputText += "Reserved4 : " + \
                    str(hex(self.FihList[i].Reserved4)) + "\n\n"

            self.OutputText += "FSP Extended Header:\n "
            self.OutputText += "Signature : " + \
                str(self.FspExtList[i].Signature.decode('utf-8')) + "\n "
            self.OutputText += "Header Length : " + \
                str(hex(self.FspExtList[i].HeaderLength)) + "\n "
            self.OutputText += "Header Revision : " + \
                str(hex(self.FspExtList[i].Revision)) + "\n "
            self.OutputText += "Fsp Producer Id : " + \
                str(self.FspExtList[i].FspProducerId.decode('utf-8')) + "\n "
            self.OutputText += "FspProducerRevision : " + \
                str(hex(self.FspExtList[i].FspProducerRevision)) + "\n\n"
            i += 1
        text.insert(tkinter.INSERT, self.OutputText)
        text.pack()
        # Configure the scrollbars
        scroll.config(command=text.yview)
        copy_button = tkinter.Button(
            window, text="Copy to Clipboard", command=copy_text_to_clipboard)
        copy_button.pack(in_=frame, side=tkinter.LEFT, padx=20, pady=10)
        exit_button = tkinter.Button(
            window, text="Close", command=window.destroy)
        exit_button.pack(in_=frame, side=tkinter.RIGHT, padx=20, pady=10)
        window.mainloop()


class state:
    def __init__(self):
        self.state = False

    def set(self, value):
        self.state = value

    def get(self):
        return self.state


class application(tkinter.Frame):
    def __init__(self, master=None):
        root = master

        self.debug = True
        self.mode = 'FSP'
        self.last_dir = '.'
        self.page_id = ''
        self.page_list = {}
        self.conf_list = {}
        self.cfg_page_dict = {}
        self.cfg_data_obj = None
        self.org_cfg_data_bin = None
        self.in_left = state()
        self.in_right = state()
        self.search_text = ''

        # Check if current directory contains a file with a .yaml extension
        # if not default self.last_dir to a Platform directory where it is
        # easier to locate *BoardPkg\CfgData\*Def.yaml files
        self.last_dir = '.'
        if not any(fname.endswith('.yaml') for fname in os.listdir('.')):
            platform_path = Path(os.path.realpath(__file__)).parents[2].\
                            joinpath('Platform')
            if platform_path.exists():
                self.last_dir = platform_path

        tkinter.Frame.__init__(self, master, borderwidth=2)

        self.menu_string = [
            'Save Config Data to Binary', 'Load Config Data from Binary',
            'Show Binary Information',
            'Load Config Changes from Delta File',
            'Save Config Changes to Delta File',
            'Save Full Config Data to Delta File',
            'Open Config BSF file'
        ]

        root.geometry("1200x800")

        # Search string
        fram = tkinter.Frame(root)
        # adding label to search box
        tkinter.Label(fram, text='Text to find:').pack(side=tkinter.LEFT)
        # adding of single line text box
        self.edit = tkinter.Entry(fram, width=30)
        # positioning of text box
        self.edit.pack(
            side=tkinter.LEFT, fill=tkinter.BOTH, expand=1, padx=(4, 4))
        # setting focus
        self.edit.focus_set()
        # adding of search button
        butt = tkinter.Button(fram, text='Search', relief=tkinter.GROOVE,
                              command=self.search_bar)
        butt.pack(side=tkinter.RIGHT, padx=(4, 4))
        fram.pack(side=tkinter.TOP, anchor=tkinter.SE)

        paned = ttk.Panedwindow(root, orient=tkinter.HORIZONTAL)
        paned.pack(fill=tkinter.BOTH, expand=True, padx=(4, 4))

        status = tkinter.Label(master, text="", bd=1, relief=tkinter.SUNKEN,
                               anchor=tkinter.W)
        status.pack(side=tkinter.BOTTOM, fill=tkinter.X)

        frame_left = ttk.Frame(paned, height=800, relief="groove")

        self.left = ttk.Treeview(frame_left, show="tree")

        # Set up tree HScroller
        pady = (10, 10)
        self.tree_scroll = ttk.Scrollbar(frame_left,
                                         orient="vertical",
                                         command=self.left.yview)
        self.left.configure(yscrollcommand=self.tree_scroll.set)
        self.left.bind("<<TreeviewSelect>>", self.on_config_page_select_change)
        self.left.bind("<Enter>", lambda e: self.in_left.set(True))
        self.left.bind("<Leave>", lambda e: self.in_left.set(False))
        self.left.bind("<MouseWheel>",  self.on_tree_scroll)

        self.left.pack(side='left',
                       fill=tkinter.BOTH,
                       expand=True,
                       padx=(5, 0),
                       pady=pady)
        self.tree_scroll.pack(side='right', fill=tkinter.Y,
                              pady=pady, padx=(0, 5))

        frame_right = ttk.Frame(paned, relief="groove")
        self.frame_right = frame_right

        self.conf_canvas = tkinter.Canvas(frame_right, highlightthickness=0)
        self.page_scroll = ttk.Scrollbar(frame_right,
                                         orient="vertical",
                                         command=self.conf_canvas.yview)
        self.right_grid = ttk.Frame(self.conf_canvas)
        self.conf_canvas.configure(yscrollcommand=self.page_scroll.set)
        self.conf_canvas.pack(side='left',
                              fill=tkinter.BOTH,
                              expand=True,
                              pady=pady,
                              padx=(5, 0))
        self.page_scroll.pack(side='right', fill=tkinter.Y,
                              pady=pady, padx=(0, 5))
        self.conf_canvas.create_window(0, 0, window=self.right_grid,
                                       anchor='nw')
        self.conf_canvas.bind('<Enter>', lambda e: self.in_right.set(True))
        self.conf_canvas.bind('<Leave>', lambda e: self.in_right.set(False))
        self.conf_canvas.bind("<Configure>", self.on_canvas_configure)
        self.conf_canvas.bind_all("<MouseWheel>", self.on_page_scroll)

        paned.add(frame_left, weight=2)
        paned.add(frame_right, weight=10)

        style = ttk.Style()
        style.layout("Treeview", [('Treeview.treearea', {'sticky': 'nswe'})])

        menubar = tkinter.Menu(root)
        file_menu = tkinter.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Open Config YAML file",
                              command=self.load_from_yaml)
        file_menu.add_command(label=self.menu_string[6],
                              command=self.load_from_bsf_file)
        file_menu.add_command(label=self.menu_string[2],
                              command=self.load_from_fd)
        file_menu.add_command(label=self.menu_string[0],
                              command=self.save_to_bin,
                              state='disabled')
        file_menu.add_command(label=self.menu_string[1],
                              command=self.load_from_bin,
                              state='disabled')
        file_menu.add_command(label=self.menu_string[3],
                              command=self.load_from_delta,
                              state='disabled')
        file_menu.add_command(label=self.menu_string[4],
                              command=self.save_to_delta,
                              state='disabled')
        file_menu.add_command(label=self.menu_string[5],
                              command=self.save_full_to_delta,
                              state='disabled')
        file_menu.add_command(label="About", command=self.about)
        menubar.add_cascade(label="File", menu=file_menu)
        self.file_menu = file_menu

        root.config(menu=menubar)

        if len(sys.argv) > 1:
            path = sys.argv[1]
            if not path.endswith('.yaml') and not path.endswith('.pkl'):
                messagebox.showerror('LOADING ERROR',
                                     "Unsupported file '%s' !" % path)
                return
            else:
                self.load_cfg_file(path)

        if len(sys.argv) > 2:
            path = sys.argv[2]
            if path.endswith('.dlt'):
                self.load_delta_file(path)
            elif path.endswith('.bin'):
                self.load_bin_file(path)
            else:
                messagebox.showerror('LOADING ERROR',
                                     "Unsupported file '%s' !" % path)
                return

    def search_bar(self):
        # get data from text box
        self.search_text = self.edit.get()
        # Clear the page and update it according to search value
        self.refresh_config_data_page()

    def set_object_name(self, widget, name):
        self.conf_list[id(widget)] = name

    def get_object_name(self, widget):
        if id(widget) in self.conf_list:
            return self.conf_list[id(widget)]
        else:
            return None

    def limit_entry_size(self, variable, limit):
        value = variable.get()
        if len(value) > limit:
            variable.set(value[:limit])

    def on_canvas_configure(self, event):
        self.right_grid.grid_columnconfigure(0, minsize=event.width)

    def on_tree_scroll(self, event):
        if not self.in_left.get() and self.in_right.get():
            # This prevents scroll event from being handled by both left and
            # right frame at the same time.
            self.on_page_scroll(event)
            return 'break'

    def on_page_scroll(self, event):
        if self.in_right.get():
            # Only scroll when it is in active area
            min, max = self.page_scroll.get()
            if not((min == 0.0) and (max == 1.0)):
                self.conf_canvas.yview_scroll(-1 * int(event.delta / 120),
                                              'units')

    def update_visibility_for_widget(self, widget, args):
        visible = True
        item = self.get_config_data_item_from_widget(widget, True)
        if item is None:
            return visible
        elif not item:
            return visible
        if self.cfg_data_obj.binseg_dict:
            str_split = item['path'].split('.')
            if str_split[-2] not in CGenYamlCfg.available_fv and \
                    str_split[-2] not in CGenYamlCfg.missing_fv:
                if self.cfg_data_obj.binseg_dict[str_split[-3]] == -1:
                    visible = False
                    widget.grid_remove()
                    return visible
            else:
                if self.cfg_data_obj.binseg_dict[str_split[-2]] == -1:
                    visible = False
                    widget.grid_remove()
                    return visible
        result = 1
        if item['condition']:
            result = self.evaluate_condition(item)
            if result == 2:
                # Gray
                widget.configure(state='disabled')
            elif result == 0:
                # Hide
                visible = False
                widget.grid_remove()
            else:
                # Show
                widget.grid()
                widget.configure(state='normal')

        if visible and self.search_text != '':
            name = item['name']
            if name.lower().find(self.search_text.lower()) == -1:
                visible = False
                widget.grid_remove()

        return visible

    def update_widgets_visibility_on_page(self):
        self.walk_widgets_in_layout(self.right_grid,
                                    self.update_visibility_for_widget)

    def combo_select_changed(self, event):
        self.update_config_data_from_widget(event.widget, None)
        self.update_widgets_visibility_on_page()

    def edit_num_finished(self, event):
        widget = event.widget
        item = self.get_config_data_item_from_widget(widget)
        if not item:
            return
        parts = item['type'].split(',')
        if len(parts) > 3:
            min = parts[2].lstrip()[1:]
            max = parts[3].rstrip()[:-1]
            min_val = array_str_to_value(min)
            max_val = array_str_to_value(max)
            text = widget.get()
            if ',' in text:
                text = '{ %s }' % text
            try:
                value = array_str_to_value(text)
                if value < min_val or value > max_val:
                    raise Exception('Invalid input!')
                self.set_config_item_value(item, text)
            except Exception:
                pass

            text = item['value'].strip('{').strip('}').strip()
            widget.delete(0, tkinter.END)
            widget.insert(0, text)

        self.update_widgets_visibility_on_page()

    def update_page_scroll_bar(self):
        # Update scrollbar
        self.frame_right.update()
        self.conf_canvas.config(scrollregion=self.conf_canvas.bbox("all"))

    def on_config_page_select_change(self, event):
        self.update_config_data_on_page()
        sel = self.left.selection()
        if len(sel) > 0:
            page_id = sel[0]
            self.build_config_data_page(page_id)
            self.update_widgets_visibility_on_page()
            self.update_page_scroll_bar()

    def walk_widgets_in_layout(self, parent, callback_function, args=None):
        for widget in parent.winfo_children():
            callback_function(widget, args)

    def clear_widgets_inLayout(self, parent=None):
        if parent is None:
            parent = self.right_grid

        for widget in parent.winfo_children():
            widget.destroy()

        parent.grid_forget()
        self.conf_list.clear()

    def build_config_page_tree(self, cfg_page, parent):
        for page in cfg_page['child']:
            page_id = next(iter(page))
            # Put CFG items into related page list
            self.page_list[page_id] = self.cfg_data_obj.get_cfg_list(page_id)
            self.page_list[page_id].sort(key=lambda x: x['order'])
            page_name = self.cfg_data_obj.get_page_title(page_id)
            child = self.left.insert(
                parent, 'end',
                iid=page_id, text=page_name,
                value=0)
            if len(page[page_id]) > 0:
                self.build_config_page_tree(page[page_id], child)

    def is_config_data_loaded(self):
        return True if len(self.page_list) else False

    def set_current_config_page(self, page_id):
        self.page_id = page_id

    def get_current_config_page(self):
        return self.page_id

    def get_current_config_data(self):
        page_id = self.get_current_config_page()
        if page_id in self.page_list:
            return self.page_list[page_id]
        else:
            return []

    invalid_values = {}

    def build_config_data_page(self, page_id):
        self.clear_widgets_inLayout()
        self.set_current_config_page(page_id)
        disp_list = []
        for item in self.get_current_config_data():
            disp_list.append(item)
        row = 0
        disp_list.sort(key=lambda x: x['order'])
        for item in disp_list:
            self.add_config_item(item, row)
            row += 2
        if self.invalid_values:
            string = 'The following contails invalid options/values \n\n'
            for i in self.invalid_values:
                string += i + ": " + str(self.invalid_values[i]) + "\n"
            reply = messagebox.showwarning('Warning!', string)
            if reply == 'ok':
                self.invalid_values.clear()

    fsp_version = ''

    def load_config_data(self, file_name):
        gen_cfg_data = CGenYamlCfg()
        if file_name.endswith('.pkl'):
            with open(file_name, "rb") as pkl_file:
                gen_cfg_data.__dict__ = marshal.load(pkl_file)
            gen_cfg_data.prepare_marshal(False)
        elif file_name.endswith('.yaml'):
            if gen_cfg_data.load_yaml(file_name) != 0:
                raise Exception(gen_cfg_data.get_last_error())
        else:
            raise Exception('Unsupported file "%s" !' % file_name)
        # checking fsp version
        if gen_cfg_data.detect_fsp():
            self.fsp_version = '2.X'
        else:
            self.fsp_version = '1.X'

        return gen_cfg_data

    def about(self):
        msg = 'Configuration Editor\n--------------------------------\n \
               Version 0.8\n2021'
        lines = msg.split('\n')
        width = 30
        text = []
        for line in lines:
            text.append(line.center(width, ' '))
        messagebox.showinfo('Config Editor', '\n'.join(text))

    def update_last_dir(self, path):
        self.last_dir = os.path.dirname(path)

    def get_open_file_name(self, ftype):
        if self.is_config_data_loaded():
            if ftype == 'dlt':
                question = ''
            elif ftype == 'bin':
                question = 'All configuration will be reloaded from BIN file, \
                            continue ?'
            elif ftype == 'yaml':
                question = ''
            elif ftype == 'bsf':
                question = ''
            else:
                raise Exception('Unsupported file type !')
            if question:
                reply = messagebox.askquestion('', question, icon='warning')
                if reply == 'no':
                    return None

        if ftype == 'yaml':
            if self.mode == 'FSP':
                file_type = 'YAML'
                file_ext = 'yaml'
            else:
                file_type = 'YAML or PKL'
                file_ext = 'pkl *.yaml'
        else:
            file_type = ftype.upper()
            file_ext = ftype

        path = filedialog.askopenfilename(
                initialdir=self.last_dir,
                title="Load file",
                filetypes=(("%s files" % file_type, "*.%s" % file_ext), (
                            "all files", "*.*")))
        if path:
            self.update_last_dir(path)
            return path
        else:
            return None

    def load_from_delta(self):
        path = self.get_open_file_name('dlt')
        if not path:
            return
        self.load_delta_file(path)

    def load_delta_file(self, path):
        self.reload_config_data_from_bin(self.org_cfg_data_bin)
        try:
            self.cfg_data_obj.override_default_value(path)
        except Exception as e:
            messagebox.showerror('LOADING ERROR', str(e))
            return
        self.update_last_dir(path)
        self.refresh_config_data_page()

    def load_from_bin(self):
        path = filedialog.askopenfilename(
            initialdir=self.last_dir,
            title="Load file",
            filetypes={("Binaries", "*.fv *.fd *.bin *.rom")})
        if not path:
            return
        self.load_bin_file(path)

    def load_bin_file(self, path):
        with open(path, 'rb') as fd:
            bin_data = bytearray(fd.read())
        if len(bin_data) < len(self.org_cfg_data_bin):
            messagebox.showerror('Binary file size is smaller than what \
                                  YAML requires !')
            return

        try:
            self.reload_config_data_from_bin(bin_data)
        except Exception as e:
            messagebox.showerror('LOADING ERROR', str(e))
            return

    def load_from_bsf_file(self):
        path = self.get_open_file_name('bsf')
        if not path:
            return
        self.load_bsf_file(path)

    def load_bsf_file(self, path):
        bsf_file = path
        dsc_file = os.path.splitext(bsf_file)[0] + '.dsc'
        yaml_file = os.path.splitext(bsf_file)[0] + '.yaml'
        bsf_to_dsc(bsf_file, dsc_file)
        dsc_to_yaml(dsc_file, yaml_file)

        self.load_cfg_file(yaml_file)
        return

    def load_from_fd(self):
        path = filedialog.askopenfilename(
            initialdir=self.last_dir,
            title="Load file",
            filetypes={("Binaries", "*.fv *.fd *.bin *.rom")})
        if not path:
            return
        self.load_fd_file(path)

    def load_fd_file(self, path):
        with open(path, 'rb') as fd:
            bin_data = bytearray(fd.read())

        fd = FirmwareDevice(0, bin_data)
        fd.ParseFd()
        fd.ParseFsp()
        fd.OutputFsp()

    def load_cfg_file(self, path):
        # Save current values in widget and clear  database
        self.clear_widgets_inLayout()
        self.left.delete(*self.left.get_children())

        self.cfg_data_obj = self.load_config_data(path)

        self.update_last_dir(path)
        self.org_cfg_data_bin = self.cfg_data_obj.generate_binary_array()
        self.build_config_page_tree(self.cfg_data_obj.get_cfg_page()['root'],
                                    '')

        msg_string = 'Click YES if it is FULL FSP '\
            + self.fsp_version + ' Binary'
        reply = messagebox.askquestion('Form', msg_string)
        if reply == 'yes':
            self.load_from_bin()

        for menu in self.menu_string:
            self.file_menu.entryconfig(menu, state="normal")

        return 0

    def load_from_yaml(self):
        path = self.get_open_file_name('yaml')
        if not path:
            return

        self.load_cfg_file(path)

    def get_save_file_name(self, extension):
        path = filedialog.asksaveasfilename(
                  initialdir=self.last_dir,
                  title="Save file",
                  defaultextension=extension)
        if path:
            self.last_dir = os.path.dirname(path)
            return path
        else:
            return None

    def save_delta_file(self, full=False):
        path = self.get_save_file_name(".dlt")
        if not path:
            return

        self.update_config_data_on_page()
        new_data = self.cfg_data_obj.generate_binary_array()
        self.cfg_data_obj.generate_delta_file_from_bin(path,
                                                       self.org_cfg_data_bin,
                                                       new_data, full)

    def save_to_delta(self):
        self.save_delta_file()

    def save_full_to_delta(self):
        self.save_delta_file(True)

    def save_to_bin(self):
        path = self.get_save_file_name(".bin")
        if not path:
            return

        self.update_config_data_on_page()
        bins = self.cfg_data_obj.save_current_to_bin()

        with open(path, 'wb') as fd:
            fd.write(bins)

    def refresh_config_data_page(self):
        self.clear_widgets_inLayout()
        self.on_config_page_select_change(None)

    def set_config_data_page(self):
        page_id_list = []
        for idx, page in enumerate(
                self.cfg_data_obj._cfg_page['root']['child']):
            page_id_list.append(list(page.keys())[0])
            page_list = self.cfg_data_obj.get_cfg_list(page_id_list[idx])
            self.cfg_page_dict[page_id_list[idx]] = 0
            for item in page_list:
                str_split = item['path'].split('.')
                if str_split[-2] not in CGenYamlCfg.available_fv and \
                        str_split[-2] not in CGenYamlCfg.missing_fv:
                    if self.cfg_data_obj.binseg_dict[str_split[-3]] != -1:
                        self.cfg_page_dict[page_id_list[idx]] += 1
                else:
                    if self.cfg_data_obj.binseg_dict[str_split[-2]] != -1:
                        self.cfg_page_dict[page_id_list[idx]] += 1
        removed_page = 0
        for idx, id in enumerate(page_id_list):
            if self.cfg_page_dict[id] == 0:
                del self.cfg_data_obj._cfg_page['root']['child'][idx-removed_page]  # noqa: E501
                removed_page += 1

    def reload_config_data_from_bin(self, bin_dat):
        self.cfg_data_obj.load_default_from_bin(bin_dat)
        self.set_config_data_page()
        self.left.delete(*self.left.get_children())
        self.build_config_page_tree(self.cfg_data_obj.get_cfg_page()['root'],
                                    '')
        self.refresh_config_data_page()

    def set_config_item_value(self, item, value_str):
        itype = item['type'].split(',')[0]
        if itype == "Table":
            new_value = value_str
        elif itype == "EditText":
            length = (self.cfg_data_obj.get_cfg_item_length(item) + 7) // 8
            new_value = value_str[:length]
            if item['value'].startswith("'"):
                new_value = "'%s'" % new_value
        else:
            try:
                new_value = self.cfg_data_obj.reformat_value_str(
                            value_str,
                            self.cfg_data_obj.get_cfg_item_length(item),
                            item['value'])
            except Exception:
                print("WARNING: Failed to format value string '%s' for '%s' !"
                      % (value_str, item['path']))
                new_value = item['value']

        if item['value'] != new_value:
            if self.debug:
                print('Update %s from %s to %s !'
                      % (item['cname'], item['value'], new_value))
            item['value'] = new_value

    def get_config_data_item_from_widget(self, widget, label=False):
        name = self.get_object_name(widget)
        if not name or not len(self.page_list):
            return None

        if name.startswith('LABEL_'):
            if label:
                path = name[6:]
            else:
                return None
        else:
            path = name

        item = self.cfg_data_obj.get_item_by_path(path)
        return item

    def update_config_data_from_widget(self, widget, args):
        item = self.get_config_data_item_from_widget(widget)
        if item is None:
            return
        elif not item:
            if isinstance(widget, tkinter.Label):
                return
            raise Exception('Failed to find "%s" !' %
                            self.get_object_name(widget))

        itype = item['type'].split(',')[0]
        if itype == "Combo":
            opt_list = self.cfg_data_obj.get_cfg_item_options(item)
            tmp_list = [opt[0] for opt in opt_list]
            idx = widget.current()
            if idx != -1:
                self.set_config_item_value(item, tmp_list[idx])
        elif itype in ["EditNum", "EditText"]:
            self.set_config_item_value(item, widget.get())
        elif itype in ["Table"]:
            new_value = bytes_to_bracket_str(widget.get())
            self.set_config_item_value(item, new_value)

    def evaluate_condition(self, item):
        try:
            result = self.cfg_data_obj.evaluate_condition(item)
        except Exception:
            print("WARNING: Condition '%s' is invalid for '%s' !"
                  % (item['condition'], item['path']))
            result = 1
        return result

    def add_config_item(self, item, row):
        parent = self.right_grid

        name = tkinter.Label(parent, text=item['name'], anchor="w")

        parts = item['type'].split(',')
        itype = parts[0]
        widget = None

        if itype == "Combo":
            # Build
            opt_list = self.cfg_data_obj.get_cfg_item_options(item)
            current_value = self.cfg_data_obj.get_cfg_item_value(item, False)
            option_list = []
            current = None

            for idx, option in enumerate(opt_list):
                option_str = option[0]
                try:
                    option_value = self.cfg_data_obj.get_value(
                                   option_str,
                                   len(option_str), False)
                except Exception:
                    option_value = 0
                    print('WARNING: Option "%s" has invalid format for "%s" !'
                          % (option_str, item['path']))
                if option_value == current_value:
                    current = idx
                option_list.append(option[1])

            widget = ttk.Combobox(parent, value=option_list, state="readonly")
            widget.bind("<<ComboboxSelected>>", self.combo_select_changed)
            widget.unbind_class("TCombobox", "<MouseWheel>")

            if current is None:
                print('WARNING: Value "%s" is an invalid option for "%s" !' %
                      (current_value, item['path']))
                self.invalid_values[item['path']] = current_value
            else:
                widget.current(current)

        elif itype in ["EditNum", "EditText"]:
            txt_val = tkinter.StringVar()
            widget = tkinter.Entry(parent, textvariable=txt_val)
            value = item['value'].strip("'")
            if itype in ["EditText"]:
                txt_val.trace(
                    'w',
                    lambda *args: self.limit_entry_size
                    (txt_val, (self.cfg_data_obj.get_cfg_item_length(item)
                               + 7) // 8))
            elif itype in ["EditNum"]:
                value = item['value'].strip("{").strip("}").strip()
                widget.bind("<FocusOut>", self.edit_num_finished)
            txt_val.set(value)

        elif itype in ["Table"]:
            bins = self.cfg_data_obj.get_cfg_item_value(item, True)
            col_hdr = item['option'].split(',')
            widget = custom_table(parent, col_hdr, bins)

        else:
            if itype and itype not in ["Reserved"]:
                print("WARNING: Type '%s' is invalid for '%s' !" %
                      (itype, item['path']))
                self.invalid_values[item['path']] = itype

        if widget:
            create_tool_tip(widget, item['help'])
            self.set_object_name(name, 'LABEL_' + item['path'])
            self.set_object_name(widget, item['path'])
            name.grid(row=row, column=0, padx=10, pady=5, sticky="nsew")
            widget.grid(row=row + 1, rowspan=1, column=0,
                        padx=10, pady=5, sticky="nsew")

    def update_config_data_on_page(self):
        self.walk_widgets_in_layout(self.right_grid,
                                    self.update_config_data_from_widget)


if __name__ == '__main__':
    root = tkinter.Tk()
    app = application(master=root)
    root.title("Config Editor")
    root.mainloop()
