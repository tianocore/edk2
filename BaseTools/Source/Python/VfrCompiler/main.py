from distutils.filelist import FileList
from pickletools import uint8
import sys
from tkinter.ttk import Treeview
from antlr4 import*
from VfrSyntaxLexer import VfrSyntaxLexer
from VfrSyntaxParser import VfrSyntaxParser
from VfrSyntaxVisitor import VfrSyntaxVisitor


def VfrParse(Infile, YamlOutFile,JsonOutFile):
    InputStream = FileStream(Infile)
    Lexer = VfrSyntaxLexer(InputStream)
    Stream = CommonTokenStream(Lexer)
    Parser = VfrSyntaxParser(Stream)
    Tree = Parser.vfrProgram()
    Visitor = VfrSyntaxVisitor()
    Visitor.visit(Tree)
    Visitor.DumpYaml(Visitor.GetRoot(), YamlOutFile)
    Visitor.DumpJson(JsonOutFile)
    
if __name__ == '__main__':
    Infile = "Atest.i"
    YamlOutFile = 'Atest.yaml'
    JsonOutFile = 'Atest.json'
    VfrParse(Infile, YamlOutFile, JsonOutFile)
   