/** @file
 
 The file is used to provides some useful interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.common;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.UUID;

/**
 The class is used to provides some useful interfaces 
 
 @since CreateMdkPkg 1.0
 
 **/
public class Tools {

    /**
     Used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        System.out.println(getCurrentDateTime());
    }

    /**
     Get current date and time and format it as "yyyy-MM-dd HH:mm"
     
     @return formatted current date and time
     
     **/
    public static String getCurrentDateTime() {
        Date now = new Date(System.currentTimeMillis());
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        return sdf.format(now);
    }

    /**
     Delete a folder and all its files
     
     @param fleFolderName The name of the folder which need be deleted
     
     @retval true - Delete successfully
     @retval false - Delete successfully
     
     **/
    public static boolean deleteFolder(File fleFolderName) {
        boolean blnIsDeleted = true;
        File[] aryAllFiles = fleFolderName.listFiles();

        for (int indexI = 0; indexI < aryAllFiles.length; indexI++) {
            if (blnIsDeleted) {
                if (aryAllFiles[indexI].isDirectory()) {
                    //
                    //If is a directory, recursively call this function to delete sub folders
                    //
                    blnIsDeleted = deleteFolder(aryAllFiles[indexI]);
                } else if (aryAllFiles[indexI].isFile()) {
                    //
                    //If is a file, delete it
                    //
                    if (!aryAllFiles[indexI].delete()) {
                        blnIsDeleted = false;
                    }
                }
            }
        }
        if (blnIsDeleted) {
            fleFolderName.delete();
        }
        return blnIsDeleted;
    }

    /**
     Generate a UUID
     
     @return the created UUID
     
     **/
    public static String generateUuidString() {
        return UUID.randomUUID().toString();
    }

    /**
     Get all system properties and output to the console
     
     **/
    public static void getSystemProperties() {
        System.out.println(System.getProperty("java.class.version"));
        System.out.println(System.getProperty("java.class.path"));
        System.out.println(System.getProperty("java.ext.dirs"));
        System.out.println(System.getProperty("os.name"));
        System.out.println(System.getProperty("os.arch"));
        System.out.println(System.getProperty("os.version"));
        System.out.println(System.getProperty("file.separator"));
        System.out.println(System.getProperty("path.separator"));
        System.out.println(System.getProperty("line.separator"));
        System.out.println(System.getProperty("user.name"));
        System.out.println(System.getProperty("user.home"));
        System.out.println(System.getProperty("user.dir"));
        System.out.println(System.getProperty("PATH"));

        System.out.println(System.getenv("PROCESSOR_REVISION"));
    }
}
