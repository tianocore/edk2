/** @file
 
 The file is used to provides all kinds of Data Validation interface 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.frameworkwizard.common;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 The class is used to provides all kinds of data validation interface

 <p>All provided interfaces are in static mode</p>
 
 **/
public class DataValidation {

    /**
     Reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {

    }

    //
    // The below is used to check common data types
    //

    /**
     Check if the imput data is int
     
     @param strInt The input string which needs validation
     
     @retval true - The input is Int
     @retval false - The input is not Int
     
     **/
    public static boolean isInt(String strInt) {
        return isMatch("^-?[0-9]\\d*$", strInt);
    }

    /**
     Check if the input data is int and it is in the valid scope
     The scope is provided by String
     
     @param strNumber The input string which needs validation
     @param BeginNumber The left boundary of the scope
     @param EndNumber The right boundary of the scope
     
     @retval true - The input is Int and in the scope;
     @retval false - The input is not Int or not in the scope
     
     **/
    public static boolean isInt(String strNumber, int BeginNumber, int EndNumber) {
        //
        //Check if the input data is int first
        //
        if (!isInt(strNumber)) {
            return false;
        }
        //
        //And then check if the data is between the scope
        //
        Integer intTemp = new Integer(strNumber);
        if ((intTemp.intValue() < BeginNumber) || (intTemp.intValue() > EndNumber)) {
            return false;
        }
        return true;
    }

    /**
    Check if the input data is long int and it is in the valid scope
    The scope is provided by String
    
    @param strNumber The input string which needs validation
    @param BeginNumber The left boundary of the scope
    @param EndNumber The right boundary of the scope
    
    @retval true - The input is Int and in the scope;
    @retval false - The input is not Int or not in the scope
    
    **/
   public static boolean isLongInt(String strNumber, long BeginNumber, long EndNumber) throws Exception{
       //
       //Check if the input data is int first
       //
       if (!isInt(strNumber)) {
           return false;
       }
       //
       //And then check if the data is between the scope
       //
       try {
    	   Long intTemp = new Long(strNumber);
    	   if ((intTemp.longValue() < BeginNumber) || (intTemp.longValue() > EndNumber)) {
               return false;
           }
       }
       catch (Exception e) {
    	   throw e;
       }
       
       return true;
   }
    /**
     Check if the input data is int and it is in the valid scope
     The scope is provided by String
     
     @param strNumber The input string which needs validation
     @param strBeginNumber The left boundary of the scope
     @param strEndNumber The right boundary of the scope
     
     @retval true - The input is Int and in the scope;
     @retval false - The input is not Int or not in the scope
     
     **/
    public static boolean isInt(String strNumber, String strBeginNumber, String strEndNumber) {
        //
        //Check if all input data are int
        //
        if (!isInt(strNumber)) {
            return false;
        }
        if (!isInt(strBeginNumber)) {
            return false;
        }
        if (!isInt(strEndNumber)) {
            return false;
        }
        //
        //And then check if the data is between the scope
        //
        Integer intI = new Integer(strNumber);
        Integer intJ = new Integer(strBeginNumber);
        Integer intK = new Integer(strEndNumber);
        if ((intI.intValue() < intJ.intValue()) || (intI.intValue() > intK.intValue())) {
            return false;
        }
        return true;
    }

    /**
     Use regex to check if the input data is in valid format
     
     @param strPattern The input regex
     @param strMatcher The input data need be checked
     
     @retval true - The data matches the regex
     @retval false - The data doesn't match the regex
     
     **/
    public static boolean isMatch(String strPattern, String strMatcher) {
        Pattern pattern = Pattern.compile(strPattern);
        Matcher matcher = pattern.matcher(strMatcher);

        return matcher.find();
    }

    //
    // The below is used to check common customized data types
    //

    /**
     Check if the input data is UiNameType
     
     @param arg0 The input string need be checked
     
     @retval true - The input is UiNameType
     @retval false - The input is not UiNameType
     
     **/
    public static boolean isUiNameType(String arg0) {
        if (arg0.length() < 1) {
            return false;
        }
        return isMatch("[^ ].*", arg0);
    }

    /**
     Check if the input data is GuidType2
     
     @param arg0 The input string need be checked
     
     @retval true - The input is GuidType2
     @retval false - The input is not GuidType2
     
     **/
    public static boolean isGuidType2(String arg0) {
        return isMatch("[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}", arg0);
    }

    /**
     Check if the input data is Guid 
     
     @param strGuid The input string need be checked
     
     @retval true - The input is Guid
     @retval false - The input is not Guid
     
     **/
    public static boolean isGuid(String arg0) {
        return isGuidType2(arg0);
    }

    /**
     Check if the input data is Version 
     
     @param arg0 The input string need be checked
     
     @retval true - The input is Version
     @retval false - The input is not Version
     
     **/
    public static boolean isVersionDataType(String arg0) {
        return isMatch("\\d+(\\.\\d+)*", arg0);
    }

    /**
     Check if the input data is Sentence
     
     @param strSentence The input string need be checked
     
     @retval true - The input is Sentence
     @retval false - The input is not Sentence
     
     **/
    public static boolean isSentence(String arg0) {
        return isMatch("(\\w+\\W*)+( )+(\\W*\\w*\\W*\\s*)*", arg0);
    }

    /**
     Check if the input data is FileNameConventio
     
     @param strSentence The input string need be checked
     
     @retval true - The input is FileNameConventio
     @retval false - The input is not FileNameConventio
     
     **/
    public static boolean isFileNameConvention(String arg0) {
        return isMatch("[a-zA-Z](\\.?[-_a-zA-Z0-9]+)*", arg0);
    }

    /**
     Check if the input data is KeywordType
     
     @param strSentence The input string need be checked
     
     @retval true - The input is KeywordType
     @retval false - The input is not KeywordType
     
     **/
    public static boolean isKeywordType(String arg0) {
        return isMatch("[a-zA-Z]+(_*[a-zA-Z0-9]*)*", arg0);
    }

    /**
     Check if the input data is FeatureFlagExpressionType
     
     @param strSentence The input string need be checked
     
     @retval true - The input is FeatureFlagExpressionType
     @retval false - The input is not FeatureFlagExpressionType
     
     **/
    public static boolean isFeatureFlagExpressionType(String arg0) {
        return (arg0.length() >= 1);
    }

    /**
     Check if the input data is FeatureFlag
     
     @param strSentence The input string need be checked
     
     @retval true - The input is FeatureFlag
     @retval false - The input is not FeatureFlag
     
     **/
    public static boolean isFeatureFlag(String arg0) {
        return isFeatureFlagExpressionType(arg0);
    }

    /**
     Check if the input data is PathAndFilename
     
     @param strSentence The input string need be checked
     
     @retval true - The input is PathAndFilename
     @retval false - The input is not PathAndFilename
     
     **/
    public static boolean isPathAndFilename(String arg0) {
        return !arg0.equals("");
    }

    /**
     Check if the input data is ToolsNameConvention
     
     @param strSentence The input string need be checked
     
     @retval true - The input is ToolsNameConvention
     @retval false - The input is not ToolsNameConvention
     
     **/
    public static boolean isToolsNameConvention(String arg0) {
        return isMatch("[a-zA-Z][a-zA-Z0-9]*", arg0);
    }

    /**
     Check if the input data is C_NameType
     
     @param strSentence The input string need be checked
     
     @retval true - The input is C_NameType
     @retval false - The input is not C_NameType
     
     **/
    public static boolean isC_NameType(String arg0) {
        return isMatch("(_)*[a-zA-Z]+((_)*[a-zA-Z0-9]*)*", arg0);
    }

    /**
     Check if the input data is HexWordArrayType
     
     @param strSentence The input string need be checked
     
     @retval true - The input is HexWordArrayType
     @retval false - The input is not HexWordArrayType
     
     **/
    public static boolean isHexWordArrayType(String arg0) {
        return arg0.length() >=6 && isMatch("((\\s)*0x([a-fA-F0-9]){4}(,)?(\\s)*)+", arg0);
    }

    /**
     Check if the input data is Hex64BitDataType
     
     @param strSentence The input string need be checked
     
     @retval true - The input is Hex64BitDataType
     @retval false - The input is not Hex64BitDataType
     
     **/
    public static boolean isHex64BitDataType(String arg0) {
        return isMatch("(0x)?[a-fA-F0-9]{1,16}", arg0);
    }

    /**
     Check if the input data is UnicodeString
     
     @param strSentence The input string need be checked
     
     @retval true - The input is UnicodeString
     @retval false - The input is not UnicodeString
     
     **/
    public static boolean isUnicodeString(String arg0) {
        return arg0.length() >= 3 && isMatch("(\\s)*L(\\:)?\"[^\"]*\"(\\s)*", arg0);
    }

    /**
     Check if the input data is HexByteArrayType
     
     @param strSentence The input string need be checked
     
     @retval true - The input is HexByteArrayType
     @retval false - The input is not HexByteArrayType
     
     **/
    public static boolean isHexByteArrayType(String arg0) {
        return arg0.length() >= 4 && isMatch("((\\s)*0x([a-fA-F0-9]){2}(,)?(\\s)*)+", arg0);
    }
    

    /**
     Check if the input data is DateType
     
     @param strDateType The input string need be checked
     
     @retval true - The input is DateType
     @retval false - The input is not DateType
     
     **/
    public static boolean isDateType(String strDateType) {
        return isMatch("[1-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9] [0-2][0-9]:[0-5][0-9]", strDateType);
    }

    /**
     Check if the input data is DosPath
     
     @param strDosPath The input string need be checked
     
     @retval true - The input is DosPath
     @retval false - The input is not DosPath
     
     **/
    public static boolean isDosPath(String strDosPath) {
        return isMatch("([a-zA-Z]:\\\\)?(((\\\\?_*-*.*[a-zA-Z0-9]*)*(_*-*.*[a-zA-Z0-9])*)+(\\\\)?)*", strDosPath);
    }

    /**
     Check if the input data is UnixPath
     
     @param strUnixPath The input string need be checked
     
     @retval true - The input is UnixPath
     @retval false - The input is not UnixPath
     
     **/
    public static boolean isUnixPath(String strUnixPath) {
        return isMatch("(\\/)?(((_*-*.*[a-zA-Z0-9]*)*(_*-*.*[a-zA-Z0-9])*)+(\\/)?)*", strUnixPath);
    }

    /**
     Check if the input data is DirectoryNamingConvention
     
     @param strDirectoryNamingConvention The input string need be checked
     
     @retval true - The input is DirectoryNamingConvention
     @retval false - The input is not DirectoryNamingConvention
     
     **/
    public static boolean isDirectoryNamingConvention(String strDirectoryNamingConvention) {
        return (isDosPath(strDirectoryNamingConvention) || isUnixPath(strDirectoryNamingConvention));
    }

    /**
     Check if the input data is HexDoubleWordDataType
     
     @param strHexDoubleWordDataType  The input string need be checked
     
     @retval true - The input is HexDoubleWordDataType
     @retval false - The input is not HexDoubleWordDataType
     
     **/
    public static boolean isHexDoubleWordDataType(String strHexDoubleWordDataType) {
        return isMatch("0x[a-fA-F0-9]{1,8}", strHexDoubleWordDataType);
    }

    /**
     Check if the input data is V1
     
     @param strV1 The input string need be checked
     
     @retval true - The input is V1
     @retval false - The input is not V1
     
     **/
    public static boolean isV1(String strV1) {
        return isMatch("((%[A-Z](_*[A-Z0-9]*)*%)+((((\\\\)?_*-*.*[a-zA-Z0-9]*)*(_*-*.*[a-zA-Z0-9])*)+(\\\\)?)*)", strV1);
    }

    /**
     Check if the input data is V2
     
     @param strV2 The input string need be checked
     
     @retval true - The input is V2
     @retval false - The input is not V2
     
     **/
    public static boolean isV2(String strV2) {
        return isMatch(
                       "(($[A-Z](_*[A-Z0-9]*)*)+||($\\([A-Z](_*[A-Z0-9]*)*\\))+||($\\{[A-Z](_*[A-Z0-9]*)*\\})+)+(\\/)?(((((_*-*.*[a-zA-Z0-9]*)*(_*-*.*[a-zA-Z0-9])*)+(\\/)?)*)*)",
                       strV2);
    }

    /**
     Check if the input data is VariableConvention
     
     @param strVariableConvention The input string need be checked
     
     @retval true - The input is VariableConvention
     @retval false - The input is not VariableConvention
     
     **/
    public static boolean isVariableConvention(String strVariableConvention) {
        return (isV1(strVariableConvention) || isV2(strVariableConvention));
    }

    /**
     Check if the input data is UCName
     
     @param strUCName The input string need be checked
     
     @retval true - The input is UCName
     @retval false - The input is not UCName
     
     **/
    public static boolean isUCName(String strUCName) {
        return isMatch("[A-Z]+(_*[A-Z0-9]*( )*)*", strUCName);
    }

    /**
     Check if the input data is HexByteDataType
     
     @param strHex64BitDataType The input string need be checked 
     
     @retval true - The input is HexByteDataType
     @retval false - The input is not HexByteDataType
     
     **/
    public static boolean isHexByteDataType(String strHex64BitDataType) {
        return isMatch("(0x)?[a-fA-F0-9]{1,2}", strHex64BitDataType);
    }

    /**
     Check if the input data is HexWordDataType
     
     @param strHexWordDataType The input string need be checked
     
     @retval true - The input is HexWordDataType
     @retval false - The input is not HexWordDataType
     
     **/
    public static boolean isHexWordDataType(String strHexWordDataType) {
        return isMatch("0x[a-fA-F0-9]{1,4}", strHexWordDataType);
    }

    /**
     Check if the input data is OverrideID
     
     @param strOverrideID The input string need be checked
     
     @retval true - The input is OverrideID
     @retval false - The input is not OverrideID
     
     **/
    public static boolean isOverrideID(String strOverrideID) {
        return isInt(strOverrideID);
    }

    /**
     Check if the input data is Supported Architectures
     
     @param strSupportedArchitectures
     @retval true - The input is Supported Architectures
     @retval false - The input isn't Supported Architectures
     
     */
    public static boolean isSupportedArchitectures(String strSupportedArchitectures) {
        return isMatch("(ALL){1}|(((IA32)|((X64)|(IPF)|(EBC)){1}((,((IA32)|(X64)|(IPF)|(EBC)){1} ){0,2}))){1}",
                       strSupportedArchitectures);
    }

    //
    //The below is used to check msaheader data type
    //

    /**
     Check if the input data is BaseName
     
     @param strBaseName The input string need be checked
     
     @retval true - The input is BaseName
     @retval false - The input is not BaseName
     
     **/
    public static boolean isBaseName(String arg0) {
        return isUiNameType(arg0);
    }

    /**
     Check if the input data is Version
     
     @param arg0 The input string need be checked

     @retval true - The input is Version
     @retval false - The input is not Version
     
     **/
    public static boolean isVersion(String arg0) {
        return isVersionDataType(arg0);
    }

    /**
     Check if the input data is Abstract
     
     @param strAbstract The input string need be checked
     
     @retval true - The input is Abstract
     @retval false - The input is not Abstract
     
     **/
    public static boolean isAbstract(String arg0) {
        return isSentence(arg0);
    }

    /**
     Check if the input data is Copyright
     
     @param strCopyright The input string need be checked
     
     @retval true - The input is Copyright
     @retval false - The input is not Copyright
     
     **/
    public static boolean isCopyright(String arg0) {
        return !arg0.equals("");
    }

    /**
     Check if the input data is Specification
     
     @param strCopyright The input string need be checked
     
     @retval true - The input is Specification
     @retval false - The input is not Specification
     
     **/
    public static boolean isSpecification(String arg0) {
        return isSentence(arg0);
    }

    //
    // The below is used to check ModuleDefinitions data types
    //
    /**
     Check if the input data is OutputFileBasename
     
     @param strCopyright The input string need be checked
     
     @retval true - The input is OutputFileBasename
     @retval false - The input is not OutputFileBasename
     
     **/
    public static boolean isOutputFileBasename(String arg0) {
        return isFileNameConvention(arg0);
    }

    //
    // The below is used to check LibraryClass data types
    //
    /**
     Check if the input data is LibraryClass
     
     @param strCopyright The input string need be checked
     
     @retval true - The input is LibraryClass
     @retval false - The input is not LibraryClass
     
     **/
    public static boolean isLibraryClass(String arg0) {
        return isKeywordType(arg0);
    }

    /**
     Check if the input data is RecommendedInstanceVersion
     
     @param strCopyright The input string need be checked
     
     @retval true - The input is RecommendedInstanceVersion
     @retval false - The input is not RecommendedInstanceVersion
     
     **/
    public static boolean isRecommendedInstanceVersion(String arg0) {
        return isVersionDataType(arg0);
    }

    //
    // The below is used to check sourcefiles data types
    //

    /**
     Check if the input data is Filename
     
     @param strPath The input string need be checked
     
     @retval true - The input is Filename
     @retval false - The input is not Filename
     
     **/
    public static boolean isFilename(String arg0) {
        return isPathAndFilename(arg0);
    }

    /**
     Check if the input data is TagName
     
     @param strPath The input string need be checked
     
     @retval true - The input is TagName
     @retval false - The input is not TagName
     
     **/
    public static boolean isTagName(String arg0) {
        return isToolsNameConvention(arg0);
    }

    /**
     Check if the input data is ToolCode
     
     @param strPath The input string need be checked
     
     @retval true - The input is ToolCode
     @retval false - The input is not ToolCode
     
     **/
    public static boolean isToolCode(String arg0) {
        return isToolsNameConvention(arg0);
    }

    /**
     Check if the input data is ToolChainFamily
     
     @param strPath The input string need be checked
     
     @retval true - The input is ToolChainFamily
     @retval false - The input is not ToolChainFamily
     
     **/
    public static boolean isToolChainFamily(String arg0) {
        return isToolsNameConvention(arg0);
    }

    //
    // The below is used to check pcdcoded data types
    //
    /**
    Check if the input data is DefaultValueType
    
    @param strPath The input string need be checked
    
    @retval true - The input is DefaultValueType
    @retval false - The input is not DefaultValueType
    
    **/
   public static boolean isDefaultValueType(String arg0) {
       return isHex64BitDataType(arg0) || isUnicodeString(arg0) || isHexByteArrayType(arg0);
   }

}
