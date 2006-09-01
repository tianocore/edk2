package org.tianocore.context;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

public class TargetFile {

    /** 
     * check the validity of path and file
     * @param String filename : the name of target file
     * @return true or false
     **/
    public static boolean parsePath(String filename) {

        String workspacePath = System.getenv("WORKSPACE");
        
        Fd = new File(workspacePath + File.separator + "Tools" + File.separator + "Conf" + File.separator + filename);

        if (Fd.exists() == true) {
            if (createTempFile(filename + "tmp") == false) {
                return false;
            }
            if (readwriteFile() == false) {
                return false;
            }
            return true;
        } else {
            try {
                Fd.createNewFile();
            } catch (IOException e) {
                System.out.printf("%n%s", "Create the file:target.txt failed!");
                return false;
            }
        }
        TargetFile.writeFile(Fd);
        return true;
    }

    /**
     * create a empty temp file, which is located at the same directory with target file
     * @param String filename : the name of target temp file
     * @return true or false
     **/
    private static boolean createTempFile(String filename) {

        String workspacePath = System.getenv("WORKSPACE");
        
        TempFd = new File(workspacePath + File.separator + "Tools" + File.separator + "Conf" + File.separator + filename);

        if (TempFd.exists() == true) {
            if (TempFd.delete() == false) {
                System.out.println("\n#  delete file failed !");
                return false;
            }
        }
        try {
            TempFd.createNewFile();
        } catch (IOException e) {
            System.out.printf("%n%s",
                    "Create the temp file:target.txttmp failed!");
            return false;
        }

        return true;
    }

    /**
     * read from target.txt and write to target.txttmp, del target.txt, rename
     * @param no paremeter
     * @return true or false
     **/
    private static boolean readwriteFile() {

        if (Fd.canRead() != true)
            return false;

        BufferedReader br = null;
        BufferedWriter bw = null;
        String textLine = null;

        try {
            br = new BufferedReader(new FileReader(Fd));
        } catch (FileNotFoundException e) {
            System.out
                    .println("\n# create the BufferedReader failed, because can't find the file:target.txt!");
            return false;
        }
        try {
            bw = new BufferedWriter(new FileWriter(TempFd));
        } catch (IOException e) {
            System.out.println("\n# create the BufferedWriter failed!");
            return false;
        }
        
        //
        //TARGET_ARCH must be in front of TARGET!!! according to the target.txt
        //
        try {
            while ((textLine = br.readLine()) != null) {
                if (textLine.trim().compareToIgnoreCase("") == 0) {
                    bw.write(textLine);
                    bw.newLine();
                } else if ((textLine.trim().charAt(0) == '#') && (textLine.indexOf("=") == -1)){
                    bw.write(textLine);
                    bw.newLine();
                } else {
                    if (textLine.indexOf("ACTIVE_PLATFORM") != -1) {
                        if(ParseParameter.pstr.length() > ParseParameter.length) {
                            bw.write(ParseParameter.pstr);
                        } else {
                            bw.write(textLine);
                        }
                        bw.newLine();
                    } else if (textLine.indexOf("TARGET_ARCH") != -1) {
                        if(ParseParameter.astr.length() > ParseParameter.length) {
                            bw.write(ParseParameter.astr);
                        } else {
                            bw.write(textLine);
                        }
                        bw.newLine();
                    } else if (textLine.indexOf("TARGET") != -1) {
                        if(ParseParameter.tstr.length() > ParseParameter.length) {
                            bw.write(ParseParameter.tstr);
                        } else {
                            bw.write(textLine);
                        }
                        bw.newLine();
                    } else if (textLine.indexOf("TOOL_CHAIN_CONF") != -1) {
                        if(ParseParameter.cstr.length() > ParseParameter.length) {
                            bw.write(ParseParameter.cstr);
                        } else {
                            bw.write(textLine);
                        }
                        bw.newLine();
                    } else if (textLine.indexOf("TOOL_CHAIN_TAG") != -1) {
                        if(ParseParameter.nstr.length() > ParseParameter.length) {
                            bw.write(ParseParameter.nstr);
                        } else {
                            bw.write(textLine);
                        }
                        bw.newLine();
                    }
                }
            }
        } catch (IOException e) {
            System.out.println("\n#  read or write file error!");
            return false;
        }

        try {
            br.close();
            bw.close();
        } catch (IOException e) {
            System.out
                    .println("\n#  close BufferedReader&BufferedWriter error");
            return false;
        }

        if (Fd.delete() == false) {
            System.out.println("\n#  delete file failed !");
            return false;
        }
        if (TempFd.renameTo(Fd) == false) {
            System.out.println("\n#  rename file failed !");
            return false;
        }

        return true;
    }

    /**
     * according to user's input args, write the file directly
     * @param File fd : the File of the target file
     * @return true or false
     **/
    private static boolean writeFile(File fd) {

        if (fd.canWrite() != true)
            return false;

        FileOutputStream outputFile = null;
        try {
            outputFile = new FileOutputStream(fd);
        } catch (FileNotFoundException e) {
            System.out
                    .println("\n#  can't find the file when open the output stream !");
            return false;
        }
        FileChannel outputChannel = outputFile.getChannel();

        ByteBuffer[] buffers = new ByteBuffer[5];
        buffers[0] = ByteBuffer.allocate(ParseParameter.pstr.toString().length());
        buffers[1] = ByteBuffer.allocate(ParseParameter.tstr.toString().length());
        buffers[2] = ByteBuffer.allocate(ParseParameter.astr.toString().length());
        buffers[3] = ByteBuffer.allocate(ParseParameter.cstr.toString().length());
        buffers[4] = ByteBuffer.allocate(ParseParameter.nstr.toString().length());

        buffers[0].put(ParseParameter.pstr.toString().getBytes()).flip();
        buffers[1].put(ParseParameter.tstr.toString().getBytes()).flip();
        buffers[2].put(ParseParameter.astr.toString().getBytes()).flip();
        buffers[3].put(ParseParameter.cstr.toString().getBytes()).flip();
        buffers[4].put(ParseParameter.nstr.toString().getBytes()).flip();

        try {
            ByteBuffer bufofCP = ByteBuffer.allocate(Copyright.length());
            bufofCP.put(Copyright.getBytes()).flip();
            outputChannel.write(bufofCP);
            
            ByteBuffer bufofFI = ByteBuffer.allocate(Fileinfo.length());
            bufofFI.put(Fileinfo.getBytes()).flip();
            outputChannel.write(bufofFI);
            
            ByteBuffer buffer0 = ByteBuffer.allocate(pusage.length());
            buffer0.put(pusage.getBytes()).flip();
            outputChannel.write(buffer0);
            outputChannel.write(buffers[0]);
            
            ByteBuffer buffer1 = ByteBuffer.allocate(tusage.length());
            buffer1.put(tusage.getBytes()).flip();
            outputChannel.write(buffer1);
            outputChannel.write(buffers[1]);
            
            ByteBuffer buffer2 = ByteBuffer.allocate(ausage.length());
            buffer2.put(ausage.getBytes()).flip();
            outputChannel.write(buffer2);
            outputChannel.write(buffers[2]);
            
            ByteBuffer buffer3 = ByteBuffer.allocate(cusage.length());
            buffer3.put(cusage.getBytes()).flip();
            outputChannel.write(buffer3);
            outputChannel.write(buffers[3]);
            
            ByteBuffer buffer4 = ByteBuffer.allocate(nusage.length());
            buffer4.put(nusage.getBytes()).flip();
            outputChannel.write(buffer4);
            outputChannel.write(buffers[4]);
            
            outputFile.close();
        } catch (IOException e) {
            System.out.println("\n# The operations of file failed !");
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

    private static final String Copyright = "#\n"
            + "#  Copyright (c) 2006, Intel Corporation\n"
            + "#\n"
            + "#  All rights reserved. This program and the accompanying materials\n"
            + "#  are licensed and made available under the terms and conditions of the BSD License\n"
            + "#  which accompanies this distribution.  The full text of the license may be found at\n"
            + "#  http://opensource.org/licenses/bsd-license.php\n"
            + "\n"
            + "#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN \"AS IS\" BASIS,\n"
            + "#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.\n";

    private static final String Fileinfo = "#\n"
            + "#  Filename: target.template\n"
            + "#\n"
            + "#  ALL Paths are Relative to WORKSPACE\n"
            + "\n"
            + "#  Separate multiple LIST entries with a SINGLE SPACE character, do not use comma characters.\n"
            + "#  Un-set an option by either commenting out the line, or not setting a value.\n";

    private static final String pusage = "#\n"
            + "#  PROPERTY              Type       Use         Description\n"
            + "#  ----------------      --------   --------    -----------------------------------------------------------\n"
            + "#  ACTIVE_PLATFORM       Filename   Recommended Specify the WORKSPACE relative Path and Filename\n"
            + "#                                               of the platform FPD file that will be used for the build\n"
            + "#                                               This line is required if and only if the current working\n"
            + "#                                               directory does not contain one or more FPD files.\n";

    private static final String tusage = "\n\n"
            + "#  TARGET                List       Optional    Zero or more of the following: DEBUG, RELEASE, \n"
            + "#                                               UserDefined; separated by a space character.  \n"
            + "#                                               If the line is missing or no value is specified, all\n"
            + "#                                               valid targets specified in the FPD file will attempt \n"
            + "#                                               to be built.  The following line will build all platform\n"
            + "#                                               targets.\n";

    private static final String ausage = "\n\n"
            + "#  TARGET_ARCH           List       Optional    What kind of architecture is the binary being target for.\n"
            + "#                                               One, or more, of the following, IA32, IA64, X64, EBC or ARM.\n"
            + "#                                               Multiple values can be specified on a single line, using \n"
            + "#                                               space charaters to separate the values.  These are used \n"
            + "#                                               during the parsing of an FPD file, restricting the build\n"
            + "#                                               output target(s.)\n"
            + "#                                               The Build Target ARCH is determined by a logical AND of:\n"
            + "#                                               FPD BuildOptions: <SupportedArchitectures> tag\n"
            + "#                                               If not specified, then all valid architectures specified \n"
            + "#                                               in the FPD file, for which tools are available, will be \n"
            + "#                                               built.\n";

    private static final String cusage = "\n\n"
            + "#  TOOL_DEFINITION_FILE  Filename  Optional   Specify the name of the filename to use for specifying \n"
            + "#                                             the tools to use for the build.  If not specified, \n"
            + "#                                             tools_def.txt will be used for the build.  This file \n"
            + "#                                             MUST be located in the WORKSPACE/Tools/Conf directory.\n";

    private static final String nusage = "\n\n"
            + "#  TAGNAME               List      Optional   Specify the name(s) of the tools_def.txt TagName to use.\n"
            + "#                                             If not specified, all applicable TagName tools will be \n"
            + "#                                             used for the build.  The list uses space character separation.\n";
}
