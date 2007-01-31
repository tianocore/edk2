/** @file
 
 The file is used to provides static interfaces to save log and error information 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import javax.swing.JOptionPane;

import org.tianocore.frameworkwizard.FrameworkWizardUI;
import org.tianocore.frameworkwizard.workspace.Workspace;

/**
 The class is used to provides static interfaces to save log and error information
 
 **/
public class Log {
    //
    //Log file directory path
    //
    private static String strLogDir = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR + "Tools"
                                      + DataType.FILE_SEPARATOR + "Logs";

    //
    //Log file
    //
    private static File fleLogFile = null;

    //
    //Wrn file
    //
    private static File fleWrnFile = null;

    //
    //Err file
    //
    private static File fleErrFile = null;

    //
    //Log file name
    //
    private static String strLogFileName = strLogDir + DataType.FILE_SEPARATOR + "frameworkwizard.log";

    //
    //Wrn file name
    //
    private static String strWrnFileName = strLogDir + DataType.FILE_SEPARATOR + "frameworkwizard.wrn";

    //
    //Err file name
    //
    private static String strErrFileName = strLogDir + DataType.FILE_SEPARATOR + "frameworkwizard.err";

    //
    //Flag for create log or not
    //
    private static boolean isSaveLog = false;

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        try {
            //Log.log("Test", "test");
            //Log.err("Test1", "test1");
            Log
               .wrn("aaa bbbbbb cccccccccccc ddddddddddd eeeeeeeeee fffffffffff gggggggggggggggggg hhhhhhhhhhhhhhhhhhhhhhhhhhhhh iiiii jjjj kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk lll mmm nn poooooooooooooooooooooooooooooooooooooooooooop");
            Log.wrn("Incorrect data type for ModuleEntryPoint");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Call writeToLogFile to save log item and log information to log file
     
     @param strItem The log item
     @param strLog The log information
     
     **/
    public static void log(String strItem, String strLog) {
        try {
            writeToLogFile(strItem + ":" + strLog);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Call writeToLogFile to save log information to log file
     
     @param strLog The log information
     
     **/
    public static void log(String strLog) {
        try {
            writeToLogFile(strLog);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Call writeToWrnFile to save wrn item and wrn information to wrn file
     
     @param strItem The wrn item
     @param strLog The wrn information
     
     **/
    public static void wrn(String strItem, String strWrn) {
        try {
            writeToWrnFile("Warning when " + strItem + "::" + strWrn);
            showWrnMessage(strWrn);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Call writeToWrnFile to save wrn information to wrn file
     
     @param strLog The wrn information
     
     **/
    public static void wrn(String strWrn) {
        try {
            writeToWrnFile("Warning::" + strWrn);
            showWrnMessage("Warning::" + strWrn);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Call writeToErrFile to save err item and err information to err file
     
     @param strItem The err item
     @param strLog The err information
     
     **/
    public static void err(String strItem, String strErr) {
        try {
            writeToErrFile("Error when " + strItem + "::" + strErr);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Call writeToErrFile to save err information to err file
     
     @param strLog The err information
     
     **/
    public static void err(String strErr) {
        try {
            writeToErrFile("Error::" + strErr);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     Brings up a dialog to show err message
     When the message's length > defined max length, wrap the text to the next line.
     
     @param strErr The input data of err message
     
     **/
    private static void showWrnMessage(String strErr) {
        String strReturn = Tools.wrapStringByWord(strErr);
        JOptionPane.showConfirmDialog(FrameworkWizardUI.getInstance(), strReturn, "Warning",
                                      JOptionPane.DEFAULT_OPTION, JOptionPane.ERROR_MESSAGE);
    }

    /**
     Open log file and write log information
     
     @param strLog The log information
     @throws IOException
     
     **/
    private static void writeToLogFile(String strLog) throws Exception {
        if (isSaveLog) {
            try {
                createLogDir();
                if (fleLogFile == null) {
                    fleLogFile = new File(strLogFileName);
                    fleLogFile.delete();
                    fleLogFile.createNewFile();
                }
                FileOutputStream fos = new FileOutputStream(fleLogFile, true);
                fos.write((Tools.getCurrentDateTime() + DataType.DOS_LINE_SEPARATOR).getBytes());
                fos.write((strLog + DataType.DOS_LINE_SEPARATOR).getBytes());
                fos.flush();
                fos.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     Open wrn file and write wrn information
     
     @param strLog The log information
     * @throws Exception 
     
     **/
    private static void writeToWrnFile(String strLog) throws Exception {
        if (isSaveLog) {
            try {
                createLogDir();
                if (fleWrnFile == null) {
                    fleWrnFile = new File(strWrnFileName);
                    fleWrnFile.delete();
                    fleWrnFile.createNewFile();
                }
                FileOutputStream fos = new FileOutputStream(fleWrnFile, true);
                fos.write((Tools.getCurrentDateTime() + DataType.DOS_LINE_SEPARATOR).getBytes());
                fos.write((strLog + DataType.DOS_LINE_SEPARATOR).getBytes());
                fos.flush();
                fos.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     Open err file and write err information
     
     @param strLog The log information
     @throws IOException
     
     **/
    private static void writeToErrFile(String strLog) throws Exception {
        if (isSaveLog) {
            try {
                createLogDir();
                if (fleErrFile == null) {
                    fleErrFile = new File(strErrFileName);
                    fleErrFile.delete();
                    fleErrFile.createNewFile();
                }
                FileOutputStream fos = new FileOutputStream(fleErrFile, true);
                fos.write((Tools.getCurrentDateTime() + DataType.DOS_LINE_SEPARATOR).getBytes());
                fos.write((strLog + DataType.DOS_LINE_SEPARATOR).getBytes());
                fos.flush();
                fos.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     Check if directory for Logs exists or not
     Create the directory if it doesn't exist  
     * @throws Exception 
     
     **/
    private static void createLogDir() throws Exception {
        File f = new File(strLogDir);
        if (!f.exists()) {
            FileOperation.newFolder(strLogDir);
        }
    }

    public static boolean isSaveLog() {
        return isSaveLog;
    }

    public static void setSaveLog(boolean isSaveLog) {
        Log.isSaveLog = isSaveLog;
    }
}
