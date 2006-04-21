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

package org.tianocore.common;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 The class is used to provides all kinds of data validation interface

 <p>All provided interfaces are in static mode</p>
 
 @since ModuleEditor 1.0
 
 **/
public class DataValidation {

    /**
     Reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

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
     Check if the input data is BaseNameConvention
     
     @param strBaseNameConvention The input string need be checked
     
     @retval true - The input is BaseNameConvention
     @retval false - The input is not BaseNameConvention
     
     **/
    public static boolean isBaseNameConvention(String strBaseNameConvention) {
        return isMatch("[A-Z]([a-zA-Z0-9])*(_)?([a-zA-Z0-9])*", strBaseNameConvention);
    }

    /**
     Check if the input data is GuidArrayType
     
     @param strGuidArrayType The input string need be checked
     
     @retval true - The input is GuidArrayType
     @retval false - The input is not GuidArrayType
     
     **/
    public static boolean isGuidArrayType(String strGuidArrayType) {
        return isMatch(
                       "0x[a-fA-F0-9]{1,8},( )*0x[a-fA-F0-9]{1,4},( )*0x[a-fA-F0-9]{1,4}(,( )*\\{)?(,?( )*0x[a-fA-F0-9]{1,2}){8}( )*(\\})?",
                       strGuidArrayType);
    }

    /**
     Check if the input data is GuidNamingConvention
     
     @param strGuidNamingConvention The input string need be checked
     
     @retval true - The input is GuidNamingConvention
     @retval false - The input is not GuidNamingConvention
     
     **/
    public static boolean isGuidNamingConvention(String strGuidNamingConvention) {
        return isMatch("[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}",
                       strGuidNamingConvention);
    }

    /**
     Check if the input data is GuidType 
     
     @param strGuidType The input string need be checked
     
     @retval true - The input is GuidType
     @reture false is not GuidType
     
     **/
    public static boolean isGuidType(String strGuidType) {
        return (isGuidArrayType(strGuidType) || isGuidNamingConvention(strGuidType));
    }

    /**
     Check if the input data is Guid 
     
     @param strGuid The input string need be checked
     
     @retval true - The input is Guid
     @retval false - The input is not Guid
     
     **/
    public static boolean isGuid(String strGuid) {
        return isGuidType(strGuid);
    }

    /**
     Check if the input data is Sentence
     
     @param strSentence The input string need be checked
     
     @retval true - The input is Sentence
     @retval false - The input is not Sentence
     
     **/
    public static boolean isSentence(String strSentence) {
        return isMatch("(\\w+\\W*)+( )+(\\W*\\w*\\W*\\s*)*", strSentence);
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
     Check if the input data is Hex64BitDataType
     
     @param strHex64BitDataType The input string need be checked
     
     @retval true - The input is Hex64BitDataType
     @retval false - The input is not Hex64BitDataType
     
     **/
    public static boolean isHex64BitDataType(String strHex64BitDataType) {
        return isMatch("(0x)?[a-fA-F0-9]{1,16}", strHex64BitDataType);
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
     Check if the input data is CName
     
     @param strCName The input string need be checked
     
     @retval true - The input is CName
     @retval false - The input is not CName
     
     **/
    public static boolean isCName(String strCName) {
        return isMatch("((_)*([a-zA-Z])+((_)*[a-zA-Z0-9]*))*", strCName);
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

    //
    //The below is used to check msaheader data type
    //

    /**
     Check if the input data is BaseName
     
     @param strBaseName The input string need be checked
     
     @retval true - The input is BaseName
     @retval false - The input is not BaseName
     
     **/
    public static boolean isBaseName(String strBaseName) {
        return isBaseNameConvention(strBaseName);
    }

    /**
     Check if the input data is Abstract
     
     @param strAbstract The input string need be checked
     
     @retval true - The input is Abstract
     @retval false - The input is not Abstract
     
     **/
    public static boolean isAbstract(String strAbstract) {
        return isSentence(strAbstract);
    }

    /**
     Check if the input data is Copyright
     
     @param strCopyright The input string need be checked
     
     @retval true - The input is Copyright
     @retval false - The input is not Copyright
     
     **/
    public static boolean isCopyright(String strCopyright) {
        return isSentence(strCopyright);
    }

    /**
     Check if the input data is Created
     
     @param strCreated The input string need be checked
     
     @retval true - The input is Created
     @retval false - The input is not Created
     
     **/
    public static boolean isCreated(String strCreated) {
        return isDateType(strCreated);
    }

    /**
     Check if the input data is Updated
     
     @param strUpdated The input string need be checked
     
     @retval true - The input is Updated
     @retval false - The input is not Updated
     
     **/
    public static boolean isUpdated(String strUpdated) {
        return isDateType(strUpdated);
    }

    //
    // The below is used to check LibraryClass data types
    //

    /**
     Check if the input data is LibraryClass
     
     @param strLibraryClass The input string need be checked
     
     @retval true - The input is LibraryClass
     @retval false - The input is not LibraryClass
     
     **/
    public static boolean isLibraryClass(String strLibraryClass) {
        return isBaseNameConvention(strLibraryClass);
    }

    //
    // The below is used to check sourcefiles data types
    //

    /**
     Check if the input data is Path
     
     @param strPath The input string need be checked
     
     @retval true - The input is Path
     @retval false - The input is not Path
     
     **/
    public static boolean isPath(String strPath) {
        return isDirectoryNamingConvention(strPath);
    }

    /**
     Check if the input data is FileName
     
     @param strFileName The input string need be checked
     
     @retval true - The input is FileName
     @retval false - The input is not FileName
     
     **/
    public static boolean isFileName(String strFileName) {
        return isVariableConvention(strFileName);
    }

    //
    // The below is used to check includes data types
    //

    /**
     Check if the input data is UpdatedDate
     
     @param strUpdatedDate The input string need be checked
     
     @retval true - The input is UpdatedDate
     @retval false - The input is not UpdatedDate
     
     **/
    public static boolean isUpdatedDate(String strUpdatedDate) {
        return isDateType(strUpdatedDate);
    }

    /**
     Check if the input data is PackageName
     
     @param strPackageName The input string need be checked
     
     @retval true - The input is PackageName
     @retval false - The input is not PackageName
     
     **/
    public static boolean isPackageName(String strPackageName) {
        return isBaseNameConvention(strPackageName);
    }

    //
    // The below is used to check protocols data types
    //

    /**
     Check if the input data is ProtocolName
     
     @param strProtocolName The input string need be checked
     
     @retval true - The input is ProtocolName
     @retval false - The input is not ProtocolName
     
     **/
    public static boolean isProtocolName(String strProtocolName) {
        return isCName(strProtocolName);
    }

    /**
     Check if the input data is ProtocolNotifyName
     
     @param strProtocolNotifyName The input string need be checked
     
     @retval true - The input is ProtocolNotifyName
     @retval false - The input is not ProtocolNotifyName
     
     **/
    public static boolean isProtocolNotifyName(String strProtocolNotifyName) {
        return isCName(strProtocolNotifyName);
    }

    //
    // The below is used to check ppis data types
    //

    /**
     Check if the input data is PpiName
     
     @param strPpiName The input string need be checked
     
     @retval true - The input is PpiName
     @retval false - The input is not PpiName
     
     **/
    public static boolean isPpiName(String strPpiName) {
        return isCName(strPpiName);
    }

    /**
     Check if the input data is PpiNotifyName
     
     @param strPpiNotifyName The input string need be checked
     
     @retval true - The input is PpiNotifyName
     @retval false - The input is not PpiNotifyName
     
     **/
    public static boolean isPpiNotifyName(String strPpiNotifyName) {
        return isCName(strPpiNotifyName);
    }

    /**
     Check if the input data is FeatureFlag
     
     @param strFeatureFlag The input string need be checked
     
     @retval true - The input is FeatureFlag
     @retval false - The input is not FeatureFlag
     
     **/
    public static boolean isFeatureFlag(String strFeatureFlag) {
        return isCName(strFeatureFlag);
    }

    //
    // The below is used to check variable data types
    //

    /**
     Check if the input data is ByteOffset
     
     @param strByteOffset The input string need be checked
     
     @retval true - The input is ByteOffset
     @retval false - The input is not ByteOffset
     
     **/
    public static boolean isByteOffset(String strByteOffset) {
        return isByteOffset(strByteOffset);
    }

    /**
     Check if the input data is BitOffset
     
     @param strBitOffset The input string need be checked
     
     @retval true - The input is BitOffset
     @retval false - The input is not BitOffset
     
     **/
    public static boolean isBitOffset(String strBitOffset) {
        return isInt(strBitOffset, 0, 8);
    }

    /**
     Check if the input data is OffsetBitSize
     
     @param strOffsetBitSize The input string need be checked
     
     @retval true - The input is OffsetBitSize
     @retval false - The input is not OffsetBitSize
     
     **/
    public static boolean isOffsetBitSize(String strOffsetBitSize) {
        return isInt(strOffsetBitSize, 0, 7);
    }

    //
    // The below is used to check formsets data types
    //

    /**
     Check if the input data is Formsets
     
     @param strFormsets The input string need be checked
     
     @retval true - The input is Formsets
     @retval false - The input is not Formsets
     
     **/
    public static boolean isFormsets(String strFormsets) {
        return isCName(strFormsets);
    }

    //
    // The below is used to check externs data types
    //

    /**
     Check if the input data is Constructor
     
     @param strConstructor The input string need be checked
     
     @retval true - The input is Constructor
     @retval false - The input is not Constructor
     
     **/
    public static boolean isConstructor(String strConstructor) {
        return isCName(strConstructor);
    }

    /**
     Check if the input data is Destructor
     
     @param strDestructor The input string need be checked
     
     @retval true - The input is Destructor
     @retval false - The input is not Destructor
     
     **/
    public static boolean isDestructor(String strDestructor) {
        return isCName(strDestructor);
    }

    /**
     Check if the input data is DriverBinding
     
     @param strDriverBinding The input string need be checked
     
     @retval true - The input is DriverBinding
     @retval false - The input is not DriverBinding
     
     **/
    public static boolean isDriverBinding(String strDriverBinding) {
        return isCName(strDriverBinding);
    }

    /**
     Check if the input data is ComponentName
     
     @param strComponentName The input string need be checked
     
     @retval true - The input is ComponentName
     @retval false - The input is not ComponentName
     
     **/
    public static boolean isComponentName(String strComponentName) {
        return isCName(strComponentName);
    }

    /**
     Check if the input data is DriverConfig
     
     @param strDriverConfig The input string need be checked
     
     @retval true - The input is DriverConfig
     @retval false - The input is not DriverConfig
     
     **/
    public static boolean isDriverConfig(String strDriverConfig) {
        return isCName(strDriverConfig);
    }

    /**
     Check if the input data is DriverDiag
     
     @param strDriverDiag The input string need be checked
     
     @retval true - The input is DriverDiag
     @retval false - The input is not DriverDiag
     
     **/
    public static boolean isDriverDiag(String strDriverDiag) {
        return isCName(strDriverDiag);
    }

    /**
     Check if the input data is SetVirtualAddressMapCallBack
     
     @param strSetVirtualAddressMapCallBack The input string need be checked
     
     @retval true - The input is SetVirtualAddressMapCallBack
     @retval false - The input is not SetVirtualAddressMapCallBack
     
     **/
    public static boolean isSetVirtualAddressMapCallBack(String strSetVirtualAddressMapCallBack) {
        return isCName(strSetVirtualAddressMapCallBack);
    }

    /**
     Check if the input data is ExitBootServicesCallBack
     
     @param strExitBootServicesCallBack The input string need be checked
     
     @retval true - The input is ExitBootServicesCallBack
     @retval false - The input is not ExitBootServicesCallBack
     
     **/
    public static boolean isExitBootServicesCallBack(String strExitBootServicesCallBack) {
        return isCName(strExitBootServicesCallBack);
    }

    /**
     Check if the input data is UserDefined
     
     @param strUserDefined The input string need be checked
     
     @retval true - The input is UserDefined
     @retval false - The input is not UserDefined
     
     **/
    public static boolean isUserDefined(String strUserDefined) {
        return isCName(strUserDefined);
    }

    //
    // The below is used to check PCDs data types
    //

    /**
     Check if the input data is Token
     
     @param strToken The input string need be checked
     
     @retval true - The input is Token
     @retval false - The input is not Token
     
     **/
    public static boolean isToken(String strToken) {
        return isHexDoubleWordDataType(strToken);
    }

    /**
     Check if the input data is MaxSku
     
     @param strMaxSku The input string need be checked
     
     @retval true - The input is MaxSku
     @retval false - The input is not MaxSku
     
     **/
    public static boolean isMaxSku(String strMaxSku) {
        return isHexByteDataType(strMaxSku);
    }

    /**
     Check if the input data is SkuId
     
     @param strSkuId The input string need be checked
     
     @retval true - The input is SkuId
     @retval false - The input is not SkuId
     
     **/
    public static boolean isSkuId(String strSkuId) {
        return isHexByteDataType(strSkuId);
    }

    /**
     Check if the input data is DatumSize
     
     @param strDatumSize The input string need be checked
     
     @retval true - The input is DatumSize
     @retval false - The input is not DatumSize
     
     **/
    public static boolean isDatumSize(String strDatumSize) {
        return isInt(strDatumSize, 1, 16777215);
    }

    /**
     Check if the input data is VariableGuid
     
     @param strVariableGuid The input string need be checked
     
     @retval true - The input is VariableGuid
     @retval false - The input is not VariableGuid
     
     **/
    public static boolean isVariableGuid(String strVariableGuid) {
        return (isGuid(strVariableGuid) || strVariableGuid.equals("0"));
    }

    /**
     Check if the input data is DataOffset
     
     @param strDataOffset The input string need be checked
     
     @retval true - The input is DataOffset
     @retval false - The input is not DataOffset
     
     **/
    public static boolean isDataOffset(String strDataOffset) {
        return isHex64BitDataType(strDataOffset);
    }

    /**
     Check if the input data is GuidOffset
     
     @param strGuidOffset The input string need be checked
     
     @retval true - The input is GuidOffset
     @retval false - The input is not GuidOffset
     
     **/
    public static boolean isGuidOffset(String strGuidOffset) {
        return isHex64BitDataType(strGuidOffset);
    }
}
