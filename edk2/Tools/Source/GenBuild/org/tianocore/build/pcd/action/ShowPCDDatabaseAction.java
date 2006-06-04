/** @file
  ShowPCDDatabase class.

  This class is the action to diplay the PCD database.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.pcd.action;

import java.io.File;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.pcd.exception.UIException;
import org.tianocore.build.pcd.ui.PCDDatabaseFrame;

/** This class is the action to show PCD database.
**/
public class ShowPCDDatabaseAction extends UIAction {
    ///
    /// The workspace path parameter.
    ///
    private String workspacePath;
    ///
    /// The FpdfileName parameter.
    ///
    private String fpdFilePath;

    /**
     set workspace path parameter for this action.
      
     @param workspacePath the string of workspace path.
    **/
    public void setWorkspacePath(String workspacePath) {
        this.workspacePath = workspacePath;
    }

    /**
      set fpd file path parameter for this action.

      @param fpdFilePath file path string
    **/
    public void setFPDFilePath(String fpdFilePath) {
        this.fpdFilePath = "./" + fpdFilePath;
    }

    /**
      check paramter for this action.
      
      @throw UIException wrong paramter.
    **/
    void checkParamter() throws UIException {
        File file = null;

        if((fpdFilePath    == null) ||(workspacePath  == null)) {
            throw new UIException("WorkspacePath and FPDFileName should be blank for CollectPCDAtion!");
        }

        if(fpdFilePath.length() == 0 || workspacePath.length() == 0) {
            throw new UIException("WorkspacePath and FPDFileName should be blank for CollectPCDAtion!");
        }

        file = new File(workspacePath);
        if(!file.exists()) {
            throw new UIException("WorkpacePath " + workspacePath + " does not exist!");
        }

        file = new File(fpdFilePath);

        if(!file.exists()) {
            throw new UIException("FPD File " + fpdFilePath + " does not exist!");
        }
    }

    /**
      Core workflow function.
      
      @throw UIException Fail to show PCD database.
    **/
    void performAction() throws UIException {
        CollectPCDAction  collectAction = null;
        PCDDatabaseFrame  dbFrame       = null;

        //
        // Initialize global data.
        //
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            workspacePath);

        //
        // Collect PCD information.
        //
        collectAction = new CollectPCDAction();

        try {
            collectAction.perform(workspacePath,
                                  fpdFilePath,
                                  ActionMessage.LOG_MESSAGE_LEVEL);
        } catch(Exception exp) {
            throw new UIException(exp.getMessage());
        }

        //
        // Start tree windows.
        //
        dbFrame = new PCDDatabaseFrame(GlobalData.getPCDMemoryDBManager());
    }

    /**
      Entry function.
      
      The action is run from command line.
     
      @param argv command line parameter.
    **/
    public static void main(String[] argv) throws UIException {
        ShowPCDDatabaseAction showAction = new ShowPCDDatabaseAction();
        //showAction.setWorkspacePath(argv[0]);
        //showAction.setFPDFilePath(argv[1]);
        showAction.setWorkspacePath("M:/tianocore/edk2/trunk/edk2");
        showAction.setFPDFilePath("EdkNt32Pkg/Nt32.fpd");
        showAction.execute();
    }
}
