/** @file
 GenFvImageTask class.

 GenFvImageTask is to call GenFvImage.exe to generate FvImage.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import java.io.File;
import java.util.LinkedList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.List;
import java.util.Iterator;
import java.io.BufferedReader;
import java.io.FileReader;

import org.tianocore.common.logger.EdkLog;

/**
  GenFvImageTask
  
  GenFvImageTask is to call GenFvImage.exe to generate the FvImage.
  
**/
public class GenFvImageTask extends Task implements EfiDefine{
    //
    // tool name
    //
    static final private String toolName = "GenFvImage";
    //
    // Pattern to match the section header (e.g. [options], [files])
    // 
    static final private Pattern sectionHeader = Pattern.compile("\\[([^\\[\\]]+)\\]");
    //
    // The name of input inf file
    //
    private FileArg infFile = new FileArg();
    //
    // Output directory
    //
    private String outputDir = ".";

    /**
      execute
      
      GenFvImageTask execute is to assemble tool command line & execute tool
      command line.
    **/
    public void execute() throws BuildException  {
        Project project = this.getOwningTarget().getProject();
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");

        if (isUptodate()) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, infFile.toFileList() + " is uptodate!");
            return;
        }

        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }

        String argument = "" + infFile;
        //
        // lauch the program
        //
        int exitCode = 0;
        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);

            LogStreamHandler streamHandler = new LogStreamHandler(this,
                    Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);

            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());
            runner.setWorkingDirectory(new File(outputDir)); 
            //
            // log command line string.
            //
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, infFile.toFileList());

            exitCode = runner.execute();
            if (exitCode != 0) {
                EdkLog.log(this, "ERROR = " + Integer.toHexString(exitCode));
            } else {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "GenFvImage succeeded!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        } finally {
            if (exitCode != 0) {
                throw new BuildException("GenFvImage: failed to generate FV file!");
            }
        }
    }
    /**
      getInfFile
      
      This function is to get class member of infFile
      @return String    name of infFile
    **/
    public String getInfFile() {
        return infFile.getValue();
    }
    
    /**
      setInfFile
      
      This function is to set class member of infFile.
      
      @param infFile  name of infFile
    **/
    public void setInfFile(String infFile) {
        this.infFile.setArg(" -I ", infFile);
    }
    
    /**
      getOutputDir
      
      This function is to get output directory.
      
      @return                Path of output directory.
    **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
      
      This function is to set output directory.
      
      @param outputDir        The output direcotry.
    **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    //
    // dependency check
    // 
    private boolean isUptodate() {
        String infName = this.infFile.getValue();
        String fvName = "";
        List<String> ffsFiles = new LinkedList<String>();
        File inf = new File(infName);

        try {
            FileReader reader = new FileReader(inf);
            BufferedReader in = new BufferedReader(reader);
            String str;

            //
            // Read the inf file line by line
            // 
            boolean inFiles = false;
            boolean inOptions = false;
            while ((str = in.readLine()) != null) {
                str = str.trim();
                if (str.length() == 0) {
                    continue;
                }

                Matcher matcher = sectionHeader.matcher(str);
                if (matcher.find()) {
                    //
                    // We take care of only "options" and "files" section
                    // 
                    String sectionName = str.substring(matcher.start(1), matcher.end(1));
                    if (sectionName.equalsIgnoreCase("options")) {
                        inOptions = true;
                        inFiles = false;
                    } else if (sectionName.equalsIgnoreCase("files")) {
                        inFiles = true;
                        inOptions = false;
                    } else {
                        inFiles = false;
                        inOptions = false;
                    }
                    continue;
                }

                //
                // skip invalid line
                // 
                int equalMarkPos = str.indexOf("=");
                if (equalMarkPos < 0) {
                    continue;
                }

                //
                // we have only interest in EFI_FILE_NAME
                // 
                String fileNameFlag = str.substring(0, equalMarkPos).trim();
                String fileName = str.substring(equalMarkPos + 1).trim();
                if (!fileNameFlag.equalsIgnoreCase("EFI_FILE_NAME")
                    || fileName.length() == 0) {
                    continue;
                }

                if (inFiles) {
                    //
                    // files specified beneath the [files] section are source files
                    // 
                    ffsFiles.add(fileName);
                } else if (inOptions) {
                    //
                    // file specified beneath the [options] section is the target file
                    // 
                    fvName = outputDir + File.separator + fileName;
                }
            }
        } catch (Exception ex) {
            throw new BuildException(ex.getMessage());
        }

        //
        // if destionation file doesn't exist, we need to generate it.
        // 
        File fvFile = new File(fvName);
        if (!fvFile.exists()) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, fvName + " doesn't exist!");
            return false;
        }

        //
        // the inf file itself will be taken as source file, check its timestamp
        // against the target file
        // 
        long fvFileTimeStamp = fvFile.lastModified();
        if (inf.lastModified() > fvFileTimeStamp) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, infName + " has been changed since last build!");
            return false;
        }

        //
        // no change in the inf file, we need to check each source files in it
        // against the target file
        // 
        for (Iterator it = ffsFiles.iterator(); it.hasNext(); ) {
            String fileName = (String)it.next();
            File file = new File(fileName);
            if (file.lastModified() > fvFileTimeStamp) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, fileName + " has been changed since last build!");
                return false;
            }
        }

        return true;
    }
}
