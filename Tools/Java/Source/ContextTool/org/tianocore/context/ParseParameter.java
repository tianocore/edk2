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
            HelpInfo.outputUsageInfo();
            return false;
        } else {
            if( args[0].charAt(0) != '-' ){
                HelpInfo.outputUsageInfo();
                return false;
            }
            for(int i=0; i<args.length; i++){
                if( (args[i].compareToIgnoreCase("-h") == 0) || 
                    (args[i].startsWith("-") && ((args[i].charAt(1) != 'a') && (args[i].charAt(1) != 'c') 
                    && (args[i].charAt(1) != 'n') && (args[i].charAt(1) != 'p') && (args[i].charAt(1) != 't') && (args[i].charAt(1) != 'm')))){
                    HelpInfo.outputUsageInfo();
                    return false;
                }
            }
        }
        
        standardizeParameter(args);
        return true; 
    }
    
    /** 
     * standardize user's input args
     * @param args -- user's input
     * @return no return value
     **/
    private static void standardizeParameter(String[] args) {
        
        //
        // the parameters's length are same.
        //
        length  = pstr.length();
        
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

            if (argstr.charAt(1) == 'p') {
                pstr += argstr.substring(2);
 //               pstr += "\n";
            } else if (argstr.charAt(1) == 't') {
                tstr += argstr.substring(2);
 //               tstr += "\n";
            } else if (argstr.charAt(1) == 'a') {
                astr += argstr.substring(2);
//                astr += "\n";
            } else if (argstr.charAt(1) == 'c') {
                cstr += argstr.substring(2);
//                cstr += "\n";
            } else if (argstr.charAt(1) == 'n') {
                nstr += argstr.substring(2);
//                nstr += "\n";
            } else if (argstr.charAt(1) == 'm') {
                mstr += argstr.substring(2);
//              mstr += "\n";
                if (argstr.charAt(3) == '0'){
                    mestr += " Disable";
                } else {
                    mestr += " Enable";
                }
          }
            i = j;
        }

    }
     
    public static int length  = 0;
    public static String pstr = new String("ACTIVE_PLATFORM                     = ");
    public static String tstr = new String("TARGET                              = ");
    public static String astr = new String("TARGET_ARCH                         = ");
    public static String cstr = new String("TOOL_CHAIN_CONF                     = ");
    public static String nstr = new String("TOOL_CHAIN_TAG                      = ");
    public static String mstr = new String("MAX_CONCURRENT_THREAD_NUMBER        = ");
    public static String mestr = new String("MULTIPLE_THREAD                     = ");

}
