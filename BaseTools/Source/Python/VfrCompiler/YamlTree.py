from VfrSyntaxVisitor import *
from VfrSyntaxLexer import *
from VfrSyntaxParser import *

class YamlTree():
    def __init__(self, Options: Options):
        self.Options = Options

    def ReadYaml(self):
        FileName = self.Options.YamlFileName
        try:
            f = open(FileName, 'r')
            #YamlDict = yaml.load(f, Loader=yaml.FullLoader)
            Config = yaml.safe_load(f)
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)
        #print(Config)
        if 'include' in Config.keys():
            self.ParseYamlHeader(Config['include'])

        if 'formset' in Config.keys():
            self.ParseYamlFormset(Config['formset'])

        # for Key in Config:
        #    print(Key)

    def ParseYamlFormset(self, FormsetValues):
        pass

    def ParseYamlHeader(self, includepaths):
        FileName = self.Options.OutputDirectory + self.Options.VfrBaseFileName + 'Header.vfr'
        try:
            f = open(FileName, 'w')
            for includepath in includepaths:
                f.write("#include <" + includepath + '>\n')
            f.close()
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)

        # call C preprocessor, gen .i file
        # delete .vfr

        # parse  and collect data structures info in the header files
        try:
            InputStream = FileStream(self.Options.HeaderFileName)
            VfrLexer = VfrSyntaxLexer(InputStream)
            VfrStream = CommonTokenStream(VfrLexer)
            VfrParser = VfrSyntaxParser(VfrStream)
            self.Visitor = VfrSyntaxVisitor(None, self.Options.OverrideClassGuid)
            self.Visitor.visit(VfrParser.vfrProgram())
        except:
            EdkLogger.error("VfrCompiler", FILE_OPEN_FAILURE,
                            "File open failed for %s" % FileName, None)
