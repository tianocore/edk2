/** @file

 The file is used to init workspace
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.workspace;

import java.io.File;

import org.tianocore.frameworkwizard.common.DataType;

public class Workspace {
    //
    // Define static return value
    //
    public final static int WORKSPACE_VALID = 0;

    public final static int WORKSPACE_NOT_DEFINED = 1;

    public final static int WORKSPACE_NOT_EXIST = 2;

    public final static int WORKSPACE_NOT_DIRECTORY = 3;

    public final static int WORKSPACE_NOT_VALID = 4;
    
    public final static int WORKSPACE_NO_TARGET_FILE = 5;

    //
    // Define class members
    //
    private static String currentWorkspace = null;

    private static String strWorkspaceDatabaseFile = DataType.FILE_SEPARATOR + "Tools" + DataType.FILE_SEPARATOR
                                                     + "Conf" + DataType.FILE_SEPARATOR + "FrameworkDatabase.db";

    private static String targetFile = DataType.FILE_SEPARATOR + "Tools" + DataType.FILE_SEPARATOR + "Conf"
                                       + DataType.FILE_SEPARATOR + "target.txt";

    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     Get Current Workspace
     
     @return currentWorkspace
     
     */
    public static String getCurrentWorkspace() {
        return currentWorkspace;
    }

    /**
     Set Current Workspace
     
     @param currentWorkspace
     The input data of currentWorkspace
     
     */
    public static void setCurrentWorkspace(String currentWorkspace) {
        Workspace.currentWorkspace = currentWorkspace;
    }

    /**
     Check if current workspace exists of not
     
     @retval true - The current WORKSPACE exists
     @retval false - The current WORKSPACE doesn't exist
     
     */
    public static int checkCurrentWorkspace() {
        return checkWorkspace(getCurrentWorkspace());
    }

    /**
     Check if current workspace exists or not via input workspace path
     
     @param strWorkspace
     The input data of WORKSPACE path
     @retval true - The current WORKSPACE exists
     @retval false - The current WORKSPACE doesn't exist
     
     */
    public static int checkWorkspace(String strWorkspace) {
        //
        // Check if WORKSPACE Environment is defined
        //
        if (strWorkspace == null || strWorkspace == "") {
            return Workspace.WORKSPACE_NOT_DEFINED;
        }

        //
        // Check if WORKSPACE Environment exists
        //
        File f = new File(strWorkspace);
        if (!f.exists()) {
            return Workspace.WORKSPACE_NOT_EXIST;
        }

        //
        // Check if WORKSPACE Environment is a directory
        //
        if (!f.isDirectory()) {
            return Workspace.WORKSPACE_NOT_DIRECTORY;
        }

        //
        // Check if FrameworkDatabase.db exists
        //
        f = new File(strWorkspace + Workspace.getStrWorkspaceDatabaseFile());
        if (!f.exists()) {
            return Workspace.WORKSPACE_NOT_VALID;
        }
        
        //
        // Check if Target.txt exists
        //
        f = new File(strWorkspace + Workspace.getTargetFile());
        if (!f.exists()) {
            return Workspace.WORKSPACE_NO_TARGET_FILE;
        }

        return Workspace.WORKSPACE_VALID;
    }

    public static String getStrWorkspaceDatabaseFile() {
        return strWorkspaceDatabaseFile;
    }

    public static void setStrWorkspaceDatabaseFile(String strWorkspaceDatabaseFile) {
        //Workspace.strWorkspaceDatabaseFile = strWorkspaceDatabaseFile;
    }

    public static String getTargetFile() {
        return targetFile;
    }
}
