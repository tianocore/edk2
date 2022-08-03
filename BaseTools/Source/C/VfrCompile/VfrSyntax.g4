grammar VfrSyntax;

// VFR Program
vfrProgram 
    :   (vfrPragmaPackDefinition | vfrDataStructDefinition | vfrDataUnionDefinition)* vfrFormSetDefinition
    ;

pragmaPackShowDef
    :   'show'
    ;

pragmaPackStackDef
    :   ('push' | 'pop') (',' StringIdentifier)? (',' Number)? 
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
    :   (TypeDef)? Struct NonNvDataMap? StringIdentifier? '{' vfrDataStructFields '}'  StringIdentifier? ';'
    ;

vfrDataUnionDefinition
    :   (TypeDef)? Union NonNvDataMap? StringIdentifier? '{' vfrDataStructFields'}' StringIdentifier? ';'
    ;

vfrDataStructFields
    : 
    (   dataStructField64
    |   dataStructField32
    |   dataStructField16
    |   dataStructField8
    |   dataStructFieldBool
    |   dataStructFieldString
    |   dataStructFieldDate
    |   dataStructFieldTime
    |   dataStructFieldRef
    |   dataStructFieldUser
    |   dataStructBitField64
    |   dataStructBitField32
    |   dataStructBitField16
    |   dataStructBitField8
    )*
    ;

dataStructField64
    :   'UINT64' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField32
    :   'UINT32' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField16
    :   'UINT16' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructField8
    :   'UINT8' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldBool
    :   'BOOLEAN' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldString
    :   'EFI_STRING_ID' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldDate
    :   'EFI_HII_DATE' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldTime
    :   'EFI_HII_TIME' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldRef
    :   'EFI_HII_REF' StringIdentifier ('[' Number ']')? ';'
    ;

dataStructFieldUser
    :   StringIdentifier StringIdentifier ('[' Number ']')? ';'
    ;

dataStructBitField64
    :   'UINT64'  StringIdentifier? ':' Number ';'
    ;
dataStructBitField32
    :   'UINT32' StringIdentifier? ':' Number ';'
    ;
dataStructBitField16
    :   'UINT16' StringIdentifier? ':' Number ';'
    ;
dataStructBitField8
    :   'UINT8' StringIdentifier? ':' Number ';'
    ;

// 2.4 VFR FormSet Definition
vfrFormSetDefinition
    :   'formset'
        'guid' '=' guidDefinition ','
        'title' '=' getStringId ','
        'help' '=' getStringId ','
        ('classguid' '=' classguidDefinition ',')?
        ('class' '=' classDefinition ',')?
        ('subclass' '=' subclassDefinition ',')?
        vfrFormSetList
        'endformset' ';'
    ;

classguidDefinition
    :   guidDefinition ('|' guidDefinition)? ('|' guidDefinition)?
    ;

classDefinition 
    :   validClassNames ('|' validClassNames)*
    ;

validClassNames 
    :   'NON_DEVICE'
    |   'DISK_DEVICE'
    |   'VIDEO_DEVICE'
    |   'NETWORK_DEVICE'
    |   'INPUT_DEVICE'
    |   'ONBOARD_DEVICE'
    |   'OTHER_DEVICE'
    |   Number
    ;

subclassDefinition 
    :   'SETUP_APPLICATION'
    |   'GENERAL_APPLICATION'
    |   'FRONT_PAGE'
    |   'SINGLE_USE'
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
        'prompt' '=' getStringId
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
        'name' '=' StringIdentifier ','
        'guid' '=' guidDefinition ';'
    ;

vfrStatementVarStoreNameValue 
    :   'namevaluevarstore' StringIdentifier ','
        ('varid' '=' Number ',')?
        ('name' '=' getStringId ',')+
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

vfrStatementHeader 
    :   'prompt' '=' getStringId ','
        'help' '=' getStringId //
    ;

vfrQuestionHeader 
    :   ('name' '=' StringIdentifier ',')?
        ('varid' '=' vfrStorageVarId ',')?
        ('questionid' '=' Number ',')?
        vfrStatementHeader
    ;

questionheaderFlagsField 
    :   'READ_ONLY'
    |   'INTERACTIVE'
    |   'RESET_REQUIRED'
    |   'OPTIONS_ONLY'
    ;
//2.10.5
vfrStorageVarId 
    :   (StringIdentifier '[' Number ']')
    |   (StringIdentifier ('.' StringIdentifier ('[' Number ']')? )* )
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
        'title' '=' getStringId ';'
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
        'text' '=' getStringId (',' 'flags' '=' vfrSubtitleFlags)?
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
        'help' '=' getStringId ','
        'text' '=' getStringId
        (',' 'text' '=' getStringId)?
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
    :   'goto'
        (   (   'devicePath' '=' getStringId ','
                'formsetguid' '=' guidDefinition ','
                'formid' '=' Number ','
                'question' '=' Number ','
            )
            |
            (   'formsetguid' '=' guidDefinition ','
                'formid' '=' Number ','
                'question' '=' Number ','
            )
            |
            (   'formid' '=' Number ','
                'question' '=' (StringIdentifier ',' | Number ',')
            )
            |
            (   Number ',' )
        )?
        vfrQuestionHeader
        (',' 'flags' '=' vfrGotoFlags)?
        (',' 'key' '=' Number)?
        (',' vfrStatementQuestionOptionList)? ';'
    ;
vfrGotoFlags 
    :   gotoFlagsField ('|' gotoFlagsField)*
    ;

gotoFlagsField 
    :  Number | questionheaderFlagsField
    ;

vfrStatementResetButton 
    :  'resetbutton'
       'defaultStore' '=' StringIdentifier ','
       vfrStatementHeader ','
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
    :   'checkbox'
        vfrQuestionHeader ','
        ('flags' '=' vfrCheckBoxFlags ',')?
        ('key' '=' Number ',')?
        vfrStatementQuestionOptionList
        'endcheckbox' ';'
    ;

vfrCheckBoxFlags 
    :   checkboxFlagsField ('|' checkboxFlagsField)*
    ;

checkboxFlagsField 
    :   Number
    |   'CHECKBOX_DEFAULT'
    |   'CHECKBOX_DEFAULT_MFG'
    |   questionheaderFlagsField
    ;

vfrStatementAction 
    :   'action'
        vfrQuestionHeader ','
        ('flags' '=' vfrActionFlags ',')?
        'config' '=' getStringId ','
        vfrStatementQuestionTagList
        'endaction' ';'
    ;

vfrActionFlags
    :   actionFlagsField ('|' actionFlagsField)*
    ;
actionFlagsField 
    :   Number | questionheaderFlagsField
    ;

vfrStatementNumericType 
    :   vfrStatementNumeric | vfrStatementOneOf
    ;

vfrStatementNumeric 
    :   'numeric'
        vfrQuestionHeader ','
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
    :   'oneof'
        vfrQuestionHeader ','
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
    :   'string'
        vfrQuestionHeader ','
        ('flags' '=' vfrStringFlagsField ',')?
        ('key' '=' Number ',')?
        'minsize' '=' Number ','
        'maxsize' '=' Number ','
        vfrStatementQuestionOptionList
        'endstring' ';'
    ;

vfrStringFlagsField 
    :   stringFlagsField ('|' stringFlagsField)*
    ;

stringFlagsField 
    :   Number
    |   'MULTI_LINE'
    |   questionheaderFlagsField
    ;

vfrStatementPassword 
    :   'password'
        vfrQuestionHeader ','
        ('flags' '=' vfrPasswordFlagsField ',')?
        ('key' '=' Number ',')?
        'minsize' '=' Number ','
        'maxsize' '=' Number ','
        vfrStatementQuestionOptionList
        'endpassword' ';'
    ;

vfrPasswordFlagsField 
    :   passwordFlagsField ('|' passwordFlagsField)*
    ;

passwordFlagsField 
    :   Number
    |   questionheaderFlagsField
    ;

vfrStatementOrderedList 
    :   'orderedlist'
        vfrQuestionHeader ','
        ('maxcontainers' '=' Number ',')?
        ('flags' '=' vfrOrderedListFlags)?
        vfrStatementQuestionOptionList
        'endlist' ';'
    ;

vfrOrderedListFlags 
    :   orderedlistFlagsField ('|' orderedlistFlagsField)*
    ;

orderedlistFlagsField 
    :   Number
    |   'UNIQUE'
    |   'NOEMPTY'
    |   questionheaderFlagsField
    ;

vfrStatementDate 
    :   'date'
        (   (   vfrQuestionHeader ','
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
    :   'time'
        (   (   vfrQuestionHeader ','
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
        'title' '=' getStringId ','
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
    :   (',' 'data' ('[' Number ']')?
        ('.' StringIdentifier ('[' Number ']')? )* '=' Number)*
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
    :   'get' '(' vfrStorageVarId('|' 'flags' '=' vfrNumericFlags)? ')'
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
    :   'set'
        '('
        vfrStorageVarId('|' 'flags' '=' vfrNumericFlags)? ','
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
/*OpenBrace('{')                           '\{'
CloseBrace('}')                          '\}'
OpenParen('(')                           '\('
CloseParen(')')                          '\)'
OpenBracket('[')                         '\['
CloseBracket(']')                        '\]'

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