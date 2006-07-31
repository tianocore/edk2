/** @file
  Token class.

  This module contains all classes releted to PCD token.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/  
package org.tianocore.pcd.entity;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.tianocore.pcd.entity.UsageIdentification;
import org.tianocore.pcd.exception.EntityException;

/** This class is to descript a PCD token object. The information of a token mainly 
    comes from MSA, SPD and setting produced by platform developer. 
**/
public class Token {
    ///
    /// Enumeration macro defintion for PCD type.
    /// BUGBUG: Not use upcase charater is to facility for reading. It may be changed
    ///         in coding review.
    public enum             PCD_TYPE {FEATURE_FLAG, FIXED_AT_BUILD, PATCHABLE_IN_MODULE, DYNAMIC, 
                                      DYNAMIC_EX, UNKNOWN}

    ///
    /// Enumeration macro definition for datum type. All type mainly comes from ProcessBind.h.
    /// Wizard maybe expand this type as "int, unsigned int, short, unsigned short etc" in 
    /// prompt dialog.
    ///
    public enum             DATUM_TYPE {UINT8, UINT16, UINT32, UINT64, BOOLEAN, POINTER, UNKNOWN}

    ///
    /// Enumeration macor defintion for usage of PCD
    ///
    public enum             PCD_USAGE {ALWAYS_PRODUCED, ALWAYS_CONSUMED, SOMETIMES_PRODUCED,
                                       SOMETIMES_CONSUMED, UNKNOWN}

    ///
    /// cName is to identify a PCD entry and will be used for generating autogen.h/autogen.c.
    /// cName will be defined in MSA, SPD and FPD, can be regarded as primary key with token space guid.
    ///
    public String           cName;

    ///
    /// Token space name is the guid defined by token itself in package or module level. This
    /// name mainly for DynamicEx type. For other PCD type token, his token space name is the 
    /// assignedtokenSpaceName as follows.
    /// tokenSpaceName is defined in MSA, SPD, FPD, can be regarded as primary key with cName.
    ///
    public String           tokenSpaceName;

    ///
    /// tokenNumber is allocated by platform. tokenNumber indicate an index for this token in
    /// platform token space. For Dynamic, dynamicEx type, this number will be re-adjust by
    /// PCD run-time database autogen tools.
    ///
    public long             tokenNumber;

    ///
    /// This token number is retrieved from FPD file for DynamicEx type. 
    /// 
    public long             dynamicExTokenNumber;

    ///
    /// All supported PCD type, this value can be retrieved from SPD
    /// Currently, only record all PCD type for this token in FPD file.
    /// 
    public List<PCD_TYPE>   supportedPcdType;

    ///
    /// If the token's item type is Dynamic or DynamicEx type, isDynamicPCD
    /// is true.
    /// 
    public boolean          isDynamicPCD;

    ///
    /// datumSize is to descript the fix size or max size for this token. 
    /// datumSize is defined in SPD.
    ///
    public int              datumSize;

    ///
    /// datum type is to descript what type can be expressed by a PCD token.
    /// For same PCD used in different module, the datum type should be unique.
    /// So it belong memeber to Token class.
    ///
    public DATUM_TYPE       datumType;

    ///
    /// skuData contains all value for SkuNumber of token.
    /// This field is for Dynamic or DynamicEx type PCD, 
    ///
    public List<SkuInstance> skuData;

    ///
    /// consumers array record all module private information who consume this PCD token.
    ///
    public Map<String, UsageInstance>  consumers;

    /**
       Constructure function for Token class
       
       @param cName             The name of token
       @param tokenSpaceName    The name of token space, it is a guid string
    **/
    public Token(String cName, String tokenSpaceName) {
        this.cName                  = cName;
        this.tokenSpaceName         = tokenSpaceName;
        this.tokenNumber            = 0;
        this.datumType              = DATUM_TYPE.UNKNOWN;
        this.datumSize              = -1;
        this.skuData                = new ArrayList<SkuInstance>();

        this.consumers              = new HashMap<String, UsageInstance>();
        this.supportedPcdType       = new ArrayList<PCD_TYPE>();
    }

    /**
      updateSupportPcdType
      
      SupportPcdType should be gotten from SPD file actually, but now it just
      record all PCD type for this token in FPD file.
      
      @param pcdType    new PCD type found in FPD file for this token.
    **/
    public void updateSupportPcdType(PCD_TYPE pcdType) {
        for (int index = 0; index < this.supportedPcdType.size(); index ++) {
            if (supportedPcdType.get(index) == pcdType) {
                return;
            }
        }

        //
        // If not found, add the pcd type to member variable supportedPcdType
        // 
        supportedPcdType.add(pcdType);
    }

    /**
       Judge whether pcdType is belong to dynamic type. Dynamic type includes
       DYNAMIC and DYNAMIC_EX.
       
       @param pcdType       the judged pcd type
       
       @return boolean
     */
    public static boolean isDynamic(PCD_TYPE pcdType) {
        if ((pcdType == PCD_TYPE.DYNAMIC   ) ||
            (pcdType == PCD_TYPE.DYNAMIC_EX)) {
            return true;
        }

        return false;
    }

    public boolean isDynamicEx() {
        for (int i = 0; i < supportedPcdType.size(); i++) {
            if (supportedPcdType.get(i) == PCD_TYPE.DYNAMIC_EX) {
                return true;
            }
        }
        
        return false;
    }

    /**
      Use "TokencName + "-" + SpaceTokenName" as primary key when adding token into database
      
      @param   cName                     Token name.
      @param   tokenSpaceName            The token space guid string defined in MSA or SPD
      
      @retval  primary key for this token in token database.
    **/
    public static String getPrimaryKeyString(String cName, String tokenSpaceName) {
        if (tokenSpaceName == null) {
            return cName + "_nullTokenSpaceGuid";
        } else {
            return cName + "_" + tokenSpaceName.toString().replace('-', '_');
        }
    }

    /**
       If skudata list contains more than one data, then Sku mechanism is enable.
       
       @retval boolean  if the number of sku data exceed to 1
     */
    public boolean isSkuEnable() {
        if (this.skuData.size() > 1) {
            return true;
        }
        return false;
    }

    /**
       If Hii type for value of token
       
       @return boolean
    **/
    public boolean isHiiEnable() {
        if (getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.HII_TYPE) {
            return true;
        }
        return false;
    }

    /**
       If Vpd type for value of token
       
       @return boolean
    **/
    public boolean isVpdEnable() {
        if (getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.VPD_TYPE) {
            return true;
        }
        return false;
    }

    /**
       Get the token primary key in token database.
       
       @return String
     */
    public String getPrimaryKeyString () {
        return Token.getPrimaryKeyString(cName, tokenSpaceName);
    }

    /**
      Judge datumType is valid
      
      @param type  The datumType want to be judged.
      
      @retval TRUE  - The type is valid.
      @retval FALSE - The type is invalid.
    **/
    public static boolean isValiddatumType(DATUM_TYPE type) {
        if ((type.ordinal() < DATUM_TYPE.UINT8.ordinal() ) || 
            (type.ordinal() > DATUM_TYPE.POINTER.ordinal())) {
            return false;
        }
        return true;
    }

    /**
      Judge pcdType is valid
      
      @param  type The PCdType want to be judged.
      
      @retval TRUE  - The type is valid.
      @retval FALSE - The type is invalid.
    **/
    public static boolean isValidpcdType(PCD_TYPE  type) {
        if ((type.ordinal() < PCD_TYPE.FEATURE_FLAG.ordinal() ) || 
            (type.ordinal() > PCD_TYPE.DYNAMIC_EX.ordinal())) {
            return false;
        }
        return true;
    }

    /**
      Add an usage instance for token
      
      @param usageInstance   The usage instance
     
      @retval TRUE  - Success to add usage instance.
      @retval FALSE - Fail to add usage instance
    **/
    public boolean addUsageInstance(UsageInstance usageInstance) throws EntityException {
        String exceptionStr;

        if (isUsageInstanceExist(usageInstance.usageId)) {
            exceptionStr = String.format("[PCD Collection Tool Exception] PCD %s for module %s has already exist in database, Please check all PCD build entries "+
                                         "in modules %s in <ModuleSA> to make sure no duplicated definitions in FPD file!",
                                         usageInstance.parentToken.cName,
                                         usageInstance.usageId.moduleName,
                                         usageInstance.usageId.moduleName);
            throw new EntityException(exceptionStr);
        }

        //
        // Put usage instance into usage instance database of this PCD token.
        // 
        consumers.put(usageInstance.getPrimaryKey(), usageInstance);

        return true;
    }

    /**
       Judge whether exist an usage instance for this token
       
       @param usageId       The UsageInstance identification for usage instance
       
       @return boolean      whether exist an usage instance for this token.
     */
    public boolean isUsageInstanceExist(UsageIdentification usageId) {
        String keyStr = UsageInstance.getPrimaryKey(usageId);

        return (consumers.get(keyStr) != null);
    }

    /**
      Get the PCD_TYPE according to the string of PCD_TYPE
      
      @param pcdTypeStr    The string of PCD_TYPE
      
      @return PCD_TYPE
    **/
    public static PCD_TYPE getpcdTypeFromString(String pcdTypeStr) {
        if (pcdTypeStr == null) {
            return PCD_TYPE.UNKNOWN;
        }

        if (pcdTypeStr.equalsIgnoreCase("FEATURE_FLAG")) {
            return PCD_TYPE.FEATURE_FLAG;
        } else if (pcdTypeStr.equalsIgnoreCase("FIXED_AT_BUILD")) {
            return PCD_TYPE.FIXED_AT_BUILD;
        } else if (pcdTypeStr.equalsIgnoreCase("PATCHABLE_IN_MODULE")) {
            return PCD_TYPE.PATCHABLE_IN_MODULE;
        } else if (pcdTypeStr.equalsIgnoreCase("DYNAMIC")) {
            return PCD_TYPE.DYNAMIC;
        } else if (pcdTypeStr.equalsIgnoreCase("DYNAMIC_EX")) {
            return PCD_TYPE.DYNAMIC_EX;
        } else {
            return PCD_TYPE.UNKNOWN;
        }
    }

    /**
      Get the string of given datumType. This string will be used for generating autogen files
     
      @param datumType   Given datumType
     
      @return The string of datum type.
    **/
    public static String getStringOfdatumType(DATUM_TYPE  datumType) {
        switch (datumType) {
        case UINT8:
            return "UINT8";
        case UINT16:
            return "UINT16";
        case UINT32:
            return "UINT32";
        case UINT64:
            return "UINT64";
        case POINTER:
            return "POINTER";
        case BOOLEAN:
            return "BOOLEAN";
        }
        return "UNKNOWN";
    }

    /**
      Get the datumType according to a string.
      
      @param datumTypeStr    The string of datumType
     
      @return DATUM_TYPE
    **/
    public static DATUM_TYPE getdatumTypeFromString(String datumTypeStr) {
        if (datumTypeStr.equalsIgnoreCase("UINT8")) {
            return DATUM_TYPE.UINT8;
        } else if (datumTypeStr.equalsIgnoreCase("UINT16")) {
            return DATUM_TYPE.UINT16;
        } else if (datumTypeStr.equalsIgnoreCase("UINT32")) {
            return DATUM_TYPE.UINT32;
        } else if (datumTypeStr.equalsIgnoreCase("UINT64")) {
            return DATUM_TYPE.UINT64;
        } else if (datumTypeStr.equalsIgnoreCase("VOID*")) {
            return DATUM_TYPE.POINTER;
        } else if (datumTypeStr.equalsIgnoreCase("BOOLEAN")) {
            return DATUM_TYPE.BOOLEAN;
        }
        return DATUM_TYPE.UNKNOWN;
    }

    /**
      Get string of given pcdType
      
      @param pcdType  The given PcdType
      
      @return The string of PCD_TYPE.
    **/
    public static String getStringOfpcdType(PCD_TYPE pcdType) {
        switch (pcdType) {
        case FEATURE_FLAG:
            return "FEATURE_FLAG";
        case FIXED_AT_BUILD:
            return "FIXED_AT_BUILD";
        case PATCHABLE_IN_MODULE:
            return "PATCHABLE_IN_MODULE";
        case DYNAMIC:
            return "DYNAMIC";
        case DYNAMIC_EX:
            return "DYNAMIC_EX";
        }
        return "UNKNOWN";
    }

    /**
      Get the PCD_USAGE according to a string
      
      @param usageStr  The string of PCD_USAGE
      
      @return The PCD_USAGE
    **/
    public static PCD_USAGE getUsageFromString(String usageStr) {
        if (usageStr == null) {
            return PCD_USAGE.UNKNOWN;
        }

        if (usageStr.equalsIgnoreCase("ALWAYS_PRODUCED")) {
            return PCD_USAGE.ALWAYS_PRODUCED;
        } else if (usageStr.equalsIgnoreCase("SOMETIMES_PRODUCED")) {
            return PCD_USAGE.SOMETIMES_PRODUCED;
        } else if (usageStr.equalsIgnoreCase("ALWAYS_CONSUMED")) {
            return PCD_USAGE.ALWAYS_CONSUMED;
        } else if (usageStr.equalsIgnoreCase("SOMETIMES_CONSUMED")) {
            return PCD_USAGE.SOMETIMES_CONSUMED;
        }

        return PCD_USAGE.UNKNOWN;
    }

    /**
      Get the string of given PCD_USAGE
      
      @param   usage   The given PCD_USAGE
      
      @return The string of PDC_USAGE.
    **/
    public static String getStringOfUsage(PCD_USAGE usage) {
        switch (usage) {
        case ALWAYS_PRODUCED:
            return "ALWAYS_PRODUCED";
        case ALWAYS_CONSUMED:
            return "ALWAYS_CONSUMED";
        case SOMETIMES_PRODUCED:
            return "SOMETIMES_PRODUCED";
        case SOMETIMES_CONSUMED:
            return "SOMETIMES_CONSUMED";
        }
        return "UNKNOWN";
    }

    /**
      Get the Defined datumType string for autogen. The string is for generating some MACROs in Autogen.h
      
      @param datumType The given datumType

      @return string of datum type for autogen.
    **/
    public static String GetAutogenDefinedatumTypeString(DATUM_TYPE datumType) {
        switch (datumType) {
        
        case UINT8:
            return "8";
        case UINT16:
            return "16";
        case BOOLEAN:
            return "BOOL";
        case POINTER:
            return "PTR";
        case UINT32:
            return "32";
        case UINT64:
            return "64";
        default:
            return null;
        }
    }

    /**
      Get the datumType String for Autogen. This string will be used for generating defintions of PCD token in autogen
      
      @param datumType   The given datumType

      @return string of datum type.
    **/

    public static String getAutogendatumTypeString(DATUM_TYPE datumType) {
        switch (datumType) {
        case UINT8:
            return "UINT8";
        case UINT16:
            return "UINT16";
        case UINT32:
            return "UINT32";
        case UINT64:
            return "UINT64";
        case POINTER:
            return "VOID*";
        case BOOLEAN:
            return "BOOLEAN";
        }
        return null;
    }

    /**
      Get the datumType string for generating some MACROs in autogen file of Library
      
      @param   datumType  The given datumType

      @return String of datum for genrating bit charater.
    **/
    public static String getAutogenLibrarydatumTypeString(DATUM_TYPE datumType) {
        switch (datumType) {
        case UINT8:
            return "8";
        case UINT16:
            return "16";
        case BOOLEAN:
            return "Bool";
        case POINTER:
            return "Ptr";
        case UINT32:
            return "32";
        case UINT64:
            return "64";
        default:
            return null;
        }
    }

    /**
       Get the sku data who id is 0.
       
       @retval DynamicTokenValue    the value of this dyanmic token.
    **/
    public DynamicTokenValue getDefaultSku() {
        DynamicTokenValue dynamicData;
        int               index;

        for (index = 0; index < this.skuData.size(); index ++) {
            if (skuData.get(index).id == 0) {
                return skuData.get(index).value;
            }
        }

        return null;
    }

    /**
       Get the number of Sku data for this token
       
       @retval int the number of sku data
    **/
    public int getSkuIdCount () {
        return this.skuData.size();
    }

    /**
       Get the size of PCD value, this PCD is POINTER type.
       
       @param str   the string of the value
       @param al    
    **/
    private void getCurrentSizeFromDefaultValue (String str, ArrayList<Integer> al) {
        if (isValidNullValue(str)) {
            al.add(new Integer(0));
        } else {
            //
            // isValidNullValue has already make sure that str here
            // always contain a valid default value of the following 3
            // cases:
            // 1) "Hello world" //Assci string
            // 2) L"Hello" //Unicode string
            // 3) {0x01, 0x02, 0x03} //Byte stream
            //
            if (str.startsWith("\"")) {
                al.add(new Integer(str.length() - 2));
            } else if (str.startsWith("L\"")){
                //
                // Unicode is 2 bytes each.
                //
                al.add(new Integer((str.length() - 3) * 2));
            } else if (str.startsWith("{")) {
                //
                // We count the number of "," in the string.
                // The number of byte is one plus the number of 
                // comma.
                //
                String str2 = str;
                
                int cnt = 0;
                int pos = 0;
                pos = str2.indexOf(",", 0);
                while (pos != -1) {
                    cnt++;
                    pos++;
                    pos = str2.indexOf(",", pos);
                }
                cnt++;
                al.add(new Integer(cnt));
            }
        }
    }

    /**
       This method can be used to get the MAX and current size
       for pointer type dynamic(ex) PCD entry
    **/ 
    public ArrayList<Integer> getPointerTypeSize () {
        ArrayList<Integer> al = new ArrayList<Integer>();
        
        //
        // For VPD_enabled and HII_enabled, we can only return the MAX size.
        // For the default DATA type dynamic PCD entry, we will return
        // the MAX size and current size for each SKU_ID.
        //
        al.add(new Integer(this.datumSize));
        
        if (!this.isVpdEnable()) {
            int idx;
            if (this.isHiiEnable()){
                for (idx = 0; idx < this.skuData.size(); idx++) {
                    String str = this.skuData.get(idx).value.hiiDefaultValue;
                    getCurrentSizeFromDefaultValue(str, al);
                }
            } else {
                for (idx = 0; idx < this.skuData.size(); idx++) {
                    String str = this.skuData.get(idx).value.value;
                    getCurrentSizeFromDefaultValue(str, al);
                }
            }
        }
        
        return al;
    }

    /**
       Get default value for a token, For HII type, HiiDefaultValue of default
       SKU 0 will be returned; For Default type, the defaultvalue of default SKU
       0 will be returned.
       
       @return String
     */
    public String getDynamicDefaultValue() {
        DynamicTokenValue dynamicData = getDefaultSku();
        if (hasDefaultValue()) {
            switch (dynamicData.type) {
            case DEFAULT_TYPE:
                return dynamicData.value;
            }
        }

        return null;
    }

    //
    // BugBug: We need change this algorithm accordingly when schema is updated
    //          to support no default value.
    //
    public boolean hasDefaultValue () {
        int               value         = 0;
        boolean           isInteger     = true;
        DynamicTokenValue dynamicValue  = null;

        if (isSkuEnable()) {
            return true;
        }
        
        if (this.isDynamicPCD) {
            dynamicValue = getDefaultSku();
            switch (dynamicValue.type) {
            case HII_TYPE:
                return true;
            case VPD_TYPE:
                return true;
            case DEFAULT_TYPE:
                return !isValidNullValue(dynamicValue.value);
            }
        }

        return false;
    }

    /**
       Judge the value is NULL value. NULL value means the value is uninitialized value
       
       @param judgedValue
       
       @return boolean
     */
    public boolean isValidNullValue(String judgedValue) {
        String      subStr;
        BigInteger  bigIntValue;

        switch (datumType) {
        case UINT8:
        case UINT16:
        case UINT32:
            if (judgedValue.length() > 2) {
                if ((judgedValue.charAt(0) == '0')        && 
                    ((judgedValue.charAt(1) == 'x') || (judgedValue.charAt(1) == 'X'))){
                    subStr      = judgedValue.substring(2, judgedValue.length());
                    bigIntValue = new BigInteger(subStr, 16);
                } else {
                    bigIntValue = new BigInteger(judgedValue);
                }
            } else {
                bigIntValue = new BigInteger(judgedValue);
            }
            if (bigIntValue.bitCount() == 0) {
                return true;
            }
            break;
        case UINT64:
            if (judgedValue.length() > 2){
                if ((judgedValue.charAt(0) == '0') && 
                    ((judgedValue.charAt(1) == 'x') ||
                     (judgedValue.charAt(1) == 'X'))) {
                    bigIntValue = new BigInteger(judgedValue.substring(2, judgedValue.length()),  16);
                    if (bigIntValue.bitCount() == 0) {
                        return true;
                    }
                } else {
                    bigIntValue = new BigInteger(judgedValue);
                    if (bigIntValue.bitCount() == 0) {
                        return true;
                    }
                }
            } else  {
                bigIntValue = new BigInteger(judgedValue);
                if (bigIntValue.bitCount() == 0) {
                    return true;
                }
            }
            break;
        case BOOLEAN:
            if (judgedValue.equalsIgnoreCase("false")) {
                return true;
            }
            break;
        case POINTER:
            if (judgedValue.equalsIgnoreCase("")        ||
                judgedValue.equalsIgnoreCase("\"\"")   ||
                judgedValue.equalsIgnoreCase("L\"\"")   ||
                (judgedValue.length() == 0)             ||
                judgedValue.equalsIgnoreCase("{0}")) {
                return true;
            }
        }
        return false;
    }

    /**
       Is the string value in Unicode
       
       @return boolean
    **/
    public boolean isHiiDefaultValueUnicodeStringType() {
        DynamicTokenValue dynamicData = getDefaultSku();
        
        if (dynamicData == null)
            return false;
        
        return dynamicData.hiiDefaultValue.startsWith("L\"")
                && dynamicData.hiiDefaultValue.endsWith("\"");
    }

    /**
       Is the string value in ANSCI
       
       @return boolean
    **/
    public boolean isHiiDefaultValueASCIIStringType() {
        DynamicTokenValue dynamicData = getDefaultSku();
    
        if (dynamicData == null)
            return false;
        
        return dynamicData.hiiDefaultValue.startsWith("\"")
        && dynamicData.hiiDefaultValue.endsWith("\"");
    }
    
    /**
       Judege whether current value is UNICODE string type.
       @return boolean
     */
    public boolean isUnicodeStringType () {
        String str = getDynamicDefaultValue();

        if (str == null) {
            return false;
        }

        if (datumType == Token.DATUM_TYPE.POINTER &&
            str.startsWith("L\"") && 
            str.endsWith("\"")) {
            return true;
        }

        return false;
    }
    
    public boolean isASCIIStringType () {
        String str = getDynamicDefaultValue();

        if (str == null) {
            return false;
        }

        if (datumType == Token.DATUM_TYPE.POINTER &&
            str.startsWith("\"") && 
            str.endsWith("\"")) {
            return true;
        }

        return false;
    }

    public boolean isByteStreamType () {
        String str = getDynamicDefaultValue();

        if (str == null) {
            return false;
        }

        if (datumType == Token.DATUM_TYPE.POINTER &&
            str.startsWith("{") && 
            str.endsWith("}")) {
            return true;
        }

        return false;
        
    }

    public String getStringTypeString () {                       
        return getDefaultSku().value.substring(2, getDefaultSku().value.length() - 1);
    }
}




