from distutils.filelist import FileList
from pickletools import uint8
import sys
from tkinter.ttk import Treeview
from antlr4 import*
from VfrSyntaxLexer import VfrSyntaxLexer
from VfrSyntaxParser import VfrSyntaxParser
from VfrSyntaxVisitor import VfrSyntaxVisitor


def VfrParse(Infile, Outfile):
    InputStream = FileStream(Infile)
    Lexer = VfrSyntaxLexer(InputStream)
    Stream = CommonTokenStream(Lexer)
    Parser = VfrSyntaxParser(Stream)
    Tree = Parser.vfrProgram()
    Visitor = VfrSyntaxVisitor()
    Visitor.visit(Tree)
    Visitor.DumpYaml(Visitor.GetRoot(), Outfile)
    
if __name__ == '__main__':
    Infile = "test\PlatformForms.i"
    Outfile = 'test\PlatformFormsdemo.yaml'
    VfrParse(Infile, Outfile)
   