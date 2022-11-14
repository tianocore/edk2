grammar VfrSyntax;
options {
    language=Python;
}
@header{

from CommonCtypes import *
from VfrFormPkg import *
from VfrUtility import *
from VfrTree import *
}

vfrProgram
    :   (vfrPragmaPackDefinition | vfrDataStructDefinition | vfrDataUnionDefinition)* vfrFormSetDefinition
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

// 2.4 VFR FormSet Definition
vfrFormSetDefinition
locals[Node=VfrTreeNode(EFI_IFR_FORM_SET_OP)]
    :   'formset'
        'guid' '=' guidDefinition ','
        'title' '=' 'STRING_TOKEN' '(' Number ')' ','
        'help' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('classguid' '=' classguidDefinition[localctx.Node] ',')?
        ('class' '=' classDefinition ',')?
        ('subclass' '=' subclassDefinition ',')?
        vfrFormSetList[localctx.Node]
        'endformset' ';'
    ;

classguidDefinition[Node]
locals[GuidList=[]]
    :   guidDefinition ('|' guidDefinition)? ('|' guidDefinition)?('|' guidDefinition)?
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
    |   Number
    ;

subclassDefinition
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   SubclassSetupApplication
    |   SubclassGeneralApplication
    |   SubclassFrontPage
    |   SubclassSingleUse
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
    |   vfrStatementDisableIfFormSet
    |   vfrStatementSuppressIfFormSet
    |   vfrStatementExtension
    )
    ;


//2.6 VFR Default Stores Definition
vfrStatementDefaultStore
locals[Node=VfrTreeNode(EFI_IFR_DEFAULTSTORE_OP)]
    :   'defaultstore' N=StringIdentifier ','
        'prompt' '=' 'STRING_TOKEN' '(' S=Number ')'
        (',' 'attribute' '=' A=Number)? ';'
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
        ('varid' '=' ID=Number ',')?
        'name' '=' SN=StringIdentifier ','
        'guid' '=' guidDefinition ';'
    ;

vfrStatementVarStoreEfi
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
        ('varid' '=' ID=Number ',')?
        'attribute' '=' vfrVarStoreEfiAttr ('|' vfrVarStoreEfiAttr)* ','
        (   'name' '=' SN=StringIdentifier ','
        |   'name' '=' 'STRING_TOKEN' '(' VN=Number ')' ','  'varsize' '=' N=Number ','
        )
        'guid' '=' guidDefinition ';'
    ;

vfrVarStoreEfiAttr
locals[Attr=0]
    :   Number;

vfrStatementVarStoreNameValue
locals[Node=VfrTreeNode(EFI_IFR_VARSTORE_NAME_VALUE_OP)]
    :   'namevaluevarstore' SN=StringIdentifier ','
        ('varid' '=' ID=Number ',')?
        ('name' '=' 'STRING_TOKEN' '(' Number ')' ',')+
        'guid' '=' guidDefinition ';'
    ;

vfrStatementDisableIfFormSet
locals[Node=VfrTreeNode(EFI_IFR_DISABLE_IF_OP)]
    :   'disableif' vfrStatementExpression ';'
        vfrFormSetList[localctx.Node]
        'endif' ';'
    ;

vfrStatementSuppressIfFormSet
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'suppressif' vfrStatementExpression ';'
        vfrFormSetList[localctx.Node]
        'endif' ';'
    ;

guidSubDefinition[Guid]
    :   Number ',' Number ',' Number ',' Number ','
        Number ',' Number ',' Number ',' Number
    ;

guidDefinition
locals[Node=VfrTreeNode(), Guid=EFI_GUID()]
    :   '{'
        Number ',' Number ',' Number ','
        (   '{' guidSubDefinition[localctx.Guid] '}'
        |   guidSubDefinition[localctx.Guid]
        )
        '}'
    ;

getStringId
locals[StringId='']
    :   'STRING_TOKEN' '(' Number ')'
    ;

vfrQuestionHeader[OpObj, QType]
    :   vfrQuestionBaseInfo[OpObj, QType]
        vfrStatementHeader[OpObj]
    ;

vfrQuestionBaseInfo[OpObj, QType]
locals[BaseInfo=EFI_VARSTORE_INFO(), QId=EFI_QUESTION_ID_INVALID, CheckFlag=True]
    :   ('name' '=' QN=StringIdentifier ',')?
        ('varid' '=' vfrStorageVarId[localctx.BaseInfo, localctx.CheckFlag] ',')?
        ('questionid' '=' ID=Number ',')?
    ;

vfrStatementHeader[OpObj]
    :   'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        'help' '=' 'STRING_TOKEN' '(' Number ')'
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

vfrConstantValueField
locals[Value=EFI_IFR_TYPE_VALUE(), ValueList=[], ListType=False]
    :   ('-')? Number
    |   'TRUE'
    |   'FALSE'
    |   'ONE'
    |   'ONES'
    |   'ZERO'
    |   Number ':' Number ':' Number
    |   Number '/' Number '/' Number
    |   Number ';' Number ';' guidDefinition ';' 'STRING_TOKEN' '(' Number ')'
    |   'STRING_TOKEN' '(' Number ')'
    |   '{' Number (',' Number)* '}'
    ;

vfrImageTag
locals[Node=VfrTreeNode(EFI_IFR_IMAGE_OP)]
    :   'image' '=' 'IMAGE_TOKEN' '(' Number ')'
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

vfrFormDefinition
locals[Node=VfrTreeNode(EFI_IFR_FORM_OP)]
    :   'form' 'formid' '=' Number ','
        'title' '=' 'STRING_TOKEN' '(' Number ')' ';'
        (vfrForm)*
        'endform' ';'
    ;

vfrForm
locals[Node]
    :
    (   vfrStatementImage
    |   vfrStatementLocked
    |   vfrStatementRules
    |   vfrStatementDefault
    |   vfrStatementStat
    |   vfrStatementQuestions
    |   vfrStatementConditional
    |   vfrStatementLabel
    |   vfrStatementBanner
    |   vfrStatementInvalid
    |   vfrStatementExtension
    |   vfrStatementModal
    |   vfrStatementRefreshEvent ';'
    )
    ;

vfrFormMapDefinition
locals[Node=VfrTreeNode(EFI_IFR_FORM_MAP_OP)]
    :   'formmap' 'formid' '=' S1=Number ','
        (   'maptitle' '=' 'STRING_TOKEN' '(' Number ')' ';'
            'mapguid' '=' guidDefinition ';'
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
        vfrStatementExpression
        'endrule' ';'
    ;

vfrStatementStat
locals[Node]
    :   vfrStatementSubTitle
    |   vfrStatementStaticText
    |   vfrStatementCrossReference
    ;

vfrStatementSubTitle
locals[Node=VfrTreeNode(EFI_IFR_SUBTITLE_OP), OpObj=CIfrSubtitle()]
    :   'subtitle'

        'text' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'flags' '=' vfrSubtitleFlags)?
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

vfrSubtitleFlags
locals[SubFlags=0]
    :   subtitleFlagsField ('|' subtitleFlagsField)*
    ;
subtitleFlagsField
locals[Flag=0]
    :   Number | 'HORIZONTAL'
    ;

vfrStatementStaticText
locals[Node=VfrTreeNode(EFI_IFR_TEXT_OP)]
    :   'text'
        'help' '=' 'STRING_TOKEN' '(' S1=Number ')' ','
        'text' '=' 'STRING_TOKEN' '(' S2=Number ')'
        (',' 'text' '=' 'STRING_TOKEN' '(' S3=Number ')')?
        (',' F='flags' '=' staticTextFlagsField ('|' staticTextFlagsField)* ',' 'key' '=' S4=Number)?
        (',' vfrStatementStatTagList[localctx.Node])? ';'
    ;

staticTextFlagsField
locals[Flag=0]
    :   N=Number | questionheaderFlagsField
    ;

vfrStatementCrossReference
locals[Node]
    :   vfrStatementGoto | vfrStatementResetButton
    ;

vfrStatementGoto
locals[Node=VfrTreeNode(EFI_IFR_REF_OP), OpObj=None, OHObj=None, QType=EFI_QUESION_TYPE.QUESTION_REF]
    :   'goto'
        (   (   DevicePath '=' 'STRING_TOKEN' '(' Number ')' ','
                FormSetGuid '=' guidDefinition ','
                FormId '=' Number ','
                Question '=' Number ','
            )
            |
            (   FormSetGuid '=' guidDefinition ','
                FormId '=' Number ','
                Question '=' Number ','
            )
            |
            (   FormId '=' Number ','
                Question '=' (QN=StringIdentifier ',' | Number ',')
            )
            |
            (   Number ',' )
        )?
        vfrQuestionHeader[localctx.OpObj, localctx.QType]
        (',' 'flags' '=' vfrGotoFlags[localctx.OpObj])?
        (',' 'key' '=' Number)?
        (',' vfrStatementQuestionOptionList[localctx.Node])? ';'
    ;

vfrGotoFlags[Obj]
locals[GotoFlags=0]
    :   gotoFlagsField('|' gotoFlagsField)*
    ;

gotoFlagsField
locals[Flag=0]
    :  N=Number | questionheaderFlagsField
    ;

vfrStatementResetButton
locals[Node=VfrTreeNode(EFI_IFR_RESET_BUTTON_OP), OpObj=CIfrResetButton()]
    :  'resetbutton'
       'defaultstore' '=' N=StringIdentifier ','
       vfrStatementHeader[localctx.OpObj] ','
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
    |   vfrStatementInconsistentIf
    |   vfrStatementNoSubmitIf
    |   vfrStatementDisableIfQuest
    |   vfrStatementRefresh
    |   vfrStatementVarstoreDevice
    |   vfrStatementExtension
    |   vfrStatementRefreshEvent
    |   vfrStatementWarningIf
    ;

vfrStatementInconsistentIf
locals[Node=VfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP)]
    :   'inconsistentif'
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression
        'endif' (';')?
    ;

vfrStatementNoSubmitIf
locals[Node=VfrTreeNode(EFI_IFR_NO_SUBMIT_IF_OP)]
    :   'nosubmitif'
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression
        'endif' (';')?
    ;

vfrStatementDisableIfQuest
locals[Node=VfrTreeNode(EFI_IFR_DISABLE_IF_OP)]
    :   'disableif' vfrStatementExpression ';'
        vfrStatementQuestionOptionList[localctx.Node]
        'endif' (';')?
    ;

vfrStatementRefresh
locals[Node=VfrTreeNode(EFI_IFR_REFRESH_OP)]
    :   'refresh' 'interval' '=' Number
    ;

vfrStatementVarstoreDevice
locals[Node=VfrTreeNode(EFI_IFR_VARSTORE_DEVICE_OP)]
    :   'varstoredevice' '=' 'STRING_TOKEN' '(' Number ')' ','
    ;

vfrStatementRefreshEvent
locals[Node=VfrTreeNode(EFI_IFR_REFRESH_ID_OP)]
    :   'refreshguid' '=' guidDefinition ','
    ;

vfrStatementWarningIf
locals[Node=VfrTreeNode(EFI_IFR_WARNING_IF_OP)]
    :   'warningif'
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('timeout' '=' Number ',')?
        vfrStatementExpression
        'endif' (';')?
    ;

vfrStatementQuestionTagList[Node]
    :   (vfrStatementQuestionTag)*
    ;

vfrStatementQuestionOptionTag
locals[Node]
    :   vfrStatementSuppressIfQuest
    |   vfrStatementGrayOutIfQuest
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
    ;

vfrStatementSuppressIfQuest
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'suppressif' vfrStatementExpression ';'
        ('flags' '=' flagsField ('|' flagsField )* ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endif' (';')?
    ;

vfrStatementGrayOutIfQuest
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'grayoutif' vfrStatementExpression ';'
        ('flags' '=' flagsField ('|' flagsField )* ',')?
        vfrStatementQuestionOptionList[localctx.Node]
        'endif' (';')?
    ;


vfrStatementDefault
locals[Node=VfrTreeNode(EFI_IFR_DEFAULT_OP)]
    :   D='default'
        (   (   vfrStatementValue ','
            |   '=' vfrConstantValueField ','
            )
            (   'defaultstore' '=' SN=StringIdentifier ','
            )?
        )
    ;

vfrStatementValue
locals[Node=VfrTreeNode(EFI_IFR_VALUE_OP)]
    :   'value' '=' vfrStatementExpression
    ;

vfrStatementOptions
locals[Node]
    :   vfrStatementOneOfOption
    ;

vfrStatementOneOfOption
locals[Node=VfrTreeNode(EFI_IFR_ONE_OF_OPTION_OP)]
    :   'option'
        'text' '=' 'STRING_TOKEN' '(' Number ')' ','
        'value' '=' vfrConstantValueField ','
        F='flags' '=' vfrOneOfOptionFlags (',' 'key' '=' KN=Number) ? (',' vfrImageTag)* ';'
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
    ;

vfrStatementRead
locals[Node=VfrTreeNode(EFI_IFR_READ_OP)]
    :   'read' vfrStatementExpression ';'
    ;

vfrStatementWrite
locals[Node=VfrTreeNode(EFI_IFR_WRITE_OP)]
    :   'write' vfrStatementExpression ';'
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
locals[Node=VfrTreeNode(EFI_IFR_CHECKBOX_OP), OpObj=CIfrCheckBox(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   L='checkbox'
        vfrQuestionBaseInfo[localctx.OpObj, localctx.QType]
        vfrStatementHeader[localctx.OpObj] ','
        (F='flags' '=' vfrCheckBoxFlags ',')?
        ('key' '=' Number ',')?
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
    ;

vfrStatementAction
locals[Node=VfrTreeNode(EFI_IFR_ACTION_OP),OpObj=CIfrAction(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'action'
        vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
        ('flags' '=' vfrActionFlags ',')?
        'config' '=' 'STRING_TOKEN' '(' Number ')' ','
        vfrStatementQuestionTagList[localctx.Node]
        'endaction' ';'
    ;

vfrActionFlags
locals[HFlags=0]
    :   actionFlagsField ('|' actionFlagsField)*
    ;
actionFlagsField
locals[HFlag=0]
    :   N=Number | questionheaderFlagsField
    ;

vfrStatementNumericType
locals[Node]
    :   vfrStatementNumeric | vfrStatementOneOf
    ;

vfrStatementNumeric
locals[Node=VfrTreeNode(EFI_IFR_NUMERIC_OP), OpObj=CIfrNumeric(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'numeric'
        vfrQuestionBaseInfo[localctx.OpObj, localctx.QType]
        vfrStatementHeader[localctx.OpObj] ','
        (F='flags' '=' vfrNumericFlags ',')?
        ('key' '=' Number ',')?
        vfrSetMinMaxStep[localctx.OpObj]
        vfrStatementQuestionOptionList[localctx.Node]
        'endnumeric' ';'
    ;

vfrSetMinMaxStep[OpObj] // CIfrMinMaxStepData
    :   'minimum' '=' (N1='-')? I=Number ','
        'maximum' '=' (N2='-')? A=Number ','
        ('step' '=' S=Number ',')?
    ;

vfrNumericFlags
locals[HFlags=0, LFlags=0,IsDisplaySpecified=False]
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
    ;

vfrStatementOneOf
locals[Node=VfrTreeNode(EFI_IFR_ONE_OF_OP), OpObj=CIfrOneOf(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'oneof'
        vfrQuestionBaseInfo[localctx.OpObj, localctx.QType]
        vfrStatementHeader[localctx.OpObj] ','
        (F='flags' '=' vfrOneofFlagsField ',')?
        (vfrSetMinMaxStep[localctx.OpObj])?
        vfrStatementQuestionOptionList[localctx.Node]
        'endoneof' ';'
    ;

vfrOneofFlagsField
locals[HFlags=0, LFlags=0]
    :   numericFlagsField ('|' numericFlagsField)*
    ;

vfrStatementStringType
locals[Node]
    :   vfrStatementString | vfrStatementPassword
    ;

vfrStatementString
locals[Node=VfrTreeNode(EFI_IFR_STRING_OP), OpObj=CIfrString(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'string'
        vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
        (F='flags' '=' vfrStringFlagsField ',')?
        ('key' '=' Number ',')?
        Min='minsize' '=' Number ','
        Max='maxsize' '=' Number ','
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
    |   'MULTI_LINE'
    |   questionheaderFlagsField
    ;

vfrStatementPassword
locals[Node=VfrTreeNode(EFI_IFR_PASSWORD_OP), OpObj=CIfrPassword(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'password'
        vfrQuestionHeader[localctx.OpObj, localctx.QType]','
        (F='flags' '=' vfrPasswordFlagsField ',')?
        ('key' '=' Number ',')?
        Min='minsize' '=' Number ','
        Max='maxsize' '=' Number ','
        ('encoding' '=' Number ',')?
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
    ;

vfrStatementOrderedList
locals[Node=VfrTreeNode(EFI_IFR_ORDERED_LIST_OP), OpObj=CIfrOrderedList(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'orderedlist'
        vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
        (M='maxcontainers' '=' Number ',')?
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
    ;

vfrStatementDate
locals[Node=VfrTreeNode(EFI_IFR_DATE_OP), OpObj=CIfrDate(), QType=EFI_QUESION_TYPE.QUESTION_DATE, Val=EFI_IFR_TYPE_VALUE()]
    :   'date'
        (   (   vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
                (F1='flags' '=' vfrDateFlags ',')?
                vfrStatementQuestionOptionList[localctx.Node]
            )
            |
            (   'year' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxDateStepDefault[localctx.Val.date, 0]
                'month' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxDateStepDefault[localctx.Val.date, 1]
                'day' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxDateStepDefault[localctx.Val.date, 2]
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
    ;

vfrStatementTime
locals[Node=VfrTreeNode(EFI_IFR_TIME_OP), OpObj=CIfrTime(), QType=EFI_QUESION_TYPE.QUESTION_TIME,  Val=EFI_IFR_TYPE_VALUE()]
    :   'time'
        (   (   vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
                (F1='flags' '=' vfrTimeFlags ',')?
                vfrStatementQuestionOptionList[localctx.Node]
            )
            |
            (
                'hour' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxTimeStepDefault[localctx.Val.time, 0]
                'minute' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxTimeStepDefault[localctx.Val.time, 1]
                'second' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxTimeStepDefault[localctx.Val.time, 2]
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
    :   'disableif' vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;


// Compatible for framework vfr file
//
vfrStatementgrayoutIfSuppressIf
    :   'suppressif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression ';'
    ;

vfrStatementsuppressIfGrayOutIf
    :   'grayoutif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression ';'
    ;

vfrStatementSuppressIfStatNew
locals[Node=VfrTreeNode(EFI_IFR_SUPPRESS_IF_OP)]
    :   'suppressif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementGrayOutIfStatNew
locals[Node=VfrTreeNode(EFI_IFR_GRAY_OUT_IF_OP)]
    :   'grayoutif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementInconsistentIfStat
locals[Node=VfrTreeNode(EFI_IFR_INCONSISTENT_IF_OP)]
    :   'inconsistentif'
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression
        'endif' ';'
    ;

vfrStatementInvalid // for compatibility
    :   vfrStatementInvalidHidden
    |   vfrStatementInvalidInventory
    |   vfrStatementInvalidSaveRestoreDefaults
    ;

vfrStatementInvalidHidden
    :   'hidden'
        'value' '=' Number ','
        'key' '=' Number ';'
    ;

vfrStatementInvalidInventory
    :   'inventory'
        'help' '=' 'STRING_TOKEN' '(' Number ')' ','
        'text' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('text' '=' 'STRING_TOKEN' '(' Number ')')?
        ';'
    ;

vfrStatementInvalidSaveRestoreDefaults
    :   (   'save'
        |   'restore'
        )
        'defaults' ','
        'formid' '=' Number ','
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        'help' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'flags' '=' flagsField ('|' flagsField)*)?
        (',' 'key' '=' Number)?
        ';'
    ;
vfrStatementLabel
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   'label' Number ';'
    ;

vfrStatementBanner // Is TimeOut needed
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP)]
    :   'banner' (',')?
        'title' '=' 'STRING_TOKEN' '(' Number ')' ','
        (   (   'line' Number ','
                'align' ('left' | 'center' | 'right') ';'
            )
            |
            (   'timeout' '=' Number ';'
            )
        )
    ;

vfrStatementExtension
locals[Node=VfrTreeNode(EFI_IFR_GUID_OP), DataBuff, Size=0, TypeName='', TypeSize=0, IsStruct=False, ArrayNum=0]
    :   'guidop'
        'guid' '=' guidDefinition
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
            |   StringIdentifier ('[' Number ']')?
            )
            (vfrExtensionData[localctx.DataBuff])*
        )?
        (
            ',' (vfrStatementExtension)*
            'endguidop'
        )?
        ';'
    ;


vfrExtensionData[DataBuff]
locals[IsStruct]
    :   ',' 'data' ('[' Number ']')?
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

vfrStatementExpression
locals[ExpInfo=ExpressionInfo()]
    :   andTerm[localctx.ExpInfo] ('OR' andTerm[localctx.ExpInfo])*
    ;

vfrStatementExpressionSub
locals[ExpInfo=ExpressionInfo()]
    :   andTerm[localctx.ExpInfo] ('OR' andTerm[localctx.ExpInfo])*
    ;

andTerm[ExpInfo]
locals[CIfrAndList=[]]
    :   bitwiseorTerm[ExpInfo] ('AND' bitwiseorTerm[ExpInfo])*
    ;

bitwiseorTerm[ExpInfo]
locals[CIfrBitWiseOrList=[]]
    :   bitwiseandTerm[ExpInfo] ('|' bitwiseandTerm[ExpInfo])*
    ;


bitwiseandTerm[ExpInfo]
locals[CIfrBitWiseAndList=[]]
    :   equalTerm[ExpInfo] ('&' equalTerm[ExpInfo])*
    ;


equalTerm[ExpInfo]
locals[CIfrEqualList=[], CIfrNotEqualList=[]]
    :   compareTerm[localctx.ExpInfo]
        (equalTermSupplementary[localctx.CIfrEqualList, localctx.CIfrNotEqualList, ExpInfo])*
    ;


equalTermSupplementary[CIfrEqualList, CIfrNotEqualList, ExpInfo]
    :   ('==' compareTerm[ExpInfo])  # equalTermEqualRule
        |
        ('!=' compareTerm[ExpInfo]) # equalTermNotEqualRule
    ;

compareTerm[ExpInfo]
locals[CIfrLessThanList=[], CIfrLessEqualList=[], CIfrGreaterThanList=[], CIfrGreaterEqualList=[]]
    :   shiftTerm[ExpInfo]
        (compareTermSupplementary[localctx.CIfrLessThanList, localctx.CIfrLessEqualList, localctx.CIfrGreaterThanList, localctx.CIfrGreaterEqualList, ExpInfo])*
    ;

compareTermSupplementary[CIfrLessThanList, CIfrLessEqualList, CIfrGreaterThanList, CIfrGreaterEqualList, ExpInfo]
    :   ('<' shiftTerm[ExpInfo])   # compareTermLessRule
        |
        ('<=' shiftTerm[ExpInfo])  #  compareTermLessEqualRule
        |
        ('>' shiftTerm[ExpInfo])   #  compareTermGreaterRule
        |
        ('>=' shiftTerm[ExpInfo])  #  compareTermGreaterEqualRule
    ;

shiftTerm[ExpInfo]
locals[CIfrShiftLeftList=[], CIfrShiftRightList=[]]
    :   addMinusTerm[ExpInfo]
        (shiftTermSupplementary[localctx.CIfrShiftLeftList, localctx.CIfrShiftRightList, ExpInfo])*
    ;

shiftTermSupplementary[CIfrShiftLeftList, CIfrShiftRightList, ExpInfo]
    :   ('<<' addMinusTerm[ExpInfo])  # shiftTermLeft
        |
        ('>>' addMinusTerm[ExpInfo]) # shiftTermRight
    ;

addMinusTerm[ExpInfo]
locals[CIfrAddList=[], CIfrSubtractList=[]]
    :   multdivmodTerm[ExpInfo]
        (addMinusTermSupplementary[localctx.CIfrAddList, localctx.CIfrSubtractList, ExpInfo])*
    ;

addMinusTermSupplementary[CIfrAddList, CIfrSubtractList, ExpInfo]
    :   ('+' multdivmodTerm[ExpInfo]) # addMinusTermpAdd
        |
        ('-' multdivmodTerm[ExpInfo]) # addMinusTermSubtract
    ;

multdivmodTerm[ExpInfo]
locals[CIfrMultiplyList=[], CIfrDivideList=[], CIfrModuloList=[]]
    :   castTerm[ExpInfo]
        (multdivmodTermSupplementary[localctx.CIfrMultiplyList, localctx.CIfrDivideList, localctx.CIfrModuloList, ExpInfo])*
    ;

multdivmodTermSupplementary[CIfrMultiplyList, CIfrDivideList, CIfrModuloList, ExpInfo]
    :   ('*' castTerm[ExpInfo]) # multdivmodTermMul
        |
        ('/' castTerm[ExpInfo]) # multdivmodTermDiv
        |
        ('%' castTerm[ExpInfo]) # multdivmodTermModulo
    ;

castTerm[ExpInfo]
locals[TBObj=None, TUObj=None]
    :   (   '('
            (  'BOOLEAN'
            |  'UINT64'
            |  'UINT32'
            |  'UINT16'
            |  'UINT8'
            )
            ')'
        )*
        atomTerm[ExpInfo]
    ;

atomTerm[ExpInfo]
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
    |   vfrExpressionMatch2[ExpInfo]
    ;

vfrExpressionCatenate[ExpInfo]
locals[CObj=None]
    :   'catenate'
        '(' vfrStatementExpressionSub ',' vfrStatementExpressionSub ')'
    ;

vfrExpressionMatch[ExpInfo]
locals[MObj=None]
    :   'match'
        '(' vfrStatementExpressionSub ',' vfrStatementExpressionSub ')'
    ;

vfrExpressionMatch2[ExpInfo]
locals[M2Obj=None]
    :   'match2'
        '(' vfrStatementExpressionSub','
        vfrStatementExpressionSub ','
        guidDefinition ')'
    ;

vfrExpressionParen[ExpInfo]
    :   '(' vfrStatementExpressionSub ')'
    ;

vfrExpressionBuildInFunction[ExpInfo]
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
locals[DObj=None]
    :   'dup'
    ;


vareqvalExp[ExpInfo]
    :   'vareqval'
        'var' '(' Number ')'
        (   '==' Number
	    |   '<=' Number
	    |   '<' Number
	    |   '>=' Number
	    |   '>' Number
        )
    ;

ideqvalExp[ExpInfo]
    :   I='ideqval' vfrQuestionDataFieldName
        (   '==' Number
	    |   '<=' Number
	    |   '<' Number
	    |   '>=' Number
	    |   '>' Number
        )
    ;

ideqidExp[ExpInfo]
    :   I='ideqid' vfrQuestionDataFieldName
        (   E='==' vfrQuestionDataFieldName
	    |   LE='<=' vfrQuestionDataFieldName
	    |   L='<' vfrQuestionDataFieldName
	    |   BE='>=' vfrQuestionDataFieldName
	    |   B='>' vfrQuestionDataFieldName
        )
    ;

ideqvallistExp[ExpInfo]
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
    :   'questionref'
        '(' ( StringIdentifier | Number ) ')'
    ;

rulerefExp[ExpInfo]
    :   'ruleref' '(' StringIdentifier ')'
    ;

stringref1Exp[ExpInfo]
    :   'stringref' '('
        (
            'STRING_TOKEN' '(' Number ')'
        |   Number
        )


    ')'
    ;

pushthisExp[ExpInfo]
    :   'pushthis'
    ;

securityExp[ExpInfo]
    :   'security' '(' guidDefinition ')'
    ;

numericVarStoreType
locals[VarType]
    :   'NUMERIC_SIZE_1'
    |   'NUMERIC_SIZE_2'
    |   'NUMERIC_SIZE_4'
    |   'NUMERIC_SIZE_8'
    ;

getExp[ExpInfo]
locals[BaseInfo=EFI_VARSTORE_INFO(), GObj=None]
    :   'get' '(' vfrStorageVarId[localctx.BaseInfo, False]('|' 'flags' '=' numericVarStoreType)? ')'
    ;

vfrExpressionConstant[ExpInfo]
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
locals[LObj=None]
    :   'length' '(' vfrStatementExpressionSub ')'
    ;

bitwisenotExp[ExpInfo]
locals[BWNObj=None]
    :   '~' '(' vfrStatementExpressionSub ')'
    ;

question23refExp[ExpInfo]
    :   'questionrefval'
        '('
        (DevicePath '=' 'STRING_TOKEN' '(' Number ')' ',' )?
        (Uuid '=' guidDefinition ',' )?
        vfrStatementExpressionSub
        ')'
    ;

stringref2Exp[ExpInfo]
locals[SR2Obj=None]
    :   'stringrefval' '(' vfrStatementExpressionSub ')'
    ;

toboolExp[ExpInfo]
locals[TBObj=None]
    :   'boolval' '(' vfrStatementExpressionSub ')'
    ;

tostringExp[ExpInfo]
locals[TSObj=None]
    :   'stringval' ('format' '=' Number ',' )?
        '(' vfrStatementExpressionSub ')'
    ;

unintExp[ExpInfo]
locals[TUObj=None]
    :   'unintval' '(' vfrStatementExpressionSub ')'
    ;

toupperExp[ExpInfo]
locals[TUObj=None]
    :   'toupper' '(' vfrStatementExpressionSub ')'
    ;

tolwerExp[ExpInfo]
locals[TLObj=None]
    :   'tolower' '(' vfrStatementExpressionSub ')'
    ;

setExp[ExpInfo]
locals[BaseInfo=EFI_VARSTORE_INFO(), TSObj=None]
    :   'set'
        '('
        vfrStorageVarId[localctx.BaseInfo, False]('|' 'flags' '=' numericVarStoreType)? ','
        vfrStatementExpressionSub
        ')'
    ;

vfrExpressionTernaryOp[ExpInfo]
    :   conditionalExp[ExpInfo]
    |   findExp[ExpInfo]
    |   midExp[ExpInfo]
    |   tokenExp[ExpInfo]
    |   spanExp[ExpInfo]
    ;

conditionalExp[ExpInfo]
locals[CObj=None]
    :   'cond'
        '('
        vfrStatementExpressionSub //
        '?'
        vfrStatementExpressionSub //
        ':'
        vfrStatementExpressionSub //
        ')'
    ;

findExp[ExpInfo]
locals[FObj=None]
    :   'find'
        '('
        findFormat[ExpInfo] ('|' findFormat[ExpInfo])*
        ','
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ')'
    ;

findFormat[ExpInfo]
locals[Format=0]
    :   'SENSITIVE' | 'INSENSITIVE'
    ;

midExp[ExpInfo]
locals[MObj=None]
    :   'mid'
        '('
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ')'
    ;

tokenExp[ExpInfo]
locals[TObj=None]
    :   'token'
        '('
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ')'
    ;

spanExp[ExpInfo]
locals[SObj=None]
    :   'span'
        '('
        'flags' '=' spanFlags ('|' spanFlags)*
        ','
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ','
        vfrStatementExpressionSub
        ')'
    ;

spanFlags
locals[Flag=0]
    :   Number
    |   'LAST_NON_MATCH'
    |   'FIRST_NON_MATCH'
    ;

vfrExpressionMap[ExpInfo]
locals[MObj=None]
    :   'map'
        '('
        vfrStatementExpressionSub
        ':'
        (   vfrStatementExpression
            ','
            vfrStatementExpression
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