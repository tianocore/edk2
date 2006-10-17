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
                (args[0].compareToIgnoreCase("-?") == 0) || (args[0].compareToIgnoreCase("/?") == 0) || 
                (args[0].compareToIgnoreCase("-help") == 0) || (args[0].compareToIgnoreCase("/help") == 0) ){
                HelpInfo.outputUsageInfo();
                return false;
            }
            if( args[0].charAt(0) != '-' ){
                System.out.printf("%s\n", "Error arguments! Please type \"ContextTool -h\" for helpinfo.");
                return false;
            }
            for(int i=0; i<args.length; i++){
                if( (args[i].startsWith("-") && 
                    ((args[i].compareTo("-a") != 0) && (args[i].compareTo("-c") != 0) && 
                    (args[i].compareTo("-n") != 0) && (args[i].compareTo("-p") != 0) && 
                    (args[i].compareTo("-t") != 0) && (args[i].compareTo("-m") != 0)))){
                    System.out.printf("%s\n", "Error arguments! Please type \"ContextTool -h\" for helpinfo.");                                                                                                                                                            
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
                    System.out.printf("%s\n", curpstr);
                    return 1;
                }
                //
                //argstr is "-p ? ", display possible setting
                //
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "assign the platform FPD file's relative path to WORKSPACE" );
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
                    System.out.printf( "%s\n", "The number of ACTIVE_PLATFORM can not more than 1.");
                    return 3;
                }
                curpstr = pstr.concat(argstr.substring(2));
                npflag = true;
            } else if (argstr.charAt(1) == 't') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    System.out.printf("%s\n", curtstr);
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "What kind of the version is the binary target, such as DEBUG, RELEASE" );
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
                    System.out.printf("%s\n", curastr);
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "What kind of architechure is the binary target, such as IA32, IA64, X64, EBC, or ARM" );
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
                    System.out.printf("%s\n", curcstr);
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "Assign a txt file with the relative path to WORKSPACE, which specify the tools to use for the build and must be located in the path: WORKSPACE/Tools/Conf/" );
                    return 2;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '0'){
                    curcstr = cstr;
                    ncflag = true;
                    continue;
                }
                String[] S = argstr.split(" ");
                if(S.length > 2){
                    System.out.printf( "%s\n", "The number of TOOL_CHAIN_CONF can not more than 1.");
                    return 3;
                }
                curcstr = cstr.concat(argstr.substring(2));
                ncflag = true;
            } else if (argstr.charAt(1) == 'n') {
                if(argstr.length() < 4 && argstr.charAt(2) == ' '){
                    System.out.printf("%s\n", curnstr);
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "Specify the TagName, such as GCC, MSFT" );
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
                    System.out.printf("%s\n", curmstr);
                    return 1;
                }
                if(argstr.length() < 6 && argstr.charAt(3) == '?'){
                    System.out.printf( "%s\n", "The number of concurrent threads. Default is 2. Recommend to set this value to one more than the number of your compurter cores or CPUs." );
                    return 2;
                }
                String[] S = argstr.split(" ");
                if(S.length > 2){
                    System.out.printf( "%s\n", "The format of number is wrong.");
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
        
        if(curpstr != null){
            System.out.printf( "%s\n", curpstr );
        }
        else{
            System.out.printf( "%s\n", pstr );
        }
        
        if(curtstr != null){
            System.out.printf( "%s\n", curtstr );
        }
        else{
            System.out.printf( "%s\n", tstr );
        }
        
        if(curastr != null){
            System.out.printf( "%s\n", curastr );
        }
        else{
            System.out.printf( "%s\n", astr );
        }
        
        if(curcstr != null){
            System.out.printf( "%s\n", curcstr );
        }
        else{
            System.out.printf( "%s\n", cstr );
        }
        
        if(curnstr != null){
            System.out.printf( "%s\n", curnstr );
        }
        else{
            System.out.printf( "%s\n", nstr );
        }
        
        if(curmstr != null){
            System.out.printf( "%s\n", curmstr );
        }
        else{
            System.out.printf( "%s\n", mstr );
        }
        
        if(curmstr != null){
            System.out.printf( "%s\n", curmestr );
        }
        else{
            System.out.printf( "%s\n", mestr );
        }
        
        return true;
    }
     
    public static int length  = 0;
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
    
    public static int plength = 0;
    public static int tlength = 0;
    public static int alength = 0;
    public static int clength = 0;
    public static int nlength = 0;
    public static int mlength = 0;
    public static int melength = 0;
    
    public static boolean npflag = false;
    public static boolean ntflag = false;
    public static boolean naflag = false;
    public static boolean ncflag = false;
    public static boolean nnflag = false;
    public static boolean nmflag = false;
    public static boolean nmeflag = false;

}
