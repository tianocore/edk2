/** @file
  CollectPCDAction class.

  This action class is to collect PCD information from MSA, SPD, FPD xml file.
  This class will be used for wizard and build tools, So it can *not* inherit
  from buildAction or wizardAction.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.pcd.action;

import java.io.BufferedReader;                                                    
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions;
import org.tianocore.FrameworkModulesDocument;
import org.tianocore.FrameworkPlatformDescriptionDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.PcdBuildDefinitionDocument.PcdBuildDefinition;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.pcd.action.ActionMessage;
import org.tianocore.build.pcd.entity.DynamicTokenValue;
import org.tianocore.build.pcd.entity.MemoryDatabaseManager;
import org.tianocore.build.pcd.entity.SkuInstance;
import org.tianocore.build.pcd.entity.Token;
import org.tianocore.build.pcd.entity.UsageInstance;
import org.tianocore.build.pcd.exception.EntityException;
import org.tianocore.ModuleTypeDef;

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

class StringTable {
    private ArrayList<String>   al; 
    private ArrayList<String>   alComments;
    private String              phase;
    int                         len; 

    public StringTable (String phase) {
        this.phase = phase;
        al = new ArrayList<String>();
        alComments = new ArrayList<String>();
        len = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.StringTableSizeMacro, phase, getSize());
    }

    private int getSize () {
        //
        // We have at least one Unicode Character in the table.
        //
        return len == 0 ? 1 : len;
    }

    public int getTableLen () {
        return al.size() == 0 ? 1 : al.size();
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.StringTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }
    
    public void genCodeNew (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable) {
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
                String str = al.get(i);
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
                cDeclCode += String.format("%-20s%s[%d]; /* %s */", "UINT16", stringTableName, str.length() + 1, alComments.get(i)) + newLine;
                
                if (i == 0) {
                    cInstCode = "/* StringTable */" + newLine;
                }
                cInstCode += tab + String.format("L\"%s\" /* %s */", al.get(i), alComments.get(i));
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

    public String getTypeDeclaration () {

        String output;

        final String stringTable = "StringTable";
        final String tab = "\t";
        final String newLine = ";\r\n";

        output =  "/* StringTable */\r\n";

        if (al.size() == 0) {
            output += tab + String.format("UINT16 %s[1] /* StringTable is Empty */", stringTable) + newLine;
        }

        for (int i = 0; i < al.size(); i++) {
            String str = al.get(i);

            if (i == 0) {
                //
                // StringTable is a well-known name in the PCD DXE driver
                //
                output += tab + String.format("UINT16       %s[%d] /* %s */", stringTable, str.length() + 1, alComments.get(i)) + newLine;
            } else {
                output += tab + String.format("UINT16       %s_%d[%d] /* %s */", stringTable, i, str.length() + 1, alComments.get(i)) + newLine;
            }
        }

        return output;

    }

    public ArrayList<String> getInstantiation () {
        ArrayList<String> output = new ArrayList<String>();

        output.add("/* StringTable */"); 

        if (al.size() == 0) {
            output.add("{ 0 }");
        } else {
            String str;

            for (int i = 0; i < al.size(); i++) {
                str = String.format("L\"%s\" /* %s */", al.get(i), alComments.get(i));
                if (i != al.size() - 1) {
                    str += ",";
                }
                output.add(str);
            }
        }

        return output;
    }

    public int add (String inputStr, Token token) {
        int i;
        int pos;

        String str = inputStr;
        
        //
        // The input can be two types:
        // "L\"Bootmode\"" or "Bootmode". 
        // We drop the L\" and \" for the first type. 
        if (str.startsWith("L\"") && str.endsWith("\"")) {
            str = str.substring(2, str.length() - 1);
        }
        //
        // Check if StringTable has this String already.
        // If so, return the current pos.
        //
        for (i = 0, pos = 0; i < al.size(); i++) {
            String s = al.get(i);;

            if (str.equals(s)) {
                return pos;
            }
            pos = s.length() + 1;
            
        }
        
        i = len;
        //
        // Include the NULL character at the end of String
        //
        len += str.length() + 1; 
        al.add(str);
        alComments.add(token.getPrimaryKeyString());

        return i;
    }
}

class SizeTable {
    private ArrayList<Integer>  al;
    private ArrayList<String>   alComments;
    private String              phase;
    private int                 len;
    
    public SizeTable (String phase) {
        this.phase = phase;
        al = new ArrayList<Integer>();
        alComments = new ArrayList<String>();
        len = 0;
    }

    public void genCodeNew (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
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

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.SizeTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
        ArrayList<String> Output = new ArrayList<String>();

        Output.add("/* SizeTable */");
        Output.add("{");
        if (al.size() == 0) {
            Output.add("\t0");
        } else {
            for (int index = 0; index < al.size(); index++) {
                Integer n = al.get(index);
                String str = "\t" + n.toString();

                if (index != (al.size() - 1)) {
                    str += ",";
                }

                str += " /* " + alComments.get(index) + " */"; 
                Output.add(str);
    
            }
        }
        Output.add("}");

        return Output;
    }

    public int add (Token token) {
        int index = len;

        len++; 
        al.add(token.datumSize);
        alComments.add(token.getPrimaryKeyString());

        return index;
    }
    
    public int getTableLen () {
        return al.size() == 0 ? 1 : al.size();
    }

}

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

    public void genCodeNew (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
        final String name = "GuidTable";
        
        CStructTypeDeclaration decl;
        String cCode = "";

        cCode += String.format(PcdDatabase.GuidTableDeclaration, phase); 
        decl = new CStructTypeDeclaration (
                                            name,
                                            8,
                                            cCode,
                                            true
                                           );  
        declaList.add(decl);


        cCode = PcdDatabase.genInstantiationStr(getInstantiation());
        instTable.put(name, cCode);
    }

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.GuidTableDeclaration, phase);
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

    public ArrayList<String> getInstantiation () {
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
            if (al.get(i).equals(uuid)) {
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

    public int getTableLen () {
        return al.size() == 0 ? 0 : al.size();
    }

}

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

    public void genCodeNew (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
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

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.SkuIdTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
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

    public int getTableLen () {
        return al.size() == 0 ? 1 : al.size();
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

    public void genCodeNew (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
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

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.LocalTokenNumberTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
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
        
        al.add(str);
        alComment.add(token.getPrimaryKeyString());

        return index;
    }
}

class ExMapTable {

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
    private ArrayList<String>    alComment;
    private String               phase;
    private int                  len;
    private int                   bodyLineNum;
    
    public ExMapTable (String phase) {
        this.phase = phase;
        al = new ArrayList<ExTriplet>();
        alComment = new ArrayList<String>();
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

    public void genCodeNew (ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) {
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
    
    public String getTypeDeclaration () {
        return String.format(PcdDatabase.ExMapTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
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

            str += "}" + " /* " + alComment.get(index) + " */" ;

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
        al.add(new ExTriplet(guidTableIdx, exTokenNum, localTokenIdx));
        alComment.add(name);

        return index;
    }

    public int getTableLen () {
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
                if (a.exTokenNumber > b.exTokenNumber) {
                    return 1;
                } else if (a.exTokenNumber > b.exTokenNumber) {
                    return 1;
                } else {
                    return 0;
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

class PcdDatabase {

    private final static int    SkuHeadAlignmentSize             = 4;
    private final String        newLine                         = "\r\n";
    private final String        commaNewLine                    = ",\r\n";
    private final String        tab                             = "\t";
    public final static String ExMapTableDeclaration            = "DYNAMICEX_MAPPING   ExMapTable[%s_EXMAPPING_TABLE_SIZE];\r\n";
    public final static String GuidTableDeclaration             = "EFI_GUID            GuidTable[%s_GUID_TABLE_SIZE];\r\n";
    public final static String LocalTokenNumberTableDeclaration = "UINT32              LocalTokenNumberTable[%s_LOCAL_TOKEN_NUMBER_TABLE_SIZE];\r\n";
    public final static String StringTableDeclaration           = "UINT16              StringTable[%s_STRING_TABLE_SIZE];\r\n";
    public final static String SizeTableDeclaration             = "UINT16              SizeTable[%s_LOCAL_TOKEN_NUMBER_TABLE_SIZE];\r\n";
    public final static String SkuIdTableDeclaration            = "UINT8               SkuIdTable[%s_SKUID_TABLE_SIZE];\r\n";


    public final static String ExMapTableSizeMacro              = "#define %s_EXMAPPING_TABLE_SIZE  %d\r\n";
    public final static String ExTokenNumber                    = "#define %s_EX_TOKEN_NUMBER       %d\r\n";
    public final static String GuidTableSizeMacro               = "#define %s_GUID_TABLE_SIZE         %d\r\n"; 
    public final static String LocalTokenNumberTableSizeMacro   = "#define %s_LOCAL_TOKEN_NUMBER_TABLE_SIZE            %d\r\n";
    public final static String LocalTokenNumberSizeMacro   		= "#define %s_LOCAL_TOKEN_NUMBER            %d\r\n";
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


    class AlignmentSizeComp implements Comparator<Token> {
        public int compare (Token a, Token b) {
            return getAlignmentSize(b) 
                    - getAlignmentSize(a);
        }
    }
    
    public PcdDatabase (ArrayList<Token> alTokens, String exePhase, int startLen) {
       phase = exePhase;

       stringTable = new StringTable(phase);
       guidTable = new GuidTable(phase);
       localTokenNumberTable = new LocalTokenNumberTable(phase);
       skuIdTable = new SkuIdTable(phase);
       sizeTable = new SizeTable(phase);
       exMapTable = new ExMapTable(phase); 

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

    private void getTwoGroupsOfTokens (ArrayList<Token> alTokens, List<Token> initTokens, List<Token> uninitTokens) {
        for (int i = 0; i < alTokens.size(); i++) {
            Token t = (Token)alTokens.get(i);
            if (t.hasDefaultValue()) {
                initTokens.add(t);
            } else {
                uninitTokens.add(t);
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

    private void ProcessTokensNew (List<Token> tokens, 
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
                                guidTable.add(t.tokenSpaceName, t.getPrimaryKeyString()), 
                                t.getPrimaryKeyString()
                                );
            }
        }

    }
    
    public void genCodeNew () throws EntityException {
        
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
        ProcessTokensNew (nexTokens, cStructDeclList, cStructInstTable, phase);
        ProcessTokensNew (exTokens, cStructDeclList, cStructInstTable, phase);
        
        stringTable.genCodeNew(cStructDeclList, cStructInstTable);
        skuIdTable.genCodeNew(cStructDeclList, cStructInstTable, phase);
        exMapTable.genCodeNew(cStructDeclList, cStructInstTable, phase);
        localTokenNumberTable.genCodeNew(cStructDeclList, cStructInstTable, phase);
        sizeTable.genCodeNew(cStructDeclList, cStructInstTable, phase);
        guidTable.genCodeNew(cStructDeclList, cStructInstTable, phase);
        
        hString = genCMacroCode ();
        
        HashMap <String, String> result;
        
        result = genCStructCode(cStructDeclList, 
                cStructInstTable, 
                phase
                );
        
        hString += result.get("initDeclStr");
        hString += result.get("uninitDeclStr");
        
        hString += String.format("#define PCD_%s_SERVICE_DRIVER_VERSION         %d", phase, version);
        
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

     public void genCode () 
        throws EntityException {

        final String newLine                        = "\r\n";
        final String declNewLine                    = ";\r\n";
        final String tab                            = "\t";
        final String commaNewLine                   = ", \r\n";

        int i;
        ArrayList<String> decla;
        ArrayList<String> inst;

        String macroStr   = "";
        String initDeclStr = "";
        String initInstStr = "";
        String uninitDeclStr = "";

        List<Token> initTokens = new ArrayList<Token> ();
        List<Token> uninitTokens = new ArrayList<Token> ();
        
        HashMap <String, ArrayList<String>> initCode = new HashMap<String, ArrayList<String>> ();
        HashMap <String, ArrayList<String>> uninitCode = new HashMap<String, ArrayList<String>> ();

        getTwoGroupsOfTokens (alTokens, initTokens, uninitTokens);

        //
        // Generate Structure Declaration for PcdTokens without Default Value
        // PEI_PCD_DATABASE_INIT
        //
        java.util.Comparator<Token> comparator = new AlignmentSizeComp();
        java.util.Collections.sort(initTokens, comparator);
        initCode = processTokens(initTokens);

        //
        // Generate Structure Declaration for PcdTokens without Default Value
        // PEI_PCD_DATABASE_UNINIT
        //
        java.util.Collections.sort(uninitTokens, comparator);
        uninitCode = processTokens(uninitTokens);

        //
        // Generate size info Macro for all Tables
        //
        macroStr += guidTable.getSizeMacro();
        macroStr += stringTable.getSizeMacro();
        macroStr += skuIdTable.getSizeMacro();
        macroStr += localTokenNumberTable.getSizeMacro();
        macroStr += exMapTable.getSizeMacro();

        //
        // Generate existance info Macro for all Tables
        //
        macroStr += guidTable.getExistanceMacro();
        macroStr += stringTable.getExistanceMacro();
        macroStr += skuIdTable.getExistanceMacro();
        macroStr += localTokenNumberTable.getExistanceMacro();
        macroStr += exMapTable.getExistanceMacro();

        //
        // Generate Structure Declaration for PcdTokens with Default Value
        // for example PEI_PCD_DATABASE_INIT
        //
        initDeclStr += "typedef struct {" + newLine;
            {
                initDeclStr += tab + exMapTable.getTypeDeclaration();
                initDeclStr += tab + guidTable.getTypeDeclaration();
                initDeclStr += tab + localTokenNumberTable.getTypeDeclaration();
                initDeclStr += tab + stringTable.getTypeDeclaration();
                initDeclStr += tab + sizeTable.getTypeDeclaration();
                initDeclStr += tab + skuIdTable.getTypeDeclaration();
                if (phase.equalsIgnoreCase("PEI")) {
                    initDeclStr += tab + "SKU_ID            SystemSkuId;" + newLine;
                }

                decla = initCode.get(new String("Declaration"));
                for (i = 0; i < decla.size(); i++)  {
                    initDeclStr += tab + decla.get(i) + declNewLine;
                }

                //
                // Generate Structure Declaration for PcdToken with SkuEnabled
                //
                decla = initCode.get("DeclarationForSku");

                for (i = 0; i < decla.size(); i++) {
                    initDeclStr += tab + decla.get(i) + declNewLine;
                }
            }
        initDeclStr += String.format("} %s_PCD_DATABASE_INIT;\r\n\r\n", phase);

        //
        // Generate MACRO for structure intialization of PCDTokens with Default Value
        // The sequence must match the sequence of declaration of the memembers in the structure
        String tmp = String.format("%s_PCD_DATABASE_INIT g%sPcdDbInit = { ", phase.toUpperCase(), phase.toUpperCase());
        initInstStr +=  tmp + newLine;
        initInstStr += tab + genInstantiationStr(exMapTable.getInstantiation()) + commaNewLine;
        initInstStr += tab + genInstantiationStr(guidTable.getInstantiation()) + commaNewLine;
        initInstStr += tab + genInstantiationStr(localTokenNumberTable.getInstantiation()) + commaNewLine; 
        initInstStr += tab + genInstantiationStr(stringTable.getInstantiation()) + commaNewLine;
        initInstStr += tab + genInstantiationStr(sizeTable.getInstantiation()) + commaNewLine;
        initInstStr += tab + genInstantiationStr(skuIdTable.getInstantiation()) + commaNewLine;
        //
        // For SystemSkuId
        //
        if (phase.equalsIgnoreCase("PEI")) {
            initInstStr += tab + "0" + tab + "/* SystemSkuId */" + commaNewLine;
        }

        inst = initCode.get("Instantiation");
        for (i = 0; i < inst.size(); i++) {
            initInstStr += tab + inst.get(i) + commaNewLine;
        }

        inst = initCode.get("InstantiationForSku");
        for (i = 0; i < inst.size(); i++) {
            initInstStr += tab + inst.get(i);
            if (i != inst.size() - 1) {
                initInstStr += commaNewLine;
            }
        }

        initInstStr += "};";

        uninitDeclStr += "typedef struct {" + newLine;
            {
                decla = uninitCode.get("Declaration");
                if (decla.size() == 0) {
                    uninitDeclStr += "UINT8 dummy /* The UINT struct is empty */" + declNewLine;
                } else {
    
                    for (i = 0; i < decla.size(); i++) {
                        uninitDeclStr += tab + decla.get(i) + declNewLine;
                    }
    
                    decla = uninitCode.get("DeclarationForSku");
    
                    for (i = 0; i < decla.size(); i++) {
                        uninitDeclStr += tab + decla.get(i) + declNewLine;
                    }
                }
            }
        uninitDeclStr += String.format("} %s_PCD_DATABASE_UNINIT;\r\n\r\n", phase);

        cString = initInstStr + newLine;
        hString = macroStr      + newLine  
              + initDeclStr   + newLine
              + uninitDeclStr + newLine
              + newLine;
        
        hString += String.format("#define PCD_%s_SERVICE_DRIVER_VERSION			%d", phase, version);

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

    private HashMap<String, ArrayList<String>> processTokens (List<Token> alToken) 
        throws EntityException {

        HashMap <String, ArrayList<String>> map = new HashMap<String, ArrayList<String>>();

        ArrayList<String> decl = new ArrayList<String>();
        ArrayList<String> declForSkuEnableType = new ArrayList<String>();
        ArrayList<String> inst = new ArrayList<String>();
        ArrayList<String> instForSkuEnableType = new ArrayList<String>();

        for (int index = 0; index < alToken.size(); index++) {
            Token token = alToken.get(index);

            if (token.isSkuEnable()) {
                //
                // BugBug: Schema only support Data type now
                //
                int tableIdx;

                tableIdx = skuIdTable.add(token);

                decl.add(getSkuEnabledTypeDeclaration(token));
                if (token.hasDefaultValue()) {
                    inst.add(getSkuEnabledTypeInstantiaion(token, tableIdx)); 
                }

                declForSkuEnableType.add(getDataTypeDeclarationForSkuEnabled(token));
                if (token.hasDefaultValue()) {
                    instForSkuEnableType.add(getDataTypeInstantiationForSkuEnabled(token));
                }

            } else {
                if (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.HII_TYPE) {
                    decl.add(getVariableEnableTypeDeclaration(token));
                    inst.add(getVariableEnableInstantiation(token));
                } else if (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.VPD_TYPE) {
                    decl.add(getVpdEnableTypeDeclaration(token));
                    inst.add(getVpdEnableTypeInstantiation(token));
                } else if (token.isUnicodeStringType()) {
                    decl.add(getStringTypeDeclaration(token));
                    inst.add(getStringTypeInstantiation(stringTable.add(token.getStringTypeString(), token), token));
                }
                else {
                    decl.add(getDataTypeDeclaration(token));
                    if (token.hasDefaultValue()) {
                        inst.add(getDataTypeInstantiation(token));
                    }
                }
            }

            sizeTable.add(token);
            localTokenNumberTable.add(token);
            token.tokenNumber = assignedTokenNumber++;

        }

        map.put("Declaration",  decl);
        map.put("DeclarationForSku", declForSkuEnableType);
        map.put("Instantiation", inst);
        map.put("InstantiationForSku", instForSkuEnableType);

        return map;
    }

    private String getSkuEnabledTypeDeclaration (Token token) {
        return String.format("%-20s%s;\r\n", "SKU_HEAD", token.getPrimaryKeyString());
    }

    private String getSkuEnabledTypeInstantiaion (Token token, int SkuTableIdx) {

        String offsetof = String.format(PcdDatabase.offsetOfSkuHeadStrTemplate, phase, token.hasDefaultValue()? "Init" : "Uninit", token.getPrimaryKeyString());
        return String.format("{ %s, %d } /* SKU_ENABLED: %s */", offsetof, SkuTableIdx, token.getPrimaryKeyString());
    }

    private String getDataTypeDeclarationForSkuEnabled (Token token) {
        String typeStr = "";

        if (token.datumType == Token.DATUM_TYPE.UINT8) {
            typeStr = "UINT8 %s_%s[%d];\r\n";
        } else if (token.datumType == Token.DATUM_TYPE.UINT16) {
            typeStr = "UINT16 %s_%s[%d];\r\n";
        } else if (token.datumType == Token.DATUM_TYPE.UINT32) {
            typeStr = "UINT32 %s_%s[%d];\r\n";
        } else if (token.datumType == Token.DATUM_TYPE.UINT64) {
            typeStr = "UINT64 %s_%s[%d];\r\n";
        } else if (token.datumType == Token.DATUM_TYPE.BOOLEAN) {
            typeStr = "BOOLEAN %s_%s[%d];\r\n";
        } else if (token.datumType == Token.DATUM_TYPE.POINTER) {
            return String.format("UINT8 %s_%s[%d];\r\n", token.getPrimaryKeyString(), "SkuDataTable", token.datumSize * token.skuData.size());
        } 

        return String.format(typeStr, token.getPrimaryKeyString(), "SkuDataTable", token.skuData.size());

    }

    private String getDataTypeInstantiationForSkuEnabled (Token token) {
        String str = "";

        if (token.datumType == Token.DATUM_TYPE.POINTER) {
            return String.format("UINT8 %s_%s[%d]", token.getPrimaryKeyString(), "SkuDataTable", token.datumSize * token.skuData.size());
        } else {
            str = "{ ";
            for (int idx = 0; idx < token.skuData.size(); idx++) {
                str += token.skuData.get(idx).toString();
                if (idx != token.skuData.size() - 1) {
                    str += ", ";
                }
            }
            str += "}";

            return str;
        }

    }

    private String getDataTypeInstantiationForVariableDefault_new (Token token, String cName, int skuId) {
        return String.format("%s /* %s */", token.skuData.get(skuId).value.hiiDefaultValue, cName);
    }

    private String getDataTypeInstantiation (Token token) {

        if (token.datumType == Token.DATUM_TYPE.POINTER) {
            return String.format("%s /* %s */", token.getDefaultSku().value, token.getPrimaryKeyString());
        } else {
            return String.format("%s /* %s */", token.getDefaultSku().value, token.getPrimaryKeyString());
        }
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
            throw new EntityException("Unknown type in getDataTypeCDeclaration");
        }
    }
    
    private void getCDeclarationString(Token t) 
        throws EntityException {
        
        if (t.isSkuEnable()) {
            privateGlobalName = String.format("%s_%s", t.getPrimaryKeyString(), skuDataTableTemplate);
        } else {
            privateGlobalName = t.getPrimaryKeyString();
        }

        if (t.isUnicodeStringType()) {
            privateGlobalCCode = String.format("%-20s%s[%d];\r\n", "STRING_HEAD", t.getPrimaryKeyString(), t.getSkuIdCount());
        } else {
            String type = getCType(t);
            if (t.datumType == Token.DATUM_TYPE.POINTER) {
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
    }
    
    private String getDataTypeDeclarationForVariableDefault_new (Token token, String cName, int skuId) {

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
            return String.format("%-20s%s[%d];\r\n", cName, token.datumSize);
        } else {
            typeStr = "";
        }

        return String.format("%-20s%s;\r\n", typeStr, cName);
    }
    
    private String getDataTypeDeclaration (Token token) {

        String typeStr = "";

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
            return String.format("UINT8 %s[%d]", token.getPrimaryKeyString(), token.datumSize);
        } else {
        }

        return String.format("%s    %s", typeStr, token.getPrimaryKeyString());
    }

    private String getVpdEnableTypeDeclaration (Token token) {
        return String.format("VPD_HEAD %s", token.getPrimaryKeyString());
    }
    
    private String getTypeInstantiation (Token t, ArrayList<CStructTypeDeclaration> declaList, HashMap<String, String> instTable, String phase) throws EntityException {
      
        int     i;

        String s;
        s = String.format("/* %s */", t.getPrimaryKeyString()) + newLine;
        s += tab + "{" + newLine;

        for (i = 0; i < t.skuData.size(); i++) {
            if (t.isUnicodeStringType() && !t.isHiiEnable()) {
                s += tab + tab + String.format("{ %d }", stringTable.add(t.skuData.get(i).value.value, t));
            } else if (t.isHiiEnable()) {
                /* VPD_HEAD definition
                   typedef struct {
                      UINT16  GuidTableIndex;   // Offset in Guid Table in units of GUID.
                      UINT16  StringIndex;      // Offset in String Table in units of UINT16.
                      UINT16  Offset;           // Offset in Variable
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
                                                        getDataTypeAlignmentSize(t),
                                                        getDataTypeDeclarationForVariableDefault_new(t, variableDefaultName, i),
                                                        true
                                                        ); 
                declaList.add(decl);
                instTable.put(variableDefaultName, getDataTypeInstantiationForVariableDefault_new (t, variableDefaultName, i));
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
    
    private String getVpdEnableTypeInstantiation (Token token) {
        return String.format("{ %s } /* %s */", token.getDefaultSku().vpdOffset,
                                                token.getPrimaryKeyString());
    }

    private String getStringTypeDeclaration (Token token) {
        return String.format("UINT16  %s", token.getPrimaryKeyString());
    }

    private String getStringTypeInstantiation (int StringTableIdx, Token token) {
        return String.format ("%d /* %s */", StringTableIdx,
                                             token.getPrimaryKeyString()); 
    }


    private String getVariableEnableTypeDeclaration (Token token) {
      return String.format("VARIABLE_HEAD  %s", token.getPrimaryKeyString());
    }

    private String getVariableEnableInstantiation (Token token) 
        throws EntityException {
        //
        // Need scott fix
        // 
        return String.format("{ %d, %d, %s } /* %s */", guidTable.add(token.getDefaultSku().variableGuid, token.getPrimaryKeyString()),
                                                        stringTable.add(token.getDefaultSku().getStringOfVariableName(), token),
                                                        token.getDefaultSku().variableOffset, 
                                                        token.getPrimaryKeyString());
    }

    public int getTotalTokenNumber () {
        return sizeTable.getTableLen();
    }

    public static String getPcdDatabaseCommonDefinitions () 
        throws EntityException {

        String retStr = "";
        try {
            File file = new File(GlobalData.getWorkspacePath() + File.separator + 
                                 "Tools" + File.separator + 
                                 "Conf" + File.separator +
                                 "Pcd" + File.separator +
                                 "PcdDatabaseCommonDefinitions.sample");
            FileReader reader = new FileReader(file);
            BufferedReader  in = new BufferedReader(reader);
            String str;
            while ((str = in.readLine()) != null) {
                retStr = retStr +"\r\n" + str;
            }
        } catch (Exception ex) {
            throw new EntityException("Fatal error when generating PcdDatabase Common Definitions");
        }

        return retStr;
    }

    public static String getPcdDxeDatabaseDefinitions () 
        throws EntityException {

        String retStr = "";
        try {
            File file = new File(GlobalData.getWorkspacePath() + File.separator + 
                                 "Tools" + File.separator + 
                                 "Conf" + File.separator +
                                 "Pcd" + File.separator +
                                 "PcdDatabaseDxeDefinitions.sample");
            FileReader reader = new FileReader(file);
            BufferedReader  in = new BufferedReader(reader);
            String str;
            while ((str = in.readLine()) != null) {
                retStr = retStr +"\r\n" + str;
            }
        } catch (Exception ex) {
            throw new EntityException("Fatal error when generating PcdDatabase Dxe Definitions");
        }

        return retStr;
    }

    public static String getPcdPeiDatabaseDefinitions () 
        throws EntityException {

        String retStr = "";
        try {
            File file = new File(GlobalData.getWorkspacePath() + File.separator + 
                                 "Tools" + File.separator + 
                                 "Conf" + File.separator +
                                 "Pcd" + File.separator +
                                 "PcdDatabasePeiDefinitions.sample");
            FileReader reader = new FileReader(file);
            BufferedReader  in = new BufferedReader(reader);
            String str;
            while ((str = in.readLine()) != null) {
                retStr = retStr +"\r\n" + str;
            }
        } catch (Exception ex) {
            throw new EntityException("Fatal error when generating PcdDatabase Pei Definitions");
        }

        return retStr;
    }

}

class ModuleInfo {
    public ModuleSADocument.ModuleSA module;
    public ModuleTypeDef.Enum        type;

    public ModuleInfo (ModuleSADocument.ModuleSA module, ModuleTypeDef.Enum type) {
        this.module = module;
        this.type   = type;
    }
}

/** This action class is to collect PCD information from MSA, SPD, FPD xml file.
    This class will be used for wizard and build tools, So it can *not* inherit
    from buildAction or UIAction.
**/
public class CollectPCDAction {
    /// memoryDatabase hold all PCD information collected from SPD, MSA, FPD.
    private MemoryDatabaseManager dbManager;

    /// Workspacepath hold the workspace information.
    private String                workspacePath;

    /// FPD file is the root file. 
    private String                fpdFilePath;

    /// Message level for CollectPCDAction.
    private int                   originalMessageLevel;

    /// Cache the fpd docment instance for private usage.
    private FrameworkPlatformDescriptionDocument fpdDocInstance;

    /**
      Set WorkspacePath parameter for this action class.

      @param workspacePath parameter for this action
    **/
    public void setWorkspacePath(String workspacePath) {
        this.workspacePath = workspacePath;
    }

    /**
      Set action message level for CollectPcdAction tool.

      The message should be restored when this action exit.

      @param actionMessageLevel parameter for this action
    **/
    public void setActionMessageLevel(int actionMessageLevel) {
        originalMessageLevel       = ActionMessage.messageLevel;
        ActionMessage.messageLevel = actionMessageLevel;
    }

    /**
      Set FPDFileName parameter for this action class.

      @param fpdFilePath    fpd file path
    **/
    public void setFPDFilePath(String fpdFilePath) {
        this.fpdFilePath = fpdFilePath;
    }

    /**
      Common function interface for outer.
      
      @param workspacePath The path of workspace of current build or analysis.
      @param fpdFilePath   The fpd file path of current build or analysis.
      @param messageLevel  The message level for this Action.
      
      @throws  Exception The exception of this function. Because it can *not* be predict
                         where the action class will be used. So only Exception can be throw.
      
    **/
    public void perform(String workspacePath, String fpdFilePath, 
                        int messageLevel) throws Exception {
        setWorkspacePath(workspacePath);
        setFPDFilePath(fpdFilePath);
        setActionMessageLevel(messageLevel);
        checkParameter();
        execute();
        ActionMessage.messageLevel = originalMessageLevel;
    }

    /**
      Core execution function for this action class.
     
      This function work flows will be:
      1) Collect and prepocess PCD information from FPD file, all PCD
      information will be stored into memory database.
      2) Generate 3 strings for
        a) All modules using Dynamic(Ex) PCD entry.(Token Number)
        b) PEI PCDDatabase (C Structure) for PCD Service PEIM.
        c) DXE PCD Database (C structure) for PCD Service DXE.
                                
      
      @throws  EntityException Exception indicate failed to execute this action.
      
    **/
    private void execute() throws EntityException {
        //
        // Get memoryDatabaseManager instance from GlobalData.
        // The memoryDatabaseManager should be initialized for whatever build
        // tools or wizard tools
        //
        if((dbManager = GlobalData.getPCDMemoryDBManager()) == null) {
            throw new EntityException("The instance of PCD memory database manager is null");
        }

        //
        // Collect all PCD information defined in FPD file.
        // Evenry token defind in FPD will be created as an token into 
        // memory database.
        //
        createTokenInDBFromFPD();
        
        //
        // Call Private function genPcdDatabaseSourceCode (void); ComponentTypeBsDriver
        // 1) Generate for PEI, DXE PCD DATABASE's definition and initialization.
        //
        genPcdDatabaseSourceCode ();
        
    }

    /**
      This function generates source code for PCD Database.
     
      @param void
      @throws EntityException  If the token does *not* exist in memory database.

    **/
    private void genPcdDatabaseSourceCode()
        throws EntityException {
        String PcdCommonHeaderString = PcdDatabase.getPcdDatabaseCommonDefinitions ();

        ArrayList<Token> alPei = new ArrayList<Token> ();
        ArrayList<Token> alDxe = new ArrayList<Token> ();

        dbManager.getTwoPhaseDynamicRecordArray(alPei, alDxe);
        PcdDatabase pcdPeiDatabase = new PcdDatabase (alPei, "PEI", 0);
        pcdPeiDatabase.genCodeNew();
        MemoryDatabaseManager.PcdPeimHString        = PcdCommonHeaderString + pcdPeiDatabase.getHString()
                                            + PcdDatabase.getPcdPeiDatabaseDefinitions();
        MemoryDatabaseManager.PcdPeimCString        = pcdPeiDatabase.getCString();

        PcdDatabase pcdDxeDatabase = new PcdDatabase (alDxe, 
                                                      "DXE",
                                                      alPei.size()
                                                      );
        pcdDxeDatabase.genCodeNew();
        MemoryDatabaseManager.PcdDxeHString   = MemoryDatabaseManager.PcdPeimHString + pcdDxeDatabase.getHString()
                                      + PcdDatabase.getPcdDxeDatabaseDefinitions();
        MemoryDatabaseManager.PcdDxeCString   = pcdDxeDatabase.getCString();
    }

    /**
      Get component array from FPD.
      
      This function maybe provided by some Global class.
      
      @return List<ModuleInfo> the component array.
      
     */
    private List<ModuleInfo> getComponentsFromFPD() 
        throws EntityException {
        List<ModuleInfo>            allModules  = new ArrayList<ModuleInfo>();
        ModuleInfo                  current     = null;
        int                         index       = 0;
        org.tianocore.Components    components  = null;
        FrameworkModulesDocument.FrameworkModules fModules = null;
        ModuleSADocument.ModuleSA[]               modules  = null;
        HashMap<String, XmlObject>                map      = new HashMap<String, XmlObject>();

        if (fpdDocInstance == null) {
            try {
                fpdDocInstance = (FrameworkPlatformDescriptionDocument)XmlObject.Factory.parse(new File(fpdFilePath));
            } catch(IOException ioE) {
                throw new EntityException("File IO error for xml file:" + fpdFilePath + "\n" + ioE.getMessage());
            } catch(XmlException xmlE) {
                throw new EntityException("Can't parse the FPD xml fle:" + fpdFilePath + "\n" + xmlE.getMessage());
            }

        }

        map.put("FrameworkPlatformDescription", fpdDocInstance);
        SurfaceAreaQuery.setDoc(map); 
        modules = SurfaceAreaQuery.getFpdModuleSAs();
        for (index = 0; index < modules.length; index ++) {
            SurfaceAreaQuery.setDoc(GlobalData.getDoc(modules[index].getModuleName()));
            allModules.add(new ModuleInfo(modules[index], 
                                          ModuleTypeDef.Enum.forString(SurfaceAreaQuery.getModuleType())));
        }
        
        return allModules;
    }

    /**
      Create token instance object into memory database, the token information
      comes for FPD file. Normally, FPD file will contain all token platform 
      informations.
      
      @return FrameworkPlatformDescriptionDocument   The FPD document instance for furture usage.
      
      @throws EntityException                        Failed to parse FPD xml file.
      
    **/
    private void createTokenInDBFromFPD() 
        throws EntityException {
        int                                 index             = 0;
        int                                 index2            = 0;
        int                                 pcdIndex          = 0;
        List<PcdBuildDefinition.PcdData>    pcdBuildDataArray = new ArrayList<PcdBuildDefinition.PcdData>();
        PcdBuildDefinition.PcdData          pcdBuildData      = null;
        Token                               token             = null;
        SkuInstance                         skuInstance       = null;
        int                                 skuIndex          = 0;
        List<ModuleInfo>                    modules           = null;
        String                              primaryKey        = null;
        String                              exceptionString   = null;
        UsageInstance                       usageInstance     = null;
        String                              primaryKey1       = null;
        String                              primaryKey2       = null;
        boolean                             isDuplicate       = false;
        Token.PCD_TYPE                      pcdType           = Token.PCD_TYPE.UNKNOWN;
        Token.DATUM_TYPE                    datumType         = Token.DATUM_TYPE.UNKNOWN;
        long                                 tokenNumber       = 0;
        String                              moduleName        = null;
        String                              datum             = null;
        int                                 maxDatumSize      = 0;

        //
        // ----------------------------------------------
        // 1), Get all <ModuleSA> from FPD file.
        // ----------------------------------------------
        // 
        modules = getComponentsFromFPD();

        if (modules == null) {
            throw new EntityException("[FPD file error] No modules in FPD file, Please check whether there are elements in <FrameworkModules> in FPD file!");
        }

        //
        // -------------------------------------------------------------------
        // 2), Loop all modules to process <PcdBuildDeclarations> for each module.
        // -------------------------------------------------------------------
        // 
        for (index = 0; index < modules.size(); index ++) {
            isDuplicate =  false;
            for (index2 = 0; index2 < index; index2 ++) {
                //
                // BUGBUG: For transition schema, we can *not* get module's version from 
                // <ModuleSAs>, It is work around code.
                // 
                primaryKey1 = UsageInstance.getPrimaryKey(modules.get(index).module.getModuleName(), 
                                                          null,
                                                          null, 
                                                          null, 
                                                          modules.get(index).module.getArch().toString(),
                                                          null);
                primaryKey2 = UsageInstance.getPrimaryKey(modules.get(index2).module.getModuleName(), 
                                                          null, 
                                                          null, 
                                                          null, 
                                                          modules.get(index2).module.getArch().toString(), 
                                                          null);
                if (primaryKey1.equalsIgnoreCase(primaryKey2)) {
                    isDuplicate = true;
                    break;
                }
            }

            if (isDuplicate) {
                continue;
            }

    	    //
    	    // It is legal for a module does not contains ANY pcd build definitions.
    	    // 
    	    if (modules.get(index).module.getPcdBuildDefinition() == null) {
                continue;
    	    }
    
            pcdBuildDataArray = modules.get(index).module.getPcdBuildDefinition().getPcdDataList();

            moduleName = modules.get(index).module.getModuleName();

            //
            // ----------------------------------------------------------------------
            // 2.1), Loop all Pcd entry for a module and add it into memory database.
            // ----------------------------------------------------------------------
            // 
            for (pcdIndex = 0; pcdIndex < pcdBuildDataArray.size(); pcdIndex ++) {
                pcdBuildData = pcdBuildDataArray.get(pcdIndex);
                primaryKey   = Token.getPrimaryKeyString(pcdBuildData.getCName(),
                                                         translateSchemaStringToUUID(pcdBuildData.getTokenSpaceGuid()));
                pcdType      = Token.getpcdTypeFromString(pcdBuildData.getItemType().toString());
                datumType    = Token.getdatumTypeFromString(pcdBuildData.getDatumType().toString());
                tokenNumber  = Long.decode(pcdBuildData.getToken().toString());
                
                if (pcdBuildData.getValue() != null) {
                    datum = pcdBuildData.getValue().toString();
                } else {
                    datum = null;
                }
                maxDatumSize = pcdBuildData.getMaxDatumSize();

                if ((pcdType    == Token.PCD_TYPE.FEATURE_FLAG) &&
                    (datumType  != Token.DATUM_TYPE.BOOLEAN)){
                    exceptionString = String.format("[FPD file error] For PCD %s in module %s, the PCD type is FEATRUE_FLAG but "+
                                                    "datum type of this PCD entry is not BOOLEAN!",
                                                    pcdBuildData.getCName(),
                                                    moduleName);
                    throw new EntityException(exceptionString);
                }

                //
                // Check <TokenSpaceGuid> is exist? In future, because all schema verification will tools
                // will check that, following checking code could be removed.
                // 
                if (pcdBuildData.getTokenSpaceGuid() == null) {
                    exceptionString = String.format("[FPD file error] There is no <TokenSpaceGuid> for PCD %s in module %s! This is required!",
                                                    pcdBuildData.getCName(),
                                                    moduleName);
                    throw new EntityException(exceptionString);
                }

                //
                // -------------------------------------------------------------------------------------------
                // 2.1.1), Do some necessary checking work for FixedAtBuild, FeatureFlag and PatchableInModule
                // -------------------------------------------------------------------------------------------
                // 
                if (!Token.isDynamic(pcdType)) {
                     //
                     // Value is required.
                     // 
                     if (datum == null) {
                         exceptionString = String.format("[FPD file error] There is no value for PCD entry %s in module %s!",
                                                         pcdBuildData.getCName(),
                                                         moduleName);
                         throw new EntityException(exceptionString);
                     }

                     //
                     // Check whether the datum size is matched datum type.
                     // 
                     if ((exceptionString = verifyDatum(pcdBuildData.getCName(), 
                                                        moduleName,
                                                        datum,
                                                        datumType,
                                                        maxDatumSize)) != null) {
                         throw new EntityException(exceptionString);
                     }
                }

                //
                // ---------------------------------------------------------------------------------
                // 2.1.2), Create token or update token information for current anaylized PCD data.
                // ---------------------------------------------------------------------------------
                // 
                if (dbManager.isTokenInDatabase(primaryKey)) {
                    //
                    // If the token is already exist in database, do some necessary checking
                    // and add a usage instance into this token in database
                    // 
                    token = dbManager.getTokenByKey(primaryKey);
    
                    //
                    // checking for DatumType, DatumType should be unique for one PCD used in different
                    // modules.
                    // 
                    if (token.datumType != datumType) {
                        exceptionString = String.format("[FPD file error] The datum type of PCD entry %s is %s, which is different with  %s defined in before!",
                                                        pcdBuildData.getCName(), 
                                                        pcdBuildData.getDatumType().toString(), 
                                                        Token.getStringOfdatumType(token.datumType));
                        throw new EntityException(exceptionString);
                    }

                    //
                    // Check token number is valid
                    // 
                    if (tokenNumber != token.tokenNumber) {
                        exceptionString = String.format("[FPD file error] The token number of PCD entry %s in module %s is different with same PCD entry in other modules!",
                                                        pcdBuildData.getCName(),
                                                        moduleName);
                        throw new EntityException(exceptionString);
                    }

                    //
                    // For same PCD used in different modules, the PCD type should all be dynamic or non-dynamic.
                    // 
                    if (token.isDynamicPCD != Token.isDynamic(pcdType)) {
                        exceptionString = String.format("[FPD file error] For PCD entry %s in module %s, you define dynamic or non-dynamic PCD type which"+
                                                        "is different with others module's",
                                                        token.cName,
                                                        moduleName);
                        throw new EntityException(exceptionString);
                    }

                    if (token.isDynamicPCD) {
                        //
                        // Check datum is equal the datum in dynamic information.
                        // For dynamic PCD, you can do not write <Value> in sperated every <PcdBuildDefinition> in different <ModuleSA>,
                        // But if you write, the <Value> must be same as the value in <DynamicPcdBuildDefinitions>.
                        // 
                        if (!token.isSkuEnable() && 
                            (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.DEFAULT_TYPE) &&
                            (datum != null)) {
                            if (!datum.equalsIgnoreCase(token.getDefaultSku().value)) {
                                exceptionString = String.format("[FPD file error] For dynamic PCD %s in module %s, the datum in <ModuleSA> is "+
                                                                "not equal to the datum in <DynamicPcdBuildDefinitions>, it is "+
                                                                "illega! You could no set <Value> in <ModuleSA> for a dynamic PCD!",
                                                                token.cName,
                                                                moduleName);
                                throw new EntityException(exceptionString);
                            }
                        }

                        if ((maxDatumSize != 0) &&
                            (maxDatumSize != token.datumSize)){
                            exceptionString = String.format("[FPD file error] For dynamic PCD %s in module %s, the max datum size is %d which "+
                                                            "is different with <MaxDatumSize> %d defined in <DynamicPcdBuildDefinitions>!",
                                                            token.cName,
                                                            moduleName,
                                                            maxDatumSize,
                                                            token.datumSize);
                            throw new EntityException(exceptionString);
                        }
                    }
                    
                } else {
                    //
                    // If the token is not in database, create a new token instance and add
                    // a usage instance into this token in database.
                    // 
                    token = new Token(pcdBuildData.getCName(), 
                                      translateSchemaStringToUUID(pcdBuildData.getTokenSpaceGuid()));
    
                    token.datumType     = datumType;
                    token.tokenNumber   = tokenNumber;
                    token.isDynamicPCD  = Token.isDynamic(pcdType);
                    token.datumSize     = maxDatumSize;
                    
                    if (token.isDynamicPCD) {
                        //
                        // For Dynamic and Dynamic Ex type, need find the dynamic information
                        // in <DynamicPcdBuildDefinition> section in FPD file.
                        // 
                        updateDynamicInformation(moduleName, 
                                                 token,
                                                 datum,
                                                 maxDatumSize);
                    }
    
                    dbManager.addTokenToDatabase(primaryKey, token);
                }

                //
                // -----------------------------------------------------------------------------------
                // 2.1.3), Add the PcdType in current module into this Pcd token's supported PCD type.
                // -----------------------------------------------------------------------------------
                // 
                token.updateSupportPcdType(pcdType);

                //
                // ------------------------------------------------
                // 2.1.4), Create an usage instance for this token.
                // ------------------------------------------------
                // 
                usageInstance = new UsageInstance(token, 
                                                  moduleName, 
                                                  null,
                                                  null,
                                                  null,
                                                  modules.get(index).type, 
                                                  pcdType,
                                                  modules.get(index).module.getArch().toString(), 
                                                  null,
                                                  datum,
                                                  maxDatumSize);
                token.addUsageInstance(usageInstance);
            }
        }
    }

    /**
       Verify the datum value according its datum size and datum type, this
       function maybe moved to FPD verification tools in future.
       
       @param cName
       @param moduleName
       @param datum
       @param datumType
       @param maxDatumSize
       
       @return String
     */
    /***/
    public String verifyDatum(String            cName,
                              String            moduleName,
                              String            datum, 
                              Token.DATUM_TYPE  datumType, 
                              int               maxDatumSize) {
        String      exceptionString = null;
        int         value;
        BigInteger  value64;
        String      subStr;
        int         index;

        if (moduleName == null) {
            moduleName = "section <DynamicPcdBuildDefinitions>";
        } else {
            moduleName = "module " + moduleName;
        }

        if (maxDatumSize == 0) {
            exceptionString = String.format("[FPD file error] You maybe miss <MaxDatumSize> for PCD %s in %s",
                                            cName,
                                            moduleName);
            return exceptionString;
        }

        switch (datumType) {
        case UINT8:
            if (maxDatumSize != 1) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT8, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                try {
                    value = Integer.decode(datum);
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is not valid "+
                                                    "digital format of UINT8",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }
                if (value > 0xFF) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s exceed"+
                                                    " the max size of UINT8 - 0xFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case UINT16:
            if (maxDatumSize != 2) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT16, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }
            if (datum != null) {
                try {
                    value = Integer.decode(datum);
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is "+
                                                    "not valid digital of UINT16",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }
                if (value > 0xFFFF) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s "+
                                                    "which exceed the range of UINT16 - 0xFFFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case UINT32:
            if (maxDatumSize != 4) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT32, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                try {
                    if (datum.length() > 2) {
                        if ((datum.charAt(0) == '0')        && 
                            ((datum.charAt(1) == 'x') || (datum.charAt(1) == 'X'))){
                            subStr = datum.substring(2, datum.length());
                            value64 = new BigInteger(subStr, 16);
                        } else {
                            value64 = new BigInteger(datum);
                        }
                    } else {
                        value64 = new BigInteger(datum);
                    }
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is not "+
                                                    "valid digital of UINT32",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

                if (value64.bitLength() > 32) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s which "+
                                                    "exceed the range of UINT32 - 0xFFFFFFFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case UINT64:
            if (maxDatumSize != 8) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT64, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                try {
                    if (datum.length() > 2) {
                        if ((datum.charAt(0) == '0')        && 
                            ((datum.charAt(1) == 'x') || (datum.charAt(1) == 'X'))){
                            subStr = datum.substring(2, datum.length());
                            value64 = new BigInteger(subStr, 16);
                        } else {
                            value64 = new BigInteger(datum);
                        }
                    } else {
                        value64 = new BigInteger(datum);
                    }
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is not valid"+
                                                    " digital of UINT64",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

                if (value64.bitLength() > 64) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s "+
                                                    "exceed the range of UINT64 - 0xFFFFFFFFFFFFFFFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case BOOLEAN:
            if (maxDatumSize != 1) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is BOOLEAN, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                if (!(datum.equalsIgnoreCase("TRUE") ||
                     datum.equalsIgnoreCase("FALSE"))) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                    "is BOOELAN, but value is not 'true'/'TRUE' or 'FALSE'/'false'",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

            }
            break;
        case POINTER:
            if (datum == null) {
                break;
            }

            char    ch     = datum.charAt(0);
            int     start, end;
            String  strValue;
            //
            // For void* type PCD, only three datum is support:
            // 1) Unicode: string with start char is "L"
            // 2) Ansci: String start char is ""
            // 3) byte array: String start char "{"
            // 
            if (ch == 'L') {
                start       = datum.indexOf('\"');
                end         = datum.lastIndexOf('\"');
                if ((start > end)           || 
                    (end   > datum.length())||
                    ((start == end) && (datum.length() > 0))) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID* and datum is "+
                                                    "a UNICODE string because start with L\", but format maybe"+
                                                    "is not right, correct UNICODE string is L\"...\"!",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

                strValue    = datum.substring(start + 1, end);
                if ((strValue.length() * 2) > maxDatumSize) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and datum is "+
                                                    "a UNICODE string, but the datum size is %d exceed to <MaxDatumSize> : %d",
                                                    cName,
                                                    moduleName,
                                                    strValue.length() * 2, 
                                                    maxDatumSize);
                    return exceptionString;
                }
            } else if (ch == '\"'){
                start       = datum.indexOf('\"');
                end         = datum.lastIndexOf('\"');
                if ((start > end)           || 
                    (end   > datum.length())||
                    ((start == end) && (datum.length() > 0))) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID* and datum is "+
                                                    "a ANSCII string because start with \", but format maybe"+
                                                    "is not right, correct ANSIC string is \"...\"!",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }
                strValue    = datum.substring(start + 1, end);
                if ((strValue.length()) > maxDatumSize) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and datum is "+
                                                    "a ANSCI string, but the datum size is %d which exceed to <MaxDatumSize> : %d",
                                                    cName,
                                                    moduleName,
                                                    strValue.length(),
                                                    maxDatumSize);
                    return exceptionString;
                }
            } else if (ch =='{') {
                String[]  strValueArray;

                start           = datum.indexOf('{');
                end             = datum.lastIndexOf('}');
                strValue        = datum.substring(start + 1, end);
                strValue        = strValue.trim();
                if (strValue.length() == 0) {
                    break;
                }
                strValueArray   = strValue.split(",");
                for (index = 0; index < strValueArray.length; index ++) {
                    try{
                        value = Integer.decode(strValueArray[index].trim());
                    } catch (NumberFormatException nfeEx) {
                        exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and "+
                                                         "it is byte array in fact. For every byte in array should be a valid"+
                                                         "byte digital, but element %s is not a valid byte digital!",
                                                         cName,
                                                         moduleName,
                                                         strValueArray[index]);
                        return exceptionString;
                    }
                    if (value > 0xFF) {
                        exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, "+
                                                        "it is byte array in fact. But the element of %s exceed the byte range",
                                                        cName,
                                                        moduleName,
                                                        strValueArray[index]);
                        return exceptionString;
                    }
                }

                if (strValueArray.length > maxDatumSize) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and datum is byte"+
                                                    "array, but the number of bytes is %d which exceed to <MaxDatumSzie> : %d!",
                                                    cName,
                                                    moduleName,
                                                    strValueArray.length,
                                                    maxDatumSize);
                    return exceptionString;
                }
            } else {
                exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*. For VOID* type, you have three format choise:\n "+
                                                "1) UNICODE string: like L\"xxxx\";\r\n"+
                                                "2) ANSIC string: like \"xxx\";\r\n"+
                                                "3) Byte array: like {0x2, 0x45, 0x23}\r\n"+
                                                "But the datum in seems does not following above format!",
                                                cName, 
                                                moduleName);
                return exceptionString;
            }
            break;
        default:
            exceptionString = String.format("[FPD file error] For PCD entry %s in %s, datum type is unknown, it should be one of "+
                                            "UINT8, UINT16, UINT32, UINT64, VOID*, BOOLEAN",
                                            cName,
                                            moduleName);
            return exceptionString;
        }
        return null;
    }

    /**
       Get dynamic information for a dynamic PCD from <DynamicPcdBuildDefinition> seciton in FPD file.
       
       This function should be implemented in GlobalData in future.
       
       @param token         The token instance which has hold module's PCD information
       @param moduleName    The name of module who will use this Dynamic PCD.
       
       @return DynamicPcdBuildDefinitions.PcdBuildData
     */
    /***/
    private DynamicPcdBuildDefinitions.PcdBuildData getDynamicInfoFromFPD(Token     token,
                                                                          String    moduleName)
        throws EntityException {
        int    index             = 0;
        String exceptionString   = null;
        String dynamicPrimaryKey = null;
        DynamicPcdBuildDefinitions                    dynamicPcdBuildDefinitions = null;
        List<DynamicPcdBuildDefinitions.PcdBuildData> dynamicPcdBuildDataArray   = null;

        //
        // If FPD document is not be opened, open and initialize it.
        // 
        if (fpdDocInstance == null) {
            try {
                fpdDocInstance = (FrameworkPlatformDescriptionDocument)XmlObject.Factory.parse(new File(fpdFilePath));
            } catch(IOException ioE) {
                throw new EntityException("File IO error for xml file:" + fpdFilePath + "\n" + ioE.getMessage());
            } catch(XmlException xmlE) {
                throw new EntityException("Can't parse the FPD xml fle:" + fpdFilePath + "\n" + xmlE.getMessage());
            }
        }

        dynamicPcdBuildDefinitions = fpdDocInstance.getFrameworkPlatformDescription().getDynamicPcdBuildDefinitions();
        if (dynamicPcdBuildDefinitions == null) {
            exceptionString = String.format("[FPD file error] There are no <PcdDynamicBuildDescriptions> in FPD file but contains Dynamic type "+
                                            "PCD entry %s in module %s!",
                                            token.cName,
                                            moduleName);
            throw new EntityException(exceptionString);
        }

        dynamicPcdBuildDataArray = dynamicPcdBuildDefinitions.getPcdBuildDataList();
        for (index = 0; index < dynamicPcdBuildDataArray.size(); index ++) {
            //
            // Check <TokenSpaceGuid> is exist? In future, because all schema verification will tools
            // will check that, following checking code could be removed.
            // 
            if (dynamicPcdBuildDataArray.get(index).getTokenSpaceGuid() == null) {
                exceptionString = String.format("[FPD file error] There is no <TokenSpaceGuid> for PCD %s in <DynamicPcdBuildDefinitions>! This is required!",
                                                dynamicPcdBuildDataArray.get(index).getCName());
                throw new EntityException(exceptionString);
            }

            dynamicPrimaryKey = Token.getPrimaryKeyString(dynamicPcdBuildDataArray.get(index).getCName(),
                                                          translateSchemaStringToUUID(dynamicPcdBuildDataArray.get(index).getTokenSpaceGuid()));
            if (dynamicPrimaryKey.equalsIgnoreCase(token.getPrimaryKeyString())) {
                return dynamicPcdBuildDataArray.get(index);
            }
        }

        return null;
    }

    /**
       Update dynamic information for PCD entry.
       
       Dynamic information is retrieved from <PcdDynamicBuildDeclarations> in
       FPD file.
       
       @param moduleName        The name of the module who use this PCD
       @param token             The token instance
       @param datum             The <datum> in module's PCD information
       @param maxDatumSize      The <maxDatumSize> in module's PCD information
       
       @return Token
     */
    private Token updateDynamicInformation(String   moduleName, 
                                           Token    token,
                                           String   datum,
                                           int      maxDatumSize) 
        throws EntityException {
        int                 index           = 0;
        int                 offset;
        String              exceptionString = null;
        DynamicTokenValue   dynamicValue;
        SkuInstance         skuInstance     = null;
        String              temp;
        boolean             hasSkuId0       = false;
        Token.PCD_TYPE      pcdType         = Token.PCD_TYPE.UNKNOWN;
        long                tokenNumber     = 0;
        String              hiiDefaultValue = null;
        String[]            variableGuidString = null;

        List<DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo>   skuInfoList = null;
        DynamicPcdBuildDefinitions.PcdBuildData                 dynamicInfo = null;

        dynamicInfo = getDynamicInfoFromFPD(token, moduleName);
        if (dynamicInfo == null) {
            exceptionString = String.format("[FPD file error] For Dynamic PCD %s used by module %s, "+
                                            "there is no dynamic information in <DynamicPcdBuildDefinitions> "+
                                            "in FPD file, but it is required!",
                                            token.cName,
                                            moduleName);
            throw new EntityException(exceptionString);
        }

        token.datumSize = dynamicInfo.getMaxDatumSize();

        exceptionString = verifyDatum(token.cName, 
                                      moduleName,
                                      null, 
                                      token.datumType, 
                                      token.datumSize);
        if (exceptionString != null) {
            throw new EntityException(exceptionString);
        }

        if ((maxDatumSize != 0) && 
            (maxDatumSize != token.datumSize)) {
            exceptionString = String.format("FPD file error] For dynamic PCD %s, the datum size in module %s is %d, but "+
                                            "the datum size in <DynamicPcdBuildDefinitions> is %d, they are not match!",
                                            token.cName,
                                            moduleName, 
                                            maxDatumSize,
                                            dynamicInfo.getMaxDatumSize());
            throw new EntityException(exceptionString);
        }
        tokenNumber = Long.decode(dynamicInfo.getToken().toString());
        if (tokenNumber != token.tokenNumber) {
            exceptionString = String.format("[FPD file error] For dynamic PCD %s, the token number in module %s is 0x%x, but"+
                                            "in <DynamicPcdBuildDefinictions>, the token number is 0x%x, they are not match!",
                                            token.cName,
                                            moduleName,
                                            token.tokenNumber,
                                            tokenNumber);
            throw new EntityException(exceptionString);
        }

        pcdType = Token.getpcdTypeFromString(dynamicInfo.getItemType().toString());
        if (pcdType == Token.PCD_TYPE.DYNAMIC_EX) {
            token.dynamicExTokenNumber = tokenNumber;
        }

        skuInfoList = dynamicInfo.getSkuInfoList();

        //
        // Loop all sku data 
        // 
        for (index = 0; index < skuInfoList.size(); index ++) {
            skuInstance = new SkuInstance();
            //
            // Although SkuId in schema is BigInteger, but in fact, sku id is 32 bit value.
            // 
            temp = skuInfoList.get(index).getSkuId().toString();
            skuInstance.id = Integer.decode(temp);
            if (skuInstance.id == 0) {
                hasSkuId0 = true;
            }
            //
            // Judge whether is DefaultGroup at first, because most case is DefautlGroup.
            // 
            if (skuInfoList.get(index).getValue() != null) {
                skuInstance.value.setValue(skuInfoList.get(index).getValue().toString());
                if ((exceptionString = verifyDatum(token.cName, 
                                                   null, 
                                                   skuInfoList.get(index).getValue().toString(), 
                                                   token.datumType, 
                                                   token.datumSize)) != null) {
                    throw new EntityException(exceptionString);
                }

                token.skuData.add(skuInstance);

                //
                // Judege wether is same of datum between module's information
                // and dynamic information.
                // 
                if (datum != null) {
                    if ((skuInstance.id == 0)                                   &&
                        !datum.toString().equalsIgnoreCase(skuInfoList.get(index).getValue().toString())) {
                        exceptionString = "[FPD file error] For dynamic PCD " + token.cName + ", the value in module " + moduleName + " is " + datum.toString() + " but the "+
                                          "value of sku 0 data in <DynamicPcdBuildDefinition> is " + skuInstance.value.value + ". They are must be same!"+
                                          " or you could not define value for a dynamic PCD in every <ModuleSA>!"; 
                        throw new EntityException(exceptionString);
                    }
                }
                continue;
            }

            //
            // Judge whether is HII group case.
            // 
            if (skuInfoList.get(index).getVariableName() != null) {
                exceptionString = null;
                if (skuInfoList.get(index).getVariableGuid() == null) {
                    exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <VariableGuid> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    if (exceptionString != null) {
                        throw new EntityException(exceptionString);
                    }                                                    
                }

                if (skuInfoList.get(index).getVariableOffset() == null) {
                    exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <VariableOffset> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    if (exceptionString != null) {
                        throw new EntityException(exceptionString);
                    }
                }

                if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                    exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <HiiDefaultValue> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    if (exceptionString != null) {
                        throw new EntityException(exceptionString);
                    }
                }

                if (skuInfoList.get(index).getHiiDefaultValue() != null) {
                    hiiDefaultValue = skuInfoList.get(index).getHiiDefaultValue().toString();
                } else {
                    hiiDefaultValue = null;
                }

                if ((exceptionString = verifyDatum(token.cName, 
                                                   null, 
                                                   hiiDefaultValue, 
                                                   token.datumType, 
                                                   token.datumSize)) != null) {
                    throw new EntityException(exceptionString);
                }

                offset = Integer.decode(skuInfoList.get(index).getVariableOffset());
                if (offset > 0xFFFF) {
                    throw new EntityException(String.format("[FPD file error] For dynamic PCD %s ,  the variable offset defined in sku %d data "+
                                                            "exceed 64K, it is not allowed!",
                                                            token.cName,
                                                            index));
                }

                //
                // Get variable guid string according to the name of guid which will be mapped into a GUID in SPD file.
                // 
                variableGuidString = GlobalData.getGuidInfoGuid(skuInfoList.get(index).getVariableGuid().toString());
                if (variableGuidString == null) {
                    throw new EntityException(String.format("[GUID Error] For dynamic PCD %s,  the variable guid %s can be found in all SPD file!",
                                                            token.cName, 
                                                            skuInfoList.get(index).getVariableGuid().toString()));
                }

                skuInstance.value.setHiiData(skuInfoList.get(index).getVariableName(),
                                             translateSchemaStringToUUID(variableGuidString[1]),
                                             skuInfoList.get(index).getVariableOffset(),
                                             skuInfoList.get(index).getHiiDefaultValue().toString());
                token.skuData.add(skuInstance);
                continue;
            }

            if (skuInfoList.get(index).getVpdOffset() != null) {
                skuInstance.value.setVpdData(skuInfoList.get(index).getVpdOffset());
                token.skuData.add(skuInstance);
                continue;
            }

            exceptionString = String.format("[FPD file error] For dynamic PCD %s, the dynamic info must "+
                                            "be one of 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                            token.cName);
            throw new EntityException(exceptionString);
        }

        if (!hasSkuId0) {
            exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions>, there are "+
                                            "no sku id = 0 data, which is required for every dynamic PCD",
                                            token.cName);
            throw new EntityException(exceptionString);
        }

        return token;
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
                throw new EntityException ("[FPD file error] Wrong format for UUID string: " + uuidString);
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

    /**
      check parameter for this action.
      
      @throws EntityException  Bad parameter.
    **/
    private void checkParameter() throws EntityException {
        File file = null;

        if((fpdFilePath    == null) ||(workspacePath  == null)) {
            throw new EntityException("WorkspacePath and FPDFileName should be blank for CollectPCDAtion!");
        }

        if(fpdFilePath.length() == 0 || workspacePath.length() == 0) {
            throw new EntityException("WorkspacePath and FPDFileName should be blank for CollectPCDAtion!");
        }

        file = new File(workspacePath);
        if(!file.exists()) {
            throw new EntityException("WorkpacePath " + workspacePath + " does not exist!");
        }

        file = new File(fpdFilePath);

        if(!file.exists()) {
            throw new EntityException("FPD File " + fpdFilePath + " does not exist!");
        }
    }

    /**
      Test case function

      @param argv  parameter from command line
    **/
    public static void main(String argv[]) throws EntityException {
        CollectPCDAction ca = new CollectPCDAction();
        ca.setWorkspacePath("m:/tianocore/edk2");
        ca.setFPDFilePath("m:/tianocore/edk2/EdkNt32Pkg/Nt32.fpd");
        ca.setActionMessageLevel(ActionMessage.MAX_MESSAGE_LEVEL);
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            "m:/tianocore/edk2");
        ca.execute();
    }
}
