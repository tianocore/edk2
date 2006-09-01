package org.tianocore.context;

public class HelpInfo {

    
    /** 
     * output the tools usage guide
     * @param no input parameter
     * @return no return value
     **/
    public static void outputUsageInfo() {
        System.out.printf("\n%s", UsageInfoString);
        System.out.printf("\n%s", DetailOfOptionString);

        for (int i = 0; i < 6; i++) {
            outputSubUsageInfo(UsageString[i], UsageStringInfo[i]);
        }
    }

    /** 
     * output the usage info which bases on cmd option 
     * @param String str1 : the cmd option
     *         String str2 : the detail of cmd option
     * @return no return value
     **/
    private static void outputSubUsageInfo(String str1, String str2) {
        splitString(str2);
        if (substrnum > 0) {
            System.out.printf("\n%4s %-30s %s", "", str1, substr[0]);
            for (int i = 1; i < substrnum; i++) {
                if (substr[i] != null)
                    System.out.printf("\n%4s %-30s %s", "", "", substr[i]);
            }
            substrnum = 0;
        } else {
            System.out.printf("\n%4s %-30s %s", "", str1, str2);
        }
    }

    /** 
     * according to the output width, split the detail info  
     * @param String str :the detail info
     * @return no return value
     **/
    private static void splitString(String str) {
        int strlength = str.length();
        if (strlength > MaxSrtingLength) {
            
            //we should modify the array to list, for it is strange to + 2
            substrnum = strlength / MaxSrtingLength + 2;
            String[] tokens = str.split("[ ]", 0);
            substr = new String[substrnum];
            int templength = 0;
            int j = 0;
            int start = 0;
            int end = 0;
            for (int i = 0; i < tokens.length; i++) {
                if ((templength = end + tokens[i].length() + 1) < (MaxSrtingLength + start)) {
                    end = templength;
                } else {
                    substr[j++] = str.substring(start, end);
                    start = end;
                    i = i - 1;
                }
            }
            substr[j] = str.substring(start, end - 1);
        }
    }

    private static String[] substr = null;

    private static int substrnum = 0;

    private static final int MaxSrtingLength = 40;

    private static final String UsageInfoString = "Usage: context [-option1] [args] [-option2] [args] ...";

    private static final String DetailOfOptionString = "Where options include:";

    private static final String HString = "-h";

    private static final String HStringInfo = "print this help message";

    private static final String AString = "-a  <list of Arch>";

    private static final String AStringInfo = "what kind of architechure is the binary target, such as IA32, IA64, X64, EBC, or ARM. Multiple values can be specified on a single line, using space to separate the values.";

    private static final String CString = "-c  <tool_definition_file.txt>";

    private static final String CStringInfo = "Assign a txt file, which specify the tools to use for the build and must be located in the path: WORKSPACE/Tools/Conf/. If no file is specified, the default filename is \"tools_def.txt\"";

    private static final String NString = "-n  <list of TagNames>";

    private static final String NStringInfo = "Specify the TagName, such as GCC, MSFT, which are defined in the \"tool_definition_file.txt\"";

    private static final String PString = "-p  <*.fpd>";

    private static final String PStringInfo = "Specify the WORKSPACE relative Path and Filename of platform FPD file that will be used for the build.";

    private static final String TString = "-t  <list of Build Targets>";

    private static final String TStringInfo = "What kind of the version is the binary target, such as DEBUG, RELEASE. Multiple values can be specified on a single line, using space to separate the values.";

    private static final String[] UsageString = { HString, AString, CString,
            NString, PString, TString };

    private static final String[] UsageStringInfo = { HStringInfo, AStringInfo,
            CStringInfo, NStringInfo, PStringInfo, TStringInfo };
}
