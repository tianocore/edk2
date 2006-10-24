/** @file
  File is HelpInfo class which is used to output the usage info. 
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

package org.tianocore.context;

import java.util.LinkedList;

public class HelpInfo {

    
    /** 
     * output the tools usage guide
     * @param no input parameter
     * @return no return value
     **/
    public static void outputUsageInfo() {
        System.out.printf("\n%s", DescripationString);
        System.out.printf("\n%s", UsageInfoString);
        System.out.printf("\n%s", DetailOfOptionString);

        for (int i = 0; i < settingnum; i++) {
            outputSubUsageInfo(UsageString[i], UsageStringInfo[i]);
        }
        
        System.out.printf("\n%s", ExampleString);
        System.out.printf("\n%s", str1);
        System.out.printf("\n%s", str2);
        System.out.printf("\n%s", str3);
        System.out.printf("\n%s", str4);
    }

    /** 
     * output the usage info which bases on cmd option 
     * @param String str1 : the cmd option
     *         String str2 : the detail of cmd option
     * @return no return value
     **/
    private static void outputSubUsageInfo(String str1, String str2) {
        
        splitString(str2);
        System.out.printf("\n%4s %-30s %s", "", str1, List.get(0));
        for (int i=1; i<List.size(); i++){
            System.out.printf("\n%4s %-30s %s", "", "", List.get(i));
        }
        List.clear();
    }

    /** 
     * according to the output width, split the detail info  
     * @param String str :the detail info
     * @return no return value
     **/
    private static void splitString(String str) {
        int strlength = str.length();
        if (strlength > MaxSrtingLength) {
            String[] tokens = str.split("[ ]", 0);
            String tempstr = null;
            int templength = 0;
            int start = 0;
            int end = 0;
            for (int i = 0; i < tokens.length; i++) {
                if ((templength = end + tokens[i].length() + 1) < (MaxSrtingLength + start)) {
                    end = templength;
                } else {
                    tempstr = str.substring(start, end);
                    List.add(tempstr);
                    start = end;
                    i = i - 1;
                }
            }
            tempstr = str.substring(start, end - 1);
            List.add(tempstr);
        } else {
            List.add(str);
        }
    }

    
    private static LinkedList<String> List = new LinkedList<String>();

    private static final int MaxSrtingLength = 40;
    
    private static final int settingnum = 7;
    
    private static final String DescripationString = "The purpose of this tool is modifying the settings in target.txt";
    
    private static final String UsageInfoString = "Usage: ContextTool [-option1] [args] [-option2] [args] ...";

    private static final String DetailOfOptionString = "Where options include:";
    
    private static final String ExampleString = "Example: ContextTool -a IA32 IA64 EBC -c Tools/Conf/tools_def.txt -t DEBUG -n GCC -p EdkNt32Pkg/Nt32.fpd -m 2\n";

    private static final String str1 = "show current sub setting: ContextTool -x";
    
    private static final String str2 = "show possible sub setting: ContextTool -x ?";
    
    private static final String str3 = "clean current sub setting: ContextTool -x 0";
    
    private static final String str4 = "x is the sub setting option, such as p, a, n, m, t, c.\n";
    
    private static final String HString = "-h";

    private static final String HStringInfo = "print usage info";

    private static final String AString = "-a  <list of Arch>";

    private static final String AStringInfo = "What kind of architechure is the binary target, such as IA32, IA64, X64, EBC, or ARM. Multiple values can be specified on a single line, using space to separate the values.";

    private static final String CString = "-c  <tool_definition_file.txt>";

    private static final String CStringInfo = "Assign a txt file with the relative path to WORKSPACE, which specify the tools to use for the build and must be located in the path: WORKSPACE/Tools/Conf/. If no file is specified, the default filename is \"tools_def.txt\"";

    private static final String NString = "-n  <list of TagNames>";

    private static final String NStringInfo = "Specify the TagName, such as GCC, MSFT, which are defined in the \"tool_definition_file.txt\"";

    private static final String PString = "-p  <*.fpd>";

    private static final String PStringInfo = "Specify the WORKSPACE relative Path and Filename of platform FPD file that will be used for the build.";

    private static final String TString = "-t  <list of Build Targets>";

    private static final String TStringInfo = "What kind of the version is the binary target, such as DEBUG, RELEASE. Multiple values can be specified on a single line, using space to separate the values.";

    private static final String MString = "-m  <num of Threads>";
    
    private static final String MStringInfo = "The number of concurrent threads. Default is 2. Recommend to set this value to one more than the number of your compurter cores or CPUs. 0 will disable MULTIPLE_THREAD and clean MAX_CONCURRENT_THREAD_NUMBER.";
    
    private static final String[] UsageString = { HString, AString, CString,
            NString, PString, TString, MString };

    private static final String[] UsageStringInfo = { HStringInfo, AStringInfo,
            CStringInfo, NStringInfo, PStringInfo, TStringInfo, MStringInfo };
}
