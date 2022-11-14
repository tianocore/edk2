from CommonCtypes import *
from VfrFormPkg import *
from antlr4 import*
# Ifr related Info -> ctypes obj
#　conditional Info
# Structure Info

class VfrTreeNode():
    def __init__(self, Opcode: int=None) -> None:

        self.OpCode = Opcode
        self.Data = None
        self.Buffer = None
        self.Condition = None
        self.Expression = None
        self.Parent = None
        self.Child = []


    def hasCondition(self) ->bool:
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


    def insertChild(self, NewNode, pos: int=None) -> None:
        if len(self.Child) == 0:
            self.Child.append(NewNode)
        else:
            if not pos:
                LastTree = self.Child[-1]
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

class VfrTree():
    def __init__(self, Root) -> None:
        self.__Root = Root
       # self.__

    def GenBinary(self, FileName):
        try:
            with open(FileName, 'wb') as f:
                self.GenBinaryDfs(self.__Root, f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE, "File open failed for %s" % FileName, None)

    def GenBinaryDfs(self, Root, f):
        if Root.OpCode != None:
            print(type(Root.Data))
            Buffer = self.__StructToStream(Root.Data.GetInfo())

            if Root.Buffer != None:
                f.write(Root.Buffer)
           # else:
                #f.write(Buffer)

        if Root.Child != []:
            for ChildNode in Root.Child:
                self.GenBinaryDfs(ChildNode, f)

    # Get data from ctypes to bytes.
    def __StructToStream(self, s) -> bytes:
        Length = sizeof(s)
        P = cast(pointer(s), POINTER(c_char * Length))
        return P.contents.raw


    def DumpYaml(self, FileName):
        try:
            with open(FileName, 'w') as f:
                f.write('## DO NOT REMOVE -- VFR Mode\n')
                self.DumpYamlDfs(self.__Root, f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE, "File open failed for %s" % FileName, None)


    def DumpYamlDfs(self, Root, f):

        if Root.OpCode != None:
            if Root.OpCode == EFI_IFR_FORM_SET_OP:
                Info = Root.Data.GetInfo()
                f.write('Formset:\n')
                f.write('  Guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')
                f.write('  Title:  {}  # Title STRING_ID\n'.format(Info.FormSetTitle))
                f.write('  Help:  {}  # Help STRING_ID\n'.format(Info.Help))
                for Guid in Root.Data.GetClassGuid():
                    f.write('  ClassGuid:  {' + '{}, {}, {},'.format('0x%x'%(Guid.Data1),'0x%x'%(Guid.Data2), '0x%x'%(Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Guid.Data4[0]), '0x%x'%(Guid.Data4[1]), '0x%x'%(Guid.Data4[2]), '0x%x'%(Guid.Data4[3]), \
                    '0x%x'%(Guid.Data4[4]), '0x%x'%(Guid.Data4[5]), '0x%x'%(Guid.Data4[6]), '0x%x'%(Guid.Data4[7])) + ' }}\n')

            if Root.OpCode == EFI_IFR_VARSTORE_OP:
                Info = Root.Data.GetInfo()
                f.write('  - varstore:\n')
                f.write('      varid:  {}\n'.format(Info.VarStoreId))
                f.write('      name:  {}\n'.format(Info.Name))
                f.write('      size:  {}\n'.format(Info.Size))
                f.write('      guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')

            if Root.OpCode == EFI_IFR_VARSTORE_EFI_OP:
                Info = Root.Data.GetInfo()
                f.write('  - efivarstore:\n')
                f.write('      varid:  {}\n'.format(Info.VarStoreId))
                f.write('      name:  {}\n'.format(Info.Name))
                f.write('      size:  {}\n'.format(Info.Size))
                f.write('      guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')
                f.write('      attribute:  {}\n'.format(Info.Attributes))

            if Root.OpCode == EFI_IFR_VARSTORE_NAME_VALUE_OP:
                Info = Root.Data.GetInfo()
                f.write('  - namevaluevarstore:\n')
                f.write('      varid:  {}\n'.format(Info.VarStoreId))
                # f.write('      name:  {}\n'.format(Info.Name))
                f.write('      guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')

            if Root.OpCode == EFI_IFR_FORM_OP:
                Info = Root.Data.GetInfo()
                f.write('  - form:\n')
                if Root.Condition != None:
                    f.write('      condition:  {}\n'.format(Root.Condition))
                f.write('      FormId:  {}  # FormId STRING_ID\n'.format(Info.FormId))
                f.write('      FormTitle:  {}  # FormTitle STRING_ID\n'.format(Info.FormTitle))

            if Root.OpCode == EFI_IFR_FORM_MAP_OP:
                Info, MethodMapList = Root.Data.GetInfo()
                f.write('  - formmap:\n')
                if Root.Condition != None:
                    f.write('      condition:  {}\n'.format(Root.Condition))
                f.write('      FormId:  {}  # FormId STRING_ID\n'.format(Info.FormId))
                for MethodMap in  MethodMapList:
                    f.write('      maptitle:  {}\n'.format(MethodMap.MethodTitle))
                    f.write('      mapguid:  {' + '{}, {}, {},'.format('0x%x'%(MethodMap.MethodIdentifier.Data1),'0x%x'%(MethodMap.MethodIdentifier.Data2), '0x%x'%(MethodMap.MethodIdentifier.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(MethodMap.MethodIdentifier.Data4[0]), '0x%x'%(MethodMap.MethodIdentifier.Data4[1]), '0x%x'%(MethodMap.MethodIdentifier.Data4[2]), '0x%x'%(MethodMap.MethodIdentifier.Data4[3]), \
                    '0x%x'%(MethodMap.MethodIdentifier.Data4[4]), '0x%x'%(MethodMap.MethodIdentifier.Data4[5]), '0x%x'%(MethodMap.MethodIdentifier.Data4[6]), '0x%x'%(MethodMap.MethodIdentifier.Data4[7])) + ' }}\n')

            if Root.OpCode == EFI_IFR_IMAGE_OP:
                Info = Root.Data.GetInfo()
                f.write('      - image:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          Id:  {} # ImageId\n'.format(Info.Id))

            if Root.OpCode == EFI_IFR_RULE_OP:
                Info = Root.Data.GetInfo()
                f.write('      - rule:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          RuleId:  {} # RuleId\n'.format(Info.RuleId))

            if Root.OpCode == EFI_IFR_SUBTITLE_OP:
                Info = Root.Data.GetInfo()
                f.write('      - subtitle:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Statement.Prompt))

            if Root.OpCode == EFI_IFR_TEXT_OP:
                Info = Root.Data.GetInfo()
                f.write('      - text:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Statement.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Statement.Help))
                f.write('          text:  {}  # Statement Help STRING_ID\n'.format(Info.TextTwo))

            if Root.OpCode == EFI_IFR_ACTION_OP:
                Info = Root.Data.GetInfo()
                f.write('      - action:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags))
                f.write('          questionconfig:  {}  # QuestionConfig\n'.format(Info.QuestionConfig))

            if Root.OpCode == EFI_IFR_ONE_OF_OP:
                Info = Root.Data.GetInfo()
                f.write('      - oneof:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))

                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags))

            if Root.OpCode == EFI_IFR_ONE_OF_OPTION_OP:
                Info = Root.Data.GetInfo()
                f.write('          - option:  {}\n'.format(Info.Option))
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))

                f.write('              option flag:  {}\n'.format(Info.Flags))
                f.write('              option type:  {}\n'.format(Info.Type))

                if type(Root.Data) == CIfrOneOfOption:
                    if Info.Type == EFI_IFR_TYPE_DATE:
                        f.write('              option value:  {}/{}/{}\n'.format(Info.Value.date.Year, Info.Value.date.Month, Info.Value.date.Day))
                    if Info.Type == EFI_IFR_TYPE_TIME:
                        f.write('              option value:  {}:{}:{}\n'.format(Info.Value.time.Hour, Info.Value.time.Minute, Info.Value.time.Second))
                    if Info.Type == EFI_IFR_TYPE_REF:
                        f.write('              option value:  {};{};'.format(Info.Value.ref.QuestionId, Info.Value.ref.FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data1),'0x%x'%(Info.Value.ref.FormSetGuid.Data2), '0x%x'%(Info.Value.ref.FormSetGuid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data4[0]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[1]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[2]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[3]), \
                        '0x%x'%(Info.Value.ref.FormSetGuid.Data4[4]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[5]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[6]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[7])) + ' }}' + ';{}\n'.format(Info.Value.ref.DevicePath))
                    if Info.Type == EFI_IFR_TYPE_STRING:
                        f.write('              option value:  {}\n'.format(Info.Value.string))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_8:
                        f.write('              option value:  {}\n'.format(Info.Value.u8))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_16:
                        f.write('              option value:  {}\n'.format(Info.Value.u16))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_32:
                        f.write('              option value:  {}\n'.format(Info.Value.u32))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_64:
                        f.write('              option value:  {}\n'.format(Info.Value.u64))
                    if Info.Type == EFI_IFR_TYPE_BOOLEAN:
                        f.write('              option value:  {}\n'.format(Info.Value.b))

                if type(Root.Data) == CIfrOneOfOption2:
                    f.write('              value:  {')
                    ValueType = Root.Data.GetValueType()
                    if ValueType == EFI_IFR_TYPE_STRING:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].string))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].string) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_8:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u8))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u8) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_16:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u16))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u16) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_32:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u32))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u32) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_64:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u64))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u64) + '}\n')

                    if ValueType == EFI_IFR_TYPE_BOOLEAN:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].b))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].b) + '}\n')


            if Root.OpCode == EFI_IFR_DEFAULT_OP:
                Info = Root.Data.GetInfo()
                f.write('          - default:\n')
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                f.write('              type:  {}\n'.format(Info.Type))
                f.write('              defaultId:  {}\n'.format(Info.DefaultId))
                if type(Root.Data) == CIfrDefault:
                    if Info.Type == EFI_IFR_TYPE_DATE:
                        f.write('              value:  {}/{}/{}\n'.format(Info.Value.date.Year, Info.Value.date.Month, Info.Value.date.Day))
                    if Info.Type == EFI_IFR_TYPE_TIME:
                        f.write('              value:  {}:{}:{}\n'.format(Info.Value.time.Hour, Info.Value.time.Minute, Info.Value.time.Second))
                    if Info.Type == EFI_IFR_TYPE_REF:
                        f.write('              option value:  {};{};'.format(Info.Value.ref.QuestionId, Info.Value.ref.FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data1),'0x%x'%(Info.Value.ref.FormSetGuid.Data2), '0x%x'%(Info.Value.ref.FormSetGuid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Value.ref.FormSetGuid.Data4[0]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[1]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[2]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[3]), \
                        '0x%x'%(Info.Value.ref.FormSetGuid.Data4[4]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[5]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[6]), '0x%x'%(Info.Value.ref.FormSetGuid.Data4[7])) + ' }}' + ';{}\n'.format(Info.Value.ref.DevicePath))
                    if Info.Type == EFI_IFR_TYPE_STRING:
                        f.write('              value:  {}\n'.format(Info.Value.string))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_8:
                        f.write('              value:  {}\n'.format(Info.Value.u8))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_16:
                        f.write('              value:  {}\n'.format(Info.Value.u16))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_32:
                        f.write('              value:  {}\n'.format(Info.Value.u32))
                    if Info.Type == EFI_IFR_TYPE_NUM_SIZE_64:
                        f.write('              value:  {}\n'.format(Info.Value.u64))
                    if Info.Type == EFI_IFR_TYPE_BOOLEAN:
                        f.write('              value:  {}\n'.format(Info.Value.b))

                if type(Root.Data) == CIfrDefault3:
                    f.write('              value:  {')
                    ValueType = Root.Data.GetValueType()
                    if ValueType == EFI_IFR_TYPE_STRING:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].string))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].string) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_8:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u8))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u8) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_16:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u16))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u16) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_32:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u32))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u32) + '}\n')

                    if ValueType == EFI_IFR_TYPE_NUM_SIZE_64:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].u64))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].u64) + '}\n')

                    if ValueType == EFI_IFR_TYPE_BOOLEAN:
                        for i in range(0, len(Info.Value)-1):
                            f.write('{},'.format(Info.Value[i].b))
                        f.write('{}'.format(Info.Value[len(Info.Value)-1].b) + '}\n')

            if Root.OpCode == EFI_IFR_ORDERED_LIST_OP:
                Info = Root.Data.GetInfo()
                f.write('      - orderedlist:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))

                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          maxContainers:  {}\n'.format(Info.MaxContainers))
                f.write('          flags:  {}\n'.format(Info.Question.Flags))

            if Root.OpCode == EFI_IFR_NUMERIC_OP:
                Info = Root.Data.GetInfo()
                f.write('      - numeric:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags))

                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_64:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u64.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u64.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u64.Step))

                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_32:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u32.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u32.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u32.Step))

                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_16:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u16.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u16.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u16.Step))

                if Root.Data.GetVarType() == EFI_IFR_TYPE_NUM_SIZE_8:
                    f.write('          maxvalue:  {}\n'.format(Info.Data.u8.MaxValue))
                    f.write('          minvalue:  {}\n'.format(Info.Data.u8.MinValue))
                    f.write('          step:  {}\n'.format(Info.Data.u8.Step))

            if Root.OpCode == EFI_IFR_CHECKBOX_OP:
                Info = Root.Data.GetInfo()
                f.write('      - checkbox:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Flags\n'.format(Info.Flags))

            if Root.OpCode == EFI_IFR_TIME_OP:
                Info = Root.Data.GetInfo()
                f.write('      - time:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          questionid:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          flags:  {}\n'.format(Info.Flags))

            if Root.OpCode == EFI_IFR_DATE_OP:
                Info = Root.Data.GetInfo()
                f.write('      - date:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          questionid:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          flags:  {}\n'.format(Info.Flags))


            if Root.OpCode == EFI_IFR_STRING_OP:
                Info = Root.Data.GetInfo()
                f.write('      - string:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags))
                f.write('          stringflags:  {}\n'.format(Info.Flags))
                f.write('          stringminsize:  {}\n'.format(Info.MinSize))
                f.write('          stringmaxsize:  {}\n'.format(Info.MaxSize))

            if Root.OpCode == EFI_IFR_PASSWORD_OP:
                Info = Root.Data.GetInfo()
                f.write('      - password:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Question.Header.Help))
                f.write('          questionid:  {}  # Question QuestionId\n'.format(Info.Question.QuestionId))
                f.write('          varstoreid:  {}  # Question VarStoreId\n'.format(Info.Question.VarStoreId))
                f.write('          varname:  {}  # Question VarName STRING_ID\n'.format(Info.Question.VarStoreInfo.VarName))
                f.write('          varoffset:  {}  # Question VarOffset\n'.format(Info.Question.VarStoreInfo.VarOffset))
                f.write('          flags:  {}  # Question Flags\n'.format(Info.Question.Flags))
                f.write('          minsize:  {}\n'.format(Info.MinSize))
                f.write('          maxsize:  {}\n'.format(Info.MaxSize))


            if Root.OpCode == EFI_IFR_RESET_BUTTON_OP:
                Info = Root.Data.GetInfo()
                f.write('      - resetbutton:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))
                f.write('          prompt:  {}  # Statement Prompt STRING_ID\n'.format(Info.Statement.Prompt))
                f.write('          help:  {}  # Statement Help STRING_ID\n'.format(Info.Statement.Help))
                f.write('          defaultid:  {}\n'.format(Info.DefaultId))

            if Root.OpCode == EFI_IFR_REF_OP:
                Info = Root.Data.GetInfo()
                f.write('      - goto:\n')
                if Root.Condition != None:
                    f.write('          condition:  {}\n'.format(Root.Condition))

                if type(Root.Data) == CIfrRef4:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          formsetid:  {' + '{}, {}, {},'.format('0x%x'%(Info.FormSetId.Data1),'0x%x'%(Info.FormSetId.Data2), '0x%x'%(Info.FormSetId.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.FormSetId.Data4[0]), '0x%x'%(Info.FormSetId.Data4[1]), '0x%x'%(Info.FormSetId.Data4[2]), '0x%x'%(Info.FormSetId.Data4[3]), \
                    '0x%x'%(Info.FormSetId.Data4[4]), '0x%x'%(Info.FormSetId.Data4[5]), '0x%x'%(Info.FormSetId.Data4[6]), '0x%x'%(Info.FormSetId.Data4[7])) + ' }}\n')
                    f.write('          questionid:  {}\n'.format(Info.QuestionId))
                    f.write('          devicepath:  {}\n'.format(Info.DevicePath))

                if type(Root.Data) == CIfrRef3:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          formsetid:  {' + '{}, {}, {},'.format('0x%x'%(Info.FormSetId.Data1),'0x%x'%(Info.FormSetId.Data2), '0x%x'%(Info.FormSetId.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.FormSetId.Data4[0]), '0x%x'%(Info.FormSetId.Data4[1]), '0x%x'%(Info.FormSetId.Data4[2]), '0x%x'%(Info.FormSetId.Data4[3]), \
                    '0x%x'%(Info.FormSetId.Data4[4]), '0x%x'%(Info.FormSetId.Data4[5]), '0x%x'%(Info.FormSetId.Data4[6]), '0x%x'%(Info.FormSetId.Data4[7])) + ' }}\n')
                    f.write('          questionid:  {}\n'.format(Info.QuestionId))

                if type(Root.Data) == CIfrRef2:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          questionid:  {}\n'.format(Info.QuestionId))

                if type(Root.Data) == CIfrRef:
                    f.write('          formid:  {}\n'.format(Info.FormId))
                    f.write('          questionid:  {}\n'.format(Info.Question.QuestionId))

                f.write('          prompt:  {}\n'.format(Info.Question.Header.Prompt))
                f.write('          help:  {}\n'.format(Info.Question.Header.Help))

            if Root.OpCode == EFI_IFR_REFRESH_OP:
                Info = Root.Data.GetInfo()
                f.write('          - refresh:\n')
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                f.write('              interval:  {}  # RefreshInterval\n'.format(Info.RefreshInterval))

            if Root.OpCode == EFI_IFR_VARSTORE_DEVICE_OP:
                Info = Root.Data.GetInfo()
                f.write('          - varstoredevice:\n')
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                f.write('              devicepath:  {}  # DevicePath\n'.format(Info.DevicePath))

            if Root.OpCode == EFI_IFR_REFRESH_ID_OP:
                Info = Root.Data.GetInfo()
                f.write('          - refreshguid:\n')
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                f.write('              eventgroupid:  {' + '{}, {}, {},'.format('0x%x'%(Info.RefreshEventGroupId.Data1),'0x%x'%(Info.RefreshEventGroupId.Data2), '0x%x'%(Info.RefreshEventGroupId.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.RefreshEventGroupId.Data4[0]), '0x%x'%(Info.RefreshEventGroupId.Data4[1]), '0x%x'%(Info.RefreshEventGroupId.Data4[2]), '0x%x'%(Info.RefreshEventGroupId.Data4[3]), \
                    '0x%x'%(Info.RefreshEventGroupId.Data4[4]), '0x%x'%(Info.RefreshEventGroupId.Data4[5]), '0x%x'%(Info.RefreshEventGroupId.Data4[6]), '0x%x'%(Info.RefreshEventGroupId.Data4[7])) + ' }}\n')

            if Root.OpCode == EFI_IFR_WARNING_IF_OP:
                Info = Root.Data.GetInfo()
                f.write('          - warningif:\n')
                if Root.Condition != None:
                    f.write('              condition:  {}\n'.format(Root.Condition))
                f.write('              warning:  {}\n'.format(Info.Warning))
                f.write('              timeOut:  {}\n'.format(Info.TimeOut))

            if Root.OpCode == EFI_IFR_GUID_OP:
                Info = Root.Data.GetInfo()
                if type(Root.Data) == CIfrLabel: # type(Info) == EFI_IFR_GUID_LABEL
                    f.write('      - label:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))

                    f.write('          labelnumber:  {}  # LabelNumber\n'.format(Info.Number))

                if type(Root.Data) == CIfrBanner:
                    f.write('      - banner:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))

                    f.write('          title:  {}\n'.format(Info.Title))
                    f.write('          linenumber:  {}\n'.format(Info.LineNumber))
                    f.write('          align:  {}\n'.format(Info.Alignment))

                if type(Root.Data) == CIfrTimeout:
                    f.write('      - banner:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))

                    f.write('          timeout:  {}\n'.format(Info.TimeOut))

                if type(Root.Data) == CIfrClass:
                    f.write('  Class:  {}\n'.format(Info.Class))

                if type(Root.Data) == CIfrSubClass:
                    f.write('  SubClass:  {}\n'.format(Info.SubClass))

                if type(Root.Data) == CIfrGuid:
                    f.write('      - guidop:\n')
                    if Root.Condition != None:
                        f.write('          condition:  {}\n'.format(Root.Condition))
                    f.write('          guid:  {' + '{}, {}, {},'.format('0x%x'%(Info.Guid.Data1),'0x%x'%(Info.Guid.Data2), '0x%x'%(Info.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(Info.Guid.Data4[0]), '0x%x'%(Info.Guid.Data4[1]), '0x%x'%(Info.Guid.Data4[2]), '0x%x'%(Info.Guid.Data4[3]), \
                    '0x%x'%(Info.Guid.Data4[4]), '0x%x'%(Info.Guid.Data4[5]), '0x%x'%(Info.Guid.Data4[6]), '0x%x'%(Info.Guid.Data4[7])) + ' }}\n')


        if Root.Child != []:
            for ChildNode in Root.Child:
                if Root.OpCode in ConditionOps:
                    if ChildNode.OpCode in ConditionOps:
                        ChildNode.Condition = Root.Condition + ' | ' + ChildNode.Condition
                    else:
                        ChildNode.Condition = Root.Condition

                self.DumpYamlDfs(ChildNode, f)

        return

    def DumpJson(self, FileName):
        try:
            with open(FileName, 'w') as f:
                f.write('{\n')
                f.write('  \"DataStruct\" : {\n')
                pNode = gCVfrVarDataTypeDB.GetDataTypeList()
                while pNode != None:
                    f.write('    \"{}\" : [\n'.format(str(pNode.TypeName)))
                    FNode = pNode.Members
                    while FNode != None:
                        f.write('      {\n')
                        f.write('        \"Name\": \"{}\",\n'.format(str(FNode.FieldName)))
                        if FNode.ArrayNum > 0:
                            f.write('        \"Type\": \"{}[{}]\",\n'.format(str(FNode.FieldType.TypeName),str(FNode.ArrayNum)))
                        else:
                            f.write('        \"Type\": \"{}\",\n'.format(str(FNode.FieldType.TypeName)))
                        f.write('        \"Offset\": {}\n'.format(str(FNode.Offset)))
                        if FNode.Next == None:
                            f.write('      }\n')
                        else:
                            f.write('      }, \n')
                        FNode = FNode.Next
                    if pNode.Next == None:
                        f.write('    ]\n')
                    else:
                        f.write('    ],\n')
                    pNode = pNode.Next
                f.write('  },\n')
                f.write('  \"DataStructAttribute\": {\n')
                pNode = gCVfrVarDataTypeDB.GetDataTypeList()
                while pNode != None:
                    f.write('    \"{}\"'.format(str(pNode.TypeName)) + ': {\n')
                    f.write('        \"Alignment\": {},\n'.format(str(pNode.Align)))
                    f.write('        \"TotalSize\": {}\n'.format(str(pNode.TotalSize)))
                    if pNode.Next == None:
                        f.write('      }\n')
                    else:
                        f.write('      },\n')
                    pNode = pNode.Next
                f.write('  },\n')
                f.write('  \"VarDefine\" : {\n')
                pVsNode = gCVfrDataStorage.GetBufferVarStoreList()
                while pVsNode != None:
                    f.write('    \"{}\"'.format(str(pVsNode.VarStoreName)) + ': {\n')
                    f.write('        \"Type\": \"{}\",\n'.format(str(pVsNode.DataType.TypeName)))
                    f.write('        \"Attributes\": {},\n'.format(str(pVsNode.Attributes)))
                    f.write('        \"VarStoreId\": {},\n'.format(str(pVsNode.VarStoreId)))
                    f.write('        \"VendorGuid\": ' + '\"{}, {}, {},'.format('0x%x'%(pVsNode.Guid.Data1),'0x%x'%(pVsNode.Guid.Data2), '0x%x'%(pVsNode.Guid.Data3)) \
                    + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(pVsNode.Guid.Data4[0]), '0x%x'%(pVsNode.Guid.Data4[1]), '0x%x'%(pVsNode.Guid.Data4[2]), '0x%x'%(pVsNode.Guid.Data4[3]), \
                    '0x%x'%(pVsNode.Guid.Data4[4]), '0x%x'%(pVsNode.Guid.Data4[5]), '0x%x'%(pVsNode.Guid.Data4[6]), '0x%x'%(pVsNode.Guid.Data4[7])) + ' }}\"\n')
                    if pVsNode.Next == None:
                        f.write('      }\n')
                    else:
                        f.write('      },\n')

                    pVsNode = pVsNode.Next
                f.write('  },\n')
                f.write('  \"Data\" : [\n')
                pVsNode = gCVfrBufferConfig.GetVarItemList()
                while pVsNode != None:
                    if pVsNode.Id == None:
                        pVsNode = pVsNode.Next
                        continue
                    pInfoNode = pVsNode.InfoStrList
                    while pInfoNode != None:
                        f.write('      {\n')
                        f.write('        \"VendorGuid\": ' + '\"{}, {}, {},'.format('0x%x'%(pVsNode.Guid.Data1),'0x%x'%(pVsNode.Guid.Data2), '0x%x'%(pVsNode.Guid.Data3)) \
                        + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(pVsNode.Guid.Data4[0]), '0x%x'%(pVsNode.Guid.Data4[1]), '0x%x'%(pVsNode.Guid.Data4[2]), '0x%x'%(pVsNode.Guid.Data4[3]), \
                        '0x%x'%(pVsNode.Guid.Data4[4]), '0x%x'%(pVsNode.Guid.Data4[5]), '0x%x'%(pVsNode.Guid.Data4[6]), '0x%x'%(pVsNode.Guid.Data4[7])) + ' }}\",\n')
                        f.write('        \"VarName\": \"{}\",\n'.format(str(pVsNode.Name)))
                        f.write('        \"DefaultStore\": \"{}\",\n'.format(str(pVsNode.Id)))
                        f.write('        \"Size\": \"{}\",\n'.format(str(pInfoNode.Width)))
                        f.write('        \"Offset\": {},\n'.format(str(pInfoNode.Offset)))
                        #f.write('        \"Value\": \"{}\"\n'.format(str(pInfoNode.Value)))
                        if pInfoNode.Type == EFI_IFR_TYPE_DATE:
                            f.write('        \"Value\": \"{}/{}/{}\"\n'.format(pInfoNode.Value.date.Year, pInfoNode.Value.date.Month, pInfoNode.Value.date.Day))
                        if pInfoNode.Type == EFI_IFR_TYPE_TIME:
                            f.write('        \"Value\": \"{}:{}:{}\"\n'.format(pInfoNode.Value.time.Hour, pInfoNode.Value.time.Minute, pInfoNode.Value.time.Second))
                        if pInfoNode.Type == EFI_IFR_TYPE_REF:
                            f.write('        \"Value\": \"{};{};'.format(pInfoNode.Value.ref.QuestionId, pInfoNode.Value.ref.FormId) +  '{' + '{}, {}, {},'.format('0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data1),'0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data2), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data3)) \
                            + ' { ' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[0]), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[1]), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[2]), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[3]), \
                            '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[4]), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[5]), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[6]), '0x%x'%(pInfoNode.Value.ref.FormSetGuid.Data4[7])) + ' }}' + ';{}\n'.format(pInfoNode.Value.ref.DevicePath))
                        if pInfoNode.Type == EFI_IFR_TYPE_STRING:
                            f.write('        \"Value\": \"{}\"\n'.format(pInfoNode.Value.string))
                        if pInfoNode.Type == EFI_IFR_TYPE_NUM_SIZE_8:
                            f.write('        \"Value\": \"{}\"\n'.format(pInfoNode.Value.u8))
                        if pInfoNode.Type == EFI_IFR_TYPE_NUM_SIZE_16:
                            f.write('        \"Value\": \"{}\"\n'.format(pInfoNode.Value.u16))
                        if pInfoNode.Type == EFI_IFR_TYPE_NUM_SIZE_32:
                            f.write('        \"Value\": \"{}\"\n'.format(pInfoNode.Value.u32))
                        if pInfoNode.Type == EFI_IFR_TYPE_NUM_SIZE_64:
                            f.write('        \"Value\": \"{}\"\n'.format(pInfoNode.Value.u64))
                        if pInfoNode.Type == EFI_IFR_TYPE_BOOLEAN:
                            f.write('        \"Value\": \"{}\"\n'.format(pInfoNode.Value.b))

                        f.write('      },\n')
                        pInfoNode = pInfoNode.Next
                    pVsNode = pVsNode.Next
                f.write('      {\n')
                f.write('        \"VendorGuid\": \"NA\",\n')
                f.write('        \"VarName\": \"NA\",\n')
                f.write('        \"DefaultStore\": \"NA\",\n')
                f.write('        \"Size\": 0,\n')
                f.write('        \"Offset\": 0,\n')
                f.write('        \"Value\": \"0x00\"\n')
                f.write('      }\n')
                f.write('  ]\n')
                f.write('}\n')

            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE, "File open failed for %s" % FileName, None)
