grammar VfrSyntax;
options {
    language=Python;
}
@header{

from CommonCtypes import *
from VfrFormPkg import *
from VfrUtility import *
}

// VFR Program
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
    :   (TypeDef)? Struct NonNvDataMap? StringIdentifier? '{' vfrDataStructFields[False] '}'  StringIdentifier? ';'
    ;

vfrDataUnionDefinition
    :   (TypeDef)? Union NonNvDataMap? StringIdentifier? '{' vfrDataStructFields[True]'}' StringIdentifier? ';'
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
    :   'UINT64' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField32[FieldInUnion]
    :   'UINT32' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField16[FieldInUnion]
    :   'UINT16' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField8[FieldInUnion]
    :   'UINT8' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldBool[FieldInUnion]
    :   'BOOLEAN' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldString[FieldInUnion]
    :   'EFI_STRING_ID' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldDate[FieldInUnion]
    :   'EFI_HII_DATE' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldTime[FieldInUnion]
    :   'EFI_HII_TIME' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldRef[FieldInUnion]
    :   'EFI_HII_REF' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldUser[FieldInUnion]
    :   StringIdentifier StringIdentifier ('[' Number ']')? ';'
    ;

dataStructBitField64[FieldInUnion]
    :   'UINT64'  StringIdentifier? ':' Number ';'
    ;
dataStructBitField32[FieldInUnion]
    :   'UINT32' StringIdentifier? ':' Number ';'
    ;
dataStructBitField16[FieldInUnion]
    :   'UINT16' StringIdentifier? ':' Number ';'
    ;
dataStructBitField8[FieldInUnion]
    :   'UINT8' StringIdentifier? ':' Number ';'
    ;

// 2.4 VFR FormSet Definition
vfrFormSetDefinition
    :   'formset'
        'guid' '=' guidDefinition ','
        'title' '=' 'STRING_TOKEN' '(' Number ')' ','
        'help' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('classguid' '=' classguidDefinition ',')?
        ('class' '=' classDefinition ',')?
        ('subclass' '=' subclassDefinition ',')?
        vfrFormSetList
        'endformset' ';'
    ;

classguidDefinition
locals[GuidList=[]]
    :   guidDefinition ('|' guidDefinition)? ('|' guidDefinition)?('|' guidDefinition)?
    ;

classDefinition
locals[CObj=CIfrClass()]
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
locals[SubObj=CIfrSubClass()]
    :   SubclassSetupApplication
    |   SubclassGeneralApplication
    |   SubclassFrontPage
    |   SubclassSingleUse
    |   Number
    ;

//2.5 VFR FormSet List Definition
vfrFormSetList 
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
    |   vfrStatementExtension // 
    )*
    ;

//2.6 VFR Default Stores Definition
vfrStatementDefaultStore
locals[DSObj=CIfrDefaultStore()]
    :   'defaultstore' StringIdentifier ','
        'prompt' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'attribute' '=' Number)? ';'
    ;

//2.7 VFR Variable Store Definition
vfrStatementVarStoreLinear 
locals[VSObj=CIfrVarStore()]
    :   'varstore'
        (   StringIdentifier ','  
        |   'UINT8' ','           
        |   'UINT16' ','          
        |   'UINT32' ',' 
        |   'UINT64' ','
        |   'EFI_HII_DATE' ','
        |   'EFI_HII_TIME' ','
        |   'EFI_HII_REF' ','
        )
        ('varid' '=' Number ',')?
        'name' '=' StringIdentifier ','
        'guid' '=' guidDefinition ';'
    ;

vfrStatementVarStoreEfi
locals[VSEObj=CIfrVarStoreEfi()]
    :   'efivarstore'
        (   StringIdentifier ','
        |   'UINT8' ','
        |   'UINT16' ','
        |   'UINT32' ','
        |   'UINT64' ','
        |   'EFI_HII_DATE' ','
        |   'EFI_HII_TIME' ','
        |   'EFI_HII_REF' ','
        )
        ('varid' '=' Number ',')?
        'attribute' '=' vfrVarStoreEfiAttr ('|' vfrVarStoreEfiAttr)* ','
        (   'name' '=' StringIdentifier ','
        |   'name' '=' 'STRING_TOKEN' '(' Number ')' ','  'varsize' '=' Number ','
        )
        'guid' '=' guidDefinition ';'
    ;

vfrVarStoreEfiAttr
locals[Attr=0]
    :   Number;                               

vfrStatementVarStoreNameValue 
locals[VSNVObj=CIfrVarStoreNameValue()]
    :   'namevaluevarstore' StringIdentifier ','
        ('varid' '=' Number ',')?
        ('name' '=' 'STRING_TOKEN' '(' Number ')' ',')+
        'guid' '=' guidDefinition ';'
    ;

vfrStatementDisableIfFormSet
    :   'disableif' vfrStatementExpression ';'
        vfrFormSetList
        'endif' ';'
    ;

vfrStatementSuppressIfFormSet 
    :   'suppressif' vfrStatementExpression ';'
        vfrFormSetList
        'endif' ';'
    ;

//2.10.1 GUID Definition

guidSubDefinition[Guid]
    :   Number ',' Number ',' Number ',' Number ','
        Number ',' Number ',' Number ',' Number
    ;

guidDefinition
locals[Guid=EFI_GUID()]
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
    :   ('name' '=' StringIdentifier ',')?
        ('varid' '=' vfrStorageVarId[localctx.BaseInfo, localctx.CheckFlag] ',')?
        ('questionid' '=' Number ',')?
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
    |   OptionOnlyFlag
    |   NVAccessFlag
    |   LateCheckFlag
    ;
//2.10.5
vfrStorageVarId[BaseInfo, CheckFlag]
locals[VarIdStr='']
    :   (StringIdentifier '[' Number ']')    # vfrStorageVarIdRule1
    |   (StringIdentifier ('.' StringIdentifier ('[' Number ']')? )* ) # vfrStorageVarIdRule2
    ;

vfrConstantValueField 
locals[Value=EFI_IFR_TYPE_VALUE(),ListType=False]
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
locals[IObj=CIfrImage()]
    :   'image' '=' 'IMAGE_TOKEN' '(' Number ')'
    ;

vfrLockedTag
locals[LObj=CIfrLocked()]
    :   'locked'
    ;

vfrStatementStatTag 
    :   vfrImageTag | vfrLockedTag
    ;

vfrStatementStatTagList 
    :   vfrStatementStatTag (',' vfrStatementStatTag)*
    ;

vfrFormDefinition 
locals[FObj=CIfrForm()]
    :   'form' 'formid' '=' Number ','
        'title' '=' 'STRING_TOKEN' '(' Number ')' ';'
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
        )*
        'endform' ';' 
    ;

vfrFormMapDefinition
    :   'formmap' 'formid' '=' Number ','
        (   'maptitle' '=' getStringId ';'
            'mapguid' '=' guidDefinition ';'
        )*
        (   vfrStatementImage
        |   vfrStatementLocked
        |   vfrStatementRules
        |   vfrStatementDefault
        |   vfrStatementStat
        |   vfrStatementQuestions
        |   vfrStatementConditional
        |   vfrStatementLabel
        |   vfrStatementBanner
        |   vfrStatementExtension
        |   vfrStatementModal
        |   vfrStatementRefreshEvent ';'
        )*
        'endform' ';' 
    ;
        
vfrStatementImage
    :   vfrImageTag ';'
    ;

vfrStatementLocked
    :   vfrLockedTag ';'
    ;

vfrStatementRules 
locals[RObj=CIfrRule()]
    :   'rule' StringIdentifier ','
        vfrStatementExpression
        'endrule' ';'
    ;

vfrStatementStat 
    :   vfrStatementSubTitle
    |   vfrStatementStaticText
    |   vfrStatementCrossReference
    ;

vfrStatementSubTitle  //2.11.5.1
locals[OpObj=CIfrSubtitle()]
    :   'subtitle' 

        'text' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'flags' '=' vfrSubtitleFlags)?
        (   (',' vfrStatementStatTagList)? ';' 
        |   
            (',' vfrStatementStatTagList)?
            (',' (vfrStatementStat | vfrStatementQuestions)* )? 
            'endsubtitle' ';'
        )
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
    :   'text'
        'help' '=' 'STRING_TOKEN' '(' Number ')' ','
        'text' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'text' '=' 'STRING_TOKEN' '(' Number ')')?
        (',' 'flags' '=' staticTextFlagsField ('|' staticTextFlagsField)* ',' 'key' '=' Number)?
        (',' vfrStatementStatTagList)? ';'
    ;

staticTextFlagsField
locals[Flag=0, Line=0]
    :   Number | questionheaderFlagsField
    ;

vfrStatementCrossReference 
    :   vfrStatementGoto | vfrStatementResetButton 
    ;

vfrStatementGoto
locals[OpObj=None, OHObj=None, QType=EFI_QUESION_TYPE.QUESTION_REF]
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
                Question '=' (StringIdentifier ',' | Number ',')
            )
            |
            (   Number ',' )
        )?
        vfrQuestionHeader[localctx.OpObj, localctx.QType]
        (',' 'flags' '=' vfrGotoFlags[localctx.OpObj])?
        (',' 'key' '=' Number)?
        (',' vfrStatementQuestionOptionList)? ';'
    ;

vfrGotoFlags[Obj]
locals[GotoFlags=0]
    :   gotoFlagsField('|' gotoFlagsField)*
    ;

gotoFlagsField
locals[Flag=0]
    :  Number | questionheaderFlagsField
    ;

vfrStatementResetButton
locals[OpObj=CIfrResetButton()]
    :  'resetbutton'
       'defaultstore' '=' StringIdentifier ','
       vfrStatementHeader[localctx.OpObj] ','
       (vfrStatementStatTagList ',')?
       'endresetbutton' ';'
    ;

vfrStatementQuestions // doing 
    :   vfrStatementBooleanType
    |   vfrStatementDate
    |   vfrStatementNumericType
    |   vfrStatementStringType
    |   vfrStatementOrderedList
    |   vfrStatementTime
    ;

vfrStatementQuestionTag 
    :   vfrStatementStatTag ','
    |   vfrStatementInconsistentIf
    |   vfrStatementNoSubmitIf
    |   vfrStatementDisableIfQuest
    |   vfrStatementRefresh
    |   vfrStatementVarstoreDevice
    |   vfrStatementExtension //pending
    |   vfrStatementRefreshEvent
    |   vfrStatementWarningIf
    ;

vfrStatementInconsistentIf 
locals[IIObj=CIfrInconsistentIf()]
    :   'inconsistentif'
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression
        'endif' (';')?
    ;

vfrStatementNoSubmitIf
locals[NSIObj=CIfrNoSubmitIf()]
    :   'nosubmitif'
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression
        'endif' (';')?
    ;

vfrStatementDisableIfQuest 
locals[DIObj=CIfrDisableIf()]
    :   'disableif' vfrStatementExpression ';'
        vfrStatementQuestionOptionList // to do ???????????
        'endif' (';')?
    ;

vfrStatementRefresh
locals[RObj=CIfrRefresh()]
    :   'refresh' 'interval' '=' Number
    ;

vfrStatementVarstoreDevice 
locals[VDObj=CIfrVarStoreDevice()]
    :   'varstoredevice' '=' 'STRING_TOKEN' '(' Number ')' ','
    ;

vfrStatementRefreshEvent
locals[RiObj=CIfrRefreshId()]
    :   'refreshguid' '=' guidDefinition ','
    ;

vfrStatementWarningIf 
locals[WIObj=CIfrWarningIf()]
    :   'warningif' 
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('timeout' '=' Number ',')?
        vfrStatementExpression
        'endif' (';')?
    ;

vfrStatementQuestionTagList
    :   (vfrStatementQuestionTag)* 
    ;

vfrStatementQuestionOptionTag // doing 
    :   vfrStatementSuppressIfQuest
    |   vfrStatementValue
    |   vfrStatementDefault
    |   vfrStatementOptions
    |   vfrStatementRead
    |   vfrStatementWrite
    ;

vfrStatementSuppressIfQuest 
locals[SIObj=CIfrSuppressIf()]
    :   'suppressif' vfrStatementExpression ';'
        vfrStatementQuestionOptionList
        'endif' (';')?
    ;

flagsField 
    :   Number 
    |   InteractiveFlag 
    |   ManufacturingFlag 
    |   DefaultFlag 
    |   ResetRequiredFlag 
    |   ReconnectRequiredFlag
    |   NVAccessFlag                                     
    |   LateCheckFlag                          
    ;

vfrStatementDefault 
locals[DObj=None]
    :   'default'
        (   (   vfrStatementValue ','
            |   '=' vfrConstantValueField ','
            )
            (   'defaultstore' '=' StringIdentifier ','
            )?
        )
    ;

vfrStatementValue 
locals[VObj=CIfrValue(), LineNum=0]
    :   'value' '=' vfrStatementExpression 
    ;

vfrStatementOptions 
    :   vfrStatementOneOfOption
    ;

vfrStatementOneOfOption 
locals[OOOObj=None] //CIfrOneOfOption()
    :   'option'
        'text' '=' 'STRING_TOKEN' '(' Number ')' ','
        'value' '=' vfrConstantValueField ','
        'flags' '=' vfrOneOfOptionFlags (',' 'key' '=' Number) ? (',' vfrImageTag)* ';'
    ;

vfrOneOfOptionFlags
locals[HFlags=0, LFlags=0, LineNum=0]
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
    |   NVAccessFlag
    |   LateCheckFlag
    ;

vfrStatementRead
locals[RObj=CIfrRead()]
    :   'read' vfrStatementExpression ';'
    ;

vfrStatementWrite
locals[WObj=CIfrWrite()]
    :   'write' vfrStatementExpression ';'
    ;

vfrStatementQuestionOptionList //doing
    :   (vfrStatementQuestionTag | vfrStatementQuestionOptionTag)*
    ;

vfrStatementBooleanType
    :   vfrStatementCheckBox | vfrStatementAction
    ;

vfrStatementCheckBox 
locals[OpObj=CIfrCheckBox(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'checkbox'
        vfrQuestionBaseInfo[localctx.OpObj, localctx.QType] 
        vfrStatementHeader[localctx.OpObj] ','
        ('flags' '=' vfrCheckBoxFlags ',')?
        ('key' '=' Number ',')?
        vfrStatementQuestionOptionList
        'endcheckbox' ';'
    ;

vfrCheckBoxFlags 
locals[LFlags=0, HFlags=0]
    :   checkboxFlagsField ('|' checkboxFlagsField)*
    ;

checkboxFlagsField
locals[LFlag=0, HFlag=0]
    :   Number
    |   'DEFAULT'
    |   'MANUFACTURING'
    |   'CHECKBOX_DEFAULT'
    |   'CHECKBOX_DEFAULT_MFG'
    |   questionheaderFlagsField
    ;

vfrStatementAction
locals[OpObj=CIfrAction(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'action'
        vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
        ('flags' '=' vfrActionFlags ',')?
        'config' '=' 'STRING_TOKEN' '(' Number ')' ','
        vfrStatementQuestionTagList
        'endaction' ';'
    ;

vfrActionFlags
locals[HFlags=0, LineNum=0]
    :   actionFlagsField ('|' actionFlagsField)*
    ;
actionFlagsField
locals[HFlag=0]
    :   Number | questionheaderFlagsField
    ;

vfrStatementNumericType 
    :   vfrStatementNumeric | vfrStatementOneOf
    ;

vfrStatementNumeric  
locals[OpObj=CIfrNumeric(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL] // BaseInfo=EFI_VARSTORE_INFO() not needed
    :   'numeric'
        vfrQuestionBaseInfo[localctx.OpObj, localctx.QType]  
        vfrStatementHeader[localctx.OpObj] ','
        ('flags' '=' vfrNumericFlags ',')?
        ('key' '=' Number ',')?
        vfrSetMinMaxStep[localctx.OpObj]
        vfrStatementQuestionOptionList
        'endnumeric' ';'
    ;

vfrSetMinMaxStep[OpObj] // CIfrMinMaxStepData
    :   'minimum' '=' ('-')? Number ','
        'maximum' '=' ('-')? Number ','
        ('step' '=' Number ',')?
    ;

vfrNumericFlags
locals[HFlags=0, LFlags=0,IsDisplaySpecified=False,LineNum=0]
    :   numericFlagsField ('|' numericFlagsField)*
    ;

numericFlagsField
locals[HFlag=0,IsSetType=False,IsDisplaySpecified=False]
    :   Number
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
locals[OpObj=CIfrOneOf(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'oneof'
        vfrQuestionBaseInfo[localctx.OpObj, localctx.QType] 
        vfrStatementHeader[localctx.OpObj] ','
        ('flags' '=' vfrOneofFlagsField ',')?
        (vfrSetMinMaxStep[localctx.OpObj])?
        vfrStatementQuestionOptionList
        'endoneof' ';'
    ;

vfrOneofFlagsField
locals[HFlags=0, LFlags=0, LineNum=0]
    :   numericFlagsField ('|' numericFlagsField)*
    ;

vfrStatementStringType 
    :   vfrStatementString | vfrStatementPassword
    ;

vfrStatementString
locals[OpObj=CIfrString(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'string'
        vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
        ('flags' '=' vfrStringFlagsField ',')?
        ('key' '=' Number ',')?
        'minsize' '=' Number ','
        'maxsize' '=' Number ','
        vfrStatementQuestionOptionList
        'endstring' ';'
    ;

vfrStringFlagsField 
locals[HFlags=0, LFlags=0, LineNum=0]
    :   stringFlagsField ('|' stringFlagsField)*
    ;

stringFlagsField
locals[HFlag=0, LFlag=0]
    :   Number
    |   'MULTI_LINE'
    |   questionheaderFlagsField
    ;

vfrStatementPassword 
locals[OpObj=CIfrPassword(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'password'
        vfrQuestionHeader[localctx.OpObj, localctx.QType]','
        ('flags' '=' vfrPasswordFlagsField ',')?
        ('key' '=' Number ',')?
        'minsize' '=' Number ','
        'maxsize' '=' Number ','
        vfrStatementQuestionOptionList
        'endpassword' ';'
    ;

vfrPasswordFlagsField 
locals[HFlags=0, LineNum=0]
    :   passwordFlagsField('|' passwordFlagsField)*
    ;

passwordFlagsField
locals[HFlag=0]
    :   Number
    |   questionheaderFlagsField
    ;

vfrStatementOrderedList 
locals[OpObj=CIfrOrderedList(), QType=EFI_QUESION_TYPE.QUESTION_NORMAL]
    :   'orderedlist'
        vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
        ('maxcontainers' '=' Number ',')?
        ('flags' '=' vfrOrderedListFlags ',')?
        vfrStatementQuestionOptionList
        'endlist' ';'
    ;

vfrOrderedListFlags 
locals[HFlags=0, LFlags=0, LineNum=0]
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
locals[OpObj=CIfrDate(), QType=EFI_QUESION_TYPE.QUESTION_DATE, Val=EFI_IFR_TYPE_VALUE()]
    :   'date'
        (   (   vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
                ('flags' '=' vfrDateFlags ',')?
                vfrStatementQuestionOptionList
            )
            |
            (   'year' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxDateStepDefault[Val.Date, 0]
                'month' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxDateStepDefault[Val.Date, 1]
                'day' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxDateStepDefault[Val.Date, 2]
                ('flags' '=' vfrDateFlags ',')?
                (vfrStatementInconsistentIf)*
            )
        )
        'enddate' ';'
    ;

minMaxDateStepDefault[Date, KeyValue]
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
        ('default' '=' Number ',')?
    ;

vfrDateFlags 
locals[LFlags=0, LineNum=0]
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
locals[OpObj=CIfrTime(), QType=EFI_QUESION_TYPE.QUESTION_TIME,  Val=EFI_IFR_TYPE_VALUE()]
    :   'time'
        (   (   vfrQuestionHeader[localctx.OpObj, localctx.QType] ','
                ('flags' '=' vfrTimeFlags ',')?
                vfrStatementQuestionOptionList
            )
            |
            (
                'hour' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxTimeStepDefault[Val.Time, 0]
                'minute' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxTimeStepDefault[Val.Time, 1]
                'second' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
                'help' '=' 'STRING_TOKEN' '(' Number ')' ','
                minMaxTimeStepDefault[Val.Time, 2]
                ('flags' '=' vfrTimeFlags ',')?
                (vfrStatementInconsistentIf)*
            )
        )
        'endtime' ';'
    ;

minMaxTimeStepDefault[Time, KeyValue]
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
        ('default' '=' Number ',')?
    ;

vfrTimeFlags 
locals[LFlags=0, LineNum=0]
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
    :   vfrStatementDisableIfStat
    |   vfrStatementSuppressIfStat //enhance to be compatible for framework endif
    |   vfrStatementGrayOutIfStat
    |   vfrStatementInconsistentIfStat   //to be compatible for framework
    ;

// new seems to be the same as the old, why new?
vfrStatementConditionalNew 
    :   vfrStatementDisableIfStat      
        vfrStatementSuppressIfStatNew  
        vfrStatementGrayOutIfStatNew   
        vfrStatementInconsistentIfStat   //to be compatible for framework
    ;

vfrStatementSuppressIfStat
locals[SIObj=CIfrSuppressIf()]
    :   vfrStatementSuppressIfStatNew
    ;

vfrStatementGrayOutIfStat 
locals[GOIObj=CIfrGrayOutIf()]
    :   vfrStatementGrayOutIfStatNew
    ;

vfrStatementStatList 
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
locals[DIObj=CIfrDisableIf()]
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
locals[SIObj=CIfrSuppressIf()]
    :   'suppressif' 
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementGrayOutIfStatNew
locals[GOIObj=CIfrGrayOutIf()]
    :   'grayoutif'
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementInconsistentIfStat 
locals[IIObj=CIfrInconsistentIf()]
    :   'inconsistentif'     
        'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        ('flags' '=' flagsField ('|' flagsField)* ',')?
        vfrStatementExpression
        'endif' ';'
    ;

vfrStatementInvalid // for compatibility
    :   vfrStatementInvalidHidden              |   vfrStatementInvalidInventory       
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
locals[LObj=CIfrLabel()]
    :   'label' Number ';'
    ;

vfrStatementBanner 
locals[BObj=CIfrBanner(), TObj=CIfrTimeout()]
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
    :   'guidop'
        'guid' '=' guidDefinition
        (   ',' 'datatype' '='
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
            vfrExtensionData
        )?
        (
            ',' (vfrStatementExtension)*
            'endguidop'
        )? 
        ';'
    ;

vfrExtensionData
    :   (vfrExtensionDataComponent)* 
    ;

vfrExtensionDataComponent
    :   ',' 'data' ('[' Number ']')? 
        ( '.' StringIdentifier ('[' Number ']')? )*  '=' Number  
    ;

vfrStatementModal 
    :   'modal' ';' 
    ;

vfrStatementExpression // to do
locals[ExpInfo=ExpressionInfo()]
    :   andTerm[localctx.ExpInfo] (vfrStatementExpressionSupplementary[localctx.ExpInfo])*
    ; 
    
vfrStatementExpressionSupplementary[ExpInfo]
locals[Line=0]
    :   'OR' andTerm[ExpInfo]
    ;

vfrStatementExpressionSub // pendingh how to do the rootlevel
locals[ExpInfo=ExpressionInfo()]
    :   andTerm[localctx.ExpInfo] (vfrStatementExpressionSubSupplementary[localctx.ExpInfo])*
    ;

vfrStatementExpressionSubSupplementary[ExpInfo]
locals[Line=0]
    :   'OR' andTerm[ExpInfo]
    ;

andTerm[ExpInfo]
locals[CIfrAndList=[]]
    :   bitwiseorTerm[ExpInfo] (andTermSupplementary[ExpInfo])*
    ;

andTermSupplementary[ExpInfo]
locals[Line=0]
    :   'AND' bitwiseorTerm[ExpInfo]
    ;

bitwiseorTerm[ExpInfo]
locals[CIfrBitWiseOrList=[]]
    :   bitwiseandTerm[ExpInfo] (bitwiseorTermSupplementary[ExpInfo])*
    ;

bitwiseorTermSupplementary[ExpInfo]
locals[Line=0]
    :   '|' bitwiseandTerm[ExpInfo]
    ;


bitwiseandTerm[ExpInfo]
locals[CIfrBitWiseAndList=[]]
    :   equalTerm[ExpInfo] (bitwiseandTermSupplementary[ExpInfo])*
    ;

bitwiseandTermSupplementary[ExpInfo]
locals[Line=0]
    :   '&' equalTerm[ExpInfo]
    ;


equalTerm[ExpInfo]
locals[CIfrEqualList=[], CIfrNotEqualList=[]]
    :   compareTerm[localctx.ExpInfo] 
        (equalTermSupplementary[localctx.CIfrEqualList, localctx.CIfrNotEqualList, ExpInfo])*
    ;

equalTermSupplementary[CIfrEqualList, CIfrNotEqualList, ExpInfo]
locals[Line=0]
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
locals[Line=0]
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
locals[Line=0]
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
locals[Line=0]
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
locals[Line=0]
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
        '(' vfrStatementExpressionSub',' ////////vfrStatementExpressionPattern
        vfrStatementExpressionSub ','////////vfrStatementExpressionString
        guidDefinition ')'/////////////////////////////
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
    :
    ;

ideqvalExp[ExpInfo]
    :   'ideqval' vfrQuestionDataFieldName '==' Number 
    ;

ideqidExp[ExpInfo]
    :   'ideqid' vfrQuestionDataFieldName '==' vfrQuestionDataFieldName 
    ;

ideqvallistExp[ExpInfo]
    :   'ideqvallist' vfrQuestionDataFieldName '==' (Number)+ 
    ;

vfrQuestionDataFieldName  //////
    :   StringIdentifier '[' Number ']'
    |   StringIdentifier ('.' StringIdentifier('[' Number ']')? )*
    ;

questionref1Exp[ExpInfo]
    :   'questionref' 
        '(' ( StringIdentifier | Number ) ')'
    ;

rulerefExp[ExpInfo]
    :   'ruleref' '(' StringIdentifier ')'
    ;

stringref1Exp[ExpInfo]
    :   'stringref' '(' getStringId ')'
    ;

pushthisExp[ExpInfo]
    :   'pushthis'
    ;

securityExp[ExpInfo]
    :   'security' '(' guidDefinition ')'
    ;

getExp[ExpInfo] 
locals[BaseInfo=EFI_VARSTORE_INFO()]
    :   'get' '(' vfrStorageVarId[localctx.BaseInfo, False]('|' 'flags' '=' vfrNumericFlags)? ')'
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
    :   'length' '(' vfrStatementExpression ')'
    ;

bitwisenotExp[ExpInfo] 
    :   '~' '(' vfrStatementExpression ')'
    ;

question23refExp[ExpInfo] 
    :   'questionrefval'
        '('
        (DevicePath '=' 'STRING_TOKEN' '(' Number ')' ',' )? //
        (Uuid '=' guidDefinition ',' )? ///
        vfrStatementExpressionSub
        ')'
    ;

stringref2Exp[ExpInfo] 
    :   'stringrefval' '(' vfrStatementExpressionSub ')'
    ;

toboolExp[ExpInfo] 
    :   'boolval' '(' vfrStatementExpressionSub ')'
    ;

tostringExp[ExpInfo] 
    :   'stringval' ('format' '=' Number ',' )?
        '(' vfrStatementExpressionSub ')'
    ;

unintExp[ExpInfo] 
    :   'unintval' '(' vfrStatementExpressionSub ')'
    ;

toupperExp[ExpInfo]
    :   'toupper' '(' vfrStatementExpressionSub ')'
    ;

tolwerExp[ExpInfo] 
    :   'tolower' '(' vfrStatementExpressionSub ')'
    ;

setExp[ExpInfo] 
locals[BaseInfo=EFI_VARSTORE_INFO()]
    :   'set'
        '('
        vfrStorageVarId[localctx.BaseInfo, False]('|' 'flags' '=' vfrNumericFlags)? ','
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
    :   'SENSITIVE' | 'INSENSITIVE'
    ;

midExp[ExpInfo] 
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
    :   Number
    |   'LAST_NON_MATCH'
    |   'FIRST_NON_MATCH'
    ;

vfrExpressionMap[ExpInfo] 
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
NotQqual : '!=';
/* 
LineDefinition                           '#line\ [0-9]+\ \'~[\']+\'[\ \t]*\n' << gCVfrErrorHandle.ParseFileScopeRecord (begexpr (), line ()); skip (); newline (); >>
*/
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