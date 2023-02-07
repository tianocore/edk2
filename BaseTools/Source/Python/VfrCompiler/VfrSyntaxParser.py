# Generated from VfrSyntax.g4 by ANTLR 4.7.2
# encoding: utf-8
from antlr4 import *
from io import StringIO
from typing.io import TextIO
import sys



from VfrCtypes import *
from VfrFormPkg import *
from VfrUtility import *
from VfrTree import *


def serializedATN():
    with StringIO() as buf:
        buf.write("\3\u608b\ua72a\u8133\ub9ed\u417c\u3be7\u7786\u5964\3\u0102")
        buf.write("\u0ba7\4\2\t\2\4\3\t\3\4\4\t\4\4\5\t\5\4\6\t\6\4\7\t\7")
        buf.write("\4\b\t\b\4\t\t\t\4\n\t\n\4\13\t\13\4\f\t\f\4\r\t\r\4\16")
        buf.write("\t\16\4\17\t\17\4\20\t\20\4\21\t\21\4\22\t\22\4\23\t\23")
        buf.write("\4\24\t\24\4\25\t\25\4\26\t\26\4\27\t\27\4\30\t\30\4\31")
        buf.write("\t\31\4\32\t\32\4\33\t\33\4\34\t\34\4\35\t\35\4\36\t\36")
        buf.write("\4\37\t\37\4 \t \4!\t!\4\"\t\"\4#\t#\4$\t$\4%\t%\4&\t")
        buf.write("&\4\'\t\'\4(\t(\4)\t)\4*\t*\4+\t+\4,\t,\4-\t-\4.\t.\4")
        buf.write("/\t/\4\60\t\60\4\61\t\61\4\62\t\62\4\63\t\63\4\64\t\64")
        buf.write("\4\65\t\65\4\66\t\66\4\67\t\67\48\t8\49\t9\4:\t:\4;\t")
        buf.write(";\4<\t<\4=\t=\4>\t>\4?\t?\4@\t@\4A\tA\4B\tB\4C\tC\4D\t")
        buf.write("D\4E\tE\4F\tF\4G\tG\4H\tH\4I\tI\4J\tJ\4K\tK\4L\tL\4M\t")
        buf.write("M\4N\tN\4O\tO\4P\tP\4Q\tQ\4R\tR\4S\tS\4T\tT\4U\tU\4V\t")
        buf.write("V\4W\tW\4X\tX\4Y\tY\4Z\tZ\4[\t[\4\\\t\\\4]\t]\4^\t^\4")
        buf.write("_\t_\4`\t`\4a\ta\4b\tb\4c\tc\4d\td\4e\te\4f\tf\4g\tg\4")
        buf.write("h\th\4i\ti\4j\tj\4k\tk\4l\tl\4m\tm\4n\tn\4o\to\4p\tp\4")
        buf.write("q\tq\4r\tr\4s\ts\4t\tt\4u\tu\4v\tv\4w\tw\4x\tx\4y\ty\4")
        buf.write("z\tz\4{\t{\4|\t|\4}\t}\4~\t~\4\177\t\177\4\u0080\t\u0080")
        buf.write("\4\u0081\t\u0081\4\u0082\t\u0082\4\u0083\t\u0083\4\u0084")
        buf.write("\t\u0084\4\u0085\t\u0085\4\u0086\t\u0086\4\u0087\t\u0087")
        buf.write("\4\u0088\t\u0088\4\u0089\t\u0089\4\u008a\t\u008a\4\u008b")
        buf.write("\t\u008b\4\u008c\t\u008c\4\u008d\t\u008d\4\u008e\t\u008e")
        buf.write("\4\u008f\t\u008f\4\u0090\t\u0090\4\u0091\t\u0091\4\u0092")
        buf.write("\t\u0092\4\u0093\t\u0093\4\u0094\t\u0094\4\u0095\t\u0095")
        buf.write("\4\u0096\t\u0096\4\u0097\t\u0097\4\u0098\t\u0098\4\u0099")
        buf.write("\t\u0099\4\u009a\t\u009a\4\u009b\t\u009b\4\u009c\t\u009c")
        buf.write("\4\u009d\t\u009d\4\u009e\t\u009e\4\u009f\t\u009f\4\u00a0")
        buf.write("\t\u00a0\4\u00a1\t\u00a1\4\u00a2\t\u00a2\4\u00a3\t\u00a3")
        buf.write("\4\u00a4\t\u00a4\4\u00a5\t\u00a5\4\u00a6\t\u00a6\4\u00a7")
        buf.write("\t\u00a7\4\u00a8\t\u00a8\4\u00a9\t\u00a9\4\u00aa\t\u00aa")
        buf.write("\4\u00ab\t\u00ab\4\u00ac\t\u00ac\4\u00ad\t\u00ad\4\u00ae")
        buf.write("\t\u00ae\4\u00af\t\u00af\4\u00b0\t\u00b0\4\u00b1\t\u00b1")
        buf.write("\4\u00b2\t\u00b2\4\u00b3\t\u00b3\4\u00b4\t\u00b4\4\u00b5")
        buf.write("\t\u00b5\4\u00b6\t\u00b6\4\u00b7\t\u00b7\4\u00b8\t\u00b8")
        buf.write("\4\u00b9\t\u00b9\4\u00ba\t\u00ba\4\u00bb\t\u00bb\4\u00bc")
        buf.write("\t\u00bc\4\u00bd\t\u00bd\4\u00be\t\u00be\4\u00bf\t\u00bf")
        buf.write("\4\u00c0\t\u00c0\4\u00c1\t\u00c1\4\u00c2\t\u00c2\4\u00c3")
        buf.write("\t\u00c3\4\u00c4\t\u00c4\4\u00c5\t\u00c5\4\u00c6\t\u00c6")
        buf.write("\4\u00c7\t\u00c7\4\u00c8\t\u00c8\4\u00c9\t\u00c9\4\u00ca")
        buf.write("\t\u00ca\4\u00cb\t\u00cb\4\u00cc\t\u00cc\3\2\3\2\3\2\7")
        buf.write("\2\u019c\n\2\f\2\16\2\u019f\13\2\3\2\5\2\u01a2\n\2\3\3")
        buf.write("\3\3\3\4\3\4\3\4\5\4\u01a9\n\4\3\4\3\4\5\4\u01ad\n\4\3")
        buf.write("\5\5\5\u01b0\n\5\3\6\3\6\3\6\3\6\3\6\3\6\5\6\u01b8\n\6")
        buf.write("\3\6\3\6\3\7\5\7\u01bd\n\7\3\7\3\7\5\7\u01c1\n\7\3\7\5")
        buf.write("\7\u01c4\n\7\3\7\3\7\3\7\3\7\5\7\u01ca\n\7\3\7\3\7\3\b")
        buf.write("\5\b\u01cf\n\b\3\b\3\b\5\b\u01d3\n\b\3\b\5\b\u01d6\n\b")
        buf.write("\3\b\3\b\3\b\3\b\5\b\u01dc\n\b\3\b\3\b\3\t\3\t\3\t\3\t")
        buf.write("\3\t\3\t\3\t\3\t\3\t\3\t\3\t\3\t\3\t\3\t\7\t\u01ee\n\t")
        buf.write("\f\t\16\t\u01f1\13\t\3\n\3\n\3\n\3\n\3\n\5\n\u01f8\n\n")
        buf.write("\3\n\3\n\3\13\3\13\3\13\3\13\3\13\5\13\u0201\n\13\3\13")
        buf.write("\3\13\3\f\3\f\3\f\3\f\3\f\5\f\u020a\n\f\3\f\3\f\3\r\3")
        buf.write("\r\3\r\3\r\3\r\5\r\u0213\n\r\3\r\3\r\3\16\3\16\3\16\3")
        buf.write("\16\3\16\5\16\u021c\n\16\3\16\3\16\3\17\3\17\3\17\3\17")
        buf.write("\3\17\5\17\u0225\n\17\3\17\3\17\3\20\3\20\3\20\3\20\3")
        buf.write("\20\5\20\u022e\n\20\3\20\3\20\3\21\3\21\3\21\3\21\3\21")
        buf.write("\5\21\u0237\n\21\3\21\3\21\3\22\3\22\3\22\3\22\3\22\5")
        buf.write("\22\u0240\n\22\3\22\3\22\3\23\3\23\3\23\3\23\3\23\5\23")
        buf.write("\u0249\n\23\3\23\3\23\3\24\3\24\5\24\u024f\n\24\3\24\3")
        buf.write("\24\3\24\3\24\3\25\3\25\5\25\u0257\n\25\3\25\3\25\3\25")
        buf.write("\3\25\3\26\3\26\5\26\u025f\n\26\3\26\3\26\3\26\3\26\3")
        buf.write("\27\3\27\5\27\u0267\n\27\3\27\3\27\3\27\3\27\3\30\3\30")
        buf.write("\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30")
        buf.write("\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30\3\30")
        buf.write("\5\30\u0285\n\30\3\30\3\30\3\30\3\30\3\30\5\30\u028c\n")
        buf.write("\30\3\30\3\30\3\30\3\30\3\30\5\30\u0293\n\30\3\30\3\30")
        buf.write("\3\30\3\30\3\31\3\31\3\31\5\31\u029c\n\31\3\31\3\31\5")
        buf.write("\31\u02a0\n\31\3\31\3\31\5\31\u02a4\n\31\3\32\3\32\3\32")
        buf.write("\7\32\u02a9\n\32\f\32\16\32\u02ac\13\32\3\33\3\33\3\34")
        buf.write("\3\34\3\35\7\35\u02b3\n\35\f\35\16\35\u02b6\13\35\3\36")
        buf.write("\3\36\3\36\3\36\3\36\3\36\3\36\3\36\3\36\3\36\5\36\u02c2")
        buf.write("\n\36\3\37\3\37\3\37\3\37\3\37\3\37\3\37\3\37\3\37\3\37")
        buf.write("\3\37\3\37\3\37\5\37\u02d1\n\37\3\37\3\37\3 \3 \3 \3 ")
        buf.write("\3 \3 \3 \3 \3 \3 \3 \3 \3 \3 \3 \3 \3 \5 \u02e6\n \3")
        buf.write(" \3 \3 \3 \5 \u02ec\n \3 \3 \3 \3 \3 \3 \3 \3 \3 \3!\3")
        buf.write("!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\5!\u0308")
        buf.write("\n!\3!\3!\3!\3!\5!\u030e\n!\3!\3!\3!\3!\3!\7!\u0315\n")
        buf.write("!\f!\16!\u0318\13!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3!\3")
        buf.write("!\3!\3!\3!\3!\5!\u032a\n!\3!\3!\3!\3!\3!\3\"\3\"\3#\3")
        buf.write("#\3#\3#\3#\3#\3#\5#\u033a\n#\3#\3#\3#\3#\3#\3#\3#\6#\u0343")
        buf.write("\n#\r#\16#\u0344\3#\3#\3#\3#\3#\3$\3$\3$\3$\3$\3$\3$\3")
        buf.write("%\3%\3%\3%\3%\3%\3%\3&\3&\3&\3&\3&\3&\3&\3&\3&\3&\3&\3")
        buf.write("&\3&\3&\3&\3&\3\'\3\'\3\'\3\'\3\'\3\'\3\'\3\'\3\'\3\'")
        buf.write("\3\'\3\'\5\'\u0376\n\'\3\'\3\'\3(\3(\3(\3(\3(\3)\3)\3")
        buf.write(")\3*\3*\3*\3*\5*\u0386\n*\3*\3*\3*\3*\3*\5*\u038d\n*\3")
        buf.write("*\3*\3*\3*\5*\u0393\n*\3+\3+\3+\3+\3+\3+\3+\3+\3+\3+\3")
        buf.write("+\3+\3+\3+\3,\3,\3,\3,\3,\3,\3,\3,\5,\u03ab\n,\3-\3-\3")
        buf.write("-\3-\3-\3-\3-\7-\u03b4\n-\f-\16-\u03b7\13-\5-\u03b9\n")
        buf.write("-\3.\5.\u03bc\n.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3")
        buf.write(".\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3.\3")
        buf.write(".\3.\3.\3.\3.\7.\u03e1\n.\f.\16.\u03e4\13.\3.\5.\u03e7")
        buf.write("\n.\3/\3/\3/\3/\3/\3/\3/\3\60\3\60\3\61\3\61\5\61\u03f4")
        buf.write("\n\61\3\62\3\62\3\62\7\62\u03f9\n\62\f\62\16\62\u03fc")
        buf.write("\13\62\3\63\3\63\3\63\3\63\3\63\3\63\3\63\3\63\3\63\3")
        buf.write("\63\3\63\3\63\3\63\7\63\u040b\n\63\f\63\16\63\u040e\13")
        buf.write("\63\3\63\3\63\3\63\3\64\3\64\3\64\3\64\3\64\3\64\3\64")
        buf.write("\3\64\3\64\3\64\3\64\3\64\3\64\3\64\3\64\5\64\u0422\n")
        buf.write("\64\3\65\3\65\3\65\3\65\3\65\3\65\3\65\3\65\3\65\3\65")
        buf.write("\3\65\3\65\3\65\3\65\3\65\3\65\3\65\7\65\u0435\n\65\f")
        buf.write("\65\16\65\u0438\13\65\3\65\7\65\u043b\n\65\f\65\16\65")
        buf.write("\u043e\13\65\3\65\3\65\3\65\3\66\3\66\3\66\3\67\3\67\3")
        buf.write("\67\38\38\38\38\38\38\38\39\39\39\59\u0453\n9\3:\3:\3")
        buf.write(":\3:\3:\3:\3:\3:\3:\3:\3:\5:\u0460\n:\3:\3:\5:\u0464\n")
        buf.write(":\3:\3:\3:\5:\u0469\n:\3:\3:\7:\u046d\n:\f:\16:\u0470")
        buf.write("\13:\5:\u0472\n:\3:\3:\5:\u0476\n:\3;\3;\5;\u047a\n;\3")
        buf.write("<\3<\3<\7<\u047f\n<\f<\16<\u0482\13<\3=\3=\3>\3>\3>\3")
        buf.write(">\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\3>\5")
        buf.write(">\u049b\n>\3>\3>\3>\3>\3>\3>\7>\u04a3\n>\f>\16>\u04a6")
        buf.write("\13>\3>\3>\3>\3>\3>\5>\u04ad\n>\3>\3>\5>\u04b1\n>\3>\3")
        buf.write(">\3?\3?\5?\u04b7\n?\3@\3@\5@\u04bb\n@\3A\3A\3A\3A\3A\3")
        buf.write("A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3")
        buf.write("A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3A\3")
        buf.write("A\3A\3A\5A\u04e9\nA\3A\3A\5A\u04ed\nA\3A\3A\3A\3A\3A\5")
        buf.write("A\u04f4\nA\3A\3A\3A\3A\5A\u04fa\nA\3A\3A\5A\u04fe\nA\3")
        buf.write("A\3A\3B\3B\3B\7B\u0505\nB\fB\16B\u0508\13B\3C\3C\5C\u050c")
        buf.write("\nC\3D\3D\3D\3D\3D\3D\3D\3D\3D\3D\5D\u0518\nD\3D\3D\3")
        buf.write("D\3E\3E\3E\3E\3E\3E\5E\u0523\nE\3F\3F\3F\3F\3F\3F\3F\3")
        buf.write("F\3F\3F\3F\5F\u0530\nF\3G\3G\3G\3G\3G\3G\3G\3G\3G\3G\3")
        buf.write("G\3G\3G\7G\u053f\nG\fG\16G\u0542\13G\3G\3G\5G\u0546\n")
        buf.write("G\3G\3G\3G\5G\u054b\nG\3H\3H\3H\3H\3H\3H\3H\3H\3H\3H\3")
        buf.write("H\3H\3H\7H\u055a\nH\fH\16H\u055d\13H\3H\3H\5H\u0561\n")
        buf.write("H\3H\3H\3H\5H\u0566\nH\3I\3I\3I\3I\3I\3I\5I\u056e\nI\3")
        buf.write("J\3J\3J\3J\3J\3K\3K\3K\3K\3K\3K\3K\3K\3L\3L\3L\3L\3L\3")
        buf.write("M\3M\3M\3M\3M\3M\3M\3M\3M\3M\3M\3M\5M\u058e\nM\3M\3M\3")
        buf.write("M\5M\u0593\nM\3N\7N\u0596\nN\fN\16N\u0599\13N\3O\3O\3")
        buf.write("O\3O\3O\3O\3O\5O\u05a2\nO\3P\3P\3P\3P\3P\3P\3P\3P\5P\u05ac")
        buf.write("\nP\3Q\3Q\3Q\3Q\3Q\3Q\3Q\3Q\7Q\u05b6\nQ\fQ\16Q\u05b9\13")
        buf.write("Q\3Q\3Q\5Q\u05bd\nQ\3Q\3Q\3Q\5Q\u05c2\nQ\3R\3R\3R\3R\3")
        buf.write("R\3R\3R\3R\7R\u05cc\nR\fR\16R\u05cf\13R\3R\3R\5R\u05d3")
        buf.write("\nR\3R\3R\3R\5R\u05d8\nR\3S\3S\3S\3S\3S\3S\3S\3S\5S\u05e2")
        buf.write("\nS\3S\3S\3S\3S\5S\u05e8\nS\3T\3T\3T\3T\3U\3U\3V\3V\3")
        buf.write("V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\3V\5V\u0603")
        buf.write("\nV\3V\3V\7V\u0607\nV\fV\16V\u060a\13V\3V\3V\3W\3W\3W")
        buf.write("\7W\u0611\nW\fW\16W\u0614\13W\3X\3X\3X\3X\3X\3X\3X\3X")
        buf.write("\3X\3X\3X\5X\u0621\nX\3Y\3Y\3Y\3Y\3Z\3Z\3Z\3Z\3[\7[\u062c")
        buf.write("\n[\f[\16[\u062f\13[\3\\\3\\\5\\\u0633\n\\\3]\3]\5]\u0637")
        buf.write("\n]\3^\3^\3^\3^\3^\3^\3^\3^\3^\5^\u0642\n^\3^\3^\3^\3")
        buf.write("^\5^\u0648\n^\3^\3^\3^\3^\3_\3_\3_\7_\u0651\n_\f_\16_")
        buf.write("\u0654\13_\3`\3`\3`\3`\3`\3`\5`\u065c\n`\3a\3a\3a\3a\3")
        buf.write("a\3a\3a\3a\5a\u0666\na\3a\3a\3a\3a\3a\3a\3a\3a\3a\3a\3")
        buf.write("a\3b\3b\3b\7b\u0676\nb\fb\16b\u0679\13b\3c\3c\5c\u067d")
        buf.write("\nc\3d\3d\5d\u0681\nd\3e\3e\3e\3e\3e\3e\3e\3e\3e\5e\u068c")
        buf.write("\ne\3e\3e\3e\3e\5e\u0692\ne\3e\3e\3e\3e\3e\3f\3f\3f\5")
        buf.write("f\u069c\nf\3f\3f\3f\3f\3f\5f\u06a3\nf\3f\3f\3f\3f\3f\3")
        buf.write("f\5f\u06ab\nf\3g\3g\3g\7g\u06b0\ng\fg\16g\u06b3\13g\3")
        buf.write("h\3h\3h\3h\3h\3h\3h\3h\3h\5h\u06be\nh\3i\3i\3i\3i\3i\3")
        buf.write("i\3i\3i\3i\5i\u06c9\ni\3i\5i\u06cc\ni\3i\3i\3i\3i\3j\3")
        buf.write("j\3j\7j\u06d5\nj\fj\16j\u06d8\13j\3k\3k\5k\u06dc\nk\3")
        buf.write("l\3l\3l\3l\3l\3l\3l\3l\5l\u06e6\nl\3l\3l\3l\3l\5l\u06ec")
        buf.write("\nl\3l\3l\3l\3l\3l\3l\3l\3l\3l\3l\3l\3l\3m\3m\3m\7m\u06fd")
        buf.write("\nm\fm\16m\u0700\13m\3n\3n\3n\5n\u0705\nn\3o\3o\3o\3o")
        buf.write("\3o\3o\3o\3o\5o\u070f\no\3o\3o\3o\3o\5o\u0715\no\3o\3")
        buf.write("o\3o\3o\3o\3o\3o\3o\3o\3o\3o\3o\5o\u0723\no\3o\3o\3o\3")
        buf.write("o\3p\3p\3p\7p\u072c\np\fp\16p\u072f\13p\3q\3q\5q\u0733")
        buf.write("\nq\3r\3r\3r\3r\3r\3r\3r\5r\u073c\nr\3r\3r\3r\3r\3r\5")
        buf.write("r\u0743\nr\3r\3r\3r\3r\3s\3s\3s\7s\u074c\ns\fs\16s\u074f")
        buf.write("\13s\3t\3t\3t\3t\5t\u0755\nt\3u\3u\3u\3u\3u\3u\3u\3u\5")
        buf.write("u\u075f\nu\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3")
        buf.write("u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3")
        buf.write("u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3")
        buf.write("u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3u\3")
        buf.write("u\3u\3u\3u\3u\5u\u07aa\nu\3u\7u\u07ad\nu\fu\16u\u07b0")
        buf.write("\13u\5u\u07b2\nu\3u\3u\3u\3v\3v\3v\3v\3v\3v\3v\3v\3v\3")
        buf.write("v\3v\3v\5v\u07c3\nv\3v\3v\3v\3v\5v\u07c9\nv\3w\3w\3w\7")
        buf.write("w\u07ce\nw\fw\16w\u07d1\13w\3x\3x\3y\3y\3y\3y\3y\3y\3")
        buf.write("y\3y\5y\u07dd\ny\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3")
        buf.write("y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3")
        buf.write("y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3")
        buf.write("y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3y\3")
        buf.write("y\3y\3y\3y\3y\3y\3y\5y\u0828\ny\3y\7y\u082b\ny\fy\16y")
        buf.write("\u082e\13y\5y\u0830\ny\3y\3y\3y\3z\3z\3z\3z\3z\3z\3z\3")
        buf.write("z\3z\3z\3z\3z\5z\u0841\nz\3z\3z\3z\3z\5z\u0847\nz\3{\3")
        buf.write("{\3{\7{\u084c\n{\f{\16{\u084f\13{\3|\3|\3}\3}\3}\3}\5")
        buf.write("}\u0857\n}\3~\3~\3~\3~\3~\3\177\3\177\3\u0080\3\u0080")
        buf.write("\3\u0081\3\u0081\3\u0081\3\u0081\3\u0081\3\u0081\5\u0081")
        buf.write("\u0868\n\u0081\3\u0082\3\u0082\3\u0082\3\u0082\5\u0082")
        buf.write("\u086e\n\u0082\3\u0083\3\u0083\3\u0083\3\u0083\7\u0083")
        buf.write("\u0874\n\u0083\f\u0083\16\u0083\u0877\13\u0083\3\u0083")
        buf.write("\3\u0083\3\u0083\3\u0084\3\u0084\3\u0084\3\u0084\3\u0084")
        buf.write("\3\u0084\7\u0084\u0882\n\u0084\f\u0084\16\u0084\u0885")
        buf.write("\13\u0084\3\u0084\3\u0084\5\u0084\u0889\n\u0084\3\u0084")
        buf.write("\3\u0084\3\u0084\3\u0085\3\u0085\3\u0085\3\u0085\3\u0085")
        buf.write("\3\u0085\7\u0085\u0894\n\u0085\f\u0085\16\u0085\u0897")
        buf.write("\13\u0085\3\u0085\3\u0085\5\u0085\u089b\n\u0085\3\u0085")
        buf.write("\3\u0085\3\u0085\3\u0086\3\u0086\3\u0086\3\u0086\3\u0086")
        buf.write("\3\u0086\7\u0086\u08a6\n\u0086\f\u0086\16\u0086\u08a9")
        buf.write("\13\u0086\3\u0086\3\u0086\5\u0086\u08ad\n\u0086\3\u0086")
        buf.write("\3\u0086\3\u0086\7\u0086\u08b2\n\u0086\f\u0086\16\u0086")
        buf.write("\u08b5\13\u0086\3\u0086\3\u0086\3\u0086\3\u0087\3\u0087")
        buf.write("\3\u0087\3\u0087\3\u0087\3\u0087\7\u0087\u08c0\n\u0087")
        buf.write("\f\u0087\16\u0087\u08c3\13\u0087\3\u0087\3\u0087\5\u0087")
        buf.write("\u08c7\n\u0087\3\u0087\3\u0087\3\u0087\7\u0087\u08cc\n")
        buf.write("\u0087\f\u0087\16\u0087\u08cf\13\u0087\3\u0087\3\u0087")
        buf.write("\3\u0087\3\u0088\3\u0088\3\u0088\3\u0088\3\u0088\3\u0088")
        buf.write("\3\u0088\3\u0088\3\u0088\3\u0088\3\u0088\3\u0088\3\u0088")
        buf.write("\7\u0088\u08e1\n\u0088\f\u0088\16\u0088\u08e4\13\u0088")
        buf.write("\3\u0088\3\u0088\5\u0088\u08e8\n\u0088\3\u0088\3\u0088")
        buf.write("\3\u0088\3\u0088\3\u0089\3\u0089\3\u0089\5\u0089\u08f1")
        buf.write("\n\u0089\3\u008a\3\u008a\3\u008a\3\u008a\3\u008a\3\u008a")
        buf.write("\3\u008a\3\u008a\3\u008a\3\u008a\3\u008b\3\u008b\3\u008b")
        buf.write("\3\u008b\3\u008b\3\u008b\3\u008b\3\u008b\3\u008b\3\u008b")
        buf.write("\3\u008b\3\u008b\3\u008b\3\u008b\3\u008b\3\u008b\3\u008b")
        buf.write("\3\u008b\3\u008b\3\u008b\3\u008b\5\u008b\u0912\n\u008b")
        buf.write("\3\u008b\3\u008b\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c")
        buf.write("\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c")
        buf.write("\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c")
        buf.write("\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c\3\u008c")
        buf.write("\7\u008c\u0930\n\u008c\f\u008c\16\u008c\u0933\13\u008c")
        buf.write("\5\u008c\u0935\n\u008c\3\u008c\3\u008c\3\u008c\3\u008c")
        buf.write("\5\u008c\u093b\n\u008c\3\u008c\3\u008c\3\u008d\3\u008d")
        buf.write("\3\u008d\3\u008d\3\u008e\3\u008e\5\u008e\u0945\n\u008e")
        buf.write("\3\u008e\3\u008e\3\u008e\3\u008e\3\u008e\3\u008e\3\u008e")
        buf.write("\3\u008e\3\u008e\3\u008e\3\u008e\3\u008e\3\u008e\3\u008e")
        buf.write("\3\u008e\3\u008e\3\u008e\5\u008e\u0958\n\u008e\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\3\u008f\3\u008f\3\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u0965\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u096b\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u0971\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u0977\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u097d\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u0983\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u0989\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u098f\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u0995\n\u008f\3\u008f")
        buf.write("\3\u008f\3\u008f\3\u008f\5\u008f\u099b\n\u008f\5\u008f")
        buf.write("\u099d\n\u008f\3\u008f\7\u008f\u09a0\n\u008f\f\u008f\16")
        buf.write("\u008f\u09a3\13\u008f\5\u008f\u09a5\n\u008f\3\u008f\3")
        buf.write("\u008f\7\u008f\u09a9\n\u008f\f\u008f\16\u008f\u09ac\13")
        buf.write("\u008f\3\u008f\5\u008f\u09af\n\u008f\3\u008f\3\u008f\3")
        buf.write("\u0090\3\u0090\3\u0090\3\u0090\3\u0090\5\u0090\u09b8\n")
        buf.write("\u0090\3\u0090\3\u0090\7\u0090\u09bc\n\u0090\f\u0090\16")
        buf.write("\u0090\u09bf\13\u0090\3\u0090\3\u0090\3\u0090\3\u0091")
        buf.write("\3\u0091\3\u0091\3\u0092\3\u0092\3\u0093\3\u0093\3\u0093")
        buf.write("\7\u0093\u09cc\n\u0093\f\u0093\16\u0093\u09cf\13\u0093")
        buf.write("\3\u0094\3\u0094\3\u0094\7\u0094\u09d4\n\u0094\f\u0094")
        buf.write("\16\u0094\u09d7\13\u0094\3\u0095\3\u0095\3\u0095\7\u0095")
        buf.write("\u09dc\n\u0095\f\u0095\16\u0095\u09df\13\u0095\3\u0096")
        buf.write("\3\u0096\3\u0096\7\u0096\u09e4\n\u0096\f\u0096\16\u0096")
        buf.write("\u09e7\13\u0096\3\u0097\3\u0097\3\u0097\7\u0097\u09ec")
        buf.write("\n\u0097\f\u0097\16\u0097\u09ef\13\u0097\3\u0098\3\u0098")
        buf.write("\7\u0098\u09f3\n\u0098\f\u0098\16\u0098\u09f6\13\u0098")
        buf.write("\3\u0099\3\u0099\3\u0099\3\u0099\5\u0099\u09fc\n\u0099")
        buf.write("\3\u009a\3\u009a\7\u009a\u0a00\n\u009a\f\u009a\16\u009a")
        buf.write("\u0a03\13\u009a\3\u009b\3\u009b\3\u009b\3\u009b\3\u009b")
        buf.write("\3\u009b\3\u009b\3\u009b\5\u009b\u0a0d\n\u009b\3\u009c")
        buf.write("\3\u009c\7\u009c\u0a11\n\u009c\f\u009c\16\u009c\u0a14")
        buf.write("\13\u009c\3\u009d\3\u009d\3\u009d\3\u009d\5\u009d\u0a1a")
        buf.write("\n\u009d\3\u009e\3\u009e\7\u009e\u0a1e\n\u009e\f\u009e")
        buf.write("\16\u009e\u0a21\13\u009e\3\u009f\3\u009f\3\u009f\3\u009f")
        buf.write("\5\u009f\u0a27\n\u009f\3\u00a0\3\u00a0\7\u00a0\u0a2b\n")
        buf.write("\u00a0\f\u00a0\16\u00a0\u0a2e\13\u00a0\3\u00a1\3\u00a1")
        buf.write("\3\u00a1\3\u00a1\3\u00a1\3\u00a1\5\u00a1\u0a36\n\u00a1")
        buf.write("\3\u00a2\7\u00a2\u0a39\n\u00a2\f\u00a2\16\u00a2\u0a3c")
        buf.write("\13\u00a2\3\u00a2\3\u00a2\3\u00a3\3\u00a3\3\u00a3\3\u00a3")
        buf.write("\3\u00a4\3\u00a4\3\u00a4\3\u00a4\3\u00a4\3\u00a4\3\u00a4")
        buf.write("\3\u00a4\3\u00a4\3\u00a4\3\u00a4\5\u00a4\u0a4f\n\u00a4")
        buf.write("\3\u00a5\3\u00a5\3\u00a5\3\u00a5\3\u00a5\3\u00a5\3\u00a5")
        buf.write("\3\u00a6\3\u00a6\3\u00a6\3\u00a6\3\u00a6\3\u00a6\3\u00a6")
        buf.write("\3\u00a7\3\u00a7\3\u00a7\3\u00a7\3\u00a7\3\u00a7\3\u00a7")
        buf.write("\3\u00a7\3\u00a7\3\u00a8\3\u00a8\3\u00a8\3\u00a8\3\u00a9")
        buf.write("\3\u00a9\3\u00a9\3\u00a9\3\u00a9\3\u00a9\3\u00a9\3\u00a9")
        buf.write("\3\u00a9\3\u00a9\3\u00a9\5\u00a9\u0a77\n\u00a9\3\u00aa")
        buf.write("\3\u00aa\3\u00ab\3\u00ab\3\u00ab\3\u00ab\3\u00ab\3\u00ab")
        buf.write("\3\u00ab\3\u00ab\3\u00ab\3\u00ab\3\u00ab\3\u00ab\3\u00ab")
        buf.write("\3\u00ab\3\u00ab\5\u00ab\u0a8a\n\u00ab\3\u00ac\3\u00ac")
        buf.write("\3\u00ac\3\u00ac\3\u00ac\3\u00ac\3\u00ac\3\u00ac\3\u00ac")
        buf.write("\3\u00ac\3\u00ac\3\u00ac\5\u00ac\u0a98\n\u00ac\3\u00ad")
        buf.write("\3\u00ad\3\u00ad\3\u00ad\3\u00ad\3\u00ad\3\u00ad\3\u00ad")
        buf.write("\3\u00ad\3\u00ad\3\u00ad\3\u00ad\5\u00ad\u0aa6\n\u00ad")
        buf.write("\3\u00ae\3\u00ae\3\u00ae\3\u00ae\6\u00ae\u0aac\n\u00ae")
        buf.write("\r\u00ae\16\u00ae\u0aad\3\u00af\3\u00af\3\u00af\3\u00af")
        buf.write("\3\u00af\3\u00af\3\u00af\7\u00af\u0ab7\n\u00af\f\u00af")
        buf.write("\16\u00af\u0aba\13\u00af\5\u00af\u0abc\n\u00af\3\u00b0")
        buf.write("\3\u00b0\3\u00b0\3\u00b0\5\u00b0\u0ac2\n\u00b0\3\u00b1")
        buf.write("\3\u00b1\3\u00b1\3\u00b1\3\u00b1\3\u00b2\3\u00b2\3\u00b2")
        buf.write("\3\u00b2\3\u00b2\3\u00b3\3\u00b3\3\u00b3\3\u00b3\3\u00b3")
        buf.write("\3\u00b3\3\u00b3\5\u00b3\u0ad5\n\u00b3\3\u00b3\3\u00b3")
        buf.write("\3\u00b4\3\u00b4\3\u00b5\3\u00b5\3\u00b5\3\u00b5\3\u00b5")
        buf.write("\3\u00b6\3\u00b6\3\u00b7\3\u00b7\3\u00b7\3\u00b7\3\u00b7")
        buf.write("\3\u00b7\3\u00b7\5\u00b7\u0ae9\n\u00b7\3\u00b7\3\u00b7")
        buf.write("\3\u00b8\3\u00b8\3\u00b9\3\u00b9\3\u00b9\3\u00b9\3\u00b9")
        buf.write("\3\u00b9\3\u00b9\3\u00b9\3\u00b9\3\u00b9\5\u00b9\u0af9")
        buf.write("\n\u00b9\3\u00ba\3\u00ba\3\u00ba\3\u00ba\3\u00ba\3\u00bb")
        buf.write("\3\u00bb\3\u00bb\3\u00bb\3\u00bb\3\u00bc\3\u00bc\3\u00bc")
        buf.write("\3\u00bc\3\u00bc\3\u00bc\3\u00bc\3\u00bc\3\u00bc\5\u00bc")
        buf.write("\u0b0e\n\u00bc\3\u00bc\3\u00bc\3\u00bc\3\u00bc\3\u00bc")
        buf.write("\5\u00bc\u0b15\n\u00bc\3\u00bc\3\u00bc\3\u00bc\3\u00bd")
        buf.write("\3\u00bd\3\u00bd\3\u00bd\3\u00bd\3\u00be\3\u00be\3\u00be")
        buf.write("\3\u00be\3\u00be\3\u00bf\3\u00bf\3\u00bf\3\u00bf\3\u00bf")
        buf.write("\5\u00bf\u0b29\n\u00bf\3\u00bf\3\u00bf\3\u00bf\3\u00bf")
        buf.write("\3\u00c0\3\u00c0\3\u00c0\3\u00c0\3\u00c0\3\u00c1\3\u00c1")
        buf.write("\3\u00c1\3\u00c1\3\u00c1\3\u00c2\3\u00c2\3\u00c2\3\u00c2")
        buf.write("\3\u00c2\3\u00c3\3\u00c3\3\u00c3\3\u00c3\3\u00c3\3\u00c3")
        buf.write("\3\u00c3\5\u00c3\u0b45\n\u00c3\3\u00c3\3\u00c3\3\u00c3")
        buf.write("\3\u00c3\3\u00c4\3\u00c4\3\u00c4\3\u00c4\3\u00c4\5\u00c4")
        buf.write("\u0b50\n\u00c4\3\u00c5\3\u00c5\3\u00c5\3\u00c5\3\u00c5")
        buf.write("\3\u00c5\3\u00c5\3\u00c5\3\u00c5\3\u00c6\3\u00c6\3\u00c6")
        buf.write("\3\u00c6\3\u00c6\7\u00c6\u0b60\n\u00c6\f\u00c6\16\u00c6")
        buf.write("\u0b63\13\u00c6\3\u00c6\3\u00c6\3\u00c6\3\u00c6\3\u00c6")
        buf.write("\3\u00c6\3\u00c6\3\u00c6\3\u00c7\3\u00c7\3\u00c8\3\u00c8")
        buf.write("\3\u00c8\3\u00c8\3\u00c8\3\u00c8\3\u00c8\3\u00c8\3\u00c8")
        buf.write("\3\u00c9\3\u00c9\3\u00c9\3\u00c9\3\u00c9\3\u00c9\3\u00c9")
        buf.write("\3\u00c9\3\u00c9\3\u00ca\3\u00ca\3\u00ca\3\u00ca\3\u00ca")
        buf.write("\3\u00ca\3\u00ca\7\u00ca\u0b88\n\u00ca\f\u00ca\16\u00ca")
        buf.write("\u0b8b\13\u00ca\3\u00ca\3\u00ca\3\u00ca\3\u00ca\3\u00ca")
        buf.write("\3\u00ca\3\u00ca\3\u00ca\3\u00cb\3\u00cb\3\u00cc\3\u00cc")
        buf.write("\3\u00cc\3\u00cc\3\u00cc\3\u00cc\3\u00cc\3\u00cc\3\u00cc")
        buf.write("\7\u00cc\u0ba0\n\u00cc\f\u00cc\16\u00cc\u0ba3\13\u00cc")
        buf.write("\3\u00cc\3\u00cc\3\u00cc\2\2\u00cd\2\4\6\b\n\f\16\20\22")
        buf.write("\24\26\30\32\34\36 \"$&(*,.\60\62\64\668:<>@BDFHJLNPR")
        buf.write("TVXZ\\^`bdfhjlnprtvxz|~\u0080\u0082\u0084\u0086\u0088")
        buf.write("\u008a\u008c\u008e\u0090\u0092\u0094\u0096\u0098\u009a")
        buf.write("\u009c\u009e\u00a0\u00a2\u00a4\u00a6\u00a8\u00aa\u00ac")
        buf.write("\u00ae\u00b0\u00b2\u00b4\u00b6\u00b8\u00ba\u00bc\u00be")
        buf.write("\u00c0\u00c2\u00c4\u00c6\u00c8\u00ca\u00cc\u00ce\u00d0")
        buf.write("\u00d2\u00d4\u00d6\u00d8\u00da\u00dc\u00de\u00e0\u00e2")
        buf.write("\u00e4\u00e6\u00e8\u00ea\u00ec\u00ee\u00f0\u00f2\u00f4")
        buf.write("\u00f6\u00f8\u00fa\u00fc\u00fe\u0100\u0102\u0104\u0106")
        buf.write("\u0108\u010a\u010c\u010e\u0110\u0112\u0114\u0116\u0118")
        buf.write("\u011a\u011c\u011e\u0120\u0122\u0124\u0126\u0128\u012a")
        buf.write("\u012c\u012e\u0130\u0132\u0134\u0136\u0138\u013a\u013c")
        buf.write("\u013e\u0140\u0142\u0144\u0146\u0148\u014a\u014c\u014e")
        buf.write("\u0150\u0152\u0154\u0156\u0158\u015a\u015c\u015e\u0160")
        buf.write("\u0162\u0164\u0166\u0168\u016a\u016c\u016e\u0170\u0172")
        buf.write("\u0174\u0176\u0178\u017a\u017c\u017e\u0180\u0182\u0184")
        buf.write("\u0186\u0188\u018a\u018c\u018e\u0190\u0192\u0194\u0196")
        buf.write("\2\20\3\2\4\5\4\2\u00ac\u00b2\u00fa\u00fa\4\2\u00b3\u00b6")
        buf.write("\u00fa\u00fa\4\2\n\n\u00fa\u00fa\5\2\u00b7\u00b9\u00bd")
        buf.write("\u00bf\u00fa\u00fa\4\2\u00ba\u00bf\u00fa\u00fa\3\2\u0085")
        buf.write("\u0086\3\2\u008a\u008c\3\2RV\3\2\u00fa\u00fb\3\2\u00ef")
        buf.write("\u00f2\4\2\u00d3\u00d9\u00fa\u00fa\3\2\u00f6\u00f7\3\2")
        buf.write("\u00f8\u00fa\2\u0c71\2\u019d\3\2\2\2\4\u01a3\3\2\2\2\6")
        buf.write("\u01a5\3\2\2\2\b\u01af\3\2\2\2\n\u01b1\3\2\2\2\f\u01bc")
        buf.write("\3\2\2\2\16\u01ce\3\2\2\2\20\u01ef\3\2\2\2\22\u01f2\3")
        buf.write("\2\2\2\24\u01fb\3\2\2\2\26\u0204\3\2\2\2\30\u020d\3\2")
        buf.write("\2\2\32\u0216\3\2\2\2\34\u021f\3\2\2\2\36\u0228\3\2\2")
        buf.write("\2 \u0231\3\2\2\2\"\u023a\3\2\2\2$\u0243\3\2\2\2&\u024c")
        buf.write("\3\2\2\2(\u0254\3\2\2\2*\u025c\3\2\2\2,\u0264\3\2\2\2")
        buf.write(".\u026c\3\2\2\2\60\u0298\3\2\2\2\62\u02a5\3\2\2\2\64\u02ad")
        buf.write("\3\2\2\2\66\u02af\3\2\2\28\u02b4\3\2\2\2:\u02c1\3\2\2")
        buf.write("\2<\u02c3\3\2\2\2>\u02d4\3\2\2\2@\u02f6\3\2\2\2B\u0330")
        buf.write("\3\2\2\2D\u0332\3\2\2\2F\u034b\3\2\2\2H\u0352\3\2\2\2")
        buf.write("J\u0359\3\2\2\2L\u0369\3\2\2\2N\u0379\3\2\2\2P\u037e\3")
        buf.write("\2\2\2R\u0385\3\2\2\2T\u0394\3\2\2\2V\u03aa\3\2\2\2X\u03b8")
        buf.write("\3\2\2\2Z\u03e6\3\2\2\2\\\u03e8\3\2\2\2^\u03ef\3\2\2\2")
        buf.write("`\u03f3\3\2\2\2b\u03f5\3\2\2\2d\u03fd\3\2\2\2f\u0421\3")
        buf.write("\2\2\2h\u0423\3\2\2\2j\u0442\3\2\2\2l\u0445\3\2\2\2n\u0448")
        buf.write("\3\2\2\2p\u0452\3\2\2\2r\u0454\3\2\2\2t\u0479\3\2\2\2")
        buf.write("v\u047b\3\2\2\2x\u0483\3\2\2\2z\u0485\3\2\2\2|\u04b6\3")
        buf.write("\2\2\2~\u04ba\3\2\2\2\u0080\u04bc\3\2\2\2\u0082\u0501")
        buf.write("\3\2\2\2\u0084\u050b\3\2\2\2\u0086\u050d\3\2\2\2\u0088")
        buf.write("\u0522\3\2\2\2\u008a\u052f\3\2\2\2\u008c\u0531\3\2\2\2")
        buf.write("\u008e\u054c\3\2\2\2\u0090\u0567\3\2\2\2\u0092\u056f\3")
        buf.write("\2\2\2\u0094\u0574\3\2\2\2\u0096\u057c\3\2\2\2\u0098\u0581")
        buf.write("\3\2\2\2\u009a\u0597\3\2\2\2\u009c\u05a1\3\2\2\2\u009e")
        buf.write("\u05ab\3\2\2\2\u00a0\u05ad\3\2\2\2\u00a2\u05c3\3\2\2\2")
        buf.write("\u00a4\u05d9\3\2\2\2\u00a6\u05e9\3\2\2\2\u00a8\u05ed\3")
        buf.write("\2\2\2\u00aa\u05ef\3\2\2\2\u00ac\u060d\3\2\2\2\u00ae\u0620")
        buf.write("\3\2\2\2\u00b0\u0622\3\2\2\2\u00b2\u0626\3\2\2\2\u00b4")
        buf.write("\u062d\3\2\2\2\u00b6\u0632\3\2\2\2\u00b8\u0636\3\2\2\2")
        buf.write("\u00ba\u0638\3\2\2\2\u00bc\u064d\3\2\2\2\u00be\u065b\3")
        buf.write("\2\2\2\u00c0\u065d\3\2\2\2\u00c2\u0672\3\2\2\2\u00c4\u067c")
        buf.write("\3\2\2\2\u00c6\u0680\3\2\2\2\u00c8\u0682\3\2\2\2\u00ca")
        buf.write("\u0698\3\2\2\2\u00cc\u06ac\3\2\2\2\u00ce\u06bd\3\2\2\2")
        buf.write("\u00d0\u06bf\3\2\2\2\u00d2\u06d1\3\2\2\2\u00d4\u06db\3")
        buf.write("\2\2\2\u00d6\u06dd\3\2\2\2\u00d8\u06f9\3\2\2\2\u00da\u0704")
        buf.write("\3\2\2\2\u00dc\u0706\3\2\2\2\u00de\u0728\3\2\2\2\u00e0")
        buf.write("\u0732\3\2\2\2\u00e2\u0734\3\2\2\2\u00e4\u0748\3\2\2\2")
        buf.write("\u00e6\u0754\3\2\2\2\u00e8\u0756\3\2\2\2\u00ea\u07b6\3")
        buf.write("\2\2\2\u00ec\u07ca\3\2\2\2\u00ee\u07d2\3\2\2\2\u00f0\u07d4")
        buf.write("\3\2\2\2\u00f2\u0834\3\2\2\2\u00f4\u0848\3\2\2\2\u00f6")
        buf.write("\u0850\3\2\2\2\u00f8\u0856\3\2\2\2\u00fa\u0858\3\2\2\2")
        buf.write("\u00fc\u085d\3\2\2\2\u00fe\u085f\3\2\2\2\u0100\u0867\3")
        buf.write("\2\2\2\u0102\u086d\3\2\2\2\u0104\u086f\3\2\2\2\u0106\u087b")
        buf.write("\3\2\2\2\u0108\u088d\3\2\2\2\u010a\u089f\3\2\2\2\u010c")
        buf.write("\u08b9\3\2\2\2\u010e\u08d3\3\2\2\2\u0110\u08f0\3\2\2\2")
        buf.write("\u0112\u08f2\3\2\2\2\u0114\u08fc\3\2\2\2\u0116\u0915\3")
        buf.write("\2\2\2\u0118\u093e\3\2\2\2\u011a\u0942\3\2\2\2\u011c\u0959")
        buf.write("\3\2\2\2\u011e\u09b2\3\2\2\2\u0120\u09c3\3\2\2\2\u0122")
        buf.write("\u09c6\3\2\2\2\u0124\u09c8\3\2\2\2\u0126\u09d0\3\2\2\2")
        buf.write("\u0128\u09d8\3\2\2\2\u012a\u09e0\3\2\2\2\u012c\u09e8\3")
        buf.write("\2\2\2\u012e\u09f0\3\2\2\2\u0130\u09fb\3\2\2\2\u0132\u09fd")
        buf.write("\3\2\2\2\u0134\u0a0c\3\2\2\2\u0136\u0a0e\3\2\2\2\u0138")
        buf.write("\u0a19\3\2\2\2\u013a\u0a1b\3\2\2\2\u013c\u0a26\3\2\2\2")
        buf.write("\u013e\u0a28\3\2\2\2\u0140\u0a35\3\2\2\2\u0142\u0a3a\3")
        buf.write("\2\2\2\u0144\u0a3f\3\2\2\2\u0146\u0a4e\3\2\2\2\u0148\u0a50")
        buf.write("\3\2\2\2\u014a\u0a57\3\2\2\2\u014c\u0a5e\3\2\2\2\u014e")
        buf.write("\u0a67\3\2\2\2\u0150\u0a76\3\2\2\2\u0152\u0a78\3\2\2\2")
        buf.write("\u0154\u0a7a\3\2\2\2\u0156\u0a8b\3\2\2\2\u0158\u0a99\3")
        buf.write("\2\2\2\u015a\u0aa7\3\2\2\2\u015c\u0abb\3\2\2\2\u015e\u0abd")
        buf.write("\3\2\2\2\u0160\u0ac3\3\2\2\2\u0162\u0ac8\3\2\2\2\u0164")
        buf.write("\u0acd\3\2\2\2\u0166\u0ad8\3\2\2\2\u0168\u0ada\3\2\2\2")
        buf.write("\u016a\u0adf\3\2\2\2\u016c\u0ae1\3\2\2\2\u016e\u0aec\3")
        buf.write("\2\2\2\u0170\u0af8\3\2\2\2\u0172\u0afa\3\2\2\2\u0174\u0aff")
        buf.write("\3\2\2\2\u0176\u0b04\3\2\2\2\u0178\u0b19\3\2\2\2\u017a")
        buf.write("\u0b1e\3\2\2\2\u017c\u0b23\3\2\2\2\u017e\u0b2e\3\2\2\2")
        buf.write("\u0180\u0b33\3\2\2\2\u0182\u0b38\3\2\2\2\u0184\u0b3d\3")
        buf.write("\2\2\2\u0186\u0b4f\3\2\2\2\u0188\u0b51\3\2\2\2\u018a\u0b5a")
        buf.write("\3\2\2\2\u018c\u0b6c\3\2\2\2\u018e\u0b6e\3\2\2\2\u0190")
        buf.write("\u0b77\3\2\2\2\u0192\u0b80\3\2\2\2\u0194\u0b94\3\2\2\2")
        buf.write("\u0196\u0b96\3\2\2\2\u0198\u019c\5\n\6\2\u0199\u019c\5")
        buf.write("\f\7\2\u019a\u019c\5\16\b\2\u019b\u0198\3\2\2\2\u019b")
        buf.write("\u0199\3\2\2\2\u019b\u019a\3\2\2\2\u019c\u019f\3\2\2\2")
        buf.write("\u019d\u019b\3\2\2\2\u019d\u019e\3\2\2\2\u019e\u01a1\3")
        buf.write("\2\2\2\u019f\u019d\3\2\2\2\u01a0\u01a2\5.\30\2\u01a1\u01a0")
        buf.write("\3\2\2\2\u01a1\u01a2\3\2\2\2\u01a2\3\3\2\2\2\u01a3\u01a4")
        buf.write("\7\3\2\2\u01a4\5\3\2\2\2\u01a5\u01a8\t\2\2\2\u01a6\u01a7")
        buf.write("\7!\2\2\u01a7\u01a9\7\u00fb\2\2\u01a8\u01a6\3\2\2\2\u01a8")
        buf.write("\u01a9\3\2\2\2\u01a9\u01ac\3\2\2\2\u01aa\u01ab\7!\2\2")
        buf.write("\u01ab\u01ad\7\u00fa\2\2\u01ac\u01aa\3\2\2\2\u01ac\u01ad")
        buf.write("\3\2\2\2\u01ad\7\3\2\2\2\u01ae\u01b0\7\u00fa\2\2\u01af")
        buf.write("\u01ae\3\2\2\2\u01af\u01b0\3\2\2\2\u01b0\t\3\2\2\2\u01b1")
        buf.write("\u01b2\7\6\2\2\u01b2\u01b3\7\7\2\2\u01b3\u01b7\7\30\2")
        buf.write("\2\u01b4\u01b8\5\4\3\2\u01b5\u01b8\5\6\4\2\u01b6\u01b8")
        buf.write("\5\b\5\2\u01b7\u01b4\3\2\2\2\u01b7\u01b5\3\2\2\2\u01b7")
        buf.write("\u01b6\3\2\2\2\u01b7\u01b8\3\2\2\2\u01b8\u01b9\3\2\2\2")
        buf.write("\u01b9\u01ba\7\31\2\2\u01ba\13\3\2\2\2\u01bb\u01bd\7\u0084")
        buf.write("\2\2\u01bc\u01bb\3\2\2\2\u01bc\u01bd\3\2\2\2\u01bd\u01be")
        buf.write("\3\2\2\2\u01be\u01c0\7P\2\2\u01bf\u01c1\7O\2\2\u01c0\u01bf")
        buf.write("\3\2\2\2\u01c0\u01c1\3\2\2\2\u01c1\u01c3\3\2\2\2\u01c2")
        buf.write("\u01c4\7\u00fb\2\2\u01c3\u01c2\3\2\2\2\u01c3\u01c4\3\2")
        buf.write("\2\2\u01c4\u01c5\3\2\2\2\u01c5\u01c6\7\26\2\2\u01c6\u01c7")
        buf.write("\5\20\t\2\u01c7\u01c9\7\27\2\2\u01c8\u01ca\7\u00fb\2\2")
        buf.write("\u01c9\u01c8\3\2\2\2\u01c9\u01ca\3\2\2\2\u01ca\u01cb\3")
        buf.write("\2\2\2\u01cb\u01cc\7 \2\2\u01cc\r\3\2\2\2\u01cd\u01cf")
        buf.write("\7\u0084\2\2\u01ce\u01cd\3\2\2\2\u01ce\u01cf\3\2\2\2\u01cf")
        buf.write("\u01d0\3\2\2\2\u01d0\u01d2\7Q\2\2\u01d1\u01d3\7O\2\2\u01d2")
        buf.write("\u01d1\3\2\2\2\u01d2\u01d3\3\2\2\2\u01d3\u01d5\3\2\2\2")
        buf.write("\u01d4\u01d6\7\u00fb\2\2\u01d5\u01d4\3\2\2\2\u01d5\u01d6")
        buf.write("\3\2\2\2\u01d6\u01d7\3\2\2\2\u01d7\u01d8\7\26\2\2\u01d8")
        buf.write("\u01d9\5\20\t\2\u01d9\u01db\7\27\2\2\u01da\u01dc\7\u00fb")
        buf.write("\2\2\u01db\u01da\3\2\2\2\u01db\u01dc\3\2\2\2\u01dc\u01dd")
        buf.write("\3\2\2\2\u01dd\u01de\7 \2\2\u01de\17\3\2\2\2\u01df\u01ee")
        buf.write("\5\22\n\2\u01e0\u01ee\5\24\13\2\u01e1\u01ee\5\26\f\2\u01e2")
        buf.write("\u01ee\5\30\r\2\u01e3\u01ee\5\32\16\2\u01e4\u01ee\5\34")
        buf.write("\17\2\u01e5\u01ee\5\36\20\2\u01e6\u01ee\5 \21\2\u01e7")
        buf.write("\u01ee\5\"\22\2\u01e8\u01ee\5$\23\2\u01e9\u01ee\5&\24")
        buf.write("\2\u01ea\u01ee\5(\25\2\u01eb\u01ee\5*\26\2\u01ec\u01ee")
        buf.write("\5,\27\2\u01ed\u01df\3\2\2\2\u01ed\u01e0\3\2\2\2\u01ed")
        buf.write("\u01e1\3\2\2\2\u01ed\u01e2\3\2\2\2\u01ed\u01e3\3\2\2\2")
        buf.write("\u01ed\u01e4\3\2\2\2\u01ed\u01e5\3\2\2\2\u01ed\u01e6\3")
        buf.write("\2\2\2\u01ed\u01e7\3\2\2\2\u01ed\u01e8\3\2\2\2\u01ed\u01e9")
        buf.write("\3\2\2\2\u01ed\u01ea\3\2\2\2\u01ed\u01eb\3\2\2\2\u01ed")
        buf.write("\u01ec\3\2\2\2\u01ee\u01f1\3\2\2\2\u01ef\u01ed\3\2\2\2")
        buf.write("\u01ef\u01f0\3\2\2\2\u01f0\21\3\2\2\2\u01f1\u01ef\3\2")
        buf.write("\2\2\u01f2\u01f3\7S\2\2\u01f3\u01f7\7\u00fb\2\2\u01f4")
        buf.write("\u01f5\7\32\2\2\u01f5\u01f6\7\u00fa\2\2\u01f6\u01f8\7")
        buf.write("\33\2\2\u01f7\u01f4\3\2\2\2\u01f7\u01f8\3\2\2\2\u01f8")
        buf.write("\u01f9\3\2\2\2\u01f9\u01fa\7 \2\2\u01fa\23\3\2\2\2\u01fb")
        buf.write("\u01fc\7T\2\2\u01fc\u0200\7\u00fb\2\2\u01fd\u01fe\7\32")
        buf.write("\2\2\u01fe\u01ff\7\u00fa\2\2\u01ff\u0201\7\33\2\2\u0200")
        buf.write("\u01fd\3\2\2\2\u0200\u0201\3\2\2\2\u0201\u0202\3\2\2\2")
        buf.write("\u0202\u0203\7 \2\2\u0203\25\3\2\2\2\u0204\u0205\7U\2")
        buf.write("\2\u0205\u0209\7\u00fb\2\2\u0206\u0207\7\32\2\2\u0207")
        buf.write("\u0208\7\u00fa\2\2\u0208\u020a\7\33\2\2\u0209\u0206\3")
        buf.write("\2\2\2\u0209\u020a\3\2\2\2\u020a\u020b\3\2\2\2\u020b\u020c")
        buf.write("\7 \2\2\u020c\27\3\2\2\2\u020d\u020e\7V\2\2\u020e\u0212")
        buf.write("\7\u00fb\2\2\u020f\u0210\7\32\2\2\u0210\u0211\7\u00fa")
        buf.write("\2\2\u0211\u0213\7\33\2\2\u0212\u020f\3\2\2\2\u0212\u0213")
        buf.write("\3\2\2\2\u0213\u0214\3\2\2\2\u0214\u0215\7 \2\2\u0215")
        buf.write("\31\3\2\2\2\u0216\u0217\7R\2\2\u0217\u021b\7\u00fb\2\2")
        buf.write("\u0218\u0219\7\32\2\2\u0219\u021a\7\u00fa\2\2\u021a\u021c")
        buf.write("\7\33\2\2\u021b\u0218\3\2\2\2\u021b\u021c\3\2\2\2\u021c")
        buf.write("\u021d\3\2\2\2\u021d\u021e\7 \2\2\u021e\33\3\2\2\2\u021f")
        buf.write("\u0220\7W\2\2\u0220\u0224\7\u00fb\2\2\u0221\u0222\7\32")
        buf.write("\2\2\u0222\u0223\7\u00fa\2\2\u0223\u0225\7\33\2\2\u0224")
        buf.write("\u0221\3\2\2\2\u0224\u0225\3\2\2\2\u0225\u0226\3\2\2\2")
        buf.write("\u0226\u0227\7 \2\2\u0227\35\3\2\2\2\u0228\u0229\7X\2")
        buf.write("\2\u0229\u022d\7\u00fb\2\2\u022a\u022b\7\32\2\2\u022b")
        buf.write("\u022c\7\u00fa\2\2\u022c\u022e\7\33\2\2\u022d\u022a\3")
        buf.write("\2\2\2\u022d\u022e\3\2\2\2\u022e\u022f\3\2\2\2\u022f\u0230")
        buf.write("\7 \2\2\u0230\37\3\2\2\2\u0231\u0232\7Y\2\2\u0232\u0236")
        buf.write("\7\u00fb\2\2\u0233\u0234\7\32\2\2\u0234\u0235\7\u00fa")
        buf.write("\2\2\u0235\u0237\7\33\2\2\u0236\u0233\3\2\2\2\u0236\u0237")
        buf.write("\3\2\2\2\u0237\u0238\3\2\2\2\u0238\u0239\7 \2\2\u0239")
        buf.write("!\3\2\2\2\u023a\u023b\7Z\2\2\u023b\u023f\7\u00fb\2\2\u023c")
        buf.write("\u023d\7\32\2\2\u023d\u023e\7\u00fa\2\2\u023e\u0240\7")
        buf.write("\33\2\2\u023f\u023c\3\2\2\2\u023f\u0240\3\2\2\2\u0240")
        buf.write("\u0241\3\2\2\2\u0241\u0242\7 \2\2\u0242#\3\2\2\2\u0243")
        buf.write("\u0244\7\u00fb\2\2\u0244\u0248\7\u00fb\2\2\u0245\u0246")
        buf.write("\7\32\2\2\u0246\u0247\7\u00fa\2\2\u0247\u0249\7\33\2\2")
        buf.write("\u0248\u0245\3\2\2\2\u0248\u0249\3\2\2\2\u0249\u024a\3")
        buf.write("\2\2\2\u024a\u024b\7 \2\2\u024b%\3\2\2\2\u024c\u024e\7")
        buf.write("S\2\2\u024d\u024f\7\u00fb\2\2\u024e\u024d\3\2\2\2\u024e")
        buf.write("\u024f\3\2\2\2\u024f\u0250\3\2\2\2\u0250\u0251\7\36\2")
        buf.write("\2\u0251\u0252\7\u00fa\2\2\u0252\u0253\7 \2\2\u0253\'")
        buf.write("\3\2\2\2\u0254\u0256\7T\2\2\u0255\u0257\7\u00fb\2\2\u0256")
        buf.write("\u0255\3\2\2\2\u0256\u0257\3\2\2\2\u0257\u0258\3\2\2\2")
        buf.write("\u0258\u0259\7\36\2\2\u0259\u025a\7\u00fa\2\2\u025a\u025b")
        buf.write("\7 \2\2\u025b)\3\2\2\2\u025c\u025e\7U\2\2\u025d\u025f")
        buf.write("\7\u00fb\2\2\u025e\u025d\3\2\2\2\u025e\u025f\3\2\2\2\u025f")
        buf.write("\u0260\3\2\2\2\u0260\u0261\7\36\2\2\u0261\u0262\7\u00fa")
        buf.write("\2\2\u0262\u0263\7 \2\2\u0263+\3\2\2\2\u0264\u0266\7V")
        buf.write("\2\2\u0265\u0267\7\u00fb\2\2\u0266\u0265\3\2\2\2\u0266")
        buf.write("\u0267\3\2\2\2\u0267\u0268\3\2\2\2\u0268\u0269\7\36\2")
        buf.write("\2\u0269\u026a\7\u00fa\2\2\u026a\u026b\7 \2\2\u026b-\3")
        buf.write("\2\2\2\u026c\u026d\7+\2\2\u026d\u026e\7[\2\2\u026e\u026f")
        buf.write("\7\b\2\2\u026f\u0270\5L\'\2\u0270\u0271\7!\2\2\u0271\u0272")
        buf.write("\7.\2\2\u0272\u0273\7\b\2\2\u0273\u0274\7\u00ec\2\2\u0274")
        buf.write("\u0275\7\30\2\2\u0275\u0276\7\u00fa\2\2\u0276\u0277\7")
        buf.write("\31\2\2\u0277\u0278\7!\2\2\u0278\u0279\7=\2\2\u0279\u027a")
        buf.write("\7\b\2\2\u027a\u027b\7\u00ec\2\2\u027b\u027c\7\30\2\2")
        buf.write("\u027c\u027d\7\u00fa\2\2\u027d\u027e\7\31\2\2\u027e\u0284")
        buf.write("\7!\2\2\u027f\u0280\7\u0083\2\2\u0280\u0281\7\b\2\2\u0281")
        buf.write("\u0282\5\60\31\2\u0282\u0283\7!\2\2\u0283\u0285\3\2\2")
        buf.write("\2\u0284\u027f\3\2\2\2\u0284\u0285\3\2\2\2\u0285\u028b")
        buf.write("\3\2\2\2\u0286\u0287\7\u0081\2\2\u0287\u0288\7\b\2\2\u0288")
        buf.write("\u0289\5\62\32\2\u0289\u028a\7!\2\2\u028a\u028c\3\2\2")
        buf.write("\2\u028b\u0286\3\2\2\2\u028b\u028c\3\2\2\2\u028c\u0292")
        buf.write("\3\2\2\2\u028d\u028e\7\u0082\2\2\u028e\u028f\7\b\2\2\u028f")
        buf.write("\u0290\5\66\34\2\u0290\u0291\7!\2\2\u0291\u0293\3\2\2")
        buf.write("\2\u0292\u028d\3\2\2\2\u0292\u0293\3\2\2\2\u0293\u0294")
        buf.write("\3\2\2\2\u0294\u0295\58\35\2\u0295\u0296\7-\2\2\u0296")
        buf.write("\u0297\7 \2\2\u0297/\3\2\2\2\u0298\u029b\5L\'\2\u0299")
        buf.write("\u029a\7(\2\2\u029a\u029c\5L\'\2\u029b\u0299\3\2\2\2\u029b")
        buf.write("\u029c\3\2\2\2\u029c\u029f\3\2\2\2\u029d\u029e\7(\2\2")
        buf.write("\u029e\u02a0\5L\'\2\u029f\u029d\3\2\2\2\u029f\u02a0\3")
        buf.write("\2\2\2\u02a0\u02a3\3\2\2\2\u02a1\u02a2\7(\2\2\u02a2\u02a4")
        buf.write("\5L\'\2\u02a3\u02a1\3\2\2\2\u02a3\u02a4\3\2\2\2\u02a4")
        buf.write("\61\3\2\2\2\u02a5\u02aa\5\64\33\2\u02a6\u02a7\7(\2\2\u02a7")
        buf.write("\u02a9\5\64\33\2\u02a8\u02a6\3\2\2\2\u02a9\u02ac\3\2\2")
        buf.write("\2\u02aa\u02a8\3\2\2\2\u02aa\u02ab\3\2\2\2\u02ab\63\3")
        buf.write("\2\2\2\u02ac\u02aa\3\2\2\2\u02ad\u02ae\t\3\2\2\u02ae\65")
        buf.write("\3\2\2\2\u02af\u02b0\t\4\2\2\u02b0\67\3\2\2\2\u02b1\u02b3")
        buf.write("\5:\36\2\u02b2\u02b1\3\2\2\2\u02b3\u02b6\3\2\2\2\u02b4")
        buf.write("\u02b2\3\2\2\2\u02b4\u02b5\3\2\2\2\u02b59\3\2\2\2\u02b6")
        buf.write("\u02b4\3\2\2\2\u02b7\u02c2\5d\63\2\u02b8\u02c2\5h\65\2")
        buf.write("\u02b9\u02c2\5j\66\2\u02ba\u02c2\5> \2\u02bb\u02c2\5@")
        buf.write("!\2\u02bc\u02c2\5D#\2\u02bd\u02c2\5<\37\2\u02be\u02c2")
        buf.write("\5F$\2\u02bf\u02c2\5H%\2\u02c0\u02c2\5\u011c\u008f\2\u02c1")
        buf.write("\u02b7\3\2\2\2\u02c1\u02b8\3\2\2\2\u02c1\u02b9\3\2\2\2")
        buf.write("\u02c1\u02ba\3\2\2\2\u02c1\u02bb\3\2\2\2\u02c1\u02bc\3")
        buf.write("\2\2\2\u02c1\u02bd\3\2\2\2\u02c1\u02be\3\2\2\2\u02c1\u02bf")
        buf.write("\3\2\2\2\u02c1\u02c0\3\2\2\2\u02c2;\3\2\2\2\u02c3\u02c4")
        buf.write("\7\u009b\2\2\u02c4\u02c5\7\u00fb\2\2\u02c5\u02c6\7!\2")
        buf.write("\2\u02c6\u02c7\7\62\2\2\u02c7\u02c8\7\b\2\2\u02c8\u02c9")
        buf.write("\7\u00ec\2\2\u02c9\u02ca\7\30\2\2\u02ca\u02cb\7\u00fa")
        buf.write("\2\2\u02cb\u02d0\7\31\2\2\u02cc\u02cd\7!\2\2\u02cd\u02ce")
        buf.write("\7\u009c\2\2\u02ce\u02cf\7\b\2\2\u02cf\u02d1\7\u00fa\2")
        buf.write("\2\u02d0\u02cc\3\2\2\2\u02d0\u02d1\3\2\2\2\u02d1\u02d2")
        buf.write("\3\2\2\2\u02d2\u02d3\7 \2\2\u02d3=\3\2\2\2\u02d4\u02e5")
        buf.write("\7\u009d\2\2\u02d5\u02d6\7\u00fb\2\2\u02d6\u02e6\7!\2")
        buf.write("\2\u02d7\u02d8\7V\2\2\u02d8\u02e6\7!\2\2\u02d9\u02da\7")
        buf.write("U\2\2\u02da\u02e6\7!\2\2\u02db\u02dc\7T\2\2\u02dc\u02e6")
        buf.write("\7!\2\2\u02dd\u02de\7S\2\2\u02de\u02e6\7!\2\2\u02df\u02e0")
        buf.write("\7X\2\2\u02e0\u02e6\7!\2\2\u02e1\u02e2\7Y\2\2\u02e2\u02e6")
        buf.write("\7!\2\2\u02e3\u02e4\7Z\2\2\u02e4\u02e6\7!\2\2\u02e5\u02d5")
        buf.write("\3\2\2\2\u02e5\u02d7\3\2\2\2\u02e5\u02d9\3\2\2\2\u02e5")
        buf.write("\u02db\3\2\2\2\u02e5\u02dd\3\2\2\2\u02e5\u02df\3\2\2\2")
        buf.write("\u02e5\u02e1\3\2\2\2\u02e5\u02e3\3\2\2\2\u02e6\u02eb\3")
        buf.write("\2\2\2\u02e7\u02e8\7\u008f\2\2\u02e8\u02e9\7\b\2\2\u02e9")
        buf.write("\u02ea\7\u00fa\2\2\u02ea\u02ec\7!\2\2\u02eb\u02e7\3\2")
        buf.write("\2\2\u02eb\u02ec\3\2\2\2\u02ec\u02ed\3\2\2\2\u02ed\u02ee")
        buf.write("\7\u008e\2\2\u02ee\u02ef\7\b\2\2\u02ef\u02f0\7\u00fb\2")
        buf.write("\2\u02f0\u02f1\7!\2\2\u02f1\u02f2\7[\2\2\u02f2\u02f3\7")
        buf.write("\b\2\2\u02f3\u02f4\5L\'\2\u02f4\u02f5\7 \2\2\u02f5?\3")
        buf.write("\2\2\2\u02f6\u0307\7\u009e\2\2\u02f7\u02f8\7\u00fb\2\2")
        buf.write("\u02f8\u0308\7!\2\2\u02f9\u02fa\7V\2\2\u02fa\u0308\7!")
        buf.write("\2\2\u02fb\u02fc\7U\2\2\u02fc\u0308\7!\2\2\u02fd\u02fe")
        buf.write("\7T\2\2\u02fe\u0308\7!\2\2\u02ff\u0300\7S\2\2\u0300\u0308")
        buf.write("\7!\2\2\u0301\u0302\7X\2\2\u0302\u0308\7!\2\2\u0303\u0304")
        buf.write("\7Y\2\2\u0304\u0308\7!\2\2\u0305\u0306\7Z\2\2\u0306\u0308")
        buf.write("\7!\2\2\u0307\u02f7\3\2\2\2\u0307\u02f9\3\2\2\2\u0307")
        buf.write("\u02fb\3\2\2\2\u0307\u02fd\3\2\2\2\u0307\u02ff\3\2\2\2")
        buf.write("\u0307\u0301\3\2\2\2\u0307\u0303\3\2\2\2\u0307\u0305\3")
        buf.write("\2\2\2\u0308\u030d\3\2\2\2\u0309\u030a\7\u008f\2\2\u030a")
        buf.write("\u030b\7\b\2\2\u030b\u030c\7\u00fa\2\2\u030c\u030e\7!")
        buf.write("\2\2\u030d\u0309\3\2\2\2\u030d\u030e\3\2\2\2\u030e\u030f")
        buf.write("\3\2\2\2\u030f\u0310\7\u009c\2\2\u0310\u0311\7\b\2\2\u0311")
        buf.write("\u0316\5B\"\2\u0312\u0313\7(\2\2\u0313\u0315\5B\"\2\u0314")
        buf.write("\u0312\3\2\2\2\u0315\u0318\3\2\2\2\u0316\u0314\3\2\2\2")
        buf.write("\u0316\u0317\3\2\2\2\u0317\u0319\3\2\2\2\u0318\u0316\3")
        buf.write("\2\2\2\u0319\u0329\7!\2\2\u031a\u031b\7\u008e\2\2\u031b")
        buf.write("\u031c\7\b\2\2\u031c\u031d\7\u00fb\2\2\u031d\u032a\7!")
        buf.write("\2\2\u031e\u031f\7\u008e\2\2\u031f\u0320\7\b\2\2\u0320")
        buf.write("\u0321\7\u00ec\2\2\u0321\u0322\7\30\2\2\u0322\u0323\7")
        buf.write("\u00fa\2\2\u0323\u0324\7\31\2\2\u0324\u0325\7!\2\2\u0325")
        buf.write("\u0326\7\u009f\2\2\u0326\u0327\7\b\2\2\u0327\u0328\7\u00fa")
        buf.write("\2\2\u0328\u032a\7!\2\2\u0329\u031a\3\2\2\2\u0329\u031e")
        buf.write("\3\2\2\2\u032a\u032b\3\2\2\2\u032b\u032c\7[\2\2\u032c")
        buf.write("\u032d\7\b\2\2\u032d\u032e\5L\'\2\u032e\u032f\7 \2\2\u032f")
        buf.write("A\3\2\2\2\u0330\u0331\7\u00fa\2\2\u0331C\3\2\2\2\u0332")
        buf.write("\u0333\7\u00a0\2\2\u0333\u0334\7\u00fb\2\2\u0334\u0339")
        buf.write("\7!\2\2\u0335\u0336\7\u008f\2\2\u0336\u0337\7\b\2\2\u0337")
        buf.write("\u0338\7\u00fa\2\2\u0338\u033a\7!\2\2\u0339\u0335\3\2")
        buf.write("\2\2\u0339\u033a\3\2\2\2\u033a\u0342\3\2\2\2\u033b\u033c")
        buf.write("\7\u008e\2\2\u033c\u033d\7\b\2\2\u033d\u033e\7\u00ec\2")
        buf.write("\2\u033e\u033f\7\30\2\2\u033f\u0340\7\u00fa\2\2\u0340")
        buf.write("\u0341\7\31\2\2\u0341\u0343\7!\2\2\u0342\u033b\3\2\2\2")
        buf.write("\u0343\u0344\3\2\2\2\u0344\u0342\3\2\2\2\u0344\u0345\3")
        buf.write("\2\2\2\u0345\u0346\3\2\2\2\u0346\u0347\7[\2\2\u0347\u0348")
        buf.write("\7\b\2\2\u0348\u0349\5L\'\2\u0349\u034a\7 \2\2\u034aE")
        buf.write("\3\2\2\2\u034b\u034c\7l\2\2\u034c\u034d\5\u0124\u0093")
        buf.write("\2\u034d\u034e\7 \2\2\u034e\u034f\58\35\2\u034f\u0350")
        buf.write("\7s\2\2\u0350\u0351\7 \2\2\u0351G\3\2\2\2\u0352\u0353")
        buf.write("\7k\2\2\u0353\u0354\5\u0124\u0093\2\u0354\u0355\7 \2\2")
        buf.write("\u0355\u0356\58\35\2\u0356\u0357\7s\2\2\u0357\u0358\7")
        buf.write(" \2\2\u0358I\3\2\2\2\u0359\u035a\7\u00fa\2\2\u035a\u035b")
        buf.write("\7!\2\2\u035b\u035c\7\u00fa\2\2\u035c\u035d\7!\2\2\u035d")
        buf.write("\u035e\7\u00fa\2\2\u035e\u035f\7!\2\2\u035f\u0360\7\u00fa")
        buf.write("\2\2\u0360\u0361\7!\2\2\u0361\u0362\7\u00fa\2\2\u0362")
        buf.write("\u0363\7!\2\2\u0363\u0364\7\u00fa\2\2\u0364\u0365\7!\2")
        buf.write("\2\u0365\u0366\7\u00fa\2\2\u0366\u0367\7!\2\2\u0367\u0368")
        buf.write("\7\u00fa\2\2\u0368K\3\2\2\2\u0369\u036a\7\26\2\2\u036a")
        buf.write("\u036b\7\u00fa\2\2\u036b\u036c\7!\2\2\u036c\u036d\7\u00fa")
        buf.write("\2\2\u036d\u036e\7!\2\2\u036e\u036f\7\u00fa\2\2\u036f")
        buf.write("\u0375\7!\2\2\u0370\u0371\7\26\2\2\u0371\u0372\5J&\2\u0372")
        buf.write("\u0373\7\27\2\2\u0373\u0376\3\2\2\2\u0374\u0376\5J&\2")
        buf.write("\u0375\u0370\3\2\2\2\u0375\u0374\3\2\2\2\u0376\u0377\3")
        buf.write("\2\2\2\u0377\u0378\7\27\2\2\u0378M\3\2\2\2\u0379\u037a")
        buf.write("\7\u00ec\2\2\u037a\u037b\7\30\2\2\u037b\u037c\7\u00fa")
        buf.write("\2\2\u037c\u037d\7\31\2\2\u037dO\3\2\2\2\u037e\u037f\5")
        buf.write("R*\2\u037f\u0380\5T+\2\u0380Q\3\2\2\2\u0381\u0382\7\u008e")
        buf.write("\2\2\u0382\u0383\7\b\2\2\u0383\u0384\7\u00fb\2\2\u0384")
        buf.write("\u0386\7!\2\2\u0385\u0381\3\2\2\2\u0385\u0386\3\2\2\2")
        buf.write("\u0386\u038c\3\2\2\2\u0387\u0388\7\u008f\2\2\u0388\u0389")
        buf.write("\7\b\2\2\u0389\u038a\5X-\2\u038a\u038b\7!\2\2\u038b\u038d")
        buf.write("\3\2\2\2\u038c\u0387\3\2\2\2\u038c\u038d\3\2\2\2\u038d")
        buf.write("\u0392\3\2\2\2\u038e\u038f\7\u0091\2\2\u038f\u0390\7\b")
        buf.write("\2\2\u0390\u0391\7\u00fa\2\2\u0391\u0393\7!\2\2\u0392")
        buf.write("\u038e\3\2\2\2\u0392\u0393\3\2\2\2\u0393S\3\2\2\2\u0394")
        buf.write("\u0395\7\62\2\2\u0395\u0396\7\b\2\2\u0396\u0397\7\u00ec")
        buf.write("\2\2\u0397\u0398\7\30\2\2\u0398\u0399\7\u00fa\2\2\u0399")
        buf.write("\u039a\7\31\2\2\u039a\u039b\7!\2\2\u039b\u039c\7=\2\2")
        buf.write("\u039c\u039d\7\b\2\2\u039d\u039e\7\u00ec\2\2\u039e\u039f")
        buf.write("\7\30\2\2\u039f\u03a0\7\u00fa\2\2\u03a0\u03a1\7\31\2\2")
        buf.write("\u03a1U\3\2\2\2\u03a2\u03ab\7~\2\2\u03a3\u03ab\7y\2\2")
        buf.write("\u03a4\u03ab\7{\2\2\u03a5\u03ab\7\u0080\2\2\u03a6\u03ab")
        buf.write("\7|\2\2\u03a7\u03ab\7\177\2\2\u03a8\u03ab\7z\2\2\u03a9")
        buf.write("\u03ab\7}\2\2\u03aa\u03a2\3\2\2\2\u03aa\u03a3\3\2\2\2")
        buf.write("\u03aa\u03a4\3\2\2\2\u03aa\u03a5\3\2\2\2\u03aa\u03a6\3")
        buf.write("\2\2\2\u03aa\u03a7\3\2\2\2\u03aa\u03a8\3\2\2\2\u03aa\u03a9")
        buf.write("\3\2\2\2\u03abW\3\2\2\2\u03ac\u03ad\7\u00fb\2\2\u03ad")
        buf.write("\u03ae\7\32\2\2\u03ae\u03af\7\u00fa\2\2\u03af\u03b9\7")
        buf.write("\33\2\2\u03b0\u03b5\7\u00fb\2\2\u03b1\u03b2\7\34\2\2\u03b2")
        buf.write("\u03b4\5\u015e\u00b0\2\u03b3\u03b1\3\2\2\2\u03b4\u03b7")
        buf.write("\3\2\2\2\u03b5\u03b3\3\2\2\2\u03b5\u03b6\3\2\2\2\u03b6")
        buf.write("\u03b9\3\2\2\2\u03b7\u03b5\3\2\2\2\u03b8\u03ac\3\2\2\2")
        buf.write("\u03b8\u03b0\3\2\2\2\u03b9Y\3\2\2\2\u03ba\u03bc\7\35\2")
        buf.write("\2\u03bb\u03ba\3\2\2\2\u03bb\u03bc\3\2\2\2\u03bc\u03bd")
        buf.write("\3\2\2\2\u03bd\u03e7\7\u00fa\2\2\u03be\u03e7\7\u00d3\2")
        buf.write("\2\u03bf\u03e7\7\u00d4\2\2\u03c0\u03e7\7\u00d5\2\2\u03c1")
        buf.write("\u03e7\7\u00d6\2\2\u03c2\u03e7\7\u00d7\2\2\u03c3\u03c4")
        buf.write("\7\u00fa\2\2\u03c4\u03c5\7\36\2\2\u03c5\u03c6\7\u00fa")
        buf.write("\2\2\u03c6\u03c7\7\36\2\2\u03c7\u03e7\7\u00fa\2\2\u03c8")
        buf.write("\u03c9\7\u00fa\2\2\u03c9\u03ca\7\37\2\2\u03ca\u03cb\7")
        buf.write("\u00fa\2\2\u03cb\u03cc\7\37\2\2\u03cc\u03e7\7\u00fa\2")
        buf.write("\2\u03cd\u03ce\7\u00fa\2\2\u03ce\u03cf\7 \2\2\u03cf\u03d0")
        buf.write("\7\u00fa\2\2\u03d0\u03d1\7 \2\2\u03d1\u03d2\5L\'\2\u03d2")
        buf.write("\u03d3\7 \2\2\u03d3\u03d4\7\u00ec\2\2\u03d4\u03d5\7\30")
        buf.write("\2\2\u03d5\u03d6\7\u00fa\2\2\u03d6\u03d7\7\31\2\2\u03d7")
        buf.write("\u03e7\3\2\2\2\u03d8\u03d9\7\u00ec\2\2\u03d9\u03da\7\30")
        buf.write("\2\2\u03da\u03db\7\u00fa\2\2\u03db\u03e7\7\31\2\2\u03dc")
        buf.write("\u03dd\7\26\2\2\u03dd\u03e2\7\u00fa\2\2\u03de\u03df\7")
        buf.write("!\2\2\u03df\u03e1\7\u00fa\2\2\u03e0\u03de\3\2\2\2\u03e1")
        buf.write("\u03e4\3\2\2\2\u03e2\u03e0\3\2\2\2\u03e2\u03e3\3\2\2\2")
        buf.write("\u03e3\u03e5\3\2\2\2\u03e4\u03e2\3\2\2\2\u03e5\u03e7\7")
        buf.write("\27\2\2\u03e6\u03bb\3\2\2\2\u03e6\u03be\3\2\2\2\u03e6")
        buf.write("\u03bf\3\2\2\2\u03e6\u03c0\3\2\2\2\u03e6\u03c1\3\2\2\2")
        buf.write("\u03e6\u03c2\3\2\2\2\u03e6\u03c3\3\2\2\2\u03e6\u03c8\3")
        buf.write("\2\2\2\u03e6\u03cd\3\2\2\2\u03e6\u03d8\3\2\2\2\u03e6\u03dc")
        buf.write("\3\2\2\2\u03e7[\3\2\2\2\u03e8\u03e9\7\u0092\2\2\u03e9")
        buf.write("\u03ea\7\b\2\2\u03ea\u03eb\7\t\2\2\u03eb\u03ec\7\30\2")
        buf.write("\2\u03ec\u03ed\7\u00fa\2\2\u03ed\u03ee\7\31\2\2\u03ee")
        buf.write("]\3\2\2\2\u03ef\u03f0\7\u0093\2\2\u03f0_\3\2\2\2\u03f1")
        buf.write("\u03f4\5\\/\2\u03f2\u03f4\5^\60\2\u03f3\u03f1\3\2\2\2")
        buf.write("\u03f3\u03f2\3\2\2\2\u03f4a\3\2\2\2\u03f5\u03fa\5`\61")
        buf.write("\2\u03f6\u03f7\7!\2\2\u03f7\u03f9\5`\61\2\u03f8\u03f6")
        buf.write("\3\2\2\2\u03f9\u03fc\3\2\2\2\u03fa\u03f8\3\2\2\2\u03fa")
        buf.write("\u03fb\3\2\2\2\u03fbc\3\2\2\2\u03fc\u03fa\3\2\2\2\u03fd")
        buf.write("\u03fe\7\67\2\2\u03fe\u03ff\7/\2\2\u03ff\u0400\7\b\2\2")
        buf.write("\u0400\u0401\7\u00fa\2\2\u0401\u0402\7!\2\2\u0402\u0403")
        buf.write("\7.\2\2\u0403\u0404\7\b\2\2\u0404\u0405\7\u00ec\2\2\u0405")
        buf.write("\u0406\7\30\2\2\u0406\u0407\7\u00fa\2\2\u0407\u0408\7")
        buf.write("\31\2\2\u0408\u040c\7 \2\2\u0409\u040b\5f\64\2\u040a\u0409")
        buf.write("\3\2\2\2\u040b\u040e\3\2\2\2\u040c\u040a\3\2\2\2\u040c")
        buf.write("\u040d\3\2\2\2\u040d\u040f\3\2\2\2\u040e\u040c\3\2\2\2")
        buf.write("\u040f\u0410\7\66\2\2\u0410\u0411\7 \2\2\u0411e\3\2\2")
        buf.write("\2\u0412\u0422\5j\66\2\u0413\u0422\5l\67\2\u0414\u0422")
        buf.write("\5n8\2\u0415\u0422\5\u00a4S\2\u0416\u0422\5p9\2\u0417")
        buf.write("\u0422\5\u0088E\2\u0418\u0422\5\u00f8}\2\u0419\u0422\5")
        buf.write("\u0118\u008d\2\u041a\u0422\5\u011a\u008e\2\u041b\u0422")
        buf.write("\5\u0110\u0089\2\u041c\u0422\5\u011c\u008f\2\u041d\u0422")
        buf.write("\5\u0120\u0091\2\u041e\u041f\5\u0096L\2\u041f\u0420\7")
        buf.write(" \2\2\u0420\u0422\3\2\2\2\u0421\u0412\3\2\2\2\u0421\u0413")
        buf.write("\3\2\2\2\u0421\u0414\3\2\2\2\u0421\u0415\3\2\2\2\u0421")
        buf.write("\u0416\3\2\2\2\u0421\u0417\3\2\2\2\u0421\u0418\3\2\2\2")
        buf.write("\u0421\u0419\3\2\2\2\u0421\u041a\3\2\2\2\u0421\u041b\3")
        buf.write("\2\2\2\u0421\u041c\3\2\2\2\u0421\u041d\3\2\2\2\u0421\u041e")
        buf.write("\3\2\2\2\u0422g\3\2\2\2\u0423\u0424\78\2\2\u0424\u0425")
        buf.write("\7/\2\2\u0425\u0426\7\b\2\2\u0426\u0427\7\u00fa\2\2\u0427")
        buf.write("\u0436\7!\2\2\u0428\u0429\79\2\2\u0429\u042a\7\b\2\2\u042a")
        buf.write("\u042b\7\u00ec\2\2\u042b\u042c\7\30\2\2\u042c\u042d\7")
        buf.write("\u00fa\2\2\u042d\u042e\7\31\2\2\u042e\u042f\7 \2\2\u042f")
        buf.write("\u0430\7:\2\2\u0430\u0431\7\b\2\2\u0431\u0432\5L\'\2\u0432")
        buf.write("\u0433\7 \2\2\u0433\u0435\3\2\2\2\u0434\u0428\3\2\2\2")
        buf.write("\u0435\u0438\3\2\2\2\u0436\u0434\3\2\2\2\u0436\u0437\3")
        buf.write("\2\2\2\u0437\u043c\3\2\2\2\u0438\u0436\3\2\2\2\u0439\u043b")
        buf.write("\5f\64\2\u043a\u0439\3\2\2\2\u043b\u043e\3\2\2\2\u043c")
        buf.write("\u043a\3\2\2\2\u043c\u043d\3\2\2\2\u043d\u043f\3\2\2\2")
        buf.write("\u043e\u043c\3\2\2\2\u043f\u0440\7\66\2\2\u0440\u0441")
        buf.write("\7 \2\2\u0441i\3\2\2\2\u0442\u0443\5\\/\2\u0443\u0444")
        buf.write("\7 \2\2\u0444k\3\2\2\2\u0445\u0446\5^\60\2\u0446\u0447")
        buf.write("\7 \2\2\u0447m\3\2\2\2\u0448\u0449\7\u0094\2\2\u0449\u044a")
        buf.write("\7\u00fb\2\2\u044a\u044b\7!\2\2\u044b\u044c\5\u0124\u0093")
        buf.write("\2\u044c\u044d\7\u0095\2\2\u044d\u044e\7 \2\2\u044eo\3")
        buf.write("\2\2\2\u044f\u0453\5r:\2\u0450\u0453\5z>\2\u0451\u0453")
        buf.write("\5~@\2\u0452\u044f\3\2\2\2\u0452\u0450\3\2\2\2\u0452\u0451")
        buf.write("\3\2\2\2\u0453q\3\2\2\2\u0454\u0455\7;\2\2\u0455\u0456")
        buf.write("\7>\2\2\u0456\u0457\7\b\2\2\u0457\u0458\7\u00ec\2\2\u0458")
        buf.write("\u0459\7\30\2\2\u0459\u045a\7\u00fa\2\2\u045a\u045f\7")
        buf.write("\31\2\2\u045b\u045c\7!\2\2\u045c\u045d\7@\2\2\u045d\u045e")
        buf.write("\7\b\2\2\u045e\u0460\5v<\2\u045f\u045b\3\2\2\2\u045f\u0460")
        buf.write("\3\2\2\2\u0460\u0475\3\2\2\2\u0461\u0462\7!\2\2\u0462")
        buf.write("\u0464\5b\62\2\u0463\u0461\3\2\2\2\u0463\u0464\3\2\2\2")
        buf.write("\u0464\u0465\3\2\2\2\u0465\u0476\7 \2\2\u0466\u0467\7")
        buf.write("!\2\2\u0467\u0469\5b\62\2\u0468\u0466\3\2\2\2\u0468\u0469")
        buf.write("\3\2\2\2\u0469\u0471\3\2\2\2\u046a\u046e\7!\2\2\u046b")
        buf.write("\u046d\5t;\2\u046c\u046b\3\2\2\2\u046d\u0470\3\2\2\2\u046e")
        buf.write("\u046c\3\2\2\2\u046e\u046f\3\2\2\2\u046f\u0472\3\2\2\2")
        buf.write("\u0470\u046e\3\2\2\2\u0471\u046a\3\2\2\2\u0471\u0472\3")
        buf.write("\2\2\2\u0472\u0473\3\2\2\2\u0473\u0474\7<\2\2\u0474\u0476")
        buf.write("\7 \2\2\u0475\u0463\3\2\2\2\u0475\u0468\3\2\2\2\u0476")
        buf.write("s\3\2\2\2\u0477\u047a\5p9\2\u0478\u047a\5\u0088E\2\u0479")
        buf.write("\u0477\3\2\2\2\u0479\u0478\3\2\2\2\u047au\3\2\2\2\u047b")
        buf.write("\u0480\5x=\2\u047c\u047d\7(\2\2\u047d\u047f\5x=\2\u047e")
        buf.write("\u047c\3\2\2\2\u047f\u0482\3\2\2\2\u0480\u047e\3\2\2\2")
        buf.write("\u0480\u0481\3\2\2\2\u0481w\3\2\2\2\u0482\u0480\3\2\2")
        buf.write("\2\u0483\u0484\t\5\2\2\u0484y\3\2\2\2\u0485\u0486\7>\2")
        buf.write("\2\u0486\u0487\7=\2\2\u0487\u0488\7\b\2\2\u0488\u0489")
        buf.write("\7\u00ec\2\2\u0489\u048a\7\30\2\2\u048a\u048b\7\u00fa")
        buf.write("\2\2\u048b\u048c\7\31\2\2\u048c\u048d\7!\2\2\u048d\u048e")
        buf.write("\7>\2\2\u048e\u048f\7\b\2\2\u048f\u0490\7\u00ec\2\2\u0490")
        buf.write("\u0491\7\30\2\2\u0491\u0492\7\u00fa\2\2\u0492\u049a\7")
        buf.write("\31\2\2\u0493\u0494\7!\2\2\u0494\u0495\7>\2\2\u0495\u0496")
        buf.write("\7\b\2\2\u0496\u0497\7\u00ec\2\2\u0497\u0498\7\30\2\2")
        buf.write("\u0498\u0499\7\u00fa\2\2\u0499\u049b\7\31\2\2\u049a\u0493")
        buf.write("\3\2\2\2\u049a\u049b\3\2\2\2\u049b\u04ac\3\2\2\2\u049c")
        buf.write("\u049d\7!\2\2\u049d\u049e\7@\2\2\u049e\u049f\7\b\2\2\u049f")
        buf.write("\u04a4\5|?\2\u04a0\u04a1\7(\2\2\u04a1\u04a3\5|?\2\u04a2")
        buf.write("\u04a0\3\2\2\2\u04a3\u04a6\3\2\2\2\u04a4\u04a2\3\2\2\2")
        buf.write("\u04a4\u04a5\3\2\2\2\u04a5\u04a7\3\2\2\2\u04a6\u04a4\3")
        buf.write("\2\2\2\u04a7\u04a8\7!\2\2\u04a8\u04a9\7t\2\2\u04a9\u04aa")
        buf.write("\7\b\2\2\u04aa\u04ab\7\u00fa\2\2\u04ab\u04ad\3\2\2\2\u04ac")
        buf.write("\u049c\3\2\2\2\u04ac\u04ad\3\2\2\2\u04ad\u04b0\3\2\2\2")
        buf.write("\u04ae\u04af\7!\2\2\u04af\u04b1\5b\62\2\u04b0\u04ae\3")
        buf.write("\2\2\2\u04b0\u04b1\3\2\2\2\u04b1\u04b2\3\2\2\2\u04b2\u04b3")
        buf.write("\7 \2\2\u04b3{\3\2\2\2\u04b4\u04b7\7\u00fa\2\2\u04b5\u04b7")
        buf.write("\5V,\2\u04b6\u04b4\3\2\2\2\u04b6\u04b5\3\2\2\2\u04b7}")
        buf.write("\3\2\2\2\u04b8\u04bb\5\u0080A\2\u04b9\u04bb\5\u0086D\2")
        buf.write("\u04ba\u04b8\3\2\2\2\u04ba\u04b9\3\2\2\2\u04bb\177\3\2")
        buf.write("\2\2\u04bc\u04ec\7n\2\2\u04bd\u04be\7*\2\2\u04be\u04bf")
        buf.write("\7\b\2\2\u04bf\u04c0\7\u00ec\2\2\u04c0\u04c1\7\30\2\2")
        buf.write("\u04c1\u04c2\7\u00fa\2\2\u04c2\u04c3\7\31\2\2\u04c3\u04c4")
        buf.write("\7!\2\2\u04c4\u04c5\7o\2\2\u04c5\u04c6\7\b\2\2\u04c6\u04c7")
        buf.write("\5L\'\2\u04c7\u04c8\7!\2\2\u04c8\u04c9\7/\2\2\u04c9\u04ca")
        buf.write("\7\b\2\2\u04ca\u04cb\7\u00fa\2\2\u04cb\u04cc\7!\2\2\u04cc")
        buf.write("\u04cd\7\u0090\2\2\u04cd\u04ce\7\b\2\2\u04ce\u04cf\7\u00fa")
        buf.write("\2\2\u04cf\u04d0\7!\2\2\u04d0\u04ed\3\2\2\2\u04d1\u04d2")
        buf.write("\7o\2\2\u04d2\u04d3\7\b\2\2\u04d3\u04d4\5L\'\2\u04d4\u04d5")
        buf.write("\7!\2\2\u04d5\u04d6\7/\2\2\u04d6\u04d7\7\b\2\2\u04d7\u04d8")
        buf.write("\7\u00fa\2\2\u04d8\u04d9\7!\2\2\u04d9\u04da\7\u0090\2")
        buf.write("\2\u04da\u04db\7\b\2\2\u04db\u04dc\7\u00fa\2\2\u04dc\u04dd")
        buf.write("\7!\2\2\u04dd\u04ed\3\2\2\2\u04de\u04df\7/\2\2\u04df\u04e0")
        buf.write("\7\b\2\2\u04e0\u04e1\7\u00fa\2\2\u04e1\u04e2\7!\2\2\u04e2")
        buf.write("\u04e3\7\u0090\2\2\u04e3\u04e8\7\b\2\2\u04e4\u04e5\7\u00fb")
        buf.write("\2\2\u04e5\u04e9\7!\2\2\u04e6\u04e7\7\u00fa\2\2\u04e7")
        buf.write("\u04e9\7!\2\2\u04e8\u04e4\3\2\2\2\u04e8\u04e6\3\2\2\2")
        buf.write("\u04e9\u04ed\3\2\2\2\u04ea\u04eb\7\u00fa\2\2\u04eb\u04ed")
        buf.write("\7!\2\2\u04ec\u04bd\3\2\2\2\u04ec\u04d1\3\2\2\2\u04ec")
        buf.write("\u04de\3\2\2\2\u04ec\u04ea\3\2\2\2\u04ec\u04ed\3\2\2\2")
        buf.write("\u04ed\u04ee\3\2\2\2\u04ee\u04f3\5P)\2\u04ef\u04f0\7!")
        buf.write("\2\2\u04f0\u04f1\7@\2\2\u04f1\u04f2\7\b\2\2\u04f2\u04f4")
        buf.write("\5\u0082B\2\u04f3\u04ef\3\2\2\2\u04f3\u04f4\3\2\2\2\u04f4")
        buf.write("\u04f9\3\2\2\2\u04f5\u04f6\7!\2\2\u04f6\u04f7\7t\2\2\u04f7")
        buf.write("\u04f8\7\b\2\2\u04f8\u04fa\7\u00fa\2\2\u04f9\u04f5\3\2")
        buf.write("\2\2\u04f9\u04fa\3\2\2\2\u04fa\u04fd\3\2\2\2\u04fb\u04fc")
        buf.write("\7!\2\2\u04fc\u04fe\5\u00b4[\2\u04fd\u04fb\3\2\2\2\u04fd")
        buf.write("\u04fe\3\2\2\2\u04fe\u04ff\3\2\2\2\u04ff\u0500\7 \2\2")
        buf.write("\u0500\u0081\3\2\2\2\u0501\u0506\5\u0084C\2\u0502\u0503")
        buf.write("\7(\2\2\u0503\u0505\5\u0084C\2\u0504\u0502\3\2\2\2\u0505")
        buf.write("\u0508\3\2\2\2\u0506\u0504\3\2\2\2\u0506\u0507\3\2\2\2")
        buf.write("\u0507\u0083\3\2\2\2\u0508\u0506\3\2\2\2\u0509\u050c\7")
        buf.write("\u00fa\2\2\u050a\u050c\5V,\2\u050b\u0509\3\2\2\2\u050b")
        buf.write("\u050a\3\2\2\2\u050c\u0085\3\2\2\2\u050d\u050e\7\u0099")
        buf.write("\2\2\u050e\u050f\7\u009b\2\2\u050f\u0510\7\b\2\2\u0510")
        buf.write("\u0511\7\u00fb\2\2\u0511\u0512\7!\2\2\u0512\u0513\5T+")
        buf.write("\2\u0513\u0517\7!\2\2\u0514\u0515\5b\62\2\u0515\u0516")
        buf.write("\7!\2\2\u0516\u0518\3\2\2\2\u0517\u0514\3\2\2\2\u0517")
        buf.write("\u0518\3\2\2\2\u0518\u0519\3\2\2\2\u0519\u051a\7\u009a")
        buf.write("\2\2\u051a\u051b\7 \2\2\u051b\u0087\3\2\2\2\u051c\u0523")
        buf.write("\5\u00b8]\2\u051d\u0523\5\u00e8u\2\u051e\u0523\5\u00c6")
        buf.write("d\2\u051f\u0523\5\u00d4k\2\u0520\u0523\5\u00e2r\2\u0521")
        buf.write("\u0523\5\u00f0y\2\u0522\u051c\3\2\2\2\u0522\u051d\3\2")
        buf.write("\2\2\u0522\u051e\3\2\2\2\u0522\u051f\3\2\2\2\u0522\u0520")
        buf.write("\3\2\2\2\u0522\u0521\3\2\2\2\u0523\u0089\3\2\2\2\u0524")
        buf.write("\u0525\5`\61\2\u0525\u0526\7!\2\2\u0526\u0530\3\2\2\2")
        buf.write("\u0527\u0530\5\u008cG\2\u0528\u0530\5\u008eH\2\u0529\u0530")
        buf.write("\5\u0090I\2\u052a\u0530\5\u0092J\2\u052b\u0530\5\u0094")
        buf.write("K\2\u052c\u0530\5\u011c\u008f\2\u052d\u0530\5\u0096L\2")
        buf.write("\u052e\u0530\5\u0098M\2\u052f\u0524\3\2\2\2\u052f\u0527")
        buf.write("\3\2\2\2\u052f\u0528\3\2\2\2\u052f\u0529\3\2\2\2\u052f")
        buf.write("\u052a\3\2\2\2\u052f\u052b\3\2\2\2\u052f\u052c\3\2\2\2")
        buf.write("\u052f\u052d\3\2\2\2\u052f\u052e\3\2\2\2\u0530\u008b\3")
        buf.write("\2\2\2\u0531\u0532\7p\2\2\u0532\u0533\7\62\2\2\u0533\u0534")
        buf.write("\7\b\2\2\u0534\u0535\7\u00ec\2\2\u0535\u0536\7\30\2\2")
        buf.write("\u0536\u0537\7\u00fa\2\2\u0537\u0538\7\31\2\2\u0538\u0545")
        buf.write("\7!\2\2\u0539\u053a\7@\2\2\u053a\u053b\7\b\2\2\u053b\u0540")
        buf.write("\5\u009eP\2\u053c\u053d\7(\2\2\u053d\u053f\5\u009eP\2")
        buf.write("\u053e\u053c\3\2\2\2\u053f\u0542\3\2\2\2\u0540\u053e\3")
        buf.write("\2\2\2\u0540\u0541\3\2\2\2\u0541\u0543\3\2\2\2\u0542\u0540")
        buf.write("\3\2\2\2\u0543\u0544\7!\2\2\u0544\u0546\3\2\2\2\u0545")
        buf.write("\u0539\3\2\2\2\u0545\u0546\3\2\2\2\u0546\u0547\3\2\2\2")
        buf.write("\u0547\u0548\5\u0124\u0093\2\u0548\u054a\7s\2\2\u0549")
        buf.write("\u054b\7 \2\2\u054a\u0549\3\2\2\2\u054a\u054b\3\2\2\2")
        buf.write("\u054b\u008d\3\2\2\2\u054c\u054d\7r\2\2\u054d\u054e\7")
        buf.write("\62\2\2\u054e\u054f\7\b\2\2\u054f\u0550\7\u00ec\2\2\u0550")
        buf.write("\u0551\7\30\2\2\u0551\u0552\7\u00fa\2\2\u0552\u0553\7")
        buf.write("\31\2\2\u0553\u0560\7!\2\2\u0554\u0555\7@\2\2\u0555\u0556")
        buf.write("\7\b\2\2\u0556\u055b\5\u009eP\2\u0557\u0558\7(\2\2\u0558")
        buf.write("\u055a\5\u009eP\2\u0559\u0557\3\2\2\2\u055a\u055d\3\2")
        buf.write("\2\2\u055b\u0559\3\2\2\2\u055b\u055c\3\2\2\2\u055c\u055e")
        buf.write("\3\2\2\2\u055d\u055b\3\2\2\2\u055e\u055f\7!\2\2\u055f")
        buf.write("\u0561\3\2\2\2\u0560\u0554\3\2\2\2\u0560\u0561\3\2\2\2")
        buf.write("\u0561\u0562\3\2\2\2\u0562\u0563\5\u0124\u0093\2\u0563")
        buf.write("\u0565\7s\2\2\u0564\u0566\7 \2\2\u0565\u0564\3\2\2\2\u0565")
        buf.write("\u0566\3\2\2\2\u0566\u008f\3\2\2\2\u0567\u0568\7l\2\2")
        buf.write("\u0568\u0569\5\u0124\u0093\2\u0569\u056a\7 \2\2\u056a")
        buf.write("\u056b\5\u00b4[\2\u056b\u056d\7s\2\2\u056c\u056e\7 \2")
        buf.write("\2\u056d\u056c\3\2\2\2\u056d\u056e\3\2\2\2\u056e\u0091")
        buf.write("\3\2\2\2\u056f\u0570\7\u00a4\2\2\u0570\u0571\7\u00a5\2")
        buf.write("\2\u0571\u0572\7\b\2\2\u0572\u0573\7\u00fa\2\2\u0573\u0093")
        buf.write("\3\2\2\2\u0574\u0575\7\u00a6\2\2\u0575\u0576\7\b\2\2\u0576")
        buf.write("\u0577\7\u00ec\2\2\u0577\u0578\7\30\2\2\u0578\u0579\7")
        buf.write("\u00fa\2\2\u0579\u057a\7\31\2\2\u057a\u057b\7!\2\2\u057b")
        buf.write("\u0095\3\2\2\2\u057c\u057d\7\u00eb\2\2\u057d\u057e\7\b")
        buf.write("\2\2\u057e\u057f\5L\'\2\u057f\u0580\7!\2\2\u0580\u0097")
        buf.write("\3\2\2\2\u0581\u0582\7q\2\2\u0582\u0583\7\62\2\2\u0583")
        buf.write("\u0584\7\b\2\2\u0584\u0585\7\u00ec\2\2\u0585\u0586\7\30")
        buf.write("\2\2\u0586\u0587\7\u00fa\2\2\u0587\u0588\7\31\2\2\u0588")
        buf.write("\u058d\7!\2\2\u0589\u058a\7M\2\2\u058a\u058b\7\b\2\2\u058b")
        buf.write("\u058c\7\u00fa\2\2\u058c\u058e\7!\2\2\u058d\u0589\3\2")
        buf.write("\2\2\u058d\u058e\3\2\2\2\u058e\u058f\3\2\2\2\u058f\u0590")
        buf.write("\5\u0124\u0093\2\u0590\u0592\7s\2\2\u0591\u0593\7 \2\2")
        buf.write("\u0592\u0591\3\2\2\2\u0592\u0593\3\2\2\2\u0593\u0099\3")
        buf.write("\2\2\2\u0594\u0596\5\u008aF\2\u0595\u0594\3\2\2\2\u0596")
        buf.write("\u0599\3\2\2\2\u0597\u0595\3\2\2\2\u0597\u0598\3\2\2\2")
        buf.write("\u0598\u009b\3\2\2\2\u0599\u0597\3\2\2\2\u059a\u05a2\5")
        buf.write("\u00a0Q\2\u059b\u05a2\5\u00a2R\2\u059c\u05a2\5\u00a6T")
        buf.write("\2\u059d\u05a2\5\u00a4S\2\u059e\u05a2\5\u00a8U\2\u059f")
        buf.write("\u05a2\5\u00b0Y\2\u05a0\u05a2\5\u00b2Z\2\u05a1\u059a\3")
        buf.write("\2\2\2\u05a1\u059b\3\2\2\2\u05a1\u059c\3\2\2\2\u05a1\u059d")
        buf.write("\3\2\2\2\u05a1\u059e\3\2\2\2\u05a1\u059f\3\2\2\2\u05a1")
        buf.write("\u05a0\3\2\2\2\u05a2\u009d\3\2\2\2\u05a3\u05ac\7\u00fa")
        buf.write("\2\2\u05a4\u05ac\7y\2\2\u05a5\u05ac\7v\2\2\u05a6\u05ac")
        buf.write("\7u\2\2\u05a7\u05ac\7{\2\2\u05a8\u05ac\7|\2\2\u05a9\u05ac")
        buf.write("\7z\2\2\u05aa\u05ac\7}\2\2\u05ab\u05a3\3\2\2\2\u05ab\u05a4")
        buf.write("\3\2\2\2\u05ab\u05a5\3\2\2\2\u05ab\u05a6\3\2\2\2\u05ab")
        buf.write("\u05a7\3\2\2\2\u05ab\u05a8\3\2\2\2\u05ab\u05a9\3\2\2\2")
        buf.write("\u05ab\u05aa\3\2\2\2\u05ac\u009f\3\2\2\2\u05ad\u05ae\7")
        buf.write("k\2\2\u05ae\u05af\5\u0124\u0093\2\u05af\u05bc\7 \2\2\u05b0")
        buf.write("\u05b1\7@\2\2\u05b1\u05b2\7\b\2\2\u05b2\u05b7\5\u009e")
        buf.write("P\2\u05b3\u05b4\7(\2\2\u05b4\u05b6\5\u009eP\2\u05b5\u05b3")
        buf.write("\3\2\2\2\u05b6\u05b9\3\2\2\2\u05b7\u05b5\3\2\2\2\u05b7")
        buf.write("\u05b8\3\2\2\2\u05b8\u05ba\3\2\2\2\u05b9\u05b7\3\2\2\2")
        buf.write("\u05ba\u05bb\7!\2\2\u05bb\u05bd\3\2\2\2\u05bc\u05b0\3")
        buf.write("\2\2\2\u05bc\u05bd\3\2\2\2\u05bd\u05be\3\2\2\2\u05be\u05bf")
        buf.write("\5\u00b4[\2\u05bf\u05c1\7s\2\2\u05c0\u05c2\7 \2\2\u05c1")
        buf.write("\u05c0\3\2\2\2\u05c1\u05c2\3\2\2\2\u05c2\u00a1\3\2\2\2")
        buf.write("\u05c3\u05c4\7K\2\2\u05c4\u05c5\5\u0124\u0093\2\u05c5")
        buf.write("\u05d2\7 \2\2\u05c6\u05c7\7@\2\2\u05c7\u05c8\7\b\2\2\u05c8")
        buf.write("\u05cd\5\u009eP\2\u05c9\u05ca\7(\2\2\u05ca\u05cc\5\u009e")
        buf.write("P\2\u05cb\u05c9\3\2\2\2\u05cc\u05cf\3\2\2\2\u05cd\u05cb")
        buf.write("\3\2\2\2\u05cd\u05ce\3\2\2\2\u05ce\u05d0\3\2\2\2\u05cf")
        buf.write("\u05cd\3\2\2\2\u05d0\u05d1\7!\2\2\u05d1\u05d3\3\2\2\2")
        buf.write("\u05d2\u05c6\3\2\2\2\u05d2\u05d3\3\2\2\2\u05d3\u05d4\3")
        buf.write("\2\2\2\u05d4\u05d5\5\u00b4[\2\u05d5\u05d7\7s\2\2\u05d6")
        buf.write("\u05d8\7 \2\2\u05d7\u05d6\3\2\2\2\u05d7\u05d8\3\2\2\2")
        buf.write("\u05d8\u00a3\3\2\2\2\u05d9\u05e1\7c\2\2\u05da\u05db\5")
        buf.write("\u00a6T\2\u05db\u05dc\7!\2\2\u05dc\u05e2\3\2\2\2\u05dd")
        buf.write("\u05de\7\b\2\2\u05de\u05df\5Z.\2\u05df\u05e0\7!\2\2\u05e0")
        buf.write("\u05e2\3\2\2\2\u05e1\u05da\3\2\2\2\u05e1\u05dd\3\2\2\2")
        buf.write("\u05e2\u05e7\3\2\2\2\u05e3\u05e4\7\u009b\2\2\u05e4\u05e5")
        buf.write("\7\b\2\2\u05e5\u05e6\7\u00fb\2\2\u05e6\u05e8\7!\2\2\u05e7")
        buf.write("\u05e3\3\2\2\2\u05e7\u05e8\3\2\2\2\u05e8\u00a5\3\2\2\2")
        buf.write("\u05e9\u05ea\7\u0096\2\2\u05ea\u05eb\7\b\2\2\u05eb\u05ec")
        buf.write("\5\u0124\u0093\2\u05ec\u00a7\3\2\2\2\u05ed\u05ee\5\u00aa")
        buf.write("V\2\u05ee\u00a9\3\2\2\2\u05ef\u05f0\7?\2\2\u05f0\u05f1")
        buf.write("\7>\2\2\u05f1\u05f2\7\b\2\2\u05f2\u05f3\7\u00ec\2\2\u05f3")
        buf.write("\u05f4\7\30\2\2\u05f4\u05f5\7\u00fa\2\2\u05f5\u05f6\7")
        buf.write("\31\2\2\u05f6\u05f7\7!\2\2\u05f7\u05f8\7\u0096\2\2\u05f8")
        buf.write("\u05f9\7\b\2\2\u05f9\u05fa\5Z.\2\u05fa\u05fb\7!\2\2\u05fb")
        buf.write("\u05fc\7@\2\2\u05fc\u05fd\7\b\2\2\u05fd\u0602\5\u00ac")
        buf.write("W\2\u05fe\u05ff\7!\2\2\u05ff\u0600\7t\2\2\u0600\u0601")
        buf.write("\7\b\2\2\u0601\u0603\7\u00fa\2\2\u0602\u05fe\3\2\2\2\u0602")
        buf.write("\u0603\3\2\2\2\u0603\u0608\3\2\2\2\u0604\u0605\7!\2\2")
        buf.write("\u0605\u0607\5\\/\2\u0606\u0604\3\2\2\2\u0607\u060a\3")
        buf.write("\2\2\2\u0608\u0606\3\2\2\2\u0608\u0609\3\2\2\2\u0609\u060b")
        buf.write("\3\2\2\2\u060a\u0608\3\2\2\2\u060b\u060c\7 \2\2\u060c")
        buf.write("\u00ab\3\2\2\2\u060d\u0612\5\u00aeX\2\u060e\u060f\7(\2")
        buf.write("\2\u060f\u0611\5\u00aeX\2\u0610\u060e\3\2\2\2\u0611\u0614")
        buf.write("\3\2\2\2\u0612\u0610\3\2\2\2\u0612\u0613\3\2\2\2\u0613")
        buf.write("\u00ad\3\2\2\2\u0614\u0612\3\2\2\2\u0615\u0621\7\u00fa")
        buf.write("\2\2\u0616\u0621\7\u00ed\2\2\u0617\u0621\7\u00ee\2\2\u0618")
        buf.write("\u0621\7y\2\2\u0619\u0621\7{\2\2\u061a\u0621\7\u0080\2")
        buf.write("\2\u061b\u0621\7|\2\2\u061c\u0621\7v\2\2\u061d\u0621\7")
        buf.write("u\2\2\u061e\u0621\7z\2\2\u061f\u0621\7}\2\2\u0620\u0615")
        buf.write("\3\2\2\2\u0620\u0616\3\2\2\2\u0620\u0617\3\2\2\2\u0620")
        buf.write("\u0618\3\2\2\2\u0620\u0619\3\2\2\2\u0620\u061a\3\2\2\2")
        buf.write("\u0620\u061b\3\2\2\2\u0620\u061c\3\2\2\2\u0620\u061d\3")
        buf.write("\2\2\2\u0620\u061e\3\2\2\2\u0620\u061f\3\2\2\2\u0621\u00af")
        buf.write("\3\2\2\2\u0622\u0623\7\u0097\2\2\u0623\u0624\5\u0124\u0093")
        buf.write("\2\u0624\u0625\7 \2\2\u0625\u00b1\3\2\2\2\u0626\u0627")
        buf.write("\7\u0098\2\2\u0627\u0628\5\u0124\u0093\2\u0628\u0629\7")
        buf.write(" \2\2\u0629\u00b3\3\2\2\2\u062a\u062c\5\u00b6\\\2\u062b")
        buf.write("\u062a\3\2\2\2\u062c\u062f\3\2\2\2\u062d\u062b\3\2\2\2")
        buf.write("\u062d\u062e\3\2\2\2\u062e\u00b5\3\2\2\2\u062f\u062d\3")
        buf.write("\2\2\2\u0630\u0633\5\u008aF\2\u0631\u0633\5\u009cO\2\u0632")
        buf.write("\u0630\3\2\2\2\u0632\u0631\3\2\2\2\u0633\u00b7\3\2\2\2")
        buf.write("\u0634\u0637\5\u00ba^\2\u0635\u0637\5\u00c0a\2\u0636\u0634")
        buf.write("\3\2\2\2\u0636\u0635\3\2\2\2\u0637\u00b9\3\2\2\2\u0638")
        buf.write("\u0639\7\\\2\2\u0639\u063a\5R*\2\u063a\u063b\5T+\2\u063b")
        buf.write("\u0641\7!\2\2\u063c\u063d\7@\2\2\u063d\u063e\7\b\2\2\u063e")
        buf.write("\u063f\5\u00bc_\2\u063f\u0640\7!\2\2\u0640\u0642\3\2\2")
        buf.write("\2\u0641\u063c\3\2\2\2\u0641\u0642\3\2\2\2\u0642\u0647")
        buf.write("\3\2\2\2\u0643\u0644\7t\2\2\u0644\u0645\7\b\2\2\u0645")
        buf.write("\u0646\7\u00fa\2\2\u0646\u0648\7!\2\2\u0647\u0643\3\2")
        buf.write("\2\2\u0647\u0648\3\2\2\2\u0648\u0649\3\2\2\2\u0649\u064a")
        buf.write("\5\u00b4[\2\u064a\u064b\7]\2\2\u064b\u064c\7 \2\2\u064c")
        buf.write("\u00bb\3\2\2\2\u064d\u0652\5\u00be`\2\u064e\u064f\7(\2")
        buf.write("\2\u064f\u0651\5\u00be`\2\u0650\u064e\3\2\2\2\u0651\u0654")
        buf.write("\3\2\2\2\u0652\u0650\3\2\2\2\u0652\u0653\3\2\2\2\u0653")
        buf.write("\u00bd\3\2\2\2\u0654\u0652\3\2\2\2\u0655\u065c\7\u00fa")
        buf.write("\2\2\u0656\u065c\7u\2\2\u0657\u065c\7v\2\2\u0658\u065c")
        buf.write("\7w\2\2\u0659\u065c\7x\2\2\u065a\u065c\5V,\2\u065b\u0655")
        buf.write("\3\2\2\2\u065b\u0656\3\2\2\2\u065b\u0657\3\2\2\2\u065b")
        buf.write("\u0658\3\2\2\2\u065b\u0659\3\2\2\2\u065b\u065a\3\2\2\2")
        buf.write("\u065c\u00bf\3\2\2\2\u065d\u065e\7\u00a1\2\2\u065e\u065f")
        buf.write("\5P)\2\u065f\u0665\7!\2\2\u0660\u0661\7@\2\2\u0661\u0662")
        buf.write("\7\b\2\2\u0662\u0663\5\u00c2b\2\u0663\u0664\7!\2\2\u0664")
        buf.write("\u0666\3\2\2\2\u0665\u0660\3\2\2\2\u0665\u0666\3\2\2\2")
        buf.write("\u0666\u0667\3\2\2\2\u0667\u0668\7\u00a2\2\2\u0668\u0669")
        buf.write("\7\b\2\2\u0669\u066a\7\u00ec\2\2\u066a\u066b\7\30\2\2")
        buf.write("\u066b\u066c\7\u00fa\2\2\u066c\u066d\7\31\2\2\u066d\u066e")
        buf.write("\7!\2\2\u066e\u066f\5\u009aN\2\u066f\u0670\7\u00a3\2\2")
        buf.write("\u0670\u0671\7 \2\2\u0671\u00c1\3\2\2\2\u0672\u0677\5")
        buf.write("\u00c4c\2\u0673\u0674\7(\2\2\u0674\u0676\5\u00c4c\2\u0675")
        buf.write("\u0673\3\2\2\2\u0676\u0679\3\2\2\2\u0677\u0675\3\2\2\2")
        buf.write("\u0677\u0678\3\2\2\2\u0678\u00c3\3\2\2\2\u0679\u0677\3")
        buf.write("\2\2\2\u067a\u067d\7\u00fa\2\2\u067b\u067d\5V,\2\u067c")
        buf.write("\u067a\3\2\2\2\u067c\u067b\3\2\2\2\u067d\u00c5\3\2\2\2")
        buf.write("\u067e\u0681\5\u00c8e\2\u067f\u0681\5\u00d0i\2\u0680\u067e")
        buf.write("\3\2\2\2\u0680\u067f\3\2\2\2\u0681\u00c7\3\2\2\2\u0682")
        buf.write("\u0683\7^\2\2\u0683\u0684\5R*\2\u0684\u0685\5T+\2\u0685")
        buf.write("\u068b\7!\2\2\u0686\u0687\7@\2\2\u0687\u0688\7\b\2\2\u0688")
        buf.write("\u0689\5\u00ccg\2\u0689\u068a\7!\2\2\u068a\u068c\3\2\2")
        buf.write("\2\u068b\u0686\3\2\2\2\u068b\u068c\3\2\2\2\u068c\u0691")
        buf.write("\3\2\2\2\u068d\u068e\7t\2\2\u068e\u068f\7\b\2\2\u068f")
        buf.write("\u0690\7\u00fa\2\2\u0690\u0692\7!\2\2\u0691\u068d\3\2")
        buf.write("\2\2\u0691\u0692\3\2\2\2\u0692\u0693\3\2\2\2\u0693\u0694")
        buf.write("\5\u00caf\2\u0694\u0695\5\u00b4[\2\u0695\u0696\7_\2\2")
        buf.write("\u0696\u0697\7 \2\2\u0697\u00c9\3\2\2\2\u0698\u0699\7")
        buf.write("`\2\2\u0699\u069b\7\b\2\2\u069a\u069c\7\35\2\2\u069b\u069a")
        buf.write("\3\2\2\2\u069b\u069c\3\2\2\2\u069c\u069d\3\2\2\2\u069d")
        buf.write("\u069e\7\u00fa\2\2\u069e\u069f\7!\2\2\u069f\u06a0\7a\2")
        buf.write("\2\u06a0\u06a2\7\b\2\2\u06a1\u06a3\7\35\2\2\u06a2\u06a1")
        buf.write("\3\2\2\2\u06a2\u06a3\3\2\2\2\u06a3\u06a4\3\2\2\2\u06a4")
        buf.write("\u06a5\7\u00fa\2\2\u06a5\u06aa\7!\2\2\u06a6\u06a7\7b\2")
        buf.write("\2\u06a7\u06a8\7\b\2\2\u06a8\u06a9\7\u00fa\2\2\u06a9\u06ab")
        buf.write("\7!\2\2\u06aa\u06a6\3\2\2\2\u06aa\u06ab\3\2\2\2\u06ab")
        buf.write("\u00cb\3\2\2\2\u06ac\u06b1\5\u00ceh\2\u06ad\u06ae\7(\2")
        buf.write("\2\u06ae\u06b0\5\u00ceh\2\u06af\u06ad\3\2\2\2\u06b0\u06b3")
        buf.write("\3\2\2\2\u06b1\u06af\3\2\2\2\u06b1\u06b2\3\2\2\2\u06b2")
        buf.write("\u00cd\3\2\2\2\u06b3\u06b1\3\2\2\2\u06b4\u06be\7\u00fa")
        buf.write("\2\2\u06b5\u06be\7\u00ef\2\2\u06b6\u06be\7\u00f0\2\2\u06b7")
        buf.write("\u06be\7\u00f1\2\2\u06b8\u06be\7\u00f2\2\2\u06b9\u06be")
        buf.write("\7\u00f3\2\2\u06ba\u06be\7\u00f4\2\2\u06bb\u06be\7\u00f5")
        buf.write("\2\2\u06bc\u06be\5V,\2\u06bd\u06b4\3\2\2\2\u06bd\u06b5")
        buf.write("\3\2\2\2\u06bd\u06b6\3\2\2\2\u06bd\u06b7\3\2\2\2\u06bd")
        buf.write("\u06b8\3\2\2\2\u06bd\u06b9\3\2\2\2\u06bd\u06ba\3\2\2\2")
        buf.write("\u06bd\u06bb\3\2\2\2\u06bd\u06bc\3\2\2\2\u06be\u00cf\3")
        buf.write("\2\2\2\u06bf\u06c0\7\60\2\2\u06c0\u06c1\5R*\2\u06c1\u06c2")
        buf.write("\5T+\2\u06c2\u06c8\7!\2\2\u06c3\u06c4\7@\2\2\u06c4\u06c5")
        buf.write("\7\b\2\2\u06c5\u06c6\5\u00d2j\2\u06c6\u06c7\7!\2\2\u06c7")
        buf.write("\u06c9\3\2\2\2\u06c8\u06c3\3\2\2\2\u06c8\u06c9\3\2\2\2")
        buf.write("\u06c9\u06cb\3\2\2\2\u06ca\u06cc\5\u00caf\2\u06cb\u06ca")
        buf.write("\3\2\2\2\u06cb\u06cc\3\2\2\2\u06cc\u06cd\3\2\2\2\u06cd")
        buf.write("\u06ce\5\u00b4[\2\u06ce\u06cf\7\61\2\2\u06cf\u06d0\7 ")
        buf.write("\2\2\u06d0\u00d1\3\2\2\2\u06d1\u06d6\5\u00ceh\2\u06d2")
        buf.write("\u06d3\7(\2\2\u06d3\u06d5\5\u00ceh\2\u06d4\u06d2\3\2\2")
        buf.write("\2\u06d5\u06d8\3\2\2\2\u06d6\u06d4\3\2\2\2\u06d6\u06d7")
        buf.write("\3\2\2\2\u06d7\u00d3\3\2\2\2\u06d8\u06d6\3\2\2\2\u06d9")
        buf.write("\u06dc\5\u00d6l\2\u06da\u06dc\5\u00dco\2\u06db\u06d9\3")
        buf.write("\2\2\2\u06db\u06da\3\2\2\2\u06dc\u00d5\3\2\2\2\u06dd\u06de")
        buf.write("\7f\2\2\u06de\u06df\5P)\2\u06df\u06e5\7!\2\2\u06e0\u06e1")
        buf.write("\7@\2\2\u06e1\u06e2\7\b\2\2\u06e2\u06e3\5\u00d8m\2\u06e3")
        buf.write("\u06e4\7!\2\2\u06e4\u06e6\3\2\2\2\u06e5\u06e0\3\2\2\2")
        buf.write("\u06e5\u06e6\3\2\2\2\u06e6\u06eb\3\2\2\2\u06e7\u06e8\7")
        buf.write("t\2\2\u06e8\u06e9\7\b\2\2\u06e9\u06ea\7\u00fa\2\2\u06ea")
        buf.write("\u06ec\7!\2\2\u06eb\u06e7\3\2\2\2\u06eb\u06ec\3\2\2\2")
        buf.write("\u06ec\u06ed\3\2\2\2\u06ed\u06ee\7h\2\2\u06ee\u06ef\7")
        buf.write("\b\2\2\u06ef\u06f0\7\u00fa\2\2\u06f0\u06f1\7!\2\2\u06f1")
        buf.write("\u06f2\7i\2\2\u06f2\u06f3\7\b\2\2\u06f3\u06f4\7\u00fa")
        buf.write("\2\2\u06f4\u06f5\7!\2\2\u06f5\u06f6\5\u00b4[\2\u06f6\u06f7")
        buf.write("\7g\2\2\u06f7\u06f8\7 \2\2\u06f8\u00d7\3\2\2\2\u06f9\u06fe")
        buf.write("\5\u00dan\2\u06fa\u06fb\7(\2\2\u06fb\u06fd\5\u00dan\2")
        buf.write("\u06fc\u06fa\3\2\2\2\u06fd\u0700\3\2\2\2\u06fe\u06fc\3")
        buf.write("\2\2\2\u06fe\u06ff\3\2\2\2\u06ff\u00d9\3\2\2\2\u0700\u06fe")
        buf.write("\3\2\2\2\u0701\u0705\7\u00fa\2\2\u0702\u0705\7\13\2\2")
        buf.write("\u0703\u0705\5V,\2\u0704\u0701\3\2\2\2\u0704\u0702\3\2")
        buf.write("\2\2\u0704\u0703\3\2\2\2\u0705\u00db\3\2\2\2\u0706\u0707")
        buf.write("\7d\2\2\u0707\u0708\5P)\2\u0708\u070e\7!\2\2\u0709\u070a")
        buf.write("\7@\2\2\u070a\u070b\7\b\2\2\u070b\u070c\5\u00dep\2\u070c")
        buf.write("\u070d\7!\2\2\u070d\u070f\3\2\2\2\u070e\u0709\3\2\2\2")
        buf.write("\u070e\u070f\3\2\2\2\u070f\u0714\3\2\2\2\u0710\u0711\7")
        buf.write("t\2\2\u0711\u0712\7\b\2\2\u0712\u0713\7\u00fa\2\2\u0713")
        buf.write("\u0715\7!\2\2\u0714\u0710\3\2\2\2\u0714\u0715\3\2\2\2")
        buf.write("\u0715\u0716\3\2\2\2\u0716\u0717\7h\2\2\u0717\u0718\7")
        buf.write("\b\2\2\u0718\u0719\7\u00fa\2\2\u0719\u071a\7!\2\2\u071a")
        buf.write("\u071b\7i\2\2\u071b\u071c\7\b\2\2\u071c\u071d\7\u00fa")
        buf.write("\2\2\u071d\u0722\7!\2\2\u071e\u071f\7j\2\2\u071f\u0720")
        buf.write("\7\b\2\2\u0720\u0721\7\u00fa\2\2\u0721\u0723\7!\2\2\u0722")
        buf.write("\u071e\3\2\2\2\u0722\u0723\3\2\2\2\u0723\u0724\3\2\2\2")
        buf.write("\u0724\u0725\5\u00b4[\2\u0725\u0726\7e\2\2\u0726\u0727")
        buf.write("\7 \2\2\u0727\u00dd\3\2\2\2\u0728\u072d\5\u00e0q\2\u0729")
        buf.write("\u072a\7(\2\2\u072a\u072c\5\u00e0q\2\u072b\u0729\3\2\2")
        buf.write("\2\u072c\u072f\3\2\2\2\u072d\u072b\3\2\2\2\u072d\u072e")
        buf.write("\3\2\2\2\u072e\u00df\3\2\2\2\u072f\u072d\3\2\2\2\u0730")
        buf.write("\u0733\7\u00fa\2\2\u0731\u0733\5V,\2\u0732\u0730\3\2\2")
        buf.write("\2\u0732\u0731\3\2\2\2\u0733\u00e1\3\2\2\2\u0734\u0735")
        buf.write("\7\63\2\2\u0735\u0736\5P)\2\u0736\u073b\7!\2\2\u0737\u0738")
        buf.write("\7\64\2\2\u0738\u0739\7\b\2\2\u0739\u073a\7\u00fa\2\2")
        buf.write("\u073a\u073c\7!\2\2\u073b\u0737\3\2\2\2\u073b\u073c\3")
        buf.write("\2\2\2\u073c\u0742\3\2\2\2\u073d\u073e\7@\2\2\u073e\u073f")
        buf.write("\7\b\2\2\u073f\u0740\5\u00e4s\2\u0740\u0741\7!\2\2\u0741")
        buf.write("\u0743\3\2\2\2\u0742\u073d\3\2\2\2\u0742\u0743\3\2\2\2")
        buf.write("\u0743\u0744\3\2\2\2\u0744\u0745\5\u00b4[\2\u0745\u0746")
        buf.write("\7\65\2\2\u0746\u0747\7 \2\2\u0747\u00e3\3\2\2\2\u0748")
        buf.write("\u074d\5\u00e6t\2\u0749\u074a\7(\2\2\u074a\u074c\5\u00e6")
        buf.write("t\2\u074b\u0749\3\2\2\2\u074c\u074f\3\2\2\2\u074d\u074b")
        buf.write("\3\2\2\2\u074d\u074e\3\2\2\2\u074e\u00e5\3\2\2\2\u074f")
        buf.write("\u074d\3\2\2\2\u0750\u0755\7\u00fa\2\2\u0751\u0755\7\u00c0")
        buf.write("\2\2\u0752\u0755\7\u00c1\2\2\u0753\u0755\5V,\2\u0754\u0750")
        buf.write("\3\2\2\2\u0754\u0751\3\2\2\2\u0754\u0752\3\2\2\2\u0754")
        buf.write("\u0753\3\2\2\2\u0755\u00e7\3\2\2\2\u0756\u07b1\7A\2\2")
        buf.write("\u0757\u0758\5P)\2\u0758\u075e\7!\2\2\u0759\u075a\7@\2")
        buf.write("\2\u075a\u075b\7\b\2\2\u075b\u075c\5\u00ecw\2\u075c\u075d")
        buf.write("\7!\2\2\u075d\u075f\3\2\2\2\u075e\u0759\3\2\2\2\u075e")
        buf.write("\u075f\3\2\2\2\u075f\u0760\3\2\2\2\u0760\u0761\5\u00b4")
        buf.write("[\2\u0761\u07b2\3\2\2\2\u0762\u0763\7C\2\2\u0763\u0764")
        buf.write("\7\u008f\2\2\u0764\u0765\7\b\2\2\u0765\u0766\7\u00fb\2")
        buf.write("\2\u0766\u0767\7\34\2\2\u0767\u0768\7\u00fb\2\2\u0768")
        buf.write("\u0769\7!\2\2\u0769\u076a\7\62\2\2\u076a\u076b\7\b\2\2")
        buf.write("\u076b\u076c\7\u00ec\2\2\u076c\u076d\7\30\2\2\u076d\u076e")
        buf.write("\7\u00fa\2\2\u076e\u076f\7\31\2\2\u076f\u0770\7!\2\2\u0770")
        buf.write("\u0771\7=\2\2\u0771\u0772\7\b\2\2\u0772\u0773\7\u00ec")
        buf.write("\2\2\u0773\u0774\7\30\2\2\u0774\u0775\7\u00fa\2\2\u0775")
        buf.write("\u0776\7\31\2\2\u0776\u0777\7!\2\2\u0777\u0778\5\u00ea")
        buf.write("v\2\u0778\u0779\7D\2\2\u0779\u077a\7\u008f\2\2\u077a\u077b")
        buf.write("\7\b\2\2\u077b\u077c\7\u00fb\2\2\u077c\u077d\7\34\2\2")
        buf.write("\u077d\u077e\7\u00fb\2\2\u077e\u077f\7!\2\2\u077f\u0780")
        buf.write("\7\62\2\2\u0780\u0781\7\b\2\2\u0781\u0782\7\u00ec\2\2")
        buf.write("\u0782\u0783\7\30\2\2\u0783\u0784\7\u00fa\2\2\u0784\u0785")
        buf.write("\7\31\2\2\u0785\u0786\7!\2\2\u0786\u0787\7=\2\2\u0787")
        buf.write("\u0788\7\b\2\2\u0788\u0789\7\u00ec\2\2\u0789\u078a\7\30")
        buf.write("\2\2\u078a\u078b\7\u00fa\2\2\u078b\u078c\7\31\2\2\u078c")
        buf.write("\u078d\7!\2\2\u078d\u078e\5\u00eav\2\u078e\u078f\7E\2")
        buf.write("\2\u078f\u0790\7\u008f\2\2\u0790\u0791\7\b\2\2\u0791\u0792")
        buf.write("\7\u00fb\2\2\u0792\u0793\7\34\2\2\u0793\u0794\7\u00fb")
        buf.write("\2\2\u0794\u0795\7!\2\2\u0795\u0796\7\62\2\2\u0796\u0797")
        buf.write("\7\b\2\2\u0797\u0798\7\u00ec\2\2\u0798\u0799\7\30\2\2")
        buf.write("\u0799\u079a\7\u00fa\2\2\u079a\u079b\7\31\2\2\u079b\u079c")
        buf.write("\7!\2\2\u079c\u079d\7=\2\2\u079d\u079e\7\b\2\2\u079e\u079f")
        buf.write("\7\u00ec\2\2\u079f\u07a0\7\30\2\2\u07a0\u07a1\7\u00fa")
        buf.write("\2\2\u07a1\u07a2\7\31\2\2\u07a2\u07a3\7!\2\2\u07a3\u07a9")
        buf.write("\5\u00eav\2\u07a4\u07a5\7@\2\2\u07a5\u07a6\7\b\2\2\u07a6")
        buf.write("\u07a7\5\u00ecw\2\u07a7\u07a8\7!\2\2\u07a8\u07aa\3\2\2")
        buf.write("\2\u07a9\u07a4\3\2\2\2\u07a9\u07aa\3\2\2\2\u07aa\u07ae")
        buf.write("\3\2\2\2\u07ab\u07ad\5\u008cG\2\u07ac\u07ab\3\2\2\2\u07ad")
        buf.write("\u07b0\3\2\2\2\u07ae\u07ac\3\2\2\2\u07ae\u07af\3\2\2\2")
        buf.write("\u07af\u07b2\3\2\2\2\u07b0\u07ae\3\2\2\2\u07b1\u0757\3")
        buf.write("\2\2\2\u07b1\u0762\3\2\2\2\u07b2\u07b3\3\2\2\2\u07b3\u07b4")
        buf.write("\7B\2\2\u07b4\u07b5\7 \2\2\u07b5\u00e9\3\2\2\2\u07b6\u07b7")
        buf.write("\7`\2\2\u07b7\u07b8\7\b\2\2\u07b8\u07b9\7\u00fa\2\2\u07b9")
        buf.write("\u07ba\7!\2\2\u07ba\u07bb\7a\2\2\u07bb\u07bc\7\b\2\2\u07bc")
        buf.write("\u07bd\7\u00fa\2\2\u07bd\u07c2\7!\2\2\u07be\u07bf\7b\2")
        buf.write("\2\u07bf\u07c0\7\b\2\2\u07c0\u07c1\7\u00fa\2\2\u07c1\u07c3")
        buf.write("\7!\2\2\u07c2\u07be\3\2\2\2\u07c2\u07c3\3\2\2\2\u07c3")
        buf.write("\u07c8\3\2\2\2\u07c4\u07c5\7c\2\2\u07c5\u07c6\7\b\2\2")
        buf.write("\u07c6\u07c7\7\u00fa\2\2\u07c7\u07c9\7!\2\2\u07c8\u07c4")
        buf.write("\3\2\2\2\u07c8\u07c9\3\2\2\2\u07c9\u00eb\3\2\2\2\u07ca")
        buf.write("\u07cf\5\u00eex\2\u07cb\u07cc\7(\2\2\u07cc\u07ce\5\u00ee")
        buf.write("x\2\u07cd\u07cb\3\2\2\2\u07ce\u07d1\3\2\2\2\u07cf\u07cd")
        buf.write("\3\2\2\2\u07cf\u07d0\3\2\2\2\u07d0\u00ed\3\2\2\2\u07d1")
        buf.write("\u07cf\3\2\2\2\u07d2\u07d3\t\6\2\2\u07d3\u00ef\3\2\2\2")
        buf.write("\u07d4\u082f\7F\2\2\u07d5\u07d6\5P)\2\u07d6\u07dc\7!\2")
        buf.write("\2\u07d7\u07d8\7@\2\2\u07d8\u07d9\7\b\2\2\u07d9\u07da")
        buf.write("\5\u00f4{\2\u07da\u07db\7!\2\2\u07db\u07dd\3\2\2\2\u07dc")
        buf.write("\u07d7\3\2\2\2\u07dc\u07dd\3\2\2\2\u07dd\u07de\3\2\2\2")
        buf.write("\u07de\u07df\5\u00b4[\2\u07df\u0830\3\2\2\2\u07e0\u07e1")
        buf.write("\7H\2\2\u07e1\u07e2\7\u008f\2\2\u07e2\u07e3\7\b\2\2\u07e3")
        buf.write("\u07e4\7\u00fb\2\2\u07e4\u07e5\7\34\2\2\u07e5\u07e6\7")
        buf.write("\u00fb\2\2\u07e6\u07e7\7!\2\2\u07e7\u07e8\7\62\2\2\u07e8")
        buf.write("\u07e9\7\b\2\2\u07e9\u07ea\7\u00ec\2\2\u07ea\u07eb\7\30")
        buf.write("\2\2\u07eb\u07ec\7\u00fa\2\2\u07ec\u07ed\7\31\2\2\u07ed")
        buf.write("\u07ee\7!\2\2\u07ee\u07ef\7=\2\2\u07ef\u07f0\7\b\2\2\u07f0")
        buf.write("\u07f1\7\u00ec\2\2\u07f1\u07f2\7\30\2\2\u07f2\u07f3\7")
        buf.write("\u00fa\2\2\u07f3\u07f4\7\31\2\2\u07f4\u07f5\7!\2\2\u07f5")
        buf.write("\u07f6\5\u00f2z\2\u07f6\u07f7\7I\2\2\u07f7\u07f8\7\u008f")
        buf.write("\2\2\u07f8\u07f9\7\b\2\2\u07f9\u07fa\7\u00fb\2\2\u07fa")
        buf.write("\u07fb\7\34\2\2\u07fb\u07fc\7\u00fb\2\2\u07fc\u07fd\7")
        buf.write("!\2\2\u07fd\u07fe\7\62\2\2\u07fe\u07ff\7\b\2\2\u07ff\u0800")
        buf.write("\7\u00ec\2\2\u0800\u0801\7\30\2\2\u0801\u0802\7\u00fa")
        buf.write("\2\2\u0802\u0803\7\31\2\2\u0803\u0804\7!\2\2\u0804\u0805")
        buf.write("\7=\2\2\u0805\u0806\7\b\2\2\u0806\u0807\7\u00ec\2\2\u0807")
        buf.write("\u0808\7\30\2\2\u0808\u0809\7\u00fa\2\2\u0809\u080a\7")
        buf.write("\31\2\2\u080a\u080b\7!\2\2\u080b\u080c\5\u00f2z\2\u080c")
        buf.write("\u080d\7J\2\2\u080d\u080e\7\u008f\2\2\u080e\u080f\7\b")
        buf.write("\2\2\u080f\u0810\7\u00fb\2\2\u0810\u0811\7\34\2\2\u0811")
        buf.write("\u0812\7\u00fb\2\2\u0812\u0813\7!\2\2\u0813\u0814\7\62")
        buf.write("\2\2\u0814\u0815\7\b\2\2\u0815\u0816\7\u00ec\2\2\u0816")
        buf.write("\u0817\7\30\2\2\u0817\u0818\7\u00fa\2\2\u0818\u0819\7")
        buf.write("\31\2\2\u0819\u081a\7!\2\2\u081a\u081b\7=\2\2\u081b\u081c")
        buf.write("\7\b\2\2\u081c\u081d\7\u00ec\2\2\u081d\u081e\7\30\2\2")
        buf.write("\u081e\u081f\7\u00fa\2\2\u081f\u0820\7\31\2\2\u0820\u0821")
        buf.write("\7!\2\2\u0821\u0827\5\u00f2z\2\u0822\u0823\7@\2\2\u0823")
        buf.write("\u0824\7\b\2\2\u0824\u0825\5\u00f4{\2\u0825\u0826\7!\2")
        buf.write("\2\u0826\u0828\3\2\2\2\u0827\u0822\3\2\2\2\u0827\u0828")
        buf.write("\3\2\2\2\u0828\u082c\3\2\2\2\u0829\u082b\5\u008cG\2\u082a")
        buf.write("\u0829\3\2\2\2\u082b\u082e\3\2\2\2\u082c\u082a\3\2\2\2")
        buf.write("\u082c\u082d\3\2\2\2\u082d\u0830\3\2\2\2\u082e\u082c\3")
        buf.write("\2\2\2\u082f\u07d5\3\2\2\2\u082f\u07e0\3\2\2\2\u0830\u0831")
        buf.write("\3\2\2\2\u0831\u0832\7G\2\2\u0832\u0833\7 \2\2\u0833\u00f1")
        buf.write("\3\2\2\2\u0834\u0835\7`\2\2\u0835\u0836\7\b\2\2\u0836")
        buf.write("\u0837\7\u00fa\2\2\u0837\u0838\7!\2\2\u0838\u0839\7a\2")
        buf.write("\2\u0839\u083a\7\b\2\2\u083a\u083b\7\u00fa\2\2\u083b\u0840")
        buf.write("\7!\2\2\u083c\u083d\7b\2\2\u083d\u083e\7\b\2\2\u083e\u083f")
        buf.write("\7\u00fa\2\2\u083f\u0841\7!\2\2\u0840\u083c\3\2\2\2\u0840")
        buf.write("\u0841\3\2\2\2\u0841\u0846\3\2\2\2\u0842\u0843\7c\2\2")
        buf.write("\u0843\u0844\7\b\2\2\u0844\u0845\7\u00fa\2\2\u0845\u0847")
        buf.write("\7!\2\2\u0846\u0842\3\2\2\2\u0846\u0847\3\2\2\2\u0847")
        buf.write("\u00f3\3\2\2\2\u0848\u084d\5\u00f6|\2\u0849\u084a\7(\2")
        buf.write("\2\u084a\u084c\5\u00f6|\2\u084b\u0849\3\2\2\2\u084c\u084f")
        buf.write("\3\2\2\2\u084d\u084b\3\2\2\2\u084d\u084e\3\2\2\2\u084e")
        buf.write("\u00f5\3\2\2\2\u084f\u084d\3\2\2\2\u0850\u0851\t\7\2\2")
        buf.write("\u0851\u00f7\3\2\2\2\u0852\u0857\5\u0104\u0083\2\u0853")
        buf.write("\u0857\5\u00fc\177\2\u0854\u0857\5\u00fe\u0080\2\u0855")
        buf.write("\u0857\5\u010e\u0088\2\u0856\u0852\3\2\2\2\u0856\u0853")
        buf.write("\3\2\2\2\u0856\u0854\3\2\2\2\u0856\u0855\3\2\2\2\u0857")
        buf.write("\u00f9\3\2\2\2\u0858\u0859\5\u0104\u0083\2\u0859\u085a")
        buf.write("\5\u010a\u0086\2\u085a\u085b\5\u010c\u0087\2\u085b\u085c")
        buf.write("\5\u010e\u0088\2\u085c\u00fb\3\2\2\2\u085d\u085e\5\u010a")
        buf.write("\u0086\2\u085e\u00fd\3\2\2\2\u085f\u0860\5\u010c\u0087")
        buf.write("\2\u0860\u00ff\3\2\2\2\u0861\u0868\5p9\2\u0862\u0868\5")
        buf.write("\u0088E\2\u0863\u0868\5\u00f8}\2\u0864\u0868\5\u0118\u008d")
        buf.write("\2\u0865\u0868\5\u011c\u008f\2\u0866\u0868\5\u0110\u0089")
        buf.write("\2\u0867\u0861\3\2\2\2\u0867\u0862\3\2\2\2\u0867\u0863")
        buf.write("\3\2\2\2\u0867\u0864\3\2\2\2\u0867\u0865\3\2\2\2\u0867")
        buf.write("\u0866\3\2\2\2\u0868\u0101\3\2\2\2\u0869\u086e\5p9\2\u086a")
        buf.write("\u086e\5\u0088E\2\u086b\u086e\5\u0118\u008d\2\u086c\u086e")
        buf.write("\5\u0110\u0089\2\u086d\u0869\3\2\2\2\u086d\u086a\3\2\2")
        buf.write("\2\u086d\u086b\3\2\2\2\u086d\u086c\3\2\2\2\u086e\u0103")
        buf.write("\3\2\2\2\u086f\u0870\7l\2\2\u0870\u0871\5\u0124\u0093")
        buf.write("\2\u0871\u0875\7 \2\2\u0872\u0874\5\u0100\u0081\2\u0873")
        buf.write("\u0872\3\2\2\2\u0874\u0877\3\2\2\2\u0875\u0873\3\2\2\2")
        buf.write("\u0875\u0876\3\2\2\2\u0876\u0878\3\2\2\2\u0877\u0875\3")
        buf.write("\2\2\2\u0878\u0879\7s\2\2\u0879\u087a\7 \2\2\u087a\u0105")
        buf.write("\3\2\2\2\u087b\u0888\7k\2\2\u087c\u087d\7@\2\2\u087d\u087e")
        buf.write("\7\b\2\2\u087e\u0883\5\u009eP\2\u087f\u0880\7(\2\2\u0880")
        buf.write("\u0882\5\u009eP\2\u0881\u087f\3\2\2\2\u0882\u0885\3\2")
        buf.write("\2\2\u0883\u0881\3\2\2\2\u0883\u0884\3\2\2\2\u0884\u0886")
        buf.write("\3\2\2\2\u0885\u0883\3\2\2\2\u0886\u0887\7!\2\2\u0887")
        buf.write("\u0889\3\2\2\2\u0888\u087c\3\2\2\2\u0888\u0889\3\2\2\2")
        buf.write("\u0889\u088a\3\2\2\2\u088a\u088b\5\u0124\u0093\2\u088b")
        buf.write("\u088c\7 \2\2\u088c\u0107\3\2\2\2\u088d\u089a\7K\2\2\u088e")
        buf.write("\u088f\7@\2\2\u088f\u0890\7\b\2\2\u0890\u0895\5\u009e")
        buf.write("P\2\u0891\u0892\7(\2\2\u0892\u0894\5\u009eP\2\u0893\u0891")
        buf.write("\3\2\2\2\u0894\u0897\3\2\2\2\u0895\u0893\3\2\2\2\u0895")
        buf.write("\u0896\3\2\2\2\u0896\u0898\3\2\2\2\u0897\u0895\3\2\2\2")
        buf.write("\u0898\u0899\7!\2\2\u0899\u089b\3\2\2\2\u089a\u088e\3")
        buf.write("\2\2\2\u089a\u089b\3\2\2\2\u089b\u089c\3\2\2\2\u089c\u089d")
        buf.write("\5\u0124\u0093\2\u089d\u089e\7 \2\2\u089e\u0109\3\2\2")
        buf.write("\2\u089f\u08ac\7k\2\2\u08a0\u08a1\7@\2\2\u08a1\u08a2\7")
        buf.write("\b\2\2\u08a2\u08a7\5\u009eP\2\u08a3\u08a4\7(\2\2\u08a4")
        buf.write("\u08a6\5\u009eP\2\u08a5\u08a3\3\2\2\2\u08a6\u08a9\3\2")
        buf.write("\2\2\u08a7\u08a5\3\2\2\2\u08a7\u08a8\3\2\2\2\u08a8\u08aa")
        buf.write("\3\2\2\2\u08a9\u08a7\3\2\2\2\u08aa\u08ab\7!\2\2\u08ab")
        buf.write("\u08ad\3\2\2\2\u08ac\u08a0\3\2\2\2\u08ac\u08ad\3\2\2\2")
        buf.write("\u08ad\u08ae\3\2\2\2\u08ae\u08af\5\u0124\u0093\2\u08af")
        buf.write("\u08b3\7 \2\2\u08b0\u08b2\5\u0100\u0081\2\u08b1\u08b0")
        buf.write("\3\2\2\2\u08b2\u08b5\3\2\2\2\u08b3\u08b1\3\2\2\2\u08b3")
        buf.write("\u08b4\3\2\2\2\u08b4\u08b6\3\2\2\2\u08b5\u08b3\3\2\2\2")
        buf.write("\u08b6\u08b7\7s\2\2\u08b7\u08b8\7 \2\2\u08b8\u010b\3\2")
        buf.write("\2\2\u08b9\u08c6\7K\2\2\u08ba\u08bb\7@\2\2\u08bb\u08bc")
        buf.write("\7\b\2\2\u08bc\u08c1\5\u009eP\2\u08bd\u08be\7(\2\2\u08be")
        buf.write("\u08c0\5\u009eP\2\u08bf\u08bd\3\2\2\2\u08c0\u08c3\3\2")
        buf.write("\2\2\u08c1\u08bf\3\2\2\2\u08c1\u08c2\3\2\2\2\u08c2\u08c4")
        buf.write("\3\2\2\2\u08c3\u08c1\3\2\2\2\u08c4\u08c5\7!\2\2\u08c5")
        buf.write("\u08c7\3\2\2\2\u08c6\u08ba\3\2\2\2\u08c6\u08c7\3\2\2\2")
        buf.write("\u08c7\u08c8\3\2\2\2\u08c8\u08c9\5\u0124\u0093\2\u08c9")
        buf.write("\u08cd\7 \2\2\u08ca\u08cc\5\u0100\u0081\2\u08cb\u08ca")
        buf.write("\3\2\2\2\u08cc\u08cf\3\2\2\2\u08cd\u08cb\3\2\2\2\u08cd")
        buf.write("\u08ce\3\2\2\2\u08ce\u08d0\3\2\2\2\u08cf\u08cd\3\2\2\2")
        buf.write("\u08d0\u08d1\7s\2\2\u08d1\u08d2\7 \2\2\u08d2\u010d\3\2")
        buf.write("\2\2\u08d3\u08d4\7p\2\2\u08d4\u08d5\7\62\2\2\u08d5\u08d6")
        buf.write("\7\b\2\2\u08d6\u08d7\7\u00ec\2\2\u08d7\u08d8\7\30\2\2")
        buf.write("\u08d8\u08d9\7\u00fa\2\2\u08d9\u08da\7\31\2\2\u08da\u08e7")
        buf.write("\7!\2\2\u08db\u08dc\7@\2\2\u08dc\u08dd\7\b\2\2\u08dd\u08e2")
        buf.write("\5\u009eP\2\u08de\u08df\7(\2\2\u08df\u08e1\5\u009eP\2")
        buf.write("\u08e0\u08de\3\2\2\2\u08e1\u08e4\3\2\2\2\u08e2\u08e0\3")
        buf.write("\2\2\2\u08e2\u08e3\3\2\2\2\u08e3\u08e5\3\2\2\2\u08e4\u08e2")
        buf.write("\3\2\2\2\u08e5\u08e6\7!\2\2\u08e6\u08e8\3\2\2\2\u08e7")
        buf.write("\u08db\3\2\2\2\u08e7\u08e8\3\2\2\2\u08e8\u08e9\3\2\2\2")
        buf.write("\u08e9\u08ea\5\u0124\u0093\2\u08ea\u08eb\7s\2\2\u08eb")
        buf.write("\u08ec\7 \2\2\u08ec\u010f\3\2\2\2\u08ed\u08f1\5\u0112")
        buf.write("\u008a\2\u08ee\u08f1\5\u0114\u008b\2\u08ef\u08f1\5\u0116")
        buf.write("\u008c\2\u08f0\u08ed\3\2\2\2\u08f0\u08ee\3\2\2\2\u08f0")
        buf.write("\u08ef\3\2\2\2\u08f1\u0111\3\2\2\2\u08f2\u08f3\7m\2\2")
        buf.write("\u08f3\u08f4\7\u0096\2\2\u08f4\u08f5\7\b\2\2\u08f5\u08f6")
        buf.write("\7\u00fa\2\2\u08f6\u08f7\7!\2\2\u08f7\u08f8\7t\2\2\u08f8")
        buf.write("\u08f9\7\b\2\2\u08f9\u08fa\7\u00fa\2\2\u08fa\u08fb\7 ")
        buf.write("\2\2\u08fb\u0113\3\2\2\2\u08fc\u08fd\7N\2\2\u08fd\u08fe")
        buf.write("\7=\2\2\u08fe\u08ff\7\b\2\2\u08ff\u0900\7\u00ec\2\2\u0900")
        buf.write("\u0901\7\30\2\2\u0901\u0902\7\u00fa\2\2\u0902\u0903\7")
        buf.write("\31\2\2\u0903\u0904\7!\2\2\u0904\u0905\7>\2\2\u0905\u0906")
        buf.write("\7\b\2\2\u0906\u0907\7\u00ec\2\2\u0907\u0908\7\30\2\2")
        buf.write("\u0908\u0909\7\u00fa\2\2\u0909\u090a\7\31\2\2\u090a\u0911")
        buf.write("\7!\2\2\u090b\u090c\7>\2\2\u090c\u090d\7\b\2\2\u090d\u090e")
        buf.write("\7\u00ec\2\2\u090e\u090f\7\30\2\2\u090f\u0910\7\u00fa")
        buf.write("\2\2\u0910\u0912\7\31\2\2\u0911\u090b\3\2\2\2\u0911\u0912")
        buf.write("\3\2\2\2\u0912\u0913\3\2\2\2\u0913\u0914\7 \2\2\u0914")
        buf.write("\u0115\3\2\2\2\u0915\u0916\t\b\2\2\u0916\u0917\7\u0087")
        buf.write("\2\2\u0917\u0918\7!\2\2\u0918\u0919\7/\2\2\u0919\u091a")
        buf.write("\7\b\2\2\u091a\u091b\7\u00fa\2\2\u091b\u091c\7!\2\2\u091c")
        buf.write("\u091d\7\62\2\2\u091d\u091e\7\b\2\2\u091e\u091f\7\u00ec")
        buf.write("\2\2\u091f\u0920\7\30\2\2\u0920\u0921\7\u00fa\2\2\u0921")
        buf.write("\u0922\7\31\2\2\u0922\u0923\7!\2\2\u0923\u0924\7=\2\2")
        buf.write("\u0924\u0925\7\b\2\2\u0925\u0926\7\u00ec\2\2\u0926\u0927")
        buf.write("\7\30\2\2\u0927\u0928\7\u00fa\2\2\u0928\u0934\7\31\2\2")
        buf.write("\u0929\u092a\7!\2\2\u092a\u092b\7@\2\2\u092b\u092c\7\b")
        buf.write("\2\2\u092c\u0931\5\u009eP\2\u092d\u092e\7(\2\2\u092e\u0930")
        buf.write("\5\u009eP\2\u092f\u092d\3\2\2\2\u0930\u0933\3\2\2\2\u0931")
        buf.write("\u092f\3\2\2\2\u0931\u0932\3\2\2\2\u0932\u0935\3\2\2\2")
        buf.write("\u0933\u0931\3\2\2\2\u0934\u0929\3\2\2\2\u0934\u0935\3")
        buf.write("\2\2\2\u0935\u093a\3\2\2\2\u0936\u0937\7!\2\2\u0937\u0938")
        buf.write("\7t\2\2\u0938\u0939\7\b\2\2\u0939\u093b\7\u00fa\2\2\u093a")
        buf.write("\u0936\3\2\2\2\u093a\u093b\3\2\2\2\u093b\u093c\3\2\2\2")
        buf.write("\u093c\u093d\7 \2\2\u093d\u0117\3\2\2\2\u093e\u093f\7")
        buf.write("L\2\2\u093f\u0940\7\u00fa\2\2\u0940\u0941\7 \2\2\u0941")
        buf.write("\u0119\3\2\2\2\u0942\u0944\7\u0088\2\2\u0943\u0945\7!")
        buf.write("\2\2\u0944\u0943\3\2\2\2\u0944\u0945\3\2\2\2\u0945\u0946")
        buf.write("\3\2\2\2\u0946\u0947\7.\2\2\u0947\u0948\7\b\2\2\u0948")
        buf.write("\u0949\7\u00ec\2\2\u0949\u094a\7\30\2\2\u094a\u094b\7")
        buf.write("\u00fa\2\2\u094b\u094c\7\31\2\2\u094c\u0957\7!\2\2\u094d")
        buf.write("\u094e\7\u008d\2\2\u094e\u094f\7\u00fa\2\2\u094f\u0950")
        buf.write("\7!\2\2\u0950\u0951\7\u0089\2\2\u0951\u0952\t\t\2\2\u0952")
        buf.write("\u0958\7 \2\2\u0953\u0954\7M\2\2\u0954\u0955\7\b\2\2\u0955")
        buf.write("\u0956\7\u00fa\2\2\u0956\u0958\7 \2\2\u0957\u094d\3\2")
        buf.write("\2\2\u0957\u0953\3\2\2\2\u0958\u011b\3\2\2\2\u0959\u095a")
        buf.write("\7\u00a7\2\2\u095a\u095b\7[\2\2\u095b\u095c\7\b\2\2\u095c")
        buf.write("\u09a4\5L\'\2\u095d\u095e\7!\2\2\u095e\u095f\7\u00a9\2")
        buf.write("\2\u095f\u099c\7\b\2\2\u0960\u0964\7S\2\2\u0961\u0962")
        buf.write("\7\32\2\2\u0962\u0963\7\u00fa\2\2\u0963\u0965\7\33\2\2")
        buf.write("\u0964\u0961\3\2\2\2\u0964\u0965\3\2\2\2\u0965\u099d\3")
        buf.write("\2\2\2\u0966\u096a\7T\2\2\u0967\u0968\7\32\2\2\u0968\u0969")
        buf.write("\7\u00fa\2\2\u0969\u096b\7\33\2\2\u096a\u0967\3\2\2\2")
        buf.write("\u096a\u096b\3\2\2\2\u096b\u099d\3\2\2\2\u096c\u0970\7")
        buf.write("U\2\2\u096d\u096e\7\32\2\2\u096e\u096f\7\u00fa\2\2\u096f")
        buf.write("\u0971\7\33\2\2\u0970\u096d\3\2\2\2\u0970\u0971\3\2\2")
        buf.write("\2\u0971\u099d\3\2\2\2\u0972\u0976\7V\2\2\u0973\u0974")
        buf.write("\7\32\2\2\u0974\u0975\7\u00fa\2\2\u0975\u0977\7\33\2\2")
        buf.write("\u0976\u0973\3\2\2\2\u0976\u0977\3\2\2\2\u0977\u099d\3")
        buf.write("\2\2\2\u0978\u097c\7R\2\2\u0979\u097a\7\32\2\2\u097a\u097b")
        buf.write("\7\u00fa\2\2\u097b\u097d\7\33\2\2\u097c\u0979\3\2\2\2")
        buf.write("\u097c\u097d\3\2\2\2\u097d\u099d\3\2\2\2\u097e\u0982\7")
        buf.write("W\2\2\u097f\u0980\7\32\2\2\u0980\u0981\7\u00fa\2\2\u0981")
        buf.write("\u0983\7\33\2\2\u0982\u097f\3\2\2\2\u0982\u0983\3\2\2")
        buf.write("\2\u0983\u099d\3\2\2\2\u0984\u0988\7X\2\2\u0985\u0986")
        buf.write("\7\32\2\2\u0986\u0987\7\u00fa\2\2\u0987\u0989\7\33\2\2")
        buf.write("\u0988\u0985\3\2\2\2\u0988\u0989\3\2\2\2\u0989\u099d\3")
        buf.write("\2\2\2\u098a\u098e\7Y\2\2\u098b\u098c\7\32\2\2\u098c\u098d")
        buf.write("\7\u00fa\2\2\u098d\u098f\7\33\2\2\u098e\u098b\3\2\2\2")
        buf.write("\u098e\u098f\3\2\2\2\u098f\u099d\3\2\2\2\u0990\u0994\7")
        buf.write("Z\2\2\u0991\u0992\7\32\2\2\u0992\u0993\7\u00fa\2\2\u0993")
        buf.write("\u0995\7\33\2\2\u0994\u0991\3\2\2\2\u0994\u0995\3\2\2")
        buf.write("\2\u0995\u099d\3\2\2\2\u0996\u099a\7\u00fb\2\2\u0997\u0998")
        buf.write("\7\32\2\2\u0998\u0999\7\u00fa\2\2\u0999\u099b\7\33\2\2")
        buf.write("\u099a\u0997\3\2\2\2\u099a\u099b\3\2\2\2\u099b\u099d\3")
        buf.write("\2\2\2\u099c\u0960\3\2\2\2\u099c\u0966\3\2\2\2\u099c\u096c")
        buf.write("\3\2\2\2\u099c\u0972\3\2\2\2\u099c\u0978\3\2\2\2\u099c")
        buf.write("\u097e\3\2\2\2\u099c\u0984\3\2\2\2\u099c\u098a\3\2\2\2")
        buf.write("\u099c\u0990\3\2\2\2\u099c\u0996\3\2\2\2\u099d\u09a1\3")
        buf.write("\2\2\2\u099e\u09a0\5\u011e\u0090\2\u099f\u099e\3\2\2\2")
        buf.write("\u09a0\u09a3\3\2\2\2\u09a1\u099f\3\2\2\2\u09a1\u09a2\3")
        buf.write("\2\2\2\u09a2\u09a5\3\2\2\2\u09a3\u09a1\3\2\2\2\u09a4\u095d")
        buf.write("\3\2\2\2\u09a4\u09a5\3\2\2\2\u09a5\u09ae\3\2\2\2\u09a6")
        buf.write("\u09aa\7!\2\2\u09a7\u09a9\5\u011c\u008f\2\u09a8\u09a7")
        buf.write("\3\2\2\2\u09a9\u09ac\3\2\2\2\u09aa\u09a8\3\2\2\2\u09aa")
        buf.write("\u09ab\3\2\2\2\u09ab\u09ad\3\2\2\2\u09ac\u09aa\3\2\2\2")
        buf.write("\u09ad\u09af\7\u00a8\2\2\u09ae\u09a6\3\2\2\2\u09ae\u09af")
        buf.write("\3\2\2\2\u09af\u09b0\3\2\2\2\u09b0\u09b1\7 \2\2\u09b1")
        buf.write("\u011d\3\2\2\2\u09b2\u09b3\7!\2\2\u09b3\u09b7\7\u00aa")
        buf.write("\2\2\u09b4\u09b5\7\32\2\2\u09b5\u09b6\7\u00fa\2\2\u09b6")
        buf.write("\u09b8\7\33\2\2\u09b7\u09b4\3\2\2\2\u09b7\u09b8\3\2\2")
        buf.write("\2\u09b8\u09bd\3\2\2\2\u09b9\u09ba\7\34\2\2\u09ba\u09bc")
        buf.write("\5\u015e\u00b0\2\u09bb\u09b9\3\2\2\2\u09bc\u09bf\3\2\2")
        buf.write("\2\u09bd\u09bb\3\2\2\2\u09bd\u09be\3\2\2\2\u09be\u09c0")
        buf.write("\3\2\2\2\u09bf\u09bd\3\2\2\2\u09c0\u09c1\7\b\2\2\u09c1")
        buf.write("\u09c2\7\u00fa\2\2\u09c2\u011f\3\2\2\2\u09c3\u09c4\5\u0122")
        buf.write("\u0092\2\u09c4\u09c5\7 \2\2\u09c5\u0121\3\2\2\2\u09c6")
        buf.write("\u09c7\7\u00ab\2\2\u09c7\u0123\3\2\2\2\u09c8\u09cd\5\u0128")
        buf.write("\u0095\2\u09c9\u09ca\7\u00dc\2\2\u09ca\u09cc\5\u0128\u0095")
        buf.write("\2\u09cb\u09c9\3\2\2\2\u09cc\u09cf\3\2\2\2\u09cd\u09cb")
        buf.write("\3\2\2\2\u09cd\u09ce\3\2\2\2\u09ce\u0125\3\2\2\2\u09cf")
        buf.write("\u09cd\3\2\2\2\u09d0\u09d5\5\u0128\u0095\2\u09d1\u09d2")
        buf.write("\7\u00dc\2\2\u09d2\u09d4\5\u0128\u0095\2\u09d3\u09d1\3")
        buf.write("\2\2\2\u09d4\u09d7\3\2\2\2\u09d5\u09d3\3\2\2\2\u09d5\u09d6")
        buf.write("\3\2\2\2\u09d6\u0127\3\2\2\2\u09d7\u09d5\3\2\2\2\u09d8")
        buf.write("\u09dd\5\u012a\u0096\2\u09d9\u09da\7\u00db\2\2\u09da\u09dc")
        buf.write("\5\u012a\u0096\2\u09db\u09d9\3\2\2\2\u09dc\u09df\3\2\2")
        buf.write("\2\u09dd\u09db\3\2\2\2\u09dd\u09de\3\2\2\2\u09de\u0129")
        buf.write("\3\2\2\2\u09df\u09dd\3\2\2\2\u09e0\u09e5\5\u012c\u0097")
        buf.write("\2\u09e1\u09e2\7(\2\2\u09e2\u09e4\5\u012c\u0097\2\u09e3")
        buf.write("\u09e1\3\2\2\2\u09e4\u09e7\3\2\2\2\u09e5\u09e3\3\2\2\2")
        buf.write("\u09e5\u09e6\3\2\2\2\u09e6\u012b\3\2\2\2\u09e7\u09e5\3")
        buf.write("\2\2\2\u09e8\u09ed\5\u012e\u0098\2\u09e9\u09ea\7)\2\2")
        buf.write("\u09ea\u09ec\5\u012e\u0098\2\u09eb\u09e9\3\2\2\2\u09ec")
        buf.write("\u09ef\3\2\2\2\u09ed\u09eb\3\2\2\2\u09ed\u09ee\3\2\2\2")
        buf.write("\u09ee\u012d\3\2\2\2\u09ef\u09ed\3\2\2\2\u09f0\u09f4\5")
        buf.write("\u0132\u009a\2\u09f1\u09f3\5\u0130\u0099\2\u09f2\u09f1")
        buf.write("\3\2\2\2\u09f3\u09f6\3\2\2\2\u09f4\u09f2\3\2\2\2\u09f4")
        buf.write("\u09f5\3\2\2\2\u09f5\u012f\3\2\2\2\u09f6\u09f4\3\2\2\2")
        buf.write("\u09f7\u09f8\7\"\2\2\u09f8\u09fc\5\u0132\u009a\2\u09f9")
        buf.write("\u09fa\7#\2\2\u09fa\u09fc\5\u0132\u009a\2\u09fb\u09f7")
        buf.write("\3\2\2\2\u09fb\u09f9\3\2\2\2\u09fc\u0131\3\2\2\2\u09fd")
        buf.write("\u0a01\5\u0136\u009c\2\u09fe\u0a00\5\u0134\u009b\2\u09ff")
        buf.write("\u09fe\3\2\2\2\u0a00\u0a03\3\2\2\2\u0a01\u09ff\3\2\2\2")
        buf.write("\u0a01\u0a02\3\2\2\2\u0a02\u0133\3\2\2\2\u0a03\u0a01\3")
        buf.write("\2\2\2\u0a04\u0a05\7%\2\2\u0a05\u0a0d\5\u0136\u009c\2")
        buf.write("\u0a06\u0a07\7$\2\2\u0a07\u0a0d\5\u0136\u009c\2\u0a08")
        buf.write("\u0a09\7\'\2\2\u0a09\u0a0d\5\u0136\u009c\2\u0a0a\u0a0b")
        buf.write("\7&\2\2\u0a0b\u0a0d\5\u0136\u009c\2\u0a0c\u0a04\3\2\2")
        buf.write("\2\u0a0c\u0a06\3\2\2\2\u0a0c\u0a08\3\2\2\2\u0a0c\u0a0a")
        buf.write("\3\2\2\2\u0a0d\u0135\3\2\2\2\u0a0e\u0a12\5\u013a\u009e")
        buf.write("\2\u0a0f\u0a11\5\u0138\u009d\2\u0a10\u0a0f\3\2\2\2\u0a11")
        buf.write("\u0a14\3\2\2\2\u0a12\u0a10\3\2\2\2\u0a12\u0a13\3\2\2\2")
        buf.write("\u0a13\u0137\3\2\2\2\u0a14\u0a12\3\2\2\2\u0a15\u0a16\7")
        buf.write("\f\2\2\u0a16\u0a1a\5\u013a\u009e\2\u0a17\u0a18\7\r\2\2")
        buf.write("\u0a18\u0a1a\5\u013a\u009e\2\u0a19\u0a15\3\2\2\2\u0a19")
        buf.write("\u0a17\3\2\2\2\u0a1a\u0139\3\2\2\2\u0a1b\u0a1f\5\u013e")
        buf.write("\u00a0\2\u0a1c\u0a1e\5\u013c\u009f\2\u0a1d\u0a1c\3\2\2")
        buf.write("\2\u0a1e\u0a21\3\2\2\2\u0a1f\u0a1d\3\2\2\2\u0a1f\u0a20")
        buf.write("\3\2\2\2\u0a20\u013b\3\2\2\2\u0a21\u0a1f\3\2\2\2\u0a22")
        buf.write("\u0a23\7\16\2\2\u0a23\u0a27\5\u013e\u00a0\2\u0a24\u0a25")
        buf.write("\7\35\2\2\u0a25\u0a27\5\u013e\u00a0\2\u0a26\u0a22\3\2")
        buf.write("\2\2\u0a26\u0a24\3\2\2\2\u0a27\u013d\3\2\2\2\u0a28\u0a2c")
        buf.write("\5\u0142\u00a2\2\u0a29\u0a2b\5\u0140\u00a1\2\u0a2a\u0a29")
        buf.write("\3\2\2\2\u0a2b\u0a2e\3\2\2\2\u0a2c\u0a2a\3\2\2\2\u0a2c")
        buf.write("\u0a2d\3\2\2\2\u0a2d\u013f\3\2\2\2\u0a2e\u0a2c\3\2\2\2")
        buf.write("\u0a2f\u0a30\7\17\2\2\u0a30\u0a36\5\u0142\u00a2\2\u0a31")
        buf.write("\u0a32\7\37\2\2\u0a32\u0a36\5\u0142\u00a2\2\u0a33\u0a34")
        buf.write("\7\20\2\2\u0a34\u0a36\5\u0142\u00a2\2\u0a35\u0a2f\3\2")
        buf.write("\2\2\u0a35\u0a31\3\2\2\2\u0a35\u0a33\3\2\2\2\u0a36\u0141")
        buf.write("\3\2\2\2\u0a37\u0a39\5\u0144\u00a3\2\u0a38\u0a37\3\2\2")
        buf.write("\2\u0a39\u0a3c\3\2\2\2\u0a3a\u0a38\3\2\2\2\u0a3a\u0a3b")
        buf.write("\3\2\2\2\u0a3b\u0a3d\3\2\2\2\u0a3c\u0a3a\3\2\2\2\u0a3d")
        buf.write("\u0a3e\5\u0146\u00a4\2\u0a3e\u0143\3\2\2\2\u0a3f\u0a40")
        buf.write("\7\30\2\2\u0a40\u0a41\t\n\2\2\u0a41\u0a42\7\31\2\2\u0a42")
        buf.write("\u0145\3\2\2\2\u0a43\u0a4f\5\u0148\u00a5\2\u0a44\u0a4f")
        buf.write("\5\u014a\u00a6\2\u0a45\u0a4f\5\u014c\u00a7\2\u0a46\u0a4f")
        buf.write("\5\u014e\u00a8\2\u0a47\u0a4f\5\u0150\u00a9\2\u0a48\u0a4f")
        buf.write("\5\u016e\u00b8\2\u0a49\u0a4f\5\u0170\u00b9\2\u0a4a\u0a4f")
        buf.write("\5\u0186\u00c4\2\u0a4b\u0a4f\5\u0196\u00cc\2\u0a4c\u0a4d")
        buf.write("\7\u00dd\2\2\u0a4d\u0a4f\5\u0146\u00a4\2\u0a4e\u0a43\3")
        buf.write("\2\2\2\u0a4e\u0a44\3\2\2\2\u0a4e\u0a45\3\2\2\2\u0a4e\u0a46")
        buf.write("\3\2\2\2\u0a4e\u0a47\3\2\2\2\u0a4e\u0a48\3\2\2\2\u0a4e")
        buf.write("\u0a49\3\2\2\2\u0a4e\u0a4a\3\2\2\2\u0a4e\u0a4b\3\2\2\2")
        buf.write("\u0a4e\u0a4c\3\2\2\2\u0a4f\u0147\3\2\2\2\u0a50\u0a51\7")
        buf.write("\u00e7\2\2\u0a51\u0a52\7\30\2\2\u0a52\u0a53\5\u0126\u0094")
        buf.write("\2\u0a53\u0a54\7!\2\2\u0a54\u0a55\5\u0126\u0094\2\u0a55")
        buf.write("\u0a56\7\31\2\2\u0a56\u0149\3\2\2\2\u0a57\u0a58\7\u00e5")
        buf.write("\2\2\u0a58\u0a59\7\30\2\2\u0a59\u0a5a\5\u0126\u0094\2")
        buf.write("\u0a5a\u0a5b\7!\2\2\u0a5b\u0a5c\5\u0126\u0094\2\u0a5c")
        buf.write("\u0a5d\7\31\2\2\u0a5d\u014b\3\2\2\2\u0a5e\u0a5f\7\u00e6")
        buf.write("\2\2\u0a5f\u0a60\7\30\2\2\u0a60\u0a61\5\u0126\u0094\2")
        buf.write("\u0a61\u0a62\7!\2\2\u0a62\u0a63\5\u0126\u0094\2\u0a63")
        buf.write("\u0a64\7!\2\2\u0a64\u0a65\5L\'\2\u0a65\u0a66\7\31\2\2")
        buf.write("\u0a66\u014d\3\2\2\2\u0a67\u0a68\7\30\2\2\u0a68\u0a69")
        buf.write("\5\u0126\u0094\2\u0a69\u0a6a\7\31\2\2\u0a6a\u014f\3\2")
        buf.write("\2\2\u0a6b\u0a77\5\u0152\u00aa\2\u0a6c\u0a77\5\u0154\u00ab")
        buf.write("\2\u0a6d\u0a77\5\u0156\u00ac\2\u0a6e\u0a77\5\u0158\u00ad")
        buf.write("\2\u0a6f\u0a77\5\u015a\u00ae\2\u0a70\u0a77\5\u0160\u00b1")
        buf.write("\2\u0a71\u0a77\5\u0162\u00b2\2\u0a72\u0a77\5\u0164\u00b3")
        buf.write("\2\u0a73\u0a77\5\u0166\u00b4\2\u0a74\u0a77\5\u0168\u00b5")
        buf.write("\2\u0a75\u0a77\5\u016c\u00b7\2\u0a76\u0a6b\3\2\2\2\u0a76")
        buf.write("\u0a6c\3\2\2\2\u0a76\u0a6d\3\2\2\2\u0a76\u0a6e\3\2\2\2")
        buf.write("\u0a76\u0a6f\3\2\2\2\u0a76\u0a70\3\2\2\2\u0a76\u0a71\3")
        buf.write("\2\2\2\u0a76\u0a72\3\2\2\2\u0a76\u0a73\3\2\2\2\u0a76\u0a74")
        buf.write("\3\2\2\2\u0a76\u0a75\3\2\2\2\u0a77\u0151\3\2\2\2\u0a78")
        buf.write("\u0a79\7\u00c7\2\2\u0a79\u0153\3\2\2\2\u0a7a\u0a7b\7\u00c8")
        buf.write("\2\2\u0a7b\u0a7c\7\u00c9\2\2\u0a7c\u0a7d\7\30\2\2\u0a7d")
        buf.write("\u0a7e\7\u00fa\2\2\u0a7e\u0a89\7\31\2\2\u0a7f\u0a80\7")
        buf.write("\"\2\2\u0a80\u0a8a\7\u00fa\2\2\u0a81\u0a82\7$\2\2\u0a82")
        buf.write("\u0a8a\7\u00fa\2\2\u0a83\u0a84\7%\2\2\u0a84\u0a8a\7\u00fa")
        buf.write("\2\2\u0a85\u0a86\7&\2\2\u0a86\u0a8a\7\u00fa\2\2\u0a87")
        buf.write("\u0a88\7\'\2\2\u0a88\u0a8a\7\u00fa\2\2\u0a89\u0a7f\3\2")
        buf.write("\2\2\u0a89\u0a81\3\2\2\2\u0a89\u0a83\3\2\2\2\u0a89\u0a85")
        buf.write("\3\2\2\2\u0a89\u0a87\3\2\2\2\u0a8a\u0155\3\2\2\2\u0a8b")
        buf.write("\u0a8c\7\u00ca\2\2\u0a8c\u0a97\5\u015c\u00af\2\u0a8d\u0a8e")
        buf.write("\7\"\2\2\u0a8e\u0a98\7\u00fa\2\2\u0a8f\u0a90\7$\2\2\u0a90")
        buf.write("\u0a98\7\u00fa\2\2\u0a91\u0a92\7%\2\2\u0a92\u0a98\7\u00fa")
        buf.write("\2\2\u0a93\u0a94\7&\2\2\u0a94\u0a98\7\u00fa\2\2\u0a95")
        buf.write("\u0a96\7\'\2\2\u0a96\u0a98\7\u00fa\2\2\u0a97\u0a8d\3\2")
        buf.write("\2\2\u0a97\u0a8f\3\2\2\2\u0a97\u0a91\3\2\2\2\u0a97\u0a93")
        buf.write("\3\2\2\2\u0a97\u0a95\3\2\2\2\u0a98\u0157\3\2\2\2\u0a99")
        buf.write("\u0a9a\7\u00cb\2\2\u0a9a\u0aa5\5\u015c\u00af\2\u0a9b\u0a9c")
        buf.write("\7\"\2\2\u0a9c\u0aa6\5\u015c\u00af\2\u0a9d\u0a9e\7$\2")
        buf.write("\2\u0a9e\u0aa6\5\u015c\u00af\2\u0a9f\u0aa0\7%\2\2\u0aa0")
        buf.write("\u0aa6\5\u015c\u00af\2\u0aa1\u0aa2\7&\2\2\u0aa2\u0aa6")
        buf.write("\5\u015c\u00af\2\u0aa3\u0aa4\7\'\2\2\u0aa4\u0aa6\5\u015c")
        buf.write("\u00af\2\u0aa5\u0a9b\3\2\2\2\u0aa5\u0a9d\3\2\2\2\u0aa5")
        buf.write("\u0a9f\3\2\2\2\u0aa5\u0aa1\3\2\2\2\u0aa5\u0aa3\3\2\2\2")
        buf.write("\u0aa6\u0159\3\2\2\2\u0aa7\u0aa8\7\u00cc\2\2\u0aa8\u0aa9")
        buf.write("\5\u015c\u00af\2\u0aa9\u0aab\7\"\2\2\u0aaa\u0aac\7\u00fa")
        buf.write("\2\2\u0aab\u0aaa\3\2\2\2\u0aac\u0aad\3\2\2\2\u0aad\u0aab")
        buf.write("\3\2\2\2\u0aad\u0aae\3\2\2\2\u0aae\u015b\3\2\2\2\u0aaf")
        buf.write("\u0ab0\7\u00fb\2\2\u0ab0\u0ab1\7\32\2\2\u0ab1\u0ab2\7")
        buf.write("\u00fa\2\2\u0ab2\u0abc\7\33\2\2\u0ab3\u0ab8\7\u00fb\2")
        buf.write("\2\u0ab4\u0ab5\7\34\2\2\u0ab5\u0ab7\5\u015e\u00b0\2\u0ab6")
        buf.write("\u0ab4\3\2\2\2\u0ab7\u0aba\3\2\2\2\u0ab8\u0ab6\3\2\2\2")
        buf.write("\u0ab8\u0ab9\3\2\2\2\u0ab9\u0abc\3\2\2\2\u0aba\u0ab8\3")
        buf.write("\2\2\2\u0abb\u0aaf\3\2\2\2\u0abb\u0ab3\3\2\2\2\u0abc\u015d")
        buf.write("\3\2\2\2\u0abd\u0ac1\7\u00fb\2\2\u0abe\u0abf\7\32\2\2")
        buf.write("\u0abf\u0ac0\7\u00fa\2\2\u0ac0\u0ac2\7\33\2\2\u0ac1\u0abe")
        buf.write("\3\2\2\2\u0ac1\u0ac2\3\2\2\2\u0ac2\u015f\3\2\2\2\u0ac3")
        buf.write("\u0ac4\7\u00cd\2\2\u0ac4\u0ac5\7\30\2\2\u0ac5\u0ac6\t")
        buf.write("\13\2\2\u0ac6\u0ac7\7\31\2\2\u0ac7\u0161\3\2\2\2\u0ac8")
        buf.write("\u0ac9\7\u00ce\2\2\u0ac9\u0aca\7\30\2\2\u0aca\u0acb\7")
        buf.write("\u00fb\2\2\u0acb\u0acc\7\31\2\2\u0acc\u0163\3\2\2\2\u0acd")
        buf.write("\u0ace\7\u00cf\2\2\u0ace\u0ad4\7\30\2\2\u0acf\u0ad0\7")
        buf.write("\u00ec\2\2\u0ad0\u0ad1\7\30\2\2\u0ad1\u0ad2\7\u00fa\2")
        buf.write("\2\u0ad2\u0ad5\7\31\2\2\u0ad3\u0ad5\7\u00fa\2\2\u0ad4")
        buf.write("\u0acf\3\2\2\2\u0ad4\u0ad3\3\2\2\2\u0ad5\u0ad6\3\2\2\2")
        buf.write("\u0ad6\u0ad7\7\31\2\2\u0ad7\u0165\3\2\2\2\u0ad8\u0ad9")
        buf.write("\7\u00d0\2\2\u0ad9\u0167\3\2\2\2\u0ada\u0adb\7\u00d1\2")
        buf.write("\2\u0adb\u0adc\7\30\2\2\u0adc\u0add\5L\'\2\u0add\u0ade")
        buf.write("\7\31\2\2\u0ade\u0169\3\2\2\2\u0adf\u0ae0\t\f\2\2\u0ae0")
        buf.write("\u016b\3\2\2\2\u0ae1\u0ae2\7\u00d2\2\2\u0ae2\u0ae3\7\30")
        buf.write("\2\2\u0ae3\u0ae8\5X-\2\u0ae4\u0ae5\7(\2\2\u0ae5\u0ae6")
        buf.write("\7@\2\2\u0ae6\u0ae7\7\b\2\2\u0ae7\u0ae9\5\u016a\u00b6")
        buf.write("\2\u0ae8\u0ae4\3\2\2\2\u0ae8\u0ae9\3\2\2\2\u0ae9\u0aea")
        buf.write("\3\2\2\2\u0aea\u0aeb\7\31\2\2\u0aeb\u016d\3\2\2\2\u0aec")
        buf.write("\u0aed\t\r\2\2\u0aed\u016f\3\2\2\2\u0aee\u0af9\5\u0172")
        buf.write("\u00ba\2\u0aef\u0af9\5\u0174\u00bb\2\u0af0\u0af9\5\u0176")
        buf.write("\u00bc\2\u0af1\u0af9\5\u0178\u00bd\2\u0af2\u0af9\5\u017a")
        buf.write("\u00be\2\u0af3\u0af9\5\u017c\u00bf\2\u0af4\u0af9\5\u017e")
        buf.write("\u00c0\2\u0af5\u0af9\5\u0180\u00c1\2\u0af6\u0af9\5\u0182")
        buf.write("\u00c2\2\u0af7\u0af9\5\u0184\u00c3\2\u0af8\u0aee\3\2\2")
        buf.write("\2\u0af8\u0aef\3\2\2\2\u0af8\u0af0\3\2\2\2\u0af8\u0af1")
        buf.write("\3\2\2\2\u0af8\u0af2\3\2\2\2\u0af8\u0af3\3\2\2\2\u0af8")
        buf.write("\u0af4\3\2\2\2\u0af8\u0af5\3\2\2\2\u0af8\u0af6\3\2\2\2")
        buf.write("\u0af8\u0af7\3\2\2\2\u0af9\u0171\3\2\2\2\u0afa\u0afb\7")
        buf.write("\u00da\2\2\u0afb\u0afc\7\30\2\2\u0afc\u0afd\5\u0126\u0094")
        buf.write("\2\u0afd\u0afe\7\31\2\2\u0afe\u0173\3\2\2\2\u0aff\u0b00")
        buf.write("\7\u00df\2\2\u0b00\u0b01\7\30\2\2\u0b01\u0b02\5\u0126")
        buf.write("\u0094\2\u0b02\u0b03\7\31\2\2\u0b03\u0175\3\2\2\2\u0b04")
        buf.write("\u0b05\7\u00e8\2\2\u0b05\u0b0d\7\30\2\2\u0b06\u0b07\7")
        buf.write("*\2\2\u0b07\u0b08\7\b\2\2\u0b08\u0b09\7\u00ec\2\2\u0b09")
        buf.write("\u0b0a\7\30\2\2\u0b0a\u0b0b\7\u00fa\2\2\u0b0b\u0b0c\7")
        buf.write("\31\2\2\u0b0c\u0b0e\7!\2\2\u0b0d\u0b06\3\2\2\2\u0b0d\u0b0e")
        buf.write("\3\2\2\2\u0b0e\u0b14\3\2\2\2\u0b0f\u0b10\7[\2\2\u0b10")
        buf.write("\u0b11\7\b\2\2\u0b11\u0b12\5L\'\2\u0b12\u0b13\7!\2\2\u0b13")
        buf.write("\u0b15\3\2\2\2\u0b14\u0b0f\3\2\2\2\u0b14\u0b15\3\2\2\2")
        buf.write("\u0b15\u0b16\3\2\2\2\u0b16\u0b17\5\u0126\u0094\2\u0b17")
        buf.write("\u0b18\7\31\2\2\u0b18\u0177\3\2\2\2\u0b19\u0b1a\7\u00e9")
        buf.write("\2\2\u0b1a\u0b1b\7\30\2\2\u0b1b\u0b1c\5\u0126\u0094\2")
        buf.write("\u0b1c\u0b1d\7\31\2\2\u0b1d\u0179\3\2\2\2\u0b1e\u0b1f")
        buf.write("\7\u00e0\2\2\u0b1f\u0b20\7\30\2\2\u0b20\u0b21\5\u0126")
        buf.write("\u0094\2\u0b21\u0b22\7\31\2\2\u0b22\u017b\3\2\2\2\u0b23")
        buf.write("\u0b28\7\u00e1\2\2\u0b24\u0b25\7\21\2\2\u0b25\u0b26\7")
        buf.write("\b\2\2\u0b26\u0b27\7\u00fa\2\2\u0b27\u0b29\7!\2\2\u0b28")
        buf.write("\u0b24\3\2\2\2\u0b28\u0b29\3\2\2\2\u0b29\u0b2a\3\2\2\2")
        buf.write("\u0b2a\u0b2b\7\30\2\2\u0b2b\u0b2c\5\u0126\u0094\2\u0b2c")
        buf.write("\u0b2d\7\31\2\2\u0b2d\u017d\3\2\2\2\u0b2e\u0b2f\7\u00e2")
        buf.write("\2\2\u0b2f\u0b30\7\30\2\2\u0b30\u0b31\5\u0126\u0094\2")
        buf.write("\u0b31\u0b32\7\31\2\2\u0b32\u017f\3\2\2\2\u0b33\u0b34")
        buf.write("\7\u00e3\2\2\u0b34\u0b35\7\30\2\2\u0b35\u0b36\5\u0126")
        buf.write("\u0094\2\u0b36\u0b37\7\31\2\2\u0b37\u0181\3\2\2\2\u0b38")
        buf.write("\u0b39\7\u00e4\2\2\u0b39\u0b3a\7\30\2\2\u0b3a\u0b3b\5")
        buf.write("\u0126\u0094\2\u0b3b\u0b3c\7\31\2\2\u0b3c\u0183\3\2\2")
        buf.write("\2\u0b3d\u0b3e\7\u00de\2\2\u0b3e\u0b3f\7\30\2\2\u0b3f")
        buf.write("\u0b44\5X-\2\u0b40\u0b41\7(\2\2\u0b41\u0b42\7@\2\2\u0b42")
        buf.write("\u0b43\7\b\2\2\u0b43\u0b45\5\u016a\u00b6\2\u0b44\u0b40")
        buf.write("\3\2\2\2\u0b44\u0b45\3\2\2\2\u0b45\u0b46\3\2\2\2\u0b46")
        buf.write("\u0b47\7!\2\2\u0b47\u0b48\5\u0126\u0094\2\u0b48\u0b49")
        buf.write("\7\31\2\2\u0b49\u0185\3\2\2\2\u0b4a\u0b50\5\u0188\u00c5")
        buf.write("\2\u0b4b\u0b50\5\u018a\u00c6\2\u0b4c\u0b50\5\u018e\u00c8")
        buf.write("\2\u0b4d\u0b50\5\u0190\u00c9\2\u0b4e\u0b50\5\u0192\u00ca")
        buf.write("\2\u0b4f\u0b4a\3\2\2\2\u0b4f\u0b4b\3\2\2\2\u0b4f\u0b4c")
        buf.write("\3\2\2\2\u0b4f\u0b4d\3\2\2\2\u0b4f\u0b4e\3\2\2\2\u0b50")
        buf.write("\u0187\3\2\2\2\u0b51\u0b52\7\u00c2\2\2\u0b52\u0b53\7\30")
        buf.write("\2\2\u0b53\u0b54\5\u0126\u0094\2\u0b54\u0b55\7\22\2\2")
        buf.write("\u0b55\u0b56\5\u0126\u0094\2\u0b56\u0b57\7\36\2\2\u0b57")
        buf.write("\u0b58\5\u0126\u0094\2\u0b58\u0b59\7\31\2\2\u0b59\u0189")
        buf.write("\3\2\2\2\u0b5a\u0b5b\7\u00c3\2\2\u0b5b\u0b5c\7\30\2\2")
        buf.write("\u0b5c\u0b61\5\u018c\u00c7\2\u0b5d\u0b5e\7(\2\2\u0b5e")
        buf.write("\u0b60\5\u018c\u00c7\2\u0b5f\u0b5d\3\2\2\2\u0b60\u0b63")
        buf.write("\3\2\2\2\u0b61\u0b5f\3\2\2\2\u0b61\u0b62\3\2\2\2\u0b62")
        buf.write("\u0b64\3\2\2\2\u0b63\u0b61\3\2\2\2\u0b64\u0b65\7!\2\2")
        buf.write("\u0b65\u0b66\5\u0126\u0094\2\u0b66\u0b67\7!\2\2\u0b67")
        buf.write("\u0b68\5\u0126\u0094\2\u0b68\u0b69\7!\2\2\u0b69\u0b6a")
        buf.write("\5\u0126\u0094\2\u0b6a\u0b6b\7\31\2\2\u0b6b\u018b\3\2")
        buf.write("\2\2\u0b6c\u0b6d\t\16\2\2\u0b6d\u018d\3\2\2\2\u0b6e\u0b6f")
        buf.write("\7\u00c4\2\2\u0b6f\u0b70\7\30\2\2\u0b70\u0b71\5\u0126")
        buf.write("\u0094\2\u0b71\u0b72\7!\2\2\u0b72\u0b73\5\u0126\u0094")
        buf.write("\2\u0b73\u0b74\7!\2\2\u0b74\u0b75\5\u0126\u0094\2\u0b75")
        buf.write("\u0b76\7\31\2\2\u0b76\u018f\3\2\2\2\u0b77\u0b78\7\u00c5")
        buf.write("\2\2\u0b78\u0b79\7\30\2\2\u0b79\u0b7a\5\u0126\u0094\2")
        buf.write("\u0b7a\u0b7b\7!\2\2\u0b7b\u0b7c\5\u0126\u0094\2\u0b7c")
        buf.write("\u0b7d\7!\2\2\u0b7d\u0b7e\5\u0126\u0094\2\u0b7e\u0b7f")
        buf.write("\7\31\2\2\u0b7f\u0191\3\2\2\2\u0b80\u0b81\7\u00c6\2\2")
        buf.write("\u0b81\u0b82\7\30\2\2\u0b82\u0b83\7@\2\2\u0b83\u0b84\7")
        buf.write("\b\2\2\u0b84\u0b89\5\u0194\u00cb\2\u0b85\u0b86\7(\2\2")
        buf.write("\u0b86\u0b88\5\u0194\u00cb\2\u0b87\u0b85\3\2\2\2\u0b88")
        buf.write("\u0b8b\3\2\2\2\u0b89\u0b87\3\2\2\2\u0b89\u0b8a\3\2\2\2")
        buf.write("\u0b8a\u0b8c\3\2\2\2\u0b8b\u0b89\3\2\2\2\u0b8c\u0b8d\7")
        buf.write("!\2\2\u0b8d\u0b8e\5\u0126\u0094\2\u0b8e\u0b8f\7!\2\2\u0b8f")
        buf.write("\u0b90\5\u0126\u0094\2\u0b90\u0b91\7!\2\2\u0b91\u0b92")
        buf.write("\5\u0126\u0094\2\u0b92\u0b93\7\31\2\2\u0b93\u0193\3\2")
        buf.write("\2\2\u0b94\u0b95\t\17\2\2\u0b95\u0195\3\2\2\2\u0b96\u0b97")
        buf.write("\7\u00ea\2\2\u0b97\u0b98\7\30\2\2\u0b98\u0b99\5\u0126")
        buf.write("\u0094\2\u0b99\u0ba1\7\36\2\2\u0b9a\u0b9b\5\u0124\u0093")
        buf.write("\2\u0b9b\u0b9c\7!\2\2\u0b9c\u0b9d\5\u0124\u0093\2\u0b9d")
        buf.write("\u0b9e\7 \2\2\u0b9e\u0ba0\3\2\2\2\u0b9f\u0b9a\3\2\2\2")
        buf.write("\u0ba0\u0ba3\3\2\2\2\u0ba1\u0b9f\3\2\2\2\u0ba1\u0ba2\3")
        buf.write("\2\2\2\u0ba2\u0ba4\3\2\2\2\u0ba3\u0ba1\3\2\2\2\u0ba4\u0ba5")
        buf.write("\7\31\2\2\u0ba5\u0197\3\2\2\2\u00f1\u019b\u019d\u01a1")
        buf.write("\u01a8\u01ac\u01af\u01b7\u01bc\u01c0\u01c3\u01c9\u01ce")
        buf.write("\u01d2\u01d5\u01db\u01ed\u01ef\u01f7\u0200\u0209\u0212")
        buf.write("\u021b\u0224\u022d\u0236\u023f\u0248\u024e\u0256\u025e")
        buf.write("\u0266\u0284\u028b\u0292\u029b\u029f\u02a3\u02aa\u02b4")
        buf.write("\u02c1\u02d0\u02e5\u02eb\u0307\u030d\u0316\u0329\u0339")
        buf.write("\u0344\u0375\u0385\u038c\u0392\u03aa\u03b5\u03b8\u03bb")
        buf.write("\u03e2\u03e6\u03f3\u03fa\u040c\u0421\u0436\u043c\u0452")
        buf.write("\u045f\u0463\u0468\u046e\u0471\u0475\u0479\u0480\u049a")
        buf.write("\u04a4\u04ac\u04b0\u04b6\u04ba\u04e8\u04ec\u04f3\u04f9")
        buf.write("\u04fd\u0506\u050b\u0517\u0522\u052f\u0540\u0545\u054a")
        buf.write("\u055b\u0560\u0565\u056d\u058d\u0592\u0597\u05a1\u05ab")
        buf.write("\u05b7\u05bc\u05c1\u05cd\u05d2\u05d7\u05e1\u05e7\u0602")
        buf.write("\u0608\u0612\u0620\u062d\u0632\u0636\u0641\u0647\u0652")
        buf.write("\u065b\u0665\u0677\u067c\u0680\u068b\u0691\u069b\u06a2")
        buf.write("\u06aa\u06b1\u06bd\u06c8\u06cb\u06d6\u06db\u06e5\u06eb")
        buf.write("\u06fe\u0704\u070e\u0714\u0722\u072d\u0732\u073b\u0742")
        buf.write("\u074d\u0754\u075e\u07a9\u07ae\u07b1\u07c2\u07c8\u07cf")
        buf.write("\u07dc\u0827\u082c\u082f\u0840\u0846\u084d\u0856\u0867")
        buf.write("\u086d\u0875\u0883\u0888\u0895\u089a\u08a7\u08ac\u08b3")
        buf.write("\u08c1\u08c6\u08cd\u08e2\u08e7\u08f0\u0911\u0931\u0934")
        buf.write("\u093a\u0944\u0957\u0964\u096a\u0970\u0976\u097c\u0982")
        buf.write("\u0988\u098e\u0994\u099a\u099c\u09a1\u09a4\u09aa\u09ae")
        buf.write("\u09b7\u09bd\u09cd\u09d5\u09dd\u09e5\u09ed\u09f4\u09fb")
        buf.write("\u0a01\u0a0c\u0a12\u0a19\u0a1f\u0a26\u0a2c\u0a35\u0a3a")
        buf.write("\u0a4e\u0a76\u0a89\u0a97\u0aa5\u0aad\u0ab8\u0abb\u0ac1")
        buf.write("\u0ad4\u0ae8\u0af8\u0b0d\u0b14\u0b28\u0b44\u0b4f\u0b61")
        buf.write("\u0b89\u0ba1")
        return buf.getvalue()


class VfrSyntaxParser ( Parser ):

    grammarFileName = "VfrSyntax.g4"

    atn = ATNDeserializer().deserialize(serializedATN())

    decisionsToDFA = [ DFA(ds, i) for i, ds in enumerate(atn.decisionToState) ]

    sharedContextCache = PredictionContextCache()

    literalNames = [ "<INVALID>", "'show'", "'push'", "'pop'", "'#pragma'",
                     "'pack'", "'='", "'IMAGE_TOKEN'", "'HORIZONTAL'", "'MULTI_LINE'",
                     "'<<'", "'>>'", "'+'", "'*'", "'%'", "'format'", "'?'",
                     "'#define'", "'#include'", "'formpkgtype'", "'{'",
                     "'}'", "'('", "')'", "'['", "']'", "'.'", "'-'", "':'",
                     "'/'", "';'", "','", "'=='", "'!='", "'<='", "'<'",
                     "'>='", "'>'", "'|'", "'&'", "'devicepath'", "'formset'",
                     "'formsetid'", "'endformset'", "'title'", "'formid'",
                     "'oneof'", "'endoneof'", "'prompt'", "'orderedlist'",
                     "'maxcontainers'", "'endlist'", "'endform'", "'form'",
                     "'formmap'", "'maptitle'", "'mapguid'", "'subtitle'",
                     "'endsubtitle'", "'help'", "'text'", "'option'", "'flags'",
                     "'date'", "'enddate'", "'year'", "'month'", "'day'",
                     "'time'", "'endtime'", "'hour'", "'minute'", "'second'",
                     "'grayoutif'", "'label'", "'timeout'", "'inventory'",
                     "'_NON_NV_DATA_MAP'", "'struct'", "'union'", "'BOOLEAN'",
                     "'UINT64'", "'UINT32'", "'UINT16'", "'UINT8'", "'EFI_STRING_ID'",
                     "'EFI_HII_DATE'", "'EFI_HII_TIME'", "'EFI_HII_REF'",
                     "'guid'", "'checkbox'", "'endcheckbox'", "'numeric'",
                     "'endnumeric'", "'minimum'", "'maximum'", "'step'",
                     "'default'", "'password'", "'endpassword'", "'string'",
                     "'endstring'", "'minsize'", "'maxsize'", "'encoding'",
                     "'suppressif'", "'disableif'", "'hidden'", "'goto'",
                     "'formsetguid'", "'inconsistentif'", "'warningif'",
                     "'nosubmitif'", "'endif'", "'key'", "'DEFAULT'", "'MANUFACTURING'",
                     "'CHECKBOX_DEFAULT'", "'CHECKBOX_DEFAULT_MFG'", "'INTERACTIVE'",
                     "'NV_ACCESS'", "'RESET_REQUIRED'", "'RECONNECT_REQUIRED'",
                     "'LATE_CHECK'", "'READ_ONLY'", "'OPTIONS_ONLY'", "'REST_STYLE'",
                     "'class'", "'subclass'", "'classguid'", "'typedef'",
                     "'restore'", "'save'", "'defaults'", "'banner'", "'align'",
                     "'left'", "'right'", "'center'", "'line'", "'name'",
                     "'varid'", "'question'", "'questionid'", "'image'",
                     "'locked'", "'rule'", "'endrule'", "'value'", "'read'",
                     "'write'", "'resetbutton'", "'endresetbutton'", "'defaultstore'",
                     "'attribute'", "'varstore'", "'efivarstore'", "'varsize'",
                     "'namevaluevarstore'", "'action'", "'config'", "'endaction'",
                     "'refresh'", "'interval'", "'varstoredevice'", "'guidop'",
                     "'endguidop'", "'datatype'", "'data'", "'modal'", "'NON_DEVICE'",
                     "'DISK_DEVICE'", "'VIDEO_DEVICE'", "'NETWORK_DEVICE'",
                     "'INPUT_DEVICE'", "'ONBOARD_DEVICE'", "'OTHER_DEVICE'",
                     "'SETUP_APPLICATION'", "'GENERAL_APPLICATION'", "'FRONT_PAGE'",
                     "'SINGLE_USE'", "'YEAR_SUPPRESS'", "'MONTH_SUPPRESS'",
                     "'DAY_SUPPRESS'", "'HOUR_SUPPRESS'", "'MINUTE_SUPPRESS'",
                     "'SECOND_SUPPRESS'", "'STORAGE_NORMAL'", "'STORAGE_TIME'",
                     "'STORAGE_WAKEUP'", "'UNIQUE'", "'NOEMPTY'", "'cond'",
                     "'find'", "'mid'", "'token'", "'span'", "'dup'", "'vareqval'",
                     "'var'", "'ideqval'", "'ideqid'", "'ideqvallist'",
                     "'questionref'", "'ruleref'", "'stringref'", "'pushthis'",
                     "'security'", "'get'", "'TRUE'", "'FALSE'", "'ONE'",
                     "'ONES'", "'ZERO'", "'UNDEFINED'", "'VERSION'", "'length'",
                     "'AND'", "'OR'", "'NOT'", "'set'", "'~'", "'boolval'",
                     "'stringval'", "'unintval'", "'toupper'", "'tolower'",
                     "'match'", "'match2'", "'catenate'", "'questionrefval'",
                     "'stringrefval'", "'map'", "'refreshguid'", "'STRING_TOKEN'",
                     "'OPTION_DEFAULT'", "'OPTION_DEFAULT_MFG'", "'NUMERIC_SIZE_1'",
                     "'NUMERIC_SIZE_2'", "'NUMERIC_SIZE_4'", "'NUMERIC_SIZE_8'",
                     "'DISPLAY_INT_DEC'", "'DISPLAY_UINT_DEC'", "'DISPLAY_UINT_HEX'",
                     "'INSENSITIVE'", "'SENSITIVE'", "'LAST_NON_MATCH'",
                     "'FIRST_NON_MATCH'" ]

    symbolicNames = [ "<INVALID>", "<INVALID>", "<INVALID>", "<INVALID>",
                      "<INVALID>", "<INVALID>", "<INVALID>", "<INVALID>",
                      "<INVALID>", "<INVALID>", "<INVALID>", "<INVALID>",
                      "<INVALID>", "<INVALID>", "<INVALID>", "<INVALID>",
                      "<INVALID>", "Define", "Include", "FormPkgType", "OpenBrace",
                      "CloseBrace", "OpenParen", "CloseParen", "OpenBracket",
                      "CloseBracket", "Dot", "Negative", "Colon", "Slash",
                      "Semicolon", "Comma", "Equal", "NotEqual", "LessEqual",
                      "Less", "GreaterEqual", "Greater", "BitWiseOr", "BitWiseAnd",
                      "DevicePath", "FormSet", "FormSetId", "EndFormSet",
                      "Title", "FormId", "OneOf", "EndOneOf", "Prompt",
                      "OrderedList", "MaxContainers", "EndList", "EndForm",
                      "Form", "FormMap", "MapTitle", "MapGuid", "Subtitle",
                      "EndSubtitle", "Help", "Text", "Option", "FLAGS",
                      "Date", "EndDate", "Year", "Month", "Day", "Time",
                      "EndTime", "Hour", "Minute", "Second", "GrayOutIf",
                      "Label", "Timeout", "Inventory", "NonNvDataMap", "Struct",
                      "Union", "Boolean", "Uint64", "Uint32", "Uint16",
                      "Uint8", "EFI_STRING_ID", "EFI_HII_DATE", "EFI_HII_TIME",
                      "EFI_HII_REF", "Uuid", "CheckBox", "EndCheckBox",
                      "Numeric", "EndNumeric", "Minimum", "Maximum", "Step",
                      "Default", "Password", "EndPassword", "String", "EndString",
                      "MinSize", "MaxSize", "Encoding", "SuppressIf", "DisableIf",
                      "Hidden", "Goto", "FormSetGuid", "InconsistentIf",
                      "WarningIf", "NoSubmitIf", "EndIf", "Key", "DefaultFlag",
                      "ManufacturingFlag", "CheckBoxDefaultFlag", "CheckBoxDefaultMfgFlag",
                      "InteractiveFlag", "NVAccessFlag", "ResetRequiredFlag",
                      "ReconnectRequiredFlag", "LateCheckFlag", "ReadOnlyFlag",
                      "OptionOnlyFlag", "RestStyleFlag", "Class", "Subclass",
                      "ClassGuid", "TypeDef", "Restore", "Save", "Defaults",
                      "Banner", "Align", "Left", "Right", "Center", "Line",
                      "Name", "VarId", "Question", "QuestionId", "Image",
                      "Locked", "Rule", "EndRule", "Value", "Read", "Write",
                      "ResetButton", "EndResetButton", "DefaultStore", "Attribute",
                      "Varstore", "Efivarstore", "VarSize", "NameValueVarStore",
                      "Action", "Config", "EndAction", "Refresh", "Interval",
                      "VarstoreDevice", "GuidOp", "EndGuidOp", "DataType",
                      "Data", "Modal", "ClassNonDevice", "ClassDiskDevice",
                      "ClassVideoDevice", "ClassNetworkDevice", "ClassInputDevice",
                      "ClassOnBoardDevice", "ClassOtherDevice", "SubclassSetupApplication",
                      "SubclassGeneralApplication", "SubclassFrontPage",
                      "SubclassSingleUse", "YearSupppressFlag", "MonthSuppressFlag",
                      "DaySuppressFlag", "HourSupppressFlag", "MinuteSuppressFlag",
                      "SecondSuppressFlag", "StorageNormalFlag", "StorageTimeFlag",
                      "StorageWakeUpFlag", "UniQueFlag", "NoEmptyFlag",
                      "Cond", "Find", "Mid", "Tok", "Span", "Dup", "VarEqVal",
                      "Var", "IdEqVal", "IdEqId", "IdEqValList", "QuestionRef",
                      "RuleRef", "StringRef", "PushThis", "Security", "Get",
                      "TrueSymbol", "FalseSymbol", "One", "Ones", "Zero",
                      "Undefined", "Version", "Length", "AND", "OR", "NOT",
                      "Set", "BitWiseNot", "BoolVal", "StringVal", "UnIntVal",
                      "ToUpper", "ToLower", "Match", "Match2", "Catenate",
                      "QuestionRefVal", "StringRefVal", "Map", "RefreshGuid",
                      "StringToken", "OptionDefault", "OptionDefaultMfg",
                      "NumericSizeOne", "NumericSizeTwo", "NumericSizeFour",
                      "NumericSizeEight", "DisPlayIntDec", "DisPlayUIntDec",
                      "DisPlayUIntHex", "Insensitive", "Sensitive", "LastNonMatch",
                      "FirstNonMatch", "Number", "StringIdentifier", "ComplexDefine",
                      "LineDefinition", "IncludeDefinition", "Whitespace",
                      "Newline", "LineComment", "Extern" ]

    RULE_vfrProgram = 0
    RULE_pragmaPackShowDef = 1
    RULE_pragmaPackStackDef = 2
    RULE_pragmaPackNumber = 3
    RULE_vfrPragmaPackDefinition = 4
    RULE_vfrDataStructDefinition = 5
    RULE_vfrDataUnionDefinition = 6
    RULE_vfrDataStructFields = 7
    RULE_dataStructField64 = 8
    RULE_dataStructField32 = 9
    RULE_dataStructField16 = 10
    RULE_dataStructField8 = 11
    RULE_dataStructFieldBool = 12
    RULE_dataStructFieldString = 13
    RULE_dataStructFieldDate = 14
    RULE_dataStructFieldTime = 15
    RULE_dataStructFieldRef = 16
    RULE_dataStructFieldUser = 17
    RULE_dataStructBitField64 = 18
    RULE_dataStructBitField32 = 19
    RULE_dataStructBitField16 = 20
    RULE_dataStructBitField8 = 21
    RULE_vfrFormSetDefinition = 22
    RULE_classguidDefinition = 23
    RULE_classDefinition = 24
    RULE_validClassNames = 25
    RULE_subclassDefinition = 26
    RULE_vfrFormSetList = 27
    RULE_vfrFormSet = 28
    RULE_vfrStatementDefaultStore = 29
    RULE_vfrStatementVarStoreLinear = 30
    RULE_vfrStatementVarStoreEfi = 31
    RULE_vfrVarStoreEfiAttr = 32
    RULE_vfrStatementVarStoreNameValue = 33
    RULE_vfrStatementDisableIfFormSet = 34
    RULE_vfrStatementSuppressIfFormSet = 35
    RULE_guidSubDefinition = 36
    RULE_guidDefinition = 37
    RULE_getStringId = 38
    RULE_vfrQuestionHeader = 39
    RULE_vfrQuestionBaseInfo = 40
    RULE_vfrStatementHeader = 41
    RULE_questionheaderFlagsField = 42
    RULE_vfrStorageVarId = 43
    RULE_vfrConstantValueField = 44
    RULE_vfrImageTag = 45
    RULE_vfrLockedTag = 46
    RULE_vfrStatementStatTag = 47
    RULE_vfrStatementStatTagList = 48
    RULE_vfrFormDefinition = 49
    RULE_vfrForm = 50
    RULE_vfrFormMapDefinition = 51
    RULE_vfrStatementImage = 52
    RULE_vfrStatementLocked = 53
    RULE_vfrStatementRules = 54
    RULE_vfrStatementStat = 55
    RULE_vfrStatementSubTitle = 56
    RULE_vfrStatementSubTitleComponent = 57
    RULE_vfrSubtitleFlags = 58
    RULE_subtitleFlagsField = 59
    RULE_vfrStatementStaticText = 60
    RULE_staticTextFlagsField = 61
    RULE_vfrStatementCrossReference = 62
    RULE_vfrStatementGoto = 63
    RULE_vfrGotoFlags = 64
    RULE_gotoFlagsField = 65
    RULE_vfrStatementResetButton = 66
    RULE_vfrStatementQuestions = 67
    RULE_vfrStatementQuestionTag = 68
    RULE_vfrStatementInconsistentIf = 69
    RULE_vfrStatementNoSubmitIf = 70
    RULE_vfrStatementDisableIfQuest = 71
    RULE_vfrStatementRefresh = 72
    RULE_vfrStatementVarstoreDevice = 73
    RULE_vfrStatementRefreshEvent = 74
    RULE_vfrStatementWarningIf = 75
    RULE_vfrStatementQuestionTagList = 76
    RULE_vfrStatementQuestionOptionTag = 77
    RULE_flagsField = 78
    RULE_vfrStatementSuppressIfQuest = 79
    RULE_vfrStatementGrayOutIfQuest = 80
    RULE_vfrStatementDefault = 81
    RULE_vfrStatementValue = 82
    RULE_vfrStatementOptions = 83
    RULE_vfrStatementOneOfOption = 84
    RULE_vfrOneOfOptionFlags = 85
    RULE_oneofoptionFlagsField = 86
    RULE_vfrStatementRead = 87
    RULE_vfrStatementWrite = 88
    RULE_vfrStatementQuestionOptionList = 89
    RULE_vfrStatementQuestionOption = 90
    RULE_vfrStatementBooleanType = 91
    RULE_vfrStatementCheckBox = 92
    RULE_vfrCheckBoxFlags = 93
    RULE_checkboxFlagsField = 94
    RULE_vfrStatementAction = 95
    RULE_vfrActionFlags = 96
    RULE_actionFlagsField = 97
    RULE_vfrStatementNumericType = 98
    RULE_vfrStatementNumeric = 99
    RULE_vfrSetMinMaxStep = 100
    RULE_vfrNumericFlags = 101
    RULE_numericFlagsField = 102
    RULE_vfrStatementOneOf = 103
    RULE_vfrOneofFlagsField = 104
    RULE_vfrStatementStringType = 105
    RULE_vfrStatementString = 106
    RULE_vfrStringFlagsField = 107
    RULE_stringFlagsField = 108
    RULE_vfrStatementPassword = 109
    RULE_vfrPasswordFlagsField = 110
    RULE_passwordFlagsField = 111
    RULE_vfrStatementOrderedList = 112
    RULE_vfrOrderedListFlags = 113
    RULE_orderedlistFlagsField = 114
    RULE_vfrStatementDate = 115
    RULE_minMaxDateStepDefault = 116
    RULE_vfrDateFlags = 117
    RULE_dateFlagsField = 118
    RULE_vfrStatementTime = 119
    RULE_minMaxTimeStepDefault = 120
    RULE_vfrTimeFlags = 121
    RULE_timeFlagsField = 122
    RULE_vfrStatementConditional = 123
    RULE_vfrStatementConditionalNew = 124
    RULE_vfrStatementSuppressIfStat = 125
    RULE_vfrStatementGrayOutIfStat = 126
    RULE_vfrStatementStatList = 127
    RULE_vfrStatementStatListOld = 128
    RULE_vfrStatementDisableIfStat = 129
    RULE_vfrStatementgrayoutIfSuppressIf = 130
    RULE_vfrStatementsuppressIfGrayOutIf = 131
    RULE_vfrStatementSuppressIfStatNew = 132
    RULE_vfrStatementGrayOutIfStatNew = 133
    RULE_vfrStatementInconsistentIfStat = 134
    RULE_vfrStatementInvalid = 135
    RULE_vfrStatementInvalidHidden = 136
    RULE_vfrStatementInvalidInventory = 137
    RULE_vfrStatementInvalidSaveRestoreDefaults = 138
    RULE_vfrStatementLabel = 139
    RULE_vfrStatementBanner = 140
    RULE_vfrStatementExtension = 141
    RULE_vfrExtensionData = 142
    RULE_vfrStatementModal = 143
    RULE_vfrModalTag = 144
    RULE_vfrStatementExpression = 145
    RULE_vfrStatementExpressionSub = 146
    RULE_andTerm = 147
    RULE_bitwiseorTerm = 148
    RULE_bitwiseandTerm = 149
    RULE_equalTerm = 150
    RULE_equalTermSupplementary = 151
    RULE_compareTerm = 152
    RULE_compareTermSupplementary = 153
    RULE_shiftTerm = 154
    RULE_shiftTermSupplementary = 155
    RULE_addMinusTerm = 156
    RULE_addMinusTermSupplementary = 157
    RULE_multdivmodTerm = 158
    RULE_multdivmodTermSupplementary = 159
    RULE_castTerm = 160
    RULE_castTermSub = 161
    RULE_atomTerm = 162
    RULE_vfrExpressionCatenate = 163
    RULE_vfrExpressionMatch = 164
    RULE_vfrExpressionMatch2 = 165
    RULE_vfrExpressionParen = 166
    RULE_vfrExpressionBuildInFunction = 167
    RULE_dupExp = 168
    RULE_vareqvalExp = 169
    RULE_ideqvalExp = 170
    RULE_ideqidExp = 171
    RULE_ideqvallistExp = 172
    RULE_vfrQuestionDataFieldName = 173
    RULE_arrayName = 174
    RULE_questionref1Exp = 175
    RULE_rulerefExp = 176
    RULE_stringref1Exp = 177
    RULE_pushthisExp = 178
    RULE_securityExp = 179
    RULE_numericVarStoreType = 180
    RULE_getExp = 181
    RULE_vfrExpressionConstant = 182
    RULE_vfrExpressionUnaryOp = 183
    RULE_lengthExp = 184
    RULE_bitwisenotExp = 185
    RULE_question23refExp = 186
    RULE_stringref2Exp = 187
    RULE_toboolExp = 188
    RULE_tostringExp = 189
    RULE_unintExp = 190
    RULE_toupperExp = 191
    RULE_tolwerExp = 192
    RULE_setExp = 193
    RULE_vfrExpressionTernaryOp = 194
    RULE_conditionalExp = 195
    RULE_findExp = 196
    RULE_findFormat = 197
    RULE_midExp = 198
    RULE_tokenExp = 199
    RULE_spanExp = 200
    RULE_spanFlags = 201
    RULE_vfrExpressionMap = 202

    ruleNames =  [ "vfrProgram", "pragmaPackShowDef", "pragmaPackStackDef",
                   "pragmaPackNumber", "vfrPragmaPackDefinition", "vfrDataStructDefinition",
                   "vfrDataUnionDefinition", "vfrDataStructFields", "dataStructField64",
                   "dataStructField32", "dataStructField16", "dataStructField8",
                   "dataStructFieldBool", "dataStructFieldString", "dataStructFieldDate",
                   "dataStructFieldTime", "dataStructFieldRef", "dataStructFieldUser",
                   "dataStructBitField64", "dataStructBitField32", "dataStructBitField16",
                   "dataStructBitField8", "vfrFormSetDefinition", "classguidDefinition",
                   "classDefinition", "validClassNames", "subclassDefinition",
                   "vfrFormSetList", "vfrFormSet", "vfrStatementDefaultStore",
                   "vfrStatementVarStoreLinear", "vfrStatementVarStoreEfi",
                   "vfrVarStoreEfiAttr", "vfrStatementVarStoreNameValue",
                   "vfrStatementDisableIfFormSet", "vfrStatementSuppressIfFormSet",
                   "guidSubDefinition", "guidDefinition", "getStringId",
                   "vfrQuestionHeader", "vfrQuestionBaseInfo", "vfrStatementHeader",
                   "questionheaderFlagsField", "vfrStorageVarId", "vfrConstantValueField",
                   "vfrImageTag", "vfrLockedTag", "vfrStatementStatTag",
                   "vfrStatementStatTagList", "vfrFormDefinition", "vfrForm",
                   "vfrFormMapDefinition", "vfrStatementImage", "vfrStatementLocked",
                   "vfrStatementRules", "vfrStatementStat", "vfrStatementSubTitle",
                   "vfrStatementSubTitleComponent", "vfrSubtitleFlags",
                   "subtitleFlagsField", "vfrStatementStaticText", "staticTextFlagsField",
                   "vfrStatementCrossReference", "vfrStatementGoto", "vfrGotoFlags",
                   "gotoFlagsField", "vfrStatementResetButton", "vfrStatementQuestions",
                   "vfrStatementQuestionTag", "vfrStatementInconsistentIf",
                   "vfrStatementNoSubmitIf", "vfrStatementDisableIfQuest",
                   "vfrStatementRefresh", "vfrStatementVarstoreDevice",
                   "vfrStatementRefreshEvent", "vfrStatementWarningIf",
                   "vfrStatementQuestionTagList", "vfrStatementQuestionOptionTag",
                   "flagsField", "vfrStatementSuppressIfQuest", "vfrStatementGrayOutIfQuest",
                   "vfrStatementDefault", "vfrStatementValue", "vfrStatementOptions",
                   "vfrStatementOneOfOption", "vfrOneOfOptionFlags", "oneofoptionFlagsField",
                   "vfrStatementRead", "vfrStatementWrite", "vfrStatementQuestionOptionList",
                   "vfrStatementQuestionOption", "vfrStatementBooleanType",
                   "vfrStatementCheckBox", "vfrCheckBoxFlags", "checkboxFlagsField",
                   "vfrStatementAction", "vfrActionFlags", "actionFlagsField",
                   "vfrStatementNumericType", "vfrStatementNumeric", "vfrSetMinMaxStep",
                   "vfrNumericFlags", "numericFlagsField", "vfrStatementOneOf",
                   "vfrOneofFlagsField", "vfrStatementStringType", "vfrStatementString",
                   "vfrStringFlagsField", "stringFlagsField", "vfrStatementPassword",
                   "vfrPasswordFlagsField", "passwordFlagsField", "vfrStatementOrderedList",
                   "vfrOrderedListFlags", "orderedlistFlagsField", "vfrStatementDate",
                   "minMaxDateStepDefault", "vfrDateFlags", "dateFlagsField",
                   "vfrStatementTime", "minMaxTimeStepDefault", "vfrTimeFlags",
                   "timeFlagsField", "vfrStatementConditional", "vfrStatementConditionalNew",
                   "vfrStatementSuppressIfStat", "vfrStatementGrayOutIfStat",
                   "vfrStatementStatList", "vfrStatementStatListOld", "vfrStatementDisableIfStat",
                   "vfrStatementgrayoutIfSuppressIf", "vfrStatementsuppressIfGrayOutIf",
                   "vfrStatementSuppressIfStatNew", "vfrStatementGrayOutIfStatNew",
                   "vfrStatementInconsistentIfStat", "vfrStatementInvalid",
                   "vfrStatementInvalidHidden", "vfrStatementInvalidInventory",
                   "vfrStatementInvalidSaveRestoreDefaults", "vfrStatementLabel",
                   "vfrStatementBanner", "vfrStatementExtension", "vfrExtensionData",
                   "vfrStatementModal", "vfrModalTag", "vfrStatementExpression",
                   "vfrStatementExpressionSub", "andTerm", "bitwiseorTerm",
                   "bitwiseandTerm", "equalTerm", "equalTermSupplementary",
                   "compareTerm", "compareTermSupplementary", "shiftTerm",
                   "shiftTermSupplementary", "addMinusTerm", "addMinusTermSupplementary",
                   "multdivmodTerm", "multdivmodTermSupplementary", "castTerm",
                   "castTermSub", "atomTerm", "vfrExpressionCatenate", "vfrExpressionMatch",
                   "vfrExpressionMatch2", "vfrExpressionParen", "vfrExpressionBuildInFunction",
                   "dupExp", "vareqvalExp", "ideqvalExp", "ideqidExp", "ideqvallistExp",
                   "vfrQuestionDataFieldName", "arrayName", "questionref1Exp",
                   "rulerefExp", "stringref1Exp", "pushthisExp", "securityExp",
                   "numericVarStoreType", "getExp", "vfrExpressionConstant",
                   "vfrExpressionUnaryOp", "lengthExp", "bitwisenotExp",
                   "question23refExp", "stringref2Exp", "toboolExp", "tostringExp",
                   "unintExp", "toupperExp", "tolwerExp", "setExp", "vfrExpressionTernaryOp",
                   "conditionalExp", "findExp", "findFormat", "midExp",
                   "tokenExp", "spanExp", "spanFlags", "vfrExpressionMap" ]

    EOF = Token.EOF
    T__0=1
    T__1=2
    T__2=3
    T__3=4
    T__4=5
    T__5=6
    T__6=7
    T__7=8
    T__8=9
    T__9=10
    T__10=11
    T__11=12
    T__12=13
    T__13=14
    T__14=15
    T__15=16
    Define=17
    Include=18
    FormPkgType=19
    OpenBrace=20
    CloseBrace=21
    OpenParen=22
    CloseParen=23
    OpenBracket=24
    CloseBracket=25
    Dot=26
    Negative=27
    Colon=28
    Slash=29
    Semicolon=30
    Comma=31
    Equal=32
    NotEqual=33
    LessEqual=34
    Less=35
    GreaterEqual=36
    Greater=37
    BitWiseOr=38
    BitWiseAnd=39
    DevicePath=40
    FormSet=41
    FormSetId=42
    EndFormSet=43
    Title=44
    FormId=45
    OneOf=46
    EndOneOf=47
    Prompt=48
    OrderedList=49
    MaxContainers=50
    EndList=51
    EndForm=52
    Form=53
    FormMap=54
    MapTitle=55
    MapGuid=56
    Subtitle=57
    EndSubtitle=58
    Help=59
    Text=60
    Option=61
    FLAGS=62
    Date=63
    EndDate=64
    Year=65
    Month=66
    Day=67
    Time=68
    EndTime=69
    Hour=70
    Minute=71
    Second=72
    GrayOutIf=73
    Label=74
    Timeout=75
    Inventory=76
    NonNvDataMap=77
    Struct=78
    Union=79
    Boolean=80
    Uint64=81
    Uint32=82
    Uint16=83
    Uint8=84
    EFI_STRING_ID=85
    EFI_HII_DATE=86
    EFI_HII_TIME=87
    EFI_HII_REF=88
    Uuid=89
    CheckBox=90
    EndCheckBox=91
    Numeric=92
    EndNumeric=93
    Minimum=94
    Maximum=95
    Step=96
    Default=97
    Password=98
    EndPassword=99
    String=100
    EndString=101
    MinSize=102
    MaxSize=103
    Encoding=104
    SuppressIf=105
    DisableIf=106
    Hidden=107
    Goto=108
    FormSetGuid=109
    InconsistentIf=110
    WarningIf=111
    NoSubmitIf=112
    EndIf=113
    Key=114
    DefaultFlag=115
    ManufacturingFlag=116
    CheckBoxDefaultFlag=117
    CheckBoxDefaultMfgFlag=118
    InteractiveFlag=119
    NVAccessFlag=120
    ResetRequiredFlag=121
    ReconnectRequiredFlag=122
    LateCheckFlag=123
    ReadOnlyFlag=124
    OptionOnlyFlag=125
    RestStyleFlag=126
    Class=127
    Subclass=128
    ClassGuid=129
    TypeDef=130
    Restore=131
    Save=132
    Defaults=133
    Banner=134
    Align=135
    Left=136
    Right=137
    Center=138
    Line=139
    Name=140
    VarId=141
    Question=142
    QuestionId=143
    Image=144
    Locked=145
    Rule=146
    EndRule=147
    Value=148
    Read=149
    Write=150
    ResetButton=151
    EndResetButton=152
    DefaultStore=153
    Attribute=154
    Varstore=155
    Efivarstore=156
    VarSize=157
    NameValueVarStore=158
    Action=159
    Config=160
    EndAction=161
    Refresh=162
    Interval=163
    VarstoreDevice=164
    GuidOp=165
    EndGuidOp=166
    DataType=167
    Data=168
    Modal=169
    ClassNonDevice=170
    ClassDiskDevice=171
    ClassVideoDevice=172
    ClassNetworkDevice=173
    ClassInputDevice=174
    ClassOnBoardDevice=175
    ClassOtherDevice=176
    SubclassSetupApplication=177
    SubclassGeneralApplication=178
    SubclassFrontPage=179
    SubclassSingleUse=180
    YearSupppressFlag=181
    MonthSuppressFlag=182
    DaySuppressFlag=183
    HourSupppressFlag=184
    MinuteSuppressFlag=185
    SecondSuppressFlag=186
    StorageNormalFlag=187
    StorageTimeFlag=188
    StorageWakeUpFlag=189
    UniQueFlag=190
    NoEmptyFlag=191
    Cond=192
    Find=193
    Mid=194
    Tok=195
    Span=196
    Dup=197
    VarEqVal=198
    Var=199
    IdEqVal=200
    IdEqId=201
    IdEqValList=202
    QuestionRef=203
    RuleRef=204
    StringRef=205
    PushThis=206
    Security=207
    Get=208
    TrueSymbol=209
    FalseSymbol=210
    One=211
    Ones=212
    Zero=213
    Undefined=214
    Version=215
    Length=216
    AND=217
    OR=218
    NOT=219
    Set=220
    BitWiseNot=221
    BoolVal=222
    StringVal=223
    UnIntVal=224
    ToUpper=225
    ToLower=226
    Match=227
    Match2=228
    Catenate=229
    QuestionRefVal=230
    StringRefVal=231
    Map=232
    RefreshGuid=233
    StringToken=234
    OptionDefault=235
    OptionDefaultMfg=236
    NumericSizeOne=237
    NumericSizeTwo=238
    NumericSizeFour=239
    NumericSizeEight=240
    DisPlayIntDec=241
    DisPlayUIntDec=242
    DisPlayUIntHex=243
    Insensitive=244
    Sensitive=245
    LastNonMatch=246
    FirstNonMatch=247
    Number=248
    StringIdentifier=249
    ComplexDefine=250
    LineDefinition=251
    IncludeDefinition=252
    Whitespace=253
    Newline=254
    LineComment=255
    Extern=256

    def __init__(self, input:TokenStream, output:TextIO = sys.stdout):
        super().__init__(input, output)
        self.checkVersion("4.7.2")
        self._interp = ParserATNSimulator(self, self.atn, self.decisionsToDFA, self.sharedContextCache)
        self._predicates = None




    class VfrProgramContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def vfrPragmaPackDefinition(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrPragmaPackDefinitionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrPragmaPackDefinitionContext,i)


        def vfrDataStructDefinition(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrDataStructDefinitionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrDataStructDefinitionContext,i)


        def vfrDataUnionDefinition(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrDataUnionDefinitionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrDataUnionDefinitionContext,i)


        def vfrFormSetDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrFormSetDefinitionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrProgram

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrProgram" ):
                return visitor.visitVfrProgram(self)
            else:
                return visitor.visitChildren(self)




    def vfrProgram(self):

        localctx = VfrSyntaxParser.VfrProgramContext(self, self._ctx, self.state)
        self.enterRule(localctx, 0, self.RULE_vfrProgram)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 411
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.T__3 or ((((_la - 78)) & ~0x3f) == 0 and ((1 << (_la - 78)) & ((1 << (VfrSyntaxParser.Struct - 78)) | (1 << (VfrSyntaxParser.Union - 78)) | (1 << (VfrSyntaxParser.TypeDef - 78)))) != 0):
                self.state = 409
                self._errHandler.sync(self)
                la_ = self._interp.adaptivePredict(self._input,0,self._ctx)
                if la_ == 1:
                    self.state = 406
                    self.vfrPragmaPackDefinition()
                    pass

                elif la_ == 2:
                    self.state = 407
                    self.vfrDataStructDefinition()
                    pass

                elif la_ == 3:
                    self.state = 408
                    self.vfrDataUnionDefinition()
                    pass


                self.state = 413
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 415
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FormSet:
                self.state = 414
                self.vfrFormSetDefinition()


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class PragmaPackShowDefContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_pragmaPackShowDef

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitPragmaPackShowDef" ):
                return visitor.visitPragmaPackShowDef(self)
            else:
                return visitor.visitChildren(self)




    def pragmaPackShowDef(self):

        localctx = VfrSyntaxParser.PragmaPackShowDefContext(self, self._ctx, self.state)
        self.enterRule(localctx, 2, self.RULE_pragmaPackShowDef)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 417
            self.match(VfrSyntaxParser.T__0)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class PragmaPackStackDefContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_pragmaPackStackDef

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitPragmaPackStackDef" ):
                return visitor.visitPragmaPackStackDef(self)
            else:
                return visitor.visitChildren(self)




    def pragmaPackStackDef(self):

        localctx = VfrSyntaxParser.PragmaPackStackDefContext(self, self._ctx, self.state)
        self.enterRule(localctx, 4, self.RULE_pragmaPackStackDef)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 419
            _la = self._input.LA(1)
            if not(_la==VfrSyntaxParser.T__1 or _la==VfrSyntaxParser.T__2):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
            self.state = 422
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,3,self._ctx)
            if la_ == 1:
                self.state = 420
                self.match(VfrSyntaxParser.Comma)
                self.state = 421
                self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 426
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 424
                self.match(VfrSyntaxParser.Comma)
                self.state = 425
                self.match(VfrSyntaxParser.Number)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class PragmaPackNumberContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_pragmaPackNumber

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitPragmaPackNumber" ):
                return visitor.visitPragmaPackNumber(self)
            else:
                return visitor.visitChildren(self)




    def pragmaPackNumber(self):

        localctx = VfrSyntaxParser.PragmaPackNumberContext(self, self._ctx, self.state)
        self.enterRule(localctx, 6, self.RULE_pragmaPackNumber)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 429
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Number:
                self.state = 428
                self.match(VfrSyntaxParser.Number)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrPragmaPackDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def pragmaPackShowDef(self):
            return self.getTypedRuleContext(VfrSyntaxParser.PragmaPackShowDefContext,0)


        def pragmaPackStackDef(self):
            return self.getTypedRuleContext(VfrSyntaxParser.PragmaPackStackDefContext,0)


        def pragmaPackNumber(self):
            return self.getTypedRuleContext(VfrSyntaxParser.PragmaPackNumberContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrPragmaPackDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrPragmaPackDefinition" ):
                return visitor.visitVfrPragmaPackDefinition(self)
            else:
                return visitor.visitChildren(self)




    def vfrPragmaPackDefinition(self):

        localctx = VfrSyntaxParser.VfrPragmaPackDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 8, self.RULE_vfrPragmaPackDefinition)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 431
            self.match(VfrSyntaxParser.T__3)
            self.state = 432
            self.match(VfrSyntaxParser.T__4)
            self.state = 433
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 437
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,6,self._ctx)
            if la_ == 1:
                self.state = 434
                self.pragmaPackShowDef()

            elif la_ == 2:
                self.state = 435
                self.pragmaPackStackDef()

            elif la_ == 3:
                self.state = 436
                self.pragmaPackNumber()


            self.state = 439
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrDataStructDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.N1 = None # Token
            self.N2 = None # Token

        def Struct(self):
            return self.getToken(VfrSyntaxParser.Struct, 0)

        def OpenBrace(self):
            return self.getToken(VfrSyntaxParser.OpenBrace, 0)

        def vfrDataStructFields(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrDataStructFieldsContext,0)


        def CloseBrace(self):
            return self.getToken(VfrSyntaxParser.CloseBrace, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def TypeDef(self):
            return self.getToken(VfrSyntaxParser.TypeDef, 0)

        def NonNvDataMap(self):
            return self.getToken(VfrSyntaxParser.NonNvDataMap, 0)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrDataStructDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrDataStructDefinition" ):
                return visitor.visitVfrDataStructDefinition(self)
            else:
                return visitor.visitChildren(self)




    def vfrDataStructDefinition(self):

        localctx = VfrSyntaxParser.VfrDataStructDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 10, self.RULE_vfrDataStructDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 442
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.TypeDef:
                self.state = 441
                self.match(VfrSyntaxParser.TypeDef)


            self.state = 444
            self.match(VfrSyntaxParser.Struct)
            self.state = 446
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.NonNvDataMap:
                self.state = 445
                self.match(VfrSyntaxParser.NonNvDataMap)


            self.state = 449
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 448
                localctx.N1 = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 451
            self.match(VfrSyntaxParser.OpenBrace)
            self.state = 452
            self.vfrDataStructFields(False)
            self.state = 453
            self.match(VfrSyntaxParser.CloseBrace)
            self.state = 455
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 454
                localctx.N2 = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 457
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrDataUnionDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.N1 = None # Token
            self.N2 = None # Token

        def Union(self):
            return self.getToken(VfrSyntaxParser.Union, 0)

        def OpenBrace(self):
            return self.getToken(VfrSyntaxParser.OpenBrace, 0)

        def vfrDataStructFields(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrDataStructFieldsContext,0)


        def CloseBrace(self):
            return self.getToken(VfrSyntaxParser.CloseBrace, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def TypeDef(self):
            return self.getToken(VfrSyntaxParser.TypeDef, 0)

        def NonNvDataMap(self):
            return self.getToken(VfrSyntaxParser.NonNvDataMap, 0)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrDataUnionDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrDataUnionDefinition" ):
                return visitor.visitVfrDataUnionDefinition(self)
            else:
                return visitor.visitChildren(self)




    def vfrDataUnionDefinition(self):

        localctx = VfrSyntaxParser.VfrDataUnionDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 12, self.RULE_vfrDataUnionDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 460
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.TypeDef:
                self.state = 459
                self.match(VfrSyntaxParser.TypeDef)


            self.state = 462
            self.match(VfrSyntaxParser.Union)
            self.state = 464
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.NonNvDataMap:
                self.state = 463
                self.match(VfrSyntaxParser.NonNvDataMap)


            self.state = 467
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 466
                localctx.N1 = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 469
            self.match(VfrSyntaxParser.OpenBrace)
            self.state = 470
            self.vfrDataStructFields(True)
            self.state = 471
            self.match(VfrSyntaxParser.CloseBrace)
            self.state = 473
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 472
                localctx.N2 = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 475
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrDataStructFieldsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.FieldInUnion = FieldInUnion

        def dataStructField64(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructField64Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructField64Context,i)


        def dataStructField32(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructField32Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructField32Context,i)


        def dataStructField16(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructField16Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructField16Context,i)


        def dataStructField8(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructField8Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructField8Context,i)


        def dataStructFieldBool(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructFieldBoolContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructFieldBoolContext,i)


        def dataStructFieldString(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructFieldStringContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructFieldStringContext,i)


        def dataStructFieldDate(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructFieldDateContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructFieldDateContext,i)


        def dataStructFieldTime(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructFieldTimeContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructFieldTimeContext,i)


        def dataStructFieldRef(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructFieldRefContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructFieldRefContext,i)


        def dataStructFieldUser(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructFieldUserContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructFieldUserContext,i)


        def dataStructBitField64(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructBitField64Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructBitField64Context,i)


        def dataStructBitField32(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructBitField32Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructBitField32Context,i)


        def dataStructBitField16(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructBitField16Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructBitField16Context,i)


        def dataStructBitField8(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DataStructBitField8Context)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DataStructBitField8Context,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrDataStructFields

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrDataStructFields" ):
                return visitor.visitVfrDataStructFields(self)
            else:
                return visitor.visitChildren(self)




    def vfrDataStructFields(self, FieldInUnion):

        localctx = VfrSyntaxParser.VfrDataStructFieldsContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 14, self.RULE_vfrDataStructFields)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 493
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 80)) & ~0x3f) == 0 and ((1 << (_la - 80)) & ((1 << (VfrSyntaxParser.Boolean - 80)) | (1 << (VfrSyntaxParser.Uint64 - 80)) | (1 << (VfrSyntaxParser.Uint32 - 80)) | (1 << (VfrSyntaxParser.Uint16 - 80)) | (1 << (VfrSyntaxParser.Uint8 - 80)) | (1 << (VfrSyntaxParser.EFI_STRING_ID - 80)) | (1 << (VfrSyntaxParser.EFI_HII_DATE - 80)) | (1 << (VfrSyntaxParser.EFI_HII_TIME - 80)) | (1 << (VfrSyntaxParser.EFI_HII_REF - 80)))) != 0) or _la==VfrSyntaxParser.StringIdentifier:
                self.state = 491
                self._errHandler.sync(self)
                la_ = self._interp.adaptivePredict(self._input,15,self._ctx)
                if la_ == 1:
                    self.state = 477
                    self.dataStructField64(FieldInUnion)
                    pass

                elif la_ == 2:
                    self.state = 478
                    self.dataStructField32(FieldInUnion)
                    pass

                elif la_ == 3:
                    self.state = 479
                    self.dataStructField16(FieldInUnion)
                    pass

                elif la_ == 4:
                    self.state = 480
                    self.dataStructField8(FieldInUnion)
                    pass

                elif la_ == 5:
                    self.state = 481
                    self.dataStructFieldBool(FieldInUnion)
                    pass

                elif la_ == 6:
                    self.state = 482
                    self.dataStructFieldString(FieldInUnion)
                    pass

                elif la_ == 7:
                    self.state = 483
                    self.dataStructFieldDate(FieldInUnion)
                    pass

                elif la_ == 8:
                    self.state = 484
                    self.dataStructFieldTime(FieldInUnion)
                    pass

                elif la_ == 9:
                    self.state = 485
                    self.dataStructFieldRef(FieldInUnion)
                    pass

                elif la_ == 10:
                    self.state = 486
                    self.dataStructFieldUser(FieldInUnion)
                    pass

                elif la_ == 11:
                    self.state = 487
                    self.dataStructBitField64(FieldInUnion)
                    pass

                elif la_ == 12:
                    self.state = 488
                    self.dataStructBitField32(FieldInUnion)
                    pass

                elif la_ == 13:
                    self.state = 489
                    self.dataStructBitField16(FieldInUnion)
                    pass

                elif la_ == 14:
                    self.state = 490
                    self.dataStructBitField8(FieldInUnion)
                    pass


                self.state = 495
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructField64Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Uint64(self):
            return self.getToken(VfrSyntaxParser.Uint64, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructField64

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructField64" ):
                return visitor.visitDataStructField64(self)
            else:
                return visitor.visitChildren(self)




    def dataStructField64(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructField64Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 16, self.RULE_dataStructField64)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 496
            self.match(VfrSyntaxParser.Uint64)
            self.state = 497
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 501
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 498
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 499
                self.match(VfrSyntaxParser.Number)
                self.state = 500
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 503
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructField32Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Uint32(self):
            return self.getToken(VfrSyntaxParser.Uint32, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructField32

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructField32" ):
                return visitor.visitDataStructField32(self)
            else:
                return visitor.visitChildren(self)




    def dataStructField32(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructField32Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 18, self.RULE_dataStructField32)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 505
            self.match(VfrSyntaxParser.Uint32)
            self.state = 506
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 510
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 507
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 508
                self.match(VfrSyntaxParser.Number)
                self.state = 509
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 512
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructField16Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Uint16(self):
            return self.getToken(VfrSyntaxParser.Uint16, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructField16

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructField16" ):
                return visitor.visitDataStructField16(self)
            else:
                return visitor.visitChildren(self)




    def dataStructField16(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructField16Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 20, self.RULE_dataStructField16)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 514
            self.match(VfrSyntaxParser.Uint16)
            self.state = 515
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 519
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 516
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 517
                self.match(VfrSyntaxParser.Number)
                self.state = 518
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 521
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructField8Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Uint8(self):
            return self.getToken(VfrSyntaxParser.Uint8, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructField8

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructField8" ):
                return visitor.visitDataStructField8(self)
            else:
                return visitor.visitChildren(self)




    def dataStructField8(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructField8Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 22, self.RULE_dataStructField8)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 523
            self.match(VfrSyntaxParser.Uint8)
            self.state = 524
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 528
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 525
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 526
                self.match(VfrSyntaxParser.Number)
                self.state = 527
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 530
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructFieldBoolContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Boolean(self):
            return self.getToken(VfrSyntaxParser.Boolean, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructFieldBool

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructFieldBool" ):
                return visitor.visitDataStructFieldBool(self)
            else:
                return visitor.visitChildren(self)




    def dataStructFieldBool(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructFieldBoolContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 24, self.RULE_dataStructFieldBool)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 532
            self.match(VfrSyntaxParser.Boolean)
            self.state = 533
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 537
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 534
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 535
                self.match(VfrSyntaxParser.Number)
                self.state = 536
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 539
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructFieldStringContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def EFI_STRING_ID(self):
            return self.getToken(VfrSyntaxParser.EFI_STRING_ID, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructFieldString

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructFieldString" ):
                return visitor.visitDataStructFieldString(self)
            else:
                return visitor.visitChildren(self)




    def dataStructFieldString(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructFieldStringContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 26, self.RULE_dataStructFieldString)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 541
            self.match(VfrSyntaxParser.EFI_STRING_ID)
            self.state = 542
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 546
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 543
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 544
                self.match(VfrSyntaxParser.Number)
                self.state = 545
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 548
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructFieldDateContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def EFI_HII_DATE(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_DATE, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructFieldDate

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructFieldDate" ):
                return visitor.visitDataStructFieldDate(self)
            else:
                return visitor.visitChildren(self)




    def dataStructFieldDate(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructFieldDateContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 28, self.RULE_dataStructFieldDate)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 550
            self.match(VfrSyntaxParser.EFI_HII_DATE)
            self.state = 551
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 555
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 552
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 553
                self.match(VfrSyntaxParser.Number)
                self.state = 554
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 557
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructFieldTimeContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def EFI_HII_TIME(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_TIME, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructFieldTime

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructFieldTime" ):
                return visitor.visitDataStructFieldTime(self)
            else:
                return visitor.visitChildren(self)




    def dataStructFieldTime(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructFieldTimeContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 30, self.RULE_dataStructFieldTime)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 559
            self.match(VfrSyntaxParser.EFI_HII_TIME)
            self.state = 560
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 564
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 561
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 562
                self.match(VfrSyntaxParser.Number)
                self.state = 563
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 566
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructFieldRefContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def EFI_HII_REF(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_REF, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructFieldRef

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructFieldRef" ):
                return visitor.visitDataStructFieldRef(self)
            else:
                return visitor.visitChildren(self)




    def dataStructFieldRef(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructFieldRefContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 32, self.RULE_dataStructFieldRef)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 568
            self.match(VfrSyntaxParser.EFI_HII_REF)
            self.state = 569
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 573
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 570
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 571
                self.match(VfrSyntaxParser.Number)
                self.state = 572
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 575
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructFieldUserContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.T = None # Token
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructFieldUser

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructFieldUser" ):
                return visitor.visitDataStructFieldUser(self)
            else:
                return visitor.visitChildren(self)




    def dataStructFieldUser(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructFieldUserContext(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 34, self.RULE_dataStructFieldUser)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 577
            localctx.T = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 578
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 582
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 579
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 580
                self.match(VfrSyntaxParser.Number)
                self.state = 581
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 584
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructBitField64Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.D = None # Token
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Colon(self):
            return self.getToken(VfrSyntaxParser.Colon, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Uint64(self):
            return self.getToken(VfrSyntaxParser.Uint64, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructBitField64

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructBitField64" ):
                return visitor.visitDataStructBitField64(self)
            else:
                return visitor.visitChildren(self)




    def dataStructBitField64(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructBitField64Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 36, self.RULE_dataStructBitField64)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 586
            localctx.D = self.match(VfrSyntaxParser.Uint64)
            self.state = 588
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 587
                localctx.N = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 590
            self.match(VfrSyntaxParser.Colon)
            self.state = 591
            self.match(VfrSyntaxParser.Number)
            self.state = 592
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructBitField32Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.D = None # Token
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Colon(self):
            return self.getToken(VfrSyntaxParser.Colon, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Uint32(self):
            return self.getToken(VfrSyntaxParser.Uint32, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructBitField32

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructBitField32" ):
                return visitor.visitDataStructBitField32(self)
            else:
                return visitor.visitChildren(self)




    def dataStructBitField32(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructBitField32Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 38, self.RULE_dataStructBitField32)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 594
            localctx.D = self.match(VfrSyntaxParser.Uint32)
            self.state = 596
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 595
                localctx.N = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 598
            self.match(VfrSyntaxParser.Colon)
            self.state = 599
            self.match(VfrSyntaxParser.Number)
            self.state = 600
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructBitField16Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.D = None # Token
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Colon(self):
            return self.getToken(VfrSyntaxParser.Colon, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Uint16(self):
            return self.getToken(VfrSyntaxParser.Uint16, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructBitField16

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructBitField16" ):
                return visitor.visitDataStructBitField16(self)
            else:
                return visitor.visitChildren(self)




    def dataStructBitField16(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructBitField16Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 40, self.RULE_dataStructBitField16)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 602
            localctx.D = self.match(VfrSyntaxParser.Uint16)
            self.state = 604
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 603
                localctx.N = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 606
            self.match(VfrSyntaxParser.Colon)
            self.state = 607
            self.match(VfrSyntaxParser.Number)
            self.state = 608
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DataStructBitField8Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, FieldInUnion=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.FieldInUnion = None
            self.D = None # Token
            self.N = None # Token
            self.FieldInUnion = FieldInUnion

        def Colon(self):
            return self.getToken(VfrSyntaxParser.Colon, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Uint8(self):
            return self.getToken(VfrSyntaxParser.Uint8, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dataStructBitField8

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDataStructBitField8" ):
                return visitor.visitDataStructBitField8(self)
            else:
                return visitor.visitChildren(self)




    def dataStructBitField8(self, FieldInUnion):

        localctx = VfrSyntaxParser.DataStructBitField8Context(self, self._ctx, self.state, FieldInUnion)
        self.enterRule(localctx, 42, self.RULE_dataStructBitField8)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 610
            localctx.D = self.match(VfrSyntaxParser.Uint8)
            self.state = 612
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.StringIdentifier:
                self.state = 611
                localctx.N = self.match(VfrSyntaxParser.StringIdentifier)


            self.state = 614
            self.match(VfrSyntaxParser.Colon)
            self.state = 615
            self.match(VfrSyntaxParser.Number)
            self.state = 616
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrFormSetDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_FORM_SET_OP)

        def FormSet(self):
            return self.getToken(VfrSyntaxParser.FormSet, 0)

        def Uuid(self):
            return self.getToken(VfrSyntaxParser.Uuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Title(self):
            return self.getToken(VfrSyntaxParser.Title, 0)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Help(self):
            return self.getToken(VfrSyntaxParser.Help, 0)

        def vfrFormSetList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrFormSetListContext,0)


        def EndFormSet(self):
            return self.getToken(VfrSyntaxParser.EndFormSet, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def ClassGuid(self):
            return self.getToken(VfrSyntaxParser.ClassGuid, 0)

        def classguidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ClassguidDefinitionContext,0)


        def Class(self):
            return self.getToken(VfrSyntaxParser.Class, 0)

        def classDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ClassDefinitionContext,0)


        def Subclass(self):
            return self.getToken(VfrSyntaxParser.Subclass, 0)

        def subclassDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.SubclassDefinitionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrFormSetDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrFormSetDefinition" ):
                return visitor.visitVfrFormSetDefinition(self)
            else:
                return visitor.visitChildren(self)




    def vfrFormSetDefinition(self):

        localctx = VfrSyntaxParser.VfrFormSetDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 44, self.RULE_vfrFormSetDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 618
            self.match(VfrSyntaxParser.FormSet)
            self.state = 619
            self.match(VfrSyntaxParser.Uuid)
            self.state = 620
            self.match(VfrSyntaxParser.T__5)
            self.state = 621
            self.guidDefinition()
            self.state = 622
            self.match(VfrSyntaxParser.Comma)
            self.state = 623
            self.match(VfrSyntaxParser.Title)
            self.state = 624
            self.match(VfrSyntaxParser.T__5)
            self.state = 625
            self.match(VfrSyntaxParser.StringToken)
            self.state = 626
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 627
            self.match(VfrSyntaxParser.Number)
            self.state = 628
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 629
            self.match(VfrSyntaxParser.Comma)
            self.state = 630
            self.match(VfrSyntaxParser.Help)
            self.state = 631
            self.match(VfrSyntaxParser.T__5)
            self.state = 632
            self.match(VfrSyntaxParser.StringToken)
            self.state = 633
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 634
            self.match(VfrSyntaxParser.Number)
            self.state = 635
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 636
            self.match(VfrSyntaxParser.Comma)
            self.state = 642
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.ClassGuid:
                self.state = 637
                self.match(VfrSyntaxParser.ClassGuid)
                self.state = 638
                self.match(VfrSyntaxParser.T__5)
                self.state = 639
                self.classguidDefinition(localctx.Node)
                self.state = 640
                self.match(VfrSyntaxParser.Comma)


            self.state = 649
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Class:
                self.state = 644
                self.match(VfrSyntaxParser.Class)
                self.state = 645
                self.match(VfrSyntaxParser.T__5)
                self.state = 646
                self.classDefinition()
                self.state = 647
                self.match(VfrSyntaxParser.Comma)


            self.state = 656
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Subclass:
                self.state = 651
                self.match(VfrSyntaxParser.Subclass)
                self.state = 652
                self.match(VfrSyntaxParser.T__5)
                self.state = 653
                self.subclassDefinition()
                self.state = 654
                self.match(VfrSyntaxParser.Comma)


            self.state = 658
            self.vfrFormSetList(localctx.Node)
            self.state = 659
            self.match(VfrSyntaxParser.EndFormSet)
            self.state = 660
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ClassguidDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.GuidList = []
            self.Node = Node

        def guidDefinition(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.GuidDefinitionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_classguidDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitClassguidDefinition" ):
                return visitor.visitClassguidDefinition(self)
            else:
                return visitor.visitChildren(self)




    def classguidDefinition(self, Node):

        localctx = VfrSyntaxParser.ClassguidDefinitionContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 46, self.RULE_classguidDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 662
            self.guidDefinition()
            self.state = 665
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,34,self._ctx)
            if la_ == 1:
                self.state = 663
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 664
                self.guidDefinition()


            self.state = 669
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,35,self._ctx)
            if la_ == 1:
                self.state = 667
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 668
                self.guidDefinition()


            self.state = 673
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.BitWiseOr:
                self.state = 671
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 672
                self.guidDefinition()


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ClassDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_GUID_OP)

        def validClassNames(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.ValidClassNamesContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.ValidClassNamesContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_classDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitClassDefinition" ):
                return visitor.visitClassDefinition(self)
            else:
                return visitor.visitChildren(self)




    def classDefinition(self):

        localctx = VfrSyntaxParser.ClassDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 48, self.RULE_classDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 675
            self.validClassNames()
            self.state = 680
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 676
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 677
                self.validClassNames()
                self.state = 682
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ValidClassNamesContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ClassName = 0

        def ClassNonDevice(self):
            return self.getToken(VfrSyntaxParser.ClassNonDevice, 0)

        def ClassDiskDevice(self):
            return self.getToken(VfrSyntaxParser.ClassDiskDevice, 0)

        def ClassVideoDevice(self):
            return self.getToken(VfrSyntaxParser.ClassVideoDevice, 0)

        def ClassNetworkDevice(self):
            return self.getToken(VfrSyntaxParser.ClassNetworkDevice, 0)

        def ClassInputDevice(self):
            return self.getToken(VfrSyntaxParser.ClassInputDevice, 0)

        def ClassOnBoardDevice(self):
            return self.getToken(VfrSyntaxParser.ClassOnBoardDevice, 0)

        def ClassOtherDevice(self):
            return self.getToken(VfrSyntaxParser.ClassOtherDevice, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_validClassNames

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitValidClassNames" ):
                return visitor.visitValidClassNames(self)
            else:
                return visitor.visitChildren(self)




    def validClassNames(self):

        localctx = VfrSyntaxParser.ValidClassNamesContext(self, self._ctx, self.state)
        self.enterRule(localctx, 50, self.RULE_validClassNames)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 683
            _la = self._input.LA(1)
            if not(((((_la - 170)) & ~0x3f) == 0 and ((1 << (_la - 170)) & ((1 << (VfrSyntaxParser.ClassNonDevice - 170)) | (1 << (VfrSyntaxParser.ClassDiskDevice - 170)) | (1 << (VfrSyntaxParser.ClassVideoDevice - 170)) | (1 << (VfrSyntaxParser.ClassNetworkDevice - 170)) | (1 << (VfrSyntaxParser.ClassInputDevice - 170)) | (1 << (VfrSyntaxParser.ClassOnBoardDevice - 170)) | (1 << (VfrSyntaxParser.ClassOtherDevice - 170)))) != 0) or _la==VfrSyntaxParser.Number):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class SubclassDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_GUID_OP)

        def SubclassSetupApplication(self):
            return self.getToken(VfrSyntaxParser.SubclassSetupApplication, 0)

        def SubclassGeneralApplication(self):
            return self.getToken(VfrSyntaxParser.SubclassGeneralApplication, 0)

        def SubclassFrontPage(self):
            return self.getToken(VfrSyntaxParser.SubclassFrontPage, 0)

        def SubclassSingleUse(self):
            return self.getToken(VfrSyntaxParser.SubclassSingleUse, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_subclassDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitSubclassDefinition" ):
                return visitor.visitSubclassDefinition(self)
            else:
                return visitor.visitChildren(self)




    def subclassDefinition(self):

        localctx = VfrSyntaxParser.SubclassDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 52, self.RULE_subclassDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 685
            _la = self._input.LA(1)
            if not(((((_la - 177)) & ~0x3f) == 0 and ((1 << (_la - 177)) & ((1 << (VfrSyntaxParser.SubclassSetupApplication - 177)) | (1 << (VfrSyntaxParser.SubclassGeneralApplication - 177)) | (1 << (VfrSyntaxParser.SubclassFrontPage - 177)) | (1 << (VfrSyntaxParser.SubclassSingleUse - 177)))) != 0) or _la==VfrSyntaxParser.Number):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrFormSetListContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.Node = Node

        def vfrFormSet(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrFormSetContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrFormSetContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrFormSetList

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrFormSetList" ):
                return visitor.visitVfrFormSetList(self)
            else:
                return visitor.visitChildren(self)




    def vfrFormSetList(self, Node):

        localctx = VfrSyntaxParser.VfrFormSetListContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 54, self.RULE_vfrFormSetList)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 690
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.Form or _la==VfrSyntaxParser.FormMap or ((((_la - 105)) & ~0x3f) == 0 and ((1 << (_la - 105)) & ((1 << (VfrSyntaxParser.SuppressIf - 105)) | (1 << (VfrSyntaxParser.DisableIf - 105)) | (1 << (VfrSyntaxParser.Image - 105)) | (1 << (VfrSyntaxParser.DefaultStore - 105)) | (1 << (VfrSyntaxParser.Varstore - 105)) | (1 << (VfrSyntaxParser.Efivarstore - 105)) | (1 << (VfrSyntaxParser.NameValueVarStore - 105)) | (1 << (VfrSyntaxParser.GuidOp - 105)))) != 0):
                self.state = 687
                self.vfrFormSet()
                self.state = 692
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrFormSetContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrFormDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrFormDefinitionContext,0)


        def vfrFormMapDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrFormMapDefinitionContext,0)


        def vfrStatementImage(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementImageContext,0)


        def vfrStatementVarStoreLinear(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementVarStoreLinearContext,0)


        def vfrStatementVarStoreEfi(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementVarStoreEfiContext,0)


        def vfrStatementVarStoreNameValue(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementVarStoreNameValueContext,0)


        def vfrStatementDefaultStore(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDefaultStoreContext,0)


        def vfrStatementDisableIfFormSet(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDisableIfFormSetContext,0)


        def vfrStatementSuppressIfFormSet(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSuppressIfFormSetContext,0)


        def vfrStatementExtension(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExtensionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrFormSet

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrFormSet" ):
                return visitor.visitVfrFormSet(self)
            else:
                return visitor.visitChildren(self)




    def vfrFormSet(self):

        localctx = VfrSyntaxParser.VfrFormSetContext(self, self._ctx, self.state)
        self.enterRule(localctx, 56, self.RULE_vfrFormSet)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 703
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Form]:
                self.state = 693
                self.vfrFormDefinition()
                pass
            elif token in [VfrSyntaxParser.FormMap]:
                self.state = 694
                self.vfrFormMapDefinition()
                pass
            elif token in [VfrSyntaxParser.Image]:
                self.state = 695
                self.vfrStatementImage()
                pass
            elif token in [VfrSyntaxParser.Varstore]:
                self.state = 696
                self.vfrStatementVarStoreLinear()
                pass
            elif token in [VfrSyntaxParser.Efivarstore]:
                self.state = 697
                self.vfrStatementVarStoreEfi()
                pass
            elif token in [VfrSyntaxParser.NameValueVarStore]:
                self.state = 698
                self.vfrStatementVarStoreNameValue()
                pass
            elif token in [VfrSyntaxParser.DefaultStore]:
                self.state = 699
                self.vfrStatementDefaultStore()
                pass
            elif token in [VfrSyntaxParser.DisableIf]:
                self.state = 700
                self.vfrStatementDisableIfFormSet()
                pass
            elif token in [VfrSyntaxParser.SuppressIf]:
                self.state = 701
                self.vfrStatementSuppressIfFormSet()
                pass
            elif token in [VfrSyntaxParser.GuidOp]:
                self.state = 702
                self.vfrStatementExtension()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementDefaultStoreContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP)
            self.N = None # Token
            self.S = None # Token
            self.A = None # Token

        def DefaultStore(self):
            return self.getToken(VfrSyntaxParser.DefaultStore, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Attribute(self):
            return self.getToken(VfrSyntaxParser.Attribute, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementDefaultStore

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementDefaultStore" ):
                return visitor.visitVfrStatementDefaultStore(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementDefaultStore(self):

        localctx = VfrSyntaxParser.VfrStatementDefaultStoreContext(self, self._ctx, self.state)
        self.enterRule(localctx, 58, self.RULE_vfrStatementDefaultStore)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 705
            self.match(VfrSyntaxParser.DefaultStore)
            self.state = 706
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 707
            self.match(VfrSyntaxParser.Comma)
            self.state = 708
            self.match(VfrSyntaxParser.Prompt)
            self.state = 709
            self.match(VfrSyntaxParser.T__5)
            self.state = 710
            self.match(VfrSyntaxParser.StringToken)
            self.state = 711
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 712
            localctx.S = self.match(VfrSyntaxParser.Number)
            self.state = 713
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 718
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 714
                self.match(VfrSyntaxParser.Comma)
                self.state = 715
                self.match(VfrSyntaxParser.Attribute)
                self.state = 716
                self.match(VfrSyntaxParser.T__5)
                self.state = 717
                localctx.A = self.match(VfrSyntaxParser.Number)


            self.state = 720
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementVarStoreLinearContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_VARSTORE_OP)
            self.TN = None # Token
            self.ID = None # Token
            self.SN = None # Token

        def Varstore(self):
            return self.getToken(VfrSyntaxParser.Varstore, 0)

        def Name(self):
            return self.getToken(VfrSyntaxParser.Name, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Uuid(self):
            return self.getToken(VfrSyntaxParser.Uuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def Uint8(self):
            return self.getToken(VfrSyntaxParser.Uint8, 0)

        def Uint16(self):
            return self.getToken(VfrSyntaxParser.Uint16, 0)

        def Uint32(self):
            return self.getToken(VfrSyntaxParser.Uint32, 0)

        def Uint64(self):
            return self.getToken(VfrSyntaxParser.Uint64, 0)

        def EFI_HII_DATE(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_DATE, 0)

        def EFI_HII_TIME(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_TIME, 0)

        def EFI_HII_REF(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_REF, 0)

        def VarId(self):
            return self.getToken(VfrSyntaxParser.VarId, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementVarStoreLinear

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementVarStoreLinear" ):
                return visitor.visitVfrStatementVarStoreLinear(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementVarStoreLinear(self):

        localctx = VfrSyntaxParser.VfrStatementVarStoreLinearContext(self, self._ctx, self.state)
        self.enterRule(localctx, 60, self.RULE_vfrStatementVarStoreLinear)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 722
            self.match(VfrSyntaxParser.Varstore)
            self.state = 739
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.StringIdentifier]:
                self.state = 723
                localctx.TN = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 724
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint8]:
                self.state = 725
                self.match(VfrSyntaxParser.Uint8)
                self.state = 726
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint16]:
                self.state = 727
                self.match(VfrSyntaxParser.Uint16)
                self.state = 728
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint32]:
                self.state = 729
                self.match(VfrSyntaxParser.Uint32)
                self.state = 730
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint64]:
                self.state = 731
                self.match(VfrSyntaxParser.Uint64)
                self.state = 732
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.EFI_HII_DATE]:
                self.state = 733
                self.match(VfrSyntaxParser.EFI_HII_DATE)
                self.state = 734
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.EFI_HII_TIME]:
                self.state = 735
                self.match(VfrSyntaxParser.EFI_HII_TIME)
                self.state = 736
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.EFI_HII_REF]:
                self.state = 737
                self.match(VfrSyntaxParser.EFI_HII_REF)
                self.state = 738
                self.match(VfrSyntaxParser.Comma)
                pass
            else:
                raise NoViableAltException(self)

            self.state = 745
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.VarId:
                self.state = 741
                self.match(VfrSyntaxParser.VarId)
                self.state = 742
                self.match(VfrSyntaxParser.T__5)
                self.state = 743
                localctx.ID = self.match(VfrSyntaxParser.Number)
                self.state = 744
                self.match(VfrSyntaxParser.Comma)


            self.state = 747
            self.match(VfrSyntaxParser.Name)
            self.state = 748
            self.match(VfrSyntaxParser.T__5)
            self.state = 749
            localctx.SN = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 750
            self.match(VfrSyntaxParser.Comma)
            self.state = 751
            self.match(VfrSyntaxParser.Uuid)
            self.state = 752
            self.match(VfrSyntaxParser.T__5)
            self.state = 753
            self.guidDefinition()
            self.state = 754
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementVarStoreEfiContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_VARSTORE_EFI_OP)
            self.TN = None # Token
            self.ID = None # Token
            self.SN = None # Token
            self.VN = None # Token
            self.N = None # Token

        def Efivarstore(self):
            return self.getToken(VfrSyntaxParser.Efivarstore, 0)

        def Attribute(self):
            return self.getToken(VfrSyntaxParser.Attribute, 0)

        def vfrVarStoreEfiAttr(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrVarStoreEfiAttrContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrVarStoreEfiAttrContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Uuid(self):
            return self.getToken(VfrSyntaxParser.Uuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Uint8(self):
            return self.getToken(VfrSyntaxParser.Uint8, 0)

        def Uint16(self):
            return self.getToken(VfrSyntaxParser.Uint16, 0)

        def Uint32(self):
            return self.getToken(VfrSyntaxParser.Uint32, 0)

        def Uint64(self):
            return self.getToken(VfrSyntaxParser.Uint64, 0)

        def EFI_HII_DATE(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_DATE, 0)

        def EFI_HII_TIME(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_TIME, 0)

        def EFI_HII_REF(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_REF, 0)

        def Name(self):
            return self.getToken(VfrSyntaxParser.Name, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def VarSize(self):
            return self.getToken(VfrSyntaxParser.VarSize, 0)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def VarId(self):
            return self.getToken(VfrSyntaxParser.VarId, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementVarStoreEfi

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementVarStoreEfi" ):
                return visitor.visitVfrStatementVarStoreEfi(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementVarStoreEfi(self):

        localctx = VfrSyntaxParser.VfrStatementVarStoreEfiContext(self, self._ctx, self.state)
        self.enterRule(localctx, 62, self.RULE_vfrStatementVarStoreEfi)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 756
            self.match(VfrSyntaxParser.Efivarstore)
            self.state = 773
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.StringIdentifier]:
                self.state = 757
                localctx.TN = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 758
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint8]:
                self.state = 759
                self.match(VfrSyntaxParser.Uint8)
                self.state = 760
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint16]:
                self.state = 761
                self.match(VfrSyntaxParser.Uint16)
                self.state = 762
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint32]:
                self.state = 763
                self.match(VfrSyntaxParser.Uint32)
                self.state = 764
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Uint64]:
                self.state = 765
                self.match(VfrSyntaxParser.Uint64)
                self.state = 766
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.EFI_HII_DATE]:
                self.state = 767
                self.match(VfrSyntaxParser.EFI_HII_DATE)
                self.state = 768
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.EFI_HII_TIME]:
                self.state = 769
                self.match(VfrSyntaxParser.EFI_HII_TIME)
                self.state = 770
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.EFI_HII_REF]:
                self.state = 771
                self.match(VfrSyntaxParser.EFI_HII_REF)
                self.state = 772
                self.match(VfrSyntaxParser.Comma)
                pass
            else:
                raise NoViableAltException(self)

            self.state = 779
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.VarId:
                self.state = 775
                self.match(VfrSyntaxParser.VarId)
                self.state = 776
                self.match(VfrSyntaxParser.T__5)
                self.state = 777
                localctx.ID = self.match(VfrSyntaxParser.Number)
                self.state = 778
                self.match(VfrSyntaxParser.Comma)


            self.state = 781
            self.match(VfrSyntaxParser.Attribute)
            self.state = 782
            self.match(VfrSyntaxParser.T__5)
            self.state = 783
            self.vfrVarStoreEfiAttr()
            self.state = 788
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 784
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 785
                self.vfrVarStoreEfiAttr()
                self.state = 790
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 791
            self.match(VfrSyntaxParser.Comma)
            self.state = 807
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,46,self._ctx)
            if la_ == 1:
                self.state = 792
                self.match(VfrSyntaxParser.Name)
                self.state = 793
                self.match(VfrSyntaxParser.T__5)
                self.state = 794
                localctx.SN = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 795
                self.match(VfrSyntaxParser.Comma)
                pass

            elif la_ == 2:
                self.state = 796
                self.match(VfrSyntaxParser.Name)
                self.state = 797
                self.match(VfrSyntaxParser.T__5)
                self.state = 798
                self.match(VfrSyntaxParser.StringToken)
                self.state = 799
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 800
                localctx.VN = self.match(VfrSyntaxParser.Number)
                self.state = 801
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 802
                self.match(VfrSyntaxParser.Comma)
                self.state = 803
                self.match(VfrSyntaxParser.VarSize)
                self.state = 804
                self.match(VfrSyntaxParser.T__5)
                self.state = 805
                localctx.N = self.match(VfrSyntaxParser.Number)
                self.state = 806
                self.match(VfrSyntaxParser.Comma)
                pass


            self.state = 809
            self.match(VfrSyntaxParser.Uuid)
            self.state = 810
            self.match(VfrSyntaxParser.T__5)
            self.state = 811
            self.guidDefinition()
            self.state = 812
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrVarStoreEfiAttrContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Attr = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrVarStoreEfiAttr

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrVarStoreEfiAttr" ):
                return visitor.visitVfrVarStoreEfiAttr(self)
            else:
                return visitor.visitChildren(self)




    def vfrVarStoreEfiAttr(self):

        localctx = VfrSyntaxParser.VfrVarStoreEfiAttrContext(self, self._ctx, self.state)
        self.enterRule(localctx, 64, self.RULE_vfrVarStoreEfiAttr)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 814
            self.match(VfrSyntaxParser.Number)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementVarStoreNameValueContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_VARSTORE_NAME_VALUE_OP)
            self.SN = None # Token
            self.ID = None # Token

        def NameValueVarStore(self):
            return self.getToken(VfrSyntaxParser.NameValueVarStore, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Uuid(self):
            return self.getToken(VfrSyntaxParser.Uuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def VarId(self):
            return self.getToken(VfrSyntaxParser.VarId, 0)

        def Name(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Name)
            else:
                return self.getToken(VfrSyntaxParser.Name, i)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementVarStoreNameValue

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementVarStoreNameValue" ):
                return visitor.visitVfrStatementVarStoreNameValue(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementVarStoreNameValue(self):

        localctx = VfrSyntaxParser.VfrStatementVarStoreNameValueContext(self, self._ctx, self.state)
        self.enterRule(localctx, 66, self.RULE_vfrStatementVarStoreNameValue)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 816
            self.match(VfrSyntaxParser.NameValueVarStore)
            self.state = 817
            localctx.SN = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 818
            self.match(VfrSyntaxParser.Comma)
            self.state = 823
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.VarId:
                self.state = 819
                self.match(VfrSyntaxParser.VarId)
                self.state = 820
                self.match(VfrSyntaxParser.T__5)
                self.state = 821
                localctx.ID = self.match(VfrSyntaxParser.Number)
                self.state = 822
                self.match(VfrSyntaxParser.Comma)


            self.state = 832
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while True:
                self.state = 825
                self.match(VfrSyntaxParser.Name)
                self.state = 826
                self.match(VfrSyntaxParser.T__5)
                self.state = 827
                self.match(VfrSyntaxParser.StringToken)
                self.state = 828
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 829
                self.match(VfrSyntaxParser.Number)
                self.state = 830
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 831
                self.match(VfrSyntaxParser.Comma)
                self.state = 834
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if not (_la==VfrSyntaxParser.Name):
                    break

            self.state = 836
            self.match(VfrSyntaxParser.Uuid)
            self.state = 837
            self.match(VfrSyntaxParser.T__5)
            self.state = 838
            self.guidDefinition()
            self.state = 839
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementDisableIfFormSetContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_DISABLE_IF_OP)

        def DisableIf(self):
            return self.getToken(VfrSyntaxParser.DisableIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def vfrFormSetList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrFormSetListContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementDisableIfFormSet

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementDisableIfFormSet" ):
                return visitor.visitVfrStatementDisableIfFormSet(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementDisableIfFormSet(self):

        localctx = VfrSyntaxParser.VfrStatementDisableIfFormSetContext(self, self._ctx, self.state)
        self.enterRule(localctx, 68, self.RULE_vfrStatementDisableIfFormSet)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 841
            self.match(VfrSyntaxParser.DisableIf)
            self.state = 842
            self.vfrStatementExpression(localctx.Node)
            self.state = 843
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 844
            self.vfrFormSetList(localctx.Node)
            self.state = 845
            self.match(VfrSyntaxParser.EndIf)
            self.state = 846
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementSuppressIfFormSetContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)

        def SuppressIf(self):
            return self.getToken(VfrSyntaxParser.SuppressIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def vfrFormSetList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrFormSetListContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementSuppressIfFormSet

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementSuppressIfFormSet" ):
                return visitor.visitVfrStatementSuppressIfFormSet(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementSuppressIfFormSet(self):

        localctx = VfrSyntaxParser.VfrStatementSuppressIfFormSetContext(self, self._ctx, self.state)
        self.enterRule(localctx, 70, self.RULE_vfrStatementSuppressIfFormSet)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 848
            self.match(VfrSyntaxParser.SuppressIf)
            self.state = 849
            self.vfrStatementExpression(localctx.Node)
            self.state = 850
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 851
            self.vfrFormSetList(localctx.Node)
            self.state = 852
            self.match(VfrSyntaxParser.EndIf)
            self.state = 853
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class GuidSubDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Guid=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Guid = None
            self.Guid = Guid

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_guidSubDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitGuidSubDefinition" ):
                return visitor.visitGuidSubDefinition(self)
            else:
                return visitor.visitChildren(self)




    def guidSubDefinition(self, Guid):

        localctx = VfrSyntaxParser.GuidSubDefinitionContext(self, self._ctx, self.state, Guid)
        self.enterRule(localctx, 72, self.RULE_guidSubDefinition)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 855
            self.match(VfrSyntaxParser.Number)
            self.state = 856
            self.match(VfrSyntaxParser.Comma)
            self.state = 857
            self.match(VfrSyntaxParser.Number)
            self.state = 858
            self.match(VfrSyntaxParser.Comma)
            self.state = 859
            self.match(VfrSyntaxParser.Number)
            self.state = 860
            self.match(VfrSyntaxParser.Comma)
            self.state = 861
            self.match(VfrSyntaxParser.Number)
            self.state = 862
            self.match(VfrSyntaxParser.Comma)
            self.state = 863
            self.match(VfrSyntaxParser.Number)
            self.state = 864
            self.match(VfrSyntaxParser.Comma)
            self.state = 865
            self.match(VfrSyntaxParser.Number)
            self.state = 866
            self.match(VfrSyntaxParser.Comma)
            self.state = 867
            self.match(VfrSyntaxParser.Number)
            self.state = 868
            self.match(VfrSyntaxParser.Comma)
            self.state = 869
            self.match(VfrSyntaxParser.Number)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class GuidDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode()
            self.Guid = EFI_GUID()

        def OpenBrace(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenBrace)
            else:
                return self.getToken(VfrSyntaxParser.OpenBrace, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def CloseBrace(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseBrace)
            else:
                return self.getToken(VfrSyntaxParser.CloseBrace, i)

        def guidSubDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidSubDefinitionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_guidDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitGuidDefinition" ):
                return visitor.visitGuidDefinition(self)
            else:
                return visitor.visitChildren(self)




    def guidDefinition(self):

        localctx = VfrSyntaxParser.GuidDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 74, self.RULE_guidDefinition)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 871
            self.match(VfrSyntaxParser.OpenBrace)
            self.state = 872
            self.match(VfrSyntaxParser.Number)
            self.state = 873
            self.match(VfrSyntaxParser.Comma)
            self.state = 874
            self.match(VfrSyntaxParser.Number)
            self.state = 875
            self.match(VfrSyntaxParser.Comma)
            self.state = 876
            self.match(VfrSyntaxParser.Number)
            self.state = 877
            self.match(VfrSyntaxParser.Comma)
            self.state = 883
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.OpenBrace]:
                self.state = 878
                self.match(VfrSyntaxParser.OpenBrace)
                self.state = 879
                self.guidSubDefinition(localctx.Guid)
                self.state = 880
                self.match(VfrSyntaxParser.CloseBrace)
                pass
            elif token in [VfrSyntaxParser.Number]:
                self.state = 882
                self.guidSubDefinition(localctx.Guid)
                pass
            else:
                raise NoViableAltException(self)

            self.state = 885
            self.match(VfrSyntaxParser.CloseBrace)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class GetStringIdContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.StringId = ''

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_getStringId

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitGetStringId" ):
                return visitor.visitGetStringId(self)
            else:
                return visitor.visitChildren(self)




    def getStringId(self):

        localctx = VfrSyntaxParser.GetStringIdContext(self, self._ctx, self.state)
        self.enterRule(localctx, 76, self.RULE_getStringId)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 887
            self.match(VfrSyntaxParser.StringToken)
            self.state = 888
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 889
            self.match(VfrSyntaxParser.Number)
            self.state = 890
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrQuestionHeaderContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None, QType=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.QType = None
            self.Node = Node
            self.QType = QType

        def vfrQuestionBaseInfo(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionBaseInfoContext,0)


        def vfrStatementHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementHeaderContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrQuestionHeader

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrQuestionHeader" ):
                return visitor.visitVfrQuestionHeader(self)
            else:
                return visitor.visitChildren(self)




    def vfrQuestionHeader(self, Node, QType):

        localctx = VfrSyntaxParser.VfrQuestionHeaderContext(self, self._ctx, self.state, Node, QType)
        self.enterRule(localctx, 78, self.RULE_vfrQuestionHeader)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 892
            self.vfrQuestionBaseInfo(Node, QType)
            self.state = 893
            self.vfrStatementHeader(Node)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrQuestionBaseInfoContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None, QType=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.QType = None
            self.BaseInfo = EFI_VARSTORE_INFO()
            self.QId = EFI_QUESTION_ID_INVALID
            self.CheckFlag = True
            self.QName = None
            self.VarIdStr = ''
            self.QN = None # Token
            self.ID = None # Token
            self.Node = Node
            self.QType = QType

        def Name(self):
            return self.getToken(VfrSyntaxParser.Name, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def VarId(self):
            return self.getToken(VfrSyntaxParser.VarId, 0)

        def vfrStorageVarId(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStorageVarIdContext,0)


        def QuestionId(self):
            return self.getToken(VfrSyntaxParser.QuestionId, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrQuestionBaseInfo

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrQuestionBaseInfo" ):
                return visitor.visitVfrQuestionBaseInfo(self)
            else:
                return visitor.visitChildren(self)




    def vfrQuestionBaseInfo(self, Node, QType):

        localctx = VfrSyntaxParser.VfrQuestionBaseInfoContext(self, self._ctx, self.state, Node, QType)
        self.enterRule(localctx, 80, self.RULE_vfrQuestionBaseInfo)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 899
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Name:
                self.state = 895
                self.match(VfrSyntaxParser.Name)
                self.state = 896
                self.match(VfrSyntaxParser.T__5)
                self.state = 897
                localctx.QN = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 898
                self.match(VfrSyntaxParser.Comma)


            self.state = 906
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.VarId:
                self.state = 901
                self.match(VfrSyntaxParser.VarId)
                self.state = 902
                self.match(VfrSyntaxParser.T__5)
                self.state = 903
                self.vfrStorageVarId(localctx.BaseInfo, localctx.CheckFlag)
                self.state = 904
                self.match(VfrSyntaxParser.Comma)


            self.state = 912
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.QuestionId:
                self.state = 908
                self.match(VfrSyntaxParser.QuestionId)
                self.state = 909
                self.match(VfrSyntaxParser.T__5)
                self.state = 910
                localctx.ID = self.match(VfrSyntaxParser.Number)
                self.state = 911
                self.match(VfrSyntaxParser.Comma)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementHeaderContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.Node = Node

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def Help(self):
            return self.getToken(VfrSyntaxParser.Help, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementHeader

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementHeader" ):
                return visitor.visitVfrStatementHeader(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementHeader(self, Node):

        localctx = VfrSyntaxParser.VfrStatementHeaderContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 82, self.RULE_vfrStatementHeader)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 914
            self.match(VfrSyntaxParser.Prompt)
            self.state = 915
            self.match(VfrSyntaxParser.T__5)
            self.state = 916
            self.match(VfrSyntaxParser.StringToken)
            self.state = 917
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 918
            self.match(VfrSyntaxParser.Number)
            self.state = 919
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 920
            self.match(VfrSyntaxParser.Comma)
            self.state = 921
            self.match(VfrSyntaxParser.Help)
            self.state = 922
            self.match(VfrSyntaxParser.T__5)
            self.state = 923
            self.match(VfrSyntaxParser.StringToken)
            self.state = 924
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 925
            self.match(VfrSyntaxParser.Number)
            self.state = 926
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class QuestionheaderFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.QHFlag = 0
            self.O = None # Token
            self.N = None # Token
            self.L = None # Token

        def ReadOnlyFlag(self):
            return self.getToken(VfrSyntaxParser.ReadOnlyFlag, 0)

        def InteractiveFlag(self):
            return self.getToken(VfrSyntaxParser.InteractiveFlag, 0)

        def ResetRequiredFlag(self):
            return self.getToken(VfrSyntaxParser.ResetRequiredFlag, 0)

        def RestStyleFlag(self):
            return self.getToken(VfrSyntaxParser.RestStyleFlag, 0)

        def ReconnectRequiredFlag(self):
            return self.getToken(VfrSyntaxParser.ReconnectRequiredFlag, 0)

        def OptionOnlyFlag(self):
            return self.getToken(VfrSyntaxParser.OptionOnlyFlag, 0)

        def NVAccessFlag(self):
            return self.getToken(VfrSyntaxParser.NVAccessFlag, 0)

        def LateCheckFlag(self):
            return self.getToken(VfrSyntaxParser.LateCheckFlag, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_questionheaderFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitQuestionheaderFlagsField" ):
                return visitor.visitQuestionheaderFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def questionheaderFlagsField(self):

        localctx = VfrSyntaxParser.QuestionheaderFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 84, self.RULE_questionheaderFlagsField)
        try:
            self.state = 936
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.ReadOnlyFlag]:
                self.enterOuterAlt(localctx, 1)
                self.state = 928
                self.match(VfrSyntaxParser.ReadOnlyFlag)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 929
                self.match(VfrSyntaxParser.InteractiveFlag)
                pass
            elif token in [VfrSyntaxParser.ResetRequiredFlag]:
                self.enterOuterAlt(localctx, 3)
                self.state = 930
                self.match(VfrSyntaxParser.ResetRequiredFlag)
                pass
            elif token in [VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 4)
                self.state = 931
                self.match(VfrSyntaxParser.RestStyleFlag)
                pass
            elif token in [VfrSyntaxParser.ReconnectRequiredFlag]:
                self.enterOuterAlt(localctx, 5)
                self.state = 932
                self.match(VfrSyntaxParser.ReconnectRequiredFlag)
                pass
            elif token in [VfrSyntaxParser.OptionOnlyFlag]:
                self.enterOuterAlt(localctx, 6)
                self.state = 933
                localctx.O = self.match(VfrSyntaxParser.OptionOnlyFlag)
                pass
            elif token in [VfrSyntaxParser.NVAccessFlag]:
                self.enterOuterAlt(localctx, 7)
                self.state = 934
                localctx.N = self.match(VfrSyntaxParser.NVAccessFlag)
                pass
            elif token in [VfrSyntaxParser.LateCheckFlag]:
                self.enterOuterAlt(localctx, 8)
                self.state = 935
                localctx.L = self.match(VfrSyntaxParser.LateCheckFlag)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStorageVarIdContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, BaseInfo=None, CheckFlag=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.BaseInfo = None
            self.CheckFlag = None
            self.VarIdStr = ''
            self.BaseInfo = BaseInfo
            self.CheckFlag = CheckFlag


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStorageVarId


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.BaseInfo = ctx.BaseInfo
            self.CheckFlag = ctx.CheckFlag
            self.VarIdStr = ctx.VarIdStr



    class VfrStorageVarIdRule1Context(VfrStorageVarIdContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.VfrStorageVarIdContext
            super().__init__(parser)
            self.SN1 = None # Token
            self.I = None # Token
            self.copyFrom(ctx)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)
        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)
        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)
        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStorageVarIdRule1" ):
                return visitor.visitVfrStorageVarIdRule1(self)
            else:
                return visitor.visitChildren(self)


    class VfrStorageVarIdRule2Context(VfrStorageVarIdContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.VfrStorageVarIdContext
            super().__init__(parser)
            self.SN2 = None # Token
            self.copyFrom(ctx)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)
        def Dot(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Dot)
            else:
                return self.getToken(VfrSyntaxParser.Dot, i)
        def arrayName(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.ArrayNameContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.ArrayNameContext,i)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStorageVarIdRule2" ):
                return visitor.visitVfrStorageVarIdRule2(self)
            else:
                return visitor.visitChildren(self)



    def vfrStorageVarId(self, BaseInfo, CheckFlag):

        localctx = VfrSyntaxParser.VfrStorageVarIdContext(self, self._ctx, self.state, BaseInfo, CheckFlag)
        self.enterRule(localctx, 86, self.RULE_vfrStorageVarId)
        self._la = 0 # Token type
        try:
            self.state = 950
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,55,self._ctx)
            if la_ == 1:
                localctx = VfrSyntaxParser.VfrStorageVarIdRule1Context(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 938
                localctx.SN1 = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 939
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 940
                localctx.I = self.match(VfrSyntaxParser.Number)
                self.state = 941
                self.match(VfrSyntaxParser.CloseBracket)
                pass

            elif la_ == 2:
                localctx = VfrSyntaxParser.VfrStorageVarIdRule2Context(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 942
                localctx.SN2 = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 947
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.Dot:
                    self.state = 943
                    self.match(VfrSyntaxParser.Dot)
                    self.state = 944
                    self.arrayName()
                    self.state = 949
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                pass


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrConstantValueFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ValueList = []
            self.ListType = False

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Negative(self):
            return self.getToken(VfrSyntaxParser.Negative, 0)

        def TrueSymbol(self):
            return self.getToken(VfrSyntaxParser.TrueSymbol, 0)

        def FalseSymbol(self):
            return self.getToken(VfrSyntaxParser.FalseSymbol, 0)

        def One(self):
            return self.getToken(VfrSyntaxParser.One, 0)

        def Ones(self):
            return self.getToken(VfrSyntaxParser.Ones, 0)

        def Zero(self):
            return self.getToken(VfrSyntaxParser.Zero, 0)

        def Colon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Colon)
            else:
                return self.getToken(VfrSyntaxParser.Colon, i)

        def Slash(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Slash)
            else:
                return self.getToken(VfrSyntaxParser.Slash, i)

        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def OpenBrace(self):
            return self.getToken(VfrSyntaxParser.OpenBrace, 0)

        def CloseBrace(self):
            return self.getToken(VfrSyntaxParser.CloseBrace, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrConstantValueField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrConstantValueField" ):
                return visitor.visitVfrConstantValueField(self)
            else:
                return visitor.visitChildren(self)




    def vfrConstantValueField(self):

        localctx = VfrSyntaxParser.VfrConstantValueFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 88, self.RULE_vfrConstantValueField)
        self._la = 0 # Token type
        try:
            self.state = 996
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,58,self._ctx)
            if la_ == 1:
                self.enterOuterAlt(localctx, 1)
                self.state = 953
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.Negative:
                    self.state = 952
                    self.match(VfrSyntaxParser.Negative)


                self.state = 955
                self.match(VfrSyntaxParser.Number)
                pass

            elif la_ == 2:
                self.enterOuterAlt(localctx, 2)
                self.state = 956
                self.match(VfrSyntaxParser.TrueSymbol)
                pass

            elif la_ == 3:
                self.enterOuterAlt(localctx, 3)
                self.state = 957
                self.match(VfrSyntaxParser.FalseSymbol)
                pass

            elif la_ == 4:
                self.enterOuterAlt(localctx, 4)
                self.state = 958
                self.match(VfrSyntaxParser.One)
                pass

            elif la_ == 5:
                self.enterOuterAlt(localctx, 5)
                self.state = 959
                self.match(VfrSyntaxParser.Ones)
                pass

            elif la_ == 6:
                self.enterOuterAlt(localctx, 6)
                self.state = 960
                self.match(VfrSyntaxParser.Zero)
                pass

            elif la_ == 7:
                self.enterOuterAlt(localctx, 7)
                self.state = 961
                self.match(VfrSyntaxParser.Number)
                self.state = 962
                self.match(VfrSyntaxParser.Colon)
                self.state = 963
                self.match(VfrSyntaxParser.Number)
                self.state = 964
                self.match(VfrSyntaxParser.Colon)
                self.state = 965
                self.match(VfrSyntaxParser.Number)
                pass

            elif la_ == 8:
                self.enterOuterAlt(localctx, 8)
                self.state = 966
                self.match(VfrSyntaxParser.Number)
                self.state = 967
                self.match(VfrSyntaxParser.Slash)
                self.state = 968
                self.match(VfrSyntaxParser.Number)
                self.state = 969
                self.match(VfrSyntaxParser.Slash)
                self.state = 970
                self.match(VfrSyntaxParser.Number)
                pass

            elif la_ == 9:
                self.enterOuterAlt(localctx, 9)
                self.state = 971
                self.match(VfrSyntaxParser.Number)
                self.state = 972
                self.match(VfrSyntaxParser.Semicolon)
                self.state = 973
                self.match(VfrSyntaxParser.Number)
                self.state = 974
                self.match(VfrSyntaxParser.Semicolon)
                self.state = 975
                self.guidDefinition()
                self.state = 976
                self.match(VfrSyntaxParser.Semicolon)
                self.state = 977
                self.match(VfrSyntaxParser.StringToken)
                self.state = 978
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 979
                self.match(VfrSyntaxParser.Number)
                self.state = 980
                self.match(VfrSyntaxParser.CloseParen)
                pass

            elif la_ == 10:
                self.enterOuterAlt(localctx, 10)
                self.state = 982
                self.match(VfrSyntaxParser.StringToken)
                self.state = 983
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 984
                self.match(VfrSyntaxParser.Number)
                self.state = 985
                self.match(VfrSyntaxParser.CloseParen)
                pass

            elif la_ == 11:
                self.enterOuterAlt(localctx, 11)
                self.state = 986
                self.match(VfrSyntaxParser.OpenBrace)
                self.state = 987
                self.match(VfrSyntaxParser.Number)
                self.state = 992
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.Comma:
                    self.state = 988
                    self.match(VfrSyntaxParser.Comma)
                    self.state = 989
                    self.match(VfrSyntaxParser.Number)
                    self.state = 994
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 995
                self.match(VfrSyntaxParser.CloseBrace)
                pass


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrImageTagContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_IMAGE_OP)

        def Image(self):
            return self.getToken(VfrSyntaxParser.Image, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrImageTag

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrImageTag" ):
                return visitor.visitVfrImageTag(self)
            else:
                return visitor.visitChildren(self)




    def vfrImageTag(self):

        localctx = VfrSyntaxParser.VfrImageTagContext(self, self._ctx, self.state)
        self.enterRule(localctx, 90, self.RULE_vfrImageTag)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 998
            self.match(VfrSyntaxParser.Image)
            self.state = 999
            self.match(VfrSyntaxParser.T__5)
            self.state = 1000
            self.match(VfrSyntaxParser.T__6)
            self.state = 1001
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1002
            self.match(VfrSyntaxParser.Number)
            self.state = 1003
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrLockedTagContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_LOCKED_OP)

        def Locked(self):
            return self.getToken(VfrSyntaxParser.Locked, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrLockedTag

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrLockedTag" ):
                return visitor.visitVfrLockedTag(self)
            else:
                return visitor.visitChildren(self)




    def vfrLockedTag(self):

        localctx = VfrSyntaxParser.VfrLockedTagContext(self, self._ctx, self.state)
        self.enterRule(localctx, 92, self.RULE_vfrLockedTag)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1005
            self.match(VfrSyntaxParser.Locked)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStatTagContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrImageTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrImageTagContext,0)


        def vfrLockedTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrLockedTagContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStatTag

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStatTag" ):
                return visitor.visitVfrStatementStatTag(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStatTag(self):

        localctx = VfrSyntaxParser.VfrStatementStatTagContext(self, self._ctx, self.state)
        self.enterRule(localctx, 94, self.RULE_vfrStatementStatTag)
        try:
            self.state = 1009
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Image]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1007
                self.vfrImageTag()
                pass
            elif token in [VfrSyntaxParser.Locked]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1008
                self.vfrLockedTag()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStatTagListContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.Node = Node

        def vfrStatementStatTag(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementStatTagContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatTagContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStatTagList

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStatTagList" ):
                return visitor.visitVfrStatementStatTagList(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStatTagList(self, Node):

        localctx = VfrSyntaxParser.VfrStatementStatTagListContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 96, self.RULE_vfrStatementStatTagList)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1011
            self.vfrStatementStatTag()
            self.state = 1016
            self._errHandler.sync(self)
            _alt = self._interp.adaptivePredict(self._input,60,self._ctx)
            while _alt!=2 and _alt!=ATN.INVALID_ALT_NUMBER:
                if _alt==1:
                    self.state = 1012
                    self.match(VfrSyntaxParser.Comma)
                    self.state = 1013
                    self.vfrStatementStatTag()
                self.state = 1018
                self._errHandler.sync(self)
                _alt = self._interp.adaptivePredict(self._input,60,self._ctx)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrFormDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_FORM_OP)

        def Form(self):
            return self.getToken(VfrSyntaxParser.Form, 0)

        def FormId(self):
            return self.getToken(VfrSyntaxParser.FormId, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def Title(self):
            return self.getToken(VfrSyntaxParser.Title, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def EndForm(self):
            return self.getToken(VfrSyntaxParser.EndForm, 0)

        def vfrForm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrFormContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrFormContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrFormDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrFormDefinition" ):
                return visitor.visitVfrFormDefinition(self)
            else:
                return visitor.visitChildren(self)




    def vfrFormDefinition(self):

        localctx = VfrSyntaxParser.VfrFormDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 98, self.RULE_vfrFormDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1019
            self.match(VfrSyntaxParser.Form)
            self.state = 1020
            self.match(VfrSyntaxParser.FormId)
            self.state = 1021
            self.match(VfrSyntaxParser.T__5)
            self.state = 1022
            self.match(VfrSyntaxParser.Number)
            self.state = 1023
            self.match(VfrSyntaxParser.Comma)
            self.state = 1024
            self.match(VfrSyntaxParser.Title)
            self.state = 1025
            self.match(VfrSyntaxParser.T__5)
            self.state = 1026
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1027
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1028
            self.match(VfrSyntaxParser.Number)
            self.state = 1029
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1030
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 1034
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 46)) & ~0x3f) == 0 and ((1 << (_la - 46)) & ((1 << (VfrSyntaxParser.OneOf - 46)) | (1 << (VfrSyntaxParser.OrderedList - 46)) | (1 << (VfrSyntaxParser.Subtitle - 46)) | (1 << (VfrSyntaxParser.Text - 46)) | (1 << (VfrSyntaxParser.Date - 46)) | (1 << (VfrSyntaxParser.Time - 46)) | (1 << (VfrSyntaxParser.GrayOutIf - 46)) | (1 << (VfrSyntaxParser.Label - 46)) | (1 << (VfrSyntaxParser.Inventory - 46)) | (1 << (VfrSyntaxParser.CheckBox - 46)) | (1 << (VfrSyntaxParser.Numeric - 46)) | (1 << (VfrSyntaxParser.Default - 46)) | (1 << (VfrSyntaxParser.Password - 46)) | (1 << (VfrSyntaxParser.String - 46)) | (1 << (VfrSyntaxParser.SuppressIf - 46)) | (1 << (VfrSyntaxParser.DisableIf - 46)) | (1 << (VfrSyntaxParser.Hidden - 46)) | (1 << (VfrSyntaxParser.Goto - 46)))) != 0) or ((((_la - 110)) & ~0x3f) == 0 and ((1 << (_la - 110)) & ((1 << (VfrSyntaxParser.InconsistentIf - 110)) | (1 << (VfrSyntaxParser.Restore - 110)) | (1 << (VfrSyntaxParser.Save - 110)) | (1 << (VfrSyntaxParser.Banner - 110)) | (1 << (VfrSyntaxParser.Image - 110)) | (1 << (VfrSyntaxParser.Locked - 110)) | (1 << (VfrSyntaxParser.Rule - 110)) | (1 << (VfrSyntaxParser.ResetButton - 110)) | (1 << (VfrSyntaxParser.Action - 110)) | (1 << (VfrSyntaxParser.GuidOp - 110)) | (1 << (VfrSyntaxParser.Modal - 110)))) != 0) or _la==VfrSyntaxParser.RefreshGuid:
                self.state = 1031
                self.vfrForm()
                self.state = 1036
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 1037
            self.match(VfrSyntaxParser.EndForm)
            self.state = 1038
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrFormContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementImage(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementImageContext,0)


        def vfrStatementLocked(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementLockedContext,0)


        def vfrStatementRules(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementRulesContext,0)


        def vfrStatementDefault(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDefaultContext,0)


        def vfrStatementStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatContext,0)


        def vfrStatementQuestions(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionsContext,0)


        def vfrStatementConditional(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementConditionalContext,0)


        def vfrStatementLabel(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementLabelContext,0)


        def vfrStatementBanner(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementBannerContext,0)


        def vfrStatementInvalid(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInvalidContext,0)


        def vfrStatementExtension(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExtensionContext,0)


        def vfrStatementModal(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementModalContext,0)


        def vfrStatementRefreshEvent(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementRefreshEventContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrForm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrForm" ):
                return visitor.visitVfrForm(self)
            else:
                return visitor.visitChildren(self)




    def vfrForm(self):

        localctx = VfrSyntaxParser.VfrFormContext(self, self._ctx, self.state)
        self.enterRule(localctx, 100, self.RULE_vfrForm)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1055
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Image]:
                self.state = 1040
                self.vfrStatementImage()
                pass
            elif token in [VfrSyntaxParser.Locked]:
                self.state = 1041
                self.vfrStatementLocked()
                pass
            elif token in [VfrSyntaxParser.Rule]:
                self.state = 1042
                self.vfrStatementRules()
                pass
            elif token in [VfrSyntaxParser.Default]:
                self.state = 1043
                self.vfrStatementDefault()
                pass
            elif token in [VfrSyntaxParser.Subtitle, VfrSyntaxParser.Text, VfrSyntaxParser.Goto, VfrSyntaxParser.ResetButton]:
                self.state = 1044
                self.vfrStatementStat()
                pass
            elif token in [VfrSyntaxParser.OneOf, VfrSyntaxParser.OrderedList, VfrSyntaxParser.Date, VfrSyntaxParser.Time, VfrSyntaxParser.CheckBox, VfrSyntaxParser.Numeric, VfrSyntaxParser.Password, VfrSyntaxParser.String, VfrSyntaxParser.Action]:
                self.state = 1045
                self.vfrStatementQuestions()
                pass
            elif token in [VfrSyntaxParser.GrayOutIf, VfrSyntaxParser.SuppressIf, VfrSyntaxParser.DisableIf, VfrSyntaxParser.InconsistentIf]:
                self.state = 1046
                self.vfrStatementConditional()
                pass
            elif token in [VfrSyntaxParser.Label]:
                self.state = 1047
                self.vfrStatementLabel()
                pass
            elif token in [VfrSyntaxParser.Banner]:
                self.state = 1048
                self.vfrStatementBanner()
                pass
            elif token in [VfrSyntaxParser.Inventory, VfrSyntaxParser.Hidden, VfrSyntaxParser.Restore, VfrSyntaxParser.Save]:
                self.state = 1049
                self.vfrStatementInvalid()
                pass
            elif token in [VfrSyntaxParser.GuidOp]:
                self.state = 1050
                self.vfrStatementExtension()
                pass
            elif token in [VfrSyntaxParser.Modal]:
                self.state = 1051
                self.vfrStatementModal()
                pass
            elif token in [VfrSyntaxParser.RefreshGuid]:
                self.state = 1052
                self.vfrStatementRefreshEvent()
                self.state = 1053
                self.match(VfrSyntaxParser.Semicolon)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrFormMapDefinitionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_FORM_MAP_OP)
            self.S1 = None # Token

        def FormMap(self):
            return self.getToken(VfrSyntaxParser.FormMap, 0)

        def FormId(self):
            return self.getToken(VfrSyntaxParser.FormId, 0)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def EndForm(self):
            return self.getToken(VfrSyntaxParser.EndForm, 0)

        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def MapTitle(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.MapTitle)
            else:
                return self.getToken(VfrSyntaxParser.MapTitle, i)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def MapGuid(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.MapGuid)
            else:
                return self.getToken(VfrSyntaxParser.MapGuid, i)

        def guidDefinition(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.GuidDefinitionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,i)


        def vfrForm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrFormContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrFormContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrFormMapDefinition

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrFormMapDefinition" ):
                return visitor.visitVfrFormMapDefinition(self)
            else:
                return visitor.visitChildren(self)




    def vfrFormMapDefinition(self):

        localctx = VfrSyntaxParser.VfrFormMapDefinitionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 102, self.RULE_vfrFormMapDefinition)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1057
            self.match(VfrSyntaxParser.FormMap)
            self.state = 1058
            self.match(VfrSyntaxParser.FormId)
            self.state = 1059
            self.match(VfrSyntaxParser.T__5)
            self.state = 1060
            localctx.S1 = self.match(VfrSyntaxParser.Number)
            self.state = 1061
            self.match(VfrSyntaxParser.Comma)
            self.state = 1076
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.MapTitle:
                self.state = 1062
                self.match(VfrSyntaxParser.MapTitle)
                self.state = 1063
                self.match(VfrSyntaxParser.T__5)
                self.state = 1064
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1065
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1066
                self.match(VfrSyntaxParser.Number)
                self.state = 1067
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1068
                self.match(VfrSyntaxParser.Semicolon)
                self.state = 1069
                self.match(VfrSyntaxParser.MapGuid)
                self.state = 1070
                self.match(VfrSyntaxParser.T__5)
                self.state = 1071
                self.guidDefinition()
                self.state = 1072
                self.match(VfrSyntaxParser.Semicolon)
                self.state = 1078
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 1082
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 46)) & ~0x3f) == 0 and ((1 << (_la - 46)) & ((1 << (VfrSyntaxParser.OneOf - 46)) | (1 << (VfrSyntaxParser.OrderedList - 46)) | (1 << (VfrSyntaxParser.Subtitle - 46)) | (1 << (VfrSyntaxParser.Text - 46)) | (1 << (VfrSyntaxParser.Date - 46)) | (1 << (VfrSyntaxParser.Time - 46)) | (1 << (VfrSyntaxParser.GrayOutIf - 46)) | (1 << (VfrSyntaxParser.Label - 46)) | (1 << (VfrSyntaxParser.Inventory - 46)) | (1 << (VfrSyntaxParser.CheckBox - 46)) | (1 << (VfrSyntaxParser.Numeric - 46)) | (1 << (VfrSyntaxParser.Default - 46)) | (1 << (VfrSyntaxParser.Password - 46)) | (1 << (VfrSyntaxParser.String - 46)) | (1 << (VfrSyntaxParser.SuppressIf - 46)) | (1 << (VfrSyntaxParser.DisableIf - 46)) | (1 << (VfrSyntaxParser.Hidden - 46)) | (1 << (VfrSyntaxParser.Goto - 46)))) != 0) or ((((_la - 110)) & ~0x3f) == 0 and ((1 << (_la - 110)) & ((1 << (VfrSyntaxParser.InconsistentIf - 110)) | (1 << (VfrSyntaxParser.Restore - 110)) | (1 << (VfrSyntaxParser.Save - 110)) | (1 << (VfrSyntaxParser.Banner - 110)) | (1 << (VfrSyntaxParser.Image - 110)) | (1 << (VfrSyntaxParser.Locked - 110)) | (1 << (VfrSyntaxParser.Rule - 110)) | (1 << (VfrSyntaxParser.ResetButton - 110)) | (1 << (VfrSyntaxParser.Action - 110)) | (1 << (VfrSyntaxParser.GuidOp - 110)) | (1 << (VfrSyntaxParser.Modal - 110)))) != 0) or _la==VfrSyntaxParser.RefreshGuid:
                self.state = 1079
                self.vfrForm()
                self.state = 1084
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 1085
            self.match(VfrSyntaxParser.EndForm)
            self.state = 1086
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementImageContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrImageTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrImageTagContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementImage

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementImage" ):
                return visitor.visitVfrStatementImage(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementImage(self):

        localctx = VfrSyntaxParser.VfrStatementImageContext(self, self._ctx, self.state)
        self.enterRule(localctx, 104, self.RULE_vfrStatementImage)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1088
            self.vfrImageTag()
            self.state = 1089
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementLockedContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrLockedTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrLockedTagContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementLocked

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementLocked" ):
                return visitor.visitVfrStatementLocked(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementLocked(self):

        localctx = VfrSyntaxParser.VfrStatementLockedContext(self, self._ctx, self.state)
        self.enterRule(localctx, 106, self.RULE_vfrStatementLocked)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1091
            self.vfrLockedTag()
            self.state = 1092
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementRulesContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_RULE_OP)

        def Rule(self):
            return self.getToken(VfrSyntaxParser.Rule, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def EndRule(self):
            return self.getToken(VfrSyntaxParser.EndRule, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementRules

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementRules" ):
                return visitor.visitVfrStatementRules(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementRules(self):

        localctx = VfrSyntaxParser.VfrStatementRulesContext(self, self._ctx, self.state)
        self.enterRule(localctx, 108, self.RULE_vfrStatementRules)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1094
            self.match(VfrSyntaxParser.Rule)
            self.state = 1095
            self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 1096
            self.match(VfrSyntaxParser.Comma)
            self.state = 1097
            self.vfrStatementExpression(localctx.Node)
            self.state = 1098
            self.match(VfrSyntaxParser.EndRule)
            self.state = 1099
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStatContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementSubTitle(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSubTitleContext,0)


        def vfrStatementStaticText(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStaticTextContext,0)


        def vfrStatementCrossReference(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementCrossReferenceContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStat

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStat" ):
                return visitor.visitVfrStatementStat(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStat(self):

        localctx = VfrSyntaxParser.VfrStatementStatContext(self, self._ctx, self.state)
        self.enterRule(localctx, 110, self.RULE_vfrStatementStat)
        try:
            self.state = 1104
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Subtitle]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1101
                self.vfrStatementSubTitle()
                pass
            elif token in [VfrSyntaxParser.Text]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1102
                self.vfrStatementStaticText()
                pass
            elif token in [VfrSyntaxParser.Goto, VfrSyntaxParser.ResetButton]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1103
                self.vfrStatementCrossReference()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementSubTitleContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_SUBTITLE_OP)

        def Subtitle(self):
            return self.getToken(VfrSyntaxParser.Subtitle, 0)

        def Text(self):
            return self.getToken(VfrSyntaxParser.Text, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def EndSubtitle(self):
            return self.getToken(VfrSyntaxParser.EndSubtitle, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def vfrSubtitleFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrSubtitleFlagsContext,0)


        def vfrStatementStatTagList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatTagListContext,0)


        def vfrStatementSubTitleComponent(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementSubTitleComponentContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSubTitleComponentContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementSubTitle

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementSubTitle" ):
                return visitor.visitVfrStatementSubTitle(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementSubTitle(self):

        localctx = VfrSyntaxParser.VfrStatementSubTitleContext(self, self._ctx, self.state)
        self.enterRule(localctx, 112, self.RULE_vfrStatementSubTitle)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1106
            self.match(VfrSyntaxParser.Subtitle)
            self.state = 1107
            self.match(VfrSyntaxParser.Text)
            self.state = 1108
            self.match(VfrSyntaxParser.T__5)
            self.state = 1109
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1110
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1111
            self.match(VfrSyntaxParser.Number)
            self.state = 1112
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1117
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,66,self._ctx)
            if la_ == 1:
                self.state = 1113
                self.match(VfrSyntaxParser.Comma)
                self.state = 1114
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1115
                self.match(VfrSyntaxParser.T__5)
                self.state = 1116
                self.vfrSubtitleFlags()


            self.state = 1139
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,71,self._ctx)
            if la_ == 1:
                self.state = 1121
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.Comma:
                    self.state = 1119
                    self.match(VfrSyntaxParser.Comma)
                    self.state = 1120
                    self.vfrStatementStatTagList(localctx.Node)


                self.state = 1123
                self.match(VfrSyntaxParser.Semicolon)
                pass

            elif la_ == 2:
                self.state = 1126
                self._errHandler.sync(self)
                la_ = self._interp.adaptivePredict(self._input,68,self._ctx)
                if la_ == 1:
                    self.state = 1124
                    self.match(VfrSyntaxParser.Comma)
                    self.state = 1125
                    self.vfrStatementStatTagList(localctx.Node)


                self.state = 1135
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.Comma:
                    self.state = 1128
                    self.match(VfrSyntaxParser.Comma)
                    self.state = 1132
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    while ((((_la - 46)) & ~0x3f) == 0 and ((1 << (_la - 46)) & ((1 << (VfrSyntaxParser.OneOf - 46)) | (1 << (VfrSyntaxParser.OrderedList - 46)) | (1 << (VfrSyntaxParser.Subtitle - 46)) | (1 << (VfrSyntaxParser.Text - 46)) | (1 << (VfrSyntaxParser.Date - 46)) | (1 << (VfrSyntaxParser.Time - 46)) | (1 << (VfrSyntaxParser.CheckBox - 46)) | (1 << (VfrSyntaxParser.Numeric - 46)) | (1 << (VfrSyntaxParser.Password - 46)) | (1 << (VfrSyntaxParser.String - 46)) | (1 << (VfrSyntaxParser.Goto - 46)))) != 0) or _la==VfrSyntaxParser.ResetButton or _la==VfrSyntaxParser.Action:
                        self.state = 1129
                        self.vfrStatementSubTitleComponent()
                        self.state = 1134
                        self._errHandler.sync(self)
                        _la = self._input.LA(1)



                self.state = 1137
                self.match(VfrSyntaxParser.EndSubtitle)
                self.state = 1138
                self.match(VfrSyntaxParser.Semicolon)
                pass


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementSubTitleComponentContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatContext,0)


        def vfrStatementQuestions(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionsContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementSubTitleComponent

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementSubTitleComponent" ):
                return visitor.visitVfrStatementSubTitleComponent(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementSubTitleComponent(self):

        localctx = VfrSyntaxParser.VfrStatementSubTitleComponentContext(self, self._ctx, self.state)
        self.enterRule(localctx, 114, self.RULE_vfrStatementSubTitleComponent)
        try:
            self.state = 1143
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Subtitle, VfrSyntaxParser.Text, VfrSyntaxParser.Goto, VfrSyntaxParser.ResetButton]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1141
                self.vfrStatementStat()
                pass
            elif token in [VfrSyntaxParser.OneOf, VfrSyntaxParser.OrderedList, VfrSyntaxParser.Date, VfrSyntaxParser.Time, VfrSyntaxParser.CheckBox, VfrSyntaxParser.Numeric, VfrSyntaxParser.Password, VfrSyntaxParser.String, VfrSyntaxParser.Action]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1142
                self.vfrStatementQuestions()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrSubtitleFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.SubFlags = 0

        def subtitleFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.SubtitleFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.SubtitleFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrSubtitleFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrSubtitleFlags" ):
                return visitor.visitVfrSubtitleFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrSubtitleFlags(self):

        localctx = VfrSyntaxParser.VfrSubtitleFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 116, self.RULE_vfrSubtitleFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1145
            self.subtitleFlagsField()
            self.state = 1150
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1146
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1147
                self.subtitleFlagsField()
                self.state = 1152
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class SubtitleFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Flag = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_subtitleFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitSubtitleFlagsField" ):
                return visitor.visitSubtitleFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def subtitleFlagsField(self):

        localctx = VfrSyntaxParser.SubtitleFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 118, self.RULE_subtitleFlagsField)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1153
            _la = self._input.LA(1)
            if not(_la==VfrSyntaxParser.T__7 or _la==VfrSyntaxParser.Number):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStaticTextContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_TEXT_OP)
            self.S1 = None # Token
            self.S2 = None # Token
            self.S3 = None # Token
            self.F = None # Token
            self.S4 = None # Token

        def Text(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Text)
            else:
                return self.getToken(VfrSyntaxParser.Text, i)

        def Help(self):
            return self.getToken(VfrSyntaxParser.Help, 0)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def staticTextFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.StaticTextFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.StaticTextFlagsFieldContext,i)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def vfrStatementStatTagList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatTagListContext,0)


        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStaticText

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStaticText" ):
                return visitor.visitVfrStatementStaticText(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStaticText(self):

        localctx = VfrSyntaxParser.VfrStatementStaticTextContext(self, self._ctx, self.state)
        self.enterRule(localctx, 120, self.RULE_vfrStatementStaticText)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1155
            self.match(VfrSyntaxParser.Text)
            self.state = 1156
            self.match(VfrSyntaxParser.Help)
            self.state = 1157
            self.match(VfrSyntaxParser.T__5)
            self.state = 1158
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1159
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1160
            localctx.S1 = self.match(VfrSyntaxParser.Number)
            self.state = 1161
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1162
            self.match(VfrSyntaxParser.Comma)
            self.state = 1163
            self.match(VfrSyntaxParser.Text)
            self.state = 1164
            self.match(VfrSyntaxParser.T__5)
            self.state = 1165
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1166
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1167
            localctx.S2 = self.match(VfrSyntaxParser.Number)
            self.state = 1168
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1176
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,74,self._ctx)
            if la_ == 1:
                self.state = 1169
                self.match(VfrSyntaxParser.Comma)
                self.state = 1170
                self.match(VfrSyntaxParser.Text)
                self.state = 1171
                self.match(VfrSyntaxParser.T__5)
                self.state = 1172
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1173
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1174
                localctx.S3 = self.match(VfrSyntaxParser.Number)
                self.state = 1175
                self.match(VfrSyntaxParser.CloseParen)


            self.state = 1194
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,76,self._ctx)
            if la_ == 1:
                self.state = 1178
                self.match(VfrSyntaxParser.Comma)
                self.state = 1179
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1180
                self.match(VfrSyntaxParser.T__5)
                self.state = 1181
                self.staticTextFlagsField()
                self.state = 1186
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 1182
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 1183
                    self.staticTextFlagsField()
                    self.state = 1188
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 1189
                self.match(VfrSyntaxParser.Comma)
                self.state = 1190
                self.match(VfrSyntaxParser.Key)
                self.state = 1191
                self.match(VfrSyntaxParser.T__5)
                self.state = 1192
                localctx.S4 = self.match(VfrSyntaxParser.Number)


            self.state = 1198
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 1196
                self.match(VfrSyntaxParser.Comma)
                self.state = 1197
                self.vfrStatementStatTagList(localctx.Node)


            self.state = 1200
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class StaticTextFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Flag = 0
            self.N = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_staticTextFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitStaticTextFlagsField" ):
                return visitor.visitStaticTextFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def staticTextFlagsField(self):

        localctx = VfrSyntaxParser.StaticTextFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 122, self.RULE_staticTextFlagsField)
        try:
            self.state = 1204
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1202
                localctx.N = self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1203
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementCrossReferenceContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementGoto(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementGotoContext,0)


        def vfrStatementResetButton(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementResetButtonContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementCrossReference

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementCrossReference" ):
                return visitor.visitVfrStatementCrossReference(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementCrossReference(self):

        localctx = VfrSyntaxParser.VfrStatementCrossReferenceContext(self, self._ctx, self.state)
        self.enterRule(localctx, 124, self.RULE_vfrStatementCrossReference)
        try:
            self.state = 1208
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Goto]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1206
                self.vfrStatementGoto()
                pass
            elif token in [VfrSyntaxParser.ResetButton]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1207
                self.vfrStatementResetButton()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementGotoContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_REF_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_REF
            self.QN = None # Token
            self.N = None # Token
            self.E = None # Token

        def Goto(self):
            return self.getToken(VfrSyntaxParser.Goto, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def vfrGotoFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrGotoFlagsContext,0)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def DevicePath(self):
            return self.getToken(VfrSyntaxParser.DevicePath, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def FormSetGuid(self):
            return self.getToken(VfrSyntaxParser.FormSetGuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def FormId(self):
            return self.getToken(VfrSyntaxParser.FormId, 0)

        def Question(self):
            return self.getToken(VfrSyntaxParser.Question, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementGoto

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementGoto" ):
                return visitor.visitVfrStatementGoto(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementGoto(self):

        localctx = VfrSyntaxParser.VfrStatementGotoContext(self, self._ctx, self.state)
        self.enterRule(localctx, 126, self.RULE_vfrStatementGoto)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1210
            self.match(VfrSyntaxParser.Goto)
            self.state = 1258
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.DevicePath]:
                self.state = 1211
                self.match(VfrSyntaxParser.DevicePath)
                self.state = 1212
                self.match(VfrSyntaxParser.T__5)
                self.state = 1213
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1214
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1215
                self.match(VfrSyntaxParser.Number)
                self.state = 1216
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1217
                self.match(VfrSyntaxParser.Comma)
                self.state = 1218
                self.match(VfrSyntaxParser.FormSetGuid)
                self.state = 1219
                self.match(VfrSyntaxParser.T__5)
                self.state = 1220
                self.guidDefinition()
                self.state = 1221
                self.match(VfrSyntaxParser.Comma)
                self.state = 1222
                self.match(VfrSyntaxParser.FormId)
                self.state = 1223
                self.match(VfrSyntaxParser.T__5)
                self.state = 1224
                self.match(VfrSyntaxParser.Number)
                self.state = 1225
                self.match(VfrSyntaxParser.Comma)
                self.state = 1226
                self.match(VfrSyntaxParser.Question)
                self.state = 1227
                self.match(VfrSyntaxParser.T__5)
                self.state = 1228
                self.match(VfrSyntaxParser.Number)
                self.state = 1229
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.FormSetGuid]:
                self.state = 1231
                self.match(VfrSyntaxParser.FormSetGuid)
                self.state = 1232
                self.match(VfrSyntaxParser.T__5)
                self.state = 1233
                self.guidDefinition()
                self.state = 1234
                self.match(VfrSyntaxParser.Comma)
                self.state = 1235
                self.match(VfrSyntaxParser.FormId)
                self.state = 1236
                self.match(VfrSyntaxParser.T__5)
                self.state = 1237
                self.match(VfrSyntaxParser.Number)
                self.state = 1238
                self.match(VfrSyntaxParser.Comma)
                self.state = 1239
                self.match(VfrSyntaxParser.Question)
                self.state = 1240
                self.match(VfrSyntaxParser.T__5)
                self.state = 1241
                self.match(VfrSyntaxParser.Number)
                self.state = 1242
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.FormId]:
                self.state = 1244
                self.match(VfrSyntaxParser.FormId)
                self.state = 1245
                self.match(VfrSyntaxParser.T__5)
                self.state = 1246
                self.match(VfrSyntaxParser.Number)
                self.state = 1247
                self.match(VfrSyntaxParser.Comma)
                self.state = 1248
                self.match(VfrSyntaxParser.Question)
                self.state = 1249
                self.match(VfrSyntaxParser.T__5)
                self.state = 1254
                self._errHandler.sync(self)
                token = self._input.LA(1)
                if token in [VfrSyntaxParser.StringIdentifier]:
                    self.state = 1250
                    localctx.QN = self.match(VfrSyntaxParser.StringIdentifier)
                    self.state = 1251
                    self.match(VfrSyntaxParser.Comma)
                    pass
                elif token in [VfrSyntaxParser.Number]:
                    self.state = 1252
                    self.match(VfrSyntaxParser.Number)
                    self.state = 1253
                    self.match(VfrSyntaxParser.Comma)
                    pass
                else:
                    raise NoViableAltException(self)

                pass
            elif token in [VfrSyntaxParser.Number]:
                self.state = 1256
                localctx.N = self.match(VfrSyntaxParser.Number)
                self.state = 1257
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.Prompt, VfrSyntaxParser.Name, VfrSyntaxParser.VarId, VfrSyntaxParser.QuestionId]:
                pass
            else:
                pass
            self.state = 1260
            self.vfrQuestionHeader(localctx.Node, localctx.QType)
            self.state = 1265
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,82,self._ctx)
            if la_ == 1:
                self.state = 1261
                self.match(VfrSyntaxParser.Comma)
                self.state = 1262
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1263
                self.match(VfrSyntaxParser.T__5)
                self.state = 1264
                self.vfrGotoFlags()


            self.state = 1271
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,83,self._ctx)
            if la_ == 1:
                self.state = 1267
                self.match(VfrSyntaxParser.Comma)
                self.state = 1268
                self.match(VfrSyntaxParser.Key)
                self.state = 1269
                self.match(VfrSyntaxParser.T__5)
                self.state = 1270
                self.match(VfrSyntaxParser.Number)


            self.state = 1275
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 1273
                localctx.E = self.match(VfrSyntaxParser.Comma)
                self.state = 1274
                self.vfrStatementQuestionOptionList(localctx.Node)


            self.state = 1277
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrGotoFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.GotoFlags = 0

        def gotoFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.GotoFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.GotoFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrGotoFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrGotoFlags" ):
                return visitor.visitVfrGotoFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrGotoFlags(self):

        localctx = VfrSyntaxParser.VfrGotoFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 128, self.RULE_vfrGotoFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1279
            self.gotoFlagsField()
            self.state = 1284
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1280
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1281
                self.gotoFlagsField()
                self.state = 1286
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class GotoFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Flag = 0
            self.N = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_gotoFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitGotoFlagsField" ):
                return visitor.visitGotoFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def gotoFlagsField(self):

        localctx = VfrSyntaxParser.GotoFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 130, self.RULE_gotoFlagsField)
        try:
            self.state = 1289
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1287
                localctx.N = self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1288
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementResetButtonContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_RESET_BUTTON_OP)
            self.N = None # Token

        def ResetButton(self):
            return self.getToken(VfrSyntaxParser.ResetButton, 0)

        def DefaultStore(self):
            return self.getToken(VfrSyntaxParser.DefaultStore, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementHeaderContext,0)


        def EndResetButton(self):
            return self.getToken(VfrSyntaxParser.EndResetButton, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def vfrStatementStatTagList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatTagListContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementResetButton

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementResetButton" ):
                return visitor.visitVfrStatementResetButton(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementResetButton(self):

        localctx = VfrSyntaxParser.VfrStatementResetButtonContext(self, self._ctx, self.state)
        self.enterRule(localctx, 132, self.RULE_vfrStatementResetButton)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1291
            self.match(VfrSyntaxParser.ResetButton)
            self.state = 1292
            self.match(VfrSyntaxParser.DefaultStore)
            self.state = 1293
            self.match(VfrSyntaxParser.T__5)
            self.state = 1294
            localctx.N = self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 1295
            self.match(VfrSyntaxParser.Comma)
            self.state = 1296
            self.vfrStatementHeader(localctx.Node)
            self.state = 1297
            self.match(VfrSyntaxParser.Comma)
            self.state = 1301
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Image or _la==VfrSyntaxParser.Locked:
                self.state = 1298
                self.vfrStatementStatTagList(localctx.Node)
                self.state = 1299
                self.match(VfrSyntaxParser.Comma)


            self.state = 1303
            self.match(VfrSyntaxParser.EndResetButton)
            self.state = 1304
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementQuestionsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementBooleanType(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementBooleanTypeContext,0)


        def vfrStatementDate(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDateContext,0)


        def vfrStatementNumericType(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementNumericTypeContext,0)


        def vfrStatementStringType(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStringTypeContext,0)


        def vfrStatementOrderedList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementOrderedListContext,0)


        def vfrStatementTime(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementTimeContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementQuestions

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementQuestions" ):
                return visitor.visitVfrStatementQuestions(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementQuestions(self):

        localctx = VfrSyntaxParser.VfrStatementQuestionsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 134, self.RULE_vfrStatementQuestions)
        try:
            self.state = 1312
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.CheckBox, VfrSyntaxParser.Action]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1306
                self.vfrStatementBooleanType()
                pass
            elif token in [VfrSyntaxParser.Date]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1307
                self.vfrStatementDate()
                pass
            elif token in [VfrSyntaxParser.OneOf, VfrSyntaxParser.Numeric]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1308
                self.vfrStatementNumericType()
                pass
            elif token in [VfrSyntaxParser.Password, VfrSyntaxParser.String]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1309
                self.vfrStatementStringType()
                pass
            elif token in [VfrSyntaxParser.OrderedList]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1310
                self.vfrStatementOrderedList()
                pass
            elif token in [VfrSyntaxParser.Time]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1311
                self.vfrStatementTime()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementQuestionTagContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementStatTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatTagContext,0)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def vfrStatementInconsistentIf(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInconsistentIfContext,0)


        def vfrStatementNoSubmitIf(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementNoSubmitIfContext,0)


        def vfrStatementDisableIfQuest(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDisableIfQuestContext,0)


        def vfrStatementRefresh(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementRefreshContext,0)


        def vfrStatementVarstoreDevice(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementVarstoreDeviceContext,0)


        def vfrStatementExtension(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExtensionContext,0)


        def vfrStatementRefreshEvent(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementRefreshEventContext,0)


        def vfrStatementWarningIf(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementWarningIfContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementQuestionTag

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementQuestionTag" ):
                return visitor.visitVfrStatementQuestionTag(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementQuestionTag(self):

        localctx = VfrSyntaxParser.VfrStatementQuestionTagContext(self, self._ctx, self.state)
        self.enterRule(localctx, 136, self.RULE_vfrStatementQuestionTag)
        try:
            self.state = 1325
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Image, VfrSyntaxParser.Locked]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1314
                self.vfrStatementStatTag()
                self.state = 1315
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.InconsistentIf]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1317
                self.vfrStatementInconsistentIf()
                pass
            elif token in [VfrSyntaxParser.NoSubmitIf]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1318
                self.vfrStatementNoSubmitIf()
                pass
            elif token in [VfrSyntaxParser.DisableIf]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1319
                self.vfrStatementDisableIfQuest()
                pass
            elif token in [VfrSyntaxParser.Refresh]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1320
                self.vfrStatementRefresh()
                pass
            elif token in [VfrSyntaxParser.VarstoreDevice]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1321
                self.vfrStatementVarstoreDevice()
                pass
            elif token in [VfrSyntaxParser.GuidOp]:
                self.enterOuterAlt(localctx, 7)
                self.state = 1322
                self.vfrStatementExtension()
                pass
            elif token in [VfrSyntaxParser.RefreshGuid]:
                self.enterOuterAlt(localctx, 8)
                self.state = 1323
                self.vfrStatementRefreshEvent()
                pass
            elif token in [VfrSyntaxParser.WarningIf]:
                self.enterOuterAlt(localctx, 9)
                self.state = 1324
                self.vfrStatementWarningIf()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementInconsistentIfContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP)

        def InconsistentIf(self):
            return self.getToken(VfrSyntaxParser.InconsistentIf, 0)

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementInconsistentIf

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementInconsistentIf" ):
                return visitor.visitVfrStatementInconsistentIf(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementInconsistentIf(self):

        localctx = VfrSyntaxParser.VfrStatementInconsistentIfContext(self, self._ctx, self.state)
        self.enterRule(localctx, 138, self.RULE_vfrStatementInconsistentIf)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1327
            self.match(VfrSyntaxParser.InconsistentIf)
            self.state = 1328
            self.match(VfrSyntaxParser.Prompt)
            self.state = 1329
            self.match(VfrSyntaxParser.T__5)
            self.state = 1330
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1331
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1332
            self.match(VfrSyntaxParser.Number)
            self.state = 1333
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1334
            self.match(VfrSyntaxParser.Comma)
            self.state = 1347
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1335
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1336
                self.match(VfrSyntaxParser.T__5)
                self.state = 1337
                self.flagsField()
                self.state = 1342
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 1338
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 1339
                    self.flagsField()
                    self.state = 1344
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 1345
                self.match(VfrSyntaxParser.Comma)


            self.state = 1349
            self.vfrStatementExpression(localctx.Node)
            self.state = 1350
            self.match(VfrSyntaxParser.EndIf)
            self.state = 1352
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,92,self._ctx)
            if la_ == 1:
                self.state = 1351
                self.match(VfrSyntaxParser.Semicolon)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementNoSubmitIfContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_NO_SUBMIT_IF_OP)

        def NoSubmitIf(self):
            return self.getToken(VfrSyntaxParser.NoSubmitIf, 0)

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementNoSubmitIf

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementNoSubmitIf" ):
                return visitor.visitVfrStatementNoSubmitIf(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementNoSubmitIf(self):

        localctx = VfrSyntaxParser.VfrStatementNoSubmitIfContext(self, self._ctx, self.state)
        self.enterRule(localctx, 140, self.RULE_vfrStatementNoSubmitIf)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1354
            self.match(VfrSyntaxParser.NoSubmitIf)
            self.state = 1355
            self.match(VfrSyntaxParser.Prompt)
            self.state = 1356
            self.match(VfrSyntaxParser.T__5)
            self.state = 1357
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1358
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1359
            self.match(VfrSyntaxParser.Number)
            self.state = 1360
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1361
            self.match(VfrSyntaxParser.Comma)
            self.state = 1374
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1362
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1363
                self.match(VfrSyntaxParser.T__5)
                self.state = 1364
                self.flagsField()
                self.state = 1369
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 1365
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 1366
                    self.flagsField()
                    self.state = 1371
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 1372
                self.match(VfrSyntaxParser.Comma)


            self.state = 1376
            self.vfrStatementExpression(localctx.Node)
            self.state = 1377
            self.match(VfrSyntaxParser.EndIf)
            self.state = 1379
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,95,self._ctx)
            if la_ == 1:
                self.state = 1378
                self.match(VfrSyntaxParser.Semicolon)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementDisableIfQuestContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_DISABLE_IF_OP)

        def DisableIf(self):
            return self.getToken(VfrSyntaxParser.DisableIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementDisableIfQuest

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementDisableIfQuest" ):
                return visitor.visitVfrStatementDisableIfQuest(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementDisableIfQuest(self):

        localctx = VfrSyntaxParser.VfrStatementDisableIfQuestContext(self, self._ctx, self.state)
        self.enterRule(localctx, 142, self.RULE_vfrStatementDisableIfQuest)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1381
            self.match(VfrSyntaxParser.DisableIf)
            self.state = 1382
            self.vfrStatementExpression(localctx.Node)
            self.state = 1383
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 1384
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1385
            self.match(VfrSyntaxParser.EndIf)
            self.state = 1387
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,96,self._ctx)
            if la_ == 1:
                self.state = 1386
                self.match(VfrSyntaxParser.Semicolon)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementRefreshContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_REFRESH_OP)

        def Refresh(self):
            return self.getToken(VfrSyntaxParser.Refresh, 0)

        def Interval(self):
            return self.getToken(VfrSyntaxParser.Interval, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementRefresh

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementRefresh" ):
                return visitor.visitVfrStatementRefresh(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementRefresh(self):

        localctx = VfrSyntaxParser.VfrStatementRefreshContext(self, self._ctx, self.state)
        self.enterRule(localctx, 144, self.RULE_vfrStatementRefresh)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1389
            self.match(VfrSyntaxParser.Refresh)
            self.state = 1390
            self.match(VfrSyntaxParser.Interval)
            self.state = 1391
            self.match(VfrSyntaxParser.T__5)
            self.state = 1392
            self.match(VfrSyntaxParser.Number)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementVarstoreDeviceContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_VARSTORE_DEVICE_OP)

        def VarstoreDevice(self):
            return self.getToken(VfrSyntaxParser.VarstoreDevice, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementVarstoreDevice

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementVarstoreDevice" ):
                return visitor.visitVfrStatementVarstoreDevice(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementVarstoreDevice(self):

        localctx = VfrSyntaxParser.VfrStatementVarstoreDeviceContext(self, self._ctx, self.state)
        self.enterRule(localctx, 146, self.RULE_vfrStatementVarstoreDevice)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1394
            self.match(VfrSyntaxParser.VarstoreDevice)
            self.state = 1395
            self.match(VfrSyntaxParser.T__5)
            self.state = 1396
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1397
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1398
            self.match(VfrSyntaxParser.Number)
            self.state = 1399
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1400
            self.match(VfrSyntaxParser.Comma)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementRefreshEventContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_REFRESH_ID_OP)

        def RefreshGuid(self):
            return self.getToken(VfrSyntaxParser.RefreshGuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementRefreshEvent

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementRefreshEvent" ):
                return visitor.visitVfrStatementRefreshEvent(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementRefreshEvent(self):

        localctx = VfrSyntaxParser.VfrStatementRefreshEventContext(self, self._ctx, self.state)
        self.enterRule(localctx, 148, self.RULE_vfrStatementRefreshEvent)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1402
            self.match(VfrSyntaxParser.RefreshGuid)
            self.state = 1403
            self.match(VfrSyntaxParser.T__5)
            self.state = 1404
            self.guidDefinition()
            self.state = 1405
            self.match(VfrSyntaxParser.Comma)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementWarningIfContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_WARNING_IF_OP)

        def WarningIf(self):
            return self.getToken(VfrSyntaxParser.WarningIf, 0)

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def Timeout(self):
            return self.getToken(VfrSyntaxParser.Timeout, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementWarningIf

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementWarningIf" ):
                return visitor.visitVfrStatementWarningIf(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementWarningIf(self):

        localctx = VfrSyntaxParser.VfrStatementWarningIfContext(self, self._ctx, self.state)
        self.enterRule(localctx, 150, self.RULE_vfrStatementWarningIf)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1407
            self.match(VfrSyntaxParser.WarningIf)
            self.state = 1408
            self.match(VfrSyntaxParser.Prompt)
            self.state = 1409
            self.match(VfrSyntaxParser.T__5)
            self.state = 1410
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1411
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1412
            self.match(VfrSyntaxParser.Number)
            self.state = 1413
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1414
            self.match(VfrSyntaxParser.Comma)
            self.state = 1419
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Timeout:
                self.state = 1415
                self.match(VfrSyntaxParser.Timeout)
                self.state = 1416
                self.match(VfrSyntaxParser.T__5)
                self.state = 1417
                self.match(VfrSyntaxParser.Number)
                self.state = 1418
                self.match(VfrSyntaxParser.Comma)


            self.state = 1421
            self.vfrStatementExpression(localctx.Node)
            self.state = 1422
            self.match(VfrSyntaxParser.EndIf)
            self.state = 1424
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,98,self._ctx)
            if la_ == 1:
                self.state = 1423
                self.match(VfrSyntaxParser.Semicolon)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementQuestionTagListContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.Node = Node

        def vfrStatementQuestionTag(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementQuestionTagContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionTagContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementQuestionTagList

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementQuestionTagList" ):
                return visitor.visitVfrStatementQuestionTagList(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementQuestionTagList(self, Node):

        localctx = VfrSyntaxParser.VfrStatementQuestionTagListContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 152, self.RULE_vfrStatementQuestionTagList)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1429
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 106)) & ~0x3f) == 0 and ((1 << (_la - 106)) & ((1 << (VfrSyntaxParser.DisableIf - 106)) | (1 << (VfrSyntaxParser.InconsistentIf - 106)) | (1 << (VfrSyntaxParser.WarningIf - 106)) | (1 << (VfrSyntaxParser.NoSubmitIf - 106)) | (1 << (VfrSyntaxParser.Image - 106)) | (1 << (VfrSyntaxParser.Locked - 106)) | (1 << (VfrSyntaxParser.Refresh - 106)) | (1 << (VfrSyntaxParser.VarstoreDevice - 106)) | (1 << (VfrSyntaxParser.GuidOp - 106)))) != 0) or _la==VfrSyntaxParser.RefreshGuid:
                self.state = 1426
                self.vfrStatementQuestionTag()
                self.state = 1431
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementQuestionOptionTagContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementSuppressIfQuest(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSuppressIfQuestContext,0)


        def vfrStatementGrayOutIfQuest(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementGrayOutIfQuestContext,0)


        def vfrStatementValue(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementValueContext,0)


        def vfrStatementDefault(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDefaultContext,0)


        def vfrStatementOptions(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementOptionsContext,0)


        def vfrStatementRead(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementReadContext,0)


        def vfrStatementWrite(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementWriteContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementQuestionOptionTag

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementQuestionOptionTag" ):
                return visitor.visitVfrStatementQuestionOptionTag(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementQuestionOptionTag(self):

        localctx = VfrSyntaxParser.VfrStatementQuestionOptionTagContext(self, self._ctx, self.state)
        self.enterRule(localctx, 154, self.RULE_vfrStatementQuestionOptionTag)
        try:
            self.state = 1439
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.SuppressIf]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1432
                self.vfrStatementSuppressIfQuest()
                pass
            elif token in [VfrSyntaxParser.GrayOutIf]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1433
                self.vfrStatementGrayOutIfQuest()
                pass
            elif token in [VfrSyntaxParser.Value]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1434
                self.vfrStatementValue()
                pass
            elif token in [VfrSyntaxParser.Default]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1435
                self.vfrStatementDefault()
                pass
            elif token in [VfrSyntaxParser.Option]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1436
                self.vfrStatementOptions()
                pass
            elif token in [VfrSyntaxParser.Read]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1437
                self.vfrStatementRead()
                pass
            elif token in [VfrSyntaxParser.Write]:
                self.enterOuterAlt(localctx, 7)
                self.state = 1438
                self.vfrStatementWrite()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class FlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.N = None # Token
            self.L = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def InteractiveFlag(self):
            return self.getToken(VfrSyntaxParser.InteractiveFlag, 0)

        def ManufacturingFlag(self):
            return self.getToken(VfrSyntaxParser.ManufacturingFlag, 0)

        def DefaultFlag(self):
            return self.getToken(VfrSyntaxParser.DefaultFlag, 0)

        def ResetRequiredFlag(self):
            return self.getToken(VfrSyntaxParser.ResetRequiredFlag, 0)

        def ReconnectRequiredFlag(self):
            return self.getToken(VfrSyntaxParser.ReconnectRequiredFlag, 0)

        def NVAccessFlag(self):
            return self.getToken(VfrSyntaxParser.NVAccessFlag, 0)

        def LateCheckFlag(self):
            return self.getToken(VfrSyntaxParser.LateCheckFlag, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_flagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitFlagsField" ):
                return visitor.visitFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def flagsField(self):

        localctx = VfrSyntaxParser.FlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 156, self.RULE_flagsField)
        try:
            self.state = 1449
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1441
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1442
                self.match(VfrSyntaxParser.InteractiveFlag)
                pass
            elif token in [VfrSyntaxParser.ManufacturingFlag]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1443
                self.match(VfrSyntaxParser.ManufacturingFlag)
                pass
            elif token in [VfrSyntaxParser.DefaultFlag]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1444
                self.match(VfrSyntaxParser.DefaultFlag)
                pass
            elif token in [VfrSyntaxParser.ResetRequiredFlag]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1445
                self.match(VfrSyntaxParser.ResetRequiredFlag)
                pass
            elif token in [VfrSyntaxParser.ReconnectRequiredFlag]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1446
                self.match(VfrSyntaxParser.ReconnectRequiredFlag)
                pass
            elif token in [VfrSyntaxParser.NVAccessFlag]:
                self.enterOuterAlt(localctx, 7)
                self.state = 1447
                localctx.N = self.match(VfrSyntaxParser.NVAccessFlag)
                pass
            elif token in [VfrSyntaxParser.LateCheckFlag]:
                self.enterOuterAlt(localctx, 8)
                self.state = 1448
                localctx.L = self.match(VfrSyntaxParser.LateCheckFlag)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementSuppressIfQuestContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)

        def SuppressIf(self):
            return self.getToken(VfrSyntaxParser.SuppressIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementSuppressIfQuest

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementSuppressIfQuest" ):
                return visitor.visitVfrStatementSuppressIfQuest(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementSuppressIfQuest(self):

        localctx = VfrSyntaxParser.VfrStatementSuppressIfQuestContext(self, self._ctx, self.state)
        self.enterRule(localctx, 158, self.RULE_vfrStatementSuppressIfQuest)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1451
            self.match(VfrSyntaxParser.SuppressIf)
            self.state = 1452
            self.vfrStatementExpression(localctx.Node)
            self.state = 1453
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 1466
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1454
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1455
                self.match(VfrSyntaxParser.T__5)
                self.state = 1456
                self.flagsField()
                self.state = 1461
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 1457
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 1458
                    self.flagsField()
                    self.state = 1463
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 1464
                self.match(VfrSyntaxParser.Comma)


            self.state = 1468
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1469
            self.match(VfrSyntaxParser.EndIf)
            self.state = 1471
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,104,self._ctx)
            if la_ == 1:
                self.state = 1470
                self.match(VfrSyntaxParser.Semicolon)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementGrayOutIfQuestContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)

        def GrayOutIf(self):
            return self.getToken(VfrSyntaxParser.GrayOutIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementGrayOutIfQuest

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementGrayOutIfQuest" ):
                return visitor.visitVfrStatementGrayOutIfQuest(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementGrayOutIfQuest(self):

        localctx = VfrSyntaxParser.VfrStatementGrayOutIfQuestContext(self, self._ctx, self.state)
        self.enterRule(localctx, 160, self.RULE_vfrStatementGrayOutIfQuest)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1473
            self.match(VfrSyntaxParser.GrayOutIf)
            self.state = 1474
            self.vfrStatementExpression(localctx.Node)
            self.state = 1475
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 1488
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1476
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1477
                self.match(VfrSyntaxParser.T__5)
                self.state = 1478
                self.flagsField()
                self.state = 1483
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 1479
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 1480
                    self.flagsField()
                    self.state = 1485
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 1486
                self.match(VfrSyntaxParser.Comma)


            self.state = 1490
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1491
            self.match(VfrSyntaxParser.EndIf)
            self.state = 1493
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,107,self._ctx)
            if la_ == 1:
                self.state = 1492
                self.match(VfrSyntaxParser.Semicolon)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementDefaultContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_DEFAULT_OP)
            self.D = None # Token
            self.V = None # VfrStatementValueContext
            self.SN = None # Token

        def Default(self):
            return self.getToken(VfrSyntaxParser.Default, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrConstantValueField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrConstantValueFieldContext,0)


        def vfrStatementValue(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementValueContext,0)


        def DefaultStore(self):
            return self.getToken(VfrSyntaxParser.DefaultStore, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementDefault

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementDefault" ):
                return visitor.visitVfrStatementDefault(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementDefault(self):

        localctx = VfrSyntaxParser.VfrStatementDefaultContext(self, self._ctx, self.state)
        self.enterRule(localctx, 162, self.RULE_vfrStatementDefault)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1495
            localctx.D = self.match(VfrSyntaxParser.Default)

            self.state = 1503
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Value]:
                self.state = 1496
                localctx.V = self.vfrStatementValue()
                self.state = 1497
                self.match(VfrSyntaxParser.Comma)
                pass
            elif token in [VfrSyntaxParser.T__5]:
                self.state = 1499
                self.match(VfrSyntaxParser.T__5)
                self.state = 1500
                self.vfrConstantValueField()
                self.state = 1501
                self.match(VfrSyntaxParser.Comma)
                pass
            else:
                raise NoViableAltException(self)

            self.state = 1509
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.DefaultStore:
                self.state = 1505
                self.match(VfrSyntaxParser.DefaultStore)
                self.state = 1506
                self.match(VfrSyntaxParser.T__5)
                self.state = 1507
                localctx.SN = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1508
                self.match(VfrSyntaxParser.Comma)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementValueContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_VALUE_OP)

        def Value(self):
            return self.getToken(VfrSyntaxParser.Value, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementValue

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementValue" ):
                return visitor.visitVfrStatementValue(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementValue(self):

        localctx = VfrSyntaxParser.VfrStatementValueContext(self, self._ctx, self.state)
        self.enterRule(localctx, 164, self.RULE_vfrStatementValue)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1511
            self.match(VfrSyntaxParser.Value)
            self.state = 1512
            self.match(VfrSyntaxParser.T__5)
            self.state = 1513
            self.vfrStatementExpression(localctx.Node)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementOptionsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementOneOfOption(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementOneOfOptionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementOptions

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementOptions" ):
                return visitor.visitVfrStatementOptions(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementOptions(self):

        localctx = VfrSyntaxParser.VfrStatementOptionsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 166, self.RULE_vfrStatementOptions)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1515
            self.vfrStatementOneOfOption()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementOneOfOptionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_ONE_OF_OPTION_OP)
            self.F = None # Token
            self.KN = None # Token
            self.T = None # Token

        def Option(self):
            return self.getToken(VfrSyntaxParser.Option, 0)

        def Text(self):
            return self.getToken(VfrSyntaxParser.Text, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Value(self):
            return self.getToken(VfrSyntaxParser.Value, 0)

        def vfrConstantValueField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrConstantValueFieldContext,0)


        def vfrOneOfOptionFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrOneOfOptionFlagsContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def vfrImageTag(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrImageTagContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrImageTagContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementOneOfOption

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementOneOfOption" ):
                return visitor.visitVfrStatementOneOfOption(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementOneOfOption(self):

        localctx = VfrSyntaxParser.VfrStatementOneOfOptionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 168, self.RULE_vfrStatementOneOfOption)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1517
            self.match(VfrSyntaxParser.Option)
            self.state = 1518
            self.match(VfrSyntaxParser.Text)
            self.state = 1519
            self.match(VfrSyntaxParser.T__5)
            self.state = 1520
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1521
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1522
            self.match(VfrSyntaxParser.Number)
            self.state = 1523
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1524
            self.match(VfrSyntaxParser.Comma)
            self.state = 1525
            self.match(VfrSyntaxParser.Value)
            self.state = 1526
            self.match(VfrSyntaxParser.T__5)
            self.state = 1527
            self.vfrConstantValueField()
            self.state = 1528
            self.match(VfrSyntaxParser.Comma)
            self.state = 1529
            localctx.F = self.match(VfrSyntaxParser.FLAGS)
            self.state = 1530
            self.match(VfrSyntaxParser.T__5)
            self.state = 1531
            self.vfrOneOfOptionFlags()
            self.state = 1536
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,110,self._ctx)
            if la_ == 1:
                self.state = 1532
                self.match(VfrSyntaxParser.Comma)
                self.state = 1533
                self.match(VfrSyntaxParser.Key)
                self.state = 1534
                self.match(VfrSyntaxParser.T__5)
                self.state = 1535
                localctx.KN = self.match(VfrSyntaxParser.Number)


            self.state = 1542
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.Comma:
                self.state = 1538
                localctx.T = self.match(VfrSyntaxParser.Comma)
                self.state = 1539
                self.vfrImageTag()
                self.state = 1544
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 1545
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrOneOfOptionFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0
            self.LFlags = 0

        def oneofoptionFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.OneofoptionFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.OneofoptionFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrOneOfOptionFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrOneOfOptionFlags" ):
                return visitor.visitVfrOneOfOptionFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrOneOfOptionFlags(self):

        localctx = VfrSyntaxParser.VfrOneOfOptionFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 170, self.RULE_vfrOneOfOptionFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1547
            self.oneofoptionFlagsField()
            self.state = 1552
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1548
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1549
                self.oneofoptionFlagsField()
                self.state = 1554
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class OneofoptionFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlag = 0
            self.LFlag = 0
            self.A = None # Token
            self.L = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def OptionDefault(self):
            return self.getToken(VfrSyntaxParser.OptionDefault, 0)

        def OptionDefaultMfg(self):
            return self.getToken(VfrSyntaxParser.OptionDefaultMfg, 0)

        def InteractiveFlag(self):
            return self.getToken(VfrSyntaxParser.InteractiveFlag, 0)

        def ResetRequiredFlag(self):
            return self.getToken(VfrSyntaxParser.ResetRequiredFlag, 0)

        def RestStyleFlag(self):
            return self.getToken(VfrSyntaxParser.RestStyleFlag, 0)

        def ReconnectRequiredFlag(self):
            return self.getToken(VfrSyntaxParser.ReconnectRequiredFlag, 0)

        def ManufacturingFlag(self):
            return self.getToken(VfrSyntaxParser.ManufacturingFlag, 0)

        def DefaultFlag(self):
            return self.getToken(VfrSyntaxParser.DefaultFlag, 0)

        def NVAccessFlag(self):
            return self.getToken(VfrSyntaxParser.NVAccessFlag, 0)

        def LateCheckFlag(self):
            return self.getToken(VfrSyntaxParser.LateCheckFlag, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_oneofoptionFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitOneofoptionFlagsField" ):
                return visitor.visitOneofoptionFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def oneofoptionFlagsField(self):

        localctx = VfrSyntaxParser.OneofoptionFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 172, self.RULE_oneofoptionFlagsField)
        try:
            self.state = 1566
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1555
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.OptionDefault]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1556
                self.match(VfrSyntaxParser.OptionDefault)
                pass
            elif token in [VfrSyntaxParser.OptionDefaultMfg]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1557
                self.match(VfrSyntaxParser.OptionDefaultMfg)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1558
                self.match(VfrSyntaxParser.InteractiveFlag)
                pass
            elif token in [VfrSyntaxParser.ResetRequiredFlag]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1559
                self.match(VfrSyntaxParser.ResetRequiredFlag)
                pass
            elif token in [VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1560
                self.match(VfrSyntaxParser.RestStyleFlag)
                pass
            elif token in [VfrSyntaxParser.ReconnectRequiredFlag]:
                self.enterOuterAlt(localctx, 7)
                self.state = 1561
                self.match(VfrSyntaxParser.ReconnectRequiredFlag)
                pass
            elif token in [VfrSyntaxParser.ManufacturingFlag]:
                self.enterOuterAlt(localctx, 8)
                self.state = 1562
                self.match(VfrSyntaxParser.ManufacturingFlag)
                pass
            elif token in [VfrSyntaxParser.DefaultFlag]:
                self.enterOuterAlt(localctx, 9)
                self.state = 1563
                self.match(VfrSyntaxParser.DefaultFlag)
                pass
            elif token in [VfrSyntaxParser.NVAccessFlag]:
                self.enterOuterAlt(localctx, 10)
                self.state = 1564
                localctx.A = self.match(VfrSyntaxParser.NVAccessFlag)
                pass
            elif token in [VfrSyntaxParser.LateCheckFlag]:
                self.enterOuterAlt(localctx, 11)
                self.state = 1565
                localctx.L = self.match(VfrSyntaxParser.LateCheckFlag)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementReadContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_READ_OP)

        def Read(self):
            return self.getToken(VfrSyntaxParser.Read, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementRead

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementRead" ):
                return visitor.visitVfrStatementRead(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementRead(self):

        localctx = VfrSyntaxParser.VfrStatementReadContext(self, self._ctx, self.state)
        self.enterRule(localctx, 174, self.RULE_vfrStatementRead)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1568
            self.match(VfrSyntaxParser.Read)
            self.state = 1569
            self.vfrStatementExpression(localctx.Node)
            self.state = 1570
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementWriteContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_WRITE_OP)

        def Write(self):
            return self.getToken(VfrSyntaxParser.Write, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementWrite

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementWrite" ):
                return visitor.visitVfrStatementWrite(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementWrite(self):

        localctx = VfrSyntaxParser.VfrStatementWriteContext(self, self._ctx, self.state)
        self.enterRule(localctx, 176, self.RULE_vfrStatementWrite)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1572
            self.match(VfrSyntaxParser.Write)
            self.state = 1573
            self.vfrStatementExpression(localctx.Node)
            self.state = 1574
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementQuestionOptionListContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.Node = Node

        def vfrStatementQuestionOption(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementQuestionOptionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementQuestionOptionList

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementQuestionOptionList" ):
                return visitor.visitVfrStatementQuestionOptionList(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementQuestionOptionList(self, Node):

        localctx = VfrSyntaxParser.VfrStatementQuestionOptionListContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 178, self.RULE_vfrStatementQuestionOptionList)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1579
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 61)) & ~0x3f) == 0 and ((1 << (_la - 61)) & ((1 << (VfrSyntaxParser.Option - 61)) | (1 << (VfrSyntaxParser.GrayOutIf - 61)) | (1 << (VfrSyntaxParser.Default - 61)) | (1 << (VfrSyntaxParser.SuppressIf - 61)) | (1 << (VfrSyntaxParser.DisableIf - 61)) | (1 << (VfrSyntaxParser.InconsistentIf - 61)) | (1 << (VfrSyntaxParser.WarningIf - 61)) | (1 << (VfrSyntaxParser.NoSubmitIf - 61)))) != 0) or ((((_la - 144)) & ~0x3f) == 0 and ((1 << (_la - 144)) & ((1 << (VfrSyntaxParser.Image - 144)) | (1 << (VfrSyntaxParser.Locked - 144)) | (1 << (VfrSyntaxParser.Value - 144)) | (1 << (VfrSyntaxParser.Read - 144)) | (1 << (VfrSyntaxParser.Write - 144)) | (1 << (VfrSyntaxParser.Refresh - 144)) | (1 << (VfrSyntaxParser.VarstoreDevice - 144)) | (1 << (VfrSyntaxParser.GuidOp - 144)))) != 0) or _la==VfrSyntaxParser.RefreshGuid:
                self.state = 1576
                self.vfrStatementQuestionOption()
                self.state = 1581
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementQuestionOptionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementQuestionTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionTagContext,0)


        def vfrStatementQuestionOptionTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionTagContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementQuestionOption

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementQuestionOption" ):
                return visitor.visitVfrStatementQuestionOption(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementQuestionOption(self):

        localctx = VfrSyntaxParser.VfrStatementQuestionOptionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 180, self.RULE_vfrStatementQuestionOption)
        try:
            self.state = 1584
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.DisableIf, VfrSyntaxParser.InconsistentIf, VfrSyntaxParser.WarningIf, VfrSyntaxParser.NoSubmitIf, VfrSyntaxParser.Image, VfrSyntaxParser.Locked, VfrSyntaxParser.Refresh, VfrSyntaxParser.VarstoreDevice, VfrSyntaxParser.GuidOp, VfrSyntaxParser.RefreshGuid]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1582
                self.vfrStatementQuestionTag()
                pass
            elif token in [VfrSyntaxParser.Option, VfrSyntaxParser.GrayOutIf, VfrSyntaxParser.Default, VfrSyntaxParser.SuppressIf, VfrSyntaxParser.Value, VfrSyntaxParser.Read, VfrSyntaxParser.Write]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1583
                self.vfrStatementQuestionOptionTag()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementBooleanTypeContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementCheckBox(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementCheckBoxContext,0)


        def vfrStatementAction(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementActionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementBooleanType

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementBooleanType" ):
                return visitor.visitVfrStatementBooleanType(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementBooleanType(self):

        localctx = VfrSyntaxParser.VfrStatementBooleanTypeContext(self, self._ctx, self.state)
        self.enterRule(localctx, 182, self.RULE_vfrStatementBooleanType)
        try:
            self.state = 1588
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.CheckBox]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1586
                self.vfrStatementCheckBox()
                pass
            elif token in [VfrSyntaxParser.Action]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1587
                self.vfrStatementAction()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementCheckBoxContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_CHECKBOX_OP)
            self.GuidNode = VfrTreeNode(EFI_IFR_GUID_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL
            self.L = None # Token
            self.F = None # Token

        def vfrQuestionBaseInfo(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionBaseInfoContext,0)


        def vfrStatementHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndCheckBox(self):
            return self.getToken(VfrSyntaxParser.EndCheckBox, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def CheckBox(self):
            return self.getToken(VfrSyntaxParser.CheckBox, 0)

        def vfrCheckBoxFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrCheckBoxFlagsContext,0)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementCheckBox

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementCheckBox" ):
                return visitor.visitVfrStatementCheckBox(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementCheckBox(self):

        localctx = VfrSyntaxParser.VfrStatementCheckBoxContext(self, self._ctx, self.state)
        self.enterRule(localctx, 184, self.RULE_vfrStatementCheckBox)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1590
            localctx.L = self.match(VfrSyntaxParser.CheckBox)
            self.state = 1591
            self.vfrQuestionBaseInfo(localctx.Node, localctx.QType)
            self.state = 1592
            self.vfrStatementHeader(localctx.Node)
            self.state = 1593
            self.match(VfrSyntaxParser.Comma)
            self.state = 1599
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1594
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1595
                self.match(VfrSyntaxParser.T__5)
                self.state = 1596
                self.vfrCheckBoxFlags()
                self.state = 1597
                self.match(VfrSyntaxParser.Comma)


            self.state = 1605
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Key:
                self.state = 1601
                self.match(VfrSyntaxParser.Key)
                self.state = 1602
                self.match(VfrSyntaxParser.T__5)
                self.state = 1603
                self.match(VfrSyntaxParser.Number)
                self.state = 1604
                self.match(VfrSyntaxParser.Comma)


            self.state = 1607
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1608
            self.match(VfrSyntaxParser.EndCheckBox)
            self.state = 1609
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrCheckBoxFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.LFlags = 0
            self.HFlags = 0

        def checkboxFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.CheckboxFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.CheckboxFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrCheckBoxFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrCheckBoxFlags" ):
                return visitor.visitVfrCheckBoxFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrCheckBoxFlags(self):

        localctx = VfrSyntaxParser.VfrCheckBoxFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 186, self.RULE_vfrCheckBoxFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1611
            self.checkboxFlagsField()
            self.state = 1616
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1612
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1613
                self.checkboxFlagsField()
                self.state = 1618
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class CheckboxFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.LFlag = 0
            self.HFlag = 0
            self.D = None # Token
            self.M = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def DefaultFlag(self):
            return self.getToken(VfrSyntaxParser.DefaultFlag, 0)

        def ManufacturingFlag(self):
            return self.getToken(VfrSyntaxParser.ManufacturingFlag, 0)

        def CheckBoxDefaultFlag(self):
            return self.getToken(VfrSyntaxParser.CheckBoxDefaultFlag, 0)

        def CheckBoxDefaultMfgFlag(self):
            return self.getToken(VfrSyntaxParser.CheckBoxDefaultMfgFlag, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_checkboxFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCheckboxFlagsField" ):
                return visitor.visitCheckboxFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def checkboxFlagsField(self):

        localctx = VfrSyntaxParser.CheckboxFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 188, self.RULE_checkboxFlagsField)
        try:
            self.state = 1625
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1619
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.DefaultFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1620
                localctx.D = self.match(VfrSyntaxParser.DefaultFlag)
                pass
            elif token in [VfrSyntaxParser.ManufacturingFlag]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1621
                localctx.M = self.match(VfrSyntaxParser.ManufacturingFlag)
                pass
            elif token in [VfrSyntaxParser.CheckBoxDefaultFlag]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1622
                self.match(VfrSyntaxParser.CheckBoxDefaultFlag)
                pass
            elif token in [VfrSyntaxParser.CheckBoxDefaultMfgFlag]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1623
                self.match(VfrSyntaxParser.CheckBoxDefaultMfgFlag)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1624
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementActionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_ACTION_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL

        def Action(self):
            return self.getToken(VfrSyntaxParser.Action, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Config(self):
            return self.getToken(VfrSyntaxParser.Config, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def vfrStatementQuestionTagList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionTagListContext,0)


        def EndAction(self):
            return self.getToken(VfrSyntaxParser.EndAction, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def vfrActionFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrActionFlagsContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementAction

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementAction" ):
                return visitor.visitVfrStatementAction(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementAction(self):

        localctx = VfrSyntaxParser.VfrStatementActionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 190, self.RULE_vfrStatementAction)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1627
            self.match(VfrSyntaxParser.Action)
            self.state = 1628
            self.vfrQuestionHeader(localctx.Node, localctx.QType)
            self.state = 1629
            self.match(VfrSyntaxParser.Comma)
            self.state = 1635
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1630
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 1631
                self.match(VfrSyntaxParser.T__5)
                self.state = 1632
                self.vfrActionFlags()
                self.state = 1633
                self.match(VfrSyntaxParser.Comma)


            self.state = 1637
            self.match(VfrSyntaxParser.Config)
            self.state = 1638
            self.match(VfrSyntaxParser.T__5)
            self.state = 1639
            self.match(VfrSyntaxParser.StringToken)
            self.state = 1640
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 1641
            self.match(VfrSyntaxParser.Number)
            self.state = 1642
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 1643
            self.match(VfrSyntaxParser.Comma)
            self.state = 1644
            self.vfrStatementQuestionTagList(localctx.Node)
            self.state = 1645
            self.match(VfrSyntaxParser.EndAction)
            self.state = 1646
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrActionFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0

        def actionFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.ActionFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.ActionFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrActionFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrActionFlags" ):
                return visitor.visitVfrActionFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrActionFlags(self):

        localctx = VfrSyntaxParser.VfrActionFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 192, self.RULE_vfrActionFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1648
            self.actionFlagsField()
            self.state = 1653
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1649
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1650
                self.actionFlagsField()
                self.state = 1655
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ActionFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlag = 0
            self.N = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_actionFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitActionFlagsField" ):
                return visitor.visitActionFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def actionFlagsField(self):

        localctx = VfrSyntaxParser.ActionFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 194, self.RULE_actionFlagsField)
        try:
            self.state = 1658
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1656
                localctx.N = self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1657
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementNumericTypeContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementNumeric(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementNumericContext,0)


        def vfrStatementOneOf(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementOneOfContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementNumericType

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementNumericType" ):
                return visitor.visitVfrStatementNumericType(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementNumericType(self):

        localctx = VfrSyntaxParser.VfrStatementNumericTypeContext(self, self._ctx, self.state)
        self.enterRule(localctx, 196, self.RULE_vfrStatementNumericType)
        try:
            self.state = 1662
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Numeric]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1660
                self.vfrStatementNumeric()
                pass
            elif token in [VfrSyntaxParser.OneOf]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1661
                self.vfrStatementOneOf()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementNumericContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_NUMERIC_OP)
            self.GuidNode = VfrTreeNode(EFI_IFR_GUID_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL
            self.F = None # Token

        def Numeric(self):
            return self.getToken(VfrSyntaxParser.Numeric, 0)

        def vfrQuestionBaseInfo(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionBaseInfoContext,0)


        def vfrStatementHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrSetMinMaxStep(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrSetMinMaxStepContext,0)


        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndNumeric(self):
            return self.getToken(VfrSyntaxParser.EndNumeric, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def vfrNumericFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrNumericFlagsContext,0)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementNumeric

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementNumeric" ):
                return visitor.visitVfrStatementNumeric(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementNumeric(self):

        localctx = VfrSyntaxParser.VfrStatementNumericContext(self, self._ctx, self.state)
        self.enterRule(localctx, 198, self.RULE_vfrStatementNumeric)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1664
            self.match(VfrSyntaxParser.Numeric)
            self.state = 1665
            self.vfrQuestionBaseInfo(localctx.Node, localctx.QType)
            self.state = 1666
            self.vfrStatementHeader(localctx.Node)
            self.state = 1667
            self.match(VfrSyntaxParser.Comma)
            self.state = 1673
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1668
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1669
                self.match(VfrSyntaxParser.T__5)
                self.state = 1670
                self.vfrNumericFlags()
                self.state = 1671
                self.match(VfrSyntaxParser.Comma)


            self.state = 1679
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Key:
                self.state = 1675
                self.match(VfrSyntaxParser.Key)
                self.state = 1676
                self.match(VfrSyntaxParser.T__5)
                self.state = 1677
                self.match(VfrSyntaxParser.Number)
                self.state = 1678
                self.match(VfrSyntaxParser.Comma)


            self.state = 1681
            self.vfrSetMinMaxStep(localctx.Node)
            self.state = 1682
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1683
            self.match(VfrSyntaxParser.EndNumeric)
            self.state = 1684
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrSetMinMaxStepContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Node=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None
            self.N1 = None # Token
            self.I = None # Token
            self.N2 = None # Token
            self.A = None # Token
            self.S = None # Token
            self.Node = Node

        def Minimum(self):
            return self.getToken(VfrSyntaxParser.Minimum, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Maximum(self):
            return self.getToken(VfrSyntaxParser.Maximum, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Step(self):
            return self.getToken(VfrSyntaxParser.Step, 0)

        def Negative(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Negative)
            else:
                return self.getToken(VfrSyntaxParser.Negative, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrSetMinMaxStep

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrSetMinMaxStep" ):
                return visitor.visitVfrSetMinMaxStep(self)
            else:
                return visitor.visitChildren(self)




    def vfrSetMinMaxStep(self, Node):

        localctx = VfrSyntaxParser.VfrSetMinMaxStepContext(self, self._ctx, self.state, Node)
        self.enterRule(localctx, 200, self.RULE_vfrSetMinMaxStep)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1686
            self.match(VfrSyntaxParser.Minimum)
            self.state = 1687
            self.match(VfrSyntaxParser.T__5)
            self.state = 1689
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Negative:
                self.state = 1688
                localctx.N1 = self.match(VfrSyntaxParser.Negative)


            self.state = 1691
            localctx.I = self.match(VfrSyntaxParser.Number)
            self.state = 1692
            self.match(VfrSyntaxParser.Comma)
            self.state = 1693
            self.match(VfrSyntaxParser.Maximum)
            self.state = 1694
            self.match(VfrSyntaxParser.T__5)
            self.state = 1696
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Negative:
                self.state = 1695
                localctx.N2 = self.match(VfrSyntaxParser.Negative)


            self.state = 1698
            localctx.A = self.match(VfrSyntaxParser.Number)
            self.state = 1699
            self.match(VfrSyntaxParser.Comma)
            self.state = 1704
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Step:
                self.state = 1700
                self.match(VfrSyntaxParser.Step)
                self.state = 1701
                self.match(VfrSyntaxParser.T__5)
                self.state = 1702
                localctx.S = self.match(VfrSyntaxParser.Number)
                self.state = 1703
                self.match(VfrSyntaxParser.Comma)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrNumericFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0
            self.LFlags = 0
            self.IsDisplaySpecified = False
            self.UpdateVarType = False

        def numericFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.NumericFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.NumericFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrNumericFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrNumericFlags" ):
                return visitor.visitVfrNumericFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrNumericFlags(self):

        localctx = VfrSyntaxParser.VfrNumericFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 202, self.RULE_vfrNumericFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1706
            self.numericFlagsField()
            self.state = 1711
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1707
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1708
                self.numericFlagsField()
                self.state = 1713
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class NumericFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlag = 0
            self.IsSetType = False
            self.IsDisplaySpecified = False
            self.N = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def NumericSizeOne(self):
            return self.getToken(VfrSyntaxParser.NumericSizeOne, 0)

        def NumericSizeTwo(self):
            return self.getToken(VfrSyntaxParser.NumericSizeTwo, 0)

        def NumericSizeFour(self):
            return self.getToken(VfrSyntaxParser.NumericSizeFour, 0)

        def NumericSizeEight(self):
            return self.getToken(VfrSyntaxParser.NumericSizeEight, 0)

        def DisPlayIntDec(self):
            return self.getToken(VfrSyntaxParser.DisPlayIntDec, 0)

        def DisPlayUIntDec(self):
            return self.getToken(VfrSyntaxParser.DisPlayUIntDec, 0)

        def DisPlayUIntHex(self):
            return self.getToken(VfrSyntaxParser.DisPlayUIntHex, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_numericFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitNumericFlagsField" ):
                return visitor.visitNumericFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def numericFlagsField(self):

        localctx = VfrSyntaxParser.NumericFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 204, self.RULE_numericFlagsField)
        try:
            self.state = 1723
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1714
                localctx.N = self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.NumericSizeOne]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1715
                self.match(VfrSyntaxParser.NumericSizeOne)
                pass
            elif token in [VfrSyntaxParser.NumericSizeTwo]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1716
                self.match(VfrSyntaxParser.NumericSizeTwo)
                pass
            elif token in [VfrSyntaxParser.NumericSizeFour]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1717
                self.match(VfrSyntaxParser.NumericSizeFour)
                pass
            elif token in [VfrSyntaxParser.NumericSizeEight]:
                self.enterOuterAlt(localctx, 5)
                self.state = 1718
                self.match(VfrSyntaxParser.NumericSizeEight)
                pass
            elif token in [VfrSyntaxParser.DisPlayIntDec]:
                self.enterOuterAlt(localctx, 6)
                self.state = 1719
                self.match(VfrSyntaxParser.DisPlayIntDec)
                pass
            elif token in [VfrSyntaxParser.DisPlayUIntDec]:
                self.enterOuterAlt(localctx, 7)
                self.state = 1720
                self.match(VfrSyntaxParser.DisPlayUIntDec)
                pass
            elif token in [VfrSyntaxParser.DisPlayUIntHex]:
                self.enterOuterAlt(localctx, 8)
                self.state = 1721
                self.match(VfrSyntaxParser.DisPlayUIntHex)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 9)
                self.state = 1722
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementOneOfContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_ONE_OF_OP)
            self.GuidNode = VfrTreeNode(EFI_IFR_GUID_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL
            self.F = None # Token

        def OneOf(self):
            return self.getToken(VfrSyntaxParser.OneOf, 0)

        def vfrQuestionBaseInfo(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionBaseInfoContext,0)


        def vfrStatementHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndOneOf(self):
            return self.getToken(VfrSyntaxParser.EndOneOf, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def vfrOneofFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrOneofFlagsFieldContext,0)


        def vfrSetMinMaxStep(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrSetMinMaxStepContext,0)


        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementOneOf

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementOneOf" ):
                return visitor.visitVfrStatementOneOf(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementOneOf(self):

        localctx = VfrSyntaxParser.VfrStatementOneOfContext(self, self._ctx, self.state)
        self.enterRule(localctx, 206, self.RULE_vfrStatementOneOf)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1725
            self.match(VfrSyntaxParser.OneOf)
            self.state = 1726
            self.vfrQuestionBaseInfo(localctx.Node, localctx.QType)
            self.state = 1727
            self.vfrStatementHeader(localctx.Node)
            self.state = 1728
            self.match(VfrSyntaxParser.Comma)
            self.state = 1734
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1729
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1730
                self.match(VfrSyntaxParser.T__5)
                self.state = 1731
                self.vfrOneofFlagsField()
                self.state = 1732
                self.match(VfrSyntaxParser.Comma)


            self.state = 1737
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Minimum:
                self.state = 1736
                self.vfrSetMinMaxStep(localctx.Node)


            self.state = 1739
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1740
            self.match(VfrSyntaxParser.EndOneOf)
            self.state = 1741
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrOneofFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0
            self.LFlags = 0
            self.UpdateVarType = False

        def numericFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.NumericFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.NumericFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrOneofFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrOneofFlagsField" ):
                return visitor.visitVfrOneofFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def vfrOneofFlagsField(self):

        localctx = VfrSyntaxParser.VfrOneofFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 208, self.RULE_vfrOneofFlagsField)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1743
            self.numericFlagsField()
            self.state = 1748
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1744
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1745
                self.numericFlagsField()
                self.state = 1750
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStringTypeContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementString(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStringContext,0)


        def vfrStatementPassword(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementPasswordContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStringType

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStringType" ):
                return visitor.visitVfrStatementStringType(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStringType(self):

        localctx = VfrSyntaxParser.VfrStatementStringTypeContext(self, self._ctx, self.state)
        self.enterRule(localctx, 210, self.RULE_vfrStatementStringType)
        try:
            self.state = 1753
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.String]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1751
                self.vfrStatementString()
                pass
            elif token in [VfrSyntaxParser.Password]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1752
                self.vfrStatementPassword()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStringContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_STRING_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL
            self.F = None # Token
            self.Min = None # Token
            self.Max = None # Token

        def String(self):
            return self.getToken(VfrSyntaxParser.String, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndString(self):
            return self.getToken(VfrSyntaxParser.EndString, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def MinSize(self):
            return self.getToken(VfrSyntaxParser.MinSize, 0)

        def MaxSize(self):
            return self.getToken(VfrSyntaxParser.MaxSize, 0)

        def vfrStringFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStringFlagsFieldContext,0)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementString

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementString" ):
                return visitor.visitVfrStatementString(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementString(self):

        localctx = VfrSyntaxParser.VfrStatementStringContext(self, self._ctx, self.state)
        self.enterRule(localctx, 212, self.RULE_vfrStatementString)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1755
            self.match(VfrSyntaxParser.String)
            self.state = 1756
            self.vfrQuestionHeader(localctx.Node, localctx.QType)
            self.state = 1757
            self.match(VfrSyntaxParser.Comma)
            self.state = 1763
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1758
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1759
                self.match(VfrSyntaxParser.T__5)
                self.state = 1760
                self.vfrStringFlagsField()
                self.state = 1761
                self.match(VfrSyntaxParser.Comma)


            self.state = 1769
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Key:
                self.state = 1765
                self.match(VfrSyntaxParser.Key)
                self.state = 1766
                self.match(VfrSyntaxParser.T__5)
                self.state = 1767
                self.match(VfrSyntaxParser.Number)
                self.state = 1768
                self.match(VfrSyntaxParser.Comma)


            self.state = 1771
            localctx.Min = self.match(VfrSyntaxParser.MinSize)
            self.state = 1772
            self.match(VfrSyntaxParser.T__5)
            self.state = 1773
            self.match(VfrSyntaxParser.Number)
            self.state = 1774
            self.match(VfrSyntaxParser.Comma)
            self.state = 1775
            localctx.Max = self.match(VfrSyntaxParser.MaxSize)
            self.state = 1776
            self.match(VfrSyntaxParser.T__5)
            self.state = 1777
            self.match(VfrSyntaxParser.Number)
            self.state = 1778
            self.match(VfrSyntaxParser.Comma)
            self.state = 1779
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1780
            self.match(VfrSyntaxParser.EndString)
            self.state = 1781
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStringFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0
            self.LFlags = 0

        def stringFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.StringFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.StringFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStringFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStringFlagsField" ):
                return visitor.visitVfrStringFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def vfrStringFlagsField(self):

        localctx = VfrSyntaxParser.VfrStringFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 214, self.RULE_vfrStringFlagsField)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1783
            self.stringFlagsField()
            self.state = 1788
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1784
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1785
                self.stringFlagsField()
                self.state = 1790
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class StringFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlag = 0
            self.LFlag = 0
            self.N = None # Token

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_stringFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitStringFlagsField" ):
                return visitor.visitStringFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def stringFlagsField(self):

        localctx = VfrSyntaxParser.StringFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 216, self.RULE_stringFlagsField)
        try:
            self.state = 1794
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1791
                localctx.N = self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.T__8]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1792
                self.match(VfrSyntaxParser.T__8)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1793
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementPasswordContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_PASSWORD_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL
            self.F = None # Token
            self.Min = None # Token
            self.Max = None # Token

        def Password(self):
            return self.getToken(VfrSyntaxParser.Password, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndPassword(self):
            return self.getToken(VfrSyntaxParser.EndPassword, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def MinSize(self):
            return self.getToken(VfrSyntaxParser.MinSize, 0)

        def MaxSize(self):
            return self.getToken(VfrSyntaxParser.MaxSize, 0)

        def vfrPasswordFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrPasswordFlagsFieldContext,0)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def Encoding(self):
            return self.getToken(VfrSyntaxParser.Encoding, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementPassword

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementPassword" ):
                return visitor.visitVfrStatementPassword(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementPassword(self):

        localctx = VfrSyntaxParser.VfrStatementPasswordContext(self, self._ctx, self.state)
        self.enterRule(localctx, 218, self.RULE_vfrStatementPassword)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1796
            self.match(VfrSyntaxParser.Password)
            self.state = 1797
            self.vfrQuestionHeader(localctx.Node, localctx.QType)
            self.state = 1798
            self.match(VfrSyntaxParser.Comma)
            self.state = 1804
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1799
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1800
                self.match(VfrSyntaxParser.T__5)
                self.state = 1801
                self.vfrPasswordFlagsField()
                self.state = 1802
                self.match(VfrSyntaxParser.Comma)


            self.state = 1810
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Key:
                self.state = 1806
                self.match(VfrSyntaxParser.Key)
                self.state = 1807
                self.match(VfrSyntaxParser.T__5)
                self.state = 1808
                self.match(VfrSyntaxParser.Number)
                self.state = 1809
                self.match(VfrSyntaxParser.Comma)


            self.state = 1812
            localctx.Min = self.match(VfrSyntaxParser.MinSize)
            self.state = 1813
            self.match(VfrSyntaxParser.T__5)
            self.state = 1814
            self.match(VfrSyntaxParser.Number)
            self.state = 1815
            self.match(VfrSyntaxParser.Comma)
            self.state = 1816
            localctx.Max = self.match(VfrSyntaxParser.MaxSize)
            self.state = 1817
            self.match(VfrSyntaxParser.T__5)
            self.state = 1818
            self.match(VfrSyntaxParser.Number)
            self.state = 1819
            self.match(VfrSyntaxParser.Comma)
            self.state = 1824
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Encoding:
                self.state = 1820
                self.match(VfrSyntaxParser.Encoding)
                self.state = 1821
                self.match(VfrSyntaxParser.T__5)
                self.state = 1822
                self.match(VfrSyntaxParser.Number)
                self.state = 1823
                self.match(VfrSyntaxParser.Comma)


            self.state = 1826
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1827
            self.match(VfrSyntaxParser.EndPassword)
            self.state = 1828
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrPasswordFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0

        def passwordFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.PasswordFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.PasswordFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrPasswordFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrPasswordFlagsField" ):
                return visitor.visitVfrPasswordFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def vfrPasswordFlagsField(self):

        localctx = VfrSyntaxParser.VfrPasswordFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 220, self.RULE_vfrPasswordFlagsField)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1830
            self.passwordFlagsField()
            self.state = 1835
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1831
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1832
                self.passwordFlagsField()
                self.state = 1837
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class PasswordFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlag = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_passwordFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitPasswordFlagsField" ):
                return visitor.visitPasswordFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def passwordFlagsField(self):

        localctx = VfrSyntaxParser.PasswordFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 222, self.RULE_passwordFlagsField)
        try:
            self.state = 1840
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1838
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1839
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementOrderedListContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_ORDERED_LIST_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL
            self.M = None # Token
            self.F = None # Token

        def OrderedList(self):
            return self.getToken(VfrSyntaxParser.OrderedList, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def EndList(self):
            return self.getToken(VfrSyntaxParser.EndList, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def vfrOrderedListFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrOrderedListFlagsContext,0)


        def MaxContainers(self):
            return self.getToken(VfrSyntaxParser.MaxContainers, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementOrderedList

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementOrderedList" ):
                return visitor.visitVfrStatementOrderedList(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementOrderedList(self):

        localctx = VfrSyntaxParser.VfrStatementOrderedListContext(self, self._ctx, self.state)
        self.enterRule(localctx, 224, self.RULE_vfrStatementOrderedList)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1842
            self.match(VfrSyntaxParser.OrderedList)
            self.state = 1843
            self.vfrQuestionHeader(localctx.Node, localctx.QType)
            self.state = 1844
            self.match(VfrSyntaxParser.Comma)
            self.state = 1849
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.MaxContainers:
                self.state = 1845
                localctx.M = self.match(VfrSyntaxParser.MaxContainers)
                self.state = 1846
                self.match(VfrSyntaxParser.T__5)
                self.state = 1847
                self.match(VfrSyntaxParser.Number)
                self.state = 1848
                self.match(VfrSyntaxParser.Comma)


            self.state = 1856
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 1851
                localctx.F = self.match(VfrSyntaxParser.FLAGS)
                self.state = 1852
                self.match(VfrSyntaxParser.T__5)
                self.state = 1853
                self.vfrOrderedListFlags()
                self.state = 1854
                self.match(VfrSyntaxParser.Comma)


            self.state = 1858
            self.vfrStatementQuestionOptionList(localctx.Node)
            self.state = 1859
            self.match(VfrSyntaxParser.EndList)
            self.state = 1860
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrOrderedListFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlags = 0
            self.LFlags = 0

        def orderedlistFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.OrderedlistFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.OrderedlistFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrOrderedListFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrOrderedListFlags" ):
                return visitor.visitVfrOrderedListFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrOrderedListFlags(self):

        localctx = VfrSyntaxParser.VfrOrderedListFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 226, self.RULE_vfrOrderedListFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1862
            self.orderedlistFlagsField()
            self.state = 1867
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1863
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1864
                self.orderedlistFlagsField()
                self.state = 1869
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class OrderedlistFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.HFlag = 0
            self.LFlag = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def UniQueFlag(self):
            return self.getToken(VfrSyntaxParser.UniQueFlag, 0)

        def NoEmptyFlag(self):
            return self.getToken(VfrSyntaxParser.NoEmptyFlag, 0)

        def questionheaderFlagsField(self):
            return self.getTypedRuleContext(VfrSyntaxParser.QuestionheaderFlagsFieldContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_orderedlistFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitOrderedlistFlagsField" ):
                return visitor.visitOrderedlistFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def orderedlistFlagsField(self):

        localctx = VfrSyntaxParser.OrderedlistFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 228, self.RULE_orderedlistFlagsField)
        try:
            self.state = 1874
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 1)
                self.state = 1870
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.UniQueFlag]:
                self.enterOuterAlt(localctx, 2)
                self.state = 1871
                self.match(VfrSyntaxParser.UniQueFlag)
                pass
            elif token in [VfrSyntaxParser.NoEmptyFlag]:
                self.enterOuterAlt(localctx, 3)
                self.state = 1872
                self.match(VfrSyntaxParser.NoEmptyFlag)
                pass
            elif token in [VfrSyntaxParser.InteractiveFlag, VfrSyntaxParser.NVAccessFlag, VfrSyntaxParser.ResetRequiredFlag, VfrSyntaxParser.ReconnectRequiredFlag, VfrSyntaxParser.LateCheckFlag, VfrSyntaxParser.ReadOnlyFlag, VfrSyntaxParser.OptionOnlyFlag, VfrSyntaxParser.RestStyleFlag]:
                self.enterOuterAlt(localctx, 4)
                self.state = 1873
                self.questionheaderFlagsField()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementDateContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_DATE_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_DATE
            self.Val = EFI_HII_DATE()
            self.F1 = None # Token
            self.F2 = None # Token

        def Date(self):
            return self.getToken(VfrSyntaxParser.Date, 0)

        def EndDate(self):
            return self.getToken(VfrSyntaxParser.EndDate, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def Year(self):
            return self.getToken(VfrSyntaxParser.Year, 0)

        def VarId(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.VarId)
            else:
                return self.getToken(VfrSyntaxParser.VarId, i)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def Dot(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Dot)
            else:
                return self.getToken(VfrSyntaxParser.Dot, i)

        def Prompt(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Prompt)
            else:
                return self.getToken(VfrSyntaxParser.Prompt, i)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Help(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Help)
            else:
                return self.getToken(VfrSyntaxParser.Help, i)

        def minMaxDateStepDefault(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.MinMaxDateStepDefaultContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.MinMaxDateStepDefaultContext,i)


        def Month(self):
            return self.getToken(VfrSyntaxParser.Month, 0)

        def Day(self):
            return self.getToken(VfrSyntaxParser.Day, 0)

        def vfrDateFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrDateFlagsContext,0)


        def vfrStatementInconsistentIf(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementInconsistentIfContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInconsistentIfContext,i)


        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementDate

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementDate" ):
                return visitor.visitVfrStatementDate(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementDate(self):

        localctx = VfrSyntaxParser.VfrStatementDateContext(self, self._ctx, self.state)
        self.enterRule(localctx, 230, self.RULE_vfrStatementDate)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1876
            self.match(VfrSyntaxParser.Date)
            self.state = 1967
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Prompt, VfrSyntaxParser.Name, VfrSyntaxParser.VarId, VfrSyntaxParser.QuestionId]:
                self.state = 1877
                self.vfrQuestionHeader(localctx.Node, localctx.QType)
                self.state = 1878
                self.match(VfrSyntaxParser.Comma)
                self.state = 1884
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.FLAGS:
                    self.state = 1879
                    localctx.F1 = self.match(VfrSyntaxParser.FLAGS)
                    self.state = 1880
                    self.match(VfrSyntaxParser.T__5)
                    self.state = 1881
                    self.vfrDateFlags()
                    self.state = 1882
                    self.match(VfrSyntaxParser.Comma)


                self.state = 1886
                self.vfrStatementQuestionOptionList(localctx.Node)
                pass
            elif token in [VfrSyntaxParser.Year]:
                self.state = 1888
                self.match(VfrSyntaxParser.Year)
                self.state = 1889
                self.match(VfrSyntaxParser.VarId)
                self.state = 1890
                self.match(VfrSyntaxParser.T__5)
                self.state = 1891
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1892
                self.match(VfrSyntaxParser.Dot)
                self.state = 1893
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1894
                self.match(VfrSyntaxParser.Comma)
                self.state = 1895
                self.match(VfrSyntaxParser.Prompt)
                self.state = 1896
                self.match(VfrSyntaxParser.T__5)
                self.state = 1897
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1898
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1899
                self.match(VfrSyntaxParser.Number)
                self.state = 1900
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1901
                self.match(VfrSyntaxParser.Comma)
                self.state = 1902
                self.match(VfrSyntaxParser.Help)
                self.state = 1903
                self.match(VfrSyntaxParser.T__5)
                self.state = 1904
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1905
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1906
                self.match(VfrSyntaxParser.Number)
                self.state = 1907
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1908
                self.match(VfrSyntaxParser.Comma)
                self.state = 1909
                self.minMaxDateStepDefault(localctx.Val, 0)
                self.state = 1910
                self.match(VfrSyntaxParser.Month)
                self.state = 1911
                self.match(VfrSyntaxParser.VarId)
                self.state = 1912
                self.match(VfrSyntaxParser.T__5)
                self.state = 1913
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1914
                self.match(VfrSyntaxParser.Dot)
                self.state = 1915
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1916
                self.match(VfrSyntaxParser.Comma)
                self.state = 1917
                self.match(VfrSyntaxParser.Prompt)
                self.state = 1918
                self.match(VfrSyntaxParser.T__5)
                self.state = 1919
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1920
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1921
                self.match(VfrSyntaxParser.Number)
                self.state = 1922
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1923
                self.match(VfrSyntaxParser.Comma)
                self.state = 1924
                self.match(VfrSyntaxParser.Help)
                self.state = 1925
                self.match(VfrSyntaxParser.T__5)
                self.state = 1926
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1927
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1928
                self.match(VfrSyntaxParser.Number)
                self.state = 1929
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1930
                self.match(VfrSyntaxParser.Comma)
                self.state = 1931
                self.minMaxDateStepDefault(localctx.Val, 1)
                self.state = 1932
                self.match(VfrSyntaxParser.Day)
                self.state = 1933
                self.match(VfrSyntaxParser.VarId)
                self.state = 1934
                self.match(VfrSyntaxParser.T__5)
                self.state = 1935
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1936
                self.match(VfrSyntaxParser.Dot)
                self.state = 1937
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 1938
                self.match(VfrSyntaxParser.Comma)
                self.state = 1939
                self.match(VfrSyntaxParser.Prompt)
                self.state = 1940
                self.match(VfrSyntaxParser.T__5)
                self.state = 1941
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1942
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1943
                self.match(VfrSyntaxParser.Number)
                self.state = 1944
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1945
                self.match(VfrSyntaxParser.Comma)
                self.state = 1946
                self.match(VfrSyntaxParser.Help)
                self.state = 1947
                self.match(VfrSyntaxParser.T__5)
                self.state = 1948
                self.match(VfrSyntaxParser.StringToken)
                self.state = 1949
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 1950
                self.match(VfrSyntaxParser.Number)
                self.state = 1951
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 1952
                self.match(VfrSyntaxParser.Comma)
                self.state = 1953
                self.minMaxDateStepDefault(localctx.Val, 2)
                self.state = 1959
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.FLAGS:
                    self.state = 1954
                    localctx.F2 = self.match(VfrSyntaxParser.FLAGS)
                    self.state = 1955
                    self.match(VfrSyntaxParser.T__5)
                    self.state = 1956
                    self.vfrDateFlags()
                    self.state = 1957
                    self.match(VfrSyntaxParser.Comma)


                self.state = 1964
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.InconsistentIf:
                    self.state = 1961
                    self.vfrStatementInconsistentIf()
                    self.state = 1966
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                pass
            else:
                raise NoViableAltException(self)

            self.state = 1969
            self.match(VfrSyntaxParser.EndDate)
            self.state = 1970
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class MinMaxDateStepDefaultContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Date=None, KeyValue=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Date = None
            self.KeyValue = None
            self.N = None # Token
            self.Date = Date
            self.KeyValue = KeyValue

        def Minimum(self):
            return self.getToken(VfrSyntaxParser.Minimum, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Maximum(self):
            return self.getToken(VfrSyntaxParser.Maximum, 0)

        def Step(self):
            return self.getToken(VfrSyntaxParser.Step, 0)

        def Default(self):
            return self.getToken(VfrSyntaxParser.Default, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_minMaxDateStepDefault

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMinMaxDateStepDefault" ):
                return visitor.visitMinMaxDateStepDefault(self)
            else:
                return visitor.visitChildren(self)




    def minMaxDateStepDefault(self, Date, KeyValue):

        localctx = VfrSyntaxParser.MinMaxDateStepDefaultContext(self, self._ctx, self.state, Date, KeyValue)
        self.enterRule(localctx, 232, self.RULE_minMaxDateStepDefault)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1972
            self.match(VfrSyntaxParser.Minimum)
            self.state = 1973
            self.match(VfrSyntaxParser.T__5)
            self.state = 1974
            self.match(VfrSyntaxParser.Number)
            self.state = 1975
            self.match(VfrSyntaxParser.Comma)
            self.state = 1976
            self.match(VfrSyntaxParser.Maximum)
            self.state = 1977
            self.match(VfrSyntaxParser.T__5)
            self.state = 1978
            self.match(VfrSyntaxParser.Number)
            self.state = 1979
            self.match(VfrSyntaxParser.Comma)
            self.state = 1984
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Step:
                self.state = 1980
                self.match(VfrSyntaxParser.Step)
                self.state = 1981
                self.match(VfrSyntaxParser.T__5)
                self.state = 1982
                self.match(VfrSyntaxParser.Number)
                self.state = 1983
                self.match(VfrSyntaxParser.Comma)


            self.state = 1990
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Default:
                self.state = 1986
                self.match(VfrSyntaxParser.Default)
                self.state = 1987
                self.match(VfrSyntaxParser.T__5)
                self.state = 1988
                localctx.N = self.match(VfrSyntaxParser.Number)
                self.state = 1989
                self.match(VfrSyntaxParser.Comma)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrDateFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.LFlags = 0

        def dateFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.DateFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.DateFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrDateFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrDateFlags" ):
                return visitor.visitVfrDateFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrDateFlags(self):

        localctx = VfrSyntaxParser.VfrDateFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 234, self.RULE_vfrDateFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 1992
            self.dateFlagsField()
            self.state = 1997
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 1993
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 1994
                self.dateFlagsField()
                self.state = 1999
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DateFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.LFlag = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def YearSupppressFlag(self):
            return self.getToken(VfrSyntaxParser.YearSupppressFlag, 0)

        def MonthSuppressFlag(self):
            return self.getToken(VfrSyntaxParser.MonthSuppressFlag, 0)

        def DaySuppressFlag(self):
            return self.getToken(VfrSyntaxParser.DaySuppressFlag, 0)

        def StorageNormalFlag(self):
            return self.getToken(VfrSyntaxParser.StorageNormalFlag, 0)

        def StorageTimeFlag(self):
            return self.getToken(VfrSyntaxParser.StorageTimeFlag, 0)

        def StorageWakeUpFlag(self):
            return self.getToken(VfrSyntaxParser.StorageWakeUpFlag, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dateFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDateFlagsField" ):
                return visitor.visitDateFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def dateFlagsField(self):

        localctx = VfrSyntaxParser.DateFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 236, self.RULE_dateFlagsField)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2000
            _la = self._input.LA(1)
            if not(((((_la - 181)) & ~0x3f) == 0 and ((1 << (_la - 181)) & ((1 << (VfrSyntaxParser.YearSupppressFlag - 181)) | (1 << (VfrSyntaxParser.MonthSuppressFlag - 181)) | (1 << (VfrSyntaxParser.DaySuppressFlag - 181)) | (1 << (VfrSyntaxParser.StorageNormalFlag - 181)) | (1 << (VfrSyntaxParser.StorageTimeFlag - 181)) | (1 << (VfrSyntaxParser.StorageWakeUpFlag - 181)))) != 0) or _la==VfrSyntaxParser.Number):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementTimeContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_TIME_OP)
            self.QType = EFI_QUESION_TYPE.QUESTION_TIME
            self.Val = EFI_HII_TIME()
            self.F1 = None # Token
            self.F2 = None # Token

        def Time(self):
            return self.getToken(VfrSyntaxParser.Time, 0)

        def EndTime(self):
            return self.getToken(VfrSyntaxParser.EndTime, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def vfrQuestionHeader(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionHeaderContext,0)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementQuestionOptionList(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionOptionListContext,0)


        def Hour(self):
            return self.getToken(VfrSyntaxParser.Hour, 0)

        def VarId(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.VarId)
            else:
                return self.getToken(VfrSyntaxParser.VarId, i)

        def StringIdentifier(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringIdentifier)
            else:
                return self.getToken(VfrSyntaxParser.StringIdentifier, i)

        def Dot(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Dot)
            else:
                return self.getToken(VfrSyntaxParser.Dot, i)

        def Prompt(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Prompt)
            else:
                return self.getToken(VfrSyntaxParser.Prompt, i)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Help(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Help)
            else:
                return self.getToken(VfrSyntaxParser.Help, i)

        def minMaxTimeStepDefault(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.MinMaxTimeStepDefaultContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.MinMaxTimeStepDefaultContext,i)


        def Minute(self):
            return self.getToken(VfrSyntaxParser.Minute, 0)

        def Second(self):
            return self.getToken(VfrSyntaxParser.Second, 0)

        def vfrTimeFlags(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrTimeFlagsContext,0)


        def vfrStatementInconsistentIf(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementInconsistentIfContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInconsistentIfContext,i)


        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementTime

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementTime" ):
                return visitor.visitVfrStatementTime(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementTime(self):

        localctx = VfrSyntaxParser.VfrStatementTimeContext(self, self._ctx, self.state)
        self.enterRule(localctx, 238, self.RULE_vfrStatementTime)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2002
            self.match(VfrSyntaxParser.Time)
            self.state = 2093
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Prompt, VfrSyntaxParser.Name, VfrSyntaxParser.VarId, VfrSyntaxParser.QuestionId]:
                self.state = 2003
                self.vfrQuestionHeader(localctx.Node, localctx.QType)
                self.state = 2004
                self.match(VfrSyntaxParser.Comma)
                self.state = 2010
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.FLAGS:
                    self.state = 2005
                    localctx.F1 = self.match(VfrSyntaxParser.FLAGS)
                    self.state = 2006
                    self.match(VfrSyntaxParser.T__5)
                    self.state = 2007
                    self.vfrTimeFlags()
                    self.state = 2008
                    self.match(VfrSyntaxParser.Comma)


                self.state = 2012
                self.vfrStatementQuestionOptionList(localctx.Node)
                pass
            elif token in [VfrSyntaxParser.Hour]:
                self.state = 2014
                self.match(VfrSyntaxParser.Hour)
                self.state = 2015
                self.match(VfrSyntaxParser.VarId)
                self.state = 2016
                self.match(VfrSyntaxParser.T__5)
                self.state = 2017
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2018
                self.match(VfrSyntaxParser.Dot)
                self.state = 2019
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2020
                self.match(VfrSyntaxParser.Comma)
                self.state = 2021
                self.match(VfrSyntaxParser.Prompt)
                self.state = 2022
                self.match(VfrSyntaxParser.T__5)
                self.state = 2023
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2024
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2025
                self.match(VfrSyntaxParser.Number)
                self.state = 2026
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2027
                self.match(VfrSyntaxParser.Comma)
                self.state = 2028
                self.match(VfrSyntaxParser.Help)
                self.state = 2029
                self.match(VfrSyntaxParser.T__5)
                self.state = 2030
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2031
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2032
                self.match(VfrSyntaxParser.Number)
                self.state = 2033
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2034
                self.match(VfrSyntaxParser.Comma)
                self.state = 2035
                self.minMaxTimeStepDefault(localctx.Val, 0)
                self.state = 2036
                self.match(VfrSyntaxParser.Minute)
                self.state = 2037
                self.match(VfrSyntaxParser.VarId)
                self.state = 2038
                self.match(VfrSyntaxParser.T__5)
                self.state = 2039
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2040
                self.match(VfrSyntaxParser.Dot)
                self.state = 2041
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2042
                self.match(VfrSyntaxParser.Comma)
                self.state = 2043
                self.match(VfrSyntaxParser.Prompt)
                self.state = 2044
                self.match(VfrSyntaxParser.T__5)
                self.state = 2045
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2046
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2047
                self.match(VfrSyntaxParser.Number)
                self.state = 2048
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2049
                self.match(VfrSyntaxParser.Comma)
                self.state = 2050
                self.match(VfrSyntaxParser.Help)
                self.state = 2051
                self.match(VfrSyntaxParser.T__5)
                self.state = 2052
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2053
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2054
                self.match(VfrSyntaxParser.Number)
                self.state = 2055
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2056
                self.match(VfrSyntaxParser.Comma)
                self.state = 2057
                self.minMaxTimeStepDefault(localctx.Val, 1)
                self.state = 2058
                self.match(VfrSyntaxParser.Second)
                self.state = 2059
                self.match(VfrSyntaxParser.VarId)
                self.state = 2060
                self.match(VfrSyntaxParser.T__5)
                self.state = 2061
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2062
                self.match(VfrSyntaxParser.Dot)
                self.state = 2063
                self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2064
                self.match(VfrSyntaxParser.Comma)
                self.state = 2065
                self.match(VfrSyntaxParser.Prompt)
                self.state = 2066
                self.match(VfrSyntaxParser.T__5)
                self.state = 2067
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2068
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2069
                self.match(VfrSyntaxParser.Number)
                self.state = 2070
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2071
                self.match(VfrSyntaxParser.Comma)
                self.state = 2072
                self.match(VfrSyntaxParser.Help)
                self.state = 2073
                self.match(VfrSyntaxParser.T__5)
                self.state = 2074
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2075
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2076
                self.match(VfrSyntaxParser.Number)
                self.state = 2077
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2078
                self.match(VfrSyntaxParser.Comma)
                self.state = 2079
                self.minMaxTimeStepDefault(localctx.Val, 2)
                self.state = 2085
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if _la==VfrSyntaxParser.FLAGS:
                    self.state = 2080
                    localctx.F2 = self.match(VfrSyntaxParser.FLAGS)
                    self.state = 2081
                    self.match(VfrSyntaxParser.T__5)
                    self.state = 2082
                    self.vfrTimeFlags()
                    self.state = 2083
                    self.match(VfrSyntaxParser.Comma)


                self.state = 2090
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.InconsistentIf:
                    self.state = 2087
                    self.vfrStatementInconsistentIf()
                    self.state = 2092
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                pass
            else:
                raise NoViableAltException(self)

            self.state = 2095
            self.match(VfrSyntaxParser.EndTime)
            self.state = 2096
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class MinMaxTimeStepDefaultContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, Time=None, KeyValue=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Time = None
            self.KeyValue = None
            self.N = None # Token
            self.Time = Time
            self.KeyValue = KeyValue

        def Minimum(self):
            return self.getToken(VfrSyntaxParser.Minimum, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Maximum(self):
            return self.getToken(VfrSyntaxParser.Maximum, 0)

        def Step(self):
            return self.getToken(VfrSyntaxParser.Step, 0)

        def Default(self):
            return self.getToken(VfrSyntaxParser.Default, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_minMaxTimeStepDefault

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMinMaxTimeStepDefault" ):
                return visitor.visitMinMaxTimeStepDefault(self)
            else:
                return visitor.visitChildren(self)




    def minMaxTimeStepDefault(self, Time, KeyValue):

        localctx = VfrSyntaxParser.MinMaxTimeStepDefaultContext(self, self._ctx, self.state, Time, KeyValue)
        self.enterRule(localctx, 240, self.RULE_minMaxTimeStepDefault)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2098
            self.match(VfrSyntaxParser.Minimum)
            self.state = 2099
            self.match(VfrSyntaxParser.T__5)
            self.state = 2100
            self.match(VfrSyntaxParser.Number)
            self.state = 2101
            self.match(VfrSyntaxParser.Comma)
            self.state = 2102
            self.match(VfrSyntaxParser.Maximum)
            self.state = 2103
            self.match(VfrSyntaxParser.T__5)
            self.state = 2104
            self.match(VfrSyntaxParser.Number)
            self.state = 2105
            self.match(VfrSyntaxParser.Comma)
            self.state = 2110
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Step:
                self.state = 2106
                self.match(VfrSyntaxParser.Step)
                self.state = 2107
                self.match(VfrSyntaxParser.T__5)
                self.state = 2108
                self.match(VfrSyntaxParser.Number)
                self.state = 2109
                self.match(VfrSyntaxParser.Comma)


            self.state = 2116
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Default:
                self.state = 2112
                self.match(VfrSyntaxParser.Default)
                self.state = 2113
                self.match(VfrSyntaxParser.T__5)
                self.state = 2114
                localctx.N = self.match(VfrSyntaxParser.Number)
                self.state = 2115
                self.match(VfrSyntaxParser.Comma)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrTimeFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.LFlags = 0

        def timeFlagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.TimeFlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.TimeFlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrTimeFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrTimeFlags" ):
                return visitor.visitVfrTimeFlags(self)
            else:
                return visitor.visitChildren(self)




    def vfrTimeFlags(self):

        localctx = VfrSyntaxParser.VfrTimeFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 242, self.RULE_vfrTimeFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2118
            self.timeFlagsField()
            self.state = 2123
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 2119
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 2120
                self.timeFlagsField()
                self.state = 2125
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class TimeFlagsFieldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.LFlag = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def HourSupppressFlag(self):
            return self.getToken(VfrSyntaxParser.HourSupppressFlag, 0)

        def MinuteSuppressFlag(self):
            return self.getToken(VfrSyntaxParser.MinuteSuppressFlag, 0)

        def SecondSuppressFlag(self):
            return self.getToken(VfrSyntaxParser.SecondSuppressFlag, 0)

        def StorageNormalFlag(self):
            return self.getToken(VfrSyntaxParser.StorageNormalFlag, 0)

        def StorageTimeFlag(self):
            return self.getToken(VfrSyntaxParser.StorageTimeFlag, 0)

        def StorageWakeUpFlag(self):
            return self.getToken(VfrSyntaxParser.StorageWakeUpFlag, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_timeFlagsField

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitTimeFlagsField" ):
                return visitor.visitTimeFlagsField(self)
            else:
                return visitor.visitChildren(self)




    def timeFlagsField(self):

        localctx = VfrSyntaxParser.TimeFlagsFieldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 244, self.RULE_timeFlagsField)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2126
            _la = self._input.LA(1)
            if not(((((_la - 184)) & ~0x3f) == 0 and ((1 << (_la - 184)) & ((1 << (VfrSyntaxParser.HourSupppressFlag - 184)) | (1 << (VfrSyntaxParser.MinuteSuppressFlag - 184)) | (1 << (VfrSyntaxParser.SecondSuppressFlag - 184)) | (1 << (VfrSyntaxParser.StorageNormalFlag - 184)) | (1 << (VfrSyntaxParser.StorageTimeFlag - 184)) | (1 << (VfrSyntaxParser.StorageWakeUpFlag - 184)))) != 0) or _la==VfrSyntaxParser.Number):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementConditionalContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementDisableIfStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDisableIfStatContext,0)


        def vfrStatementSuppressIfStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSuppressIfStatContext,0)


        def vfrStatementGrayOutIfStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementGrayOutIfStatContext,0)


        def vfrStatementInconsistentIfStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInconsistentIfStatContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementConditional

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementConditional" ):
                return visitor.visitVfrStatementConditional(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementConditional(self):

        localctx = VfrSyntaxParser.VfrStatementConditionalContext(self, self._ctx, self.state)
        self.enterRule(localctx, 246, self.RULE_vfrStatementConditional)
        try:
            self.state = 2132
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.DisableIf]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2128
                self.vfrStatementDisableIfStat()
                pass
            elif token in [VfrSyntaxParser.SuppressIf]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2129
                self.vfrStatementSuppressIfStat()
                pass
            elif token in [VfrSyntaxParser.GrayOutIf]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2130
                self.vfrStatementGrayOutIfStat()
                pass
            elif token in [VfrSyntaxParser.InconsistentIf]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2131
                self.vfrStatementInconsistentIfStat()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementConditionalNewContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementDisableIfStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementDisableIfStatContext,0)


        def vfrStatementSuppressIfStatNew(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSuppressIfStatNewContext,0)


        def vfrStatementGrayOutIfStatNew(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementGrayOutIfStatNewContext,0)


        def vfrStatementInconsistentIfStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInconsistentIfStatContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementConditionalNew

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementConditionalNew" ):
                return visitor.visitVfrStatementConditionalNew(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementConditionalNew(self):

        localctx = VfrSyntaxParser.VfrStatementConditionalNewContext(self, self._ctx, self.state)
        self.enterRule(localctx, 248, self.RULE_vfrStatementConditionalNew)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2134
            self.vfrStatementDisableIfStat()
            self.state = 2135
            self.vfrStatementSuppressIfStatNew()
            self.state = 2136
            self.vfrStatementGrayOutIfStatNew()
            self.state = 2137
            self.vfrStatementInconsistentIfStat()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementSuppressIfStatContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementSuppressIfStatNew(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementSuppressIfStatNewContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementSuppressIfStat

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementSuppressIfStat" ):
                return visitor.visitVfrStatementSuppressIfStat(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementSuppressIfStat(self):

        localctx = VfrSyntaxParser.VfrStatementSuppressIfStatContext(self, self._ctx, self.state)
        self.enterRule(localctx, 250, self.RULE_vfrStatementSuppressIfStat)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2139
            self.vfrStatementSuppressIfStatNew()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementGrayOutIfStatContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementGrayOutIfStatNew(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementGrayOutIfStatNewContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementGrayOutIfStat

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementGrayOutIfStat" ):
                return visitor.visitVfrStatementGrayOutIfStat(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementGrayOutIfStat(self):

        localctx = VfrSyntaxParser.VfrStatementGrayOutIfStatContext(self, self._ctx, self.state)
        self.enterRule(localctx, 252, self.RULE_vfrStatementGrayOutIfStat)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2141
            self.vfrStatementGrayOutIfStatNew()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStatListContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrStatementStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatContext,0)


        def vfrStatementQuestions(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionsContext,0)


        def vfrStatementConditional(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementConditionalContext,0)


        def vfrStatementLabel(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementLabelContext,0)


        def vfrStatementExtension(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExtensionContext,0)


        def vfrStatementInvalid(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInvalidContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStatList

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStatList" ):
                return visitor.visitVfrStatementStatList(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStatList(self):

        localctx = VfrSyntaxParser.VfrStatementStatListContext(self, self._ctx, self.state)
        self.enterRule(localctx, 254, self.RULE_vfrStatementStatList)
        try:
            self.state = 2149
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Subtitle, VfrSyntaxParser.Text, VfrSyntaxParser.Goto, VfrSyntaxParser.ResetButton]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2143
                self.vfrStatementStat()
                pass
            elif token in [VfrSyntaxParser.OneOf, VfrSyntaxParser.OrderedList, VfrSyntaxParser.Date, VfrSyntaxParser.Time, VfrSyntaxParser.CheckBox, VfrSyntaxParser.Numeric, VfrSyntaxParser.Password, VfrSyntaxParser.String, VfrSyntaxParser.Action]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2144
                self.vfrStatementQuestions()
                pass
            elif token in [VfrSyntaxParser.GrayOutIf, VfrSyntaxParser.SuppressIf, VfrSyntaxParser.DisableIf, VfrSyntaxParser.InconsistentIf]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2145
                self.vfrStatementConditional()
                pass
            elif token in [VfrSyntaxParser.Label]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2146
                self.vfrStatementLabel()
                pass
            elif token in [VfrSyntaxParser.GuidOp]:
                self.enterOuterAlt(localctx, 5)
                self.state = 2147
                self.vfrStatementExtension()
                pass
            elif token in [VfrSyntaxParser.Inventory, VfrSyntaxParser.Hidden, VfrSyntaxParser.Restore, VfrSyntaxParser.Save]:
                self.enterOuterAlt(localctx, 6)
                self.state = 2148
                self.vfrStatementInvalid()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementStatListOldContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def vfrStatementStat(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatContext,0)


        def vfrStatementQuestions(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementQuestionsContext,0)


        def vfrStatementLabel(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementLabelContext,0)


        def vfrStatementInvalid(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInvalidContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementStatListOld

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementStatListOld" ):
                return visitor.visitVfrStatementStatListOld(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementStatListOld(self):

        localctx = VfrSyntaxParser.VfrStatementStatListOldContext(self, self._ctx, self.state)
        self.enterRule(localctx, 256, self.RULE_vfrStatementStatListOld)
        try:
            self.state = 2155
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Subtitle, VfrSyntaxParser.Text, VfrSyntaxParser.Goto, VfrSyntaxParser.ResetButton]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2151
                self.vfrStatementStat()
                pass
            elif token in [VfrSyntaxParser.OneOf, VfrSyntaxParser.OrderedList, VfrSyntaxParser.Date, VfrSyntaxParser.Time, VfrSyntaxParser.CheckBox, VfrSyntaxParser.Numeric, VfrSyntaxParser.Password, VfrSyntaxParser.String, VfrSyntaxParser.Action]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2152
                self.vfrStatementQuestions()
                pass
            elif token in [VfrSyntaxParser.Label]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2153
                self.vfrStatementLabel()
                pass
            elif token in [VfrSyntaxParser.Inventory, VfrSyntaxParser.Hidden, VfrSyntaxParser.Restore, VfrSyntaxParser.Save]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2154
                self.vfrStatementInvalid()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementDisableIfStatContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_DISABLE_IF_OP)

        def DisableIf(self):
            return self.getToken(VfrSyntaxParser.DisableIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def vfrStatementStatList(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementStatListContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatListContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementDisableIfStat

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementDisableIfStat" ):
                return visitor.visitVfrStatementDisableIfStat(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementDisableIfStat(self):

        localctx = VfrSyntaxParser.VfrStatementDisableIfStatContext(self, self._ctx, self.state)
        self.enterRule(localctx, 258, self.RULE_vfrStatementDisableIfStat)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2157
            self.match(VfrSyntaxParser.DisableIf)
            self.state = 2158
            self.vfrStatementExpression(localctx.Node)
            self.state = 2159
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 2163
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 46)) & ~0x3f) == 0 and ((1 << (_la - 46)) & ((1 << (VfrSyntaxParser.OneOf - 46)) | (1 << (VfrSyntaxParser.OrderedList - 46)) | (1 << (VfrSyntaxParser.Subtitle - 46)) | (1 << (VfrSyntaxParser.Text - 46)) | (1 << (VfrSyntaxParser.Date - 46)) | (1 << (VfrSyntaxParser.Time - 46)) | (1 << (VfrSyntaxParser.GrayOutIf - 46)) | (1 << (VfrSyntaxParser.Label - 46)) | (1 << (VfrSyntaxParser.Inventory - 46)) | (1 << (VfrSyntaxParser.CheckBox - 46)) | (1 << (VfrSyntaxParser.Numeric - 46)) | (1 << (VfrSyntaxParser.Password - 46)) | (1 << (VfrSyntaxParser.String - 46)) | (1 << (VfrSyntaxParser.SuppressIf - 46)) | (1 << (VfrSyntaxParser.DisableIf - 46)) | (1 << (VfrSyntaxParser.Hidden - 46)) | (1 << (VfrSyntaxParser.Goto - 46)))) != 0) or ((((_la - 110)) & ~0x3f) == 0 and ((1 << (_la - 110)) & ((1 << (VfrSyntaxParser.InconsistentIf - 110)) | (1 << (VfrSyntaxParser.Restore - 110)) | (1 << (VfrSyntaxParser.Save - 110)) | (1 << (VfrSyntaxParser.ResetButton - 110)) | (1 << (VfrSyntaxParser.Action - 110)) | (1 << (VfrSyntaxParser.GuidOp - 110)))) != 0):
                self.state = 2160
                self.vfrStatementStatList()
                self.state = 2165
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2166
            self.match(VfrSyntaxParser.EndIf)
            self.state = 2167
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementgrayoutIfSuppressIfContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def SuppressIf(self):
            return self.getToken(VfrSyntaxParser.SuppressIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementgrayoutIfSuppressIf

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementgrayoutIfSuppressIf" ):
                return visitor.visitVfrStatementgrayoutIfSuppressIf(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementgrayoutIfSuppressIf(self):

        localctx = VfrSyntaxParser.VfrStatementgrayoutIfSuppressIfContext(self, self._ctx, self.state)
        self.enterRule(localctx, 260, self.RULE_vfrStatementgrayoutIfSuppressIf)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2169
            self.match(VfrSyntaxParser.SuppressIf)
            self.state = 2182
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 2170
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2171
                self.match(VfrSyntaxParser.T__5)
                self.state = 2172
                self.flagsField()
                self.state = 2177
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 2173
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 2174
                    self.flagsField()
                    self.state = 2179
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 2180
                self.match(VfrSyntaxParser.Comma)


            self.state = 2184
            self.vfrStatementExpression(localctx.Node)
            self.state = 2185
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementsuppressIfGrayOutIfContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def GrayOutIf(self):
            return self.getToken(VfrSyntaxParser.GrayOutIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementsuppressIfGrayOutIf

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementsuppressIfGrayOutIf" ):
                return visitor.visitVfrStatementsuppressIfGrayOutIf(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementsuppressIfGrayOutIf(self):

        localctx = VfrSyntaxParser.VfrStatementsuppressIfGrayOutIfContext(self, self._ctx, self.state)
        self.enterRule(localctx, 262, self.RULE_vfrStatementsuppressIfGrayOutIf)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2187
            self.match(VfrSyntaxParser.GrayOutIf)
            self.state = 2200
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 2188
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2189
                self.match(VfrSyntaxParser.T__5)
                self.state = 2190
                self.flagsField()
                self.state = 2195
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 2191
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 2192
                    self.flagsField()
                    self.state = 2197
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 2198
                self.match(VfrSyntaxParser.Comma)


            self.state = 2202
            self.vfrStatementExpression(localctx.Node)
            self.state = 2203
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementSuppressIfStatNewContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)

        def SuppressIf(self):
            return self.getToken(VfrSyntaxParser.SuppressIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def vfrStatementStatList(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementStatListContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatListContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementSuppressIfStatNew

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementSuppressIfStatNew" ):
                return visitor.visitVfrStatementSuppressIfStatNew(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementSuppressIfStatNew(self):

        localctx = VfrSyntaxParser.VfrStatementSuppressIfStatNewContext(self, self._ctx, self.state)
        self.enterRule(localctx, 264, self.RULE_vfrStatementSuppressIfStatNew)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2205
            self.match(VfrSyntaxParser.SuppressIf)
            self.state = 2218
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 2206
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2207
                self.match(VfrSyntaxParser.T__5)
                self.state = 2208
                self.flagsField()
                self.state = 2213
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 2209
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 2210
                    self.flagsField()
                    self.state = 2215
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 2216
                self.match(VfrSyntaxParser.Comma)


            self.state = 2220
            self.vfrStatementExpression(localctx.Node)
            self.state = 2221
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 2225
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 46)) & ~0x3f) == 0 and ((1 << (_la - 46)) & ((1 << (VfrSyntaxParser.OneOf - 46)) | (1 << (VfrSyntaxParser.OrderedList - 46)) | (1 << (VfrSyntaxParser.Subtitle - 46)) | (1 << (VfrSyntaxParser.Text - 46)) | (1 << (VfrSyntaxParser.Date - 46)) | (1 << (VfrSyntaxParser.Time - 46)) | (1 << (VfrSyntaxParser.GrayOutIf - 46)) | (1 << (VfrSyntaxParser.Label - 46)) | (1 << (VfrSyntaxParser.Inventory - 46)) | (1 << (VfrSyntaxParser.CheckBox - 46)) | (1 << (VfrSyntaxParser.Numeric - 46)) | (1 << (VfrSyntaxParser.Password - 46)) | (1 << (VfrSyntaxParser.String - 46)) | (1 << (VfrSyntaxParser.SuppressIf - 46)) | (1 << (VfrSyntaxParser.DisableIf - 46)) | (1 << (VfrSyntaxParser.Hidden - 46)) | (1 << (VfrSyntaxParser.Goto - 46)))) != 0) or ((((_la - 110)) & ~0x3f) == 0 and ((1 << (_la - 110)) & ((1 << (VfrSyntaxParser.InconsistentIf - 110)) | (1 << (VfrSyntaxParser.Restore - 110)) | (1 << (VfrSyntaxParser.Save - 110)) | (1 << (VfrSyntaxParser.ResetButton - 110)) | (1 << (VfrSyntaxParser.Action - 110)) | (1 << (VfrSyntaxParser.GuidOp - 110)))) != 0):
                self.state = 2222
                self.vfrStatementStatList()
                self.state = 2227
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2228
            self.match(VfrSyntaxParser.EndIf)
            self.state = 2229
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementGrayOutIfStatNewContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_GRAY_OUT_IF_OP)

        def GrayOutIf(self):
            return self.getToken(VfrSyntaxParser.GrayOutIf, 0)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def vfrStatementStatList(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementStatListContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementStatListContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementGrayOutIfStatNew

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementGrayOutIfStatNew" ):
                return visitor.visitVfrStatementGrayOutIfStatNew(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementGrayOutIfStatNew(self):

        localctx = VfrSyntaxParser.VfrStatementGrayOutIfStatNewContext(self, self._ctx, self.state)
        self.enterRule(localctx, 266, self.RULE_vfrStatementGrayOutIfStatNew)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2231
            self.match(VfrSyntaxParser.GrayOutIf)
            self.state = 2244
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 2232
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2233
                self.match(VfrSyntaxParser.T__5)
                self.state = 2234
                self.flagsField()
                self.state = 2239
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 2235
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 2236
                    self.flagsField()
                    self.state = 2241
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 2242
                self.match(VfrSyntaxParser.Comma)


            self.state = 2246
            self.vfrStatementExpression(localctx.Node)
            self.state = 2247
            self.match(VfrSyntaxParser.Semicolon)
            self.state = 2251
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while ((((_la - 46)) & ~0x3f) == 0 and ((1 << (_la - 46)) & ((1 << (VfrSyntaxParser.OneOf - 46)) | (1 << (VfrSyntaxParser.OrderedList - 46)) | (1 << (VfrSyntaxParser.Subtitle - 46)) | (1 << (VfrSyntaxParser.Text - 46)) | (1 << (VfrSyntaxParser.Date - 46)) | (1 << (VfrSyntaxParser.Time - 46)) | (1 << (VfrSyntaxParser.GrayOutIf - 46)) | (1 << (VfrSyntaxParser.Label - 46)) | (1 << (VfrSyntaxParser.Inventory - 46)) | (1 << (VfrSyntaxParser.CheckBox - 46)) | (1 << (VfrSyntaxParser.Numeric - 46)) | (1 << (VfrSyntaxParser.Password - 46)) | (1 << (VfrSyntaxParser.String - 46)) | (1 << (VfrSyntaxParser.SuppressIf - 46)) | (1 << (VfrSyntaxParser.DisableIf - 46)) | (1 << (VfrSyntaxParser.Hidden - 46)) | (1 << (VfrSyntaxParser.Goto - 46)))) != 0) or ((((_la - 110)) & ~0x3f) == 0 and ((1 << (_la - 110)) & ((1 << (VfrSyntaxParser.InconsistentIf - 110)) | (1 << (VfrSyntaxParser.Restore - 110)) | (1 << (VfrSyntaxParser.Save - 110)) | (1 << (VfrSyntaxParser.ResetButton - 110)) | (1 << (VfrSyntaxParser.Action - 110)) | (1 << (VfrSyntaxParser.GuidOp - 110)))) != 0):
                self.state = 2248
                self.vfrStatementStatList()
                self.state = 2253
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2254
            self.match(VfrSyntaxParser.EndIf)
            self.state = 2255
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementInconsistentIfStatContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP)

        def InconsistentIf(self):
            return self.getToken(VfrSyntaxParser.InconsistentIf, 0)

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementExpression(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,0)


        def EndIf(self):
            return self.getToken(VfrSyntaxParser.EndIf, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementInconsistentIfStat

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementInconsistentIfStat" ):
                return visitor.visitVfrStatementInconsistentIfStat(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementInconsistentIfStat(self):

        localctx = VfrSyntaxParser.VfrStatementInconsistentIfStatContext(self, self._ctx, self.state)
        self.enterRule(localctx, 268, self.RULE_vfrStatementInconsistentIfStat)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2257
            self.match(VfrSyntaxParser.InconsistentIf)
            self.state = 2258
            self.match(VfrSyntaxParser.Prompt)
            self.state = 2259
            self.match(VfrSyntaxParser.T__5)
            self.state = 2260
            self.match(VfrSyntaxParser.StringToken)
            self.state = 2261
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2262
            self.match(VfrSyntaxParser.Number)
            self.state = 2263
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2264
            self.match(VfrSyntaxParser.Comma)
            self.state = 2277
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.FLAGS:
                self.state = 2265
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2266
                self.match(VfrSyntaxParser.T__5)
                self.state = 2267
                self.flagsField()
                self.state = 2272
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 2268
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 2269
                    self.flagsField()
                    self.state = 2274
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 2275
                self.match(VfrSyntaxParser.Comma)


            self.state = 2279
            self.vfrStatementExpression(localctx.Node)
            self.state = 2280
            self.match(VfrSyntaxParser.EndIf)
            self.state = 2281
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementInvalidContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def vfrStatementInvalidHidden(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInvalidHiddenContext,0)


        def vfrStatementInvalidInventory(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInvalidInventoryContext,0)


        def vfrStatementInvalidSaveRestoreDefaults(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementInvalidSaveRestoreDefaultsContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementInvalid

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementInvalid" ):
                return visitor.visitVfrStatementInvalid(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementInvalid(self):

        localctx = VfrSyntaxParser.VfrStatementInvalidContext(self, self._ctx, self.state)
        self.enterRule(localctx, 270, self.RULE_vfrStatementInvalid)
        try:
            self.state = 2286
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Hidden]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2283
                self.vfrStatementInvalidHidden()
                pass
            elif token in [VfrSyntaxParser.Inventory]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2284
                self.vfrStatementInvalidInventory()
                pass
            elif token in [VfrSyntaxParser.Restore, VfrSyntaxParser.Save]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2285
                self.vfrStatementInvalidSaveRestoreDefaults()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementInvalidHiddenContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Hidden(self):
            return self.getToken(VfrSyntaxParser.Hidden, 0)

        def Value(self):
            return self.getToken(VfrSyntaxParser.Value, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementInvalidHidden

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementInvalidHidden" ):
                return visitor.visitVfrStatementInvalidHidden(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementInvalidHidden(self):

        localctx = VfrSyntaxParser.VfrStatementInvalidHiddenContext(self, self._ctx, self.state)
        self.enterRule(localctx, 272, self.RULE_vfrStatementInvalidHidden)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2288
            self.match(VfrSyntaxParser.Hidden)
            self.state = 2289
            self.match(VfrSyntaxParser.Value)
            self.state = 2290
            self.match(VfrSyntaxParser.T__5)
            self.state = 2291
            self.match(VfrSyntaxParser.Number)
            self.state = 2292
            self.match(VfrSyntaxParser.Comma)
            self.state = 2293
            self.match(VfrSyntaxParser.Key)
            self.state = 2294
            self.match(VfrSyntaxParser.T__5)
            self.state = 2295
            self.match(VfrSyntaxParser.Number)
            self.state = 2296
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementInvalidInventoryContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Inventory(self):
            return self.getToken(VfrSyntaxParser.Inventory, 0)

        def Help(self):
            return self.getToken(VfrSyntaxParser.Help, 0)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Text(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Text)
            else:
                return self.getToken(VfrSyntaxParser.Text, i)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementInvalidInventory

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementInvalidInventory" ):
                return visitor.visitVfrStatementInvalidInventory(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementInvalidInventory(self):

        localctx = VfrSyntaxParser.VfrStatementInvalidInventoryContext(self, self._ctx, self.state)
        self.enterRule(localctx, 274, self.RULE_vfrStatementInvalidInventory)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2298
            self.match(VfrSyntaxParser.Inventory)
            self.state = 2299
            self.match(VfrSyntaxParser.Help)
            self.state = 2300
            self.match(VfrSyntaxParser.T__5)
            self.state = 2301
            self.match(VfrSyntaxParser.StringToken)
            self.state = 2302
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2303
            self.match(VfrSyntaxParser.Number)
            self.state = 2304
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2305
            self.match(VfrSyntaxParser.Comma)
            self.state = 2306
            self.match(VfrSyntaxParser.Text)
            self.state = 2307
            self.match(VfrSyntaxParser.T__5)
            self.state = 2308
            self.match(VfrSyntaxParser.StringToken)
            self.state = 2309
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2310
            self.match(VfrSyntaxParser.Number)
            self.state = 2311
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2312
            self.match(VfrSyntaxParser.Comma)
            self.state = 2319
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Text:
                self.state = 2313
                self.match(VfrSyntaxParser.Text)
                self.state = 2314
                self.match(VfrSyntaxParser.T__5)
                self.state = 2315
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2316
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2317
                self.match(VfrSyntaxParser.Number)
                self.state = 2318
                self.match(VfrSyntaxParser.CloseParen)


            self.state = 2321
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementInvalidSaveRestoreDefaultsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Defaults(self):
            return self.getToken(VfrSyntaxParser.Defaults, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def FormId(self):
            return self.getToken(VfrSyntaxParser.FormId, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Prompt(self):
            return self.getToken(VfrSyntaxParser.Prompt, 0)

        def StringToken(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.StringToken)
            else:
                return self.getToken(VfrSyntaxParser.StringToken, i)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def Help(self):
            return self.getToken(VfrSyntaxParser.Help, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Save(self):
            return self.getToken(VfrSyntaxParser.Save, 0)

        def Restore(self):
            return self.getToken(VfrSyntaxParser.Restore, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def flagsField(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FlagsFieldContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FlagsFieldContext,i)


        def Key(self):
            return self.getToken(VfrSyntaxParser.Key, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementInvalidSaveRestoreDefaults

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementInvalidSaveRestoreDefaults" ):
                return visitor.visitVfrStatementInvalidSaveRestoreDefaults(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementInvalidSaveRestoreDefaults(self):

        localctx = VfrSyntaxParser.VfrStatementInvalidSaveRestoreDefaultsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 276, self.RULE_vfrStatementInvalidSaveRestoreDefaults)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2323
            _la = self._input.LA(1)
            if not(_la==VfrSyntaxParser.Restore or _la==VfrSyntaxParser.Save):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
            self.state = 2324
            self.match(VfrSyntaxParser.Defaults)
            self.state = 2325
            self.match(VfrSyntaxParser.Comma)
            self.state = 2326
            self.match(VfrSyntaxParser.FormId)
            self.state = 2327
            self.match(VfrSyntaxParser.T__5)
            self.state = 2328
            self.match(VfrSyntaxParser.Number)
            self.state = 2329
            self.match(VfrSyntaxParser.Comma)
            self.state = 2330
            self.match(VfrSyntaxParser.Prompt)
            self.state = 2331
            self.match(VfrSyntaxParser.T__5)
            self.state = 2332
            self.match(VfrSyntaxParser.StringToken)
            self.state = 2333
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2334
            self.match(VfrSyntaxParser.Number)
            self.state = 2335
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2336
            self.match(VfrSyntaxParser.Comma)
            self.state = 2337
            self.match(VfrSyntaxParser.Help)
            self.state = 2338
            self.match(VfrSyntaxParser.T__5)
            self.state = 2339
            self.match(VfrSyntaxParser.StringToken)
            self.state = 2340
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2341
            self.match(VfrSyntaxParser.Number)
            self.state = 2342
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2354
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,182,self._ctx)
            if la_ == 1:
                self.state = 2343
                self.match(VfrSyntaxParser.Comma)
                self.state = 2344
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2345
                self.match(VfrSyntaxParser.T__5)
                self.state = 2346
                self.flagsField()
                self.state = 2351
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.BitWiseOr:
                    self.state = 2347
                    self.match(VfrSyntaxParser.BitWiseOr)
                    self.state = 2348
                    self.flagsField()
                    self.state = 2353
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)



            self.state = 2360
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 2356
                self.match(VfrSyntaxParser.Comma)
                self.state = 2357
                self.match(VfrSyntaxParser.Key)
                self.state = 2358
                self.match(VfrSyntaxParser.T__5)
                self.state = 2359
                self.match(VfrSyntaxParser.Number)


            self.state = 2362
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementLabelContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_GUID_OP)

        def Label(self):
            return self.getToken(VfrSyntaxParser.Label, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementLabel

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementLabel" ):
                return visitor.visitVfrStatementLabel(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementLabel(self):

        localctx = VfrSyntaxParser.VfrStatementLabelContext(self, self._ctx, self.state)
        self.enterRule(localctx, 278, self.RULE_vfrStatementLabel)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2364
            self.match(VfrSyntaxParser.Label)
            self.state = 2365
            self.match(VfrSyntaxParser.Number)
            self.state = 2366
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementBannerContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_GUID_OP)

        def Banner(self):
            return self.getToken(VfrSyntaxParser.Banner, 0)

        def Title(self):
            return self.getToken(VfrSyntaxParser.Title, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Line(self):
            return self.getToken(VfrSyntaxParser.Line, 0)

        def Align(self):
            return self.getToken(VfrSyntaxParser.Align, 0)

        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Timeout(self):
            return self.getToken(VfrSyntaxParser.Timeout, 0)

        def Left(self):
            return self.getToken(VfrSyntaxParser.Left, 0)

        def Center(self):
            return self.getToken(VfrSyntaxParser.Center, 0)

        def Right(self):
            return self.getToken(VfrSyntaxParser.Right, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementBanner

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementBanner" ):
                return visitor.visitVfrStatementBanner(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementBanner(self):

        localctx = VfrSyntaxParser.VfrStatementBannerContext(self, self._ctx, self.state)
        self.enterRule(localctx, 280, self.RULE_vfrStatementBanner)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2368
            self.match(VfrSyntaxParser.Banner)
            self.state = 2370
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 2369
                self.match(VfrSyntaxParser.Comma)


            self.state = 2372
            self.match(VfrSyntaxParser.Title)
            self.state = 2373
            self.match(VfrSyntaxParser.T__5)
            self.state = 2374
            self.match(VfrSyntaxParser.StringToken)
            self.state = 2375
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2376
            self.match(VfrSyntaxParser.Number)
            self.state = 2377
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2378
            self.match(VfrSyntaxParser.Comma)
            self.state = 2389
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Line]:
                self.state = 2379
                self.match(VfrSyntaxParser.Line)
                self.state = 2380
                self.match(VfrSyntaxParser.Number)
                self.state = 2381
                self.match(VfrSyntaxParser.Comma)
                self.state = 2382
                self.match(VfrSyntaxParser.Align)
                self.state = 2383
                _la = self._input.LA(1)
                if not(((((_la - 136)) & ~0x3f) == 0 and ((1 << (_la - 136)) & ((1 << (VfrSyntaxParser.Left - 136)) | (1 << (VfrSyntaxParser.Right - 136)) | (1 << (VfrSyntaxParser.Center - 136)))) != 0)):
                    self._errHandler.recoverInline(self)
                else:
                    self._errHandler.reportMatch(self)
                    self.consume()
                self.state = 2384
                self.match(VfrSyntaxParser.Semicolon)
                pass
            elif token in [VfrSyntaxParser.Timeout]:
                self.state = 2385
                self.match(VfrSyntaxParser.Timeout)
                self.state = 2386
                self.match(VfrSyntaxParser.T__5)
                self.state = 2387
                self.match(VfrSyntaxParser.Number)
                self.state = 2388
                self.match(VfrSyntaxParser.Semicolon)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementExtensionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_GUID_OP)
            self.Buffer = None
            self.Size = 0
            self.TypeName = ''
            self.TypeSize = 0
            self.IsStruct = False
            self.ArrayNum = 0
            self.D = None # Token

        def GuidOp(self):
            return self.getToken(VfrSyntaxParser.GuidOp, 0)

        def Uuid(self):
            return self.getToken(VfrSyntaxParser.Uuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def EndGuidOp(self):
            return self.getToken(VfrSyntaxParser.EndGuidOp, 0)

        def DataType(self):
            return self.getToken(VfrSyntaxParser.DataType, 0)

        def Uint64(self):
            return self.getToken(VfrSyntaxParser.Uint64, 0)

        def Uint32(self):
            return self.getToken(VfrSyntaxParser.Uint32, 0)

        def Uint16(self):
            return self.getToken(VfrSyntaxParser.Uint16, 0)

        def Uint8(self):
            return self.getToken(VfrSyntaxParser.Uint8, 0)

        def Boolean(self):
            return self.getToken(VfrSyntaxParser.Boolean, 0)

        def EFI_STRING_ID(self):
            return self.getToken(VfrSyntaxParser.EFI_STRING_ID, 0)

        def EFI_HII_DATE(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_DATE, 0)

        def EFI_HII_TIME(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_TIME, 0)

        def EFI_HII_REF(self):
            return self.getToken(VfrSyntaxParser.EFI_HII_REF, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def vfrExtensionData(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrExtensionDataContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrExtensionDataContext,i)


        def vfrStatementExtension(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExtensionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExtensionContext,i)


        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementExtension

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementExtension" ):
                return visitor.visitVfrStatementExtension(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementExtension(self):

        localctx = VfrSyntaxParser.VfrStatementExtensionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 282, self.RULE_vfrStatementExtension)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2391
            self.match(VfrSyntaxParser.GuidOp)
            self.state = 2392
            self.match(VfrSyntaxParser.Uuid)
            self.state = 2393
            self.match(VfrSyntaxParser.T__5)
            self.state = 2394
            self.guidDefinition()
            self.state = 2466
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,198,self._ctx)
            if la_ == 1:
                self.state = 2395
                self.match(VfrSyntaxParser.Comma)
                self.state = 2396
                localctx.D = self.match(VfrSyntaxParser.DataType)
                self.state = 2397
                self.match(VfrSyntaxParser.T__5)
                self.state = 2458
                self._errHandler.sync(self)
                token = self._input.LA(1)
                if token in [VfrSyntaxParser.Uint64]:
                    self.state = 2398
                    self.match(VfrSyntaxParser.Uint64)
                    self.state = 2402
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2399
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2400
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2401
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.Uint32]:
                    self.state = 2404
                    self.match(VfrSyntaxParser.Uint32)
                    self.state = 2408
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2405
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2406
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2407
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.Uint16]:
                    self.state = 2410
                    self.match(VfrSyntaxParser.Uint16)
                    self.state = 2414
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2411
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2412
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2413
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.Uint8]:
                    self.state = 2416
                    self.match(VfrSyntaxParser.Uint8)
                    self.state = 2420
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2417
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2418
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2419
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.Boolean]:
                    self.state = 2422
                    self.match(VfrSyntaxParser.Boolean)
                    self.state = 2426
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2423
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2424
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2425
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.EFI_STRING_ID]:
                    self.state = 2428
                    self.match(VfrSyntaxParser.EFI_STRING_ID)
                    self.state = 2432
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2429
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2430
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2431
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.EFI_HII_DATE]:
                    self.state = 2434
                    self.match(VfrSyntaxParser.EFI_HII_DATE)
                    self.state = 2438
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2435
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2436
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2437
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.EFI_HII_TIME]:
                    self.state = 2440
                    self.match(VfrSyntaxParser.EFI_HII_TIME)
                    self.state = 2444
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2441
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2442
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2443
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.EFI_HII_REF]:
                    self.state = 2446
                    self.match(VfrSyntaxParser.EFI_HII_REF)
                    self.state = 2450
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2447
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2448
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2449
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                elif token in [VfrSyntaxParser.StringIdentifier]:
                    self.state = 2452
                    self.match(VfrSyntaxParser.StringIdentifier)
                    self.state = 2456
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==VfrSyntaxParser.OpenBracket:
                        self.state = 2453
                        self.match(VfrSyntaxParser.OpenBracket)
                        self.state = 2454
                        self.match(VfrSyntaxParser.Number)
                        self.state = 2455
                        self.match(VfrSyntaxParser.CloseBracket)


                    pass
                else:
                    raise NoViableAltException(self)

                self.state = 2463
                self._errHandler.sync(self)
                _alt = self._interp.adaptivePredict(self._input,197,self._ctx)
                while _alt!=2 and _alt!=ATN.INVALID_ALT_NUMBER:
                    if _alt==1:
                        self.state = 2460
                        self.vfrExtensionData()
                    self.state = 2465
                    self._errHandler.sync(self)
                    _alt = self._interp.adaptivePredict(self._input,197,self._ctx)



            self.state = 2476
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Comma:
                self.state = 2468
                self.match(VfrSyntaxParser.Comma)
                self.state = 2472
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.GuidOp:
                    self.state = 2469
                    self.vfrStatementExtension()
                    self.state = 2474
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                self.state = 2475
                self.match(VfrSyntaxParser.EndGuidOp)


            self.state = 2478
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExtensionDataContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.TFName = ''
            self.FName = ''
            self.TFValue = None
            self.I = None # Token
            self.N = None # Token

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def Data(self):
            return self.getToken(VfrSyntaxParser.Data, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def Dot(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Dot)
            else:
                return self.getToken(VfrSyntaxParser.Dot, i)

        def arrayName(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.ArrayNameContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.ArrayNameContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExtensionData

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExtensionData" ):
                return visitor.visitVfrExtensionData(self)
            else:
                return visitor.visitChildren(self)




    def vfrExtensionData(self):

        localctx = VfrSyntaxParser.VfrExtensionDataContext(self, self._ctx, self.state)
        self.enterRule(localctx, 284, self.RULE_vfrExtensionData)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2480
            self.match(VfrSyntaxParser.Comma)
            self.state = 2481
            self.match(VfrSyntaxParser.Data)
            self.state = 2485
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 2482
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 2483
                localctx.I = self.match(VfrSyntaxParser.Number)
                self.state = 2484
                self.match(VfrSyntaxParser.CloseBracket)


            self.state = 2491
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.Dot:
                self.state = 2487
                self.match(VfrSyntaxParser.Dot)
                self.state = 2488
                self.arrayName()
                self.state = 2493
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2494
            self.match(VfrSyntaxParser.T__5)
            self.state = 2495
            localctx.N = self.match(VfrSyntaxParser.Number)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementModalContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = None

        def vfrModalTag(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrModalTagContext,0)


        def Semicolon(self):
            return self.getToken(VfrSyntaxParser.Semicolon, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementModal

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementModal" ):
                return visitor.visitVfrStatementModal(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementModal(self):

        localctx = VfrSyntaxParser.VfrStatementModalContext(self, self._ctx, self.state)
        self.enterRule(localctx, 286, self.RULE_vfrStatementModal)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2497
            self.vfrModalTag()
            self.state = 2498
            self.match(VfrSyntaxParser.Semicolon)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrModalTagContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Node = VfrTreeNode(EFI_IFR_MODAL_TAG_OP)

        def Modal(self):
            return self.getToken(VfrSyntaxParser.Modal, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrModalTag

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrModalTag" ):
                return visitor.visitVfrModalTag(self)
            else:
                return visitor.visitChildren(self)




    def vfrModalTag(self):

        localctx = VfrSyntaxParser.VfrModalTagContext(self, self._ctx, self.state)
        self.enterRule(localctx, 288, self.RULE_vfrModalTag)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2500
            self.match(VfrSyntaxParser.Modal)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementExpressionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ParentNode=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ParentNode = None
            self.ExpInfo = ExpressionInfo()
            self.Nodes = []
            self.L = None # Token
            self.ParentNode = ParentNode

        def andTerm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.AndTermContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.AndTermContext,i)


        def OR(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OR)
            else:
                return self.getToken(VfrSyntaxParser.OR, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementExpression

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementExpression" ):
                return visitor.visitVfrStatementExpression(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementExpression(self, ParentNode):

        localctx = VfrSyntaxParser.VfrStatementExpressionContext(self, self._ctx, self.state, ParentNode)
        self.enterRule(localctx, 290, self.RULE_vfrStatementExpression)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2502
            self.andTerm(localctx.ExpInfo)
            self.state = 2507
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.OR:
                self.state = 2503
                localctx.L = self.match(VfrSyntaxParser.OR)
                self.state = 2504
                self.andTerm(localctx.ExpInfo)
                self.state = 2509
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrStatementExpressionSubContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ParentNodes=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ParentNodes = None
            self.ExpInfo = ExpressionInfo()
            self.Nodes = []
            self.ParentNodes = ParentNodes

        def andTerm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.AndTermContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.AndTermContext,i)


        def OR(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OR)
            else:
                return self.getToken(VfrSyntaxParser.OR, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrStatementExpressionSub

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrStatementExpressionSub" ):
                return visitor.visitVfrStatementExpressionSub(self)
            else:
                return visitor.visitChildren(self)




    def vfrStatementExpressionSub(self, ParentNodes):

        localctx = VfrSyntaxParser.VfrStatementExpressionSubContext(self, self._ctx, self.state, ParentNodes)
        self.enterRule(localctx, 292, self.RULE_vfrStatementExpressionSub)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2510
            self.andTerm(localctx.ExpInfo)
            self.state = 2515
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.OR:
                self.state = 2511
                self.match(VfrSyntaxParser.OR)
                self.state = 2512
                self.andTerm(localctx.ExpInfo)
                self.state = 2517
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class AndTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.Line = None
            self.L = None # Token
            self.ExpInfo = ExpInfo

        def bitwiseorTerm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.BitwiseorTermContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.BitwiseorTermContext,i)


        def AND(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.AND)
            else:
                return self.getToken(VfrSyntaxParser.AND, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_andTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitAndTerm" ):
                return visitor.visitAndTerm(self)
            else:
                return visitor.visitChildren(self)




    def andTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.AndTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 294, self.RULE_andTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2518
            self.bitwiseorTerm(ExpInfo)
            self.state = 2523
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.AND:
                self.state = 2519
                localctx.L = self.match(VfrSyntaxParser.AND)
                self.state = 2520
                self.bitwiseorTerm(ExpInfo)
                self.state = 2525
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class BitwiseorTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.Line = None
            self.L = None # Token
            self.ExpInfo = ExpInfo

        def bitwiseandTerm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.BitwiseandTermContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.BitwiseandTermContext,i)


        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_bitwiseorTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitBitwiseorTerm" ):
                return visitor.visitBitwiseorTerm(self)
            else:
                return visitor.visitChildren(self)




    def bitwiseorTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.BitwiseorTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 296, self.RULE_bitwiseorTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2526
            self.bitwiseandTerm(ExpInfo)
            self.state = 2531
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 2527
                localctx.L = self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 2528
                self.bitwiseandTerm(ExpInfo)
                self.state = 2533
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class BitwiseandTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.Line = None
            self.L = None # Token
            self.ExpInfo = ExpInfo

        def equalTerm(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.EqualTermContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.EqualTermContext,i)


        def BitWiseAnd(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseAnd)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseAnd, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_bitwiseandTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitBitwiseandTerm" ):
                return visitor.visitBitwiseandTerm(self)
            else:
                return visitor.visitChildren(self)




    def bitwiseandTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.BitwiseandTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 298, self.RULE_bitwiseandTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2534
            self.equalTerm(ExpInfo)
            self.state = 2539
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseAnd:
                self.state = 2535
                localctx.L = self.match(VfrSyntaxParser.BitWiseAnd)
                self.state = 2536
                self.equalTerm(ExpInfo)
                self.state = 2541
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class EqualTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.Line = None
            self.ExpInfo = ExpInfo

        def compareTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CompareTermContext,0)


        def equalTermSupplementary(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.EqualTermSupplementaryContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.EqualTermSupplementaryContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_equalTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitEqualTerm" ):
                return visitor.visitEqualTerm(self)
            else:
                return visitor.visitChildren(self)




    def equalTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.EqualTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 300, self.RULE_equalTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2542
            self.compareTerm(ExpInfo)
            self.state = 2546
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.Equal or _la==VfrSyntaxParser.NotEqual:
                self.state = 2543
                self.equalTermSupplementary(ExpInfo)
                self.state = 2548
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class EqualTermSupplementaryContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_equalTermSupplementary


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.ExpInfo = ctx.ExpInfo
            self.Nodes = ctx.Nodes



    class EqualTermEqualRuleContext(EqualTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.EqualTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def Equal(self):
            return self.getToken(VfrSyntaxParser.Equal, 0)
        def compareTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CompareTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitEqualTermEqualRule" ):
                return visitor.visitEqualTermEqualRule(self)
            else:
                return visitor.visitChildren(self)


    class EqualTermNotEqualRuleContext(EqualTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.EqualTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def NotEqual(self):
            return self.getToken(VfrSyntaxParser.NotEqual, 0)
        def compareTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CompareTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitEqualTermNotEqualRule" ):
                return visitor.visitEqualTermNotEqualRule(self)
            else:
                return visitor.visitChildren(self)



    def equalTermSupplementary(self, ExpInfo):

        localctx = VfrSyntaxParser.EqualTermSupplementaryContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 302, self.RULE_equalTermSupplementary)
        try:
            self.state = 2553
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Equal]:
                localctx = VfrSyntaxParser.EqualTermEqualRuleContext(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 2549
                self.match(VfrSyntaxParser.Equal)
                self.state = 2550
                self.compareTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.NotEqual]:
                localctx = VfrSyntaxParser.EqualTermNotEqualRuleContext(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 2551
                self.match(VfrSyntaxParser.NotEqual)
                self.state = 2552
                self.compareTerm(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class CompareTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def shiftTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ShiftTermContext,0)


        def compareTermSupplementary(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.CompareTermSupplementaryContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.CompareTermSupplementaryContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_compareTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCompareTerm" ):
                return visitor.visitCompareTerm(self)
            else:
                return visitor.visitChildren(self)




    def compareTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.CompareTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 304, self.RULE_compareTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2555
            self.shiftTerm(ExpInfo)
            self.state = 2559
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while (((_la) & ~0x3f) == 0 and ((1 << _la) & ((1 << VfrSyntaxParser.LessEqual) | (1 << VfrSyntaxParser.Less) | (1 << VfrSyntaxParser.GreaterEqual) | (1 << VfrSyntaxParser.Greater))) != 0):
                self.state = 2556
                self.compareTermSupplementary(ExpInfo)
                self.state = 2561
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class CompareTermSupplementaryContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_compareTermSupplementary


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.ExpInfo = ctx.ExpInfo
            self.Nodes = ctx.Nodes



    class CompareTermLessRuleContext(CompareTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.CompareTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def Less(self):
            return self.getToken(VfrSyntaxParser.Less, 0)
        def shiftTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ShiftTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCompareTermLessRule" ):
                return visitor.visitCompareTermLessRule(self)
            else:
                return visitor.visitChildren(self)


    class CompareTermGreaterEqualRuleContext(CompareTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.CompareTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def GreaterEqual(self):
            return self.getToken(VfrSyntaxParser.GreaterEqual, 0)
        def shiftTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ShiftTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCompareTermGreaterEqualRule" ):
                return visitor.visitCompareTermGreaterEqualRule(self)
            else:
                return visitor.visitChildren(self)


    class CompareTermGreaterRuleContext(CompareTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.CompareTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def Greater(self):
            return self.getToken(VfrSyntaxParser.Greater, 0)
        def shiftTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ShiftTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCompareTermGreaterRule" ):
                return visitor.visitCompareTermGreaterRule(self)
            else:
                return visitor.visitChildren(self)


    class CompareTermLessEqualRuleContext(CompareTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.CompareTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def LessEqual(self):
            return self.getToken(VfrSyntaxParser.LessEqual, 0)
        def shiftTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ShiftTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCompareTermLessEqualRule" ):
                return visitor.visitCompareTermLessEqualRule(self)
            else:
                return visitor.visitChildren(self)



    def compareTermSupplementary(self, ExpInfo):

        localctx = VfrSyntaxParser.CompareTermSupplementaryContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 306, self.RULE_compareTermSupplementary)
        try:
            self.state = 2570
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Less]:
                localctx = VfrSyntaxParser.CompareTermLessRuleContext(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 2562
                self.match(VfrSyntaxParser.Less)
                self.state = 2563
                self.shiftTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.LessEqual]:
                localctx = VfrSyntaxParser.CompareTermLessEqualRuleContext(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 2564
                self.match(VfrSyntaxParser.LessEqual)
                self.state = 2565
                self.shiftTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Greater]:
                localctx = VfrSyntaxParser.CompareTermGreaterRuleContext(self, localctx)
                self.enterOuterAlt(localctx, 3)
                self.state = 2566
                self.match(VfrSyntaxParser.Greater)
                self.state = 2567
                self.shiftTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.GreaterEqual]:
                localctx = VfrSyntaxParser.CompareTermGreaterEqualRuleContext(self, localctx)
                self.enterOuterAlt(localctx, 4)
                self.state = 2568
                self.match(VfrSyntaxParser.GreaterEqual)
                self.state = 2569
                self.shiftTerm(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ShiftTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def addMinusTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.AddMinusTermContext,0)


        def shiftTermSupplementary(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.ShiftTermSupplementaryContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.ShiftTermSupplementaryContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_shiftTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitShiftTerm" ):
                return visitor.visitShiftTerm(self)
            else:
                return visitor.visitChildren(self)




    def shiftTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.ShiftTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 308, self.RULE_shiftTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2572
            self.addMinusTerm(ExpInfo)
            self.state = 2576
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.T__9 or _la==VfrSyntaxParser.T__10:
                self.state = 2573
                self.shiftTermSupplementary(ExpInfo)
                self.state = 2578
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ShiftTermSupplementaryContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_shiftTermSupplementary


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.ExpInfo = ctx.ExpInfo
            self.Nodes = ctx.Nodes



    class ShiftTermRightContext(ShiftTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.ShiftTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def addMinusTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.AddMinusTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitShiftTermRight" ):
                return visitor.visitShiftTermRight(self)
            else:
                return visitor.visitChildren(self)


    class ShiftTermLeftContext(ShiftTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.ShiftTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def addMinusTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.AddMinusTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitShiftTermLeft" ):
                return visitor.visitShiftTermLeft(self)
            else:
                return visitor.visitChildren(self)



    def shiftTermSupplementary(self, ExpInfo):

        localctx = VfrSyntaxParser.ShiftTermSupplementaryContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 310, self.RULE_shiftTermSupplementary)
        try:
            self.state = 2583
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.T__9]:
                localctx = VfrSyntaxParser.ShiftTermLeftContext(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 2579
                self.match(VfrSyntaxParser.T__9)
                self.state = 2580
                self.addMinusTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.T__10]:
                localctx = VfrSyntaxParser.ShiftTermRightContext(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 2581
                self.match(VfrSyntaxParser.T__10)
                self.state = 2582
                self.addMinusTerm(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class AddMinusTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def multdivmodTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.MultdivmodTermContext,0)


        def addMinusTermSupplementary(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.AddMinusTermSupplementaryContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.AddMinusTermSupplementaryContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_addMinusTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitAddMinusTerm" ):
                return visitor.visitAddMinusTerm(self)
            else:
                return visitor.visitChildren(self)




    def addMinusTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.AddMinusTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 312, self.RULE_addMinusTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2585
            self.multdivmodTerm(ExpInfo)
            self.state = 2589
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.T__11 or _la==VfrSyntaxParser.Negative:
                self.state = 2586
                self.addMinusTermSupplementary(ExpInfo)
                self.state = 2591
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class AddMinusTermSupplementaryContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_addMinusTermSupplementary


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.ExpInfo = ctx.ExpInfo
            self.Nodes = ctx.Nodes



    class AddMinusTermpAddContext(AddMinusTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.AddMinusTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def multdivmodTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.MultdivmodTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitAddMinusTermpAdd" ):
                return visitor.visitAddMinusTermpAdd(self)
            else:
                return visitor.visitChildren(self)


    class AddMinusTermSubtractContext(AddMinusTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.AddMinusTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def Negative(self):
            return self.getToken(VfrSyntaxParser.Negative, 0)
        def multdivmodTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.MultdivmodTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitAddMinusTermSubtract" ):
                return visitor.visitAddMinusTermSubtract(self)
            else:
                return visitor.visitChildren(self)



    def addMinusTermSupplementary(self, ExpInfo):

        localctx = VfrSyntaxParser.AddMinusTermSupplementaryContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 314, self.RULE_addMinusTermSupplementary)
        try:
            self.state = 2596
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.T__11]:
                localctx = VfrSyntaxParser.AddMinusTermpAddContext(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 2592
                self.match(VfrSyntaxParser.T__11)
                self.state = 2593
                self.multdivmodTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Negative]:
                localctx = VfrSyntaxParser.AddMinusTermSubtractContext(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 2594
                self.match(VfrSyntaxParser.Negative)
                self.state = 2595
                self.multdivmodTerm(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class MultdivmodTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def castTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CastTermContext,0)


        def multdivmodTermSupplementary(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.MultdivmodTermSupplementaryContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.MultdivmodTermSupplementaryContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_multdivmodTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMultdivmodTerm" ):
                return visitor.visitMultdivmodTerm(self)
            else:
                return visitor.visitChildren(self)




    def multdivmodTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.MultdivmodTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 316, self.RULE_multdivmodTerm)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2598
            self.castTerm(ExpInfo)
            self.state = 2602
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while (((_la) & ~0x3f) == 0 and ((1 << _la) & ((1 << VfrSyntaxParser.T__12) | (1 << VfrSyntaxParser.T__13) | (1 << VfrSyntaxParser.Slash))) != 0):
                self.state = 2599
                self.multdivmodTermSupplementary(ExpInfo)
                self.state = 2604
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class MultdivmodTermSupplementaryContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_multdivmodTermSupplementary


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.ExpInfo = ctx.ExpInfo
            self.Nodes = ctx.Nodes



    class MultdivmodTermDivContext(MultdivmodTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.MultdivmodTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def Slash(self):
            return self.getToken(VfrSyntaxParser.Slash, 0)
        def castTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CastTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMultdivmodTermDiv" ):
                return visitor.visitMultdivmodTermDiv(self)
            else:
                return visitor.visitChildren(self)


    class MultdivmodTermMulContext(MultdivmodTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.MultdivmodTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def castTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CastTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMultdivmodTermMul" ):
                return visitor.visitMultdivmodTermMul(self)
            else:
                return visitor.visitChildren(self)


    class MultdivmodTermModuloContext(MultdivmodTermSupplementaryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.MultdivmodTermSupplementaryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def castTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.CastTermContext,0)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMultdivmodTermModulo" ):
                return visitor.visitMultdivmodTermModulo(self)
            else:
                return visitor.visitChildren(self)



    def multdivmodTermSupplementary(self, ExpInfo):

        localctx = VfrSyntaxParser.MultdivmodTermSupplementaryContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 318, self.RULE_multdivmodTermSupplementary)
        try:
            self.state = 2611
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.T__12]:
                localctx = VfrSyntaxParser.MultdivmodTermMulContext(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 2605
                self.match(VfrSyntaxParser.T__12)
                self.state = 2606
                self.castTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Slash]:
                localctx = VfrSyntaxParser.MultdivmodTermDivContext(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 2607
                self.match(VfrSyntaxParser.Slash)
                self.state = 2608
                self.castTerm(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.T__13]:
                localctx = VfrSyntaxParser.MultdivmodTermModuloContext(self, localctx)
                self.enterOuterAlt(localctx, 3)
                self.state = 2609
                self.match(VfrSyntaxParser.T__13)
                self.state = 2610
                self.castTerm(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class CastTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def atomTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.AtomTermContext,0)


        def castTermSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.CastTermSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.CastTermSubContext,i)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_castTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCastTerm" ):
                return visitor.visitCastTerm(self)
            else:
                return visitor.visitChildren(self)




    def castTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.CastTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 320, self.RULE_castTerm)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2616
            self._errHandler.sync(self)
            _alt = self._interp.adaptivePredict(self._input,218,self._ctx)
            while _alt!=2 and _alt!=ATN.INVALID_ALT_NUMBER:
                if _alt==1:
                    self.state = 2613
                    self.castTermSub()
                self.state = 2618
                self._errHandler.sync(self)
                _alt = self._interp.adaptivePredict(self._input,218,self._ctx)

            self.state = 2619
            self.atomTerm(ExpInfo)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class CastTermSubContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.CastType = 0xFF

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Boolean(self):
            return self.getToken(VfrSyntaxParser.Boolean, 0)

        def Uint64(self):
            return self.getToken(VfrSyntaxParser.Uint64, 0)

        def Uint32(self):
            return self.getToken(VfrSyntaxParser.Uint32, 0)

        def Uint16(self):
            return self.getToken(VfrSyntaxParser.Uint16, 0)

        def Uint8(self):
            return self.getToken(VfrSyntaxParser.Uint8, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_castTermSub

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitCastTermSub" ):
                return visitor.visitCastTermSub(self)
            else:
                return visitor.visitChildren(self)




    def castTermSub(self):

        localctx = VfrSyntaxParser.CastTermSubContext(self, self._ctx, self.state)
        self.enterRule(localctx, 322, self.RULE_castTermSub)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2621
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2622
            _la = self._input.LA(1)
            if not(((((_la - 80)) & ~0x3f) == 0 and ((1 << (_la - 80)) & ((1 << (VfrSyntaxParser.Boolean - 80)) | (1 << (VfrSyntaxParser.Uint64 - 80)) | (1 << (VfrSyntaxParser.Uint32 - 80)) | (1 << (VfrSyntaxParser.Uint16 - 80)) | (1 << (VfrSyntaxParser.Uint8 - 80)))) != 0)):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
            self.state = 2623
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class AtomTermContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def vfrExpressionCatenate(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionCatenateContext,0)


        def vfrExpressionMatch(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionMatchContext,0)


        def vfrExpressionMatch2(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionMatch2Context,0)


        def vfrExpressionParen(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionParenContext,0)


        def vfrExpressionBuildInFunction(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionBuildInFunctionContext,0)


        def vfrExpressionConstant(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionConstantContext,0)


        def vfrExpressionUnaryOp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionUnaryOpContext,0)


        def vfrExpressionTernaryOp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionTernaryOpContext,0)


        def vfrExpressionMap(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrExpressionMapContext,0)


        def NOT(self):
            return self.getToken(VfrSyntaxParser.NOT, 0)

        def atomTerm(self):
            return self.getTypedRuleContext(VfrSyntaxParser.AtomTermContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_atomTerm

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitAtomTerm" ):
                return visitor.visitAtomTerm(self)
            else:
                return visitor.visitChildren(self)




    def atomTerm(self, ExpInfo):

        localctx = VfrSyntaxParser.AtomTermContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 324, self.RULE_atomTerm)
        try:
            self.state = 2636
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Catenate]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2625
                self.vfrExpressionCatenate(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Match]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2626
                self.vfrExpressionMatch(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Match2]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2627
                self.vfrExpressionMatch2(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.OpenParen]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2628
                self.vfrExpressionParen(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Dup, VfrSyntaxParser.VarEqVal, VfrSyntaxParser.IdEqVal, VfrSyntaxParser.IdEqId, VfrSyntaxParser.IdEqValList, VfrSyntaxParser.QuestionRef, VfrSyntaxParser.RuleRef, VfrSyntaxParser.StringRef, VfrSyntaxParser.PushThis, VfrSyntaxParser.Security, VfrSyntaxParser.Get]:
                self.enterOuterAlt(localctx, 5)
                self.state = 2629
                self.vfrExpressionBuildInFunction(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.TrueSymbol, VfrSyntaxParser.FalseSymbol, VfrSyntaxParser.One, VfrSyntaxParser.Ones, VfrSyntaxParser.Zero, VfrSyntaxParser.Undefined, VfrSyntaxParser.Version, VfrSyntaxParser.Number]:
                self.enterOuterAlt(localctx, 6)
                self.state = 2630
                self.vfrExpressionConstant(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Length, VfrSyntaxParser.Set, VfrSyntaxParser.BitWiseNot, VfrSyntaxParser.BoolVal, VfrSyntaxParser.StringVal, VfrSyntaxParser.UnIntVal, VfrSyntaxParser.ToUpper, VfrSyntaxParser.ToLower, VfrSyntaxParser.QuestionRefVal, VfrSyntaxParser.StringRefVal]:
                self.enterOuterAlt(localctx, 7)
                self.state = 2631
                self.vfrExpressionUnaryOp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Cond, VfrSyntaxParser.Find, VfrSyntaxParser.Mid, VfrSyntaxParser.Tok, VfrSyntaxParser.Span]:
                self.enterOuterAlt(localctx, 8)
                self.state = 2632
                self.vfrExpressionTernaryOp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Map]:
                self.enterOuterAlt(localctx, 9)
                self.state = 2633
                self.vfrExpressionMap(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.NOT]:
                self.enterOuterAlt(localctx, 10)
                self.state = 2634
                self.match(VfrSyntaxParser.NOT)
                self.state = 2635
                self.atomTerm(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionCatenateContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Catenate(self):
            return self.getToken(VfrSyntaxParser.Catenate, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionCatenate

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionCatenate" ):
                return visitor.visitVfrExpressionCatenate(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionCatenate(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionCatenateContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 326, self.RULE_vfrExpressionCatenate)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2638
            self.match(VfrSyntaxParser.Catenate)
            self.state = 2639
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2640
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2641
            self.match(VfrSyntaxParser.Comma)
            self.state = 2642
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2643
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionMatchContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Match(self):
            return self.getToken(VfrSyntaxParser.Match, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionMatch

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionMatch" ):
                return visitor.visitVfrExpressionMatch(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionMatch(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionMatchContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 328, self.RULE_vfrExpressionMatch)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2645
            self.match(VfrSyntaxParser.Match)
            self.state = 2646
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2647
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2648
            self.match(VfrSyntaxParser.Comma)
            self.state = 2649
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2650
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionMatch2Context(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Match2(self):
            return self.getToken(VfrSyntaxParser.Match2, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionMatch2

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionMatch2" ):
                return visitor.visitVfrExpressionMatch2(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionMatch2(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionMatch2Context(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 330, self.RULE_vfrExpressionMatch2)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2652
            self.match(VfrSyntaxParser.Match2)
            self.state = 2653
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2654
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2655
            self.match(VfrSyntaxParser.Comma)
            self.state = 2656
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2657
            self.match(VfrSyntaxParser.Comma)
            self.state = 2658
            self.guidDefinition()
            self.state = 2659
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionParenContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionParen

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionParen" ):
                return visitor.visitVfrExpressionParen(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionParen(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionParenContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 332, self.RULE_vfrExpressionParen)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2661
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2662
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2663
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionBuildInFunctionContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = None
            self.ExpInfo = ExpInfo

        def dupExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.DupExpContext,0)


        def vareqvalExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VareqvalExpContext,0)


        def ideqvalExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.IdeqvalExpContext,0)


        def ideqidExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.IdeqidExpContext,0)


        def ideqvallistExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.IdeqvallistExpContext,0)


        def questionref1Exp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.Questionref1ExpContext,0)


        def rulerefExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.RulerefExpContext,0)


        def stringref1Exp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.Stringref1ExpContext,0)


        def pushthisExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.PushthisExpContext,0)


        def securityExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.SecurityExpContext,0)


        def getExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GetExpContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionBuildInFunction

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionBuildInFunction" ):
                return visitor.visitVfrExpressionBuildInFunction(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionBuildInFunction(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionBuildInFunctionContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 334, self.RULE_vfrExpressionBuildInFunction)
        try:
            self.state = 2676
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Dup]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2665
                self.dupExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.VarEqVal]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2666
                self.vareqvalExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.IdEqVal]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2667
                self.ideqvalExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.IdEqId]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2668
                self.ideqidExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.IdEqValList]:
                self.enterOuterAlt(localctx, 5)
                self.state = 2669
                self.ideqvallistExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.QuestionRef]:
                self.enterOuterAlt(localctx, 6)
                self.state = 2670
                self.questionref1Exp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.RuleRef]:
                self.enterOuterAlt(localctx, 7)
                self.state = 2671
                self.rulerefExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.StringRef]:
                self.enterOuterAlt(localctx, 8)
                self.state = 2672
                self.stringref1Exp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.PushThis]:
                self.enterOuterAlt(localctx, 9)
                self.state = 2673
                self.pushthisExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Security]:
                self.enterOuterAlt(localctx, 10)
                self.state = 2674
                self.securityExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Get]:
                self.enterOuterAlt(localctx, 11)
                self.state = 2675
                self.getExp(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class DupExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = VfrTreeNode(EFI_IFR_DUP_OP)
            self.ExpInfo = ExpInfo

        def Dup(self):
            return self.getToken(VfrSyntaxParser.Dup, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_dupExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitDupExp" ):
                return visitor.visitDupExp(self)
            else:
                return visitor.visitChildren(self)




    def dupExp(self, ExpInfo):

        localctx = VfrSyntaxParser.DupExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 336, self.RULE_dupExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2678
            self.match(VfrSyntaxParser.Dup)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VareqvalExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = None
            self.VN = None # Token
            self.ExpInfo = ExpInfo

        def VarEqVal(self):
            return self.getToken(VfrSyntaxParser.VarEqVal, 0)

        def Var(self):
            return self.getToken(VfrSyntaxParser.Var, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def Equal(self):
            return self.getToken(VfrSyntaxParser.Equal, 0)

        def LessEqual(self):
            return self.getToken(VfrSyntaxParser.LessEqual, 0)

        def Less(self):
            return self.getToken(VfrSyntaxParser.Less, 0)

        def GreaterEqual(self):
            return self.getToken(VfrSyntaxParser.GreaterEqual, 0)

        def Greater(self):
            return self.getToken(VfrSyntaxParser.Greater, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vareqvalExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVareqvalExp" ):
                return visitor.visitVareqvalExp(self)
            else:
                return visitor.visitChildren(self)




    def vareqvalExp(self, ExpInfo):

        localctx = VfrSyntaxParser.VareqvalExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 338, self.RULE_vareqvalExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2680
            self.match(VfrSyntaxParser.VarEqVal)
            self.state = 2681
            self.match(VfrSyntaxParser.Var)
            self.state = 2682
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2683
            localctx.VN = self.match(VfrSyntaxParser.Number)
            self.state = 2684
            self.match(VfrSyntaxParser.CloseParen)
            self.state = 2695
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Equal]:
                self.state = 2685
                self.match(VfrSyntaxParser.Equal)
                self.state = 2686
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.LessEqual]:
                self.state = 2687
                self.match(VfrSyntaxParser.LessEqual)
                self.state = 2688
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.Less]:
                self.state = 2689
                self.match(VfrSyntaxParser.Less)
                self.state = 2690
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.GreaterEqual]:
                self.state = 2691
                self.match(VfrSyntaxParser.GreaterEqual)
                self.state = 2692
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.Greater]:
                self.state = 2693
                self.match(VfrSyntaxParser.Greater)
                self.state = 2694
                self.match(VfrSyntaxParser.Number)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class IdeqvalExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = None
            self.I = None # Token
            self.ExpInfo = ExpInfo

        def vfrQuestionDataFieldName(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionDataFieldNameContext,0)


        def IdEqVal(self):
            return self.getToken(VfrSyntaxParser.IdEqVal, 0)

        def Equal(self):
            return self.getToken(VfrSyntaxParser.Equal, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def LessEqual(self):
            return self.getToken(VfrSyntaxParser.LessEqual, 0)

        def Less(self):
            return self.getToken(VfrSyntaxParser.Less, 0)

        def GreaterEqual(self):
            return self.getToken(VfrSyntaxParser.GreaterEqual, 0)

        def Greater(self):
            return self.getToken(VfrSyntaxParser.Greater, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_ideqvalExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitIdeqvalExp" ):
                return visitor.visitIdeqvalExp(self)
            else:
                return visitor.visitChildren(self)




    def ideqvalExp(self, ExpInfo):

        localctx = VfrSyntaxParser.IdeqvalExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 340, self.RULE_ideqvalExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2697
            localctx.I = self.match(VfrSyntaxParser.IdEqVal)
            self.state = 2698
            self.vfrQuestionDataFieldName()
            self.state = 2709
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Equal]:
                self.state = 2699
                self.match(VfrSyntaxParser.Equal)
                self.state = 2700
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.LessEqual]:
                self.state = 2701
                self.match(VfrSyntaxParser.LessEqual)
                self.state = 2702
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.Less]:
                self.state = 2703
                self.match(VfrSyntaxParser.Less)
                self.state = 2704
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.GreaterEqual]:
                self.state = 2705
                self.match(VfrSyntaxParser.GreaterEqual)
                self.state = 2706
                self.match(VfrSyntaxParser.Number)
                pass
            elif token in [VfrSyntaxParser.Greater]:
                self.state = 2707
                self.match(VfrSyntaxParser.Greater)
                self.state = 2708
                self.match(VfrSyntaxParser.Number)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class IdeqidExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = None
            self.I = None # Token
            self.E = None # Token
            self.LE = None # Token
            self.L = None # Token
            self.BE = None # Token
            self.B = None # Token
            self.ExpInfo = ExpInfo

        def vfrQuestionDataFieldName(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrQuestionDataFieldNameContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionDataFieldNameContext,i)


        def IdEqId(self):
            return self.getToken(VfrSyntaxParser.IdEqId, 0)

        def Equal(self):
            return self.getToken(VfrSyntaxParser.Equal, 0)

        def LessEqual(self):
            return self.getToken(VfrSyntaxParser.LessEqual, 0)

        def Less(self):
            return self.getToken(VfrSyntaxParser.Less, 0)

        def GreaterEqual(self):
            return self.getToken(VfrSyntaxParser.GreaterEqual, 0)

        def Greater(self):
            return self.getToken(VfrSyntaxParser.Greater, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_ideqidExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitIdeqidExp" ):
                return visitor.visitIdeqidExp(self)
            else:
                return visitor.visitChildren(self)




    def ideqidExp(self, ExpInfo):

        localctx = VfrSyntaxParser.IdeqidExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 342, self.RULE_ideqidExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2711
            localctx.I = self.match(VfrSyntaxParser.IdEqId)
            self.state = 2712
            self.vfrQuestionDataFieldName()
            self.state = 2723
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Equal]:
                self.state = 2713
                localctx.E = self.match(VfrSyntaxParser.Equal)
                self.state = 2714
                self.vfrQuestionDataFieldName()
                pass
            elif token in [VfrSyntaxParser.LessEqual]:
                self.state = 2715
                localctx.LE = self.match(VfrSyntaxParser.LessEqual)
                self.state = 2716
                self.vfrQuestionDataFieldName()
                pass
            elif token in [VfrSyntaxParser.Less]:
                self.state = 2717
                localctx.L = self.match(VfrSyntaxParser.Less)
                self.state = 2718
                self.vfrQuestionDataFieldName()
                pass
            elif token in [VfrSyntaxParser.GreaterEqual]:
                self.state = 2719
                localctx.BE = self.match(VfrSyntaxParser.GreaterEqual)
                self.state = 2720
                self.vfrQuestionDataFieldName()
                pass
            elif token in [VfrSyntaxParser.Greater]:
                self.state = 2721
                localctx.B = self.match(VfrSyntaxParser.Greater)
                self.state = 2722
                self.vfrQuestionDataFieldName()
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class IdeqvallistExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = None
            self.ExpInfo = ExpInfo

        def IdEqValList(self):
            return self.getToken(VfrSyntaxParser.IdEqValList, 0)

        def vfrQuestionDataFieldName(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrQuestionDataFieldNameContext,0)


        def Equal(self):
            return self.getToken(VfrSyntaxParser.Equal, 0)

        def Number(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Number)
            else:
                return self.getToken(VfrSyntaxParser.Number, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_ideqvallistExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitIdeqvallistExp" ):
                return visitor.visitIdeqvallistExp(self)
            else:
                return visitor.visitChildren(self)




    def ideqvallistExp(self, ExpInfo):

        localctx = VfrSyntaxParser.IdeqvallistExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 344, self.RULE_ideqvallistExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2725
            self.match(VfrSyntaxParser.IdEqValList)
            self.state = 2726
            self.vfrQuestionDataFieldName()
            self.state = 2727
            self.match(VfrSyntaxParser.Equal)
            self.state = 2729
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while True:
                self.state = 2728
                self.match(VfrSyntaxParser.Number)
                self.state = 2731
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if not (_la==VfrSyntaxParser.Number):
                    break

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrQuestionDataFieldNameContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.QId = EFI_QUESTION_ID_INVALID
            self.Mask = 0
            self.VarIdStr = ''
            self.Line = None


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrQuestionDataFieldName


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)
            self.QId = ctx.QId
            self.Mask = ctx.Mask
            self.VarIdStr = ctx.VarIdStr
            self.Line = ctx.Line



    class VfrQuestionDataFieldNameRule2Context(VfrQuestionDataFieldNameContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.VfrQuestionDataFieldNameContext
            super().__init__(parser)
            self.SN2 = None # Token
            self.copyFrom(ctx)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)
        def Dot(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Dot)
            else:
                return self.getToken(VfrSyntaxParser.Dot, i)
        def arrayName(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.ArrayNameContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.ArrayNameContext,i)


        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrQuestionDataFieldNameRule2" ):
                return visitor.visitVfrQuestionDataFieldNameRule2(self)
            else:
                return visitor.visitChildren(self)


    class VfrQuestionDataFieldNameRule1Context(VfrQuestionDataFieldNameContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a VfrSyntaxParser.VfrQuestionDataFieldNameContext
            super().__init__(parser)
            self.SN1 = None # Token
            self.I = None # Token
            self.copyFrom(ctx)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)
        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)
        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)
        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrQuestionDataFieldNameRule1" ):
                return visitor.visitVfrQuestionDataFieldNameRule1(self)
            else:
                return visitor.visitChildren(self)



    def vfrQuestionDataFieldName(self):

        localctx = VfrSyntaxParser.VfrQuestionDataFieldNameContext(self, self._ctx, self.state)
        self.enterRule(localctx, 346, self.RULE_vfrQuestionDataFieldName)
        self._la = 0 # Token type
        try:
            self.state = 2745
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,226,self._ctx)
            if la_ == 1:
                localctx = VfrSyntaxParser.VfrQuestionDataFieldNameRule1Context(self, localctx)
                self.enterOuterAlt(localctx, 1)
                self.state = 2733
                localctx.SN1 = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2734
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 2735
                localctx.I = self.match(VfrSyntaxParser.Number)
                self.state = 2736
                self.match(VfrSyntaxParser.CloseBracket)
                pass

            elif la_ == 2:
                localctx = VfrSyntaxParser.VfrQuestionDataFieldNameRule2Context(self, localctx)
                self.enterOuterAlt(localctx, 2)
                self.state = 2737
                localctx.SN2 = self.match(VfrSyntaxParser.StringIdentifier)
                self.state = 2742
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                while _la==VfrSyntaxParser.Dot:
                    self.state = 2738
                    self.match(VfrSyntaxParser.Dot)
                    self.state = 2739
                    self.arrayName()
                    self.state = 2744
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)

                pass


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ArrayNameContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.SubStr = ''
            self.SubStrZ = ''
            self.N = None # Token

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def OpenBracket(self):
            return self.getToken(VfrSyntaxParser.OpenBracket, 0)

        def CloseBracket(self):
            return self.getToken(VfrSyntaxParser.CloseBracket, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_arrayName

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitArrayName" ):
                return visitor.visitArrayName(self)
            else:
                return visitor.visitChildren(self)




    def arrayName(self):

        localctx = VfrSyntaxParser.ArrayNameContext(self, self._ctx, self.state)
        self.enterRule(localctx, 348, self.RULE_arrayName)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2747
            self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 2751
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.OpenBracket:
                self.state = 2748
                self.match(VfrSyntaxParser.OpenBracket)
                self.state = 2749
                localctx.N = self.match(VfrSyntaxParser.Number)
                self.state = 2750
                self.match(VfrSyntaxParser.CloseBracket)


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Questionref1ExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = VfrTreeNode(EFI_IFR_QUESTION_REF1_OP)
            self.ExpInfo = ExpInfo

        def QuestionRef(self):
            return self.getToken(VfrSyntaxParser.QuestionRef, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_questionref1Exp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitQuestionref1Exp" ):
                return visitor.visitQuestionref1Exp(self)
            else:
                return visitor.visitChildren(self)




    def questionref1Exp(self, ExpInfo):

        localctx = VfrSyntaxParser.Questionref1ExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 350, self.RULE_questionref1Exp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2753
            self.match(VfrSyntaxParser.QuestionRef)
            self.state = 2754
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2755
            _la = self._input.LA(1)
            if not(_la==VfrSyntaxParser.Number or _la==VfrSyntaxParser.StringIdentifier):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
            self.state = 2756
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class RulerefExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = VfrTreeNode(EFI_IFR_RULE_REF_OP)
            self.ExpInfo = ExpInfo

        def RuleRef(self):
            return self.getToken(VfrSyntaxParser.RuleRef, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def StringIdentifier(self):
            return self.getToken(VfrSyntaxParser.StringIdentifier, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_rulerefExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitRulerefExp" ):
                return visitor.visitRulerefExp(self)
            else:
                return visitor.visitChildren(self)




    def rulerefExp(self, ExpInfo):

        localctx = VfrSyntaxParser.RulerefExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 352, self.RULE_rulerefExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2758
            self.match(VfrSyntaxParser.RuleRef)
            self.state = 2759
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2760
            self.match(VfrSyntaxParser.StringIdentifier)
            self.state = 2761
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Stringref1ExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = VfrTreeNode(EFI_IFR_STRING_REF1_OP)
            self.ExpInfo = ExpInfo

        def StringRef(self):
            return self.getToken(VfrSyntaxParser.StringRef, 0)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_stringref1Exp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitStringref1Exp" ):
                return visitor.visitStringref1Exp(self)
            else:
                return visitor.visitChildren(self)




    def stringref1Exp(self, ExpInfo):

        localctx = VfrSyntaxParser.Stringref1ExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 354, self.RULE_stringref1Exp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2763
            self.match(VfrSyntaxParser.StringRef)
            self.state = 2764
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2770
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.StringToken]:
                self.state = 2765
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2766
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2767
                self.match(VfrSyntaxParser.Number)
                self.state = 2768
                self.match(VfrSyntaxParser.CloseParen)
                pass
            elif token in [VfrSyntaxParser.Number]:
                self.state = 2769
                self.match(VfrSyntaxParser.Number)
                pass
            else:
                raise NoViableAltException(self)

            self.state = 2772
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class PushthisExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = VfrTreeNode(EFI_IFR_THIS_OP)
            self.ExpInfo = ExpInfo

        def PushThis(self):
            return self.getToken(VfrSyntaxParser.PushThis, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_pushthisExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitPushthisExp" ):
                return visitor.visitPushthisExp(self)
            else:
                return visitor.visitChildren(self)




    def pushthisExp(self, ExpInfo):

        localctx = VfrSyntaxParser.PushthisExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 356, self.RULE_pushthisExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2774
            self.match(VfrSyntaxParser.PushThis)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class SecurityExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = VfrTreeNode(EFI_IFR_SECURITY_OP)
            self.ExpInfo = ExpInfo

        def Security(self):
            return self.getToken(VfrSyntaxParser.Security, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_securityExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitSecurityExp" ):
                return visitor.visitSecurityExp(self)
            else:
                return visitor.visitChildren(self)




    def securityExp(self, ExpInfo):

        localctx = VfrSyntaxParser.SecurityExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 358, self.RULE_securityExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2776
            self.match(VfrSyntaxParser.Security)
            self.state = 2777
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2778
            self.guidDefinition()
            self.state = 2779
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class NumericVarStoreTypeContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.VarType = None

        def NumericSizeOne(self):
            return self.getToken(VfrSyntaxParser.NumericSizeOne, 0)

        def NumericSizeTwo(self):
            return self.getToken(VfrSyntaxParser.NumericSizeTwo, 0)

        def NumericSizeFour(self):
            return self.getToken(VfrSyntaxParser.NumericSizeFour, 0)

        def NumericSizeEight(self):
            return self.getToken(VfrSyntaxParser.NumericSizeEight, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_numericVarStoreType

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitNumericVarStoreType" ):
                return visitor.visitNumericVarStoreType(self)
            else:
                return visitor.visitChildren(self)




    def numericVarStoreType(self):

        localctx = VfrSyntaxParser.NumericVarStoreTypeContext(self, self._ctx, self.state)
        self.enterRule(localctx, 360, self.RULE_numericVarStoreType)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2781
            _la = self._input.LA(1)
            if not(((((_la - 237)) & ~0x3f) == 0 and ((1 << (_la - 237)) & ((1 << (VfrSyntaxParser.NumericSizeOne - 237)) | (1 << (VfrSyntaxParser.NumericSizeTwo - 237)) | (1 << (VfrSyntaxParser.NumericSizeFour - 237)) | (1 << (VfrSyntaxParser.NumericSizeEight - 237)))) != 0)):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class GetExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.BaseInfo = EFI_VARSTORE_INFO()
            self.Node = VfrTreeNode(EFI_IFR_GET_OP)
            self.ExpInfo = ExpInfo

        def Get(self):
            return self.getToken(VfrSyntaxParser.Get, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStorageVarId(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStorageVarIdContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def BitWiseOr(self):
            return self.getToken(VfrSyntaxParser.BitWiseOr, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def numericVarStoreType(self):
            return self.getTypedRuleContext(VfrSyntaxParser.NumericVarStoreTypeContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_getExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitGetExp" ):
                return visitor.visitGetExp(self)
            else:
                return visitor.visitChildren(self)




    def getExp(self, ExpInfo):

        localctx = VfrSyntaxParser.GetExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 362, self.RULE_getExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2783
            self.match(VfrSyntaxParser.Get)
            self.state = 2784
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2785
            self.vfrStorageVarId(localctx.BaseInfo, False)
            self.state = 2790
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.BitWiseOr:
                self.state = 2786
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 2787
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2788
                self.match(VfrSyntaxParser.T__5)
                self.state = 2789
                self.numericVarStoreType()


            self.state = 2792
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionConstantContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Node = None
            self.ExpInfo = ExpInfo

        def TrueSymbol(self):
            return self.getToken(VfrSyntaxParser.TrueSymbol, 0)

        def FalseSymbol(self):
            return self.getToken(VfrSyntaxParser.FalseSymbol, 0)

        def One(self):
            return self.getToken(VfrSyntaxParser.One, 0)

        def Ones(self):
            return self.getToken(VfrSyntaxParser.Ones, 0)

        def Zero(self):
            return self.getToken(VfrSyntaxParser.Zero, 0)

        def Undefined(self):
            return self.getToken(VfrSyntaxParser.Undefined, 0)

        def Version(self):
            return self.getToken(VfrSyntaxParser.Version, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionConstant

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionConstant" ):
                return visitor.visitVfrExpressionConstant(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionConstant(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionConstantContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 364, self.RULE_vfrExpressionConstant)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2794
            _la = self._input.LA(1)
            if not(((((_la - 209)) & ~0x3f) == 0 and ((1 << (_la - 209)) & ((1 << (VfrSyntaxParser.TrueSymbol - 209)) | (1 << (VfrSyntaxParser.FalseSymbol - 209)) | (1 << (VfrSyntaxParser.One - 209)) | (1 << (VfrSyntaxParser.Ones - 209)) | (1 << (VfrSyntaxParser.Zero - 209)) | (1 << (VfrSyntaxParser.Undefined - 209)) | (1 << (VfrSyntaxParser.Version - 209)) | (1 << (VfrSyntaxParser.Number - 209)))) != 0)):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionUnaryOpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = None
            self.ExpInfo = ExpInfo

        def lengthExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.LengthExpContext,0)


        def bitwisenotExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.BitwisenotExpContext,0)


        def question23refExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.Question23refExpContext,0)


        def stringref2Exp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.Stringref2ExpContext,0)


        def toboolExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ToboolExpContext,0)


        def tostringExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.TostringExpContext,0)


        def unintExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.UnintExpContext,0)


        def toupperExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ToupperExpContext,0)


        def tolwerExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.TolwerExpContext,0)


        def setExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.SetExpContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionUnaryOp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionUnaryOp" ):
                return visitor.visitVfrExpressionUnaryOp(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionUnaryOp(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionUnaryOpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 366, self.RULE_vfrExpressionUnaryOp)
        try:
            self.state = 2806
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Length]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2796
                self.lengthExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.BitWiseNot]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2797
                self.bitwisenotExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.QuestionRefVal]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2798
                self.question23refExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.StringRefVal]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2799
                self.stringref2Exp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.BoolVal]:
                self.enterOuterAlt(localctx, 5)
                self.state = 2800
                self.toboolExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.StringVal]:
                self.enterOuterAlt(localctx, 6)
                self.state = 2801
                self.tostringExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.UnIntVal]:
                self.enterOuterAlt(localctx, 7)
                self.state = 2802
                self.unintExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.ToUpper]:
                self.enterOuterAlt(localctx, 8)
                self.state = 2803
                self.toupperExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.ToLower]:
                self.enterOuterAlt(localctx, 9)
                self.state = 2804
                self.tolwerExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Set]:
                self.enterOuterAlt(localctx, 10)
                self.state = 2805
                self.setExp(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class LengthExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Length(self):
            return self.getToken(VfrSyntaxParser.Length, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_lengthExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitLengthExp" ):
                return visitor.visitLengthExp(self)
            else:
                return visitor.visitChildren(self)




    def lengthExp(self, ExpInfo):

        localctx = VfrSyntaxParser.LengthExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 368, self.RULE_lengthExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2808
            self.match(VfrSyntaxParser.Length)
            self.state = 2809
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2810
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2811
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class BitwisenotExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def BitWiseNot(self):
            return self.getToken(VfrSyntaxParser.BitWiseNot, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_bitwisenotExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitBitwisenotExp" ):
                return visitor.visitBitwisenotExp(self)
            else:
                return visitor.visitChildren(self)




    def bitwisenotExp(self, ExpInfo):

        localctx = VfrSyntaxParser.BitwisenotExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 370, self.RULE_bitwisenotExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2813
            self.match(VfrSyntaxParser.BitWiseNot)
            self.state = 2814
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2815
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2816
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Question23refExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def QuestionRefVal(self):
            return self.getToken(VfrSyntaxParser.QuestionRefVal, 0)

        def OpenParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.OpenParen)
            else:
                return self.getToken(VfrSyntaxParser.OpenParen, i)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.CloseParen)
            else:
                return self.getToken(VfrSyntaxParser.CloseParen, i)

        def DevicePath(self):
            return self.getToken(VfrSyntaxParser.DevicePath, 0)

        def StringToken(self):
            return self.getToken(VfrSyntaxParser.StringToken, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Uuid(self):
            return self.getToken(VfrSyntaxParser.Uuid, 0)

        def guidDefinition(self):
            return self.getTypedRuleContext(VfrSyntaxParser.GuidDefinitionContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_question23refExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitQuestion23refExp" ):
                return visitor.visitQuestion23refExp(self)
            else:
                return visitor.visitChildren(self)




    def question23refExp(self, ExpInfo):

        localctx = VfrSyntaxParser.Question23refExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 372, self.RULE_question23refExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2818
            self.match(VfrSyntaxParser.QuestionRefVal)
            self.state = 2819
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2827
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.DevicePath:
                self.state = 2820
                self.match(VfrSyntaxParser.DevicePath)
                self.state = 2821
                self.match(VfrSyntaxParser.T__5)
                self.state = 2822
                self.match(VfrSyntaxParser.StringToken)
                self.state = 2823
                self.match(VfrSyntaxParser.OpenParen)
                self.state = 2824
                self.match(VfrSyntaxParser.Number)
                self.state = 2825
                self.match(VfrSyntaxParser.CloseParen)
                self.state = 2826
                self.match(VfrSyntaxParser.Comma)


            self.state = 2834
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.Uuid:
                self.state = 2829
                self.match(VfrSyntaxParser.Uuid)
                self.state = 2830
                self.match(VfrSyntaxParser.T__5)
                self.state = 2831
                self.guidDefinition()
                self.state = 2832
                self.match(VfrSyntaxParser.Comma)


            self.state = 2836
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2837
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Stringref2ExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def StringRefVal(self):
            return self.getToken(VfrSyntaxParser.StringRefVal, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_stringref2Exp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitStringref2Exp" ):
                return visitor.visitStringref2Exp(self)
            else:
                return visitor.visitChildren(self)




    def stringref2Exp(self, ExpInfo):

        localctx = VfrSyntaxParser.Stringref2ExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 374, self.RULE_stringref2Exp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2839
            self.match(VfrSyntaxParser.StringRefVal)
            self.state = 2840
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2841
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2842
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ToboolExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def BoolVal(self):
            return self.getToken(VfrSyntaxParser.BoolVal, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_toboolExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitToboolExp" ):
                return visitor.visitToboolExp(self)
            else:
                return visitor.visitChildren(self)




    def toboolExp(self, ExpInfo):

        localctx = VfrSyntaxParser.ToboolExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 376, self.RULE_toboolExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2844
            self.match(VfrSyntaxParser.BoolVal)
            self.state = 2845
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2846
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2847
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class TostringExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def StringVal(self):
            return self.getToken(VfrSyntaxParser.StringVal, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_tostringExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitTostringExp" ):
                return visitor.visitTostringExp(self)
            else:
                return visitor.visitChildren(self)




    def tostringExp(self, ExpInfo):

        localctx = VfrSyntaxParser.TostringExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 378, self.RULE_tostringExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2849
            self.match(VfrSyntaxParser.StringVal)
            self.state = 2854
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.T__14:
                self.state = 2850
                self.match(VfrSyntaxParser.T__14)
                self.state = 2851
                self.match(VfrSyntaxParser.T__5)
                self.state = 2852
                self.match(VfrSyntaxParser.Number)
                self.state = 2853
                self.match(VfrSyntaxParser.Comma)


            self.state = 2856
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2857
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2858
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class UnintExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def UnIntVal(self):
            return self.getToken(VfrSyntaxParser.UnIntVal, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_unintExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitUnintExp" ):
                return visitor.visitUnintExp(self)
            else:
                return visitor.visitChildren(self)




    def unintExp(self, ExpInfo):

        localctx = VfrSyntaxParser.UnintExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 380, self.RULE_unintExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2860
            self.match(VfrSyntaxParser.UnIntVal)
            self.state = 2861
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2862
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2863
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ToupperExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def ToUpper(self):
            return self.getToken(VfrSyntaxParser.ToUpper, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_toupperExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitToupperExp" ):
                return visitor.visitToupperExp(self)
            else:
                return visitor.visitChildren(self)




    def toupperExp(self, ExpInfo):

        localctx = VfrSyntaxParser.ToupperExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 382, self.RULE_toupperExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2865
            self.match(VfrSyntaxParser.ToUpper)
            self.state = 2866
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2867
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2868
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class TolwerExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def ToLower(self):
            return self.getToken(VfrSyntaxParser.ToLower, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_tolwerExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitTolwerExp" ):
                return visitor.visitTolwerExp(self)
            else:
                return visitor.visitChildren(self)




    def tolwerExp(self, ExpInfo):

        localctx = VfrSyntaxParser.TolwerExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 384, self.RULE_tolwerExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2870
            self.match(VfrSyntaxParser.ToLower)
            self.state = 2871
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2872
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2873
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class SetExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.BaseInfo = EFI_VARSTORE_INFO()
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Set(self):
            return self.getToken(VfrSyntaxParser.Set, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStorageVarId(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStorageVarIdContext,0)


        def Comma(self):
            return self.getToken(VfrSyntaxParser.Comma, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def BitWiseOr(self):
            return self.getToken(VfrSyntaxParser.BitWiseOr, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def numericVarStoreType(self):
            return self.getTypedRuleContext(VfrSyntaxParser.NumericVarStoreTypeContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_setExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitSetExp" ):
                return visitor.visitSetExp(self)
            else:
                return visitor.visitChildren(self)




    def setExp(self, ExpInfo):

        localctx = VfrSyntaxParser.SetExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 386, self.RULE_setExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2875
            self.match(VfrSyntaxParser.Set)
            self.state = 2876
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2877
            self.vfrStorageVarId(localctx.BaseInfo, False)
            self.state = 2882
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if _la==VfrSyntaxParser.BitWiseOr:
                self.state = 2878
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 2879
                self.match(VfrSyntaxParser.FLAGS)
                self.state = 2880
                self.match(VfrSyntaxParser.T__5)
                self.state = 2881
                self.numericVarStoreType()


            self.state = 2884
            self.match(VfrSyntaxParser.Comma)
            self.state = 2885
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2886
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionTernaryOpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = None
            self.ExpInfo = ExpInfo

        def conditionalExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.ConditionalExpContext,0)


        def findExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.FindExpContext,0)


        def midExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.MidExpContext,0)


        def tokenExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.TokenExpContext,0)


        def spanExp(self):
            return self.getTypedRuleContext(VfrSyntaxParser.SpanExpContext,0)


        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionTernaryOp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionTernaryOp" ):
                return visitor.visitVfrExpressionTernaryOp(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionTernaryOp(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionTernaryOpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 388, self.RULE_vfrExpressionTernaryOp)
        try:
            self.state = 2893
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [VfrSyntaxParser.Cond]:
                self.enterOuterAlt(localctx, 1)
                self.state = 2888
                self.conditionalExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Find]:
                self.enterOuterAlt(localctx, 2)
                self.state = 2889
                self.findExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Mid]:
                self.enterOuterAlt(localctx, 3)
                self.state = 2890
                self.midExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Tok]:
                self.enterOuterAlt(localctx, 4)
                self.state = 2891
                self.tokenExp(ExpInfo)
                pass
            elif token in [VfrSyntaxParser.Span]:
                self.enterOuterAlt(localctx, 5)
                self.state = 2892
                self.spanExp(ExpInfo)
                pass
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ConditionalExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Cond(self):
            return self.getToken(VfrSyntaxParser.Cond, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def Colon(self):
            return self.getToken(VfrSyntaxParser.Colon, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_conditionalExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitConditionalExp" ):
                return visitor.visitConditionalExp(self)
            else:
                return visitor.visitChildren(self)




    def conditionalExp(self, ExpInfo):

        localctx = VfrSyntaxParser.ConditionalExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 390, self.RULE_conditionalExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2895
            self.match(VfrSyntaxParser.Cond)
            self.state = 2896
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2897
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2898
            self.match(VfrSyntaxParser.T__15)
            self.state = 2899
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2900
            self.match(VfrSyntaxParser.Colon)
            self.state = 2901
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2902
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class FindExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Find(self):
            return self.getToken(VfrSyntaxParser.Find, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def findFormat(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.FindFormatContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.FindFormatContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_findExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitFindExp" ):
                return visitor.visitFindExp(self)
            else:
                return visitor.visitChildren(self)




    def findExp(self, ExpInfo):

        localctx = VfrSyntaxParser.FindExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 392, self.RULE_findExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2904
            self.match(VfrSyntaxParser.Find)
            self.state = 2905
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2906
            self.findFormat(ExpInfo)
            self.state = 2911
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 2907
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 2908
                self.findFormat(ExpInfo)
                self.state = 2913
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2914
            self.match(VfrSyntaxParser.Comma)
            self.state = 2915
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2916
            self.match(VfrSyntaxParser.Comma)
            self.state = 2917
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2918
            self.match(VfrSyntaxParser.Comma)
            self.state = 2919
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2920
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class FindFormatContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Format = 0
            self.ExpInfo = ExpInfo

        def Sensitive(self):
            return self.getToken(VfrSyntaxParser.Sensitive, 0)

        def Insensitive(self):
            return self.getToken(VfrSyntaxParser.Insensitive, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_findFormat

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitFindFormat" ):
                return visitor.visitFindFormat(self)
            else:
                return visitor.visitChildren(self)




    def findFormat(self, ExpInfo):

        localctx = VfrSyntaxParser.FindFormatContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 394, self.RULE_findFormat)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2922
            _la = self._input.LA(1)
            if not(_la==VfrSyntaxParser.Insensitive or _la==VfrSyntaxParser.Sensitive):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class MidExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Mid(self):
            return self.getToken(VfrSyntaxParser.Mid, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_midExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitMidExp" ):
                return visitor.visitMidExp(self)
            else:
                return visitor.visitChildren(self)




    def midExp(self, ExpInfo):

        localctx = VfrSyntaxParser.MidExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 396, self.RULE_midExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2924
            self.match(VfrSyntaxParser.Mid)
            self.state = 2925
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2926
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2927
            self.match(VfrSyntaxParser.Comma)
            self.state = 2928
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2929
            self.match(VfrSyntaxParser.Comma)
            self.state = 2930
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2931
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class TokenExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Tok(self):
            return self.getToken(VfrSyntaxParser.Tok, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_tokenExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitTokenExp" ):
                return visitor.visitTokenExp(self)
            else:
                return visitor.visitChildren(self)




    def tokenExp(self, ExpInfo):

        localctx = VfrSyntaxParser.TokenExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 398, self.RULE_tokenExp)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2933
            self.match(VfrSyntaxParser.Tok)
            self.state = 2934
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2935
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2936
            self.match(VfrSyntaxParser.Comma)
            self.state = 2937
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2938
            self.match(VfrSyntaxParser.Comma)
            self.state = 2939
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2940
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class SpanExpContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.ExpInfo = ExpInfo

        def Span(self):
            return self.getToken(VfrSyntaxParser.Span, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def FLAGS(self):
            return self.getToken(VfrSyntaxParser.FLAGS, 0)

        def spanFlags(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.SpanFlagsContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.SpanFlagsContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def vfrStatementExpressionSub(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionSubContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,i)


        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def BitWiseOr(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.BitWiseOr)
            else:
                return self.getToken(VfrSyntaxParser.BitWiseOr, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_spanExp

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitSpanExp" ):
                return visitor.visitSpanExp(self)
            else:
                return visitor.visitChildren(self)




    def spanExp(self, ExpInfo):

        localctx = VfrSyntaxParser.SpanExpContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 400, self.RULE_spanExp)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2942
            self.match(VfrSyntaxParser.Span)
            self.state = 2943
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2944
            self.match(VfrSyntaxParser.FLAGS)
            self.state = 2945
            self.match(VfrSyntaxParser.T__5)
            self.state = 2946
            self.spanFlags()
            self.state = 2951
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.BitWiseOr:
                self.state = 2947
                self.match(VfrSyntaxParser.BitWiseOr)
                self.state = 2948
                self.spanFlags()
                self.state = 2953
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2954
            self.match(VfrSyntaxParser.Comma)
            self.state = 2955
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2956
            self.match(VfrSyntaxParser.Comma)
            self.state = 2957
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2958
            self.match(VfrSyntaxParser.Comma)
            self.state = 2959
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2960
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class SpanFlagsContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.Flag = 0

        def Number(self):
            return self.getToken(VfrSyntaxParser.Number, 0)

        def LastNonMatch(self):
            return self.getToken(VfrSyntaxParser.LastNonMatch, 0)

        def FirstNonMatch(self):
            return self.getToken(VfrSyntaxParser.FirstNonMatch, 0)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_spanFlags

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitSpanFlags" ):
                return visitor.visitSpanFlags(self)
            else:
                return visitor.visitChildren(self)




    def spanFlags(self):

        localctx = VfrSyntaxParser.SpanFlagsContext(self, self._ctx, self.state)
        self.enterRule(localctx, 402, self.RULE_spanFlags)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2962
            _la = self._input.LA(1)
            if not(((((_la - 246)) & ~0x3f) == 0 and ((1 << (_la - 246)) & ((1 << (VfrSyntaxParser.LastNonMatch - 246)) | (1 << (VfrSyntaxParser.FirstNonMatch - 246)) | (1 << (VfrSyntaxParser.Number - 246)))) != 0)):
                self._errHandler.recoverInline(self)
            else:
                self._errHandler.reportMatch(self)
                self.consume()
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class VfrExpressionMapContext(ParserRuleContext):

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1, ExpInfo=None):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.ExpInfo = None
            self.Nodes = []
            self.Node = VfrTreeNode()
            self.ExpInfo = ExpInfo

        def Map(self):
            return self.getToken(VfrSyntaxParser.Map, 0)

        def OpenParen(self):
            return self.getToken(VfrSyntaxParser.OpenParen, 0)

        def vfrStatementExpressionSub(self):
            return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionSubContext,0)


        def Colon(self):
            return self.getToken(VfrSyntaxParser.Colon, 0)

        def CloseParen(self):
            return self.getToken(VfrSyntaxParser.CloseParen, 0)

        def vfrStatementExpression(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(VfrSyntaxParser.VfrStatementExpressionContext)
            else:
                return self.getTypedRuleContext(VfrSyntaxParser.VfrStatementExpressionContext,i)


        def Comma(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Comma)
            else:
                return self.getToken(VfrSyntaxParser.Comma, i)

        def Semicolon(self, i:int=None):
            if i is None:
                return self.getTokens(VfrSyntaxParser.Semicolon)
            else:
                return self.getToken(VfrSyntaxParser.Semicolon, i)

        def getRuleIndex(self):
            return VfrSyntaxParser.RULE_vfrExpressionMap

        def accept(self, visitor:ParseTreeVisitor):
            if hasattr( visitor, "visitVfrExpressionMap" ):
                return visitor.visitVfrExpressionMap(self)
            else:
                return visitor.visitChildren(self)




    def vfrExpressionMap(self, ExpInfo):

        localctx = VfrSyntaxParser.VfrExpressionMapContext(self, self._ctx, self.state, ExpInfo)
        self.enterRule(localctx, 404, self.RULE_vfrExpressionMap)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 2964
            self.match(VfrSyntaxParser.Map)
            self.state = 2965
            self.match(VfrSyntaxParser.OpenParen)
            self.state = 2966
            self.vfrStatementExpressionSub(localctx.Nodes)
            self.state = 2967
            self.match(VfrSyntaxParser.Colon)
            self.state = 2975
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==VfrSyntaxParser.OpenParen or ((((_la - 192)) & ~0x3f) == 0 and ((1 << (_la - 192)) & ((1 << (VfrSyntaxParser.Cond - 192)) | (1 << (VfrSyntaxParser.Find - 192)) | (1 << (VfrSyntaxParser.Mid - 192)) | (1 << (VfrSyntaxParser.Tok - 192)) | (1 << (VfrSyntaxParser.Span - 192)) | (1 << (VfrSyntaxParser.Dup - 192)) | (1 << (VfrSyntaxParser.VarEqVal - 192)) | (1 << (VfrSyntaxParser.IdEqVal - 192)) | (1 << (VfrSyntaxParser.IdEqId - 192)) | (1 << (VfrSyntaxParser.IdEqValList - 192)) | (1 << (VfrSyntaxParser.QuestionRef - 192)) | (1 << (VfrSyntaxParser.RuleRef - 192)) | (1 << (VfrSyntaxParser.StringRef - 192)) | (1 << (VfrSyntaxParser.PushThis - 192)) | (1 << (VfrSyntaxParser.Security - 192)) | (1 << (VfrSyntaxParser.Get - 192)) | (1 << (VfrSyntaxParser.TrueSymbol - 192)) | (1 << (VfrSyntaxParser.FalseSymbol - 192)) | (1 << (VfrSyntaxParser.One - 192)) | (1 << (VfrSyntaxParser.Ones - 192)) | (1 << (VfrSyntaxParser.Zero - 192)) | (1 << (VfrSyntaxParser.Undefined - 192)) | (1 << (VfrSyntaxParser.Version - 192)) | (1 << (VfrSyntaxParser.Length - 192)) | (1 << (VfrSyntaxParser.NOT - 192)) | (1 << (VfrSyntaxParser.Set - 192)) | (1 << (VfrSyntaxParser.BitWiseNot - 192)) | (1 << (VfrSyntaxParser.BoolVal - 192)) | (1 << (VfrSyntaxParser.StringVal - 192)) | (1 << (VfrSyntaxParser.UnIntVal - 192)) | (1 << (VfrSyntaxParser.ToUpper - 192)) | (1 << (VfrSyntaxParser.ToLower - 192)) | (1 << (VfrSyntaxParser.Match - 192)) | (1 << (VfrSyntaxParser.Match2 - 192)) | (1 << (VfrSyntaxParser.Catenate - 192)) | (1 << (VfrSyntaxParser.QuestionRefVal - 192)) | (1 << (VfrSyntaxParser.StringRefVal - 192)) | (1 << (VfrSyntaxParser.Map - 192)) | (1 << (VfrSyntaxParser.Number - 192)))) != 0):
                self.state = 2968
                self.vfrStatementExpression(localctx.Node)
                self.state = 2969
                self.match(VfrSyntaxParser.Comma)
                self.state = 2970
                self.vfrStatementExpression(localctx.Node)
                self.state = 2971
                self.match(VfrSyntaxParser.Semicolon)
                self.state = 2977
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 2978
            self.match(VfrSyntaxParser.CloseParen)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx





