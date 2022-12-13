from distutils.filelist import FileList
from pickletools import uint8
import sys
import yaml
from tkinter.ttk import Treeview
from antlr4 import *
from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *


class VfrCompiler():

    def __init__(self, InputFile):
        self.__Root = VfrTreeNode()
        self.__InputFile = InputFile
        self.__VfrTree = VfrTree(self.__Root)

    def PreProcess(self):
        gVfrErrorHandle.SetInputFile(self.__InputFile)

    def Compile(self):
        self.__Visitor = VfrSyntaxVisitor(self.__Root)
        InputStream = FileStream(self.__InputFile)
        Lexer = VfrSyntaxLexer(InputStream)
        Stream = CommonTokenStream(Lexer)
        Parser = VfrSyntaxParser(Stream)
        self.__Visitor.visit(Parser.vfrProgram())
        if gFormPkg.HavePendingUnassigned() == True:
            gFormPkg.PendingAssignPrintAll()


    def GenFiles(self):
        self.__VfrTree.DumpYaml(self.__InputFile)
        self.__VfrTree.DumpJson(self.__InputFile)
        self.__VfrTree.GenBinary(self.__InputFile)
        self.__VfrTree.GenCFile(self.__InputFile)
        self.__VfrTree.GenRecordListFile(self.__InputFile)
        # self.__VfrTree.GenBinaryFiles(self.__InputFile)


class YamlCompiler():
    def __init__(self, InputFile):
        self.__Root = VfrTreeNode()
        self.__InputFile = InputFile
        self.__VfrTree = VfrTree(self.__Root)

    def PreProcess(self):
        pass

    def Compile(self):
        self.__VfrTree.ReadYaml(self.__InputFile)



if __name__ == '__main__':
    InputFile = 'test.i'

    VCompiler = VfrCompiler(InputFile)
    VCompiler.PreProcess()
    VCompiler.Compile()
    VCompiler.GenFiles()

    InputFile = 'source.yaml'
    YCompiler = YamlCompiler(InputFile)
    YCompiler.PreProcess()
    YCompiler.Compile()
