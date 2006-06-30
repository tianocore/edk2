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
    // Define class members
    //
    private static String currentWorkspace = null;

    private static String strWorkspaceDatabaseFile = DataType.FILE_SEPARATOR + "Tools" + DataType.FILE_SEPARATOR
                                                     + "Conf" + DataType.FILE_SEPARATOR + "FrameworkDatabase.db";

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
    public static boolean checkCurrentWorkspace() {
        return checkWorkspace(getCurrentWorkspace());
    }

    /**
     Check if current workspace exists or not via input workspace path
     
     @param strWorkspace
     The input data of WORKSPACE path
     @retval true - The current WORKSPACE exists
     @retval false - The current WORKSPACE doesn't exist
     
     */
    public static boolean checkWorkspace(String strWorkspace) {
        if (strWorkspace == null || strWorkspace == "") {
            return false;
        }
        //
        // Check workspace directory
        //
        File f = new File(strWorkspace);
        if (!f.isDirectory()) {
            return false;
        }
        if (!f.exists()) {
            return false;
        }
        
        //
        // Check FrameworkDatabase.db
        //
        f = new File(strWorkspace + Workspace.getStrWorkspaceDatabaseFile());
        if (!f.exists()) {
            return false;
        }
        
        return true;
    }

    public static String getStrWorkspaceDatabaseFile() {
        return strWorkspaceDatabaseFile;
    }

    public static void setStrWorkspaceDatabaseFile(String strWorkspaceDatabaseFile) {
        //Workspace.strWorkspaceDatabaseFile = strWorkspaceDatabaseFile;
    }
}
