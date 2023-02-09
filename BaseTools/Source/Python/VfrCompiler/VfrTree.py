import yaml
import CppHeaderParser
from VfrFormPkg import *
from antlr4 import *
from VfrCtypes import *
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from Common.LongFilePathSupport import LongFilePath
# Ifr related Info -> ctypes obj
#　conditional Info
# Structure Info

VFR_COMPILER_VERSION = "2.01 (UEFI 2.4)"
BUILD_VERSION = 'Developer Build based on Revision: Unknown'


class VfrTreeNode():

    def __init__(self, OpCode=None, Data=None, Buffer=None) -> None:

        self.OpCode = OpCode
        self.Data = Data
        self.Buffer = Buffer
        self.Condition = None
        self.Expression = None
        self.Offset = None
        self.Parent = None
        self.Child = []
        self.Level = -1

    def hasCondition(self) -> bool:
        if self.Condition == None:
            return False
        else:
            return True

    def hasChild(self) -> bool:
        if self.Child == []:
            return False
        else:
            return True

    def isFinalChild(self) -> bool:
        ParTree = self.Parent
        if ParTree:
            if ParTree.Child[-1] == self:
                return True
        return False

    def insertChild(self, NewNode, pos: int = None) -> None:
        if NewNode != None:
            if not pos:
                self.Child.append(NewNode)
            else:
                self.Child.insert(pos, NewNode)

            NewNode.Parent = self

    # lastNode.insertRel(newNode)
    def insertRel(self, newNode) -> None:
        if self.Parent:
            parentTree = self.Parent
            new_index = parentTree.Child.index(self) + 1
            parentTree.Child.insert(new_index, newNode)
        self.NextRel = newNode
        newNode.LastRel = self

    def deleteNode(self, deletekey: str) -> None:
        FindStatus, DeleteTree = self.FindNode(deletekey)
        if FindStatus:
            parentTree = DeleteTree.Parent
            lastTree = DeleteTree.LastRel
            nextTree = DeleteTree.NextRel
            if parentTree:
                index = parentTree.Child.index(DeleteTree)
                del parentTree.Child[index]
            if lastTree and nextTree:
                lastTree.NextRel = nextTree
                nextTree.LastRel = lastTree
            elif lastTree:
                lastTree.NextRel = None
            elif nextTree:
                nextTree.LastRel = None
            return DeleteTree
        else:
            print('Could not find the target tree')
            return None


ExpOps = [
    EFI_IFR_DUP_OP, EFI_IFR_EQ_ID_VAL_OP, EFI_IFR_QUESTION_REF1_OP,
    EFI_IFR_EQ_ID_VAL_OP, EFI_IFR_EQ_ID_ID_OP, EFI_IFR_EQ_ID_VAL_LIST_OP,
    EFI_IFR_RULE_REF_OP, EFI_IFR_STRING_REF1_OP, EFI_IFR_THIS_OP,
    EFI_IFR_SECURITY_OP, EFI_IFR_GET_OP, EFI_IFR_TRUE_OP, EFI_IFR_FALSE_OP,
    EFI_IFR_ONE_OP, EFI_IFR_ONES_OP, EFI_IFR_ZERO_OP, EFI_IFR_UNDEFINED_OP,
    EFI_IFR_VERSION_OP, EFI_IFR_UINT64_OP, EFI_IFR_QUESTION_REF2_OP,
    EFI_IFR_QUESTION_REF3_OP, EFI_IFR_SET_OP, EFI_IFR_DEFAULTSTORE_OP,
    EFI_IFR_OR_OP
]

class Options():

    def __init__(self):
        self.VfrFileName = None
        self.VfrBaseFileName = None
        self.IncludePaths = None
        self.OutputDirectory = None
        self.CreateRecordListFile = True
        self.RecordListFileName = None
        self.CreateIfrPkgFile = True
        self.PkgOutputFileName = None
        self.COutputFileName = None
        self.CreateYamlFile = True
        self.YamlFileName = None
        self.CreateJsonFile = True
        self.JsonFileName = None
        self.SkipCPreprocessor = False
        self.CPreprocessorOptions = None
        self.PreprocessorOutputFileName = None
        self.HasOverrideClassGuid = False
        self.OverrideClassGuid = None
        self.WarningAsError = False
        self.AutoDefault = False
        self.CheckDefault = False

        self.ProcessedInFileName = None
        self.UniStrDefFileName = None
        self.ExpandedHeaderFileName = None # save header info for yaml
        self.ProcessedYAMLFileName = None
        self.CompileYaml = True


class VfrTree():

    def __init__(self, Root: VfrTreeNode, Options: Options) -> None:
        self.Root = Root
        self.Options = Options

    def GenBinaryFiles(self):
        Hpk = None
        C = None
        RecordLines = []
        if self.Options.CreateIfrPkgFile:
            # GenBinary
            PkgHdr = gFormPkg.BuildPkgHdr()
            try:
                HpkFile = self.Options.PkgOutputFileName
                Hpk = open(HpkFile, 'wb')
                Hpk.write(gFormPkg.StructToStream(PkgHdr))
            except:
                EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                                "File open failed for %s" % HpkFile)

            #GenCFile
            try:
                CFile = self.Options.COutputFileName
                C = open(CFile, 'w')
                C.write('//\n')
                C.write('//' + ' ' + 'DO NOT EDIT -- auto-generated file\n')
                C.write('//\n')
                C.write('//' + ' ' +
                        'This file is generated by the vfrcompiler utility\n')

                BaseName = 'unsigned char ' + self.Options.VfrBaseFileName + 'Bin[] = {\n'
                C.write(BaseName)
                C.write('  //' + ' ' + 'ARRAY LENGTH\n')
                PkgLength = PkgHdr.Length + sizeof(ctypes.c_uint32)
                for B in PkgLength.to_bytes(4, byteorder='little', signed=True):
                    C.write('  0x%02x,' % B)
                C.write('\n')
                C.write('  //' + ' ' + 'PACKAGE HEADER\n')
                HeaderBuffer = gFormPkg.StructToStream(PkgHdr)
                for B in HeaderBuffer:
                    C.write('  0x%02x,' % B)
                C.write('\n')
                C.write('  //' + ' ' + 'PACKAGE DATA\n')
                self.Index = 0
            except:
                EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                                "File open failed for %s" % CFile)

        if self.Options.CreateRecordListFile:
            # GenRecordList
            try:
                LstFile = self.Options.RecordListFileName
                Lst = open(LstFile, 'w')
                Lst.write('//\n//  VFR compiler version {} {}\n//\n'.format(
                    VFR_COMPILER_VERSION, BUILD_VERSION))
            except:
                EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                                "File open failed for %s" % LstFile)

        self._GenBinaryFilesDfs(self.Root, Hpk, C, RecordLines)

        if self.Options.CreateIfrPkgFile:
            #GenCFile
            try:
                C.write('\n};\n')
            except:
                EdkLogger.error("VfrCompiler", FILE_WRITE_FAILURE,
                                "File write failed for %s" % CFile, None)

        if self.Options.CreateRecordListFile:
            # GenRecordList
            try:
                if self.Options.SkipCPreprocessor:
                    In = open(self.Options.VfrFileName, 'r') #
                else:
                    In = open(self.Options.PreprocessorOutputFileName, 'r')
                InFileLines = []
                for Line in In:
                    InFileLines.append(Line)
                Hpk.close()
                C.close()
            except:
                EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                                "File open failed for %s" % self.Options.PreprocessorOutputFileName, None)
            try:
                InsertedLine = 0
                for RecordLine in RecordLines:
                    InFileLines.insert(RecordLine.LineNo + InsertedLine,
                                    RecordLine.Record)
                    InsertedLine += 1
                for Line in InFileLines:
                    Lst.write('{}'.format(Line))

                Lst.write('//\n//  All Opcode Record List\n//\n')
                for RecordLine in RecordLines:
                    Lst.write('{}'.format(RecordLine.Record))
                Lst.write('\nTotal Size of all record is' +
                        ' {:0>8x}'.format(gFormPkg.Offset))
                gVfrVarDataTypeDB.Dump(Lst)
                In.close()
                Lst.close()
            except:
                EdkLogger.error("VfrCompiler", FILE_WRITE_FAILURE,
                                "File write failed for %s" % LstFile, None)

    def _GenBinaryFilesDfs(self, Root, Hpk, C, RecordLines):
        if Root.OpCode != None:

            if Root.OpCode in ExpOps:
                # The Data is likely to be modified, so generate buffer here
                Root.Buffer = gFormPkg.StructToStream(Root.Data.GetInfo())
            if Root.Buffer != None:

                if self.Options.CreateIfrPkgFile and self.Options.CreateRecordListFile:
                    try:
                        Hpk.write(Root.Buffer)
                    except:
                        EdkLogger.error(
                            "VfrCompiler", FILE_WRITE_FAILURE,
                            "File write failed for %s" %
                            (self.Options.PkgOutputFileName))

                    try:
                        LineBuffer = ''
                        for i in range(0, len(Root.Buffer)):
                            self.Index += 1
                            Data = Root.Buffer[i]
                            if self.Index == gFormPkg.PkgLength:
                                C.write('0x%02x' % Data)
                            elif self.Index % BYTES_PRE_LINE == 1:
                                C.write('  0x%02x,  ' % Data)
                            elif self.Index % BYTES_PRE_LINE == 0:
                                C.write('0x%02x,\n' % Data)
                            else:
                                C.write('0x%02x,  ' % Data)

                            LineBuffer += '{:0>2X} '.format(Root.Buffer[i])

                        Record = '>{:0>8x}: '.format(
                            Root.Offset) + LineBuffer + '\n'
                        LineNo = Root.Data.GetLineNo()
                        RecordLines.append(ReCordNode(Record, LineNo))
                    except:
                        EdkLogger.error(
                            "VfrCompiler", FILE_WRITE_FAILURE,
                            "File write failed for %s" %
                            (self.Options.COutputFileName))

                if self.Options.CreateIfrPkgFile and not self.Options.CreateRecordListFile:

                    try:
                        Hpk.write(Root.Buffer)
                    except:
                        EdkLogger.error(
                            "VfrCompiler", FILE_WRITE_FAILURE,
                            "File write failed for %s" %
                            (self.Options.PkgOutputFileName))

                    try:
                        for i in range(0, len(Root.Buffer)):
                            self.Index += 1
                            Data = Root.Buffer[i]
                            if self.Index == gFormPkg.PkgLength:
                                C.write('0x%02x' % Data)
                            elif self.Index % BYTES_PRE_LINE == 1:
                                C.write('  0x%02x,  ' % Data)
                            elif self.Index % BYTES_PRE_LINE == 0:
                                C.write('0x%02x,\n' % Data)
                            else:
                                C.write('0x%02x,  ' % Data)
                    except:
                        EdkLogger.error(
                            "VfrCompiler", FILE_WRITE_FAILURE,
                            "File write failed for %s" %
                            (self.Options.COutputFileName))

                if not self.Options.CreateIfrPkgFile and self.Options.CreateRecordListFile:

                    LineBuffer = ''
                    for i in range(0, len(Root.Buffer)):
                        LineBuffer += '{:0>2X} '.format(Root.Buffer[i])

                    Record = '>{:0>8x}: '.format(
                    Root.Offset) + LineBuffer + '\n'
                    LineNo = Root.Data.GetLineNo()
                    RecordLines.append(ReCordNode(Record, LineNo))

        if Root.Child != []:
            for ChildNode in Root.Child:
                self._GenBinaryFilesDfs(ChildNode, Hpk, C, RecordLines)

    def GenBinary(self):
        FileName = self.Options.PkgOutputFileName

        try:
            with open(FileName, 'wb') as f:
                PkgHdr = gFormPkg.BuildPkgHdr()
                f.write(gFormPkg.StructToStream(PkgHdr))
                self._GenBinaryDfs(self.Root, f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

    def _GenBinaryDfs(self, Root, f):
        if Root.OpCode != None:
            if Root.OpCode in ExpOps:
                # The Data is likely to be modified, so generate buffer here
                Root.Buffer = gFormPkg.StructToStream(Root.Data.GetInfo())

            if Root.Buffer != None:
                try:
                    f.write(Root.Buffer)
                except:
                    EdkLogger.error(
                        "VfrCompiler", FILE_WRITE_FAILURE,
                        "File write failed for %s" %
                        (self.Options.PkgOutputFileName), None)

        if Root.Child != []:
            for ChildNode in Root.Child:
                self._GenBinaryDfs(ChildNode, f)

    def GenCFile(self):
        FileName = self.Options.COutputFileName
        try:
            with open(FileName, 'w') as f:
                PkgHdr = gFormPkg.BuildPkgHdr()
                f.write('//\n')
                f.write('//' + ' ' + 'DO NOT EDIT -- auto-generated file\n')
                f.write('//\n')
                f.write('//' + ' ' +
                        'This file is generated by the vfrcompiler utility\n')

                BaseName = 'unsigned char ' + self.Options.VfrBaseFileName + 'Bin[] = {\n'
                f.write(BaseName)
                f.write('  //' + ' ' + 'ARRAY LENGTH\n')
                PkgLength = PkgHdr.Length + sizeof(ctypes.c_uint32)
                for B in PkgLength.to_bytes(4, byteorder='little',
                                            signed=True):
                    f.write('  0x%02x,' % B)
                f.write('\n')
                f.write('  //' + ' ' + 'PACKAGE HEADER\n')
                HeaderBuffer = gFormPkg.StructToStream(PkgHdr)
                for B in HeaderBuffer:
                    f.write('  0x%02x,' % B)
                f.write('\n')
                f.write('  //' + ' ' + 'PACKAGE DATA\n')
                self.Index = 0
                self._GenCFileDfs(self.Root, gFormPkg.PkgLength, f)
                f.write('\n};\n')
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

    def _GenCFileDfs(self, Root, Length, f):
        if Root.OpCode != None:
            if Root.Buffer != None:
                try:
                    for i in range(0, len(Root.Buffer)):

                        self.Index += 1
                        Data = Root.Buffer[i]
                        if self.Index == Length:
                            f.write('0x%02X' % Data)
                        elif self.Index % BYTES_PRE_LINE == 1:
                            f.write('  0x%02X,  ' % Data)
                        elif self.Index % BYTES_PRE_LINE == 0:
                            f.write('0x%02X,\n' % Data)
                        else:
                            f.write('0x%02X,  ' % Data)
                except:
                    EdkLogger.error(
                        "VfrCompiler", FILE_WRITE_FAILURE,
                        "File write failed for %s" %
                        (self.Options.COutputFileName), None)

        if Root.Child != []:
            for ChildNode in Root.Child:
                self._GenCFileDfs(ChildNode, Length, f)

    def GenRecordListFile(self):
        FileName = self.Options.RecordListFileName
        try:
            In = open(self.Options.PreprocessorOutputFileName, 'r')
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % self.Options.PreprocessorOutputFileName, None)
        InFileLines = []
        for Line in In:
            InFileLines.append(Line)

        RecordLines = []
        self._GenRecordListFileDfs(self.Root, RecordLines)

        InsertedLine = 0
        for RecordLine in RecordLines:
            InFileLines.insert(RecordLine.LineNo + InsertedLine,
                               RecordLine.Record)
            InsertedLine += 1

        try:
            Out = open(FileName, 'w')
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

        try:
            Out.write('//\n//  VFR compiler version {} {}\n//\n'.format(
                VFR_COMPILER_VERSION, BUILD_VERSION))
            for Line in InFileLines:
                Out.write('{}'.format(Line))

            Out.write('//\n//  All Opcode Record List\n//\n')
            for RecordLine in RecordLines:
                Out.write('{}'.format(RecordLine.Record))
            Out.write('\nTotal Size of all record is' +
                      ' {:0>8x}'.format(gFormPkg.Offset))
            gVfrVarDataTypeDB.Dump(Out)
            In.close()
            Out.close()
        except:
            EdkLogger.error(
                "VfrCompiler", FILE_WRITE_FAILURE, "File write failed for %s" % FileName)

    def _GenRecordListFileDfs(self, Root, RecordLines):
        if Root.OpCode != None:
            #f.write('{}\n'.format(type(Root.Data)))
            LineBuffer = ''
            if Root.Buffer != None:
                for i in range(0, len(Root.Buffer)):
                    LineBuffer += '{:0>2X} '.format(Root.Buffer[i])

                Record = '>{:0>8x}: '.format(Root.Offset) + LineBuffer + '\n'
                LineNo = Root.Data.GetLineNo()
                RecordLines.append(ReCordNode(Record, LineNo))

        if Root.Child != []:
            for ChildNode in Root.Child:
                self._GenRecordListFileDfs(ChildNode, RecordLines)

    def DumpJson(self):
        FileName = self.Options.JsonFileName
        try:
            with open(FileName, 'w') as f:
                f.write('{\n')
                f.write('  \"DataStruct\" : {\n')
                pNode = gVfrVarDataTypeDB.GetDataTypeList()
                while pNode != None:
                    f.write('    \"{}\" : [\n'.format(str(pNode.TypeName)))
                    FNode = pNode.Members
                    while FNode != None:
                        f.write('{\n')
                        f.write('  \"Name\": \"{}\",\n'.format(
                            str(FNode.FieldName)))
                        if FNode.ArrayNum > 0:
                            f.write('  \"Type\": \"{}[{}]\",\n'.format(
                                str(FNode.FieldType.TypeName),
                                str(FNode.ArrayNum)))
                        else:
                            f.write('  \"Type\": \"{}\",\n'.format(
                                str(FNode.FieldType.TypeName)))
                        f.write('  \"Offset\": {}\n'.format(
                            str(FNode.Offset)))
                        if FNode.Next == None:
                            f.write('}\n')
                        else:
                            f.write('}, \n')
                        FNode = FNode.Next
                    if pNode.Next == None:
                        f.write('    ]\n')
                    else:
                        f.write('    ],\n')
                    pNode = pNode.Next
                f.write('  },\n')
                f.write('  \"DataStructAttribute\": {\n')
                pNode = gVfrVarDataTypeDB.GetDataTypeList()
                while pNode != None:
                    f.write('    \"{}\"'.format(str(pNode.TypeName)) + ': {\n')
                    f.write('  \"Alignment\": {},\n'.format(
                        str(pNode.Align)))
                    f.write('  \"TotalSize\": {}\n'.format(
                        str(pNode.TotalSize)))
                    if pNode.Next == None:
                        f.write('}\n')
                    else:
                        f.write('},\n')
                    pNode = pNode.Next
                f.write('  },\n')
                f.write('  \"VarDefine\" : {\n')
                pVsNode = gVfrDataStorage.GetBufferVarStoreList()
                while pVsNode != None:
                    f.write('    \"{}\"'.format(str(pVsNode.VarStoreName)) +
                            ': {\n')
                    f.write('  \"Type\": \"{}\",\n'.format(
                        str(pVsNode.DataType.TypeName)))
                    f.write('  \"Attributes\": {},\n'.format(
                        str(pVsNode.Attributes)))
                    f.write('  \"VarStoreId\": {},\n'.format(
                        str(pVsNode.VarStoreId)))
                    f.write('  \"VendorGuid\": ' + '\"{}, {}, {},'.format('0x%x'%(pVsNode.Guid.Data1),'0x%x'%(pVsNode.Guid.Data2), '0x%x'%(pVsNode.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(pVsNode.Guid.Data4[0]), '0x%x'%(pVsNode.Guid.Data4[1]), '0x%x'%(pVsNode.Guid.Data4[2]), '0x%x'%(pVsNode.Guid.Data4[3]), \
                    '0x%x'%(pVsNode.Guid.Data4[4]), '0x%x'%(pVsNode.Guid.Data4[5]), '0x%x'%(pVsNode.Guid.Data4[6]), '0x%x'%(pVsNode.Guid.Data4[7])) + ' }}\"\n')
                    if pVsNode.Next == None:
                        f.write('}\n')
                    else:
                        f.write('},\n')

                    pVsNode = pVsNode.Next
                f.write('  },\n')
                f.write('  \"Data\" : [\n')
                pVsNode = gVfrBufferConfig.GetVarItemList()
                while pVsNode != None:
                    if pVsNode.Id == None:
                        pVsNode = pVsNode.Next
                        continue
                    pInfoNode = pVsNode.InfoStrList
                    while pInfoNode != None:
                        f.write('{\n')
                        f.write('  \"VendorGuid\": ' + '\"{}, {}, {},'.format('0x%x'%(pVsNode.Guid.Data1),'0x%x'%(pVsNode.Guid.Data2), '0x%x'%(pVsNode.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(pVsNode.Guid.Data4[0]), '0x%x'%(pVsNode.Guid.Data4[1]), '0x%x'%(pVsNode.Guid.Data4[2]), '0x%x'%(pVsNode.Guid.Data4[3]), \
                        '0x%x'%(pVsNode.Guid.Data4[4]), '0x%x'%(pVsNode.Guid.Data4[5]), '0x%x'%(pVsNode.Guid.Data4[6]), '0x%x'%(pVsNode.Guid.Data4[7])) + ' }}\",\n')
                        f.write('  \"VarName\": \"{}\",\n'.format(
                            str(pVsNode.Name)))
                        f.write('  \"DefaultStore\": \"{}\",\n'.format(
                            str(pVsNode.Id)))
                        f.write('  \"Size\": \"{}\",\n'.format(
                            str(pInfoNode.Width)))
                        f.write('  \"Offset\": {},\n'.format(
                            str(pInfoNode.Offset)))
                        #f.write('  \"Value\": \"{}\"\n'.format(str(pInfoNode.Value)))
                        if pInfoNode.Type == EFI_IFR_TYPE_DATE:
                            f.write('  \"Value\": \"{}/{}/{}\"\n'.format(
                                pInfoNode.Value.Year, pInfoNode.Value.Month,
                                pInfoNode.Value.Day))
                        elif pInfoNode.Type == EFI_IFR_TYPE_TIME:
                            f.write('  \"Value\": \"{}:{}:{}\"\n'.format(
                                pInfoNode.Value.Hour, pInfoNode.Value.Minute,
                                pInfoNode.Value.Second))
                        elif pInfoNode.Type == EFI_IFR_TYPE_REF:
                            f.write('  \"Value\": \"{};{};'.format(pInfoNode.Value.QuestionId, pInfoNode.Value.FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(pInfoNode.Value.FormSetGuid.Data1),'0x%x'%(pInfoNode.Value.FormSetGuid.Data2), '0x%x'%(pInfoNode.Value.FormSetGuid.Data3)) \
                            + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(pInfoNode.Value.FormSetGuid.Data4[0]), '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[1]), '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[2]), '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[3]), \
                            '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[4]), '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[5]), '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[6]), '0x%x'%(pInfoNode.Value.FormSetGuid.Data4[7])) + ' }}' + ';{}\n'.format(pInfoNode.Value.DevicePath))
                        else:
                            f.write('  \"Value\": \"{}\"\n'.format(
                                pInfoNode.Value))

                        f.write('},\n')
                        pInfoNode = pInfoNode.Next
                    pVsNode = pVsNode.Next
                f.write('{\n')
                f.write('  \"VendorGuid\": \"NA\",\n')
                f.write('  \"VarName\": \"NA\",\n')
                f.write('  \"DefaultStore\": \"NA\",\n')
                f.write('  \"Size\": 0,\n')
                f.write('  \"Offset\": 0,\n')
                f.write('  \"Value\": \"0x00\"\n')
                f.write('}\n')
                f.write('  ]\n')
                f.write('}\n')

            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

    def DumpYaml(self):
        FileName = self.Options.YamlFileName
        try:
            with open(FileName, 'w') as f:
                f.write('## DO NOT REMOVE -- VFR Mode\n')
                f.write('include:\n')
                HeaderFiles = self._InsertHeaderToYaml(f)
                UniDict = self._GetUniDictsForYaml() # filepath
                VfrDict = self._GetVfrDictsForYaml()
                HeaderDict = self._GetHeaderDictsForYaml(HeaderFiles)
                self._DumpYamlDfsWithUni(self.Root, f, UniDict, HeaderDict, VfrDict)
                # self.DumpYamlDfs(self.Root, f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

    def _InsertHeaderToYaml(self, f): # can be refined
        FileName = self.Options.VfrFileName
        try:
            fFile = open(LongFilePath(FileName), mode='r')
            line = fFile.readline()
            IsHeaderLine = False
            HeaderFiles = []
            while line:
                if "#include" in line:
                    IsHeaderLine = True
                    if line.find('<') != -1:
                        HeaderFile = line[line.find('<') + 1:line.find('>')]
                        f.write('- ' + HeaderFile + '\n')
                        HeaderFiles.append(HeaderFile)
                    if line.find('\"') != -1:
                        l = line.find('\"') + 1
                        r = l + line[l:].find('\"')
                        HeaderFile = line[l:r]
                        f.write('- ' + HeaderFile + '\n')
                        HeaderFiles.append(HeaderFile)
                line = fFile.readline()
                if IsHeaderLine == True and "#include" not in line:
                    break
            fFile.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)
        return HeaderFiles

    # parse uni string tokens defined in DxeStrDefs.h (saved in debug directory)
    def _GetUniDictsForYaml(self):
        if self.Options.ProcessedInFileName.find('/') == -1:
            self.Options.ProcessedInFileName = self.Options.ProcessedInFileName.replace('\\', '/')
        FileList = []
        if self.Options.ProcessedInFileName.find('DriverSample') != -1:
            FileList = self.FindIncludeHeaderFile('/edk2/', self.Options.ProcessedInFileName.split('/')[-3][:-3] + 'StrDefs.h')
        else:
            FileList = self.FindIncludeHeaderFile('/edk2/', self.Options.ProcessedInFileName.split('/')[-3] + 'StrDefs.h')
        if self.Options.UniStrDefFileName == []:
            EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                            "File/directory %s not found in workspace" % (self.Options.ProcessedInFileName.split('/')[-3] + 'StrDefs.h'), None)
        self.Options.UniStrDefFileName = FileList[0]
        CppHeader = CppHeaderParser.CppHeader(self.Options.UniStrDefFileName)
        UniDict = {}
        for Define in CppHeader.defines:
            Items = Define.split()
            if len(Items) == 2 and Items[1].find('0x') != -1: # need refine
                UniDict[int(Items[1], 16)] = Items[0]
        return UniDict

    def ParseDefines(self, Defines, HeaderDict):
        for Define in Defines:
            #print(Define)
            Items = Define.split()
            if len(Items) == 2 and Items[1].find('0x') != -1:
                HeaderDict[int(Items[1], 16)] = Items[0]
                HeaderDict[Items[1]] = Items[0]
            elif len(Items) > 2 and Items[0].find('GUID') != -1:
                GuidStr = ''
                for i in range(1, len(Items)):
                    if Items[i] != '{' and Items[i] != '}' and Items[i] != '{{' and Items[i] !='}}' and Items[i] != '\\':
                        if ('{' in Items[i]):
                            Items[i] = Items[i][1:]
                        if '}' in Items[i]:
                            Items[i] = Items[i][:-1]

                        GuidStr += Items[i]
                GuidItems = GuidStr.split(',')
                Guid = EFI_GUID()
                Guid.Data1 = int(GuidItems[0], 16)
                Guid.Data2 = int(GuidItems[1], 16)
                Guid.Data3 = int(GuidItems[2], 16)
                for i in range(0, 8):
                    Guid.Data4[i] = int(GuidItems[i+3], 16)
                if Items[0].find('\\') != -1:
                    Items[0] = Items[0][:-1]
                Key = Items[0]
                Value = Guid.to_string()
                HeaderDict[Value] = Key

    def FindIncludeHeaderFile(self, Start, Name):
        FileList = []
        for Relpath, Dirs, Files in os.walk(Start):
            if Name in Files:
                FullPath = os.path.join(Start, Relpath, Name)
                FileList.append(os.path.normpath(os.path.abspath(FullPath)))
        return FileList

    # parse header files defined in vfr
    def _GetHeaderDictsForYaml(self, HeaderFiles):
        HeaderDict = {}
        for HeaderFile in HeaderFiles:
            FileName = HeaderFile.split('/')[-1]
            FileList = self.FindIncludeHeaderFile("/edk2/", FileName)
            if FileList == []:
                EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                            "File/directory %s not found in workspace" % (HeaderFile), None)
            for File in FileList:
                if File.find(HeaderFile.replace('/','\\')) != -1:
                    CppHeader = CppHeaderParser.CppHeader(File)
                    self.ParseDefines(CppHeader.defines, HeaderDict)
                    break
            for Include in CppHeader.includes:
                Include = Include[1:-1]
                IncludeFileName = Include.split('/')[1]
                IncludeHeaderFileList = self.FindIncludeHeaderFile("/edk2/", IncludeFileName)
                #print(IncludeHeaderFileList)
                if IncludeHeaderFileList == []:
                    EdkLogger.error("VfrCompiler", FILE_NOT_FOUND,
                                    "File/directory %s not found in workspace" % IncludeFileName, None)
                for File in IncludeHeaderFileList:
                    if File.find(Include.replace('/','\\')) != -1:
                        CppHeader = CppHeaderParser.CppHeader(File)
                        self.ParseDefines(CppHeader.defines, HeaderDict)
                        break
        #print(HeaderDict)
        return HeaderDict

    def _GetVfrDictsForYaml(self):
        VfrDict = {}
        File = open(self.Options.VfrFileName, 'r')
        Defines = []
        Wrap = False
        IncompleteLine = ''
        for Line in File:
            if Wrap == True:
                Defines.append(IncompleteLine + Line.replace('\n', ''))
                Wrap = False
                continue
            if "#define" in Line:
                if Line.find('\\') != -1:
                    Wrap = True
                    IncompleteLine = Line.split(" ", 1)[1].replace('\\\n', '')
                else:
                    Defines.append(Line.split(" ", 1)[1].replace('\n', ''))
        self.ParseDefines(Defines, VfrDict)
        return VfrDict

    def _GenST(self, Key):
        return 'STRING_TOKEN(' + Key  + ')\n'

    def _DumpQuestionInfosWithUni(self, Root, f, ValueIndent, UniDict, HeaderDict):

        Info = Root.Data.GetInfo()
        if Root.Condition != None:
            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(Root.Condition))
        if Root.Data.QName != None:
            f.write(ValueIndent + 'name:  {}  #  Optional Input\n'.
                                format(Root.Data.QName))
        if Root.Data.VarIdStr != '':
            f.write(ValueIndent + 'varid:  {}  #  Optional Input\n'.
                                format(Root.Data.VarIdStr))
        if Root.Data.HasQuestionId:
            if Info.Question.QuestionId in HeaderDict.keys():
                f.write(ValueIndent + 'questionid:  ' + HeaderDict[Info.Question.QuestionId] +' # Optional Input, Need to compute if None\n')
            else:
                f.write(ValueIndent + 'questionid:  {}  # Optional Input, Need to compute if None\n'
                        .format("0x%x" % Info.Question.QuestionId))

        f.write(ValueIndent + 'prompt:  ' + self._GenST(UniDict[Info.Question.Header.Prompt]))
        f.write(ValueIndent + 'help:  ' + self._GenST(UniDict[Info.Question.Header.Help]))
        if Root.Data.FlagsStream != '':
            f.write(ValueIndent + 'flags:  {}  # Optional input , flags\n'.
                        format(Root.Data.FlagsStream))
        if Root.Data.HasKey:
            if Info.Question.QuestionId in HeaderDict.keys():
                f.write(ValueIndent + 'key:  ' + HeaderDict[Info.Question.QuestionId] +' # Optional Input\n')
            else:
                f.write(ValueIndent + 'key:  {} # Optional input, key\n'.format("0x%0x " % Info.Question.QuestionId))


    def _DumpYamlDfsWithUni(self, Root, f, UniDict, HeaderDict, VfrDict):
        try:
            if Root.OpCode != None:
                if Root.Level == 0:
                    KeyIndent = ''
                    ValueIndent = ''
                else:
                    KeyIndent = ' ' *((Root.Level *2 - 1) * 2)
                    ValueIndent = ' ' * ((Root.Level*2 + 1) * 2)

                Info = Root.Data.GetInfo()

                if Root.OpCode == EFI_IFR_FORM_SET_OP:
                    f.write(KeyIndent + 'formset:\n')
                    ValueIndent = ' ' * (Root.Level + 1) * 2
                    f.write(ValueIndent + 'guid:  '  + HeaderDict[Info.Guid.to_string()] + '\n')
                    f.write(ValueIndent +
                            'title:  ' + self._GenST(UniDict[Info.FormSetTitle]))
                    f.write(
                        ValueIndent + 'help:  ' + self._GenST(UniDict[Info.Help]))
                    if len(Root.Data.GetClassGuid()) == 1:
                        Guid = Root.Data.GetClassGuid()[0]
                        f.write(ValueIndent + 'classguid:  '  + HeaderDict[Guid.to_string()] + '\n')
                    else:
                        for i in range(0, len(Root.Data.GetClassGuid())):
                            Guid = Root.Data.GetClassGuid()[i]
                            f.write(ValueIndent + 'classguid:  '  + HeaderDict[Guid.to_string()] + '\n')
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_VARSTORE_OP:
                    f.write(KeyIndent + '- varstore:\n')
                    f.write(ValueIndent +
                            'type:  {}\n'.format(Root.Data.Type))
                    if Root.Data.HasVarStoreId:
                        f.write(
                            ValueIndent +
                            'varid:  {} # Optional Input, Need to assign if None\n'
                            .format('0x%04x' % (Info.VarStoreId)))
                    Name = ''
                    for i in range(0, len(Info.Name)):
                        Name += chr(Info.Name[i])
                    f.write(ValueIndent + 'name:  {}\n'.format(Name))
                    f.write(ValueIndent + 'guid:  ' + HeaderDict[Info.Guid.to_string()] + '\n')

                if Root.OpCode == EFI_IFR_VARSTORE_EFI_OP:
                    f.write(KeyIndent + '- efivarstore:\n')
                    f.write(ValueIndent +
                            'type:  {}\n'.format(Root.Data.Type))
                    if Root.Data.HasVarStoreId:
                        f.write(
                            ValueIndent +
                            'varid:  {} # Optional Input, Need to assign if None\n'
                            .format('0x%04x' % (Info.VarStoreId)))
                    Name = ''
                    for i in range(0, len(Info.Name)):
                        Name += chr(Info.Name[i])
                    f.write(ValueIndent + 'name:  {}\n'.format(Name))
                    f.write(ValueIndent + 'guid:  ' + HeaderDict[Info.Guid.to_string()] + '\n')
                    Attributes = Root.Data.AttributesText.split('|')
                    AttributesText = ''
                    for i in range(0, len(Attributes)):
                        Attribute = int(Attributes[i], 16)
                        if Attribute in HeaderDict.keys():
                            AttributesText += HeaderDict[Attribute]
                        else:
                            AttributesText += Attribute
                        if i != len(Attributes) - 1:
                            AttributesText += ' | '
                    f.write(ValueIndent + 'attribute:  {} \n'.format(AttributesText))
                    # f.write(ValueIndent + 'size:  {} # Need to Compute\n'.format(Info.Size))

                if Root.OpCode == EFI_IFR_VARSTORE_NAME_VALUE_OP:
                    f.write(KeyIndent + '- namevaluevarstore:\n')
                    f.write(ValueIndent +
                            'type:  {}\n'.format(Root.Data.Type))
                    if Root.Data.HasVarStoreId:
                        f.write(
                        ValueIndent +
                        'varid:  {} # Optional Input, Need to assign if None\n'
                        .format('0x%04x' % (Info.VarStoreId)))
                    for NameItem in Root.Data.NameItemList:
                        f.write(ValueIndent +
                                'name:  ' + self._GenST(UniDict[NameItem]))
                    f.write(ValueIndent + 'guid:  ' + HeaderDict[Info.Guid.to_string()] + '\n')

                if Root.OpCode == EFI_IFR_DEFAULTSTORE_OP:
                    gVfrDefaultStore.UpdateDefaultType(Root)
                    Info = Root.Data.GetInfo()
                    Type = Root.Data.Type
                    if Type != 'Standard Defaults' and Type != 'Standard ManuFacturing':
                        f.write(KeyIndent + '- defaultstore:\n')
                        f.write(ValueIndent + 'type:  {}\n'.format(Type))
                        f.write(ValueIndent +
                                'prompt:  ' + self._GenST(UniDict[Info.DefaultName]))
                        f.write(ValueIndent +
                                'attribute:  {} # Default ID\n'.format(
                                    '0x%04x' % (Info.DefaultId)))

                if Root.OpCode == EFI_IFR_FORM_OP:
                    f.write(KeyIndent + '- form: \n')
                    if (Info.FormId in HeaderDict.keys()) and ("FORM_ID" in HeaderDict[Info.FormId]):
                        f.write(ValueIndent + 'formid:  ' + HeaderDict[Info.FormId] + '\n')
                    else:
                        f.write(ValueIndent + 'formid:  {} \n'.format("0x%x" % Info.FormId))

                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'title:  ' + self._GenST(UniDict[Info.FormTitle]))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')


                if Root.OpCode == EFI_IFR_FORM_MAP_OP:
                    MethodMapList = Root.Data.GetMethodMapList()
                    f.write(KeyIndent + '- formmap: \n')
                    f.write(
                        ValueIndent +
                        'formid:  ' + HeaderDict[Info.FormId] + '\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    if MethodMapList != []:
                        f.write(ValueIndent + 'map: # optional input\n')
                    for MethodMap in MethodMapList:
                        f.write(
                            ValueIndent +
                            '- maptitle:  {}\n'.format(MethodMap.MethodTitle))
                        f.write(ValueIndent + '  mapguid:  \'{' + '{}, {}, {},'.format('0x%x'%(MethodMap.MethodIdentifier.Data1),'0x%x'%(MethodMap.MethodIdentifier.Data2), '0x%x'%(MethodMap.MethodIdentifier.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(MethodMap.MethodIdentifier.Data4[0]), '0x%x'%(MethodMap.MethodIdentifier.Data4[1]), '0x%x'%(MethodMap.MethodIdentifier.Data4[2]), '0x%x'%(MethodMap.MethodIdentifier.Data4[3]), \
                        '0x%x'%(MethodMap.MethodIdentifier.Data4[4]), '0x%x'%(MethodMap.MethodIdentifier.Data4[5]), '0x%x'%(MethodMap.MethodIdentifier.Data4[6]), '0x%x'%(MethodMap.MethodIdentifier.Data4[7])) + ' }}\'\n')
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_IMAGE_OP:
                    f.write(KeyIndent + '- image:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'id:  {} # ImageId\n'.format(Info.Id))

                if Root.OpCode == EFI_IFR_RULE_OP:  #
                    f.write(KeyIndent + '- rule:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'rulename:  {} \n'.format(Root.Data.GetRuleName()))
                    f.write(ValueIndent +
                            'expression:  {} \n'.format(Root.Expression))

                if Root.OpCode == EFI_IFR_SUBTITLE_OP:
                    f.write(KeyIndent + '- subtitle:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'prompt:  ' + self._GenST(UniDict[Info.Statement.Prompt]))
                    if Root.Data.FlagsStream != '':
                        f.write(
                            ValueIndent +
                            'flags:  {}  # Optional Input\n'.format(Root.Data.FlagsStream))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_TEXT_OP:
                    f.write(KeyIndent + '- text:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    if type(Info) == EFI_IFR_TEXT:
                        f.write(ValueIndent +
                                'help:  ' + self._GenST(UniDict[Info.Statement.Help]))
                        f.write(ValueIndent +
                                'prompt:  ' + self._GenST(UniDict[Info.Statement.Prompt]))
                        if Info.TextTwo in UniDict.keys():
                            f.write(
                                ValueIndent +
                                'text:  ' + self._GenST(UniDict[Info.TextTwo]))
                    if type(Info) == EFI_IFR_ACTION:
                        f.write(ValueIndent +
                                'help:  ' + self._GenST(UniDict[Info.Question.Header.Help]))
                        f.write(ValueIndent +
                                'prompt:  ' + self._GenST(UniDict[Info.Question.Header.Prompt]))
                        if Root.Data.FlagsStream != '':
                            f.write(
                            ValueIndent +
                            'flags:  {}  # Optional Input, Question Flags\n'
                            .format(Root.Data.FlagsStream))
                        if Root.Data.HasKey:
                            if Info.Question.QuestionId in HeaderDict.keys():
                                f.write(ValueIndent + 'key:  ' + HeaderDict[Info.Question.QuestionId] + '\n')
                            else:
                                f.write(ValueIndent +'key:  {}  # Optional Input, Question QuestionId\n'.format('0x%04x' % (Info.Question.QuestionId)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_ACTION_OP:
                    f.write(KeyIndent + '- action:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    f.write(ValueIndent + 'questionconfig:  {}  # QuestionConfig\n'.
                        format(Info.QuestionConfig))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_ONE_OF_OP:
                    f.write(KeyIndent + '- oneof:\n')

                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)

                    if Root.Data.HasMinMax:
                        f.write(ValueIndent + 'maximum:  {} # Optional Input\n'.format(
                                '0x%0x' % (Info.Data.MaxValue)))
                        f.write(ValueIndent + 'minimum:  {} # Optional Input\n'.format(
                                '0x%0x' %(Info.Data.MinValue)))
                    if Root.Data.HasStep:
                        f.write(ValueIndent + 'step:  {} # Optional Input\n'.format(
                            Info.Data.Step))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_ONE_OF_OPTION_OP:
                    f.write(KeyIndent + '- option:  \n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'text:  ' + self._GenST(UniDict[Info.Option]))

                    if type(Root.Data) == IfrOneOfOption:
                        if Root.Data.ValueStream != '':
                            f.write(ValueIndent + 'value:  {}\n'.format(Root.Data.ValueStream))

                    if Root.Data.FlagsStream != '':
                        f.write(ValueIndent + 'flags:  {} # Optional Input\n'.
                            format(Root.Data.FlagsStream))
                    if Root.Data.IfrOptionKey != None:
                        f.write(ValueIndent + 'key:  {} # Optional Input\n'.
                            format('0x%04x' % (Root.Data.GetIfrOptionKey())))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_DEFAULT_OP:
                    f.write(KeyIndent + '- default:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))

                    if type(Root.Data) == IfrDefault:
                        if Root.Data.ValueStream != '':
                            f.write(ValueIndent + 'value:  {}\n'.format(Root.Data.ValueStream))

                        else:
                            f.write(ValueIndent + 'value:  {')
                            for i in range(0, len(Info.Value) - 1):
                                f.write('{},'.format(Info.Value[i]))
                            f.write('{}'.format(Info.Value[len(Info.Value) -
                                                           1]) + '}\n')

                    elif type(Root.Data) == IfrDefault2:
                        f.write(ValueIndent + 'value: \'{}\'\n'.format(
                            Root.Child[0].Expression))

                    if Root.Data.DefaultStore != '':
                        f.write(ValueIndent + 'defaultstore: {}\n'.format(
                            Root.Data.DefaultStore))

                if Root.OpCode == EFI_IFR_ORDERED_LIST_OP:
                    f.write(KeyIndent + '- orderedlist:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    if Root.Data.HasMaxContainers:
                        f.write(ValueIndent + 'maxcontainers:  {} ## Optional Input, Need to compute if None\n'
                        .format(Info.MaxContainers))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_NUMERIC_OP:
                    f.write(KeyIndent + '- numeric:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)

                    f.write(ValueIndent + 'maximum:  {} # Optional Input\n'.format(
                            '0x%0x' % (Info.Data.MaxValue)))
                    f.write(ValueIndent + 'minimum:  {} # Optional Input\n'.format(
                            '0x%0x' %(Info.Data.MinValue)))
                    if Root.Data.HasStep:
                        f.write(ValueIndent + 'step:  {} # Optional Input\n'.format(
                        '0x%0x' %(Info.Data.Step)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_CHECKBOX_OP:
                    f.write(KeyIndent + '- checkbox:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_TIME_OP:
                    f.write(KeyIndent + '- time:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)

                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_DATE_OP:
                    f.write(KeyIndent + '- date:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_STRING_OP:
                    f.write(KeyIndent + '- string:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    f.write(ValueIndent + 'minsize:  {} \n'.format(Info.MinSize))
                    f.write(ValueIndent + 'maxsize:  {} \n'.format(Info.MaxSize))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_PASSWORD_OP:
                    f.write(KeyIndent + '- password:\n')
                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    f.write(ValueIndent + 'minsize:  {} \n'.format(Info.MinSize))
                    f.write(ValueIndent + 'maxsize:  {} \n'.format(Info.MaxSize))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_RESET_BUTTON_OP:
                    f.write(KeyIndent + '- resetbutton:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent + 'defaultstore:  {}\n'.format(
                        Root.Data.DefaultStore))
                    f.write(ValueIndent +
                            'prompt:  ' + self._GenST(UniDict[Info.Statement.Prompt]))
                    f.write(ValueIndent + 'help:  ' + self._GenST(UniDict[Info.Statement.Help]))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_REF_OP:
                    f.write(KeyIndent + '- goto:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))

                    if type(Root.Data) == IfrRef4:
                        if Info.FormId in HeaderDict.keys() and ("FORM_ID" in HeaderDict[Info.FormId]):
                            f.write(ValueIndent + 'formid:  ' + HeaderDict[Info.FormId] + '\n')
                        else:
                            f.write(ValueIndent + 'formid:  {}\n'.format('0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'formsetguid:  ' + HeaderDict[Info.FormSetId.to_string()] + ' # Optional Input\n')
                        f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format("0x%x" %Info.QuestionId))
                        f.write(ValueIndent + 'devicepath:  {} #  Optional Input\n'.
                            format('0x%04x' % (Info.DevicePath)))

                    if type(Root.Data) == IfrRef3:
                        if Info.FormId in HeaderDict.keys() and ("FORM_ID" in HeaderDict[Info.FormId]):
                            f.write(ValueIndent + 'formid:  ' + HeaderDict[Info.FormId] + '\n')
                        else:
                            f.write(ValueIndent + 'formid:  {}\n'.format('0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'formsetguid:  ' + HeaderDict[Info.FormSetId.to_string()] + ' # Optional Input\n')
                        f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format("0x%x" %Info.QuestionId))

                    if type(Root.Data) == IfrRef2:
                        if Info.FormId in HeaderDict.keys() and ("FORM_ID" in HeaderDict[Info.FormId]):
                            f.write(ValueIndent + 'formid:  ' + HeaderDict[Info.FormId] + '\n')
                        else:
                            f.write(ValueIndent + 'formid:  {}\n'.format('0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format("0x%x" %Info.QuestionId))

                    if type(Root.Data) == IfrRef:
                        if Info.FormId in HeaderDict.keys() and ("FORM_ID" in HeaderDict[Info.FormId]):
                            f.write(ValueIndent + 'formid:  ' + HeaderDict[Info.FormId] + '\n')
                        else:
                            f.write(ValueIndent + 'formid:  {}\n'.format('0x%x' % (Info.FormId)))
                        #f.write(ValueIndent + 'question:  ' + HeaderDict[Info.Question.QuestionId] + ' #  Optional Input\n')

                    self._DumpQuestionInfosWithUni(Root, f, ValueIndent, UniDict, HeaderDict)
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_REFRESH_OP:
                    f.write(KeyIndent + '- refresh:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'interval:  {}  # RefreshInterval\n'.
                        format(Info.RefreshInterval))

                if Root.OpCode == EFI_IFR_VARSTORE_DEVICE_OP:
                    f.write(KeyIndent + '- varstoredevice:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'devicepath:  {}  # DevicePath\n'.
                        format(Info.DevicePath))

                if Root.OpCode == EFI_IFR_REFRESH_ID_OP:
                    f.write(KeyIndent + '- refreshguid:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'guid:  ' + HeaderDict[Info.RefreshEventGroupId.to_string()] + '\n')

                if Root.OpCode == EFI_IFR_WARNING_IF_OP:
                    f.write(KeyIndent + '- warningif:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'warning:  {}\n'.format(
                        Info.Warning))
                    if Root.Data.HasTimeOut:
                        f.write(ValueIndent + 'timeout:  {} # optional input \n'.format(
                        Info.TimeOut))
                    f.write(ValueIndent + 'expression:  {}\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_GUID_OP:
                    if type(Root.Data
                            ) == IfrLabel:  # type(Info) == EFI_IFR_GUID_LABEL
                        f.write(KeyIndent + '- label:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                        if Info.Number in HeaderDict.keys():
                            f.write(ValueIndent + 'number:  ' + HeaderDict[Info.Number] + '\n')
                        elif Info.Number in VfrDict.keys():
                            f.write(ValueIndent + 'number:  ' + VfrDict[Info.Number] + '\n')
                        else:
                            f.write(ValueIndent + 'number:  {}  # Number\n'.format(
                            '0x%x' % (Info.Number)))


                    if type(Root.Data) == IfrBanner:
                        f.write(KeyIndent + '- banner:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))

                        f.write(ValueIndent + 'title:  {}\n'.format(Info.Title))
                        f.write(ValueIndent + 'line:  {}\n'.format(
                            Info.LineNumber))
                        f.write(ValueIndent + 'align:  {}\n'.format(
                            Info.Alignment))

                    if type(Root.Data) == IfrTimeout:
                        f.write(KeyIndent + '- banner:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))

                        f.write(ValueIndent + 'timeout:  {}\n'.format(
                            Info.TimeOut))

                    if type(Root.Data) == IfrClass:
                        ValueIndent = ' ' * ((Root.Level-1) * 2 + 1)
                        f.write(ValueIndent + 'class:  {}\n'.format(Info.Class))

                    if type(Root.Data) == IfrSubClass:
                        ValueIndent = ' ' * ((Root.Level-1) * 2 + 1)
                        f.write(ValueIndent + 'subclass:  {}\n'.format(Info.SubClass))

                    if type(Root.Data) == IfrExtensionGuid:
                        if type(Root.Parent.Data) == IfrExtensionGuid:
                            Root.Level -= 1
                            KeyIndent = ' ' *((Root.Level *2 - 1) * 2)
                            ValueIndent = ' ' * ((Root.Level*2 + 1) * 2)
                        f.write(KeyIndent + '- guidop:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                        f.write(ValueIndent + 'guid:  ' + HeaderDict[Info.Guid.to_string()] + '\n')
                        if Root.Data.GetDataType() != '':
                            f.write(ValueIndent + 'databuffer: #optional input\n')
                            f.write(ValueIndent + '- {}: \n'.format(Root.Data.GetDataType()))
                            for data in Root.Data.GetFieldList():
                                f.write(ValueIndent + '  {}:  {}\n'.format(data[0], '0x%x' %(data[1])))


                if Root.OpCode == EFI_IFR_INCONSISTENT_IF_OP:
                    if type(Root.Data) == IfrInconsistentIf2:  #
                        f.write(KeyIndent + '- inconsistentif:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                        f.write(ValueIndent + 'prompt:  ' + self._GenST(UniDict[Info.Error]))
                        f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                                Root.Expression))

                if Root.OpCode == EFI_IFR_NO_SUBMIT_IF_OP:
                    f.write(KeyIndent + '- nosubmitif:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'prompt:  ' + self._GenST(UniDict[Info.Error]))
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_READ_OP:
                    f.write(KeyIndent + '- read:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_WRITE_OP:
                    f.write(KeyIndent + '- write:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_VALUE_OP and Root.Parent.OpCode != EFI_IFR_DEFAULT_OP:  #
                    f.write(KeyIndent + '- value:\n')
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_MODAL_TAG_OP:
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(KeyIndent + '- modal: tag\n')

                if Root.OpCode == EFI_IFR_LOCKED_OP:
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(KeyIndent + '- locked: tag\n')

            self.LastOp = Root.OpCode

        except:
            EdkLogger.error(
                "VfrCompiler", FILE_WRITE_FAILURE, "File write failed for %s" %
                (self.Options.YamlFileName), None)

        if Root.Child != []:
            for ChildNode in Root.Child:
                if Root.OpCode in ConditionOps:
                    if ChildNode.OpCode in ConditionOps:
                        ChildNode.Condition = Root.Condition + ' | ' + ChildNode.Condition
                    else:
                        ChildNode.Condition = Root.Condition

                    if type(ChildNode.Data) == IfrInconsistentIf2:
                        ChildNode.Level = Root.Level + 1
                    else:
                        ChildNode.Level = Root.Level
                else:
                    if type(Root.Data) == IfrGuid and (ChildNode.OpCode in [EFI_IFR_CHECKBOX_OP, EFI_IFR_NUMERIC_OP, EFI_IFR_ONE_OF_OP]):
                        ChildNode.Level = Root.Level
                    else:
                        ChildNode.Level = Root.Level + 1


                self._DumpYamlDfsWithUni(ChildNode, f, UniDict, HeaderDict, VfrDict)

        return

    def DumpQuestionInfos(self, Root, f,  ValueIndent):

        Info = Root.Data.GetInfo()
        if Root.Condition != None:
            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(Root.Condition))
        if Root.Data.QName != None:
            f.write(ValueIndent + 'name:  {}  #  Optional Input\n'.
                                format(Root.Data.QName))
        if Root.Data.VarIdStr != '':
            f.write(ValueIndent + 'varid:  {}  #  Optional Input\n'.
                                format(Root.Data.VarIdStr))
        if Root.Data.HasQuestionId:
            f.write(ValueIndent + 'questionid:  {}  # Optional Input, Need to compute if None\n'
                        .format(Info.Question.QuestionId))
        f.write(ValueIndent + 'prompt:  {}  # Statement Prompt STRING_ID\n'
                        .format('0x%04x' % (Info.Question.Header.Prompt)))
        f.write(ValueIndent + 'help:  {}  # Statement Help STRING_ID\n'.
                        format('0x%04x' % (Info.Question.Header.Help)))
        if Root.Data.FlagsStream != '':
            f.write(ValueIndent + 'flags:  {}  # Optional input , flags\n'.
                        format(Root.Data.FlagsStream))
        if Root.Data.HasKey:
            f.write(ValueIndent + 'key: {} # Optional input, key \n'.format(Root.Data.HasKey))

    def DumpYamlDfs(self, Root, f):
        try:
            if Root.OpCode != None:
                if Root.Level == 0:
                    KeyIndent = ''
                    ValueIndent = ''
                else:
                    KeyIndent = ' ' *((Root.Level *2 - 1) * 2)
                    ValueIndent = ' ' * ((Root.Level*2 + 1) * 2)

                Info = Root.Data.GetInfo()

                if Root.OpCode == EFI_IFR_FORM_SET_OP:
                    f.write(KeyIndent + 'formset:\n')
                    ValueIndent = ' ' * (Root.Level + 1) * 2
                    f.write(ValueIndent + 'guid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                        '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\'\n')
                    f.write(ValueIndent +
                            'title:  {}  # Title STRING_ID\n'.format(
                                '0x%04x' % (Info.FormSetTitle)))
                    f.write(
                        ValueIndent +
                        'help:  {}  # Help STRING_ID\n'.format('0x%04x' %
                                                               (Info.Help)))
                    if len(Root.Data.GetClassGuid()) == 1:
                        Guid = Root.Data.GetClassGuid()[0]
                        f.write(ValueIndent + 'classguid:  \'{'  + '{}, {}, {},'.format('0x%x'%(Guid.Data1),'0x%x'%(Guid.Data2), '0x%x'%(Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Guid.Data4[0]), '0x%x'%(Guid.Data4[1]), '0x%x'%(Guid.Data4[2]), '0x%x'%(Guid.Data4[3]), \
                        '0x%x'%(Guid.Data4[4]), '0x%x'%(Guid.Data4[5]), '0x%x'%(Guid.Data4[6]), '0x%x'%(Guid.Data4[7])) + ' }}\'\n')
                    else:
                        for i in range(0, len(Root.Data.GetClassGuid())):
                            Guid = Root.Data.GetClassGuid()[i]
                            f.write(ValueIndent + 'classguid{}:  '.format(i+1) + '\'{'  + '{}, {}, {},'.format('0x%x'%(Guid.Data1),'0x%x'%(Guid.Data2), '0x%x'%(Guid.Data3)) \
                            + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Guid.Data4[0]), '0x%x'%(Guid.Data4[1]), '0x%x'%(Guid.Data4[2]), '0x%x'%(Guid.Data4[3]), \
                            '0x%x'%(Guid.Data4[4]), '0x%x'%(Guid.Data4[5]), '0x%x'%(Guid.Data4[6]), '0x%x'%(Guid.Data4[7])) + ' }}\'\n')
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_VARSTORE_OP:
                    f.write(KeyIndent + '- varstore:\n')
                    f.write(ValueIndent +
                            'type:  {}\n'.format(Root.Data.Type))
                    if Root.Data.HasVarStoreId:
                        f.write(
                            ValueIndent +
                            'varid:  {} # Optional Input, Need to assign if None\n'
                            .format('0x%04x' % (Info.VarStoreId)))
                    Name = ''
                    for i in range(0, len(Info.Name)):
                        Name += chr(Info.Name[i])
                    f.write(ValueIndent + 'name:  {}\n'.format(Name))
                    f.write(ValueIndent + 'guid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                        '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\'\n')
                    # f.write(ValueIndent + 'size:  {} # Need to Compute\n'.format(Info.Size))

                if Root.OpCode == EFI_IFR_VARSTORE_EFI_OP:
                    f.write(KeyIndent + '- efivarstore:\n')
                    f.write(ValueIndent +
                            'type:  {}\n'.format(Root.Data.Type))
                    if Root.Data.HasVarStoreId:
                        f.write(
                            ValueIndent +
                            'varid:  {} # Optional Input, Need to assign if None\n'
                            .format('0x%04x' % (Info.VarStoreId)))
                    Name = ''
                    for i in range(0, len(Info.Name)):
                        Name += chr(Info.Name[i])
                    f.write(ValueIndent + 'name:  {}\n'.format(Name))
                    f.write(ValueIndent + 'guid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                        '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\'\n')
                    f.write(
                        ValueIndent +
                        'attribute:  {} \n'
                        .format(Root.Data.AttributesText))
                    # f.write(ValueIndent + 'size:  {} # Need to Compute\n'.format(Info.Size))

                if Root.OpCode == EFI_IFR_VARSTORE_NAME_VALUE_OP:
                    f.write(KeyIndent + '- namevaluevarstore:\n')
                    f.write(ValueIndent +
                            'type:  {}\n'.format(Root.Data.Type))
                    if Root.Data.HasVarStoreId:
                        f.write(
                        ValueIndent +
                        'varid:  {} # Optional Input, Need to assign if None\n'
                        .format('0x%04x' % (Info.VarStoreId)))
                    for NameItem in Root.Data.NameItemList:
                        f.write(ValueIndent +
                                'name:  {} # NameList STRING_ID\n'.format(
                                    '0x%04x' % (NameItem)))
                    f.write(ValueIndent + 'guid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                        '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\'\n')

                if Root.OpCode == EFI_IFR_DEFAULTSTORE_OP:
                    gVfrDefaultStore.UpdateDefaultType(Root)
                    Info = Root.Data.GetInfo()
                    Type = Root.Data.Type
                    if Type != 'Standard Defaults' and Type != 'Standard ManuFacturing':
                        f.write(KeyIndent + '- defaultstore:\n')
                        f.write(ValueIndent + 'type:  {}\n'.format(Type))
                        f.write(ValueIndent +
                                'prompt:  {} # DefaultName STRING_ID\n'.format(
                                    '0x%04x' % (Info.DefaultName)))
                        f.write(ValueIndent +
                                'attribute:  {} # Default ID\n'.format(
                                    '0x%04x' % (Info.DefaultId)))

                if Root.OpCode == EFI_IFR_FORM_OP:
                    f.write(KeyIndent + '- form: \n')
                    f.write(
                        ValueIndent +
                        'formid:  {} \n'.format(Info.FormId))
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'title:  {}  # FormTitle STRING_ID\n'.format(
                                '0x%04x' % (Info.FormTitle)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')


                if Root.OpCode == EFI_IFR_FORM_MAP_OP:
                    MethodMapList = Root.Data.GetMethodMapList()
                    f.write(KeyIndent + '- formmap: \n')
                    f.write(
                        ValueIndent +
                        'formid:  {} # FormId STRING_ID\n'.format(Info.FormId))
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    if MethodMapList != []:
                        f.write(ValueIndent + 'map: # optional input\n')
                    for MethodMap in MethodMapList:
                        f.write(
                            ValueIndent +
                            '- maptitle:  {}\n'.format(MethodMap.MethodTitle))
                        f.write(ValueIndent + '  mapguid:  \'{' + '{}, {}, {},'.format('0x%x'%(MethodMap.MethodIdentifier.Data1),'0x%x'%(MethodMap.MethodIdentifier.Data2), '0x%x'%(MethodMap.MethodIdentifier.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(MethodMap.MethodIdentifier.Data4[0]), '0x%x'%(MethodMap.MethodIdentifier.Data4[1]), '0x%x'%(MethodMap.MethodIdentifier.Data4[2]), '0x%x'%(MethodMap.MethodIdentifier.Data4[3]), \
                        '0x%x'%(MethodMap.MethodIdentifier.Data4[4]), '0x%x'%(MethodMap.MethodIdentifier.Data4[5]), '0x%x'%(MethodMap.MethodIdentifier.Data4[6]), '0x%x'%(MethodMap.MethodIdentifier.Data4[7])) + ' }}\'\n')
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_IMAGE_OP:
                    f.write(KeyIndent + '- image:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'id:  {} # ImageId\n'.format(Info.Id))

                if Root.OpCode == EFI_IFR_RULE_OP:  #
                    f.write(KeyIndent + '- rule:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'rulename:  {} \n'.format(Root.Data.GetRuleName()))
                    f.write(ValueIndent +
                            'expression:  {} \n'.format(Root.Expression))

                if Root.OpCode == EFI_IFR_SUBTITLE_OP:
                    f.write(KeyIndent + '- subtitle:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent +
                            'prompt:  {}  # Statement Prompt STRING_ID\n'.
                            format('0x%04x' % (Info.Statement.Prompt)))
                    if Root.Data.FlagsStream != '':
                        f.write(
                            ValueIndent +
                            'flags:  {}  # Optional Input\n'.format(Root.Data.FlagsStream))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_TEXT_OP:
                    f.write(KeyIndent + '- text:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    if type(Info) == EFI_IFR_TEXT:
                        f.write(ValueIndent +
                                'help:  {}  # Statement Help STRING_ID\n'.
                                format('0x%04x' % (Info.Statement.Help)))
                        f.write(ValueIndent +
                                'prompt:  {}  # Statement Prompt STRING_ID\n'.
                                format('0x%04x' % (Info.Statement.Prompt)))
                        f.write(
                            ValueIndent +
                            'text:  {}  # Optional Input, Statement TextTwo STRING_ID\n'
                            .format('0x%04x' % (Info.TextTwo)))
                    if type(Info) == EFI_IFR_ACTION:
                        f.write(ValueIndent +
                                'help:  {}  # Question Help STRING_ID\n'.
                                format('0x%04x' % (Info.Question.Header.Help)))
                        f.write(ValueIndent +
                                'prompt:  {}  # Question Prompt STRING_ID\n'.
                                format('0x%04x' %
                                       (Info.Question.Header.Prompt)))
                        if Root.Data.FlagsStream != '':
                            f.write(
                            ValueIndent +
                            'flags:  {}  # Optional Input, Question Flags\n'
                            .format(Root.Data.FlagsStream))
                        if Root.Data.HasKey:
                            f.write(
                            ValueIndent +
                            'key:  {}  # Optional Input, Question QuestionId\n'
                            .format('0x%04x' % (Info.Question.QuestionId)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_ACTION_OP:
                    f.write(KeyIndent + '- action:\n')

                    self.DumpQuestionInfos(Root, f, ValueIndent)

                    #f.write('varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                    #f.write(ValueIndent + 'questionflags:  {}  # Question Flags\n'.format(Info.Question.Flags))
                    f.write(ValueIndent + 'questionconfig:  {}  # QuestionConfig\n'.
                        format(Info.QuestionConfig))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_ONE_OF_OP:
                    f.write(KeyIndent + '- oneof:\n')

                    self.DumpQuestionInfos(Root, f, ValueIndent)

                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    if Root.Data.HasMinMax:
                        f.write(ValueIndent + 'maximum:  {} # Optional Input\n'.format(
                                Info.Data.MaxValue))
                        f.write(ValueIndent + 'minimum:  {} # Optional Input\n'.format(
                                Info.Data.MinValue))
                    if Root.Data.HasStep:
                        f.write(ValueIndent + 'step:  {} # Optional Input\n'.format(
                            '0x%0x' %(Info.Data.Step)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_ONE_OF_OPTION_OP:
                    f.write(KeyIndent + '- option:  \n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'text:  {} # Option STRING_ID\n'.
                            format('0x%04x' % (Info.Option)))

                    if type(Root.Data) == IfrOneOfOption:
                        if Root.Data.ValueStream != '':
                            f.write(ValueIndent + 'value:  {}\n'.format(Root.Data.ValueStream))
                        '''
                        if len(Info.Value) == 1:
                            f.write(ValueIndent + 'value:  {}\n'.format(
                                '0x%x' % (Info.Value[0])))
                        else:
                            f.write(ValueIndent + 'value:  {')
                            ValueType = Root.Data.ValueType
                            for i in range(0, len(Info.Value) - 1):
                                f.write('{},'.format(Info.Value[i]))
                            f.write('{}'.format(Info.Value[len(Info.Value) -
                                                           1]) + '}\n')
                        '''
                    if Root.Data.FlagsStream != '':
                        f.write(ValueIndent + 'flags:  {} # Optional Input\n'.
                            format(Root.Data.FlagsStream))
                    if Root.Data.IfrOptionKey != None:
                        f.write(ValueIndent + 'key:  {} # Optional Input\n'.
                            format('0x%04x' % (Root.Data.GetIfrOptionKey())))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_DEFAULT_OP:
                    f.write(KeyIndent + '- default:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    # f.write(ValueIndent + 'defaultId:  {}\n'.format(Info.DefaultId))
                    # f.write(ValueIndent + 'type:  {}\n'.format(Info.Type))
                    if type(Root.Data) == IfrDefault:
                        if Root.Data.ValueStream != '':
                            f.write(ValueIndent + 'value:  {}\n'.format(Root.Data.ValueStream))

                        else:
                            f.write(ValueIndent + 'value:  {')
                            for i in range(0, len(Info.Value) - 1):
                                f.write('{},'.format(Info.Value[i]))
                            f.write('{}'.format(Info.Value[len(Info.Value) -
                                                           1]) + '}\n')

                    elif type(Root.Data) == IfrDefault2:
                        f.write(ValueIndent + 'value: \'{}\'\n'.format(
                            Root.Child[0].Expression))

                    if Root.Data.DefaultStore != '':
                        f.write(ValueIndent + 'defaultstore: {}\n'.format(
                            Root.Data.DefaultStore))

                        '''
                        if len(Info.Value) == 1:
                            if Info.Type == EFI_IFR_TYPE_DATE:
                                f.write(ValueIndent + 'value:  {}/{}/{}\n'.
                                        format(Info.Value[0].Year,
                                               Info.Value[0].Month,
                                               Info.Value[0].Day))
                            elif Info.Type == EFI_IFR_TYPE_TIME:
                                f.write(ValueIndent + 'value:  {}:{}:{}\n'.
                                        format(Info.Value[0].Hour,
                                               Info.Value[0].Minute,
                                               Info.Value[0].Second))
                            elif Info.Type == EFI_IFR_TYPE_REF:
                                f.write(ValueIndent + 'value:  {};{};'.format(Info.Value[0].QuestionId, Info.Value[0].FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(Info.Value[0].FormSetGuid.Data1),'0x%x'%(Info.Value[0].FormSetGuid.Data2), '0x%x'%(Info.Value[0].FormSetGuid.Data3)) \
                                + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Value[0].FormSetGuid.Data4[0]), '0x%x'%(Info.Value[0].FormSetGuid.Data4[1]), '0x%x'%(Info.Value[0].FormSetGuid.Data4[2]), '0x%x'%(Info.Value[0].FormSetGuid.Data4[3]), \
                                '0x%x'%(Info.Value[0].FormSetGuid.Data4[4]), '0x%x'%(Info.Value[0].FormSetGuid.Data4[5]), '0x%x'%(Info.Value[0].FormSetGuid.Data4[6]), '0x%x'%(Info.Value[0].FormSetGuid.Data4[7])) + ' }}' + ';{}\n'.format(Info.Value[0].DevicePath))
                            else:
                                f.write(ValueIndent + 'value:  {}\n'.format(
                                        Info.Value[0]))
                        '''

                if Root.OpCode == EFI_IFR_ORDERED_LIST_OP:
                    f.write(KeyIndent + '- orderedlist:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    #f.write('varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                    f.write(ValueIndent + 'maxcontainers:  {} ## Optional Input, Need to compute if None\n'
                        .format(Info.MaxContainers))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_NUMERIC_OP:
                    f.write(KeyIndent + '- numeric:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)

                    #f.write('varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                    #f.write('varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                    #f.write('varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    f.write(ValueIndent + 'maximum:  {} # Optional Input\n'.format(
                            Info.Data.MaxValue))
                    f.write(ValueIndent + 'minimum:  {} # Optional Input\n'.format(
                            Info.Data.MinValue))
                    if Root.Data.HasStep:
                        f.write(ValueIndent + 'step:  {} # Optional Input\n'.format(
                        Info.Data.Step))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_CHECKBOX_OP:
                    f.write(KeyIndent + '- checkbox:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    #f.write('varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                    #f.write('varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                    #f.write('varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_TIME_OP:
                    f.write(KeyIndent + '- time:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    #f.write('varname:  {}\n'.format(Info.Question.VarStoreInfo.VarName))
                    #f.write('varoffset:  {}\n'.format(Info.Question.VarStoreInfo.VarOffset))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_DATE_OP:
                    f.write(KeyIndent + '- date:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    #f.write('varname:  {}\n'.format(Info.Question.VarStoreInfo.VarName))
                    #f.write('varoffset:  {}\n'.format(Info.Question.VarStoreInfo.VarOffset))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_STRING_OP:
                    f.write(KeyIndent + '- string:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    #f.write('varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                    #f.write('varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                    #f.write('varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    f.write(ValueIndent + 'minsize:  {} \n'.format(Info.MinSize))
                    f.write(ValueIndent + 'maxsize:  {} \n'.format(Info.MaxSize))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_PASSWORD_OP:
                    f.write(KeyIndent + '- password:\n')
                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    #f.write('varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                    #f.write('varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                    #f.write('varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                    #f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    #f.write(ValueIndent + 'opcodeflags:  {}  # optional input\n'.format('0x%x' % (Info.Flags)))
                    f.write(ValueIndent + 'minsize:  {} \n'.format(Info.MinSize))
                    f.write(ValueIndent + 'maxsize:  {} \n'.format(Info.MaxSize))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_RESET_BUTTON_OP:
                    f.write(KeyIndent + '- resetbutton:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))
                    f.write(ValueIndent + 'defaultstore:  {}\n'.format(
                        Root.Data.DefaultStore))
                    f.write(ValueIndent +
                            'prompt:  {}  # Statement Prompt STRING_ID\n'.
                            format('0x%04x' % (Info.Statement.Prompt)))
                    f.write(ValueIndent + 'help:  {}  # Statement Help STRING_ID\n'.
                        format('0x%04x' % (Info.Statement.Help)))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_REF_OP:
                    f.write(KeyIndent + '- goto:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent +
                                'condition:  \'{}\'\n'.format(Root.Condition))

                    if type(Root.Data) == IfrRef4:
                        f.write(ValueIndent + 'formid:  {}\n'.format(
                            '0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'formsetguid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.FormSetId.Data1),'0x%x'%(Info.FormSetId.Data2), '0x%x'%(Info.FormSetId.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.FormSetId.Data4[0]), '0x%x'%(Info.FormSetId.Data4[1]), '0x%x'%(Info.FormSetId.Data4[2]), '0x%x'%(Info.FormSetId.Data4[3]), \
                        '0x%x'%(Info.FormSetId.Data4[4]), '0x%x'%(Info.FormSetId.Data4[5]), '0x%x'%(Info.FormSetId.Data4[6]), '0x%x'%(Info.FormSetId.Data4[7])) + ' }}\' #  Optional Input\n')
                        #f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format('0x%x' % (Info.QuestionId)))
                        f.write(ValueIndent + 'devicepath:  {} #  Optional Input\n'.
                            format('0x%04x' % (Info.DevicePath)))

                    if type(Root.Data) == IfrRef3:
                        f.write(ValueIndent + 'formid:  {}\n'.format(
                            '0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'formsetguid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.FormSetId.Data1),'0x%x'%(Info.FormSetId.Data2), '0x%x'%(Info.FormSetId.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.FormSetId.Data4[0]), '0x%x'%(Info.FormSetId.Data4[1]), '0x%x'%(Info.FormSetId.Data4[2]), '0x%x'%(Info.FormSetId.Data4[3]), \
                        '0x%x'%(Info.FormSetId.Data4[4]), '0x%x'%(Info.FormSetId.Data4[5]), '0x%x'%(Info.FormSetId.Data4[6]), '0x%x'%(Info.FormSetId.Data4[7])) + ' }}\' #  Optional Input\n')
                        f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format('0x%x' % (Info.QuestionId)))

                    if type(Root.Data) == IfrRef2:
                        f.write(ValueIndent + 'formid:  {}\n'.format(
                            '0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format(Info.QuestionId))

                    if type(Root.Data) == IfrRef:
                        f.write(ValueIndent + 'formid:  {}\n'.format(
                            '0x%x' % (Info.FormId)))
                        f.write(ValueIndent + 'question:  {} #  Optional Input\n'.format('0x%04x' % (Info.Question.QuestionId)))

                    self.DumpQuestionInfos(Root, f, ValueIndent)
                    f.write(ValueIndent + 'questionflags:  {} # Optional Input \n'.format(Info.Question.Flags))
                    if Root.Child != [] and Root.Child[0].OpCode != EFI_IFR_END_OP:
                        f.write(ValueIndent + 'component:  \n')

                if Root.OpCode == EFI_IFR_REFRESH_OP:
                    f.write(KeyIndent + '- refresh:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'interval:  {}  # RefreshInterval\n'.
                        format(Info.RefreshInterval))

                if Root.OpCode == EFI_IFR_VARSTORE_DEVICE_OP:
                    f.write(KeyIndent + '- varstoredevice:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'devicepath:  {}  # DevicePath\n'.
                        format(Info.DevicePath))

                if Root.OpCode == EFI_IFR_REFRESH_ID_OP:
                    f.write(KeyIndent + '- refreshguid:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'guid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.RefreshEventGroupId.Data1),'0x%x'%(Info.RefreshEventGroupId.Data2), '0x%x'%(Info.RefreshEventGroupId.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.RefreshEventGroupId.Data4[0]), '0x%x'%(Info.RefreshEventGroupId.Data4[1]), '0x%x'%(Info.RefreshEventGroupId.Data4[2]), '0x%x'%(Info.RefreshEventGroupId.Data4[3]), \
                        '0x%x'%(Info.RefreshEventGroupId.Data4[4]), '0x%x'%(Info.RefreshEventGroupId.Data4[5]), '0x%x'%(Info.RefreshEventGroupId.Data4[6]), '0x%x'%(Info.RefreshEventGroupId.Data4[7])) + ' }}\'\n')

                if Root.OpCode == EFI_IFR_WARNING_IF_OP:
                    f.write(KeyIndent + '- warningif:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'warning:  {}\n'.format(
                        Info.Warning))
                    if Root.Data.HasTimeOut:
                        f.write(ValueIndent + 'timeout:  {} # optional input \n'.format(
                        Info.TimeOut))
                    f.write(ValueIndent + 'expression:  {}\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_GUID_OP:
                    if type(Root.Data
                            ) == IfrLabel:  # type(Info) == EFI_IFR_GUID_LABEL
                        f.write(KeyIndent + '- label:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))

                        f.write(ValueIndent + 'number:  {}  # Number\n'.format(
                            '0x%x' % (Info.Number)))

                    if type(Root.Data) == IfrBanner:
                        f.write(KeyIndent + '- banner:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))

                        f.write(ValueIndent + 'title:  {}\n'.format(Info.Title))
                        f.write(ValueIndent + 'line:  {}\n'.format(
                            Info.LineNumber))
                        f.write(ValueIndent + 'align:  {}\n'.format(
                            Info.Alignment))

                    if type(Root.Data) == IfrTimeout:
                        f.write(KeyIndent + '- banner:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))

                        f.write(ValueIndent + 'timeout:  {}\n'.format(
                            Info.TimeOut))

                    if type(Root.Data) == IfrClass:
                        ValueIndent = ' ' * ((Root.Level-1) * 2 + 1)
                        f.write(ValueIndent + 'class:  {}\n'.format(Info.Class))

                    if type(Root.Data) == IfrSubClass:
                        ValueIndent = ' ' * ((Root.Level-1) * 2 + 1)
                        f.write(ValueIndent + 'subclass:  {}\n'.format(Info.SubClass))

                    if type(Root.Data) == IfrExtensionGuid:
                        if type(Root.Parent.Data) == IfrExtensionGuid:
                            Root.Level -= 1
                            KeyIndent = ' ' *((Root.Level *2 - 1) * 2)
                            ValueIndent = ' ' * ((Root.Level*2 + 1) * 2)
                        f.write(KeyIndent + '- guidop:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                        f.write(ValueIndent + 'guid:  \'{' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                        '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\'\n')
                        if Root.Data.GetDataType() != '':
                            f.write(ValueIndent + 'databuffer: #optional input\n')
                            f.write(ValueIndent + '- {}: \n'.format(Root.Data.GetDataType()))
                            for data in Root.Data.GetFieldList():
                                f.write(ValueIndent + '  {}:  {}\n'.format(data[0], '0x%x' %(data[1])))


                if Root.OpCode == EFI_IFR_INCONSISTENT_IF_OP:
                    if type(Root.Data) == IfrInconsistentIf2:  #
                        f.write(KeyIndent + '- inconsistentif:\n')
                        if Root.Condition != None:
                            f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                        f.write(ValueIndent + 'prompt:  {} # STRING_ID\n'.format(
                            '0x%04x' % (Info.Error)))
                        f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                                Root.Expression))

                if Root.OpCode == EFI_IFR_NO_SUBMIT_IF_OP:
                    f.write(KeyIndent + '- nosubmitif:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'prompt:  {} # STRING_ID\n'.format(
                        '0x%04x' % (Info.Error)))
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_READ_OP:
                    f.write(KeyIndent + '- read:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_WRITE_OP:
                    f.write(KeyIndent + '- write:\n')
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_VALUE_OP and Root.Parent.OpCode != EFI_IFR_DEFAULT_OP:  #
                    f.write(KeyIndent + '- value:\n')
                    f.write(ValueIndent + 'expression:  \'{}\'\n'.format(
                        Root.Expression))

                if Root.OpCode == EFI_IFR_MODAL_TAG_OP:
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(KeyIndent + '- modal: tag\n')

                if Root.OpCode == EFI_IFR_LOCKED_OP:
                    if Root.Condition != None:
                        f.write(ValueIndent + 'condition:  \'{}\'\n'.format(
                                Root.Condition))
                    f.write(KeyIndent + '- locked: tag\n')

            self.LastOp = Root.OpCode

        except:
            EdkLogger.error(
                "VfrCompiler", FILE_WRITE_FAILURE, "File write failed for %s" %
                (self.Options.YamlFileName), None)

        if Root.Child != []:
            for ChildNode in Root.Child:
                if Root.OpCode in ConditionOps:
                    if ChildNode.OpCode in ConditionOps:
                        ChildNode.Condition = Root.Condition + ' | ' + ChildNode.Condition
                    else:
                        ChildNode.Condition = Root.Condition

                    if type(ChildNode.Data) == IfrInconsistentIf2:
                        ChildNode.Level = Root.Level + 1
                    else:
                        ChildNode.Level = Root.Level
                else:
                    if type(Root.Data) == IfrGuid and (ChildNode.OpCode in [EFI_IFR_CHECKBOX_OP, EFI_IFR_NUMERIC_OP, EFI_IFR_ONE_OF_OP]):
                        ChildNode.Level = Root.Level
                    else:
                        ChildNode.Level = Root.Level + 1


                self.DumpYamlDfs(ChildNode, f)

        return
