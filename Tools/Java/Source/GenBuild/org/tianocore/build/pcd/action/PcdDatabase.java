/** @file
  PcdDatabase class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.pcd.action;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.tianocore.pcd.entity.DynamicTokenValue;
import org.tianocore.pcd.entity.Token;
import org.tianocore.pcd.exception.EntityException;

/**
    CStructTypeDeclaration

    This class is used to store the declaration string, such as
    "UINT32 PcdPlatformFlashBaseAddress", of
    each memember in the C structure, which is a standard C language
    feature used to implement a simple and efficient database for
    dynamic(ex) type PCD entry.
**/
class CStructTypeDeclaration {
    String key;
    int alignmentSize;
    String cCode;
    boolean initTable;

    public CStructTypeDeclaration (String key, int alignmentSize, String cCode, boolean initTable) {
        this.key = key;
        this.alignmentSize = alignmentSize;
        this.cCode = cCode;
        this.initTable = initTable;
    }
}

/**
    StringTable

    This class is used to store the String in a PCD database.

**/
class StringTable {
    class UnicodeString {
        //
        // In Schema, we define VariableName in DynamicPcdBuildDefinitions in FPD
        // file to be HexWordArrayType. For example, Unicode String L"Setup" is 
        // <VariableName>0x0053 0x0065 0x0074 0x0075 0x0070</VariableName>. 
        // We use raw to differentiate if the String is in form of L"Setup" (raw is false) or
        // in form of {0x0053, 0x0065, 0x0074, 0x0075, 0x0070}
        //
        // This str is the string that can be pasted directly into the C structure. 
        // For example, this str can be two forms:
        //      
        //      L"Setup",
        //      {0x0053, 0065, 0x0074, 0x0075, 0x0070, 0x0000}, //This is another form of L"Setup"
        //
        public String      str;
        //
        // This len includes the NULL character at the end of the String.
        //
        public int         len;
        
        public UnicodeString (String str, int len) {
            this.str = str;
            this.len = len;
        }
    }
    
    private ArrayList<UnicodeString>   al;
    private ArrayList<String>   alComments;
    private String              phase;
    int                         stringTableCharNum;

    public StringTable (String phase) {
        this.phase = phase;
        al = new ArrayList<UnicodeString>();
        alComments = new ArrayList<String>();
        stringTableCharNum = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.StringTableSizeMacro, phase, getSize());
    }

    private int getSize () {
        //
        // We have at least one Unicode Character in the table.
        //
        return stringTableCharNum == 0 ? 1 : stringTableCharNum;
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.StringTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public void genCode (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable) {
        final String stringTable = "StringTable";
        final String tab         = "\t";
        final String newLine     = "\r\n";
        final String commaNewLine = ",\r\n";

        CStructTypeDeclaration decl;

        String cDeclCode = "";
        String cInstCode = "";

        //
        // If we have a empty StringTable
        //
        if (al.size() == 0) {
            cDeclCode += String.format("%-20s%s[1]; /* StringTable is empty */", "UINT16", stringTable) + newLine;
            decl = new CStructTypeDeclaration (
                                                stringTable,
                                                2,
                                                cDeclCode,
                                                true
                                        );
            declaList.add(decl);

            cInstCode = String.format("/* %s */", stringTable) + newLine + tab + "{ 0 }";
            instTable.put(stringTable, cInstCode);
        } else {

            //
            // If there is any String in the StringTable
            //
            for (int i = 0; i < al.size(); i++) {
                UnicodeString uStr = al.get(i);
                String stringTableName;

                if (i == 0) {
                    //
                    // StringTable is a well-known name in the PCD DXE driver
                    //
                    stringTableName = stringTable;

                } else {
                    stringTableName = String.format("%s_%d", stringTable, i);
                    cDeclCode += tab;
                }
                cDeclCode += String.format("%-20s%s[%d]; /* %s */", "UINT16",
                                           stringTableName, uStr.len,
                                           alComments.get(i))
                             + newLine;

                if (i == 0) {
                    cInstCode = "/* StringTable */" + newLine;
                }

                cInstCode += tab + String.format("%s /* %s */", uStr.str, alComments.get(i));
                if (i != al.size() - 1) {
                    cInstCode += commaNewLine;
                }
            }

            decl = new CStructTypeDeclaration (
                    stringTable,
                    2,
                    cDeclCode,
                    true
            );
            declaList.add(decl);

            instTable.put(stringTable, cInstCode);
        }
    }
    
    public int add (List inputStr, Token token) {
        String str;
        
        str = "{";
        
        for (int i = 0; i < inputStr.size(); i++) {
            str += " " + inputStr.get(i) + ",";
        }
        
        str +=  " 0x0000";
            
        str += "}";
        //
        // This is a raw Unicode String
        //
        return addToTable (str, inputStr.size() + 1, token);
    }

    public int add (String inputStr, Token token) {

        int len;
        String str = inputStr;

        //
        // The input can be two types:
        // "L\"Bootmode\"" or "Bootmode".
        // We drop the L\" and \" for the first type.
        if (str.startsWith("L\"") && str.endsWith("\"")) {
            //
            // Substract the character of "L", """, """.
            // and add in the NULL character. So it is 2.
            //
            len = str.length() - 2;
        } else {
            //
            // Include the NULL character.
            //
            len = str.length() + 1;
            str = "L\"" + str + "\"";
        }
        
        //
        // After processing, this is L"A String Sample" type of string.
        //
        return addToTable (str, len, token);
    }
    
    private int addToTable (String inputStr, int len, Token token) {
        int i;
        int pos;

        //
        // Check if StringTable has this String already.
        // If so, return the current pos.
        //
        for (i = 0, pos = 0; i < al.size(); i++) {
            UnicodeString s = al.get(i);;

            if (inputStr.equals(s.str)) {
                return pos;
            }
            pos += s.len;
        }

        i = stringTableCharNum;
        //
        // Include the NULL character at the end of String
        //
        stringTableCharNum += len;
        al.add(new UnicodeString(inputStr, len));
        alComments.add(token.getPrimaryKeyString());

        return i;
    }
}

/**
    SizeTable

    This class is used to store the Size information for
    POINTER TYPE PCD entry in a PCD database.

**/
class SizeTable {
    private ArrayList<ArrayList<Integer>>  al;
    private ArrayList<String>   alComments;
    private int                 len;
    private String              phase;

    public SizeTable (String phase) {
        al = new ArrayList<ArrayList<Integer>>();
        alComments = new ArrayList<String>();
        len = 0;
        this.phase = phase;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.SizeTableSizeMacro, phase, getSize());
    }

    private int getSize() {
        return len == 0 ? 1 : len;
    }

    public void genCode (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
        final String name = "SizeTable";

        CStructTypeDeclaration decl;
        String cCode;

        cCode = String.format(PcdDatabase.SizeTableDeclaration, phase);
        decl = new CStructTypeDeclaration (
                                            name,
                                            2,
                                            cCode,
                                            true
                                           );
        declaList.add(decl);


        cCode = PcdDatabase.genInstantiationStr(getInstantiation());
        instTable.put(name, cCode);
    }

    private ArrayList<String> getInstantiation () {
        final String comma   = ",";
        ArrayList<String> Output = new ArrayList<String>();

        Output.add("/* SizeTable */");
        Output.add("{");
        if (al.size() == 0) {
            Output.add("\t0");
        } else {
            for (int index = 0; index < al.size(); index++) {
                ArrayList<Integer> ial = al.get(index);

                String str = "\t";

                for (int index2 = 0; index2 < ial.size(); index2++) {
                    str += " " + ial.get(index2).toString();
                    if (index2 != ial.size() - 1) {
                        str += comma;
                    }
                }

                str += " /* " + alComments.get(index) + " */";

                if (index != (al.size() - 1)) {
                    str += comma;
                }

                Output.add(str);

            }
        }
        Output.add("}");

        return Output;
    }

    public void add (Token token) {

        //
        // We only have size information for POINTER type PCD entry.
        //
        if (token.datumType != Token.DATUM_TYPE.POINTER) {
            return;
        }

        ArrayList<Integer> ial = token.getPointerTypeSize();

        len+= ial.size();

        al.add(ial);
        alComments.add(token.getPrimaryKeyString());

        return;
    }

}

/**
    GuidTable

    This class is used to store the GUIDs in a PCD database.
**/
class GuidTable {
    private ArrayList<UUID> al;
    private ArrayList<String> alComments;
    private String          phase;
    private int             len;
    private int             bodyLineNum;

    public GuidTable (String phase) {
        this.phase = phase;
        al = new ArrayList<UUID>();
        alComments = new ArrayList<String>();
        len = 0;
        bodyLineNum = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.GuidTableSizeMacro, phase, getSize());
    }

    private int getSize () {
        return (al.size() == 0)? 1 : al.size();
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.GuidTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public void genCode (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
        final String name = "GuidTable";

        CStructTypeDeclaration decl;
        String cCode = "";

        cCode += String.format(PcdDatabase.GuidTableDeclaration, phase);
        decl = new CStructTypeDeclaration (
                                            name,
                                            4,
                                            cCode,
                                            true
                                           );
        declaList.add(decl);


        cCode = PcdDatabase.genInstantiationStr(getInstantiation());
        instTable.put(name, cCode);
    }

    private String getUuidCString (UUID uuid) {
        String[]  guidStrArray;

        guidStrArray =(uuid.toString()).split("-");

        return String.format("{0x%s, 0x%s, 0x%s, {0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s}}",
                                         guidStrArray[0],
                                         guidStrArray[1],
                                         guidStrArray[2],
                                        (guidStrArray[3].substring(0, 2)),
                                        (guidStrArray[3].substring(2, 4)),
                                        (guidStrArray[4].substring(0, 2)),
                                        (guidStrArray[4].substring(2, 4)),
                                        (guidStrArray[4].substring(4, 6)),
                                        (guidStrArray[4].substring(6, 8)),
                                        (guidStrArray[4].substring(8, 10)),
                                        (guidStrArray[4].substring(10, 12))
                                        );
    }

    private ArrayList<String> getInstantiation () {
        ArrayList<String> Output = new ArrayList<String>();

        Output.add("/* GuidTable */");
        Output.add("{");

        if (al.size() == 0) {
            Output.add("\t" + getUuidCString(new UUID(0, 0)));
        }

        for (int i = 0; i < al.size(); i++) {
            String str = "\t" + getUuidCString(al.get(i));

            str += "/* " + alComments.get(i) +  " */";
            if (i != (al.size() - 1)) {
                str += ",";
            }
            Output.add(str);
            bodyLineNum++;

        }
        Output.add("}");

        return Output;
    }

    public int add (UUID uuid, String name) {
        //
        // Check if GuidTable has this entry already.
        // If so, return the GuidTable index.
        //
        for (int i = 0; i < al.size(); i++) {
            if (al.get(i).compareTo(uuid) == 0) {
                return i;
            }
        }

        len++;
        al.add(uuid);
        alComments.add(name);

        //
        // Return the previous Table Index
        //
        return len - 1;
    }

}

/**
    SkuIdTable

    This class is used to store the SKU IDs in a PCD database.

**/
class SkuIdTable {
    private ArrayList<Integer[]> al;
    private ArrayList<String>    alComment;
    private String               phase;
    private int                  len;

    public SkuIdTable (String phase) {
        this.phase = phase;
        al = new ArrayList<Integer[]>();
        alComment = new ArrayList<String>();
        len = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.SkuIdTableSizeMacro, phase, getSize());
    }

    private int getSize () {
        return (len == 0)? 1 : len;
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.SkuTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public void genCode (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
        final String name = "SkuIdTable";

        CStructTypeDeclaration decl;
        String cCode = "";

        cCode += String.format(PcdDatabase.SkuIdTableDeclaration, phase);
        decl = new CStructTypeDeclaration (
                                            name,
                                            1,
                                            cCode,
                                            true
                                           );
        declaList.add(decl);


        cCode = PcdDatabase.genInstantiationStr(getInstantiation());
        instTable.put(name, cCode);

        //
        // SystemSkuId is in PEI phase PCD Database
        //
        if (phase.equalsIgnoreCase("PEI")) {
            decl = new CStructTypeDeclaration (
                                                "SystemSkuId",
                                                1,
                                                String.format("%-20sSystemSkuId;\r\n", "SKU_ID"),
                                                true
                                              );
            declaList.add(decl);

            instTable.put("SystemSkuId", "0");
        }

    }

    private ArrayList<String> getInstantiation () {
        ArrayList<String> Output = new ArrayList<String> ();

        Output.add("/* SkuIdTable */");
        Output.add("{");

        if (al.size() == 0) {
            Output.add("\t0");
        }

        for (int index = 0; index < al.size(); index++) {
            String str;

            str = "/* " + alComment.get(index) + "*/ ";
            str += "/* MaxSku */ ";


            Integer[] ia = al.get(index);

            str += "\t" + ia[0].toString() + ", ";
            for (int index2 = 1; index2 < ia.length; index2++) {
               str += ia[index2].toString();
               if (!((index2 == ia.length - 1) && (index == al.size() - 1))) {
                   str += ", ";
               }
            }

            Output.add(str);

        }

        Output.add("}");

        return Output;
    }

    public int add (Token token) {

        int index;
        int pos;

        //
        // Check if this SKU_ID Array is already in the table
        //
        pos = 0;
        for (Object o: al) {
            Integer [] s = (Integer[]) o;
            boolean different = false;
            if (s[0] == token.getSkuIdCount()) {
                for (index = 1; index < s.length; index++) {
                    if (s[index] != token.skuData.get(index-1).id) {
                        different = true;
                        break;
                    }
                }
            } else {
                different = true;
            }
            if (different) {
                pos += s[0] + 1;
            } else {
                return pos;
            }
        }

        Integer [] skuIds = new Integer[token.skuData.size() + 1];
        skuIds[0] = new Integer(token.skuData.size());
        for (index = 1; index < skuIds.length; index++) {
            skuIds[index] = new Integer(token.skuData.get(index - 1).id);
        }

        index = len;

        len += skuIds.length;
        al.add(skuIds);
        alComment.add(token.getPrimaryKeyString());

        return index;
    }

}

class LocalTokenNumberTable {
    private ArrayList<String>    al;
    private ArrayList<String>    alComment;
    private String               phase;
    private int                  len;

    public LocalTokenNumberTable (String phase) {
        this.phase = phase;
        al = new ArrayList<String>();
        alComment = new ArrayList<String>();

        len = 0;
    }

    public String getSizeMacro () {
    	return String.format(PcdDatabase.LocalTokenNumberTableSizeMacro, phase, getSize())
    			+ String.format(PcdDatabase.LocalTokenNumberSizeMacro, phase, al.size());
    }

    public int getSize () {
        return (al.size() == 0)? 1 : al.size();
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.DatabaseExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public void genCode (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
        final String name = "LocalTokenNumberTable";

        CStructTypeDeclaration decl;
        String cCode = "";

        cCode += String.format(PcdDatabase.LocalTokenNumberTableDeclaration, phase);
        decl = new CStructTypeDeclaration (
                                            name,
                                            4,
                                            cCode,
                                            true
                                           );
        declaList.add(decl);

        cCode = PcdDatabase.genInstantiationStr(getInstantiation());
        instTable.put(name, cCode);
    }

    private ArrayList<String> getInstantiation () {
        ArrayList<String> output = new ArrayList<String>();

        output.add("/* LocalTokenNumberTable */");
        output.add("{");

        if (al.size() == 0) {
            output.add("\t0");
        }

        for (int index = 0; index < al.size(); index++) {
            String str;

            str = "\t" + (String)al.get(index);

            str += " /* " + alComment.get(index) + " */ ";


            if (index != (al.size() - 1)) {
                str += ",";
            }

            output.add(str);

        }

        output.add("}");

        return output;
    }

    public int add (Token token) {
        int index = len;
        String str;

        len++;

        str =  String.format(PcdDatabase.offsetOfStrTemplate, phase, token.hasDefaultValue() ? "Init" : "Uninit", token.getPrimaryKeyString());

        if (token.isUnicodeStringType()) {
            str += " | PCD_TYPE_STRING";
        }

        if (token.isSkuEnable()) {
            str += " | PCD_TYPE_SKU_ENABLED";
        }

        if (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.HII_TYPE) {
            str += " | PCD_TYPE_HII";
        }

        if (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.VPD_TYPE) {
            str += " | PCD_TYPE_VPD";
        }

        switch (token.datumType) {
        case UINT8:
        case BOOLEAN:
            str += " | PCD_DATUM_TYPE_UINT8";
            break;
        case UINT16:
            str += " | PCD_DATUM_TYPE_UINT16";
            break;
        case UINT32:
            str += " | PCD_DATUM_TYPE_UINT32";
            break;
        case UINT64:
            str += " | PCD_DATUM_TYPE_UINT64";
            break;
        case POINTER:
            str += " | PCD_DATUM_TYPE_POINTER";
            break;
        }

        al.add(str);
        alComment.add(token.getPrimaryKeyString());

        return index;
    }
}

/**
    ExMapTable

    This class is used to store the table of mapping information
    between DynamicEX ID pair(Guid, TokenNumber) and
    the local token number assigned by PcdDatabase class.
**/
class ExMapTable {

    /**
        ExTriplet

        This class is used to store the mapping information
        between DynamicEX ID pair(Guid, TokenNumber) and
        the local token number assigned by PcdDatabase class.
    **/
    class ExTriplet {
        public Integer guidTableIdx;
        public Long exTokenNumber;
        public Long localTokenIdx;

        public ExTriplet (int guidTableIdx, long exTokenNumber, long localTokenIdx) {
            this.guidTableIdx = new Integer(guidTableIdx);
            this.exTokenNumber = new Long(exTokenNumber);
            this.localTokenIdx = new Long(localTokenIdx);
        }
    }

    private ArrayList<ExTriplet> al;
    private Map<ExTriplet, String> alComment;
    private String               phase;
    private int                  len;
    private int                   bodyLineNum;

    public ExMapTable (String phase) {
        this.phase = phase;
        al = new ArrayList<ExTriplet>();
        alComment = new HashMap<ExTriplet, String>();
        bodyLineNum = 0;
        len = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.ExMapTableSizeMacro, phase, getTableLen())
             + String.format(PcdDatabase.ExTokenNumber, phase, al.size());
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.ExMapTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public void genCode (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
        final String exMapTableName = "ExMapTable";

        sortTable();

        CStructTypeDeclaration decl;
        String cCode = "";

        cCode += String.format(PcdDatabase.ExMapTableDeclaration, phase);
        decl = new CStructTypeDeclaration (
                                            exMapTableName,
                                            4,
                                            cCode,
                                            true
                                           );
        declaList.add(decl);


        cCode = PcdDatabase.genInstantiationStr(getInstantiation());
        instTable.put(exMapTableName, cCode);
    }

    private ArrayList<String> getInstantiation () {
        ArrayList<String> Output = new ArrayList<String>();

        Output.add("/* ExMapTable */");
        Output.add("{");
        if (al.size() == 0) {
            Output.add("\t{0, 0, 0}");
        }

        int index;
        for (index = 0; index < al.size(); index++) {
            String str;

            ExTriplet e = (ExTriplet)al.get(index);

            str = "\t" + "{ " + String.format("0x%08X", e.exTokenNumber) + ", ";
            str += e.localTokenIdx.toString() + ", ";
            str += e.guidTableIdx.toString();

            str += "}" + " /* " + alComment.get(e) + " */" ;

            if (index != al.size() - 1) {
                str += ",";
            }

            Output.add(str);
            bodyLineNum++;

        }

        Output.add("}");

        return Output;
    }

    public int add (int localTokenIdx, long exTokenNum, int guidTableIdx, String name) {
        int index = len;

        len++;
        ExTriplet et = new ExTriplet(guidTableIdx, exTokenNum, localTokenIdx);

        al.add(et);
        alComment.put(et, name);

        return index;
    }

    private int getTableLen () {
        return al.size() == 0 ? 1 : al.size();
    }

    //
    // To simplify the algorithm for GetNextToken and GetNextTokenSpace in
    // PCD PEIM/Driver, we need to sort the ExMapTable according to the
    // following order:
    // 1) ExGuid
    // 2) ExTokenNumber
    //
    class ExTripletComp implements Comparator<ExTriplet> {
        public int compare (ExTriplet a, ExTriplet b) {
            if (a.guidTableIdx == b.guidTableIdx ) {
                //
                // exTokenNumber is long, we can't use simple substraction.
                //
                if (a.exTokenNumber > b.exTokenNumber) {
                    return 1;
                } else if (a.exTokenNumber == b.exTokenNumber) {
                    return 0;
                } else {
                    return -1;
                }
            }

            return a.guidTableIdx - b.guidTableIdx;
        }
    }

    private void sortTable () {
        java.util.Comparator<ExTriplet> comparator = new ExTripletComp();
        java.util.Collections.sort(al, comparator);
    }
}

/**
    PcdDatabase

    This class is used to generate C code for Autogen.h and Autogen.c of
    a PCD service DXE driver and PCD service PEIM.
**/
public class PcdDatabase {

    private final static int    SkuHeadAlignmentSize             = 4;
    private final String        newLine                         = "\r\n";
    private final String        commaNewLine                    = ",\r\n";
    private final String        tab                             = "\t";
    public final static String ExMapTableDeclaration            = "DYNAMICEX_MAPPING   ExMapTable[%s_EXMAPPING_TABLE_SIZE];\r\n";
    public final static String GuidTableDeclaration             = "EFI_GUID            GuidTable[%s_GUID_TABLE_SIZE];\r\n";
    public final static String LocalTokenNumberTableDeclaration = "UINT32              LocalTokenNumberTable[%s_LOCAL_TOKEN_NUMBER_TABLE_SIZE];\r\n";
    public final static String StringTableDeclaration           = "UINT16              StringTable[%s_STRING_TABLE_SIZE];\r\n";
    public final static String SizeTableDeclaration             = "SIZE_INFO           SizeTable[%s_SIZE_TABLE_SIZE];\r\n";
    public final static String SkuIdTableDeclaration            = "UINT8               SkuIdTable[%s_SKUID_TABLE_SIZE];\r\n";


    public final static String ExMapTableSizeMacro              = "#define %s_EXMAPPING_TABLE_SIZE  %d\r\n";
    public final static String ExTokenNumber                    = "#define %s_EX_TOKEN_NUMBER       %d\r\n";
    public final static String GuidTableSizeMacro               = "#define %s_GUID_TABLE_SIZE         %d\r\n";
    public final static String LocalTokenNumberTableSizeMacro   = "#define %s_LOCAL_TOKEN_NUMBER_TABLE_SIZE            %d\r\n";
    public final static String LocalTokenNumberSizeMacro   		= "#define %s_LOCAL_TOKEN_NUMBER            %d\r\n";
    public final static String SizeTableSizeMacro               = "#define %s_SIZE_TABLE_SIZE            %d\r\n";
    public final static String StringTableSizeMacro             = "#define %s_STRING_TABLE_SIZE       %d\r\n";
    public final static String SkuIdTableSizeMacro              = "#define %s_SKUID_TABLE_SIZE        %d\r\n";


    public final static String ExMapTableExistenceMacro         = "#define %s_EXMAP_TABLE_EMPTY    %s\r\n";
    public final static String GuidTableExistenceMacro          = "#define %s_GUID_TABLE_EMPTY     %s\r\n";
    public final static String DatabaseExistenceMacro           = "#define %s_DATABASE_EMPTY       %s\r\n";
    public final static String StringTableExistenceMacro        = "#define %s_STRING_TABLE_EMPTY   %s\r\n";
    public final static String SkuTableExistenceMacro           = "#define %s_SKUID_TABLE_EMPTY    %s\r\n";

    public final static String offsetOfSkuHeadStrTemplate       = "offsetof(%s_PCD_DATABASE, %s.%s_SkuDataTable)";
    public final static String offsetOfVariableEnabledDefault   = "offsetof(%s_PCD_DATABASE, %s.%s_VariableDefault_%d)";
    public final static String offsetOfStrTemplate              = "offsetof(%s_PCD_DATABASE, %s.%s)";

    private final static String  skuDataTableTemplate           = "SkuDataTable";


    private StringTable stringTable;
    private GuidTable   guidTable;
    private LocalTokenNumberTable localTokenNumberTable;
    private SkuIdTable  skuIdTable;
    private SizeTable   sizeTable;
    private ExMapTable  exMapTable;

    private ArrayList<Token> alTokens;
    private String phase;
    private int assignedTokenNumber;

    //
    // Use two class global variable to store
    // temperary
    //
    private String      privateGlobalName;
    private String      privateGlobalCCode;
    //
    // After Major changes done to the PCD
    // database generation class PcdDatabase
    // Please increment the version and please
    // also update the version number in PCD
    // service PEIM and DXE driver accordingly.
    //
    private final int version = 2;

    private String hString;
    private String cString;

    /**
        Constructor for PcdDatabase class.

        <p>We have two PCD dynamic(ex) database for the Framework implementation. One
        for PEI phase and the other for DXE phase.  </p>

        @param alTokens A ArrayList of Dynamic(EX) PCD entry.
        @param exePhase The phase to generate PCD database for: valid input
                        is "PEI" or "DXE".
        @param startLen The starting Local Token Number for the PCD database. For
                        PEI phase, the starting Local Token Number starts from 0.
                        For DXE phase, the starting Local Token Number starts
                        from the total number of PCD entry of PEI phase.
        @return void
    **/
    public PcdDatabase (ArrayList<Token> alTokens, String exePhase, int startLen) {
       phase = exePhase;

       stringTable = new StringTable(phase);
       guidTable = new GuidTable(phase);
       localTokenNumberTable = new LocalTokenNumberTable(phase);
       skuIdTable = new SkuIdTable(phase);
       sizeTable = new SizeTable(phase);
       exMapTable = new ExMapTable(phase);

       //
       // Local token number 0 is reserved for INVALID_TOKEN_NUMBER.
       // So we will increment 1 for the startLen passed from the
       // constructor.
       //
       assignedTokenNumber = startLen + 1;
       this.alTokens = alTokens;
    }

    private void getNonExAndExTokens (ArrayList<Token> alTokens, List<Token> nexTokens, List<Token> exTokens) {
        for (int i = 0; i < alTokens.size(); i++) {
            Token t = (Token)alTokens.get(i);
            if (t.isDynamicEx()) {
                exTokens.add(t);
            } else {
                nexTokens.add(t);
            }
        }

        return;
    }

    private int getDataTypeAlignmentSize (Token token) {
        switch (token.datumType) {
        case UINT8:
            return 1;
        case UINT16:
            return 2;
        case UINT32:
            return 4;
        case UINT64:
            return 8;
        case POINTER:
            return 1;
        case BOOLEAN:
            return 1;
        default:
            return 1;
        }
    }

    private int getHiiPtrTypeAlignmentSize(Token token) {
        switch (token.datumType) {
        case UINT8:
            return 1;
        case UINT16:
            return 2;
        case UINT32:
            return 4;
        case UINT64:
            return 8;
        case POINTER:
            if (token.isHiiEnable()) {
                if (token.isHiiDefaultValueUnicodeStringType()) {
                    return 2;
                }
            }
            return 1;
        case BOOLEAN:
            return 1;
        default:
            return 1;
        }
    }

    private int getAlignmentSize (Token token) {
        if (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.HII_TYPE) {
            return 2;
        }

        if (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.VPD_TYPE) {
            return 4;
        }

        if (token.isUnicodeStringType()) {
            return 2;
        }

        return getDataTypeAlignmentSize(token);
     }

    public String getCString () {
        return cString;
    }

    public String getHString () {
        return hString;
    }

    private void genCodeWorker(Token t,
            ArrayList<CStructTypeDeclaration> declaList,
            HashMap<String, String> instTable, String phase)
            throws EntityException {

        CStructTypeDeclaration decl;

        //
        // Insert SKU_HEAD if isSkuEnable is true
        //
        if (t.isSkuEnable()) {
            int tableIdx;
            tableIdx = skuIdTable.add(t);
            decl = new CStructTypeDeclaration(t.getPrimaryKeyString(),
                    SkuHeadAlignmentSize, getSkuEnabledTypeDeclaration(t), true);
            declaList.add(decl);
            instTable.put(t.getPrimaryKeyString(),
                    getSkuEnabledTypeInstantiaion(t, tableIdx));
        }

        //
        // Insert PCD_ENTRY declaration and instantiation
        //
        getCDeclarationString(t);

        decl = new CStructTypeDeclaration(privateGlobalName,
                getAlignmentSize(t), privateGlobalCCode, t.hasDefaultValue());
        declaList.add(decl);

        if (t.hasDefaultValue()) {
            instTable.put(privateGlobalName,
                          getTypeInstantiation(t, declaList, instTable, phase)
                          );
        }

    }

    private void ProcessTokens (List<Token> tokens,
                                   ArrayList<CStructTypeDeclaration> cStructDeclList,
                                   HashMap<String, String> cStructInstTable,
                                   String phase
                                   )
    throws EntityException {

        for (int idx = 0; idx < tokens.size(); idx++) {
            Token t = tokens.get(idx);

            genCodeWorker (t, cStructDeclList, cStructInstTable, phase);

            sizeTable.add(t);
            localTokenNumberTable.add(t);
            t.tokenNumber = assignedTokenNumber++;

            //
            // Add a mapping if this dynamic PCD entry is a EX type
            //
            if (t.isDynamicEx()) {
                exMapTable.add((int)t.tokenNumber,
                                t.dynamicExTokenNumber,
                                guidTable.add(translateSchemaStringToUUID(t.tokenSpaceName), t.getPrimaryKeyString()),
                                t.getPrimaryKeyString()
                                );
            }
        }

    }

    public void genCode () throws EntityException {

        ArrayList<CStructTypeDeclaration> cStructDeclList = new ArrayList<CStructTypeDeclaration>();
        HashMap<String, String> cStructInstTable = new HashMap<String, String>();

        List<Token> nexTokens = new ArrayList<Token> ();
        List<Token> exTokens = new ArrayList<Token> ();

        getNonExAndExTokens (alTokens, nexTokens, exTokens);

        //
        // We have to process Non-Ex type PCD entry first. The reason is
        // that our optimization assumes that the Token Number of Non-Ex
        // PCD entry start from 1 (for PEI phase) and grows continously upwards.
        //
        // EX type token number starts from the last Non-EX PCD entry and
        // grows continously upwards.
        //
        ProcessTokens (nexTokens, cStructDeclList, cStructInstTable, phase);
        ProcessTokens (exTokens, cStructDeclList, cStructInstTable, phase);

        stringTable.genCode(cStructDeclList, cStructInstTable);
        skuIdTable.genCode(cStructDeclList, cStructInstTable, phase);
        exMapTable.genCode(cStructDeclList, cStructInstTable, phase);
        localTokenNumberTable.genCode(cStructDeclList, cStructInstTable, phase);
        sizeTable.genCode(cStructDeclList, cStructInstTable, phase);
        guidTable.genCode(cStructDeclList, cStructInstTable, phase);

        hString = genCMacroCode ();

        HashMap <String, String> result;

        result = genCStructCode(cStructDeclList,
                cStructInstTable,
                phase
                );

        hString += result.get("initDeclStr");
        hString += result.get("uninitDeclStr");

        hString += String.format("#define PCD_%s_SERVICE_DRIVER_AUTOGEN_VERSION         %d", phase, version);

        cString = newLine + newLine + result.get("initInstStr");

    }

    private String genCMacroCode () {
        String macroStr   = "";

        //
        // Generate size info Macro for all Tables
        //
        macroStr += guidTable.getSizeMacro();
        macroStr += stringTable.getSizeMacro();
        macroStr += skuIdTable.getSizeMacro();
        macroStr += localTokenNumberTable.getSizeMacro();
        macroStr += exMapTable.getSizeMacro();
        macroStr += sizeTable.getSizeMacro();

        //
        // Generate existance info Macro for all Tables
        //
        macroStr += guidTable.getExistanceMacro();
        macroStr += stringTable.getExistanceMacro();
        macroStr += skuIdTable.getExistanceMacro();
        macroStr += localTokenNumberTable.getExistanceMacro();
        macroStr += exMapTable.getExistanceMacro();

        macroStr += newLine;

        return macroStr;
    }

    private HashMap <String, String> genCStructCode(
                                            ArrayList<CStructTypeDeclaration> declaList,
                                            HashMap<String, String> instTable,
                                            String phase
                                            ) {

        int i;
        HashMap <String, String> result = new HashMap<String, String>();
        HashMap <Integer, ArrayList<String>>    alignmentInitDecl = new HashMap<Integer, ArrayList<String>>();
        HashMap <Integer, ArrayList<String>>    alignmentUninitDecl = new HashMap<Integer, ArrayList<String>>();
        HashMap <Integer, ArrayList<String>>    alignmentInitInst = new HashMap<Integer, ArrayList<String>>();

        //
        // Initialize the storage for each alignment
        //
        for (i = 8; i > 0; i>>=1) {
            alignmentInitDecl.put(new Integer(i), new ArrayList<String>());
            alignmentInitInst.put(new Integer(i), new ArrayList<String>());
            alignmentUninitDecl.put(new Integer(i), new ArrayList<String>());
        }

        String initDeclStr   = "typedef struct {" + newLine;
        String initInstStr   = String.format("%s_PCD_DATABASE_INIT g%sPcdDbInit = { ", phase.toUpperCase(), phase.toUpperCase()) + newLine;
        String uninitDeclStr = "typedef struct {" + newLine;

        //
        // Sort all C declaration and instantiation base on Alignment Size
        //
        for (Object d : declaList) {
            CStructTypeDeclaration decl = (CStructTypeDeclaration) d;

            if (decl.initTable) {
                alignmentInitDecl.get(new Integer(decl.alignmentSize)).add(decl.cCode);
                alignmentInitInst.get(new Integer(decl.alignmentSize)).add(instTable.get(decl.key));
            } else {
                alignmentUninitDecl.get(new Integer(decl.alignmentSize)).add(decl.cCode);
            }
        }

        //
        // Generate code for every alignment size
        //
        boolean uinitDatabaseEmpty = true;
        for (int align = 8; align > 0; align >>= 1) {
            ArrayList<String> declaListBasedOnAlignment = alignmentInitDecl.get(new Integer(align));
            ArrayList<String> instListBasedOnAlignment = alignmentInitInst.get(new Integer(align));
            for (i = 0; i < declaListBasedOnAlignment.size(); i++) {
                initDeclStr += tab + declaListBasedOnAlignment.get(i);
                initInstStr += tab + instListBasedOnAlignment.get(i);

                //
                // We made a assumption that both PEI_PCD_DATABASE and DXE_PCD_DATABASE
                // has a least one data memember with alignment size of 1. So we can
                // remove the last "," in the C structure instantiation string. Luckily,
                // this is true as both data structure has SKUID_TABLE anyway.
                //
                if ((align == 1) && (i == declaListBasedOnAlignment.size() - 1)) {
                    initInstStr += newLine;
                } else {
                    initInstStr += commaNewLine;
                }
            }

            declaListBasedOnAlignment = alignmentUninitDecl.get(new Integer(align));

            if (declaListBasedOnAlignment.size() != 0) {
                uinitDatabaseEmpty = false;
            }

            for (Object d : declaListBasedOnAlignment) {
                String s = (String)d;
                uninitDeclStr += tab + s;
            }
        }

        if (uinitDatabaseEmpty) {
            uninitDeclStr += tab + String.format("%-20sdummy; /* PCD_DATABASE_UNINIT is emptry */\r\n", "UINT8");
        }

        initDeclStr += String.format("} %s_PCD_DATABASE_INIT;", phase) + newLine + newLine;
        initInstStr += "};" + newLine;
        uninitDeclStr += String.format("} %s_PCD_DATABASE_UNINIT;", phase) + newLine + newLine;

        result.put("initDeclStr", initDeclStr);
        result.put("initInstStr", initInstStr);
        result.put("uninitDeclStr", uninitDeclStr);

        return result;
    }

    public static String genInstantiationStr (ArrayList<String> alStr) {
        String str = "";
        for (int i = 0; i< alStr.size(); i++) {
            if (i != 0) {
                str += "\t";
            }
            str += alStr.get(i);
            if (i != alStr.size() - 1) {
                str += "\r\n";
            }
        }

        return str;
    }

    private String getSkuEnabledTypeDeclaration (Token token) {
        return String.format("%-20s%s;\r\n", "SKU_HEAD", token.getPrimaryKeyString());
    }

    private String getSkuEnabledTypeInstantiaion (Token token, int SkuTableIdx) {

        String offsetof = String.format(PcdDatabase.offsetOfSkuHeadStrTemplate, phase, token.hasDefaultValue()? "Init" : "Uninit", token.getPrimaryKeyString());
        return String.format("{ %s, %d } /* SKU_ENABLED: %s */", offsetof, SkuTableIdx, token.getPrimaryKeyString());
    }

    private String getDataTypeInstantiationForVariableDefault (Token token, String cName, int skuId) {
        return String.format("%s /* %s */", token.skuData.get(skuId).value.hiiDefaultValue, cName);
    }

    private String getCType (Token t)
        throws EntityException {

        if (t.isHiiEnable()) {
            return "VARIABLE_HEAD";
        }

        if (t.isVpdEnable()) {
            return "VPD_HEAD";
        }

        if (t.isUnicodeStringType()) {
            return "STRING_HEAD";
        }

        switch (t.datumType) {
        case UINT64:
            return "UINT64";
        case UINT32:
            return "UINT32";
        case UINT16:
            return "UINT16";
        case UINT8:
            return "UINT8";
        case BOOLEAN:
            return "BOOLEAN";
        case POINTER:
            return "UINT8";
        default:
            throw new EntityException("Unknown DatumType in getDataTypeCDeclaration");
        }
    }

    //
    // privateGlobalName and privateGlobalCCode is used to pass output to caller of getCDeclarationString
    //
    private void getCDeclarationString(Token t)
        throws EntityException {

        if (t.isSkuEnable()) {
            privateGlobalName = String.format("%s_%s", t.getPrimaryKeyString(), skuDataTableTemplate);
        } else {
            privateGlobalName = t.getPrimaryKeyString();
        }

        String type = getCType(t);
        if ((t.datumType == Token.DATUM_TYPE.POINTER) && (!t.isHiiEnable()) && (!t.isUnicodeStringType())) {
            int bufferSize;
            if (t.isASCIIStringType()) {
                //
                // Build tool will add a NULL string at the end of the ASCII string
                //
                bufferSize = t.datumSize + 1;
            } else {
                bufferSize = t.datumSize;
            }
            privateGlobalCCode = String.format("%-20s%s[%d][%d];\r\n", type, privateGlobalName, t.getSkuIdCount(), bufferSize);
        } else {
            privateGlobalCCode = String.format("%-20s%s[%d];\r\n", type, privateGlobalName, t.getSkuIdCount());
        }
    }

    private String getDataTypeDeclarationForVariableDefault (Token token, String cName, int skuId)
        throws EntityException {

        String typeStr;

        if (token.datumType == Token.DATUM_TYPE.UINT8) {
            typeStr = "UINT8";
        } else if (token.datumType == Token.DATUM_TYPE.UINT16) {
            typeStr = "UINT16";
        } else if (token.datumType == Token.DATUM_TYPE.UINT32) {
            typeStr = "UINT32";
        } else if (token.datumType == Token.DATUM_TYPE.UINT64) {
            typeStr = "UINT64";
        } else if (token.datumType == Token.DATUM_TYPE.BOOLEAN) {
            typeStr = "BOOLEAN";
        } else if (token.datumType == Token.DATUM_TYPE.POINTER) {
            int size;
            if (token.isHiiDefaultValueUnicodeStringType()) {
                typeStr = "UINT16";
                //
                // Include the NULL charactor
                //
                size = token.datumSize / 2 + 1;
            } else {
                typeStr = "UINT8";
                if (token.isHiiDefaultValueASCIIStringType()) {
                    //
                    // Include the NULL charactor
                    //
                    size = token.datumSize + 1;
                } else {
                    size = token.datumSize;
                }
            }
            return String.format("%-20s%s[%d];\r\n", typeStr, cName, size);
        } else {
            throw new EntityException("Unknown DATUM_TYPE type in when generating code for VARIABLE_ENABLED PCD entry");
        }

        return String.format("%-20s%s;\r\n", typeStr, cName);
    }

    private String getTypeInstantiation (Token t, ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) throws EntityException {

        int     i;

        String s;
        s = String.format("/* %s */", t.getPrimaryKeyString()) + newLine;
        s += tab + "{" + newLine;

        for (i = 0; i < t.skuData.size(); i++) {
            if (t.isUnicodeStringType()) {
                s += tab + tab + String.format(" %d ", stringTable.add(t.skuData.get(i).value.value, t));
            } else if (t.isHiiEnable()) {
                /* VPD_HEAD definition
                   typedef struct {
                      UINT16  GuidTableIndex;   // Offset in Guid Table in units of GUID.
                      UINT16  StringIndex;      // Offset in String Table in units of UINT16.
                      UINT16  Offset;           // Offset in Variable
                      UINT16  DefaultValueOffset; // Offset of the Default Value
                    } VARIABLE_HEAD  ;
                 */
                String variableDefaultName = String.format("%s_VariableDefault_%d", t.getPrimaryKeyString(), i);

                s += tab + tab + String.format("{ %d, %d, %s, %s }", guidTable.add(t.skuData.get(i).value.variableGuid, t.getPrimaryKeyString()),
                                                          stringTable.add(t.skuData.get(i).value.getStringOfVariableName(), t),
                                                          t.skuData.get(i).value.variableOffset,
                                                          String.format("offsetof(%s_PCD_DATABASE, Init.%s)", phase, variableDefaultName)
                                                          );
                //
                // We need to support the default value, so we add the declaration and
                // the instantiation for the default value.
                //
                CStructTypeDeclaration decl = new CStructTypeDeclaration (variableDefaultName,
                                                        getHiiPtrTypeAlignmentSize(t),
                                                        getDataTypeDeclarationForVariableDefault(t, variableDefaultName, i),
                                                        true
                                                        );
                declaList.add(decl);
                instTable.put(variableDefaultName, getDataTypeInstantiationForVariableDefault (t, variableDefaultName, i));
            } else if (t.isVpdEnable()) {
                    /* typedef  struct {
                        UINT32  Offset;
                      } VPD_HEAD;
                    */
                s += tab + tab + String.format("{ %s }", t.skuData.get(i).value.vpdOffset);
            } else {
                if (t.isByteStreamType()) {
                    //
                    // Byte stream type input has their own "{" "}", so we won't help to insert.
                    //
                    s += tab + tab + String.format(" %s ", t.skuData.get(i).value.value);
                } else {
                    s += tab + tab + String.format("{ %s }", t.skuData.get(i).value.value);
                }
            }

            if (i != t.skuData.size() - 1) {
                s += commaNewLine;
            } else {
                s += newLine;
            }

        }

        s += tab + "}";

        return s;
    }

    public static String getPcdDatabaseCommonDefinitions () {

		String retStr;

		retStr 	= "//\r\n";
		retStr += "// The following definition will be generated by build tool\r\n";
		retStr += "//\r\n";
		retStr += "\r\n";
		retStr += "//\r\n";
		retStr += "// Common definitions\r\n";
		retStr += "//\r\n";
		retStr += "typedef UINT8 SKU_ID;\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_TYPE_SHIFT        28\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_TYPE_DATA         (0x0 << PCD_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_TYPE_HII    	    (0x8 << PCD_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_TYPE_VPD    	    (0x4 << PCD_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_TYPE_SKU_ENABLED 	(0x2 << PCD_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_TYPE_STRING       (0x1 << PCD_TYPE_SHIFT)\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_TYPE_ALL_SET      (PCD_TYPE_DATA | PCD_TYPE_HII | PCD_TYPE_VPD | PCD_TYPE_SKU_ENABLED | PCD_TYPE_STRING)\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_DATUM_TYPE_SHIFT  24\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_DATUM_TYPE_POINTER        (0x0 << PCD_DATUM_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_DATUM_TYPE_UINT8          (0x1 << PCD_DATUM_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_DATUM_TYPE_UINT16    	    (0x2 << PCD_DATUM_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_DATUM_TYPE_UINT32    	    (0x4 << PCD_DATUM_TYPE_SHIFT)\r\n";
		retStr += "#define PCD_DATUM_TYPE_UINT64        	(0x8 << PCD_DATUM_TYPE_SHIFT)\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_DATUM_TYPE_ALL_SET    (PCD_DATUM_TYPE_POINTER | \\\r\n";
		retStr += "                                    PCD_DATUM_TYPE_UINT8  | \\\r\n";
		retStr += "                                    PCD_DATUM_TYPE_UINT16 | \\\r\n";
		retStr += "                                    PCD_DATUM_TYPE_UINT32 | \\\r\n";
		retStr += "                                    PCD_DATUM_TYPE_UINT64)\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_DATABASE_OFFSET_MASK (~(PCD_TYPE_ALL_SET | PCD_DATUM_TYPE_ALL_SET))\r\n";
		retStr += "\r\n";
		retStr += "typedef struct  {\r\n";
		retStr += "  UINT32                ExTokenNumber;\r\n";
		retStr += "  UINT16                LocalTokenNumber;   // PCD Number of this particular platform build\r\n";
		retStr += "  UINT16                ExGuidIndex;        // Index of GuidTable\r\n";
		retStr += "} DYNAMICEX_MAPPING;\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		retStr += "typedef struct {\r\n";
		retStr += "  UINT32  SkuDataStartOffset; //We have to use offsetof MACRO as we don't know padding done by compiler\r\n";
		retStr += "  UINT32  SkuIdTableOffset;   //Offset from the PCD_DB\r\n";
		retStr += "} SKU_HEAD;\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		retStr += "typedef struct {\r\n";
		retStr += "  UINT16  GuidTableIndex;     // Offset in Guid Table in units of GUID.\r\n";
		retStr += "  UINT16  StringIndex;        // Offset in String Table in units of UINT16.\r\n";
		retStr += "  UINT16  Offset;             // Offset in Variable\r\n";
		retStr += "  UINT16  DefaultValueOffset; // Offset of the Default Value\r\n";
		retStr += "} VARIABLE_HEAD  ;\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		retStr += "typedef  struct {\r\n";
		retStr += "  UINT32  Offset;\r\n";
		retStr += "} VPD_HEAD;\r\n";
		retStr += "\r\n";
		retStr += "typedef UINT16 STRING_HEAD;\r\n";
		retStr += "\r\n";
		retStr += "typedef UINT16 SIZE_INFO;\r\n";
		retStr += "\r\n";
		retStr += "#define offsetof(s,m)                 (UINT32) (UINTN) &(((s *)0)->m)\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		
		return retStr;
    }

    public static String getPcdDxeDatabaseDefinitions ()
        throws EntityException {

        String retStr = "";
		
		retStr += "\r\n";
		retStr += "typedef struct {\r\n";
		retStr += "  DXE_PCD_DATABASE_INIT Init;\r\n";
		retStr += "  DXE_PCD_DATABASE_UNINIT Uninit;\r\n";
		retStr += "} DXE_PCD_DATABASE;\r\n";
		retStr += "\r\n";
		retStr += "\r\n";
		retStr += "typedef struct {\r\n";
		retStr += "  PEI_PCD_DATABASE PeiDb;\r\n";
		retStr += "  DXE_PCD_DATABASE DxeDb;\r\n";
		retStr += "} PCD_DATABASE;\r\n";
		retStr += "\r\n";
		retStr += "#define DXE_NEX_TOKEN_NUMBER (DXE_LOCAL_TOKEN_NUMBER - DXE_EX_TOKEN_NUMBER)\r\n";
		retStr += "\r\n";
		retStr += "#define PCD_TOTAL_TOKEN_NUMBER (PEI_LOCAL_TOKEN_NUMBER + DXE_LOCAL_TOKEN_NUMBER)\r\n";
		retStr += "\r\n";
		retStr += "\r\n";

        return retStr;
    }

    public static String getPcdPeiDatabaseDefinitions ()
        throws EntityException {

		String retStr = "";
		
		retStr += "\r\n";
		retStr += "typedef struct {\r\n";
		retStr += "  PEI_PCD_DATABASE_INIT Init;\r\n";
		retStr += "  PEI_PCD_DATABASE_UNINIT Uninit;\r\n";
		retStr += "} PEI_PCD_DATABASE;\r\n";
		retStr += "\r\n";
		retStr += "#define PEI_NEX_TOKEN_NUMBER (PEI_LOCAL_TOKEN_NUMBER - PEI_EX_TOKEN_NUMBER)\r\n";
		retStr += "\r\n";

        return retStr;
    }

    /**
       Translate the schema string to UUID instance.

       In schema, the string of UUID is defined as following two types string:
        1) GuidArrayType: pattern = 0x[a-fA-F0-9]{1,8},( )*0x[a-fA-F0-9]{1,4},(
        )*0x[a-fA-F0-9]{1,4}(,( )*\{)?(,?( )*0x[a-fA-F0-9]{1,2}){8}( )*(\})?

        2) GuidNamingConvention: pattern =
        [a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}

       This function will convert string and create uuid instance.

       @param uuidString    UUID string in XML file

       @return UUID         UUID instance
    **/
    private UUID translateSchemaStringToUUID(String uuidString)
        throws EntityException {
        String      temp;
        String[]    splitStringArray;
        int         index;
        int         chIndex;
        int         chLen;

        if (uuidString == null) {
            return null;
        }

        if (uuidString.length() == 0) {
            return null;
        }

        if (uuidString.equals("0") ||
            uuidString.equalsIgnoreCase("0x0")) {
            return new UUID(0, 0);
        }

        uuidString = uuidString.replaceAll("\\{", "");
        uuidString = uuidString.replaceAll("\\}", "");

        //
        // If the UUID schema string is GuidArrayType type then need translate
        // to GuidNamingConvention type at first.
        //
        if ((uuidString.charAt(0) == '0') && ((uuidString.charAt(1) == 'x') || (uuidString.charAt(1) == 'X'))) {
            splitStringArray = uuidString.split("," );
            if (splitStringArray.length != 11) {
                throw new EntityException ("[FPD file error] Wrong format for GUID string: " + uuidString);
            }

            //
            // Remove blank space from these string and remove header string "0x"
            //
            for (index = 0; index < 11; index ++) {
                splitStringArray[index] = splitStringArray[index].trim();
                splitStringArray[index] = splitStringArray[index].substring(2, splitStringArray[index].length());
            }

            //
            // Add heading '0' to normalize the string length
            //
            for (index = 3; index < 11; index ++) {
                chLen = splitStringArray[index].length();
                for (chIndex = 0; chIndex < 2 - chLen; chIndex ++) {
                    splitStringArray[index] = "0" + splitStringArray[index];
                }
            }

            //
            // construct the final GuidNamingConvention string
            //
            temp = String.format("%s-%s-%s-%s%s-%s%s%s%s%s%s",
                                 splitStringArray[0],
                                 splitStringArray[1],
                                 splitStringArray[2],
                                 splitStringArray[3],
                                 splitStringArray[4],
                                 splitStringArray[5],
                                 splitStringArray[6],
                                 splitStringArray[7],
                                 splitStringArray[8],
                                 splitStringArray[9],
                                 splitStringArray[10]);
            uuidString = temp;
        }

        return UUID.fromString(uuidString);
    }
}
