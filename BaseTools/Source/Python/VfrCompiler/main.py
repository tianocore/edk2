from distutils.filelist import FileList
from pickletools import uint8
import sys
from tkinter.ttk import Treeview
from antlr4 import*
from VfrSyntaxLexer import VfrSyntaxLexer
from VfrSyntaxParser import VfrSyntaxParser
from VfrSyntaxVisitor import VfrSyntaxVisitor


def VfrParse(infile, outfile):
    input_stream = FileStream(infile)
    lexer = VfrSyntaxLexer(input_stream)
    stream = CommonTokenStream(lexer)
    parser = VfrSyntaxParser(stream)
    tree = parser.vfrProgram()
    Visitor = VfrSyntaxVisitor()
    Visitor.visit(tree)
    Visitor.DumpYaml(Visitor.GetRoot(), outfile)
    
if __name__ == '__main__':
    infile = "test\\test.i"
    outfile = 'test\demo.yaml'
    VfrParse(infile, outfile)
   