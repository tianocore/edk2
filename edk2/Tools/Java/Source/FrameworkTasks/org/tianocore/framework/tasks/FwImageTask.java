/** @file
 FwImageTask class.

 FwImageTask is used to call FwImage.ext to generate the FwImage.


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

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import org.tianocore.common.logger.EdkLog;

/**
  FwImageTask class.

  FwImageTask is used to call FwImage.ext to generate the FwImage.
**/
public class FwImageTask extends Task implements EfiDefine {
    //
    // fwimage tool name
    // 
    private static String toolName = "FwImage";
    //
    // time&data
    //
    private ToolArg time = new ToolArg();
    //
    // input PE image
    //
    private FileArg peImage = new FileArg();
    //
    // output EFI image
    //
    private FileArg outImage = new FileArg();
    //
    // component type
    //
    private ToolArg componentType = new ToolArg();

    /**
      execute

      FwimageTask execute function is to assemble tool command line & execute
      tool command line

      @throws BuidException
    **/
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }
        //
        // argument of tools
        //
        String argument = "" + time + componentType + peImage + outImage;
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

            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, peImage.toFileList() + " => " + outImage.toFileList());

            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "FwImage succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(this, "ERROR = " + Integer.toHexString(revl));
                throw new BuildException("FwImage failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      setTime

      This function is to set operation of class member "time".

      @param time            string of time
    **/
    public void setTime(String time) {
        this.time.setArg(" -t ", time);
    }

    /**
      getTime

      This function is to get class member "time"
      @return time          string of time
    **/
    public String getTime() {
        return this.time.getValue();
    }

    /**
      getPeImage

      This function is to get class member "peImage".
      @return                name of PE image
    **/
    public String getPeImage() {
        return this.peImage.getValue();
    }

    /**
      setPeImage

      This function is to set class member "peImage"
      @param  peImage        name of PE image
    **/
    public void setPeImage(String peImage) {
        this.peImage.setArg(" ", peImage);
    }

    /**
      getOutImage

      This function is to get class member "outImage".
      @return                 name of output EFI image
    **/
    public String getOutImage() {
        return this.outImage.getValue();
    }

    /**
      setOutImage

      This function is to set class member "outImage".
      @param outImage         name of output EFI image
    **/
    public void setOutImage(String outImage) {
        this.outImage.setArg(" ", outImage);
    }

    /**
      getComponentType

      This function is to get class member "componentType".

      @return                 string of componentType
    **/
    public String getComponentType() {
        return this.componentType.getValue();
    }

    /**
      setComponentType

      This function is to set class member "componentType".
      @param  componentType   string of component type
    **/
    public void setComponentType(String componentType) {
        this.componentType.setArg(" ", componentType);
    }
}
