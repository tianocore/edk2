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
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.FrameworkPlatformDescriptionDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PcdBuildDeclarationsDocument.PcdBuildDeclarations.PcdBuildData;
import org.tianocore.PcdDefinitionsDocument.PcdDefinitions;
import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.pcd.action.ActionMessage;
import org.tianocore.build.pcd.entity.MemoryDatabaseManager;
import org.tianocore.build.pcd.entity.SkuInstance;
import org.tianocore.build.pcd.entity.Token;
import org.tianocore.build.pcd.entity.UsageInstance;
import org.tianocore.build.pcd.exception.EntityException;

class StringTable {
    private ArrayList<String>   al; 
		private ArrayList<String>   alComments;
    private String              phase;
    int                         len; 
		int													bodyStart;
		int													bodyLineNum;

    public StringTable (String phase) {
        this.phase = phase;
        al = new ArrayList<String>();
				alComments = new ArrayList<String>();
        len = 0;
				bodyStart = 0;
				bodyLineNum = 0;
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
								output += tab + String.format("UINT16 			%s[%d] /* %s */", stringTable, str.length() + 1, alComments.get(i)) + newLine;
						} else {
								output += tab + String.format("UINT16 			%s_%d[%d] /* %s */", stringTable, i, str.length() + 1, alComments.get(i)) + newLine;
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

    public int add (String str, Token token) {
        int i;

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
		private int							bodyStart;
		private int							bodyLineNum;

    public SizeTable (String phase) {
        this.phase = phase;
        al = new ArrayList<Integer>();
				alComments = new ArrayList<String>();
        len = 0;
				bodyStart = 0;
				bodyLineNum = 0;
    }

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.SizeTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
				ArrayList<String> Output = new ArrayList<String>();

        Output.add("/* SizeTable */");
        Output.add("{");
				bodyStart = 2;

				if (al.size() == 0) {
						Output.add("0");
				} else {
						for (int index = 0; index < al.size(); index++) {
								Integer n = al.get(index);
								String str = n.toString();

								if (index != (al.size() - 1)) {
										str += ",";
								}

								str += " /* " + alComments.get(index) + " */"; 
								Output.add(str);
								bodyLineNum++;
		
						}
				}
				Output.add("}");

        return Output;
    }

		public int getBodyStart() {
				return bodyStart;
		}

		public int getBodyLineNum () {
				return bodyLineNum;
		}

    public int add (Token token) {
        int index = len;

        len++; 
        al.add(token.datumSize);
				alComments.add(token.getPrimaryKeyString());

        return index;
    }

		private int getDatumSize(Token token) {
				/*
				switch (token.datumType) {
				case Token.DATUM_TYPE.UINT8:
						return 1;
				default:
						return 0;
				}
				*/
				return 0;
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
		private int							bodyStart;
		private int							bodyLineNum;

    public GuidTable (String phase) {
        this.phase = phase;
        al = new ArrayList<UUID>();
				alComments = new ArrayList<String>();
        len = 0;
				bodyStart = 0;
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

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.GuidTableDeclaration, phase);
    }

		private String getUuidCString (UUID uuid) {
				String[]  guidStrArray;

				guidStrArray =(uuid.toString()).split("-");

				return String.format("{ 0x%s, 0x%s, 0x%s, { 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s } }",
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
				bodyStart = 2;

				if (al.size() == 0) {
						Output.add(getUuidCString(new UUID(0, 0)));
				}
        
        for (Object u : al) {
            UUID uuid = (UUID)u;
						String str = getUuidCString(uuid);

						if (al.indexOf(u) != (al.size() - 1)) {
								str += ",";
						}
            Output.add(str);
						bodyLineNum++;

        }
				Output.add("}");

        return Output;
    }

		public int getBodyStart() {
				return bodyStart;
		}

		public int getBodyLineNum () {
				return bodyLineNum;
		}

    public int add (UUID uuid, String name) {
        int index = len;
        //
        // Include the NULL character at the end of String
        //
        len++; 
        al.add(uuid);

        return index;
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
		private int										bodyStart;
		private int										bodyLineNum;

    public SkuIdTable (String phase) {
        this.phase = phase;
        al = new ArrayList<Integer[]>();
				alComment = new ArrayList<String>();
				bodyStart = 0;
				bodyLineNum = 0;
        len = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.SkuIdTableSizeMacro, phase, getSize());
    }

    private int getSize () {
        return (al.size() == 0)? 1 : al.size();
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.SkuTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.SkuIdTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
				ArrayList<String> Output = new ArrayList<String> ();

        Output.add("/* SkuIdTable */");
        Output.add("{");
				bodyStart = 2;

				if (al.size() == 0) {
						Output.add("0");
				}
        
        for (int index = 0; index < al.size(); index++) {
						String str;

						str = "/* " + alComment.get(index) + "*/ ";
						str += "/* MaxSku */ ";


						Integer[] ia = al.get(index);

						str += ia[0].toString() + ", ";
						for (int index2 = 1; index2 < ia.length; index2++) {
							 str += ia[index2].toString();
							 if (index != al.size() - 1) {
									 str += ", ";
							 }
						}

            Output.add(str);
						bodyLineNum++;

        }

				Output.add("}");

        return Output;
    }

    public int add (Token token) {

				int index;

				Integer [] skuIds = new Integer[token.maxSkuCount + 1];
				skuIds[0] = new Integer(token.maxSkuCount);
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
		private int 									bodyStart;
		private int										bodyLineNum;

    public LocalTokenNumberTable (String phase) {
        this.phase = phase;
        al = new ArrayList<String>();
				alComment = new ArrayList<String>();
				bodyStart = 0;
				bodyLineNum = 0;

        len = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.LocalTokenNumberTableSizeMacro, phase, getSize());
    }

    public int getSize () {
        return (al.size() == 0)? 1 : al.size();
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.DatabaseExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.LocalTokenNumberTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
				ArrayList<String> output = new ArrayList<String>();

        output.add("/* LocalTokenNumberTable */");
        output.add("{");
				bodyStart = 2;

				if (al.size() == 0) {
						output.add("0");
				}
        
        for (int index = 0; index < al.size(); index++) {
						String str;

						str = (String)al.get(index);

						str += " /* " + alComment.get(index) + " */ ";


						if (index != (al.size() - 1)) {
								str += ",";
						}

            output.add(str);

        }

				bodyLineNum = al.size();

				output.add("}");

        return output;
    }

    public int add (Token token) {
        int index = len;
				String str;

        len++; 

				str =  String.format(PcdDatabase.offsetOfStrTemplate, phase, token.hasDefaultValue() ? "Init" : "Uninit", token.getPrimaryKeyString());

				if (token.isStringType()) {
						str += " | PCD_TYPE_STRING";
				}

				if (token.skuEnabled) {
						str += " | PCD_TYPE_SKU_ENABLED";
				}

				if (token.hiiEnabled) {
						str += " | PCD_TYPE_HII";
				}

				if (token.vpdEnabled) {
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
		private int										bodyStart;
		private int										bodyLineNum;
		private int										base;

    public ExMapTable (String phase) {
        this.phase = phase;
        al = new ArrayList<ExTriplet>();
				alComment = new ArrayList<String>();
				bodyStart = 0;
				bodyLineNum = 0;
        len = 0;
    }

    public String getSizeMacro () {
        return String.format(PcdDatabase.ExMapTableSizeMacro, phase, getTableLen())
						 + String.format(PcdDatabase.ExTokenNumber, phase, al.size());
    }

    private int getSize () {
        return (al.size() == 0)? 1 : al.size();
    }

    public String getExistanceMacro () {
        return String.format(PcdDatabase.ExMapTableExistenceMacro, phase, (al.size() == 0)? "TRUE":"FALSE");
    }

    public String getTypeDeclaration () {
        return String.format(PcdDatabase.ExMapTableDeclaration, phase);
    }

    public ArrayList<String> getInstantiation () {
				ArrayList<String> Output = new ArrayList<String>();

        Output.add("/* ExMapTable */");
        Output.add("{");
				bodyStart = 2;

				if (al.size() == 0) {
						Output.add("{0, 0, 0}");
				}
        
				int index;
        for (index = 0; index < al.size(); index++) {
						String str;

						ExTriplet e = (ExTriplet)al.get(index);

						str = "{ " + e.exTokenNumber.toString() + ", ";
						str += e.localTokenIdx.toString() + ", ";
						str += e.guidTableIdx.toString();

						str += " /* " + alComment.get(index) + " */";

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

}

class PcdDatabase {

    public final static String ExMapTableDeclaration            = "DYNAMICEX_MAPPING ExMapTable[%s_EXMAPPING_TABLE_SIZE];\r\n";
    public final static String GuidTableDeclaration             = "EFI_GUID          GuidTable[%s_GUID_TABLE_SIZE];\r\n";
    public final static String LocalTokenNumberTableDeclaration = "UINT32            LocalTokenNumberTable[%s_LOCAL_TOKEN_NUMBER];\r\n";
    public final static String StringTableDeclaration           = "UINT16            StringTable[%s_STRING_TABLE_SIZE];\r\n";
    public final static String SizeTableDeclaration             = "UINT16            SizeTable[%s_LOCAL_TOKEN_NUMBER];\r\n";
    public final static String SkuIdTableDeclaration              = "UINT8             SkuIdTable[%s_SKUID_TABLE_SIZE];\r\n";


    public final static String ExMapTableSizeMacro              = "#define %s_EXMAPPING_TABLE_SIZE  %d\r\n";
		public final static String ExTokenNumber										= "#define %s_EX_TOKEN_NUMBER				%d\r\n";
    public final static String GuidTableSizeMacro               = "#define %s_GUID_TABLE_SIZE         %d\r\n";
    public final static String LocalTokenNumberTableSizeMacro   = "#define %s_LOCAL_TOKEN_NUMBER            %d\r\n";
    public final static String StringTableSizeMacro             = "#define %s_STRING_TABLE_SIZE       %d\r\n";
    public final static String SkuIdTableSizeMacro              = "#define %s_SKUID_TABLE_SIZE        %d\r\n";


    public final static String ExMapTableExistenceMacro         = "#define %s_EXMAP_TABLE_EMPTY    %s\r\n"; 
    public final static String GuidTableExistenceMacro          = "#define %s_GUID_TABLE_EMPTY     %s\r\n";
    public final static String DatabaseExistenceMacro           = "#define %s_DATABASE_EMPTY       %s\r\n";
    public final static String StringTableExistenceMacro        = "#define %s_STRING_TABLE_EMPTY   %s\r\n";
    public final static String SkuTableExistenceMacro           = "#define %s_SKUID_TABLE_EMPTY    %s\r\n";

		public final static String offsetOfSkuHeadStrTemplate		  	= "offsetof(%s_PCD_DATABASE, %s.%s_SkuDataTable)";
		public final static String offsetOfStrTemplate              = "offsetof(%s_PCD_DATABASE, %s.%s)";

		private StringTable stringTable;
		private GuidTable		guidTable;
		private LocalTokenNumberTable localTokenNumberTable;
		private SkuIdTable	skuIdTable;
		private SizeTable   sizeTable;
		private ExMapTable  exMapTable;

		private ArrayList<Token> alTokens;
		private String phase;
		private int assignedTokenNumber;

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

			 assignedTokenNumber = startLen;
			 this.alTokens = alTokens;
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

		private int getAlignmentSize (Token token) {
				if (token.hiiEnabled) {
						return 2;
				}

				if (token.vpdEnabled) {
						return 4;
				}

				if (token.isStringType()) {
						return 2;
				}

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
						}
						return 1;
		 }

		public String getCString () {
				return cString;
		}

		public String getHString () {
				return hString;
		}

     public void genCode () {

				final String newLine                        = "\r\n";
				final String instNewLine										= "\\\r\n";
				final String declNewLine                    = ";\r\n";
				final String tab                            = "\t";
				final String commaInstNewLine										= "\t,\\\r\n";
				final String commaNewLine										= ", \r\n";

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
				java.util.Comparator comparator = new AlignmentSizeComp();
				List<Token> list = initTokens;
				java.util.Collections.sort(list, comparator);
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
				/*
				inst = stringTable.getInstantiation();
				for (i = 0; i < inst.size(); i++ ) {
						initInstStr += tab + inst.get(i) + commaNewLine; 
				}
				*/
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
						  + initDeclStr 	+ newLine
							+ uninitDeclStr + newLine
						  + newLine;

		}

		private String genInstantiationStr (ArrayList<String> alStr) {
				String str = "";
				for (int i = 0; i< alStr.size(); i++) {
						str += "\t" + alStr.get(i);
						if (i != alStr.size() - 1) {
								str += "\r\n";
						}
				}

				return str;
		}

		private HashMap<String, ArrayList<String>> processTokens (List<Token> alToken) {

				ArrayList[]  output = new ArrayList[4];
				HashMap <String, ArrayList<String>> map = new HashMap<String, ArrayList<String>>();

				ArrayList<String> decl = new ArrayList<String>();
				ArrayList<String> declForSkuEnableType = new ArrayList<String>();
				ArrayList<String> inst = new ArrayList<String>();
				ArrayList<String> instForSkuEnableType = new ArrayList<String>();

				for (int index = 0; index < alToken.size(); index++) {
						Token token = alToken.get(index);

						if (token.skuEnabled) {
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
								if (token.hiiEnabled) {
										decl.add(getVariableEnableTypeDeclaration(token));
										inst.add(getVariableEnableInstantiation(token));
								} else if (token.vpdEnabled) {
										decl.add(getVpdEnableTypeDeclaration(token));
										inst.add(getVpdEnableTypeInstantiation(token));
								} else if (token.isStringType()) {
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
						token.assignedtokenNumber = assignedTokenNumber++;

				}

				map.put("Declaration",  decl);
				map.put("DeclarationForSku", declForSkuEnableType);
				map.put("Instantiation", inst);
				map.put("InstantiationForSku", instForSkuEnableType);

				return map;
		}

		private String getSkuEnabledTypeDeclaration (Token token) {
				return String.format("SKU_HEAD %s;\r\n", token.getPrimaryKeyString());
		}

		private String getSkuEnabledTypeInstantiaion (Token token, int SkuTableIdx) {

				String offsetof = String.format(PcdDatabase.offsetOfSkuHeadStrTemplate, phase, token.hasDefaultValue()? "Init" : "Uninit", token.getPrimaryKeyString());
				return String.format("{ %s, %d }", offsetof, SkuTableIdx);
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
						return String.format("UINT8 %s_s[%d];\r\n", token.getPrimaryKeyString(), "SkuDataTable", token.datumSize * token.maxSkuCount);
				} 

				return String.format(typeStr, token.getPrimaryKeyString(), "SkuDataTable", token.maxSkuCount);

		}

		private String getDataTypeInstantiationForSkuEnabled (Token token) {
				String str = "";

				if (token.datumType == Token.DATUM_TYPE.POINTER) {
						return String.format("UINT8 %s_s[%d]", token.getPrimaryKeyString(), "SkuDataTable", token.datumSize * token.maxSkuCount);
				} else {
						str = "{ ";
						for (int idx = 0; idx < token.maxSkuCount; idx++) {
								str += token.skuData.get(idx).toString();
								if (idx != token.maxSkuCount - 1) {
										str += ", ";
								}
						}
						str += "}";

						return str;
				}

		}

		private String getDataTypeInstantiation (Token token) {

				String typeStr = "";

				if (token.datumType == Token.DATUM_TYPE.POINTER) {
						return String.format("%s /* %s */", token.datum.toString(), token.getPrimaryKeyString());
				} else {
						return String.format("%s /* %s */", token.datum.toString(), token.getPrimaryKeyString());
				}
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

				return String.format("%s		%s", typeStr, token.getPrimaryKeyString());
		}

		private String getVpdEnableTypeDeclaration (Token token) {
				return String.format("VPD_HEAD %s", token.getPrimaryKeyString());
		}

		private String getVpdEnableTypeInstantiation (Token token) {
				return String.format("{ %d } /* %s */", token.vpdOffset,
																								token.getPrimaryKeyString());
		}

		private String getStringTypeDeclaration (Token token) {
				return String.format("UINT16	%s", token.getPrimaryKeyString());
		}

		private String getStringTypeInstantiation (int StringTableIdx, Token token) {
				return String.format ("%d /* %s */", StringTableIdx,
																						 token.getPrimaryKeyString()); 
		}


		private String getVariableEnableTypeDeclaration (Token token) {
			return String.format("VARIABLE_HEAD  %s", token.getPrimaryKeyString());
		}

		private String getVariableEnableInstantiation (Token token) {
				return String.format("{ %d, %d, %d } /* %s */", guidTable.add(token.variableGuid, token.getPrimaryKeyString()),
																												stringTable.add(token.variableName, token),
																												token.variableOffset, 
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
						System.out.println(GlobalData.getWorkspacePath());
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
      1) Get all token's platform information from FPD, and create token object into memory database.
      2) Get all token's module information from MSA, and create usage instance for every module's PCD entry.
      3) Get all token's inherited information from MSA's library, and create usage instance 
         for module who consume this library and create usage instance for library for building.
      4) Collect token's package information from SPD, update these information for token in memory
         database.
      5) Generate 3 strings for a) All modules using Dynamic(Ex) PCD entry. (Token Number)
                                b) PEI PCD Database (C Structure) for PCD Service PEIM
                                c) DXE PCD Database (C structure) for PCD Service DXE
                                
      
      @throws  EntityException Exception indicate failed to execute this action.
      
    **/
    private void execute() throws EntityException {
        FrameworkPlatformDescriptionDocument fpdDoc               = null;
        Object[][]                           modulePCDArray       = null;
        Map<String, XmlObject>               docMap               = null;
        ModuleSADocument.ModuleSA[]          moduleSAs            = null;
        UsageInstance                        usageInstance        = null;
        String                               packageName          = null;
        String                               packageFullPath      = null;
        int                                  index                = 0;
        int                                  libraryIndex         = 0;
        int                                  pcdArrayIndex        = 0;
        List<String>                         listLibraryInstance  = null;
        String                               componentTypeStr     = null;

        //
        // Collect all PCD information defined in FPD file.
        // Evenry token defind in FPD will be created as an token into 
        // memory database.
        //
        fpdDoc = createTokenInDBFromFPD();

        //
        // Searching MSA and SPD document. 
        // The information of MSA will be used to create usage instance into database.
        // The information of SPD will be used to update the token information in database.
        //

        HashMap<String, XmlObject> map = new HashMap<String, XmlObject>();
        map.put("FrameworkPlatformDescription", fpdDoc);
        SurfaceAreaQuery.setDoc(map);    

        moduleSAs = SurfaceAreaQuery.getFpdModules();
        for(index = 0; index < moduleSAs.length; index ++) {
            //
            // Get module document and use SurfaceAreaQuery to get PCD information
            //
            docMap = GlobalData.getDoc(moduleSAs[index].getModuleName());
            SurfaceAreaQuery.setDoc(docMap);
            modulePCDArray    = SurfaceAreaQuery.getModulePCDTokenArray();
            componentTypeStr  = SurfaceAreaQuery.getComponentType();
            packageName       = 
                GlobalData.getPackageNameForModule(moduleSAs[index].getModuleName());
            packageFullPath   = this.workspacePath + File.separator    +
                                GlobalData.getPackagePath(packageName) +
                                packageName + ".spd";

            if(modulePCDArray != null) {
                //
                // If current MSA contains <PCDs> information, then create usage
                // instance for PCD information from MSA
                //
                for(pcdArrayIndex = 0; pcdArrayIndex < modulePCDArray.length; 
                     pcdArrayIndex ++) {
                    usageInstance = 
                        createUsageInstanceFromMSA(moduleSAs[index].getModuleName(),
                                                   modulePCDArray[pcdArrayIndex]);

                    if(usageInstance == null) {
                        continue;
                    }
                    //
                    // Get remaining PCD information from the package which this module belongs to
                    //
                    updateTokenBySPD(usageInstance, packageFullPath);
                }
            }

            //
            // Get inherit PCD information which inherit from library instance of this module.
            //
            listLibraryInstance = 
                SurfaceAreaQuery.getLibraryInstance(moduleSAs[index].getArch().toString(),
                                                    CommonDefinition.AlwaysConsumed);
            if(listLibraryInstance != null) {
                for(libraryIndex = 0; libraryIndex < listLibraryInstance.size(); 
                     libraryIndex ++) {
                    inheritPCDFromLibraryInstance(listLibraryInstance.get(libraryIndex),
                                                  moduleSAs[index].getModuleName(),
                                                  packageName,
                                                  componentTypeStr);
                }
            }
        }
        
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

    private void genPcdDatabaseSourceCode			()
			throws EntityException {
				String PcdCommonHeaderString = PcdDatabase.getPcdDatabaseCommonDefinitions ();

				ArrayList<Token> alPei = new ArrayList<Token> ();
				ArrayList<Token> alDxe = new ArrayList<Token> ();

				dbManager.getTwoPhaseDynamicRecordArray(alPei, alDxe);
        PcdDatabase pcdPeiDatabase = new PcdDatabase (alPei, "PEI", 0);
				pcdPeiDatabase.genCode();
				dbManager.PcdPeimHString        = PcdCommonHeaderString + pcdPeiDatabase.getHString()
																						+ PcdDatabase.getPcdPeiDatabaseDefinitions();
				dbManager.PcdPeimCString				= pcdPeiDatabase.getCString();

        PcdDatabase pcdDxeDatabase = new PcdDatabase (alDxe, 
																											"DXE",
																											alPei.size()
																											);
				pcdDxeDatabase.genCode();
				dbManager.PcdDxeHString   = dbManager.PcdPeimHString + pcdDxeDatabase.getHString()
																			+ PcdDatabase.getPcdDxeDatabaseDefinitions();
				dbManager.PcdDxeCString		= pcdDxeDatabase.getCString();
		}

		/**
      This function will collect inherit PCD information from library for a module.
     
      This function will create two usage instance for inherited PCD token, one is 
      for module and another is for library.
      For module, if it inherited a PCD token from library, this PCD token's value 
      should be instanced in module level, and belongs to module.
      For library, it also need a usage instance for build.
      
      @param libraryName         The name of library instance.
      @param moduleName          The name of module.
      @param packageName         The name of package while module belongs to.
      @param parentcomponentType The component type of module.
      
      @throws EntityException  If the token does *not* exist in memory database.
      
    **/
    private void inheritPCDFromLibraryInstance(String libraryName,
                                               String moduleName,
                                               String packageName,
                                               String parentcomponentType) 
        throws EntityException {
        Map<String, XmlObject>  docMap            = null;
        String                  primaryKeyString  = null;
        Object[][]              libPcdDataArray   = null;
        UUID                    nullUUID          = new UUID(0,0);
        UUID                    platformUUID      = nullUUID;
        UUID                    tokenSpaceGuid    = null;
        int                     tokenIndex        = 0;
        Token                   token             = null;
        Token.PCD_TYPE          pcdType           = Token.PCD_TYPE.UNKNOWN;
        UsageInstance           usageInstance     = null;
        String                  packageFullPath   = null;

        //
        // Query PCD information from library's document.
        //
        docMap          = GlobalData.getDoc(libraryName);
        SurfaceAreaQuery.setDoc(docMap);
        libPcdDataArray = SurfaceAreaQuery.getModulePCDTokenArray();

        if(libPcdDataArray == null) {
            return;
        }

        for(tokenIndex = 0; tokenIndex < libPcdDataArray.length; tokenIndex ++) {
            tokenSpaceGuid =((UUID)libPcdDataArray[tokenIndex][2] == null) ? 
                             nullUUID :(UUID)libPcdDataArray[tokenIndex][2];

            //
            // Get token from memory database. The token must be created from FPD already.
            //
            primaryKeyString = Token.getPrimaryKeyString((String)libPcdDataArray[tokenIndex][0],
                                                         tokenSpaceGuid,
                                                         platformUUID
                                                         );

            if(dbManager.isTokenInDatabase(primaryKeyString)) {
                token = dbManager.getTokenByKey(primaryKeyString);
            } else {
                throw new EntityException("The PCD token " + primaryKeyString + 
                                          " defined in module " + moduleName + 
                                          " does not exist in FPD file!");
            }      

            //
            // Create usage instance for module.
            //
            pcdType = Token.getpcdTypeFromString((String)libPcdDataArray[tokenIndex][1]);
            usageInstance = new UsageInstance(token,
                                              Token.PCD_USAGE.ALWAYS_CONSUMED,
                                              pcdType,
                                              CommonDefinition.getComponentType(parentcomponentType),
                                              libPcdDataArray[tokenIndex][3],
                                              null,
                                             (String) libPcdDataArray[tokenIndex][5],
                                              "",
                                              moduleName,
                                              packageName,
                                              true);
            if(Token.PCD_USAGE.UNKNOWN == token.isUsageInstanceExist(moduleName)) {
                token.addUsageInstance(usageInstance);

                packageFullPath = this.workspacePath + File.separator    +
                                  GlobalData.getPackagePath(packageName) +
                                  packageName + ".spd";
                updateTokenBySPD(usageInstance, packageFullPath);
            }

            //
            // We need create second usage instance for inherited case, which
            // add library as an usage instance, because when build a module, and 
            // if module inherited from base library, then build process will build
            // library at first. 
            //
            if(Token.PCD_USAGE.UNKNOWN == token.isUsageInstanceExist(libraryName)) {
                packageName   = GlobalData.getPackageNameForModule(libraryName);
                usageInstance = new UsageInstance(token,
                                                  Token.PCD_USAGE.ALWAYS_CONSUMED,
                                                  pcdType,
                                                  CommonDefinition.ComponentTypeLibrary,
                                                  libPcdDataArray[tokenIndex][3],
                                                  null,
                                                 (String)libPcdDataArray[tokenIndex][5],
                                                  "",
                                                  libraryName,
                                                  packageName,
                                                  false);
                token.addUsageInstance(usageInstance);
            }
        }
    }

    /**
      Create usage instance for PCD token defined in MSA document

      A PCD token maybe used by many modules, and every module is one of usage
      instance of this token. For ALWAY_CONSUMED, SOMETIMES_CONSUMED, it is 
      consumer type usage instance of this token, and for ALWAYS_PRODUCED, 
      SOMETIMES_PRODUCED, it is produce type usage instance.
     
      @param moduleName      The name of module 
      @param tokenInfoInMsa  The PCD token information array retrieved from MSA.
      
      @return UsageInstance  The usage instance created in memroy database.
      
      @throws EntityException  If token did not exist in database yet.
      
    **/
    private UsageInstance createUsageInstanceFromMSA(String   moduleName,
                                                     Object[] tokenInfoInMsa) 
        throws EntityException {
        String          packageName         = null;
        UsageInstance   usageInstance       = null;
        UUID            tokenSpaceGuid      = null;
        UUID            nullUUID            = new UUID(0,0);
        String          primaryKeyString    = null;
        UUID            platformTokenSpace  = nullUUID;
        Token           token               = null;
        Token.PCD_TYPE  pcdType             = Token.PCD_TYPE.UNKNOWN;
        Token.PCD_USAGE pcdUsage            = Token.PCD_USAGE.UNKNOWN;

        tokenSpaceGuid =((UUID)tokenInfoInMsa[2] == null) ? nullUUID :(UUID)tokenInfoInMsa[2];

        primaryKeyString = Token.getPrimaryKeyString((String)tokenInfoInMsa[0],
                                                     tokenSpaceGuid,
                                                     platformTokenSpace);

        //
        // Get token object from memory database firstly.
        //
        if(dbManager.isTokenInDatabase(primaryKeyString)) {
            token = dbManager.getTokenByKey(primaryKeyString);
        } else {
            throw new EntityException("The PCD token " + primaryKeyString + " defined in module " + 
                                      moduleName + " does not exist in FPD file!" );
        }
        pcdType     = Token.getpcdTypeFromString((String)tokenInfoInMsa[1]);
        pcdUsage    = Token.getUsageFromString((String)tokenInfoInMsa[4]);

        packageName = GlobalData.getPackageNameForModule(moduleName);

        if(Token.PCD_USAGE.UNKNOWN != token.isUsageInstanceExist(moduleName)) {
            //
            // BUGBUG: It is legal that same base name exist in one FPD file. In furture
            //         we should use "Guid, Version, Package" and "Arch" to differ a module.
            //         So currently, warning should be disabled.
            //
            //ActionMessage.warning(this,
            //                      "In module " + moduleName + " exist more than one PCD token " + token.cName
            //                      );
            return null;
        }

        //
        // BUGBUG: following code could be enabled at current schema. Because 
        //         current schema does not provide usage information.
        // 
        // For FEATRURE_FLAG, FIXED_AT_BUILD, PATCH_IN_MODULE type PCD token, his 
        // usage is always ALWAYS_CONSUMED
        //
        //if((pcdType != Token.PCD_TYPE.DYNAMIC) &&
        //   (pcdType != Token.PCD_TYPE.DYNAMIC_EX)) {
        pcdUsage = Token.PCD_USAGE.ALWAYS_CONSUMED;
        //}

        usageInstance = new UsageInstance(token,
                                          pcdUsage,
                                          pcdType,
                                          CommonDefinition.getComponentType(SurfaceAreaQuery.getComponentType()),
                                          tokenInfoInMsa[3],
                                          null,
                                         (String) tokenInfoInMsa[5],
                                          "",
                                          moduleName,
                                          packageName,
                                          false);

        //
        // Use default value defined in MSA to update datum of token,
        // if datum of token does not defined in FPD file.
        //
        if((token.datum == null) &&(tokenInfoInMsa[3] != null)) {
            token.datum = tokenInfoInMsa[3];
        }

        token.addUsageInstance(usageInstance);

        return usageInstance;
    }

    /**
      Create token instance object into memory database, the token information
      comes for FPD file. Normally, FPD file will contain all token platform 
      informations.
     
      This fucntion should be executed at firsly before others collection work
      such as searching token information from MSA, SPD.
      
      @return FrameworkPlatformDescriptionDocument   The FPD document instance for furture usage.
      
      @throws EntityException                        Failed to parse FPD xml file.
      
    **/
    private FrameworkPlatformDescriptionDocument createTokenInDBFromFPD() 
        throws EntityException {
        XmlObject                            doc               = null;
        FrameworkPlatformDescriptionDocument fpdDoc            = null;
        int                                  index             = 0;
        List<PcdBuildData>                   pcdBuildDataArray = new ArrayList<PcdBuildData>();
        PcdBuildData                         pcdBuildData      = null;
        Token                                token             = null;
        UUID                                 nullUUID          = new UUID(0,0);
        UUID                                 platformTokenSpace= nullUUID;
        List                                 skuDataArray      = new ArrayList();
        SkuInstance                          skuInstance       = null;
        int                                  skuIndex          = 0;

        //
        // Get all tokens from FPD file and create token into database.
        // 

        try {
            doc = XmlObject.Factory.parse(new File(fpdFilePath));
        } catch(IOException ioE) {
            throw new EntityException("Can't find the FPD xml fle:" + fpdFilePath);
        } catch(XmlException xmlE) {
            throw new EntityException("Can't parse the FPD xml fle:" + fpdFilePath);
        }

        //
        // Get memoryDatabaseManager instance from GlobalData.
        //
        if((dbManager = GlobalData.getPCDMemoryDBManager()) == null) {
            throw new EntityException("The instance of PCD memory database manager is null");
        }

        dbManager = new MemoryDatabaseManager();

        if(!(doc instanceof FrameworkPlatformDescriptionDocument)) {
            throw new EntityException("File " + fpdFilePath + 
                                       " is not a FrameworkPlatformDescriptionDocument");
        }

        fpdDoc =(FrameworkPlatformDescriptionDocument)doc;

        //
        // Add all tokens in FPD into Memory Database.
        //
        pcdBuildDataArray = 
            fpdDoc.getFrameworkPlatformDescription().getPcdBuildDeclarations().getPcdBuildDataList();
        for(index = 0; 
             index < fpdDoc.getFrameworkPlatformDescription().getPcdBuildDeclarations().sizeOfPcdBuildDataArray(); 
             index ++) {
            pcdBuildData = pcdBuildDataArray.get(index);
            token        = new Token(pcdBuildData.getCName(), new UUID(0, 0), new UUID(0, 0));
            //
            // BUGBUG: in FPD, <defaultValue> should be defined as <Value>
            //
            token.datum        = pcdBuildData.getDefaultValue();
            token.tokenNumber  = Integer.decode(pcdBuildData.getToken().getStringValue());
            token.hiiEnabled   = pcdBuildData.getHiiEnable();
            token.variableGuid = Token.getGUIDFromSchemaObject(pcdBuildData.getVariableGuid());
            token.variableName = pcdBuildData.getVariableName();
            token.variableOffset = Integer.decode(pcdBuildData.getDataOffset());
            token.skuEnabled   = pcdBuildData.getSkuEnable();
            token.maxSkuCount  = Integer.decode(pcdBuildData.getMaxSku());
            token.skuId        = Integer.decode(pcdBuildData.getSkuId());
            token.skuDataArrayEnabled  = pcdBuildData.getSkuDataArrayEnable();
            token.assignedtokenNumber  = Integer.decode(pcdBuildData.getToken().getStringValue());
            skuDataArray               = pcdBuildData.getSkuDataArray1();
            token.datumType    = Token.getdatumTypeFromString(pcdBuildData.getDatumType().toString());
						token.datumSize    = pcdBuildData.getDatumSize();

            if(skuDataArray != null) {
                for(skuIndex = 0; skuIndex < skuDataArray.size(); skuIndex ++) {
                    //
                    // BUGBUG: Now in current schema, The value is defined as String type, 
                    // it is not correct, the type should be same as the datumType
                    //
                    skuInstance = new SkuInstance(((PcdBuildData.SkuData)skuDataArray.get(skuIndex)).getId(),
                                                  ((PcdBuildData.SkuData)skuDataArray.get(skuIndex)).getValue());
                    token.skuData.add(skuInstance);
                }
            }

            if(dbManager.isTokenInDatabase(Token.getPrimaryKeyString(token.cName, 
                                                                      token.tokenSpaceName, 
                                                                      platformTokenSpace))) {
                //
                // If found duplicate token, Should tool be hold?
                //
                ActionMessage.warning(this, 
                                       "Token " + token.cName + " exists in token database");
                continue;
            }
            token.pcdType = Token.getpcdTypeFromString(pcdBuildData.getItemType().toString());
            dbManager.addTokenToDatabase(Token.getPrimaryKeyString(token.cName, 
                                                                   token.tokenSpaceName, 
                                                                   platformTokenSpace), 
                                         token);
        }

        return fpdDoc;
    }

    /**
      Update PCD token in memory database by help information in SPD.

      After create token from FPD and create usage instance from MSA, we should collect
      PCD package level information from SPD and update token information in memory 
      database.
      
      @param usageInstance   The usage instance defined in MSA and want to search in SPD.
      @param packageFullPath The SPD file path.
      
      @throws EntityException Failed to parse SPD xml file.
      
    **/
    private void updateTokenBySPD(UsageInstance  usageInstance,
                                  String         packageFullPath) 
        throws EntityException {
        PackageSurfaceAreaDocument      pkgDoc          = null;
        PcdDefinitions                  pcdDefinitions  = null;
        List<PcdDefinitions.PcdEntry>   pcdEntryArray   = new ArrayList<PcdDefinitions.PcdEntry>();
        int                             index           = 0;
        boolean                         isFoundInSpd    = false;
        Token.DATUM_TYPE                datumType       = Token.DATUM_TYPE.UNKNOWN;

        try {
            pkgDoc =(PackageSurfaceAreaDocument)XmlObject.Factory.parse(new File(packageFullPath));
        } catch(IOException ioE) {
            throw new EntityException("Can't find the FPD xml fle:" + packageFullPath);
        } catch(XmlException xmlE) {
            throw new EntityException("Can't parse the FPD xml fle:" + packageFullPath);
        }
        pcdDefinitions = pkgDoc.getPackageSurfaceArea().getPcdDefinitions();
        //
        // It is illege for SPD file does not contains any PCD information.
        //
        if (pcdDefinitions == null) {
            return;
        }

        pcdEntryArray = pcdDefinitions.getPcdEntryList();
        if (pcdEntryArray == null) {
            return;
        }
        for(index = 0; index < pcdEntryArray.size(); index ++) {
            if(pcdEntryArray.get(index).getCName().equalsIgnoreCase(
                usageInstance.parentToken.cName)) {
                isFoundInSpd = true;
                //
                // From SPD file , we can get following information.
                //  Token:        Token number defined in package level.
                //  PcdItemType:  This item does not single one. It means all supported item type.
                //  datumType:    UINT8, UNIT16, UNIT32, UINT64, VOID*, BOOLEAN 
                //  datumSize:    The size of default value or maxmine size.
                //  defaultValue: This value is defined in package level.
                //  HelpText:     The help text is provided in package level.
                //

                usageInstance.parentToken.tokenNumber = Integer.decode(pcdEntryArray.get(index).getToken());

                if(pcdEntryArray.get(index).getDatumType() != null) {
                    datumType = Token.getdatumTypeFromString(
                        pcdEntryArray.get(index).getDatumType().toString());
                    if(usageInstance.parentToken.datumType == Token.DATUM_TYPE.UNKNOWN) {
                        usageInstance.parentToken.datumType = datumType;
                    } else {
                        if(datumType != usageInstance.parentToken.datumType) {
                            throw new EntityException("Different datum types are defined for Token :" + 
                                                      usageInstance.parentToken.cName);
                        }
                    }

                } else {
                    throw new EntityException("The datum type for token " + usageInstance.parentToken.cName + 
                                              " is not defind in SPD file " + packageFullPath);
                }

                usageInstance.defaultValueInSPD = pcdEntryArray.get(index).getDefaultValue();
                usageInstance.helpTextInSPD     = "Help Text in SPD";

                //
                // If token's datum is not valid, it indicate that datum is not provided
                // in FPD and defaultValue is not provided in MSA, then use defaultValue
                // in SPD as the datum of token.
                //
                if(usageInstance.parentToken.datum == null) {
                    if(pcdEntryArray.get(index).getDefaultValue() != null) {
                        usageInstance.parentToken.datum = pcdEntryArray.get(index).getDefaultValue();
                    } else {
                        throw new EntityException("FPD does not provide datum for token " + usageInstance.parentToken.cName +
                                                  ", MSA and SPD also does not provide <defaultValue> for this token!");
                    }
                }
            }
        }
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
        ca.setWorkspacePath("G:/mdk");
        ca.setFPDFilePath("G:/mdk/EdkNt32Pkg/build/Nt32.fpd");
        ca.setActionMessageLevel(ActionMessage.MAX_MESSAGE_LEVEL);
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            "G:/mdk");
        ca.execute();
    }
}
