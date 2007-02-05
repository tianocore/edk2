/** @file
  File is ParseParameter class which is used to parse the validity of user's input args
  and standardize them. 
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.context;

public class ParseParameter {

    
    /** 
     * check the validity of user's input args
     * @param args -- user's input
     * @return true or false
     **/
    public static boolean checkParameter(String[] args) {
        
        if(args.length == 0){
            TargetFile.readFile();
            outputCurSetting();
            return false;
        } else {
            if( (args[0].compareToIgnoreCase("-h") == 0) || (args[0].compareToIgnoreCase("/h") == 0) || 
                (args[0].compareToIgnoreCase("--help") == 0) || (args[0].compareToIgnoreCase("/help") == 0) ){
                HelpInfo.outputUsageInfo();
                return false;
            }
            if( args[0].charAt(0) != '-' ){
                System.out.printf("%s\n", "Error Parameters! Please type \"ContextTool -h\" for helpinfo.");
                return false;
            }
            for(int i=0; i<args.length; i++){
                if( (args[i].startsWith("-") && 
                    ((args[i].compareTo("-a") != 0) && (args[i].compareTo("-c") != 0) && 
                    (args[i].compareTo("-n") != 0) && (args[i].compareTo("-p") != 0) && 
                    (args[i].compareTo("-t") != 0) && (args[i].compareTo("-m") != 0)))){
                    System.out.printf("%s\n", "Error Parameters! Please type \"ContextTool -h\" for helpinfo.");                                                                                                                                                            
                    return false;
                }
            }
        }
        
        return true; 
    }
    
    /** 
     * standardize user's input args
     * @param args -- user's input
     * @return no return value
     **/
    public static int standardizeParameter(String[] args) {
        
        
        StringBuffer InputData = new StringBuffer();
        for (int i = 0; i < args.length; i++) {
            InputData.append(args[i]);
            InputData.append(" ");
        }

        int i = 0;
        while (i < InputData.length()) {
            int j = InputData.indexOf("-", i + 1);
            if (j == -1)
                j = InputData.length();

            String argstr = InputData.substring(i, j);
            i = j;
            if (argstr.charAt(1) == 'p') {
                //
                // argstr is "-p ", display current setting
                //
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    if (curpstr != null) {
                        System.out.printf("%s\n", curpstr);
                    } else {
                        System.out.printf("No ACTIVE_PLATFORM defined \n");
                    }
                    return 1;
                }
                //
                //argstr is "-p ? ", display possible setting
                //
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    String workspacePath = System.getenv("WORKSPACE");
                    System.out.printf( "%s\n", "Assign a platform FPD file with relative path to " + workspacePath);
                    return 2;
                }
                //
                //argstr is "-p 0 ", clean current setting
                //
                if(argstr.length() < 6 && argstr.charAt(3) == '0'){
                    curpstr = pstr;
                    npflag = true;
                    continue;
                }
                String[] S = argstr.split(" ");
                if(S.length > 2){
                    System.out.printf( "%s\n", "There should be none or only one ACTIVE_PLATFORM. Please check the number of value which follow \"-p\".");
                    return 3;
                }
                curpstr = pstr.concat(argstr.substring(2));
                npflag = true;
            } else if (argstr.charAt(1) == 't') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    if (curtstr != null) {
                        System.out.printf("%s\n", curtstr);
                    } else {
                        System.out.printf("No TARGET defined\n");
                    }
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "What kind of the version is the binary target, such as DEBUG, RELEASE." );
                    return 2;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '0'){
                    curtstr = tstr;
                    ntflag = true;
                    continue;
                }
                curtstr = tstr.concat(argstr.substring(2));
                ntflag = true;
            } else if (argstr.charAt(1) == 'a') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    if (curastr != null) {
                        System.out.printf("%s\n", curastr);
                    } else {
                        System.out.printf("No TARGET_ARCH defined\n");
                    }
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "What kind of architechure is the binary target, such as IA32, IA64, X64, EBC, or ARM." );
                    return 2;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '0'){
                    curastr = astr;
                    naflag = true;
                    continue;
                }
                curastr = astr.concat(argstr.substring(2));
                naflag = true;
            } else if (argstr.charAt(1) == 'c') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    if (curcstr != null) {
                        System.out.printf("%s\n", curcstr);
                    } else {
                        System.out.printf("No TOOL_CHAIN_CONF defined\n");
                    }
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    String workspacePath = System.getenv("WORKSPACE");
                    System.out.printf( "%s\n", "Assign a txt file with relative path to " + workspacePath + ", which specify the tools to use for the build and must be located in the path:" + workspacePath + "\\Tools\\Conf" );
                    return 2;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '0'){
                    curcstr = cstr;
                    ncflag = true;
                    continue;
                }
                String[] S = argstr.split(" ");
                if(S.length > 2){
                    System.out.printf( "%s\n", "There should be one and only one TOOL_CHAIN_CONF. Please check the number of value which follow \"-c\".");
                    return 3;
                }
                curcstr = cstr.concat(argstr.substring(2));
                ncflag = true;
            } else if (argstr.charAt(1) == 'n') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    if (curnstr != null) {
                        System.out.printf("%s\n", curnstr);
                    } else {
                        System.out.printf("No TOOL_CHAIN_TAG defined\n");
                    }
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "Specify the TagName, such as GCC, MSFT." );
                    return 2;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '0'){
                    curnstr = nstr;
                    nnflag = true;
                    continue;
                }
                curnstr = nstr.concat(argstr.substring(2));
                nnflag = true;
            } else if (argstr.charAt(1) == 'm') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    if (curmstr != null) {
                        System.out.printf("%s\n", curmstr);
                    } else {
                        System.out.printf("No MAX_CONCURRENT_THREAD_NUMBER defined\n");
                    }
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "The number of concurrent threads. Default is 2. Recommend to set this value to one more than the number of your compurter cores or CPUs." );
                    return 2;
                }
                String[] S = argstr.split(" ");
                if(S.length > 2){
                    System.out.printf( "%s\n", "There should be one and only one integer, which doesn't include space.");
                    return 3;
                }
                mstr += argstr.substring(2);
                curmstr = mstr;
                nmflag = true;
                if (argstr.charAt(3) == '0'){
                    mestr += " Disable";
                } else {
                    mestr += " Enable";
                }
                curmestr = mestr;
                nmeflag = true;
            }
            
        }
        return 0;
    }

    
    public static boolean outputCurSetting(){
        
        System.out.printf( "%s\n", "The current setting is:" );
        String[] A = { pstr, tstr, astr, cstr, nstr, mstr, mestr };
        String[] B = { curpstr, curtstr, curastr, curcstr, curnstr, curmstr, curmestr };
        
        for(int i=0; i<A.length; i++){
            if(B[i] != null){
                System.out.printf( "%s\n", B[i] );
            }
            else{
                System.out.printf( "%s\n", A[i] );
            }
            
        }
        
        return true;
    }

    
    public static String pstr = new String("ACTIVE_PLATFORM                     = ");
    public static String tstr = new String("TARGET                              = ");
    public static String astr = new String("TARGET_ARCH                         = ");
    public static String cstr = new String("TOOL_CHAIN_CONF                     = ");
    public static String nstr = new String("TOOL_CHAIN_TAG                      = ");
    public static String mstr = new String("MAX_CONCURRENT_THREAD_NUMBER        = ");
    public static String mestr = new String("MULTIPLE_THREAD                     = ");
    
    public static String curpstr = null;
    public static String curtstr = null;
    public static String curastr = null;
    public static String curcstr = null;
    public static String curnstr = null;
    public static String curmstr = null;
    public static String curmestr = null;
    
    public static boolean npflag = false;
    public static boolean ntflag = false;
    public static boolean naflag = false;
    public static boolean ncflag = false;
    public static boolean nnflag = false;
    public static boolean nmflag = false;
    public static boolean nmeflag = false;

}
