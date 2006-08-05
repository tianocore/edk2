/** @file
 PeiReBaseTask class.

 PeiReBaseTask is used to call PeiReBase.exe to rebase efi fv file.
 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;

import java.io.File;

import org.apache.tools.ant.Task;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;
import org.tianocore.logger.EdkLog;

/**
  PeiReBaseTask class.

  PeiReBaseTask is used to call PeiReBase.exe to rebase efi fv file.
**/
public class PeiReBaseTask extends Task implements EfiDefine {
    ///
    /// tool name
    ///
    private String toolName = "PeiReBase";
    // /
    // / Input file
    // /
    private String inputFile = "";
    private String inputFileName = "";
    // /
    // / Output file
    // /
    private String outputFile = "";

    // /
    // / Output directory, this variable is added by jave wrap
    // /
    private String outputDir = "";
    
    ///
    /// Base address
    ///
    private String baseAddr = "";
    
    ///
    /// Architecture 
    ///
    private String arch = "";
    
    /**
     * execute
     * 
     * PeiReBaseTask execute function is to assemble tool command line & execute
     * tool command line
     * 
     * @throws BuidException
     */
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        //
        // set Logger
        //
        FrameworkLogger logger = new FrameworkLogger(project, toolName.toLowerCase());
        EdkLog.setLogLevel(project.getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);
        
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        String argument;
        if (this.arch.equalsIgnoreCase("IA32")){
            command = toolName + "_IA32";
        }else if (this.arch.equalsIgnoreCase("X64")){
            command = toolName + "_X64";
        }else if (this.arch.equalsIgnoreCase("IPF")){
            command = toolName + "_IPF";
        }else {
            command = toolName + "_IA32";
        }
        if (path != null) {
            command = path + File.separatorChar + command;
        }
        
        //
        // argument of tools
        //
        File file = new File(outputFile);
        if (!file.isAbsolute() && (!this.outputDir.equalsIgnoreCase(""))) {
            argument = inputFile + " " +  "-O " + outputDir + File.separatorChar
                    + outputFile + " " + this.baseAddr + " "
                    + "-M " + outputDir + + File.separatorChar + outputFile + ".map";
        } else {
            argument = inputFile + " " + "-O " + outputFile + " " + this.baseAddr+ " " + "-M " + outputFile + ".map";
        }
        
        //
        // return value of fwimage execution
        //
        int revl = -1;

        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);

            LogStreamHandler streamHandler = new LogStreamHandler(this,
                    Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);

            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());
            //
            // Set debug log information.
            //
            EdkLog.log(EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(EdkLog.EDK_INFO, this.inputFileName);
            
            revl = runner.execute();
            
            if (EFI_SUCCESS == revl) {
                //
                // command execution success
                //
                EdkLog.log(EdkLog.EDK_VERBOSE, "PeiReBase succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(EdkLog.EDK_INFO, "ERROR = " + Integer.toHexString(revl));
                throw new BuildException("PeiReBase failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
     * getInputFile
     * 
     * This function is to get class member "inputFile".
     * 
     * @return string of input file name.
     */
    public String getInputFile() {
        return inputFile;
    }

    /**
     * setComponentType
     * 
     * This function is to set class member "inputFile".
     * 
     * @param inputFile
     *            string of input file name.
     */
    public void setInputFile(String inputFile) {
        this.inputFileName = (new File(inputFile)).getName();
        this.inputFile = "-I " + inputFile;
    }

    /**
     * getOutputFile
     * 
     * This function is to get class member "outputFile"
     * 
     * @return outputFile string of output file name.
     */
    public String getOutputFile() {
        return outputFile;
    }

    /**
     * setOutputFile
     * 
     * This function is to set class member "outputFile"
     * 
     * @param outputFile
     *            string of output file name.
     */
    public void setOutputFile(String outputFile) {
        this.outputFile = outputFile;
    }

    /**
     * getOutputDir
     * 
     * This function is to get class member "outputDir"
     * 
     * @return outputDir string of output directory.
     */
    public String getOutputDir() {
        return outputDir;
    }

    /**
     * setOutputDir
     * 
     * This function is to set class member "outputDir"
     * 
     * @param outputDir
     *            string of output directory.
     */
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    /**
     * getBaseAddr
     * 
     * This function is to get class member "baseAddr"
     * 
     * @return baseAddr   string of base address.
     */
    public String getBaseAddr() {
        return baseAddr;
    }

    /**
     * setBaseAddr
     * 
     * This function is to set class member "baseAddr"
     * 
     * @param baseAddr    string of base address
     */
    public void setBaseAddr(String baseAddr) {
        this.baseAddr = "-B " +  baseAddr;
    }

    /**
     * getArch
     * 
     * This function is to get class member "arch".
     * 
     * @return arch       Architecture
     */
    public String getArch() {
        return arch;
    }

    /**
     * setArch
     * 
     * This function is to set class member "arch" 
     * 
     * @param arch         Architecture
     */
    public void setArch(String arch) {
        this.arch = arch;
    }
}
