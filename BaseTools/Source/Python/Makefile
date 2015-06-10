## @file
# Windows makefile for Python tools build.
#
# Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

!IFNDEF PYTHON_FREEZER_PATH
!ERROR PYTHON_FREEZER_PATH must be defined!
!ENDIF

!IF EXIST ($(PYTHON_FREEZER_PATH)\cxfreeze)
# Using cx_Freeze 4.2.3 with Python 2.7.2
FREEZE=$(PYTHON_FREEZER_PATH)\cxfreeze
!ELSE
# Using cx_Freeze 3.0.3 with Python 2.5.4
FREEZE=$(PYTHON_FREEZER_PATH)\FreezePython.exe
!ENDIF

MODULES=encodings.cp437,encodings.gbk,encodings.utf_16,encodings.utf_8,encodings.utf_16_le,encodings.latin_1,encodings.ascii

# DOS del command doesn't support ":\\" in the file path, such as j:\\BaseTools. Convert ":\\" to ":\"
BASE_TOOLS_PATH = $(BASE_TOOLS_PATH::\\=:\)
EDK_TOOLS_PATH  = $(EDK_TOOLS_PATH::\\=:\)

BIN_DIR=$(EDK_TOOLS_PATH)\Bin\Win32

APPLICATIONS=$(BIN_DIR)\build.exe $(BIN_DIR)\GenFds.exe $(BIN_DIR)\Trim.exe $(BIN_DIR)\TargetTool.exe $(BIN_DIR)\GenDepex.exe $(BIN_DIR)\GenPatchPcdTable.exe $(BIN_DIR)\PatchPcdValue.exe $(BIN_DIR)\BPDG.exe $(BIN_DIR)\UPT.exe $(BIN_DIR)\Rsa2048Sha256Sign.exe $(BIN_DIR)\Rsa2048Sha256GenerateKeys.exe $(BIN_DIR)\Ecc.exe

COMMON_PYTHON=$(BASE_TOOLS_PATH)\Source\Python\Common\BuildToolError.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Database.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\DataType.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\DecClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Dictionary.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\DscClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\EdkIIWorkspace.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\EdkIIWorkspaceBuild.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\EdkLogger.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Expression.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\FdfClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\FdfParserLite.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\GlobalData.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Identification.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\InfClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\LongFilePathOs.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\LongFilePathOsPath.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\LongFilePathSupport.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\MigrationUtilities.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Misc.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Parsing.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\String.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\TargetTxtClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\ToolDefClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\VpdInfoFile.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\BuildVersion.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\CommonClass.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\DataClass.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\Exceptions.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\FdfClass.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\ModuleClass.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\PackageClass.py \
              $(BASE_TOOLS_PATH)\Source\Python\CommonDataClass\PlatformClass.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\Table.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableDataModel.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableDec.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableDsc.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableEotReport.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableFdf.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableFile.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableFunction.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableIdentifier.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableInf.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TablePcd.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableQuery.py \
              $(BASE_TOOLS_PATH)\Source\Python\Table\TableReport.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\BuildClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaDataTable.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaFileCommentParser.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaFileParser.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaFileTable.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\WorkspaceCommon.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\WorkspaceDatabase.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\AutoGen.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\BuildEngine.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\GenC.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\GenDepex.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\GenMake.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\GenPcdDb.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\InfSectionParser.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\StrGather.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\UniClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\AutoGen\ValidCheckingInfoObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\RangeExpression.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\VariableAttributes.py

CMD_BUILD=$(BASE_TOOLS_PATH)\Source\Python\build\BuildReport.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenPatchPcdTable\GenPatchPcdTable.py \
    $(BASE_TOOLS_PATH)\Source\Python\PatchPcdValue\PatchPcdValue.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\c.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\CLexer.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\CodeFragment.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\CodeFragmentCollector.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\CParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\Database.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\EfiCompressor.pyd \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\Eot.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\EotGlobalData.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\EotToolError.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\FileProfile.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\FvImage.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\InfParserLite.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\LzmaCompressor.pyd \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\Parser.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\ParserWarning.py \
    $(BASE_TOOLS_PATH)\Source\Python\Eot\Report.py

CMD_GENFDS=$(BASE_TOOLS_PATH)\Source\Python\GenFds\AprioriSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Attribute.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Capsule.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\CapsuleData.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\ComponentStatement.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\CompressSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\DataSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\DepexSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\EfiSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Fd.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\FdfParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Ffs.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\FfsFileStatement.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\FfsInfStatement.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Fv.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\FvImageSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\GenFds.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\GenFdsGlobalVariable.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\GuidSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\OptionRom.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\OptRomFileStatement.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\OptRomInfStatement.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Region.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Rule.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\RuleComplexFile.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\RuleSimpleFile.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Section.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\UiSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\VerSection.py \
    $(BASE_TOOLS_PATH)\Source\Python\GenFds\Vtf.py

CMD_BPDG=$(BASE_TOOLS_PATH)\Source\Python\BPDG\GenVpd.py \
    $(BASE_TOOLS_PATH)\Source\Python\BPDG\StringTable.py

CMD_UPT=$(BASE_TOOLS_PATH)\Source\Python\UPT\Core\DependencyRules.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Core\DistributionPackageClass.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Core\FileHook.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Core\IpiDb.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Core\PackageFile.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Dll\sqlite3.dll \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\GenMetaFile\GenDecFile.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\GenMetaFile\GenInfFile.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\GenMetaFile\GenMetaFileMisc.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\GenMetaFile\GenXmlFile.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\InstallPkg.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\InventoryWs.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\CommentGenerating.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\CommentParsing.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\DataType.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\ExpressionValidate.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\GlobalData.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\Misc.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\ParserValidate.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\Parsing.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\String.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\UniClassObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Library\Xml\XmlRoutines.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Logger\Log.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Logger\StringTable.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Logger\ToolError.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\MkPkg.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\DecObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfBinaryObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfBuildOptionObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfCommonObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfDefineCommonObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfDefineObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfDepexObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfGuidObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfHeaderObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfLibraryClassesObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfMisc.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfPackagesObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfPcdObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfPpiObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfProtocolObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfSoucesObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\Parser\InfUserExtensionObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\POM\CommonObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\POM\ModuleObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Object\POM\PackageObject.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\DecParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\DecParserMisc.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfAsBuiltProcess.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfBinarySectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfBuildOptionSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfDefineSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfDepexSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfGuidPpiProtocolSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfLibrarySectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfPackageSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfParserMisc.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfPcdSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Parser\InfSourceSectionParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\PomAdapter\DecPomAlignment.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\PomAdapter\InfPomAlignment.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\PomAdapter\InfPomAlignmentMisc.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\ReplacePkg.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\RmPkg.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\UnitTest\CommentGeneratingUnitTest.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\UnitTest\CommentParsingUnitTest.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\UnitTest\DecParserTest.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\UnitTest\DecParserUnitTest.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\UnitTest\InfBinarySectionTest.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\CommonXml.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\GuidProtocolPpiXml.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\IniToXml.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\ModuleSurfaceAreaXml.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\PackageSurfaceAreaXml.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\PcdXml.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\XmlParser.py \
    $(BASE_TOOLS_PATH)\Source\Python\UPT\Xml\XmlParserMisc.py

all: SetPythonPath $(APPLICATIONS)

SetPythonPath:
  set PYTHONPATH=$(BASE_TOOLS_PATH)\Source\Python

$(BIN_DIR)\build.exe: $(BASE_TOOLS_PATH)\Source\Python\build\build.py $(COMMON_PYTHON) $(CMD_BUILD) $(CMD_GENFDS)
  @pushd . & @cd build & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) build.py & @popd

$(BIN_DIR)\GenFds.exe: $(BASE_TOOLS_PATH)\Source\Python\GenFds\GenFds.py $(COMMON_PYTHON) $(CMD_GENFDS)
  @pushd . & @cd GenFds & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) GenFds.py & @popd

$(BIN_DIR)\Trim.exe: $(BASE_TOOLS_PATH)\Source\Python\Trim\Trim.py $(COMMON_PYTHON)
  @pushd . & @cd Trim & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) Trim.py & @popd

$(BIN_DIR)\GenDepex.exe: $(BASE_TOOLS_PATH)\Source\Python\AutoGen\GenDepex.py $(COMMON_PYTHON)
  @pushd . & @cd AutoGen & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) GenDepex.py & @popd

$(BIN_DIR)\TargetTool.exe: $(BASE_TOOLS_PATH)\Source\Python\TargetTool\TargetTool.py $(COMMON_PYTHON)
  @pushd . & @cd TargetTool & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) TargetTool.py & @popd

$(BIN_DIR)\GenPatchPcdTable.exe: $(BASE_TOOLS_PATH)\Source\Python\GenPatchPcdTable\GenPatchPcdTable.py $(COMMON_PYTHON)
  @pushd . & @cd GenPatchPcdTable & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) GenPatchPcdTable.py & @popd

$(BIN_DIR)\PatchPcdValue.exe: $(BASE_TOOLS_PATH)\Source\Python\PatchPcdValue\PatchPcdValue.py $(COMMON_PYTHON)
  @pushd . & @cd PatchPcdValue & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) PatchPcdValue.py & @popd

$(BIN_DIR)\BPDG.exe: $(BASE_TOOLS_PATH)\Source\Python\BPDG\BPDG.py $(COMMON_PYTHON) $(CMD_BPDG)
  @pushd . & @cd BPDG & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) BPDG.py & @popd

$(BIN_DIR)\UPT.exe: $(BASE_TOOLS_PATH)\Source\Python\UPT\UPT.py $(CMD_UPT)
  @pushd . & @cd UPT & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) UPT.py & @popd

$(BIN_DIR)\Rsa2048Sha256Sign.exe: $(BASE_TOOLS_PATH)\Source\Python\Rsa2048Sha256Sign\Rsa2048Sha256Sign.py
  @pushd . & @cd Rsa2048Sha256Sign & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) Rsa2048Sha256Sign.py & @popd
  @pushd . & @copy /Y /B $(BASE_TOOLS_PATH)\Source\Python\Rsa2048Sha256Sign\TestSigningPrivateKey.pem $(BIN_DIR)\TestSigningPrivateKey.pem & @popd
  
$(BIN_DIR)\Rsa2048Sha256GenerateKeys.exe: $(BASE_TOOLS_PATH)\Source\Python\Rsa2048Sha256Sign\Rsa2048Sha256GenerateKeys.py
  @pushd . & @cd Rsa2048Sha256Sign & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) Rsa2048Sha256GenerateKeys.py & @popd

$(BIN_DIR)\Ecc.exe: $(BASE_TOOLS_PATH)\Source\Python\Ecc\Ecc.py $(CMD_ECC)
  @pushd . & @cd Ecc & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) Ecc.py & @popd
  @pushd . & @copy /Y /B $(BASE_TOOLS_PATH)\Source\Python\Ecc\config.ini $(BIN_DIR)\config.ini & @popd
  @pushd . & @copy /Y /B $(BASE_TOOLS_PATH)\Source\Python\Ecc\exception.xml $(BIN_DIR)\exception.xml & @popd
  
clean:
cleanall:  
  @del /f /q $(BIN_DIR)\*.pyd $(BIN_DIR)\*.dll
  @for %%i in ($(APPLICATIONS)) do @del /f /q %%i

