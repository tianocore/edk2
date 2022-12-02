from distutils.filelist import FileList
from pickletools import uint8
import sys
from tkinter.ttk import Treeview
from antlr4 import*
from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *

class VfrCompiler():
    def __init__(self, InputFile) -> None:
        self.__Root = VfrTreeNode()
        self.__InputFile = InputFile
        self.__VfrTree = VfrTree(self.__Root)

    def PreProcess(self):
        gCVfrErrorHandle.SetInputFile(self.__InputFile)

    def Compile(self):
        self.__Visitor = VfrSyntaxVisitor(self.__Root)
        InputStream = FileStream(self.__InputFile)
        Lexer = VfrSyntaxLexer(InputStream)
        Stream = CommonTokenStream(Lexer)
        Parser = VfrSyntaxParser(Stream)
        Tree = Parser.vfrProgram()
        self.__Visitor.visit(Tree)
        self.__VfrTree.DumpYaml(self.__InputFile)
        self.__VfrTree.DumpJson(self.__InputFile)
        self.__VfrTree.GenBinary(self.__InputFile)
        self.__VfrTree.GenCFile(self.__InputFile)
        self.__VfrTree.GenRecordListFile(self.__InputFile)



if __name__ == '__main__':
    InputFile = 'test.i'

    Compiler = VfrCompiler(InputFile)
    Compiler.PreProcess()
    Compiler.Compile()
