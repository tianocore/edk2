/** @file
 
 The file is used to override AbstractCellEditor to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging.workspace.command;

import java.io.File;
import java.io.IOException;

import org.tianocore.common.Log;
import org.tianocore.common.Tools;
import org.tianocore.packaging.MdkPkg;

/**
 The class is used to override AbstractCellEditor to provides customized interfaces 
 
 @since CreateMdkPkg 1.0

 **/
public class InstallWorkspace {
    /**
     Main class, reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     This is the default constructor
     Reserved
     
     **/
    public InstallWorkspace() {
        // TODO
    }

    /**
     Check if exist target dir
     
     @param strInstallDir The install target dir
     @retval true - The target exists
     @retval false - The target doesn't exist
     
     **/
    public static boolean isExistInstallDir(String strInstallDir) {
        File id = new File(strInstallDir);
        return id.exists();
    }

    /**
     Create install target dir
     
     @param strInstallDir The install target dir
     @retval true - Install success
     @retval false - Install fail
     
     **/
    public static boolean createInstallDir(String strInstallDir) {
        File id = new File(strInstallDir);
        try {
            return id.mkdir();
        } catch (Exception e) {
            System.out.print(e.getMessage());
            return false;
        }
    }

    /**
     Reserved
     
     @return boolean
     
     **/
    public static boolean setSystemEnvironment() {
        return true;
    }

    /**
     Reserved 
     
     @return boolean
     **/
    public static boolean setToolChainPath() {
        return true;
    }

    /**
     Reserved
     
     @return boolean
     **/
    public static boolean setToolChain() {
        return true;
    }

    /**
     Reserved
     
     @return boolean
     **/
    public static boolean setFrameworkDatabase() {
        return true;
    }

    /**
     Delete setup files and directory
     
     @param strPath The delete target dir
     @retval true - Delete success
     @retval false - Delete fail
     
     **/
    public static boolean delSetupPackage(String strPath) {
        File f = new File(strPath);
        try {
            Tools.deleteFolder(f);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return true;
    }

    /**
     
     @param strInstallDir The install target dir
     @param strJarFile The install target file
     @retval true - Install success
     @retval false - Install fail
     @throws IOException
     
     **/
    public static boolean installPackage(String strInstallDir, String strJarFile) throws IOException {
        Log.log("Install Dir", strInstallDir);
        Log.log("Jar File Path", strJarFile);

        MdkPkg mp = new MdkPkg(strJarFile);
        try {
            mp.install(strInstallDir + System.getProperty("file.separator"));
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            Log.log("Install Err", e.toString());
        }
        return false;
    }
}
