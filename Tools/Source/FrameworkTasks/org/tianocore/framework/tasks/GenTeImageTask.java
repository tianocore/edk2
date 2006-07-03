/** @file
 GenTeImageTask class.

 GenTeImageTask is used to call GenTEImage.exe to generate TE image .
 
 
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
 * GenTeImageTask class.
 * 
 * GenTeImageTask is used to call GenAcpiTable.exe to generate ACPI Table image .
 */
public class GenTeImageTask extends Task implements EfiDefine {
    ///
    /// tool name
    ///
    private String toolName = "GenTeImage";
    ///
    /// input file
    ///
    private String inputFile = "";

    ///
    /// output file
    ///
    private String outputFile = "";

    ///
    /// output directory, this variable is added by jave wrap
    ///
    private String outputDir = "";

    ///
    /// Verbose flag
    ///
    private String verbose = "";

    ///
    /// Dump flag
    ///
    private String dump = "";

    /**
     * assemble tool command line & execute tool command line
     * 
     * @throws BuildException
     */
    /**
     * execute
     * 
     * GenTeImgaeTask execute function is to assemble tool command line & execute
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
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separatorChar + toolName;
        }
        //
        // argument of tools
        //
        File file = new File(outputFile);
        if (!file.isAbsolute() && (!this.outputDir.equalsIgnoreCase(""))) {
            argument = this.verbose + this.dump + "-o " +this.outputDir
                    + File.separatorChar + this.outputFile + " "
                    + this.inputFile;
        } else {
            argument = this.verbose + this.dump + "-o " + this.outputFile
                    + " " + this.inputFile;
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
            EdkLog.log(EdkLog.EDK_INFO, Commandline.toString(cmdline
                    .getCommandline()));

            revl = runner.execute();

            if (EFI_SUCCESS == revl) {
                //
                // command execution success
                //
                EdkLog.log(EdkLog.EDK_INFO, "GenTeImage succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(EdkLog.EDK_ERROR, "GenTeImage failed. (error="
                        + Integer.toHexString(revl) + ")");
                throw new BuildException("GenTeImage failed. (error="
                        + Integer.toHexString(revl) + ")");

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
        this.inputFile = inputFile;
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
        this.outputFile = outputFile  + " ";
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
     * getVerbose
     * 
     * This function is to get class member "verbose"
     * 
     * @return verbose the flag of verbose.
     */
    public String getVerbose() {
        return this.verbose;
    }

    /**
     * setVerbose
     * 
     * This function is to set class member "verbose"
     * 
     * @param verbose
     *            True or False.
     */
    public void setVerbose(boolean verbose) {
        if (verbose) {
            this.verbose = "-v ";
        }
    }

    /**
     * getDump
     * 
     * This function is to get class member "dump"
     * 
     * @return verbose the flag of dump.
     */
    public String getDump() {
        return dump;
    }

    /**
     * setDump
     * 
     * This function is to set class member "dump"
     * 
     * @param dump
     *            True or False.
     */
    public void setDump(boolean dump) {
        if (dump) {
            this.dump = "-dump ";
        }
    }
}
