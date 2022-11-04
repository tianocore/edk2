# Generated from VfrSyntax.g4 by ANTLR 4.7.2
from antlr4 import *
if __name__ is not None and "." in __name__:
    from .VfrSyntaxParser import VfrSyntaxParser
else:
    from VfrSyntaxParser import VfrSyntaxParser


from CommonCtypes import *
from VfrFormPkg import *
from VfrUtility import *
from VfrTree import *


# This class defines a complete listener for a parse tree produced by VfrSyntaxParser.
class VfrSyntaxListener(ParseTreeListener):

    # Enter a parse tree produced by VfrSyntaxParser#vfrProgram.
    def enterVfrProgram(self, ctx:VfrSyntaxParser.VfrProgramContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrProgram.
    def exitVfrProgram(self, ctx:VfrSyntaxParser.VfrProgramContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#pragmaPackShowDef.
    def enterPragmaPackShowDef(self, ctx:VfrSyntaxParser.PragmaPackShowDefContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#pragmaPackShowDef.
    def exitPragmaPackShowDef(self, ctx:VfrSyntaxParser.PragmaPackShowDefContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#pragmaPackStackDef.
    def enterPragmaPackStackDef(self, ctx:VfrSyntaxParser.PragmaPackStackDefContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#pragmaPackStackDef.
    def exitPragmaPackStackDef(self, ctx:VfrSyntaxParser.PragmaPackStackDefContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#pragmaPackNumber.
    def enterPragmaPackNumber(self, ctx:VfrSyntaxParser.PragmaPackNumberContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#pragmaPackNumber.
    def exitPragmaPackNumber(self, ctx:VfrSyntaxParser.PragmaPackNumberContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrPragmaPackDefinition.
    def enterVfrPragmaPackDefinition(self, ctx:VfrSyntaxParser.VfrPragmaPackDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrPragmaPackDefinition.
    def exitVfrPragmaPackDefinition(self, ctx:VfrSyntaxParser.VfrPragmaPackDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrDataStructDefinition.
    def enterVfrDataStructDefinition(self, ctx:VfrSyntaxParser.VfrDataStructDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrDataStructDefinition.
    def exitVfrDataStructDefinition(self, ctx:VfrSyntaxParser.VfrDataStructDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrDataUnionDefinition.
    def enterVfrDataUnionDefinition(self, ctx:VfrSyntaxParser.VfrDataUnionDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrDataUnionDefinition.
    def exitVfrDataUnionDefinition(self, ctx:VfrSyntaxParser.VfrDataUnionDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrDataStructFields.
    def enterVfrDataStructFields(self, ctx:VfrSyntaxParser.VfrDataStructFieldsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrDataStructFields.
    def exitVfrDataStructFields(self, ctx:VfrSyntaxParser.VfrDataStructFieldsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructField64.
    def enterDataStructField64(self, ctx:VfrSyntaxParser.DataStructField64Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructField64.
    def exitDataStructField64(self, ctx:VfrSyntaxParser.DataStructField64Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructField32.
    def enterDataStructField32(self, ctx:VfrSyntaxParser.DataStructField32Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructField32.
    def exitDataStructField32(self, ctx:VfrSyntaxParser.DataStructField32Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructField16.
    def enterDataStructField16(self, ctx:VfrSyntaxParser.DataStructField16Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructField16.
    def exitDataStructField16(self, ctx:VfrSyntaxParser.DataStructField16Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructField8.
    def enterDataStructField8(self, ctx:VfrSyntaxParser.DataStructField8Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructField8.
    def exitDataStructField8(self, ctx:VfrSyntaxParser.DataStructField8Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructFieldBool.
    def enterDataStructFieldBool(self, ctx:VfrSyntaxParser.DataStructFieldBoolContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructFieldBool.
    def exitDataStructFieldBool(self, ctx:VfrSyntaxParser.DataStructFieldBoolContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructFieldString.
    def enterDataStructFieldString(self, ctx:VfrSyntaxParser.DataStructFieldStringContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructFieldString.
    def exitDataStructFieldString(self, ctx:VfrSyntaxParser.DataStructFieldStringContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructFieldDate.
    def enterDataStructFieldDate(self, ctx:VfrSyntaxParser.DataStructFieldDateContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructFieldDate.
    def exitDataStructFieldDate(self, ctx:VfrSyntaxParser.DataStructFieldDateContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructFieldTime.
    def enterDataStructFieldTime(self, ctx:VfrSyntaxParser.DataStructFieldTimeContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructFieldTime.
    def exitDataStructFieldTime(self, ctx:VfrSyntaxParser.DataStructFieldTimeContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructFieldRef.
    def enterDataStructFieldRef(self, ctx:VfrSyntaxParser.DataStructFieldRefContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructFieldRef.
    def exitDataStructFieldRef(self, ctx:VfrSyntaxParser.DataStructFieldRefContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructFieldUser.
    def enterDataStructFieldUser(self, ctx:VfrSyntaxParser.DataStructFieldUserContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructFieldUser.
    def exitDataStructFieldUser(self, ctx:VfrSyntaxParser.DataStructFieldUserContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructBitField64.
    def enterDataStructBitField64(self, ctx:VfrSyntaxParser.DataStructBitField64Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructBitField64.
    def exitDataStructBitField64(self, ctx:VfrSyntaxParser.DataStructBitField64Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructBitField32.
    def enterDataStructBitField32(self, ctx:VfrSyntaxParser.DataStructBitField32Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructBitField32.
    def exitDataStructBitField32(self, ctx:VfrSyntaxParser.DataStructBitField32Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructBitField16.
    def enterDataStructBitField16(self, ctx:VfrSyntaxParser.DataStructBitField16Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructBitField16.
    def exitDataStructBitField16(self, ctx:VfrSyntaxParser.DataStructBitField16Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dataStructBitField8.
    def enterDataStructBitField8(self, ctx:VfrSyntaxParser.DataStructBitField8Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dataStructBitField8.
    def exitDataStructBitField8(self, ctx:VfrSyntaxParser.DataStructBitField8Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrFormSetDefinition.
    def enterVfrFormSetDefinition(self, ctx:VfrSyntaxParser.VfrFormSetDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrFormSetDefinition.
    def exitVfrFormSetDefinition(self, ctx:VfrSyntaxParser.VfrFormSetDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#classguidDefinition.
    def enterClassguidDefinition(self, ctx:VfrSyntaxParser.ClassguidDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#classguidDefinition.
    def exitClassguidDefinition(self, ctx:VfrSyntaxParser.ClassguidDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#classDefinition.
    def enterClassDefinition(self, ctx:VfrSyntaxParser.ClassDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#classDefinition.
    def exitClassDefinition(self, ctx:VfrSyntaxParser.ClassDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#validClassNames.
    def enterValidClassNames(self, ctx:VfrSyntaxParser.ValidClassNamesContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#validClassNames.
    def exitValidClassNames(self, ctx:VfrSyntaxParser.ValidClassNamesContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#subclassDefinition.
    def enterSubclassDefinition(self, ctx:VfrSyntaxParser.SubclassDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#subclassDefinition.
    def exitSubclassDefinition(self, ctx:VfrSyntaxParser.SubclassDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrFormSetList.
    def enterVfrFormSetList(self, ctx:VfrSyntaxParser.VfrFormSetListContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrFormSetList.
    def exitVfrFormSetList(self, ctx:VfrSyntaxParser.VfrFormSetListContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrFormSet.
    def enterVfrFormSet(self, ctx:VfrSyntaxParser.VfrFormSetContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrFormSet.
    def exitVfrFormSet(self, ctx:VfrSyntaxParser.VfrFormSetContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementDefaultStore.
    def enterVfrStatementDefaultStore(self, ctx:VfrSyntaxParser.VfrStatementDefaultStoreContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementDefaultStore.
    def exitVfrStatementDefaultStore(self, ctx:VfrSyntaxParser.VfrStatementDefaultStoreContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreLinear.
    def enterVfrStatementVarStoreLinear(self, ctx:VfrSyntaxParser.VfrStatementVarStoreLinearContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreLinear.
    def exitVfrStatementVarStoreLinear(self, ctx:VfrSyntaxParser.VfrStatementVarStoreLinearContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreEfi.
    def enterVfrStatementVarStoreEfi(self, ctx:VfrSyntaxParser.VfrStatementVarStoreEfiContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreEfi.
    def exitVfrStatementVarStoreEfi(self, ctx:VfrSyntaxParser.VfrStatementVarStoreEfiContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrVarStoreEfiAttr.
    def enterVfrVarStoreEfiAttr(self, ctx:VfrSyntaxParser.VfrVarStoreEfiAttrContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrVarStoreEfiAttr.
    def exitVfrVarStoreEfiAttr(self, ctx:VfrSyntaxParser.VfrVarStoreEfiAttrContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreNameValue.
    def enterVfrStatementVarStoreNameValue(self, ctx:VfrSyntaxParser.VfrStatementVarStoreNameValueContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementVarStoreNameValue.
    def exitVfrStatementVarStoreNameValue(self, ctx:VfrSyntaxParser.VfrStatementVarStoreNameValueContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfFormSet.
    def enterVfrStatementDisableIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementDisableIfFormSetContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfFormSet.
    def exitVfrStatementDisableIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementDisableIfFormSetContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfFormSet.
    def enterVfrStatementSuppressIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfFormSetContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfFormSet.
    def exitVfrStatementSuppressIfFormSet(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfFormSetContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#guidSubDefinition.
    def enterGuidSubDefinition(self, ctx:VfrSyntaxParser.GuidSubDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#guidSubDefinition.
    def exitGuidSubDefinition(self, ctx:VfrSyntaxParser.GuidSubDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#guidDefinition.
    def enterGuidDefinition(self, ctx:VfrSyntaxParser.GuidDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#guidDefinition.
    def exitGuidDefinition(self, ctx:VfrSyntaxParser.GuidDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#getStringId.
    def enterGetStringId(self, ctx:VfrSyntaxParser.GetStringIdContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#getStringId.
    def exitGetStringId(self, ctx:VfrSyntaxParser.GetStringIdContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrQuestionHeader.
    def enterVfrQuestionHeader(self, ctx:VfrSyntaxParser.VfrQuestionHeaderContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrQuestionHeader.
    def exitVfrQuestionHeader(self, ctx:VfrSyntaxParser.VfrQuestionHeaderContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrQuestionBaseInfo.
    def enterVfrQuestionBaseInfo(self, ctx:VfrSyntaxParser.VfrQuestionBaseInfoContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrQuestionBaseInfo.
    def exitVfrQuestionBaseInfo(self, ctx:VfrSyntaxParser.VfrQuestionBaseInfoContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementHeader.
    def enterVfrStatementHeader(self, ctx:VfrSyntaxParser.VfrStatementHeaderContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementHeader.
    def exitVfrStatementHeader(self, ctx:VfrSyntaxParser.VfrStatementHeaderContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#questionheaderFlagsField.
    def enterQuestionheaderFlagsField(self, ctx:VfrSyntaxParser.QuestionheaderFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#questionheaderFlagsField.
    def exitQuestionheaderFlagsField(self, ctx:VfrSyntaxParser.QuestionheaderFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStorageVarIdRule1.
    def enterVfrStorageVarIdRule1(self, ctx:VfrSyntaxParser.VfrStorageVarIdRule1Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStorageVarIdRule1.
    def exitVfrStorageVarIdRule1(self, ctx:VfrSyntaxParser.VfrStorageVarIdRule1Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStorageVarIdRule2.
    def enterVfrStorageVarIdRule2(self, ctx:VfrSyntaxParser.VfrStorageVarIdRule2Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStorageVarIdRule2.
    def exitVfrStorageVarIdRule2(self, ctx:VfrSyntaxParser.VfrStorageVarIdRule2Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrConstantValueField.
    def enterVfrConstantValueField(self, ctx:VfrSyntaxParser.VfrConstantValueFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrConstantValueField.
    def exitVfrConstantValueField(self, ctx:VfrSyntaxParser.VfrConstantValueFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrImageTag.
    def enterVfrImageTag(self, ctx:VfrSyntaxParser.VfrImageTagContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrImageTag.
    def exitVfrImageTag(self, ctx:VfrSyntaxParser.VfrImageTagContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrLockedTag.
    def enterVfrLockedTag(self, ctx:VfrSyntaxParser.VfrLockedTagContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrLockedTag.
    def exitVfrLockedTag(self, ctx:VfrSyntaxParser.VfrLockedTagContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStatTag.
    def enterVfrStatementStatTag(self, ctx:VfrSyntaxParser.VfrStatementStatTagContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStatTag.
    def exitVfrStatementStatTag(self, ctx:VfrSyntaxParser.VfrStatementStatTagContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStatTagList.
    def enterVfrStatementStatTagList(self, ctx:VfrSyntaxParser.VfrStatementStatTagListContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStatTagList.
    def exitVfrStatementStatTagList(self, ctx:VfrSyntaxParser.VfrStatementStatTagListContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrFormDefinition.
    def enterVfrFormDefinition(self, ctx:VfrSyntaxParser.VfrFormDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrFormDefinition.
    def exitVfrFormDefinition(self, ctx:VfrSyntaxParser.VfrFormDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrForm.
    def enterVfrForm(self, ctx:VfrSyntaxParser.VfrFormContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrForm.
    def exitVfrForm(self, ctx:VfrSyntaxParser.VfrFormContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrFormMapDefinition.
    def enterVfrFormMapDefinition(self, ctx:VfrSyntaxParser.VfrFormMapDefinitionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrFormMapDefinition.
    def exitVfrFormMapDefinition(self, ctx:VfrSyntaxParser.VfrFormMapDefinitionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementImage.
    def enterVfrStatementImage(self, ctx:VfrSyntaxParser.VfrStatementImageContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementImage.
    def exitVfrStatementImage(self, ctx:VfrSyntaxParser.VfrStatementImageContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementLocked.
    def enterVfrStatementLocked(self, ctx:VfrSyntaxParser.VfrStatementLockedContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementLocked.
    def exitVfrStatementLocked(self, ctx:VfrSyntaxParser.VfrStatementLockedContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementRules.
    def enterVfrStatementRules(self, ctx:VfrSyntaxParser.VfrStatementRulesContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementRules.
    def exitVfrStatementRules(self, ctx:VfrSyntaxParser.VfrStatementRulesContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStat.
    def enterVfrStatementStat(self, ctx:VfrSyntaxParser.VfrStatementStatContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStat.
    def exitVfrStatementStat(self, ctx:VfrSyntaxParser.VfrStatementStatContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementSubTitle.
    def enterVfrStatementSubTitle(self, ctx:VfrSyntaxParser.VfrStatementSubTitleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementSubTitle.
    def exitVfrStatementSubTitle(self, ctx:VfrSyntaxParser.VfrStatementSubTitleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementSubTitleComponent.
    def enterVfrStatementSubTitleComponent(self, ctx:VfrSyntaxParser.VfrStatementSubTitleComponentContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementSubTitleComponent.
    def exitVfrStatementSubTitleComponent(self, ctx:VfrSyntaxParser.VfrStatementSubTitleComponentContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrSubtitleFlags.
    def enterVfrSubtitleFlags(self, ctx:VfrSyntaxParser.VfrSubtitleFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrSubtitleFlags.
    def exitVfrSubtitleFlags(self, ctx:VfrSyntaxParser.VfrSubtitleFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#subtitleFlagsField.
    def enterSubtitleFlagsField(self, ctx:VfrSyntaxParser.SubtitleFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#subtitleFlagsField.
    def exitSubtitleFlagsField(self, ctx:VfrSyntaxParser.SubtitleFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStaticText.
    def enterVfrStatementStaticText(self, ctx:VfrSyntaxParser.VfrStatementStaticTextContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStaticText.
    def exitVfrStatementStaticText(self, ctx:VfrSyntaxParser.VfrStatementStaticTextContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#staticTextFlagsField.
    def enterStaticTextFlagsField(self, ctx:VfrSyntaxParser.StaticTextFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#staticTextFlagsField.
    def exitStaticTextFlagsField(self, ctx:VfrSyntaxParser.StaticTextFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementCrossReference.
    def enterVfrStatementCrossReference(self, ctx:VfrSyntaxParser.VfrStatementCrossReferenceContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementCrossReference.
    def exitVfrStatementCrossReference(self, ctx:VfrSyntaxParser.VfrStatementCrossReferenceContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementGoto.
    def enterVfrStatementGoto(self, ctx:VfrSyntaxParser.VfrStatementGotoContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementGoto.
    def exitVfrStatementGoto(self, ctx:VfrSyntaxParser.VfrStatementGotoContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrGotoFlags.
    def enterVfrGotoFlags(self, ctx:VfrSyntaxParser.VfrGotoFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrGotoFlags.
    def exitVfrGotoFlags(self, ctx:VfrSyntaxParser.VfrGotoFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#gotoFlagsField.
    def enterGotoFlagsField(self, ctx:VfrSyntaxParser.GotoFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#gotoFlagsField.
    def exitGotoFlagsField(self, ctx:VfrSyntaxParser.GotoFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementResetButton.
    def enterVfrStatementResetButton(self, ctx:VfrSyntaxParser.VfrStatementResetButtonContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementResetButton.
    def exitVfrStatementResetButton(self, ctx:VfrSyntaxParser.VfrStatementResetButtonContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementQuestions.
    def enterVfrStatementQuestions(self, ctx:VfrSyntaxParser.VfrStatementQuestionsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementQuestions.
    def exitVfrStatementQuestions(self, ctx:VfrSyntaxParser.VfrStatementQuestionsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTag.
    def enterVfrStatementQuestionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTag.
    def exitVfrStatementQuestionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIf.
    def enterVfrStatementInconsistentIf(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIf.
    def exitVfrStatementInconsistentIf(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementNoSubmitIf.
    def enterVfrStatementNoSubmitIf(self, ctx:VfrSyntaxParser.VfrStatementNoSubmitIfContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementNoSubmitIf.
    def exitVfrStatementNoSubmitIf(self, ctx:VfrSyntaxParser.VfrStatementNoSubmitIfContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfQuest.
    def enterVfrStatementDisableIfQuest(self, ctx:VfrSyntaxParser.VfrStatementDisableIfQuestContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfQuest.
    def exitVfrStatementDisableIfQuest(self, ctx:VfrSyntaxParser.VfrStatementDisableIfQuestContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementRefresh.
    def enterVfrStatementRefresh(self, ctx:VfrSyntaxParser.VfrStatementRefreshContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementRefresh.
    def exitVfrStatementRefresh(self, ctx:VfrSyntaxParser.VfrStatementRefreshContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementVarstoreDevice.
    def enterVfrStatementVarstoreDevice(self, ctx:VfrSyntaxParser.VfrStatementVarstoreDeviceContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementVarstoreDevice.
    def exitVfrStatementVarstoreDevice(self, ctx:VfrSyntaxParser.VfrStatementVarstoreDeviceContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementRefreshEvent.
    def enterVfrStatementRefreshEvent(self, ctx:VfrSyntaxParser.VfrStatementRefreshEventContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementRefreshEvent.
    def exitVfrStatementRefreshEvent(self, ctx:VfrSyntaxParser.VfrStatementRefreshEventContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementWarningIf.
    def enterVfrStatementWarningIf(self, ctx:VfrSyntaxParser.VfrStatementWarningIfContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementWarningIf.
    def exitVfrStatementWarningIf(self, ctx:VfrSyntaxParser.VfrStatementWarningIfContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTagList.
    def enterVfrStatementQuestionTagList(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagListContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionTagList.
    def exitVfrStatementQuestionTagList(self, ctx:VfrSyntaxParser.VfrStatementQuestionTagListContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionTag.
    def enterVfrStatementQuestionOptionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionTagContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionTag.
    def exitVfrStatementQuestionOptionTag(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionTagContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#flagsField.
    def enterFlagsField(self, ctx:VfrSyntaxParser.FlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#flagsField.
    def exitFlagsField(self, ctx:VfrSyntaxParser.FlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfQuest.
    def enterVfrStatementSuppressIfQuest(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfQuestContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfQuest.
    def exitVfrStatementSuppressIfQuest(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfQuestContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfQuest.
    def enterVfrStatementGrayOutIfQuest(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfQuestContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfQuest.
    def exitVfrStatementGrayOutIfQuest(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfQuestContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementDefault.
    def enterVfrStatementDefault(self, ctx:VfrSyntaxParser.VfrStatementDefaultContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementDefault.
    def exitVfrStatementDefault(self, ctx:VfrSyntaxParser.VfrStatementDefaultContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementValue.
    def enterVfrStatementValue(self, ctx:VfrSyntaxParser.VfrStatementValueContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementValue.
    def exitVfrStatementValue(self, ctx:VfrSyntaxParser.VfrStatementValueContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementOptions.
    def enterVfrStatementOptions(self, ctx:VfrSyntaxParser.VfrStatementOptionsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementOptions.
    def exitVfrStatementOptions(self, ctx:VfrSyntaxParser.VfrStatementOptionsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementOneOfOption.
    def enterVfrStatementOneOfOption(self, ctx:VfrSyntaxParser.VfrStatementOneOfOptionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementOneOfOption.
    def exitVfrStatementOneOfOption(self, ctx:VfrSyntaxParser.VfrStatementOneOfOptionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrOneOfOptionFlags.
    def enterVfrOneOfOptionFlags(self, ctx:VfrSyntaxParser.VfrOneOfOptionFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrOneOfOptionFlags.
    def exitVfrOneOfOptionFlags(self, ctx:VfrSyntaxParser.VfrOneOfOptionFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#oneofoptionFlagsField.
    def enterOneofoptionFlagsField(self, ctx:VfrSyntaxParser.OneofoptionFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#oneofoptionFlagsField.
    def exitOneofoptionFlagsField(self, ctx:VfrSyntaxParser.OneofoptionFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementRead.
    def enterVfrStatementRead(self, ctx:VfrSyntaxParser.VfrStatementReadContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementRead.
    def exitVfrStatementRead(self, ctx:VfrSyntaxParser.VfrStatementReadContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementWrite.
    def enterVfrStatementWrite(self, ctx:VfrSyntaxParser.VfrStatementWriteContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementWrite.
    def exitVfrStatementWrite(self, ctx:VfrSyntaxParser.VfrStatementWriteContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionList.
    def enterVfrStatementQuestionOptionList(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionListContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOptionList.
    def exitVfrStatementQuestionOptionList(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionListContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOption.
    def enterVfrStatementQuestionOption(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementQuestionOption.
    def exitVfrStatementQuestionOption(self, ctx:VfrSyntaxParser.VfrStatementQuestionOptionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementBooleanType.
    def enterVfrStatementBooleanType(self, ctx:VfrSyntaxParser.VfrStatementBooleanTypeContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementBooleanType.
    def exitVfrStatementBooleanType(self, ctx:VfrSyntaxParser.VfrStatementBooleanTypeContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementCheckBox.
    def enterVfrStatementCheckBox(self, ctx:VfrSyntaxParser.VfrStatementCheckBoxContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementCheckBox.
    def exitVfrStatementCheckBox(self, ctx:VfrSyntaxParser.VfrStatementCheckBoxContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrCheckBoxFlags.
    def enterVfrCheckBoxFlags(self, ctx:VfrSyntaxParser.VfrCheckBoxFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrCheckBoxFlags.
    def exitVfrCheckBoxFlags(self, ctx:VfrSyntaxParser.VfrCheckBoxFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#checkboxFlagsField.
    def enterCheckboxFlagsField(self, ctx:VfrSyntaxParser.CheckboxFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#checkboxFlagsField.
    def exitCheckboxFlagsField(self, ctx:VfrSyntaxParser.CheckboxFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementAction.
    def enterVfrStatementAction(self, ctx:VfrSyntaxParser.VfrStatementActionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementAction.
    def exitVfrStatementAction(self, ctx:VfrSyntaxParser.VfrStatementActionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrActionFlags.
    def enterVfrActionFlags(self, ctx:VfrSyntaxParser.VfrActionFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrActionFlags.
    def exitVfrActionFlags(self, ctx:VfrSyntaxParser.VfrActionFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#actionFlagsField.
    def enterActionFlagsField(self, ctx:VfrSyntaxParser.ActionFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#actionFlagsField.
    def exitActionFlagsField(self, ctx:VfrSyntaxParser.ActionFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementNumericType.
    def enterVfrStatementNumericType(self, ctx:VfrSyntaxParser.VfrStatementNumericTypeContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementNumericType.
    def exitVfrStatementNumericType(self, ctx:VfrSyntaxParser.VfrStatementNumericTypeContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementNumeric.
    def enterVfrStatementNumeric(self, ctx:VfrSyntaxParser.VfrStatementNumericContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementNumeric.
    def exitVfrStatementNumeric(self, ctx:VfrSyntaxParser.VfrStatementNumericContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrSetMinMaxStep.
    def enterVfrSetMinMaxStep(self, ctx:VfrSyntaxParser.VfrSetMinMaxStepContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrSetMinMaxStep.
    def exitVfrSetMinMaxStep(self, ctx:VfrSyntaxParser.VfrSetMinMaxStepContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrNumericFlags.
    def enterVfrNumericFlags(self, ctx:VfrSyntaxParser.VfrNumericFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrNumericFlags.
    def exitVfrNumericFlags(self, ctx:VfrSyntaxParser.VfrNumericFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#numericFlagsField.
    def enterNumericFlagsField(self, ctx:VfrSyntaxParser.NumericFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#numericFlagsField.
    def exitNumericFlagsField(self, ctx:VfrSyntaxParser.NumericFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementOneOf.
    def enterVfrStatementOneOf(self, ctx:VfrSyntaxParser.VfrStatementOneOfContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementOneOf.
    def exitVfrStatementOneOf(self, ctx:VfrSyntaxParser.VfrStatementOneOfContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrOneofFlagsField.
    def enterVfrOneofFlagsField(self, ctx:VfrSyntaxParser.VfrOneofFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrOneofFlagsField.
    def exitVfrOneofFlagsField(self, ctx:VfrSyntaxParser.VfrOneofFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStringType.
    def enterVfrStatementStringType(self, ctx:VfrSyntaxParser.VfrStatementStringTypeContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStringType.
    def exitVfrStatementStringType(self, ctx:VfrSyntaxParser.VfrStatementStringTypeContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementString.
    def enterVfrStatementString(self, ctx:VfrSyntaxParser.VfrStatementStringContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementString.
    def exitVfrStatementString(self, ctx:VfrSyntaxParser.VfrStatementStringContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStringFlagsField.
    def enterVfrStringFlagsField(self, ctx:VfrSyntaxParser.VfrStringFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStringFlagsField.
    def exitVfrStringFlagsField(self, ctx:VfrSyntaxParser.VfrStringFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#stringFlagsField.
    def enterStringFlagsField(self, ctx:VfrSyntaxParser.StringFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#stringFlagsField.
    def exitStringFlagsField(self, ctx:VfrSyntaxParser.StringFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementPassword.
    def enterVfrStatementPassword(self, ctx:VfrSyntaxParser.VfrStatementPasswordContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementPassword.
    def exitVfrStatementPassword(self, ctx:VfrSyntaxParser.VfrStatementPasswordContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrPasswordFlagsField.
    def enterVfrPasswordFlagsField(self, ctx:VfrSyntaxParser.VfrPasswordFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrPasswordFlagsField.
    def exitVfrPasswordFlagsField(self, ctx:VfrSyntaxParser.VfrPasswordFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#passwordFlagsField.
    def enterPasswordFlagsField(self, ctx:VfrSyntaxParser.PasswordFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#passwordFlagsField.
    def exitPasswordFlagsField(self, ctx:VfrSyntaxParser.PasswordFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementOrderedList.
    def enterVfrStatementOrderedList(self, ctx:VfrSyntaxParser.VfrStatementOrderedListContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementOrderedList.
    def exitVfrStatementOrderedList(self, ctx:VfrSyntaxParser.VfrStatementOrderedListContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrOrderedListFlags.
    def enterVfrOrderedListFlags(self, ctx:VfrSyntaxParser.VfrOrderedListFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrOrderedListFlags.
    def exitVfrOrderedListFlags(self, ctx:VfrSyntaxParser.VfrOrderedListFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#orderedlistFlagsField.
    def enterOrderedlistFlagsField(self, ctx:VfrSyntaxParser.OrderedlistFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#orderedlistFlagsField.
    def exitOrderedlistFlagsField(self, ctx:VfrSyntaxParser.OrderedlistFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementDate.
    def enterVfrStatementDate(self, ctx:VfrSyntaxParser.VfrStatementDateContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementDate.
    def exitVfrStatementDate(self, ctx:VfrSyntaxParser.VfrStatementDateContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#minMaxDateStepDefault.
    def enterMinMaxDateStepDefault(self, ctx:VfrSyntaxParser.MinMaxDateStepDefaultContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#minMaxDateStepDefault.
    def exitMinMaxDateStepDefault(self, ctx:VfrSyntaxParser.MinMaxDateStepDefaultContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrDateFlags.
    def enterVfrDateFlags(self, ctx:VfrSyntaxParser.VfrDateFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrDateFlags.
    def exitVfrDateFlags(self, ctx:VfrSyntaxParser.VfrDateFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dateFlagsField.
    def enterDateFlagsField(self, ctx:VfrSyntaxParser.DateFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dateFlagsField.
    def exitDateFlagsField(self, ctx:VfrSyntaxParser.DateFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementTime.
    def enterVfrStatementTime(self, ctx:VfrSyntaxParser.VfrStatementTimeContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementTime.
    def exitVfrStatementTime(self, ctx:VfrSyntaxParser.VfrStatementTimeContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#minMaxTimeStepDefault.
    def enterMinMaxTimeStepDefault(self, ctx:VfrSyntaxParser.MinMaxTimeStepDefaultContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#minMaxTimeStepDefault.
    def exitMinMaxTimeStepDefault(self, ctx:VfrSyntaxParser.MinMaxTimeStepDefaultContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrTimeFlags.
    def enterVfrTimeFlags(self, ctx:VfrSyntaxParser.VfrTimeFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrTimeFlags.
    def exitVfrTimeFlags(self, ctx:VfrSyntaxParser.VfrTimeFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#timeFlagsField.
    def enterTimeFlagsField(self, ctx:VfrSyntaxParser.TimeFlagsFieldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#timeFlagsField.
    def exitTimeFlagsField(self, ctx:VfrSyntaxParser.TimeFlagsFieldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementConditional.
    def enterVfrStatementConditional(self, ctx:VfrSyntaxParser.VfrStatementConditionalContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementConditional.
    def exitVfrStatementConditional(self, ctx:VfrSyntaxParser.VfrStatementConditionalContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementConditionalNew.
    def enterVfrStatementConditionalNew(self, ctx:VfrSyntaxParser.VfrStatementConditionalNewContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementConditionalNew.
    def exitVfrStatementConditionalNew(self, ctx:VfrSyntaxParser.VfrStatementConditionalNewContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStat.
    def enterVfrStatementSuppressIfStat(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStat.
    def exitVfrStatementSuppressIfStat(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStat.
    def enterVfrStatementGrayOutIfStat(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStat.
    def exitVfrStatementGrayOutIfStat(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStatList.
    def enterVfrStatementStatList(self, ctx:VfrSyntaxParser.VfrStatementStatListContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStatList.
    def exitVfrStatementStatList(self, ctx:VfrSyntaxParser.VfrStatementStatListContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementStatListOld.
    def enterVfrStatementStatListOld(self, ctx:VfrSyntaxParser.VfrStatementStatListOldContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementStatListOld.
    def exitVfrStatementStatListOld(self, ctx:VfrSyntaxParser.VfrStatementStatListOldContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfStat.
    def enterVfrStatementDisableIfStat(self, ctx:VfrSyntaxParser.VfrStatementDisableIfStatContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementDisableIfStat.
    def exitVfrStatementDisableIfStat(self, ctx:VfrSyntaxParser.VfrStatementDisableIfStatContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementgrayoutIfSuppressIf.
    def enterVfrStatementgrayoutIfSuppressIf(self, ctx:VfrSyntaxParser.VfrStatementgrayoutIfSuppressIfContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementgrayoutIfSuppressIf.
    def exitVfrStatementgrayoutIfSuppressIf(self, ctx:VfrSyntaxParser.VfrStatementgrayoutIfSuppressIfContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementsuppressIfGrayOutIf.
    def enterVfrStatementsuppressIfGrayOutIf(self, ctx:VfrSyntaxParser.VfrStatementsuppressIfGrayOutIfContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementsuppressIfGrayOutIf.
    def exitVfrStatementsuppressIfGrayOutIf(self, ctx:VfrSyntaxParser.VfrStatementsuppressIfGrayOutIfContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStatNew.
    def enterVfrStatementSuppressIfStatNew(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatNewContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementSuppressIfStatNew.
    def exitVfrStatementSuppressIfStatNew(self, ctx:VfrSyntaxParser.VfrStatementSuppressIfStatNewContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStatNew.
    def enterVfrStatementGrayOutIfStatNew(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatNewContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementGrayOutIfStatNew.
    def exitVfrStatementGrayOutIfStatNew(self, ctx:VfrSyntaxParser.VfrStatementGrayOutIfStatNewContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIfStat.
    def enterVfrStatementInconsistentIfStat(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfStatContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementInconsistentIfStat.
    def exitVfrStatementInconsistentIfStat(self, ctx:VfrSyntaxParser.VfrStatementInconsistentIfStatContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementInvalid.
    def enterVfrStatementInvalid(self, ctx:VfrSyntaxParser.VfrStatementInvalidContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementInvalid.
    def exitVfrStatementInvalid(self, ctx:VfrSyntaxParser.VfrStatementInvalidContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementInvalidHidden.
    def enterVfrStatementInvalidHidden(self, ctx:VfrSyntaxParser.VfrStatementInvalidHiddenContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementInvalidHidden.
    def exitVfrStatementInvalidHidden(self, ctx:VfrSyntaxParser.VfrStatementInvalidHiddenContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementInvalidInventory.
    def enterVfrStatementInvalidInventory(self, ctx:VfrSyntaxParser.VfrStatementInvalidInventoryContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementInvalidInventory.
    def exitVfrStatementInvalidInventory(self, ctx:VfrSyntaxParser.VfrStatementInvalidInventoryContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementInvalidSaveRestoreDefaults.
    def enterVfrStatementInvalidSaveRestoreDefaults(self, ctx:VfrSyntaxParser.VfrStatementInvalidSaveRestoreDefaultsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementInvalidSaveRestoreDefaults.
    def exitVfrStatementInvalidSaveRestoreDefaults(self, ctx:VfrSyntaxParser.VfrStatementInvalidSaveRestoreDefaultsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementLabel.
    def enterVfrStatementLabel(self, ctx:VfrSyntaxParser.VfrStatementLabelContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementLabel.
    def exitVfrStatementLabel(self, ctx:VfrSyntaxParser.VfrStatementLabelContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementBanner.
    def enterVfrStatementBanner(self, ctx:VfrSyntaxParser.VfrStatementBannerContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementBanner.
    def exitVfrStatementBanner(self, ctx:VfrSyntaxParser.VfrStatementBannerContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementExtension.
    def enterVfrStatementExtension(self, ctx:VfrSyntaxParser.VfrStatementExtensionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementExtension.
    def exitVfrStatementExtension(self, ctx:VfrSyntaxParser.VfrStatementExtensionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExtensionData.
    def enterVfrExtensionData(self, ctx:VfrSyntaxParser.VfrExtensionDataContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExtensionData.
    def exitVfrExtensionData(self, ctx:VfrSyntaxParser.VfrExtensionDataContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementModal.
    def enterVfrStatementModal(self, ctx:VfrSyntaxParser.VfrStatementModalContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementModal.
    def exitVfrStatementModal(self, ctx:VfrSyntaxParser.VfrStatementModalContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrModalTag.
    def enterVfrModalTag(self, ctx:VfrSyntaxParser.VfrModalTagContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrModalTag.
    def exitVfrModalTag(self, ctx:VfrSyntaxParser.VfrModalTagContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementExpression.
    def enterVfrStatementExpression(self, ctx:VfrSyntaxParser.VfrStatementExpressionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementExpression.
    def exitVfrStatementExpression(self, ctx:VfrSyntaxParser.VfrStatementExpressionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrStatementExpressionSub.
    def enterVfrStatementExpressionSub(self, ctx:VfrSyntaxParser.VfrStatementExpressionSubContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrStatementExpressionSub.
    def exitVfrStatementExpressionSub(self, ctx:VfrSyntaxParser.VfrStatementExpressionSubContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#andTerm.
    def enterAndTerm(self, ctx:VfrSyntaxParser.AndTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#andTerm.
    def exitAndTerm(self, ctx:VfrSyntaxParser.AndTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#bitwiseorTerm.
    def enterBitwiseorTerm(self, ctx:VfrSyntaxParser.BitwiseorTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#bitwiseorTerm.
    def exitBitwiseorTerm(self, ctx:VfrSyntaxParser.BitwiseorTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#bitwiseandTerm.
    def enterBitwiseandTerm(self, ctx:VfrSyntaxParser.BitwiseandTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#bitwiseandTerm.
    def exitBitwiseandTerm(self, ctx:VfrSyntaxParser.BitwiseandTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#equalTerm.
    def enterEqualTerm(self, ctx:VfrSyntaxParser.EqualTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#equalTerm.
    def exitEqualTerm(self, ctx:VfrSyntaxParser.EqualTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#equalTermEqualRule.
    def enterEqualTermEqualRule(self, ctx:VfrSyntaxParser.EqualTermEqualRuleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#equalTermEqualRule.
    def exitEqualTermEqualRule(self, ctx:VfrSyntaxParser.EqualTermEqualRuleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#equalTermNotEqualRule.
    def enterEqualTermNotEqualRule(self, ctx:VfrSyntaxParser.EqualTermNotEqualRuleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#equalTermNotEqualRule.
    def exitEqualTermNotEqualRule(self, ctx:VfrSyntaxParser.EqualTermNotEqualRuleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#compareTerm.
    def enterCompareTerm(self, ctx:VfrSyntaxParser.CompareTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#compareTerm.
    def exitCompareTerm(self, ctx:VfrSyntaxParser.CompareTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#compareTermLessRule.
    def enterCompareTermLessRule(self, ctx:VfrSyntaxParser.CompareTermLessRuleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#compareTermLessRule.
    def exitCompareTermLessRule(self, ctx:VfrSyntaxParser.CompareTermLessRuleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#compareTermLessEqualRule.
    def enterCompareTermLessEqualRule(self, ctx:VfrSyntaxParser.CompareTermLessEqualRuleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#compareTermLessEqualRule.
    def exitCompareTermLessEqualRule(self, ctx:VfrSyntaxParser.CompareTermLessEqualRuleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#compareTermGreaterRule.
    def enterCompareTermGreaterRule(self, ctx:VfrSyntaxParser.CompareTermGreaterRuleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#compareTermGreaterRule.
    def exitCompareTermGreaterRule(self, ctx:VfrSyntaxParser.CompareTermGreaterRuleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#compareTermGreaterEqualRule.
    def enterCompareTermGreaterEqualRule(self, ctx:VfrSyntaxParser.CompareTermGreaterEqualRuleContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#compareTermGreaterEqualRule.
    def exitCompareTermGreaterEqualRule(self, ctx:VfrSyntaxParser.CompareTermGreaterEqualRuleContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#shiftTerm.
    def enterShiftTerm(self, ctx:VfrSyntaxParser.ShiftTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#shiftTerm.
    def exitShiftTerm(self, ctx:VfrSyntaxParser.ShiftTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#shiftTermLeft.
    def enterShiftTermLeft(self, ctx:VfrSyntaxParser.ShiftTermLeftContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#shiftTermLeft.
    def exitShiftTermLeft(self, ctx:VfrSyntaxParser.ShiftTermLeftContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#shiftTermRight.
    def enterShiftTermRight(self, ctx:VfrSyntaxParser.ShiftTermRightContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#shiftTermRight.
    def exitShiftTermRight(self, ctx:VfrSyntaxParser.ShiftTermRightContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#addMinusTerm.
    def enterAddMinusTerm(self, ctx:VfrSyntaxParser.AddMinusTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#addMinusTerm.
    def exitAddMinusTerm(self, ctx:VfrSyntaxParser.AddMinusTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#addMinusTermpAdd.
    def enterAddMinusTermpAdd(self, ctx:VfrSyntaxParser.AddMinusTermpAddContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#addMinusTermpAdd.
    def exitAddMinusTermpAdd(self, ctx:VfrSyntaxParser.AddMinusTermpAddContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#addMinusTermSubtract.
    def enterAddMinusTermSubtract(self, ctx:VfrSyntaxParser.AddMinusTermSubtractContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#addMinusTermSubtract.
    def exitAddMinusTermSubtract(self, ctx:VfrSyntaxParser.AddMinusTermSubtractContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#multdivmodTerm.
    def enterMultdivmodTerm(self, ctx:VfrSyntaxParser.MultdivmodTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#multdivmodTerm.
    def exitMultdivmodTerm(self, ctx:VfrSyntaxParser.MultdivmodTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#multdivmodTermMul.
    def enterMultdivmodTermMul(self, ctx:VfrSyntaxParser.MultdivmodTermMulContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#multdivmodTermMul.
    def exitMultdivmodTermMul(self, ctx:VfrSyntaxParser.MultdivmodTermMulContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#multdivmodTermDiv.
    def enterMultdivmodTermDiv(self, ctx:VfrSyntaxParser.MultdivmodTermDivContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#multdivmodTermDiv.
    def exitMultdivmodTermDiv(self, ctx:VfrSyntaxParser.MultdivmodTermDivContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#multdivmodTermModulo.
    def enterMultdivmodTermModulo(self, ctx:VfrSyntaxParser.MultdivmodTermModuloContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#multdivmodTermModulo.
    def exitMultdivmodTermModulo(self, ctx:VfrSyntaxParser.MultdivmodTermModuloContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#castTerm.
    def enterCastTerm(self, ctx:VfrSyntaxParser.CastTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#castTerm.
    def exitCastTerm(self, ctx:VfrSyntaxParser.CastTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#atomTerm.
    def enterAtomTerm(self, ctx:VfrSyntaxParser.AtomTermContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#atomTerm.
    def exitAtomTerm(self, ctx:VfrSyntaxParser.AtomTermContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionCatenate.
    def enterVfrExpressionCatenate(self, ctx:VfrSyntaxParser.VfrExpressionCatenateContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionCatenate.
    def exitVfrExpressionCatenate(self, ctx:VfrSyntaxParser.VfrExpressionCatenateContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionMatch.
    def enterVfrExpressionMatch(self, ctx:VfrSyntaxParser.VfrExpressionMatchContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionMatch.
    def exitVfrExpressionMatch(self, ctx:VfrSyntaxParser.VfrExpressionMatchContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionMatch2.
    def enterVfrExpressionMatch2(self, ctx:VfrSyntaxParser.VfrExpressionMatch2Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionMatch2.
    def exitVfrExpressionMatch2(self, ctx:VfrSyntaxParser.VfrExpressionMatch2Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionParen.
    def enterVfrExpressionParen(self, ctx:VfrSyntaxParser.VfrExpressionParenContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionParen.
    def exitVfrExpressionParen(self, ctx:VfrSyntaxParser.VfrExpressionParenContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionBuildInFunction.
    def enterVfrExpressionBuildInFunction(self, ctx:VfrSyntaxParser.VfrExpressionBuildInFunctionContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionBuildInFunction.
    def exitVfrExpressionBuildInFunction(self, ctx:VfrSyntaxParser.VfrExpressionBuildInFunctionContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#dupExp.
    def enterDupExp(self, ctx:VfrSyntaxParser.DupExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#dupExp.
    def exitDupExp(self, ctx:VfrSyntaxParser.DupExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vareqvalExp.
    def enterVareqvalExp(self, ctx:VfrSyntaxParser.VareqvalExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vareqvalExp.
    def exitVareqvalExp(self, ctx:VfrSyntaxParser.VareqvalExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#ideqvalExp.
    def enterIdeqvalExp(self, ctx:VfrSyntaxParser.IdeqvalExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#ideqvalExp.
    def exitIdeqvalExp(self, ctx:VfrSyntaxParser.IdeqvalExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#ideqidExp.
    def enterIdeqidExp(self, ctx:VfrSyntaxParser.IdeqidExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#ideqidExp.
    def exitIdeqidExp(self, ctx:VfrSyntaxParser.IdeqidExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#ideqvallistExp.
    def enterIdeqvallistExp(self, ctx:VfrSyntaxParser.IdeqvallistExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#ideqvallistExp.
    def exitIdeqvallistExp(self, ctx:VfrSyntaxParser.IdeqvallistExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldNameRule1.
    def enterVfrQuestionDataFieldNameRule1(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameRule1Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldNameRule1.
    def exitVfrQuestionDataFieldNameRule1(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameRule1Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldNameRule2.
    def enterVfrQuestionDataFieldNameRule2(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameRule2Context):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrQuestionDataFieldNameRule2.
    def exitVfrQuestionDataFieldNameRule2(self, ctx:VfrSyntaxParser.VfrQuestionDataFieldNameRule2Context):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#arrayName.
    def enterArrayName(self, ctx:VfrSyntaxParser.ArrayNameContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#arrayName.
    def exitArrayName(self, ctx:VfrSyntaxParser.ArrayNameContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#questionref1Exp.
    def enterQuestionref1Exp(self, ctx:VfrSyntaxParser.Questionref1ExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#questionref1Exp.
    def exitQuestionref1Exp(self, ctx:VfrSyntaxParser.Questionref1ExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#rulerefExp.
    def enterRulerefExp(self, ctx:VfrSyntaxParser.RulerefExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#rulerefExp.
    def exitRulerefExp(self, ctx:VfrSyntaxParser.RulerefExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#stringref1Exp.
    def enterStringref1Exp(self, ctx:VfrSyntaxParser.Stringref1ExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#stringref1Exp.
    def exitStringref1Exp(self, ctx:VfrSyntaxParser.Stringref1ExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#pushthisExp.
    def enterPushthisExp(self, ctx:VfrSyntaxParser.PushthisExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#pushthisExp.
    def exitPushthisExp(self, ctx:VfrSyntaxParser.PushthisExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#securityExp.
    def enterSecurityExp(self, ctx:VfrSyntaxParser.SecurityExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#securityExp.
    def exitSecurityExp(self, ctx:VfrSyntaxParser.SecurityExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#numericVarStoreType.
    def enterNumericVarStoreType(self, ctx:VfrSyntaxParser.NumericVarStoreTypeContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#numericVarStoreType.
    def exitNumericVarStoreType(self, ctx:VfrSyntaxParser.NumericVarStoreTypeContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#getExp.
    def enterGetExp(self, ctx:VfrSyntaxParser.GetExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#getExp.
    def exitGetExp(self, ctx:VfrSyntaxParser.GetExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionConstant.
    def enterVfrExpressionConstant(self, ctx:VfrSyntaxParser.VfrExpressionConstantContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionConstant.
    def exitVfrExpressionConstant(self, ctx:VfrSyntaxParser.VfrExpressionConstantContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionUnaryOp.
    def enterVfrExpressionUnaryOp(self, ctx:VfrSyntaxParser.VfrExpressionUnaryOpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionUnaryOp.
    def exitVfrExpressionUnaryOp(self, ctx:VfrSyntaxParser.VfrExpressionUnaryOpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#lengthExp.
    def enterLengthExp(self, ctx:VfrSyntaxParser.LengthExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#lengthExp.
    def exitLengthExp(self, ctx:VfrSyntaxParser.LengthExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#bitwisenotExp.
    def enterBitwisenotExp(self, ctx:VfrSyntaxParser.BitwisenotExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#bitwisenotExp.
    def exitBitwisenotExp(self, ctx:VfrSyntaxParser.BitwisenotExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#question23refExp.
    def enterQuestion23refExp(self, ctx:VfrSyntaxParser.Question23refExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#question23refExp.
    def exitQuestion23refExp(self, ctx:VfrSyntaxParser.Question23refExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#stringref2Exp.
    def enterStringref2Exp(self, ctx:VfrSyntaxParser.Stringref2ExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#stringref2Exp.
    def exitStringref2Exp(self, ctx:VfrSyntaxParser.Stringref2ExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#toboolExp.
    def enterToboolExp(self, ctx:VfrSyntaxParser.ToboolExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#toboolExp.
    def exitToboolExp(self, ctx:VfrSyntaxParser.ToboolExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#tostringExp.
    def enterTostringExp(self, ctx:VfrSyntaxParser.TostringExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#tostringExp.
    def exitTostringExp(self, ctx:VfrSyntaxParser.TostringExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#unintExp.
    def enterUnintExp(self, ctx:VfrSyntaxParser.UnintExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#unintExp.
    def exitUnintExp(self, ctx:VfrSyntaxParser.UnintExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#toupperExp.
    def enterToupperExp(self, ctx:VfrSyntaxParser.ToupperExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#toupperExp.
    def exitToupperExp(self, ctx:VfrSyntaxParser.ToupperExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#tolwerExp.
    def enterTolwerExp(self, ctx:VfrSyntaxParser.TolwerExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#tolwerExp.
    def exitTolwerExp(self, ctx:VfrSyntaxParser.TolwerExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#setExp.
    def enterSetExp(self, ctx:VfrSyntaxParser.SetExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#setExp.
    def exitSetExp(self, ctx:VfrSyntaxParser.SetExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionTernaryOp.
    def enterVfrExpressionTernaryOp(self, ctx:VfrSyntaxParser.VfrExpressionTernaryOpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionTernaryOp.
    def exitVfrExpressionTernaryOp(self, ctx:VfrSyntaxParser.VfrExpressionTernaryOpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#conditionalExp.
    def enterConditionalExp(self, ctx:VfrSyntaxParser.ConditionalExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#conditionalExp.
    def exitConditionalExp(self, ctx:VfrSyntaxParser.ConditionalExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#findExp.
    def enterFindExp(self, ctx:VfrSyntaxParser.FindExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#findExp.
    def exitFindExp(self, ctx:VfrSyntaxParser.FindExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#findFormat.
    def enterFindFormat(self, ctx:VfrSyntaxParser.FindFormatContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#findFormat.
    def exitFindFormat(self, ctx:VfrSyntaxParser.FindFormatContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#midExp.
    def enterMidExp(self, ctx:VfrSyntaxParser.MidExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#midExp.
    def exitMidExp(self, ctx:VfrSyntaxParser.MidExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#tokenExp.
    def enterTokenExp(self, ctx:VfrSyntaxParser.TokenExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#tokenExp.
    def exitTokenExp(self, ctx:VfrSyntaxParser.TokenExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#spanExp.
    def enterSpanExp(self, ctx:VfrSyntaxParser.SpanExpContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#spanExp.
    def exitSpanExp(self, ctx:VfrSyntaxParser.SpanExpContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#spanFlags.
    def enterSpanFlags(self, ctx:VfrSyntaxParser.SpanFlagsContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#spanFlags.
    def exitSpanFlags(self, ctx:VfrSyntaxParser.SpanFlagsContext):
        pass


    # Enter a parse tree produced by VfrSyntaxParser#vfrExpressionMap.
    def enterVfrExpressionMap(self, ctx:VfrSyntaxParser.VfrExpressionMapContext):
        pass

    # Exit a parse tree produced by VfrSyntaxParser#vfrExpressionMap.
    def exitVfrExpressionMap(self, ctx:VfrSyntaxParser.VfrExpressionMapContext):
        pass


