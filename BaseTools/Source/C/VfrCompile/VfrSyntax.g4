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
    :   (TypeDef)? Struct NonNvDataMap? StringIdentifier? '{' vfrDataStructFields[True] '}'  StringIdentifier? ';'
    ;

vfrDataUnionDefinition
    :   (TypeDef)? Union NonNvDataMap? StringIdentifier? '{' vfrDataStructFields[False]'}' StringIdentifier? ';'
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
    :   guidDefinition ('|' guidDefinition)? ('|' guidDefinition)?('|' guidDefinition)?
    ;

classDefinition 
    :   validClassNames ('|' validClassNames)*
    ;

validClassNames 
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
    |   vfrStatementDisableIfFormSet
    |   vfrStatementSuppressIfFormSet
    |   vfrStatementExtension
    )*
    ;

//2.6 VFR Default Stores Definition
vfrStatementDefaultStore
    :   'defaultstore' StringIdentifier ','
        'prompt' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'attribute' '=' Number)? ';'
    ;

//2.7 VFR Variable Store Definition
vfrStatementVarStoreLinear 
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
        'attribute' '=' Number ('|' Number)* ','
        (   'name' '=' StringIdentifier ','
        |   'name' '=' 'STRING_TOKEN' '(' Number ')' ','  'varsize' '=' Number ','
        )
        'guid' '=' guidDefinition ';'
    ;


vfrStatementVarStoreNameValue 
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

guidSubDefinition
    :   Number ',' Number ',' Number ',' Number ','
        Number ',' Number ',' Number ',' Number
    ;

guidDefinition
    :   '{'
        Number ',' Number ',' Number ','
        (   '{' guidSubDefinition '}' 
        |   guidSubDefinition
        )
        '}'
    ;

getStringId 
    :   'STRING_TOKEN' '(' Number ')'
    ;

vfrStatementHeader[QHObj]
    :   'prompt' '=' 'STRING_TOKEN' '(' Number ')' ','
        'help' '=' 'STRING_TOKEN' '(' Number ')' 
    ;

vfrQuestionHeader[QHObj, QType=None]
    :   vfrQuestionBaseInfo[QHObj, QType]
        vfrStatementHeader[QHObj]
    ;

vfrQuestionBaseInfo[QHObj, QType=None]
locals[BaseInfo=EFI_VARSTORE_INFO(), VarIdStr='', CheckFlag=True]
    :   ('name' '=' StringIdentifier ',')?
        ('varid' '=' vfrStorageVarId[localctx.BaseInfo,localctx.VarIdStr, localctx.CheckFlag] ',')?
        ('questionid' '=' Number ',')?
    ;

questionheaderFlagsField
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
vfrStorageVarId[BaseInfo, VarIdStr, CheckFlag]
    :   (StringIdentifier '[' Number ']')    # vfrStorageVarIdRule1
    |   (StringIdentifier ('.' StringIdentifier ('[' Number ']')? )* ) # vfrStorageVarIdRule2
    ;

vfrConstantValueField 
    :   Number
    |   'TRUE'
    |   'FALSE'
    |   'ONE'
    |   'ONES'
    |   'ZERO'
    |   Number ':' Number ':' Number
    |   Number '/' Number '/' Number
    |   'STRING_TOKEN' '(' Number ')'
    |   (Number (',' Number)* )?
    ;

vfrImageTag 
    :   'image' '=' 'IMAGE_TOKEN' '(' Number ')'
    ;

vfrLockedTag
    :   'locked'
    ;

vfrStatementStatTag 
    :   vfrImageTag | vfrLockedTag
    ;

vfrStatementStatTagList 
    :   vfrStatementStatTag (',' vfrStatementStatTag)*
    ;

vfrFormDefinition 
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
        |   vfrStatementExtension
        |   vfrStatementModal
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
    :   'subtitle' 

        'text' '=' 'STRING_TOKEN' '(' Number ')'
        (',' 'flags' '=' vfrSubtitleFlags)?
        (   (',' vfrStatementStatTagList)? ';' 
        |   (',' vfrStatementStatTagList)?
            (',' (vfrStatementStat | vfrStatementQuestions)* )? 
            'endsubtitle' ';'
        )
    ;

vfrSubtitleFlags 
    :   subtitleFlagsField ('|' subtitleFlagsField)* ';'
    ;
subtitleFlagsField 
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
    :   Number | questionheaderFlagsField
    ;

vfrStatementCrossReference 
    :   vfrStatementGoto | vfrStatementResetButton 
    ;

vfrStatementGoto
locals[QHObj=None, OHObj=None, Guid=None, QType=EFI_QUESION_TYPE.QUESTION_REF]
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
        vfrQuestionHeader[localctx.QHObj, localctx.QType]
        (',' 'flags' '=' vfrGotoFlags[localctx.QHObj])?
        (',' 'key' '=' Number)?
        (',' vfrStatementQuestionOptionList)? ';'
    ;

vfrGotoFlags[Obj]
locals[HFlags=0]
    :   gotoFlagsField[localctx.HFlags]('|' gotoFlagsField[localctx.HFlags])*
    ;

gotoFlagsField[HFlags]
    :  Number | questionheaderFlagsField
    ;

vfrStatementResetButton
locals[RBObj]
    :  'resetbutton'
       'defaultStore' '=' StringIdentifier ','
       vfrStatementHeader[localctx.RBObj] ','
       (vfrStatementStatTagList ',')?
       'endresetbutton' ';'
    ;

vfrStatementQuestions 
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
    |   vfrStatementExtension
    |   vfrStatementRefreshEvent
    |   vfrStatementWarningIf
    ;

vfrStatementInconsistentIf 
    :   'inconsistentif'
        'prompt' '=' getStringId ','
        vfrStatementExpression
        'endif'
    ;

vfrStatementNoSubmitIf
    :   'nosubmitif'
        'prompt' '=' getStringId ','
        vfrStatementExpression
        'endif'
    ;

vfrStatementDisableIfQuest 
    :   'disableif' vfrStatementExpression ';'
        vfrStatementQuestionOptionList
        'endif'
    ;

vfrStatementRefresh
    :   'refresh' 'interval' '=' Number
    ;

vfrStatementVarstoreDevice 
    :   'varstoredevice' '=' getStringId ','
    ;

vfrStatementRefreshEvent
    :   'refreshguid' '=' guidDefinition ','
    ;

vfrStatementWarningIf 
    :   'warningif' 'prompt' '=' getStringId ',' ('timeout' '=' Number ',')?
        vfrStatementExpression
        'endif'
    ;

vfrStatementQuestionTagList
    :   (vfrStatementQuestionTag)* 
    ;

vfrStatementQuestionOptionTag 
    :   vfrStatementSuppressIfQuest
    |   vfrStatementValue
    |   vfrStatementDefault
    |   vfrStatementOptions
    |   vfrStatementRead
    |   vfrStatementWrite
    ;

vfrStatementSuppressIfQuest 
    :   'suppressif' vfrStatementExpression ';'
        vfrStatementQuestionOptionList
        'endif'
    ;

vfrStatementDefault 
    :   'default'
        (   (   vfrStatementValue ','
            |   '=' vfrConstantValueField ','
            )
            (   'defaultstore' '=' StringIdentifier ','
            )?
        )
    ;

vfrStatementValue 
    :   'value' '=' vfrStatementExpression ';'
    ;

vfrStatementOptions 
    :   vfrStatementOneOfOption
    ;

vfrStatementOneOfOption 
    :   'option'
        'text' '=' getStringId ','
        'value' '=' vfrConstantValueField ','
        'flags' '=' vfrOneOfOptionFlags(',' vfrImageTag)* ';'
    ;

vfrOneOfOptionFlags 
    :   oneofoptionFlagsField ('|' oneofoptionFlagsField)*
    ;

oneofoptionFlagsField 
    :   Number
    |   'OPTION_DEFAULT'
    |   'OPTION_DEFAULT_MFG'
    |   'INTERACTIVE'
    |   'RESET_REQUIRED'
    |   'DEFAULT'
    ;

vfrStatementRead
    :   'read' vfrStatementExpression ';'
    ;

vfrStatementWrite
    :   'write' vfrStatementExpression ';'
    ;

vfrStatementQuestionOptionList 
    :   (vfrStatementQuestionTag | vfrStatementQuestionOptionTag)*
    ;

vfrStatementBooleanType
    :   vfrStatementCheckBox | vfrStatementAction
    ;

vfrStatementCheckBox 
locals[QHObj=None, BaseInfo=None, QId=None]
    :   'checkbox'
        vfrQuestionBaseInfo[localctx.QHObj, None] 
        vfrStatementHeader[localctx.QHObj] ','
        ('flags' '=' vfrCheckBoxFlags ',')?
        ('key' '=' Number ',')?
        vfrStatementQuestionOptionList
        'endcheckbox' ';'
    ;

vfrCheckBoxFlags 
locals[HFlags=0]
    :   checkboxFlagsField[localctx.HFlags] ('|' checkboxFlagsField[localctx.HFlags])*
    ;

checkboxFlagsField[HFlags]
    :   Number
    |   'CHECKBOX_DEFAULT'
    |   'CHECKBOX_DEFAULT_MFG'
    |   questionheaderFlagsField
    ;

vfrStatementAction
locals[QHObj=None]
    :   'action'
        vfrQuestionHeader[localctx.QHObj] ','
        ('flags' '=' vfrActionFlags ',')?
        'config' '=' getStringId ','
        vfrStatementQuestionTagList
        'endaction' ';'
    ;

vfrActionFlags
locals[HFlags=0]
    :   actionFlagsField[localctx.HFlags] ('|' actionFlagsField[localctx.HFlags])*
    ;
actionFlagsField[HFlags]
    :   Number | questionheaderFlagsField
    ;

vfrStatementNumericType 
    :   vfrStatementNumeric | vfrStatementOneOf
    ;

vfrStatementNumeric 
locals[QHObj=None, BaseInfo=None, QId=None]
    :   'numeric'
        vfrQuestionBaseInfo[localctx.QHObj, None]  
        vfrStatementHeader[localctx.QHObj] ','
        ('flags' '=' vfrNumericFlags ',')?
        ('key' '=' Number ',')?
        vfrSetMinMaxStep
        vfrStatementQuestionOptionList
        'endnumeric' ';'
    ;

vfrSetMinMaxStep 
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
    ;

vfrNumericFlags 
    :   numericFlagsField ('|' numericFlagsField)*
    ;

numericFlagsField
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
locals[QHObj=None, BaseInfo=None, QId=None]
    :   'oneof'
        vfrQuestionBaseInfo[localctx.QHObj, None] 
        vfrStatementHeader[localctx.QHObj] ','
        ('flags' '=' vfrOneofFlagsField ',')?
        (vfrSetMinMaxStep)?
        vfrStatementQuestionOptionList
        'endoneof' ';'
    ;

vfrOneofFlagsField
    :   numericFlagsField ('|' numericFlagsField)*
    ;

vfrStatementStringType 
    :   vfrStatementString | vfrStatementPassword
    ;

vfrStatementString
locals[QHObj=None,QType=None]
    :   'string'
        vfrQuestionHeader[localctx.QHObj, localctx.QType] ','
        ('flags' '=' vfrStringFlagsField ',')?
        ('key' '=' Number ',')?
        'minsize' '=' Number ','
        'maxsize' '=' Number ','
        vfrStatementQuestionOptionList
        'endstring' ';'
    ;

vfrStringFlagsField 
locals[HFlags=0]
    :   stringFlagsField[localctx.HFlags] ('|' stringFlagsField[localctx.HFlags])*
    ;

stringFlagsField[HFlags]
    :   Number
    |   'MULTI_LINE'
    |   questionheaderFlagsField
    ;

vfrStatementPassword 
locals[QHObj=None]
    :   'password'
        vfrQuestionHeader[localctx.QHObj] ','
        ('flags' '=' vfrPasswordFlagsField ',')?
        ('key' '=' Number ',')?
        'minsize' '=' Number ','
        'maxsize' '=' Number ','
        vfrStatementQuestionOptionList
        'endpassword' ';'
    ;

vfrPasswordFlagsField 
locals[HFlags=0]
    :   passwordFlagsField[localctx.HFlags]('|' passwordFlagsField[localctx.HFlags])*
    ;

passwordFlagsField[HFlags]
    :   Number
    |   questionheaderFlagsField
    ;

vfrStatementOrderedList 
locals[QHObj]
    :   'orderedlist'
        vfrQuestionHeader[localctx.QHObj] ','
        ('maxcontainers' '=' Number ',')?
        ('flags' '=' vfrOrderedListFlags)?
        vfrStatementQuestionOptionList
        'endlist' ';'
    ;

vfrOrderedListFlags 
locals[HFlags=0]
    :   orderedlistFlagsField[localctx.HFlags] ('|' orderedlistFlagsField[localctx.HFlags])*
    ;

orderedlistFlagsField[HFlags]
    :   Number
    |   'UNIQUE'
    |   'NOEMPTY'
    |   questionheaderFlagsField
    ;

vfrStatementDate 
locals[QHObj=None]
    :   'date'
        (   (   vfrQuestionHeader[localctx.QHObj] ','
                ('flags' '=' vfrDateFlags ',')?
                vfrStatementQuestionOptionList
            )
            |
            (   'year' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' getStringId ','
                'help' '=' getStringId ','
                minMaxDateStepDefault
                'month' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' getStringId ','
                'help' '=' getStringId ','
                minMaxDateStepDefault
                'day' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' getStringId ','
                'help' '=' getStringId ','
                minMaxDateStepDefault
                (vfrStatementInconsistentIf)*
            )
        )
        'enddate' ';'
    ;

minMaxDateStepDefault 
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
        ('default' '=' Number ',')?
    ;

vfrDateFlags 
    :   dateFlagsField ('|' dateFlagsField)*
    ;
dateFlagsField 
    :   Number
    |   'YEAR_SUPPRESS'
    |   'MONTH_SUPPRESS'
    |   'DAY_SUPPRESS'
    |   'STORAGE_NORMAL'
    |   'STORAGE_TIME'
    |   'STORAGE_WAKEUP'
    ;

vfrStatementTime 
locals[QHObj=None, QType=None]
    :   'time'
        (   (   vfrQuestionHeader[localctx.QHObj, localctx.QType] ','
                ('flags' '=' vfrTimeFlags ',')?
                vfrStatementQuestionOptionList
            )
            |
            (
                'hour' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' getStringId ','
                'help' '=' getStringId ','
                minMaxTimeStepDefault
                'minute' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' getStringId ','
                'help' '=' getStringId ','
                minMaxTimeStepDefault
                'second' 'varid' '=' StringIdentifier '.' StringIdentifier ','
                'prompt' '=' getStringId ','
                'help' '=' getStringId ','
                minMaxTimeStepDefault
                (vfrStatementInconsistentIf)*
            )
        )
        'endtime' ';'
    ;

minMaxTimeStepDefault 
    :   'minimum' '=' Number ','
        'maximum' '=' Number ','
        ('step' '=' Number ',')?
        ('default' '=' Number ',')?
    ;

vfrTimeFlags 
    :   timeFlagsField ('|' timeFlagsField)*
    ;

timeFlagsField 
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
    |   vfrStatementSuppressIfStat
    |   vfrStatementGrayOutIfStat
    ;

vfrStatementStatList 
    :   vfrStatementStat
    |   vfrStatementQuestions
    |   vfrStatementConditional
    |   vfrStatementLabel
    |   vfrStatementExtension
    ;

vfrStatementDisableIfStat 
    :   'disableif' vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementSuppressIfStat 
    :   'suppressif' vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementGrayOutIfStat 
    :   'grayoutif' vfrStatementExpression ';'
        (vfrStatementStatList)*
        'endif' ';'
    ;

vfrStatementLabel 
    :   'label' Number ';'
    ;

vfrStatementBanner 
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
        (vfrExtensionDataDotArea)*  '=' Number  
    ;

vfrExtensionDataDotArea
    :   '.' StringIdentifier ('[' Number ']')?
    ;

vfrStatementModal 
    :   'modal' ';' 
    ;

vfrStatementExpression
    :   andTerm ( 'OR' andTerm)*
    ;

andTerm
    :   bitwiseorTerm ( 'AND' bitwiseorTerm)*
    ;

bitwiseorTerm
    :   bitwiseandTerm ('|' bitwiseandTerm)*
    ;

bitwiseandTerm
    :   equalTerm ('&' equalTerm)*
    ;

equalTerm 
    :   compareTerm ('==' compareTerm | '!=' compareTerm)*
    ;

compareTerm
    :   shiftTerm
        (   '<' shiftTerm
        |   '<=' shiftTerm
        |   '>' shiftTerm
        |   '>=' shiftTerm
        )*
    ;

shiftTerm 
    :   addMinusTerm
        (   '<<' addMinusTerm
        |   '>>' addMinusTerm
        )*
    ;

addMinusTerm 
    :   multdivmodTerm
        (   '+' multdivmodTerm
        |   '-' multdivmodTerm
        )*
    ;

multdivmodTerm 
    :   castTerm
        (   '*' castTerm
        |   '/' castTerm
        |   '%' castTerm
        )*
    ;

castTerm 
    :   (   '('
            (  'BOOLEAN'
            |  'UINT64'
            |  'UINT32'
            |  'UINT16'
            |  'UINT8'
            )
            ')'
        )*
        atomTerm
    ;

atomTerm
    :   vfrExpressionCatenate
    |   vfrExpressionMatch
    |   vfrExpressionParen
    |   vfrExpressionBuildInFunction
    |   vfrExpressionConstant
    |   vfrExpressionUnaryOp
    |   vfrExpressionTernaryOp
    |   vfrExpressionMap
    |   ('NOT' atomTerm)
    |   vfrExpressionMatch2
    ;

vfrExpressionCatenate
    :   'catenate'
        '(' vfrStatementExpression ',' vfrStatementExpression ')'
    ;

vfrExpressionMatch
    :   'match'
        '(' vfrStatementExpression ',' vfrStatementExpression ')'
    ;

vfrExpressionParen
    :   '(' vfrStatementExpression ')'
    ;

vfrExpressionBuildInFunction
    :   dupExp
    |   ideqvalExp
    |   ideqidExp
    |   ideqvallistExp
    |   questionref1Exp
    |   rulerefExp
    |   stringref1Exp
    |   pushthisExp
    |   securityExp
    |   getExp
    ;

dupExp
    :   'dup'
    ;

ideqvalExp 
    :   'ideqval' vfrQuestionDataFieldName '==' Number 
    ;

ideqidExp 
    :   'ideqid' vfrQuestionDataFieldName '==' vfrQuestionDataFieldName 
    ;

ideqvallistExp 
    :   'ideqvallist' vfrQuestionDataFieldName '==' (Number)+ 
    ;

vfrQuestionDataFieldName  //////
    :   StringIdentifier '[' Number ']'
    |   StringIdentifier ('.' StringIdentifier('[' Number ']')? )*
    ;

questionref1Exp 
    :   'questionref' '(' StringIdentifier | Number ')'
    ;

rulerefExp 
    :   'ruleref' '(' StringIdentifier ')'
    ;

stringref1Exp 
    :   'stringref' '(' getStringId ')'
    ;

pushthisExp
    :   'pushthis'
    ;

securityExp 
    :   'security' '(' guidDefinition ')'
    ;

getExp 
locals[BaseInfo=None, VarIdStr=None]
    :   'get' '(' vfrStorageVarId[localctx.BaseInfo, localctx.VarIdStr, False]('|' 'flags' '=' vfrNumericFlags)? ')'
    ;

vfrExpressionConstant
    :   'TRUE'
    |   'FALSE'
    |   'ONE'
    |   'ONES'
    |   'ZERO'
    |   'UNDEFINED'
    |   'VERSION'
    |   Number
    ;

vfrExpressionUnaryOp 
    :   lengthExp
    |   bitwisenotExp
    |   question23refExp
    |   stringref2Exp
    |   toboolExp
    |   tostringExp
    |   unintExp
    |   toupperExp
    |   tolwerExp
    |   setExp
    ;

lengthExp 
    :   'length' '(' vfrStatementExpression ')'
    ;

bitwisenotExp 
    :   '~' '(' vfrStatementExpression ')'
    ;

question23refExp 
    :   'questionrefval'
        '('
        (DevicePath '=' 'STRING_TOKEN' '(' Number ')' ',' )? //
        (Uuid '=' guidDefinition ',' )? ///
        ')'
        vfrStatementExpression
    ;

stringref2Exp
    :   'stringrefval' '(' vfrStatementExpression ')'
    ;

toboolExp
    :   'boolval' '(' vfrStatementExpression ')'
    ;

tostringExp
    :   'stringval' ('format' '=' Number ',' )?
        '(' vfrStatementExpression ')'
    ;

unintExp
    :   'unintval' '(' vfrStatementExpression ')'
    ;

toupperExp
    :   'toupper' '(' vfrStatementExpression ')'
    ;

tolwerExp
    :   'tolower' '(' vfrStatementExpression ')'
    ;

setExp
locals[BaseInfo=None, VarIdStr=None]
    :   'set'
        '('
        vfrStorageVarId[localctx.BaseInfo, localctx.VarIdStr, False]('|' 'flags' '=' vfrNumericFlags)? ','
        vfrStatementExpression
        ')'
    ;

vfrExpressionTernaryOp 
    :   conditionalExp
    |   findExp
    |   midExp
    |   tokenExp
    |   spanExp
    ;

conditionalExp 
    :   'cond'
        '('
        vfrStatementExpression //
        '?'
        vfrStatementExpression //
        ':'  
        vfrStatementExpression //
        ')'
    ;

findExp 
    :   'find'
        '('
        findFormat ('|' findFormat)*
        ','
        vfrStatementExpression
        ','
        vfrStatementExpression
        ','
        vfrStatementExpression
        ')'
    ;

findFormat 
    :   'SENSITIVE' | 'INSENSITIVE'
    ;

midExp 
    :   'mid'
        '('
        vfrStatementExpression
        ','
        vfrStatementExpression
        ','
        vfrStatementExpression
        ')'
    ;

tokenExp 
    :   'token'
        '('
        vfrStatementExpression
        ','
        vfrStatementExpression
        ','
        vfrStatementExpression
        ')'
    ;

spanExp 
    :   'span'
        '('
        'flags' '=' spanFlags ('|' spanFlags)*
        ','
        vfrStatementExpression
        ','
        vfrStatementExpression
        ','
        vfrStatementExpression
        ')'
    ;

spanFlags
    :   Number
    |   'LAST_NON_MATCH'
    |   'FIRST_NON_MATCH'
    ;

vfrExpressionMap 
    :   'map'
        '('
        vfrStatementExpression
        ':'
        (   vfrStatementExpression
            ','
            vfrStatementExpression
            ';'
        )*
        ')'
    ;

vfrExpressionMatch2
    :   'match2'
        '(' vfrStatementExpression',' ////////vfrStatementExpressionPattern
        vfrStatementExpression ','////////vfrStatementExpressionString
        guidDefinition ')'/////////////////////////////
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
STEP : 'step';
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

VarId : 'varid';
Question : 'question';
QuestionId : 'questionid';
Image : 'image';
Locked : 'locked';
Rule : 'rule';
EndRule : 'endrule';
Value : 'value';
Read : 'read';
Write : 'write';
ResetButton : 'resetbutton';
EndResetButton : 'endresetbutton';
DefaultStore : 'defaultstore';
Attribute : 'attribute';
Varstore : 'varstore';
Efivarstore : 'efivarstore';
VarSize : 'varsize';
NameValueVarStore : 'namevaluevarstore';
Action : 'action';
Config : 'config';
EndAction :'endaction';
Refresh : 'refresh';
Interval : 'interval';
VarstoreDevice : 'varstoredevice';
GuidOp : 'guidop';
EndGuidOp : 'endguidop';
DataType : 'datatype';
Data : 'data';
Modal : 'modal';

//
// Define the class and subclass tokens
//
ClassNonDevice : 'NON_DEVICE';
ClassDiskDevice : 'DISK_DEVICE';
ClassVideoDevice : 'VIDEO_DEVICE';
ClassNetworkDevice : 'NETWORK_DEVICE';
ClassInputDevice : 'INPUT_DEVICE';
ClassOnBoardDevice : 'ONBOARD_DEVICE';
ClassOtherDevice : 'OTHER_DEVICE';

SubclassSetupApplication : 'SETUP_APPLICATION';
SubclassGeneralApplication : 'GENERAL_APPLICATION';
SubclassFrontPage : 'FRONT_PAGE';
SubclassSingleUse : 'SINGLE_USE';

Cond : 'cond';
Find : 'find';
Mid : 'mid';
Tok : 'token';
Span : 'span';


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