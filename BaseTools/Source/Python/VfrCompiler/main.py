from distutils.filelist import FileList
from pickletools import uint8
import sys
from tkinter.ttk import Treeview
from antlr4 import*
from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *

class VfrCompiler():
    def __init__(self) -> None:
        Root = VfrTreeNode()
        self.__Visitor = VfrSyntaxVisitor(Root)
        self.__VfrTree = VfrTree(Root)

    def PreProcess(self):
        pass


    def Compile(self, Infile, YamlOutFile, JsonOutFile, BinaryOutFile, CFile, RecordListFile):
        gCVfrErrorHandle.SetInputFile(Infile)
        InputStream = FileStream(Infile)
        Lexer = VfrSyntaxLexer(InputStream)
        Stream = CommonTokenStream(Lexer)
        Parser = VfrSyntaxParser(Stream)
        tree = Parser.vfrProgram()
        self.__Visitor.visit(tree)
        self.__VfrTree.DumpYaml(YamlOutFile)
        self.__VfrTree.DumpJson(JsonOutFile)
        self.__VfrTree.GenBinary(BinaryOutFile)
        self.__VfrTree.GenCFile(Infile, CFile)
        self.__VfrTree.GenRecordListFile(RecordListFile)

    def AdjustBin(self):
        pass

    def GenBinary(self):
        pass



if __name__ == '__main__':
    Infile = 'test.i'
    YamlOutFile = 'test.yaml'
    JsonOutFile = 'test.json'
    BinaryOutFile = 'test.hpk'
    CFile = 'test.c'
    RecordListFile = 'test.lst'
    Compiler = VfrCompiler()
    Compiler.PreProcess()
    Compiler.Compile(Infile, YamlOutFile, JsonOutFile, BinaryOutFile, CFile, RecordListFile)
