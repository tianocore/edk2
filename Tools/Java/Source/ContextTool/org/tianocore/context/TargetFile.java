/** @file
  File is TargetFile class which is used to generate the new target.txt. 
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.context;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class TargetFile {

   
    
    /** 
     * validate the filename
     * @param String filename : the name of target file
     * 
     * @return true or false
     **/
    public static boolean validateFilename(String filename) {
        
        String workspacePath = System.getenv("WORKSPACE");
        
        Fd = new File(workspacePath + File.separator + "Tools" + File.separator + "Conf" + File.separator + filename);
        
        if (Fd.exists() == true && Fd.canRead() == true)
            return true;
        else
            return false;
    }
    

    /**
     * create a empty temp file, which is located at the same directory with target file
     * @param String filename : the name of target temp file
     * @return true or false
     **/
    public static boolean createTempFile(String filename) {

        String workspacePath = System.getenv("WORKSPACE");
        
        TempFd = new File(workspacePath + File.separator + "Tools" + File.separator + "Conf" + File.separator + filename + "tmp");

        if (TempFd.exists() == true) {
            if (TempFd.delete() == false) {
                System.out.printf("%n%s%n", "target.txttmp has been existed, and failed in deletion!");
                return false;
            }
        }
        try {
            TempFd.createNewFile();
        } catch (IOException e) {
            System.out.printf("%n%s%n", "Failed in creation of the temp file:target.txttmp!");
            return false;
        }

        return true;
    }

    /**
     * read from target.txt and write to target.txttmp, del target.txt, rename
     * @param no paremeter
     * @return true or false
     **/
    public static boolean readwriteFile() {

        if (Fd.canRead() != true)
            return false;

        BufferedReader br = null;
        BufferedWriter bw = null;
        String textLine = null;

        try {
            br = new BufferedReader(new FileReader(Fd));
        } catch (FileNotFoundException e) {
            System.out
                    .println("\n# Creating BufferedReader fail!");
            return false;
        }
        try {
            bw = new BufferedWriter(new FileWriter(TempFd));
        } catch (IOException e) {
            System.out.println("\n# Creating the BufferedWriter fail!");
            return false;
        }
        
        //
        //TARGET_ARCH must be in front of TARGET!!! according to the target.txt
        //
        try {
            while ((textLine = br.readLine()) != null) {
                //
                // the line is composed of Space
                //
                if (textLine.trim().compareToIgnoreCase("") == 0) {
                    bw.write(textLine);
                    bw.newLine();
                } 
                //
                // the line starts with "#", and no "="
                //
                else if ((textLine.trim().charAt(0) == '#') && (textLine.indexOf("=") == -1)){
                    bw.write(textLine);
                    bw.newLine();
                } else {
                    //
                    //modify at the first time, and there should be "*ACTIVE_PLATFORM*=*" in the line
                    //
                    if (textLine.indexOf("ACTIVE_PLATFORM") != -1) {
                        if(pflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.npflag == true) {
                                    bw.write(ParseParameter.curpstr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                pflag = false;
                                continue;
                            }
                            if(ParseParameter.npflag == true) {
                                bw.write(ParseParameter.curpstr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            pflag = false;
                        }
                    } else if (textLine.indexOf("TARGET_ARCH") != -1) {
                        if(aflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.naflag == true) {
                                    bw.write(ParseParameter.curastr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                aflag = false;
                                continue;
                            }
                            if(ParseParameter.naflag == true) {
                                bw.write(ParseParameter.curastr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            aflag = false;
                        }
                    } else if (textLine.indexOf("TARGET") != -1) {
                        if(tflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.ntflag == true) {
                                    bw.write(ParseParameter.curtstr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                tflag = false;
                                continue;
                            }
                            if(ParseParameter.ntflag == true) {
                                bw.write(ParseParameter.curtstr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            tflag = false;
                        }
                    } else if (textLine.indexOf("TOOL_CHAIN_CONF") != -1) {
                        if(cflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.ncflag == true) {
                                    bw.write(ParseParameter.curcstr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                cflag = false;
                                continue;
                            }
                            if(ParseParameter.ncflag == true) {
                                bw.write(ParseParameter.curcstr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            cflag = false;
                        }
                    } else if (textLine.indexOf("TOOL_CHAIN_TAG") != -1) {
                        if(nflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.nnflag == true) {
                                    bw.write(ParseParameter.curnstr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                nflag = false;
                                continue;
                            }
                            if(ParseParameter.nnflag == true) {
                                bw.write(ParseParameter.curnstr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            nflag = false;
                        }
                    } else if (textLine.indexOf("MAX_CONCURRENT_THREAD_NUMBER") != -1) {
                        if(mflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.nmflag == true) {
                                    bw.write(ParseParameter.curmstr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                mflag = false;
                                continue;
                            }
                            if(ParseParameter.nmflag == true) {
                                bw.write(ParseParameter.curmstr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            mflag = false;
                        }
                    }else if (textLine.indexOf("MULTIPLE_THREAD") != -1) {
                        if(meflag == true){
                            if(textLine.trim().charAt(0) == '#'){
                                if(ParseParameter.nmeflag == true) {
                                    bw.write(ParseParameter.curmestr);
                                }else{
                                    bw.write(textLine);
                                }
                                bw.newLine();
                                meflag = false;
                                continue;
                            }
                            if(ParseParameter.nmeflag == true) {
                                bw.write(ParseParameter.curmestr);
                            } else {
                                bw.write(textLine);
                            }
                            bw.newLine();
                            meflag = false;
                        }
                    }
                }
            }
            //
            //user maybe delete the line *ACTIVE_PLATFORM*=*
            //
            if( (pflag == true) && (ParseParameter.npflag == true) ){
                bw.write(ParseParameter.curpstr);
                bw.newLine();
            } else if ( (tflag == true) && (ParseParameter.ntflag == true) ){
                bw.write(ParseParameter.curtstr);
                bw.newLine();
            } else if ( (aflag == true) && (ParseParameter.naflag == true) ){
                bw.write(ParseParameter.curastr);
                bw.newLine();
            } else if ( (cflag == true) && (ParseParameter.ncflag == true) ){
                bw.write(ParseParameter.curcstr);
                bw.newLine();
            } else if ( (nflag == true) && (ParseParameter.nnflag == true) ){
                bw.write(ParseParameter.curnstr);
                bw.newLine();
            } else if ( (meflag == true) && (ParseParameter.nmeflag == true) ){
                bw.write(ParseParameter.curmestr);
                bw.newLine();
            } else if ( (mflag == true) && (ParseParameter.nmflag == true) ){
                bw.write(ParseParameter.curmstr);
                bw.newLine();
            }
        } catch (IOException e) {
            System.out.println("\n# Reading or Writing file fail!");
            return false;
        }

        try {
            br.close();
            bw.close();
        } catch (IOException e) {
            System.out
                    .println("\n# Closing BufferedReader&BufferedWriter fail!");
            return false;
        }

        if (Fd.delete() == false) {
            System.out.println("\n# Deleting file fail!");
            return false;
        }
        if (TempFd.renameTo(Fd) == false) {
            System.out.println("\n#  Renaming file failed!");
            return false;
        }

        return true;
    }
    
    /**
     * read the file and output the lines which include setting
     * @param File fd : the File of the target file
     * @return String: the current setting
     **/
    public static boolean readFile() {
        
        BufferedReader br = null;
        String textLine = null;

        try {
            br = new BufferedReader(new FileReader(Fd));
        } catch (FileNotFoundException e) {
            System.out
                    .println("\n# Creating BufferedReader fail!");
            return false;
        }
        try {
            while ((textLine = br.readLine()) != null) {
                //
                // the line is composed of Space
                //
                if (textLine.trim().compareToIgnoreCase("") == 0) {
                    continue;
                } 
                //
                // the line starts with "#"
                //
                else if ((textLine.trim().charAt(0) == '#')){
                    continue;
                } else {
                    if (textLine.indexOf("ACTIVE_PLATFORM") != -1) {
                        ParseParameter.curpstr = textLine;
                    } else if (textLine.indexOf("TARGET_ARCH") != -1) {
                        ParseParameter.curastr = textLine;
                    } else if (textLine.indexOf("TARGET") != -1) {
                        ParseParameter.curtstr = textLine;
                    } else if (textLine.indexOf("TOOL_CHAIN_CONF") != -1) {
                        ParseParameter.curcstr = textLine;
                    } else if (textLine.indexOf("TOOL_CHAIN_TAG") != -1) {
                        ParseParameter.curnstr = textLine;
                    } else if (textLine.indexOf("MAX_CONCURRENT_THREAD_NUMBER") != -1) {
                        ParseParameter.curmstr = textLine;
                    } else if (textLine.indexOf("MULTIPLE_THREAD") != -1) {
                        ParseParameter.curmestr = textLine;
                    }
                }
            }
        } catch (IOException e) {
            System.out.println("\n# Reading file fail!");
            return false;
        }

        try {
            br.close();
        } catch (IOException e) {
            System.out
                    .println("\n# Closing BufferedReader fail!");
            return false;
        }
        return true;
    }
    

    ///
    /// point to target.txttmp, a temp file, which is created and deleted during the tool's runtime.
    ///
    private static File TempFd;
    
    ///
    /// point to target.txt.
    ///
    private static File Fd;
    
    ///
    /// when the flag is true, the corresponding str should be add at the end of file.
    ///
    private static boolean pflag = true;
    private static boolean tflag = true;
    private static boolean aflag = true;
    private static boolean cflag = true;
    private static boolean nflag = true;
    private static boolean mflag = true;
    private static boolean meflag = true;


}
