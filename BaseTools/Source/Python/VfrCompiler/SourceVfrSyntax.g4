grammar SourceVfrSyntax;
options {
    language=Python;
}
@header{

from VfrCtypes import *
from VfrFormPkg import *
from VfrUtility import *
from VfrTree import *
from VfrPreProcess import *
}

vfrProgram
    :   vfrHeader vfrFormSetDefinition?
    ;

vfrHeader
    :   (vfrPragmaPackDefinition | vfrDataStructDefinition | vfrDataUnionDefinition)*
    ;

pragmaPackShowDef
    :   'show'
    ;

pragmaPackStackDef
    :   ('push' | 'pop')  (',' StringIdentifier)? (',' Number)?
    ;

pragmaPackNumber
    :   Number?
    ;

vfrPragmaPackDefinition
    :   '#pragma' 'pack' '('
        (   pragmaPackShowDef
        |   pragmaPackStackDef
        |   pragmaPackNumber
        )?
    ')'
    ;

vfrDataStructDefinition
    :   (TypeDef)? Struct NonNvDataMap? N1=StringIdentifier? '{' vfrDataStructFields[False] '}'  N2=StringIdentifier? ';'
    ;

vfrDataUnionDefinition
    :   (TypeDef)? Union NonNvDataMap? N1=StringIdentifier? '{' vfrDataStructFields[True]'}' N2=StringIdentifier? ';'
    ;

vfrDataStructFields[FieldInUnion]
    :
    (   dataStructField64[FieldInUnion]
    |   dataStructField32[FieldInUnion]
    |   dataStructField16[FieldInUnion]
    |   dataStructField8[FieldInUnion]
    |   dataStructFieldBool[FieldInUnion]
    |   dataStructFieldString[FieldInUnion]
    |   dataStructFieldDate[FieldInUnion]
    |   dataStructFieldTime[FieldInUnion]
    |   dataStructFieldRef[FieldInUnion]
    |   dataStructFieldUser[FieldInUnion]
    |   dataStructBitField64[FieldInUnion]
    |   dataStructBitField32[FieldInUnion]
    |   dataStructBitField16[FieldInUnion]
    |   dataStructBitField8[FieldInUnion]
    )*
    ;

dataStructField64[FieldInUnion]
    :   'UINT64' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField32[FieldInUnion]
    :   'UINT32' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField16[FieldInUnion]
    :   'UINT16' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField8[FieldInUnion]
    :   'UINT8' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldBool[FieldInUnion]
    :   'BOOLEAN' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldString[FieldInUnion]
    :   'EFI_STRING_ID' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldDate[FieldInUnion]
    :   'EFI_HII_DATE' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldTime[FieldInUnion]
    :   'EFI_HII_TIME' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldRef[FieldInUnion]
    :   'EFI_HII_REF' N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldUser[FieldInUnion]
    :   T=StringIdentifier N=StringIdentifier ('[' Number ']')? ';'
    ;

dataStructBitField64[FieldInUnion]
    :   D='UINT64'  N=StringIdentifier? ':' Number ';'
    ;
dataStructBitField32[FieldInUnion]
    :   D='UINT32' N=StringIdentifier? ':' Number ';'
    ;
dataStructBitField16[FieldInUnion]
    :   D='UINT16' N=StringIdentifier? ':' Number ';'
    ;
dataStructBitField8[FieldInUnion]
    :   D='UINT8' N=StringIdentifier? ':' Number ';'
    ;

// VFR FormSet Definition
vfrFormSetDefinition
locals[Node=VfrTreeNode(EFI_IFR_FORM_SET_OP)]
    :   'formset'
        'guid' '=' S1=StringIdentifier ','
        'title' '=' 'STRING_TOKEN' '(' S2=StringIdentifier ')' ','
        'help' '=' 'STRING_TOKEN' '(' S3=StringIdentifier ')' ','
        ('classguid' '=' classguidDefinition[localctx.Node] ',')?
        ('class' '=' classDefinition ',')?
        ('subclass' '=' subclassDefinition ',')?
        vfrFormSetList[localctx.Node]
        'endformset' ';'
    ;

classguidDefinition[Node]
locals[GuidList=[]]
    :   StringIdentifier ('|' StringIdentifier)? ('|' StringIdentifier)?('|' StringIdentifier)?
    ;

classDefinition
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   validClassNames ('|' validClassNames)*
    ;

validClassNames
locals[ClassName=0]
    :   ClassNonDevice
    |   ClassDiskDevice
    |   ClassVideoDevice
    |   ClassNetworkDevice
    |   ClassInputDevice
    |   ClassOnBoardDevice
    |   ClassOtherDevice
    |   S=StringIdentifier
    |   Number
    ;

subclassDefinition
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   SubclassSetupApplication
    |   SubclassGeneralApplication
    |   SubclassFrontPage
    |   SubclassSingleUse
    |   S=StringIdentifier
    |   Number
    ;


vfrFormSetList[Node]
    : (vfrFormSet)*
    ;

//2.5 VFR FormSet List Definition
vfrFormSet
locals[Node=None]
    :
    (   vfrFormDefinition
    |   vfrFormMapDefinition
    |   vfrStatementImage
    |   vfrStatementVarStoreLinear
    |   vfrStatementVarStoreEfi
    |   vfrStatementVarStoreNameValue
    |   vfrStatementDefaultStore
    |   vfrStatementDisableIfFormSet //
    |   vfrStatementSuppressIfFormSet //
    |   vfrStatementExtension
    )
    ;


//2.6 VFR Default Stores Definition
vfrStatementDefaultStore
locals[Node=VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP)]
    :   'defaultstore' D=StringIdentifier ','
        'prompt' '=' 'STRING_TOKEN' '(' P=StringIdentifier ')'
        (',' 'attribute' '=' (A=Number | S=StringIdentifier))? ';'
    ;

//2.7 VFR Variable Store Definition
vfrStatementVarStoreLinear
locals[Node=VfrTreeNode(EFI_IFR_VARSTORE_OP)]
    :   'varstore'
        (   TN=StringIdentifier ','
        |   'UINT8' ','
        |   'UINT16' ','
        |   'UINT32' ','
        |   'UINT64' ','
        |   'EFI_HII_DATE' ','
        |   'EFI_HII_TIME' ','
        |   'EFI_HII_REF' ','
        )
        ('varid' '=' (ID=Number | S=StringIdentifier) ',')?
        'name' '=' SN=StringIdentifier ','
        'guid' '=' SG=StringIdentifier ';'
    ;

vfrStatementVarStoreEfi //
locals[Node=VfrTreeNode(EFI_IFR_VARSTORE_EFI_OP)]
    :   'efivarstore'
        (   TN=StringIdentifier ','
        |   'UINT8' ','
        |   'UINT16' ','
        |   'UINT32' ','
        |   'UINT64' ','
        |   'EFI_HII_DATE' ','
        |   'EFI_HII_TIME' ','
        |   'EFI_HII_REF' ','
        )
        ('varid' '=' (ID=Number | S1=StringIdentifier) ',')?
        'attribute' '=' vfrVarStoreEfiAttr ('|' vfrVarStoreEfiAttr)* ','
        (   'name' '=' SN=StringIdentifier ','
        |   'name' '=' 'STRING_TOKEN' '(' VN=StringIdentifier ')' ','  'varsize' '=' (N=Number | S2=StringIdentifier)  ','
        )
        'guid' '=' SG=StringIdentifier ';'
    ;

vfrVarStoreEfiAttr
locals[Attr=0]
    :   N=Number
    |   S=StringIdentifier ;

vfrStatementVarStoreNameValue
locals[Node=VfrTreeNode(EFI_IFR_VARSTORE_NAME_VALUE_OP)]
    :   'namevaluevarstore' SN=StringIdentifier ','
        ('varid' '=' (ID=Number | SID=StringIdentifier)  ',')?
        ('name' '=' 'STRING_TOKEN' '(' SN2=StringIdentifier ')' ',')+
        'guid' '=' SG=StringIdentifier ';'
    ;

vfrStatementDisableIfFormSet
locals[Node=VfrTreeNode(EFI_IFR_DISABLE_IF_OP)]
    :   'disableif' vfrStatementExpression[localctx.Node] ';'
        vfrFormSetList[localctx.Node]
        'endif' ';'
    ;

vfrStatementSuppressIfFormSet
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'suppressif' vfrStatementExpression[localctx.Node] ';'
        vfrFormSetList[localctx.Node]
        'endif' ';'
    ;

getStringId
locals[StringId='']
    :   'STRING_TOKEN' '(' StringIdentifier ')'
    ;

vfrQuestionHeader[Node, QType]
    :   vfrQuestionBaseInfo[Node, QType]
        vfrStatementHeader[Node]
    ;

vfrQuestionBaseInfo[Node, QType]
locals[BaseInfo=EFI_VARSTORE_INFO(), QId=EFI_QUESTION_ID_INVALID, CheckFlag=True, QName=None, VarIdStr='']
    :   ('name' '=' QN=StringIdentifier ',')?
        ('varid' '=' vfrStorageVarId[localctx.BaseInfo, localctx.CheckFlag] ',')?
        ('questionid' '=' (ID=Number | SID=StringIdentifier)  ',')?
    ;

vfrStatementHeader[Node]
    :   'prompt' '=' 'STRING_TOKEN' '(' P=StringIdentifier ')' ','
        'help' '=' 'STRING_TOKEN' '(' H=StringIdentifier ')'
    ;

questionheaderFlagsField
locals[QHFlag=0]
    :   ReadOnlyFlag
    |   InteractiveFlag
    |   ResetRequiredFlag
    |   RestStyleFlag
    |   ReconnectRequiredFlag
    |   O=OptionOnlyFlag
    |   N=NVAccessFlag
    |   L=LateCheckFlag
    ;
//2.10.5
vfrStorageVarId[BaseInfo, CheckFlag]
locals[VarIdStr='']
    :   (SN1=StringIdentifier '[' I=Number ']')    # vfrStorageVarIdRule1
    |   (SN2=StringIdentifier ('.' arrayName)* ) # vfrStorageVarIdRule2
    ;

vfrConstantValueField[Node]
locals[ValueList=[], ListType=False]
    :   'TRUE'
    |   'FALSE'
    |   'ONE'
    |   'ONES'
    |   'ZERO'
    |   S1=StringIdentifier
    |   ('-')? Number
    |   Number ':' Number ':' Number
    |   Number '/' Number '/' Number
    |   Number ';' Number ';' S2=StringIdentifier ';' 'STRING_TOKEN' '(' S3=StringIdentifier ')'
    |   'STRING_TOKEN' '(' S4=StringIdentifier ')'
    |   '{' Number (',' Number)* '}'
    ;

vfrImageTag
locals[Node=VfrTreeNode(EFI_IFR_IMAGE_OP)]
    :   'image' '=' 'IMAGE_TOKEN' '(' StringIdentifier ')'
    ;

vfrLockedTag
locals[Node=VfrTreeNode(EFI_IFR_LOCKED_OP)]
    :   'locked'
    ;

vfrStatementStatTag
locals[Node]
    :   vfrImageTag | vfrLockedTag
    ;

vfrStatementStatTagList[Node]
    :   vfrStatementStatTag (',' vfrStatementStatTag)*
    ;

vfrFormDefinition //2.14
locals[Node=VfrTreeNode(EFI_IFR_FORM_OP)]
    :   'form' 'formid' '=' (N=Number | S=StringIdentifier) ','
        'title' '=' 'STRING_TOKEN' '(' ST=StringIdentifier ')' ';'
        (vfrForm)*
        'endform' ';'
    ;

vfrForm
locals[Node]
    :
    (   vfrStatementImage
    |   vfrStatementLocked
    |   vfrStatementRules //
    |   vfrStatementDefault
    |   vfrStatementStat
    |   vfrStatementQuestions
    |   vfrStatementConditional //
    |   vfrStatementLabel
    |   vfrStatementBanner
    |   vfrStatementInvalid //
    |   vfrStatementExtension
    |   vfrStatementModal
    |   vfrStatementRefreshEvent ';'
    )
    ;

vfrFormMapDefinition
locals[Node=VfrTreeNode(EFI_IFR_FORM_MAP_OP)]
    :   'formmap' 'formid' '=' (N=Number| S=StringIdentifier)  ','
        (   'maptitle' '=' 'STRING_TOKEN' '(' SM=StringIdentifier ')' ';'
            'mapguid' '=' SG=StringIdentifier ';'
        )*
        (vfrForm)*
        'endform' ';'
    ;

vfrStatementImage
locals[Node]
    :   vfrImageTag ';'
    ;

vfrStatementLocked
locals[Node]
    :   vfrLockedTag ';'
    ;

vfrStatementRules
locals[Node=VfrTreeNode(EFI_IFR_RULE_OP)]
    :   'rule' StringIdentifier ','
        vfrStatementExpression[localctx.Node]
        'endrule' ';'
    ;

vfrStatementStat
locals[Node]
    :   vfrStatementSubTitle
    |   vfrStatementStaticText
    |   vfrStatementCrossReference
    ;

vfrStatementSubTitle
locals[Node=VfrTreeNode(EFI_IFR_SUBTITLE_OP)]
    :   'subtitle'

        'text' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')'
        (',' 'flags' '=' vfrSubtitleFlags[localctx.Node])?
        (   (',' vfrStatementStatTagList[localctx.Node])? ';'
        |
            (',' vfrStatementStatTagList[localctx.Node])?
            (',' (vfrStatementSubTitleComponent)* )?
            'endsubtitle' ';'
        )
    ;

vfrStatementSubTitleComponent
locals[Node]
    :  vfrStatementStat | vfrStatementQuestions
    ;

vfrSubtitleFlags[Node]
locals[SubFlags=0]
    :   subtitleFlagsField ('|' subtitleFlagsField)*
    ;
subtitleFlagsField
locals[Flag=0]
    :   N=Number
    |   H='HORIZONTAL'
    |   S=StringIdentifier
    ;

vfrStatementStaticText
locals[Node=VfrTreeNode(EFI_IFR_TEXT_OP)]
    :   'text'
        'help' '=' 'STRING_TOKEN' '(' S1=StringIdentifier ')' ','
        'text' '=' 'STRING_TOKEN' '(' S2=StringIdentifier ')'
        (',' 'text' '=' 'STRING_TOKEN' '(' S3=StringIdentifier ')')?
        (',' F='flags' '=' staticTextFlagsField ('|' staticTextFlagsField)* ',' 'key' '=' (N=Number | S4=StringIdentifier) )?
        (',' vfrStatementStatTagList[localctx.Node])? ';'
    ;

staticTextFlagsField
locals[Flag=0]
    :   N=Number
    |   questionheaderFlagsField
    |   S=StringIdentifier
    ;

vfrStatementCrossReference
locals[Node]
    :   vfrStatementGoto | vfrStatementResetButton
    ;

vfrStatementGoto
locals[Node=VfrTreeNode(EFI_IFR_REF_OP), QType=EFI_QUESION_TYPE.QUESTION_REF]
    :   'goto'
        (   (   DevicePath '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
                FormSetGuid '=' SG1=StringIdentifier ','
                FormId '=' (NF1=Number | SF1=StringIdentifier)  ','
                Question '=' (NQ1=Number | SQ1=StringIdentifier) ','
            )
            |
            (   FormSetGuid '=' SG2=StringIdentifier ','
                FormId '=' (NF2=Number | SF2=StringIdentifier)  ','
                Question '=' (NQ2=Number | SQ2=StringIdentifier) ','
            )
            |
            (   FormId '=' (NF3=Number | SF3=StringIdentifier) ','
                Question '=' (SQ3=StringIdentifier ',' | NQ3=Number ',')//
            )
            |
            (   (N1=Number | S1=StringIdentifier) ',' )
        )?
        vfrQuestionHeader[localctx.Node, localctx.QType]
        (',' 'flags' '=' vfrGotoFlags)?
        (',' 'key' '=' (KN=Number | SN=StringIdentifier))?
        (E=',' vfrStatementQuestionOptionList[localctx.Node])? ';'
    ;

vfrGotoFlags
locals[GotoFlags=0]
    :   gotoFlagsField('|' gotoFlagsField)*
    ;

gotoFlagsField
locals[Flag=0]
    :  N=Number | questionheaderFlagsField | S=StringIdentifier
    ;

vfrStatementResetButton
locals[Node=VfrTreeNode(EFI_IFR_RESET_BUTTON_OP)]
    :  'resetbutton'
       'defaultstore' '=' N=StringIdentifier ','
       vfrStatementHeader[localctx.Node] ','
       (vfrStatementStatTagList[localctx.Node] ',')?
       'endresetbutton' ';'
    ;

vfrStatementQuestions
locals[Node]
    :   vfrStatementBooleanType
    |   vfrStatementDate
    |   vfrStatementNumericType
    |   vfrStatementStringType
    |   vfrStatementOrderedList
    |   vfrStatementTime
    ;

vfrStatementQuestionTag
locals[Node]
    :   vfrStatementStatTag ','
    |   vfrStatementInconsistentIf //
    |   vfrStatementNoSubmitIf //
    |   vfrStatementDisableIfQuest //
    |   vfrStatementRefresh
    |   vfrStatementVarstoreDevice
    |   vfrStatementExtension
    |   vfrStatementRefreshEvent
    |   vfrStatementWarningIf
    ;

vfrStatementInconsistentIf
locals[Node=VfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP)]
    :   'inconsistentif'
        'prompt' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        (F='flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node]
        'endif' (';')?
    ;

vfrStatementNoSubmitIf //
locals[Node=VfrTreeNode(EFI_IFR_NO_SUBMIT_IF_OP)]
    :   'nosubmitif'
        'prompt' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        (F='flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node]
        'endif' (';')?
    ;

vfrStatementDisableIfQuest
locals[Node=VfrTreeNode(EFI_IFR_DISABLE_IF_OP)]
    :   'disableif' vfrStatementExpression[localctx.Node] ';'
        vfrStatementQuestionOptionList[localctx.Node]
        'endif' (';')?
    ;

vfrStatementRefresh
locals[Node=VfrTreeNode(EFI_IFR_REFRESH_OP)]
    :   'refresh' 'interval' '=' N=Number | S=StringIdentifier
    ;

vfrStatementVarstoreDevice
locals[Node=VfrTreeNode(EFI_IFR_VARSTORE_DEVICE_OP)]
    :   'varstoredevice' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
    ;

vfrStatementRefreshEvent
locals[Node=VfrTreeNode(EFI_IFR_REFRESH_ID_OP)]
    :   'refreshguid' '=' S=StringIdentifier ','
    ;

vfrStatementWarningIf
locals[Node=VfrTreeNode(EFI_IFR_WARNING_IF_OP)]
    :   'warningif'
        'prompt' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        ('timeout' '=' (N=Number | ST=StringIdentifier)',')?
        vfrStatementExpression[localctx.Node]
        'endif' (';')?
    ;

vfrStatementQuestionTagList[Node]
    :   (vfrStatementQuestionTag)*
    ;

vfrStatementQuestionOptionTag
locals[Node]
    :   vfrStatementSuppressIfQuest //
    |   vfrStatementGrayOutIfQuest //
    |   vfrStatementValue
    |   vfrStatementDefault
    |   vfrStatementOptions
    |   vfrStatementRead
    |   vfrStatementWrite
    ;

flagsField
    :   Number
    |   InteractiveFlag
    |   ManufacturingFlag
    |   DefaultFlag
    |   ResetRequiredFlag
    |   ReconnectRequiredFlag
    |   N=NVAccessFlag
    |   L=LateCheckFlag
    |   S=StringIdentifier
    ;

vfrStatementSuppressIfQuest //////////////
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'suppressif' vfrStatementExpression[localctx.Node] ';'
        (F='flags' '=' flagsField ('|' flagsField )* ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endif' (';')?
    ;

vfrStatementGrayOutIfQuest /////////////////
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'grayoutif' vfrStatementExpression[localctx.Node] ';'
        (F='flags' '=' flagsField ('|' flagsField )* ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endif' (';')?
    ;


vfrStatementDefault
locals[Node=VfrTreeNode(EFI_IFR_DEFAULT_OP)]
    :   D='default'
        (   (   V=vfrStatementValue ','
            |   '=' vfrConstantValueField[localctx.Node] ','
            )
            (   'defaultstore' '=' SN=StringIdentifier ','
            )?
        )
    ;

vfrStatementValue //
locals[Node=VfrTreeNode(EFI_IFR_VALUE_OP)]
    :   'value' '=' vfrStatementExpression[localctx.Node]
    ;

vfrStatementOptions
locals[Node]
    :   vfrStatementOneOfOption
    ;

vfrStatementOneOfOption
locals[Node=VfrTreeNode(EFI_IFR_ONE_OF_OPTION_OP)]
    :   'option'
        'text' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        'value' '=' vfrConstantValueField[localctx.Node] ','
        F='flags' '=' vfrOneOfOptionFlags (',' 'key' '=' (KN=Number | KS=StringIdentifier)) ? (T=',' vfrImageTag)* ';'
    ;

vfrOneOfOptionFlags
locals[HFlags=0, LFlags=0]
    :   oneofoptionFlagsField ('|' oneofoptionFlagsField)*;

oneofoptionFlagsField
	locals[HFlag=0, LFlag=0]
    :   Number
    |   OptionDefault
    |   OptionDefaultMfg
    |   InteractiveFlag
    |   ResetRequiredFlag
    |   RestStyleFlag
    |   ReconnectRequiredFlag
    |   ManufacturingFlag
    |   DefaultFlag
    |   A=NVAccessFlag
    |   L=LateCheckFlag
    |   S=StringIdentifier
    ;

vfrStatementRead
locals[Node=VfrTreeNode(EFI_IFR_READ_OP)]
    :   'read' vfrStatementExpression[localctx.Node] ';'
    ;

vfrStatementWrite
locals[Node=VfrTreeNode(EFI_IFR_WRITE_OP)]
    :   'write' vfrStatementExpression[localctx.Node] ';'
    ;

vfrStatementQuestionOptionList[Node]
    :   (vfrStatementQuestionOption)*
    ;



vfrStatementQuestionOption
locals[Node]
    :   vfrStatementQuestionTag | vfrStatementQuestionOptionTag
    ;

vfrStatementBooleanType
locals[Node]
    :   vfrStatementCheckBox | vfrStatementAction
    ;

vfrStatementCheckBox
locals[Node=VfrTreeNode(EFI_IFR_CHECKBOX_OP),GuidNode=VfrTreeNode(EFI_IFR_GUID_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   L='checkbox'
        vfrQuestionBaseInfo[localctx.Node, localctx.QType]
        vfrStatementHeader[localctx.Node] ','
        (F='flags' '=' vfrCheckBoxFlags ',')?
        ('key' '=' (N=Number | S=StringIdentifier ) ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endcheckbox' ';'
    ;

vfrCheckBoxFlags
locals[LFlags=0, HFlags=0]
    :   checkboxFlagsField ('|' checkboxFlagsField)*
    ;

checkboxFlagsField
locals[LFlag=0, HFlag=0]
    :   Number
    |   D='DEFAULT'
    |   M='MANUFACTURING'
    |   'CHECKBOX_DEFAULT'
    |   'CHECKBOX_DEFAULT_MFG'
    |   questionheaderFlagsField
    |   S=StringIdentifier
    ;

vfrStatementAction
locals[Node=VfrTreeNode(EFI_IFR_ACTION_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'action'
        vfrQuestionHeader[localctx.Node, localctx.QType] ','
        ('flags' '=' vfrActionFlags ',')?
        'config' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        vfrStatementQuestionTagList[localctx.Node]
        'endaction' ';'
    ;

vfrActionFlags
locals[HFlags=0]
    :   actionFlagsField ('|' actionFlagsField)*
    ;
actionFlagsField
locals[HFlag=0]
    :   N=Number | questionheaderFlagsField | S=StringIdentifier
    ;

vfrStatementNumericType
locals[Node]
    :   vfrStatementNumeric | vfrStatementOneOf
    ;

vfrStatementNumeric
locals[Node=VfrTreeNode(EFI_IFR_NUMERIC_OP), GuidNode=VfrTreeNode(EFI_IFR_GUID_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'numeric'
        vfrQuestionBaseInfo[localctx.Node, localctx.QType]
        vfrStatementHeader[localctx.Node] ','
        (F='flags' '=' vfrNumericFlags ',')?
        ('key' '=' (N=Number | S=StringIdentifier) ',')?
        vfrSetMinMaxStep[localctx.Node]
        vfrStatementQuestionOptionList[localctx.Node]
        'endnumeric' ';'
    ;

vfrSetMinMaxStep[Node] // CIfrMinMaxStepData
    :   'minimum' '='  ((N1='-')? I=Number)  ','
        'maximum' '='  ((N2='-')? A=Number)  ','
        ('step' '=' S=Number ',')?
    ;

vfrNumericFlags
locals[HFlags=0, LFlags=0, IsDisplaySpecified=False, UpdateVarType=False]
    :   numericFlagsField ('|' numericFlagsField)*
    ;

numericFlagsField
locals[HFlag=0,IsSetType=False,IsDisplaySpecified=False]
    :   N=Number
    |   'NUMERIC_SIZE_1'
    |   'NUMERIC_SIZE_2'
    |   'NUMERIC_SIZE_4'
    |   'NUMERIC_SIZE_8'
    |   'DISPLAY_INT_DEC'
    |   'DISPLAY_UINT_DEC'
    |   'DISPLAY_UINT_HEX'
    |   questionheaderFlagsField
    |   S=StringIdentifier
    ;

vfrStatementOneOf
locals[Node=VfrTreeNode(EFI_IFR_ONE_OF_OP), GuidNode=VfrTreeNode(EFI_IFR_GUID_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'oneof'
        vfrQuestionBaseInfo[localctx.Node, localctx.QType]
        vfrStatementHeader[localctx.Node] ','
        (F='flags' '=' vfrOneofFlagsField ',')? //
        (vfrSetMinMaxStep[localctx.Node])?
        vfrStatementQuestionOptionList[localctx.Node]
        'endoneof' ';'
    ;

vfrOneofFlagsField
locals[HFlags=0, LFlags=0, UpdateVarType=False]
    :   numericFlagsField ('|' numericFlagsField)*
    ;

vfrStatementStringType
locals[Node]
    :   vfrStatementString | vfrStatementPassword
    ;

vfrStatementString
locals[Node=VfrTreeNode(EFI_IFR_STRING_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'string'
        vfrQuestionHeader[localctx.Node, localctx.QType] ','
        (F='flags' '=' vfrStringFlagsField ',')?
        ('key' '=' (N=Number | S=StringIdentifier) ',')?
        Min='minsize' '=' (N1=Number | S1=StringIdentifier)  ','
        Max='maxsize' '=' (N2=Number | S2=StringIdentifier)  ','
        vfrStatementQuestionOptionList[localctx.Node]
        'endstring' ';'
    ;

vfrStringFlagsField
locals[HFlags=0, LFlags=0]
    :   stringFlagsField ('|' stringFlagsField)*
    ;

stringFlagsField
locals[HFlag=0, LFlag=0]
    :   N=Number
    |   M='MULTI_LINE'
    |   questionheaderFlagsField
    |   S=StringIdentifier
    ;

vfrStatementPassword
locals[Node=VfrTreeNode(EFI_IFR_PASSWORD_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'password'
        vfrQuestionHeader[localctx.Node, localctx.QType]','
        (F='flags' '=' vfrPasswordFlagsField ',')?
        ('key' '=' N=Number | S=StringIdentifier ',')?
        Min='minsize' '=' (N1=Number | S1=StringIdentifier) ','
        Max='maxsize' '=' (N2=Number | S2=StringIdentifier) ','
        ('encoding' '=' (EN=Number | ES=StringIdentifier) ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endpassword' ';'
    ;

vfrPasswordFlagsField
locals[HFlags=0]
    :   passwordFlagsField('|' passwordFlagsField)*
    ;

passwordFlagsField
locals[HFlag=0]
    :   Number
    |   questionheaderFlagsField
    |   S=StringIdentifier
    ;

vfrStatementOrderedList
locals[Node=VfrTreeNode(EFI_IFR_ORDERED_LIST_OP), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'orderedlist'
        vfrQuestionHeader[localctx.Node, localctx.QType] ','
        (M='maxcontainers' '=' (N=Number |S=StringIdentifier) ',')?
        (F='flags' '=' vfrOrderedListFlags ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endlist' ';'
    ;

vfrOrderedListFlags
locals[HFlags=0, LFlags=0]
    :   orderedlistFlagsField ('|' orderedlistFlagsField)*
    ;

orderedlistFlagsField
locals[HFlag=0, LFlag=0]
    :   Number
    |   'UNIQUE'
    |   'NOEMPTY'
    |   questionheaderFlagsField
    |   S=StringIdentifier
    ;

vfrStatementDate
locals[Node=VfrTreeNode(EFI_IFR_DATE_OP), QType=EFI_QUESION_TYPE.QUESTION_DATE, Val=EFI_HII_DATE()]
    :   'date'
        (   (   vfrQuestionHeader[localctx.Node, localctx.QType] ','
                (F1='flags' '=' vfrDateFlags ',')?
                vfrStatementQuestionOptionList[localctx.Node]
            )
            |
            (   'year' 'varid' '=' S1=StringIdentifier '.' S2=StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' S3=StringIdentifier ')' ','
                'help' '=' 'STRING_TOKEN' '(' S4=StringIdentifier ')' ','
                minMaxDateStepDefault[localctx.Val, 0]
                'month' 'varid' '=' S5=StringIdentifier '.' S6=StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' S7=StringIdentifier ')' ','
                'help' '=' 'STRING_TOKEN' '(' S8=StringIdentifier ')' ','
                minMaxDateStepDefault[localctx.Val, 1]
                'day' 'varid' '=' S9=StringIdentifier '.' S10=StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' S11=StringIdentifier ')' ','
                'help' '=' 'STRING_TOKEN' '(' S12=StringIdentifier ')' ','
                minMaxDateStepDefault[localctx.Val, 2]
                (F2='flags' '=' vfrDateFlags ',')?
                (vfrStatementInconsistentIf)*
            )
        )
        'enddate' ';'
    ;

minMaxDateStepDefault[Date, KeyValue]
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
        ('default' '=' N=Number ',')?
    ;

vfrDateFlags
locals[LFlags=0]
    :   dateFlagsField ('|' dateFlagsField)*
    ;
dateFlagsField
locals[LFlag=0]
    :   Number
    |   'YEAR_SUPPRESS'
    |   'MONTH_SUPPRESS'
    |   'DAY_SUPPRESS'
    |   'STORAGE_NORMAL'
    |   'STORAGE_TIME'
    |   'STORAGE_WAKEUP'
    |   S=StringIdentifier
    ;

vfrStatementTime
locals[Node=VfrTreeNode(EFI_IFR_TIME_OP), QType=EFI_QUESION_TYPE.QUESTION_TIME,  Val=EFI_HII_TIME()]
    :   'time'
        (   (   vfrQuestionHeader[localctx.Node, localctx.QType] ','
                (F1='flags' '=' vfrTimeFlags ',')?
                vfrStatementQuestionOptionList[localctx.Node]
            )
            |
            (
                'hour' 'varid' '=' S1=StringIdentifier '.' S2=StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' S3=StringIdentifier ')' ','
                'help' '=' 'STRING_TOKEN' '(' S4=StringIdentifier ')' ','
                minMaxTimeStepDefault[localctx.Val, 0]
                'minute' 'varid' '=' S5=StringIdentifier '.' S6=StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' S7=StringIdentifier ')' ','
                'help' '=' 'STRING_TOKEN' '(' S8=StringIdentifier ')' ','
                minMaxTimeStepDefault[localctx.Val, 1]
                'second' 'varid' '=' S9=StringIdentifier '.' S10=StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' S11=StringIdentifier ')' ','
                'help' '=' 'STRING_TOKEN' '(' S12=StringIdentifier ')' ','
                minMaxTimeStepDefault[localctx.Val, 2]
                (F2='flags' '=' vfrTimeFlags ',')?
                (vfrStatementInconsistentIf)*
            )
        )
        'endtime' ';'
    ;

minMaxTimeStepDefault[Time, KeyValue]
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
        ('default' '=' N=Number ',')?
    ;

vfrTimeFlags
locals[LFlags=0]
    :   timeFlagsField ('|' timeFlagsField)*
    ;

timeFlagsField
locals[LFlag=0]
    :   Number
    |   'HOUR_SUPPRESS'
    |   'MINUTE_SUPPRESS'
    |   'SECOND_SUPPRESS'
    |   'STORAGE_NORMAL'
    |   'STORAGE_TIME'
    |   'STORAGE_WAKEUP'
    |   S=StringIdentifier
    ;

vfrStatementConditional
locals[Node]
    :   vfrStatementDisableIfStat
    |   vfrStatementSuppressIfStat //enhance to be compatible for framework endif
    |   vfrStatementGrayOutIfStat
    |   vfrStatementInconsistentIfStat   //to be compatible for framework
    ;

// new seems to be the same as the old, why new?
vfrStatementConditionalNew
locals[Node]
    :   vfrStatementDisableIfStat
        vfrStatementSuppressIfStatNew
        vfrStatementGrayOutIfStatNew
        vfrStatementInconsistentIfStat   //to be compatible for framework
    ;

vfrStatementSuppressIfStat
locals[Node]
    :   vfrStatementSuppressIfStatNew
    ;

vfrStatementGrayOutIfStat
locals[Node]
    :   vfrStatementGrayOutIfStatNew
    ;

vfrStatementStatList
locals[Node]
    :   vfrStatementStat
    |   vfrStatementQuestions
    |   vfrStatementConditional
    |   vfrStatementLabel
    |   vfrStatementExtension
    |   vfrStatementInvalid
    ;

vfrStatementStatListOld
    :   vfrStatementStat
    |   vfrStatementQuestions
    |   vfrStatementLabel
  // Just for framework vfr compatibility
    |   vfrStatementInvalid
    ;

vfrStatementDisableIfStat
locals[Node=VfrTreeNode(EFI_IFR_DISABLE_IF_OP)]
    :   'disableif' vfrStatementExpression[localctx.Node] ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;


// Compatible for framework vfr file
//
vfrStatementgrayoutIfSuppressIf
locals[Node]
    :   'suppressif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node] ';'
    ;

vfrStatementsuppressIfGrayOutIf
locals[Node]
    :   'grayoutif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node] ';'
    ;

vfrStatementSuppressIfStatNew
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'suppressif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node] ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementGrayOutIfStatNew
locals[Node=VfrTreeNode(EFI_IFR_GRAY_OUT_IF_OP)]
    :   'grayoutif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node] ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementInconsistentIfStat
locals[Node=VfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP)]
    :   'inconsistentif'
        'prompt' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression[localctx.Node]
        'endif' ';'
    ;

vfrStatementInvalid // for compatibility
    :   vfrStatementInvalidHidden
    |   vfrStatementInvalidInventory
    |   vfrStatementInvalidSaveRestoreDefaults
    ;

vfrStatementInvalidHidden
    :   'hidden'
        'value' '=' (V=Number | S=StringIdentifier) ','
        'key' '=' (KV=Number | KS=StringIdentifier) ';'
    ;

vfrStatementInvalidInventory
    :   'inventory'
        'help' '=' 'STRING_TOKEN' '(' StringIdentifier ')' ','
        'text' '=' 'STRING_TOKEN' '(' StringIdentifier ')' ','
        ('text' '=' 'STRING_TOKEN' '(' StringIdentifier ')')?
        ';'
    ;

vfrStatementInvalidSaveRestoreDefaults
    :   (   'save'
        |   'restore'
        )
        'defaults' ','
        'formid' '=' (N=Number | S=StringIdentifier) ','
        'prompt' '=' 'STRING_TOKEN' '(' PS=StringIdentifier ')' ','
        'help' '=' 'STRING_TOKEN' '(' HS=StringIdentifier ')'
        (',' 'flags' '=' flagsField ('|' flagsField)*)?
        (',' 'key' '=' (KN=Number | KS=StringIdentifier))?
        ';'
    ;
vfrStatementLabel
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   'label' (N=Number | S=StringIdentifier) ';'
    ;

vfrStatementBanner // Is TimeOut needed
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   'banner' (',')?
        'title' '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ','
        (   (   'line' (NL=Number | SL=StringIdentifier) ','
                'align' ('left' | 'center' | 'right') ';'
            )
            |
            (   'timeout' '=' (TN=Number | TS=StringIdentifier) ';'
            )
        )
    ;

vfrStatementExtension
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP), Buffer=None, Size=0, TypeName='', TypeSize=0, IsStruct=False, ArrayNum=0]
    :   'guidop'
        'guid' '=' S=StringIdentifier
        (   ',' D='datatype' '='
            (   'UINT64' ('[' Number ']')?
            |   'UINT32' ('[' Number ']')?
            |   'UINT16' ('[' Number ']')?
            |   'UINT8' ('[' Number ']')?
            |   'BOOLEAN' ('[' Number ']')?
            |   'EFI_STRING_ID' ('[' Number ']')?
            |   'EFI_HII_DATE' ('[' Number ']')?
            |   'EFI_HII_TIME' ('[' Number ']')?
            |   'EFI_HII_REF' ('[' Number ']')?
            |   D=StringIdentifier ('[' Number ']')?
            )
            (vfrExtensionData)*
        )?
        (
            ',' (vfrStatementExtension)*
            'endguidop'
        )?
        ';'
    ;


vfrExtensionData
locals[TFName='', FName='', TFValue=None]
    :   ',' 'data' ('[' I=Number ']')?
        ( '.' arrayName)*  '=' N=Number
    ;


vfrStatementModal
locals[Node]
    : vfrModalTag ';'
    ;

vfrModalTag
locals[Node=VfrTreeNode(EFI_IFR_MODAL_TAG_OP)]
    :   'modal'
    ;

vfrStatementExpression[ParentNode]
locals[ExpInfo=ExpressionInfo(), Nodes=[]]
    :   andTerm[localctx.ExpInfo] (L='OR' andTerm[localctx.ExpInfo])*
    ;

vfrStatementExpressionSub[ParentNodes]
locals[ExpInfo=ExpressionInfo(), Nodes=[]]
    :   andTerm[localctx.ExpInfo] ('OR' andTerm[localctx.ExpInfo])*
    ;

andTerm[ExpInfo]
locals[Nodes=[], Line]
    :   bitwiseorTerm[ExpInfo] (L='AND' bitwiseorTerm[ExpInfo])*
    ;

bitwiseorTerm[ExpInfo]
locals[Nodes=[], Line]
    :   bitwiseandTerm[ExpInfo] (L='|' bitwiseandTerm[ExpInfo])*
    ;


bitwiseandTerm[ExpInfo]
locals[Nodes=[], Line]
    :   equalTerm[ExpInfo] (L='&' equalTerm[ExpInfo])*
    ;


equalTerm[ExpInfo]
locals[Nodes=[], Line]
    :   compareTerm[ExpInfo]
        (equalTermSupplementary[ExpInfo])*
    ;


equalTermSupplementary[ExpInfo]
locals[Nodes=[]]
    :   ('==' compareTerm[ExpInfo])  # equalTermEqualRule
        |
        ('!=' compareTerm[ExpInfo]) # equalTermNotEqualRule
    ;

compareTerm[ExpInfo]
locals[Nodes=[]]
    :   shiftTerm[ExpInfo]
        (compareTermSupplementary[ExpInfo])*
    ;

compareTermSupplementary[ExpInfo]
locals[Nodes=[]]
    :   ('<' shiftTerm[ExpInfo])   # compareTermLessRule
        |
        ('<=' shiftTerm[ExpInfo])  #  compareTermLessEqualRule
        |
        ('>' shiftTerm[ExpInfo])   #  compareTermGreaterRule
        |
        ('>=' shiftTerm[ExpInfo])  #  compareTermGreaterEqualRule
    ;

shiftTerm[ExpInfo]
locals[Nodes=[]]
    :   addMinusTerm[ExpInfo]
        (shiftTermSupplementary[ExpInfo])*
    ;

shiftTermSupplementary[ExpInfo]
locals[Nodes=[]]
    :   ('<<' addMinusTerm[ExpInfo])  # shiftTermLeft
        |
        ('>>' addMinusTerm[ExpInfo]) # shiftTermRight
    ;

addMinusTerm[ExpInfo]
locals[Nodes=[]]
    :   multdivmodTerm[ExpInfo]
        (addMinusTermSupplementary[ExpInfo])*
    ;

addMinusTermSupplementary[ExpInfo]
locals[Nodes=[]]
    :   ('+' multdivmodTerm[ExpInfo]) # addMinusTermpAdd
        |
        ('-' multdivmodTerm[ExpInfo]) # addMinusTermSubtract
    ;

multdivmodTerm[ExpInfo]
locals[Nodes=[]]
    :   castTerm[ExpInfo]
        (multdivmodTermSupplementary[ExpInfo])*
    ;

multdivmodTermSupplementary[ExpInfo]
locals[Nodes=[]]
    :   ('*' castTerm[ExpInfo]) # multdivmodTermMul
        |
        ('/' castTerm[ExpInfo]) # multdivmodTermDiv
        |
        ('%' castTerm[ExpInfo]) # multdivmodTermModulo
    ;

castTerm[ExpInfo]
locals[Nodes=[]]
    :   (castTermSub)*
        atomTerm[ExpInfo]
    ;

castTermSub
locals[CastType=0xFF]
    :   '('
        (  'BOOLEAN'
        |  'UINT64'
        |  'UINT32'
        |  'UINT16'
        |  'UINT8'
        )
        ')'

    ;

atomTerm[ExpInfo]
locals[Nodes=[]]
    :   vfrExpressionCatenate[ExpInfo]
    |   vfrExpressionMatch[ExpInfo]
    |   vfrExpressionMatch2[ExpInfo]
    |   vfrExpressionParen[ExpInfo]
    |   vfrExpressionBuildInFunction[ExpInfo]
    |   vfrExpressionConstant[ExpInfo]
    |   vfrExpressionUnaryOp[ExpInfo]
    |   vfrExpressionTernaryOp[ExpInfo]
    |   vfrExpressionMap[ExpInfo]
    |   ('NOT' atomTerm[ExpInfo])
    ;

vfrExpressionCatenate[ExpInfo]
locals[Nodes=[]]
    :   'catenate'
        '(' vfrStatementExpressionSub[localctx.Nodes] ',' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

vfrExpressionMatch[ExpInfo]
locals[Nodes=[]]
    :   'match'
        '(' vfrStatementExpressionSub[localctx.Nodes]  ',' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

vfrExpressionMatch2[ExpInfo]
locals[Nodes=[]]
    :   'match2'
        '(' vfrStatementExpressionSub[localctx.Nodes] ','
        vfrStatementExpressionSub[localctx.Nodes]  ','
        S=StringIdentifier ')'
    ;

vfrExpressionParen[ExpInfo]
locals[Nodes=[]]
    :   '(' vfrStatementExpressionSub[localctx.Nodes]  ')'
    ;

vfrExpressionBuildInFunction[ExpInfo]
locals[Node]
    :   dupExp[ExpInfo]
    |   vareqvalExp[ExpInfo]
    |   ideqvalExp[ExpInfo]
    |   ideqidExp[ExpInfo]
    |   ideqvallistExp[ExpInfo]
    |   questionref1Exp[ExpInfo]
    |   rulerefExp[ExpInfo]
    |   stringref1Exp[ExpInfo]
    |   pushthisExp[ExpInfo]
    |   securityExp[ExpInfo]
    |   getExp[ExpInfo]
    ;

dupExp[ExpInfo]
locals[Node=VfrTreeNode(EFI_IFR_DUP_OP)]
    :   'dup'
    ;


vareqvalExp[ExpInfo] //2.15
locals[Node]
    :   'vareqval'
        'var' '(' VN=Number ')'
        (   '==' Number
	    |   '<=' Number
	    |   '<'  Number
	    |   '>=' Number
	    |   '>'  Number
        )
    ;

ideqvalExp[ExpInfo]
locals[Node]
    :   I='ideqval' vfrQuestionDataFieldName
        (   '==' Number
	    |   '<=' Number
	    |   '<' Number
	    |   '>=' Number
	    |   '>' Number
        )
    ;

ideqidExp[ExpInfo]
locals[Node]
    :   I='ideqid' vfrQuestionDataFieldName
        (   E='==' vfrQuestionDataFieldName
	    |   LE='<=' vfrQuestionDataFieldName
	    |   L='<' vfrQuestionDataFieldName
	    |   BE='>=' vfrQuestionDataFieldName
	    |   B='>' vfrQuestionDataFieldName
        )
    ;

ideqvallistExp[ExpInfo]
locals[Node]
    :   'ideqvallist' vfrQuestionDataFieldName '==' (Number)+
    ;

vfrQuestionDataFieldName
locals[QId=EFI_QUESTION_ID_INVALID, Mask=0, VarIdStr='', Line=None]
    :   (SN1=StringIdentifier '[' I=Number ']') # vfrQuestionDataFieldNameRule1
        |   (SN2=StringIdentifier ('.' arrayName)*) # vfrQuestionDataFieldNameRule2
    ;

arrayName
locals[SubStr='', SubStrZ='']
    : StringIdentifier ('[' N=Number ']')?
    ;

questionref1Exp[ExpInfo]
locals[Node=VfrTreeNode(EFI_IFR_QUESTION_REF1_OP)]
    :   'questionref'
        '(' ( StringIdentifier | Number ) ')'
    ;

rulerefExp[ExpInfo]
locals[Node=VfrTreeNode(EFI_IFR_RULE_REF_OP)]
    :   'ruleref' '(' StringIdentifier ')'
    ;

stringref1Exp[ExpInfo]
locals[Node=VfrTreeNode(EFI_IFR_STRING_REF1_OP)]
    :   'stringref' '('
        (
            'STRING_TOKEN' '(' S=StringIdentifier ')'
        |   Number
        )


    ')'
    ;

pushthisExp[ExpInfo]
locals[Node=VfrTreeNode(EFI_IFR_THIS_OP)]
    :   'pushthis'
    ;

securityExp[ExpInfo]
locals[Node=VfrTreeNode(EFI_IFR_SECURITY_OP)]
    :   'security' '(' S=StringIdentifier ')'
    ;

numericVarStoreType
locals[VarType]
    :   'NUMERIC_SIZE_1'
    |   'NUMERIC_SIZE_2'
    |   'NUMERIC_SIZE_4'
    |   'NUMERIC_SIZE_8'
    ;

getExp[ExpInfo]
locals[BaseInfo=EFI_VARSTORE_INFO(), Node=VfrTreeNode(EFI_IFR_GET_OP)]
    :   'get' '(' vfrStorageVarId[localctx.BaseInfo, False]('|' 'flags' '=' numericVarStoreType)? ')'
    ;

vfrExpressionConstant[ExpInfo] //
locals[Node]
    :   'TRUE'
    |   'FALSE'
    |   'ONE'
    |   'ONES'
    |   'ZERO'
    |   'UNDEFINED'
    |   'VERSION'
    |   Number
    ;

vfrExpressionUnaryOp[ExpInfo]
locals[Nodes]
    :   lengthExp[ExpInfo]
    |   bitwisenotExp[ExpInfo]
    |   question23refExp[ExpInfo]
    |   stringref2Exp[ExpInfo]
    |   toboolExp[ExpInfo]
    |   tostringExp[ExpInfo]
    |   unintExp[ExpInfo]
    |   toupperExp[ExpInfo]
    |   tolwerExp[ExpInfo]
    |   setExp[ExpInfo]
    ;

lengthExp[ExpInfo]
locals[Nodes=[]]
    :   'length' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

bitwisenotExp[ExpInfo]
locals[Nodes=[]]
    :   '~' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

question23refExp[ExpInfo]
locals[Nodes=[]]
    :   'questionrefval'
        '('
        (DevicePath '=' 'STRING_TOKEN' '(' S=StringIdentifier ')' ',' )?
        (Uuid '=' S2=StringIdentifier ',' )?
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

stringref2Exp[ExpInfo]
locals[Nodes=[]]
    :   'stringrefval' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

toboolExp[ExpInfo]
locals[Nodes=[]]
    :   'boolval' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

tostringExp[ExpInfo]
locals[Nodes=[]]
    :   'stringval' ('format' '=' Number ',' )?
        '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

unintExp[ExpInfo]
locals[Nodes=[]]
    :   'unintval' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

toupperExp[ExpInfo]
locals[Nodes=[]]
    :   'toupper' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

tolwerExp[ExpInfo]
locals[Nodes=[]]
    :   'tolower' '(' vfrStatementExpressionSub[localctx.Nodes] ')'
    ;

setExp[ExpInfo]
locals[BaseInfo=EFI_VARSTORE_INFO(), Nodes=[]]
    :   'set'
        '('
        vfrStorageVarId[localctx.BaseInfo, False]('|' 'flags' '=' numericVarStoreType)? ','
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

vfrExpressionTernaryOp[ExpInfo]
locals[Nodes]
    :   conditionalExp[ExpInfo]
    |   findExp[ExpInfo]
    |   midExp[ExpInfo]
    |   tokenExp[ExpInfo]
    |   spanExp[ExpInfo]
    ;

conditionalExp[ExpInfo]
locals[Nodes=[]]
    :   'cond'
        '('
        vfrStatementExpressionSub[localctx.Nodes]
        '?'
        vfrStatementExpressionSub[localctx.Nodes]
        ':'
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

findExp[ExpInfo]
locals[Nodes=[]]
    :   'find'
        '('
        findFormat[ExpInfo] ('|' findFormat[ExpInfo])*
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

findFormat[ExpInfo]
locals[Format=0]
    :   'SENSITIVE' | 'INSENSITIVE'
    ;

midExp[ExpInfo]
locals[Nodes=[]]
    :   'mid'
        '('
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

tokenExp[ExpInfo]
locals[Nodes=[]]
    :   'token'
        '('
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

spanExp[ExpInfo]
locals[Nodes=[]]
    :   'span'
        '('
        'flags' '=' spanFlags ('|' spanFlags)*
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ','
        vfrStatementExpressionSub[localctx.Nodes]
        ')'
    ;

spanFlags
locals[Flag=0]
    :   Number
    |   'LAST_NON_MATCH'
    |   'FIRST_NON_MATCH'
    ;

vfrExpressionMap[ExpInfo]
locals[Nodes=[], Node=VfrTreeNode()]
    :   'map'
        '('
        vfrStatementExpressionSub[localctx.Nodes]
        ':'
        (   vfrStatementExpression[localctx.Node]
            ','
            vfrStatementExpression[localctx.Node]
            ';'
        )*
        ')'
    ;


Define :'#define';
Include : '#include';
FormPkgType : 'formpkgtype';
OpenBrace : '{';
CloseBrace : '}';
OpenParen : '(';
CloseParen : ')';
OpenBracket : '[';
CloseBracket : ']';
Dot : '.';
Negative : '-';
Colon : ':';
Slash : '/';
Semicolon : ';';
Comma : ',';
Equal : '==';
NotEqual : '!=';
LessEqual: '<=';
Less:'<';
GreaterEqual:'>=';
Greater:'>';
BitWiseOr: '|';
BitWiseAnd: '&';

DevicePath : 'devicepath';
FormSet : 'formset';
FormSetId : 'formsetid';
EndFormSet : 'endformset';
Title : 'title';
FormId : 'formid';
OneOf : 'oneof';
EndOneOf : 'endoneof';
Prompt : 'prompt';
OrderedList : 'orderedlist';
MaxContainers : 'maxcontainers';
EndList : 'endlist';
EndForm : 'endform';
Form : 'form';
FormMap : 'formmap';
MapTitle : 'maptitle';
MapGuid : 'mapguid';
Subtitle : 'subtitle';
EndSubtitle : 'endsubtitle';
Help : 'help';
Text : 'text';
Option : 'option';
FLAGS : 'flags';
Date : 'date';
EndDate : 'enddate';
Year : 'year';
Month : 'month';
Day : 'day';
Time : 'time';
EndTime : 'endtime';
Hour : 'hour';
Minute : 'minute';
Second : 'second';
GrayOutIf : 'grayoutif';
Label : 'label';
Timeout : 'timeout';
Inventory : 'inventory';
NonNvDataMap : '_NON_NV_DATA_MAP';
Struct : 'struct';
Union : 'union';
Boolean : 'BOOLEAN';
Uint64 : 'UINT64';
Uint32 : 'UINT32';
Uint16 : 'UINT16';
Uint8 :'UINT8';
EFI_STRING_ID :'EFI_STRING_ID';
EFI_HII_DATE : 'EFI_HII_DATE';
EFI_HII_TIME : 'EFI_HII_TIME';
EFI_HII_REF : 'EFI_HII_REF';
Uuid : 'guid';
CheckBox : 'checkbox';
EndCheckBox : 'endcheckbox';
Numeric : 'numeric';
EndNumeric : 'endnumeric';
Minimum : 'minimum';
Maximum : 'maximum';
Step : 'step';
Default : 'default';
Password : 'password';
EndPassword : 'endpassword';
String : 'string';
EndString : 'endstring';
MinSize : 'minsize';
MaxSize : 'maxsize';
Encoding : 'encoding';
SuppressIf : 'suppressif';
DisableIf : 'disableif';
Hidden : 'hidden';
Goto : 'goto';
FormSetGuid : 'formsetguid';
InconsistentIf : 'inconsistentif';
WarningIf : 'warningif';
NoSubmitIf : 'nosubmitif';
EndIf : 'endif';
Key : 'key';
DefaultFlag : 'DEFAULT';
ManufacturingFlag : 'MANUFACTURING';
CheckBoxDefaultFlag : 'CHECKBOX_DEFAULT';
CheckBoxDefaultMfgFlag : 'CHECKBOX_DEFAULT_MFG';
InteractiveFlag : 'INTERACTIVE';
NVAccessFlag : 'NV_ACCESS';
ResetRequiredFlag : 'RESET_REQUIRED';
ReconnectRequiredFlag : 'RECONNECT_REQUIRED';
LateCheckFlag : 'LATE_CHECK';
ReadOnlyFlag : 'READ_ONLY';
OptionOnlyFlag : 'OPTIONS_ONLY';
RestStyleFlag : 'REST_STYLE';
Class : 'class';
Subclass : 'subclass';
ClassGuid : 'classguid';
TypeDef : 'typedef';
Restore : 'restore';
Save : 'save';
Defaults : 'defaults';
Banner :  'banner';
Align : 'align';
Left : 'left';
Right : 'right';
Center : 'center';
Line : 'line';
Name : 'name';

VarId: 'varid';
Question: 'question';
QuestionId: 'questionid';
Image: 'image';
Locked: 'locked';
Rule: 'rule';
EndRule: 'endrule';
Value: 'value';
Read: 'read';
Write: 'write';
ResetButton: 'resetbutton';
EndResetButton: 'endresetbutton';
DefaultStore: 'defaultstore';
Attribute: 'attribute';
Varstore: 'varstore';
Efivarstore: 'efivarstore';
VarSize: 'varsize';
NameValueVarStore: 'namevaluevarstore';
Action: 'action';
Config: 'config';
EndAction: 'endaction';
Refresh: 'refresh';
Interval: 'interval';
VarstoreDevice: 'varstoredevice';
GuidOp: 'guidop';
EndGuidOp: 'endguidop';
DataType: 'datatype';
Data: 'data';
Modal: 'modal';

//
// Define the class and subclass tokens
//
//
ClassNonDevice: 'NON_DEVICE';
ClassDiskDevice: 'DISK_DEVICE';
ClassVideoDevice: 'VIDEO_DEVICE';
ClassNetworkDevice: 'NETWORK_DEVICE';
ClassInputDevice: 'INPUT_DEVICE';
ClassOnBoardDevice: 'ONBOARD_DEVICE';
ClassOtherDevice: 'OTHER_DEVICE';

SubclassSetupApplication: 'SETUP_APPLICATION';
SubclassGeneralApplication: 'GENERAL_APPLICATION';
SubclassFrontPage: 'FRONT_PAGE';
SubclassSingleUse: 'SINGLE_USE';

YearSupppressFlag: 'YEAR_SUPPRESS';
MonthSuppressFlag: 'MONTH_SUPPRESS';
DaySuppressFlag: 'DAY_SUPPRESS';
HourSupppressFlag: 'HOUR_SUPPRESS';
MinuteSuppressFlag: 'MINUTE_SUPPRESS';
SecondSuppressFlag: 'SECOND_SUPPRESS';
StorageNormalFlag: 'STORAGE_NORMAL';
StorageTimeFlag: 'STORAGE_TIME';
StorageWakeUpFlag: 'STORAGE_WAKEUP';

UniQueFlag: 'UNIQUE';
NoEmptyFlag: 'NOEMPTY';

Cond: 'cond';
Find: 'find';
Mid: 'mid';
Tok: 'token';
Span: 'span';

// The syntax of expression

Dup: 'dup';
VarEqVal: 'vareqval';
Var: 'var';
IdEqVal: 'ideqval';
IdEqId: 'ideqid';
IdEqValList: 'ideqvallist';
QuestionRef: 'questionref';
RuleRef: 'ruleref';
StringRef: 'stringref';
PushThis: 'pushthis';
Security: 'security';
Get: 'get';
TrueSymbol: 'TRUE';
FalseSymbol: 'FALSE';
One: 'ONE';
Ones: 'ONES';
Zero: 'ZERO';
Undefined: 'UNDEFINED';
Version: 'VERSION';
Length: 'length';
AND: 'AND';
OR: 'OR';
NOT: 'NOT';
Set: 'set';
BitWiseNot: '~';
BoolVal: 'boolval';
StringVal: 'stringval';
UnIntVal: 'unintval';
ToUpper: 'toupper';
ToLower: 'tolower';
Match: 'match';
Match2: 'match2';
Catenate: 'catenate';
QuestionRefVal: 'questionrefval';
StringRefVal: 'stringrefval';
Map: 'map';
RefreshGuid: 'refreshguid';
StringToken: 'STRING_TOKEN';

OptionDefault: 'OPTION_DEFAULT';
OptionDefaultMfg: 'OPTION_DEFAULT_MFG';

NumericSizeOne: 'NUMERIC_SIZE_1';
NumericSizeTwo: 'NUMERIC_SIZE_2';
NumericSizeFour: 'NUMERIC_SIZE_4';
NumericSizeEight: 'NUMERIC_SIZE_8';
DisPlayIntDec: 'DISPLAY_INT_DEC';
DisPlayUIntDec: 'DISPLAY_UINT_DEC';
DisPlayUIntHex: 'DISPLAY_UINT_HEX';

Insensitive:  'INSENSITIVE';
Sensitive: 'SENSITIVE';

LastNonMatch: 'LAST_NON_MATCH';
FirstNonMatch: 'FIRST_NON_MATCH';

Number
    :   ('0x'[0-9A-Fa-f]+) | [0-9]+
    ;

StringIdentifier
    :   [A-Za-z_][A-Za-z_0-9]*
    ;

ComplexDefine
    :   '#' Whitespace? 'define'  ~[#\r\n]*
        -> skip
    ;

LineDefinition
    :   '#' Whitespace? 'line'  ~[#\r\n]*
        -> skip
    ;


IncludeDefinition
    :   '#' Whitespace? 'include'  ~[#\r\n]*
        -> skip
    ;

Whitespace
    :   [ \t]+
        -> skip
    ;

GuidSubDefinition
    :   Number ',' Number ',' Number ',' Number ','
        Number ',' Number ',' Number ',' Number
    ;

GuidDefinition
    :   '{'
        Number ',' Number ',' Number ','
        (   '{' GuidSubDefinition '}'
        |   GuidSubDefinition
        )
        '}'
        ->skip
    ;

DefineLine
    :   '#' Whitespace? 'define'  .* [\r\n] Whitespace? '{' ~[\r\n]*
        ->skip
    ;

Newline
    :   (   '\r' '\n'?
        |   '\n'
        )
        -> skip
    ;

LineComment
    :   '//' ~[\r\n]*
        -> skip
    ;

// Skip over 'extern' in any included .H file
Extern
    : 'extern' ~[#\r\n]*
        -> skip
    ;