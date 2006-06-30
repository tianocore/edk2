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

/**
 The class is used to provides static interfaces to save log and error information
 
 **/
public class Log {

    //
    //Log file
    //
    private static File fleLogFile = null;

    //
    //Err file
    //
    private static File fleErrFile = null;

    //
    //Log file name
    //
    static String strLogFileName = "Log.log";

    //
    //Err file name
    //
    static String strErrFileName = "Err.log";

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        try {
            Log.log("Test", "test");
            Log.err("Test1", "test1");
            Log.err("sdfsdfsd fsdfsdfsdfsdfj dsfksdjflsdjf sdkfjsdklfjsdkf dskfsjdkfjks dskfjsdklfjsdkf sdkfjsdlf sdkfjsdk kdfjskdf sdkfjsdkf ksdjfksdfjskdf sdkfsjdfksd fskdfjsdf", "dfsdf sdfksdf sd sdfksd fsdf");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     This is the default constructor
     Do nothing
     
     **/
    public Log() {
    }

    /**
     Call writeToLogFile to save log item and log information to log file
     
     @param strItem The log item
     @param strLog The log information
     
     **/
    public static void log(String strItem, String strLog) {
        try {
            writeToLogFile(strItem + ":" + strLog);
        } catch (IOException e) {
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
        } catch (IOException e) {
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
            showErrMessage("Error when " + strItem + "::" + strErr);
        } catch (IOException e) {
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
            showErrMessage("Error::" + strErr);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     Brings up a dialog to show err message
     When the message's length > defined max length, wrap the text to the next line.
     
     @param strErr The input data of err message
     
     **/
    private static void showErrMessage(String strErr) {
        int intMaxLength = 40;
        String strReturn = "";
        String strTemp = "";
        while (strErr.length() > 0) {
            if (strErr.length() > intMaxLength) {
                strTemp = strErr.substring(0, intMaxLength);
                strErr = strErr.substring(strTemp.length());
                strReturn = strReturn + strTemp + DataType.UNIX_LINE_SEPARATOR;
                
            } else if (strErr.length() <= intMaxLength) {
                strReturn = strReturn + strErr;
                strErr = "";
            }
        }
        JOptionPane.showConfirmDialog(null, strReturn, "Error", JOptionPane.DEFAULT_OPTION, JOptionPane.ERROR_MESSAGE);
    }

    /**
     Open log file and write log information
     
     @param strLog The log information
     @throws IOException
     
     **/
    private static void writeToLogFile(String strLog) throws IOException {
        try {
            if (fleLogFile == null) {
                fleLogFile = new File(strLogFileName);
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

    /**
     Open err file and write err information
     
     @param strLog The log information
     @throws IOException
     
     **/
    private static void writeToErrFile(String strLog) throws IOException {
        try {
            if (fleErrFile == null) {
                fleErrFile = new File(strErrFileName);
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
