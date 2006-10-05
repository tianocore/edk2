/** @file
 
 The file is used to provides interfaces for file operations 
 
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
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;

public class FileOperation {

    /**
     
     @param args
     * @throws Exception 
     
     **/
    public static void main(String[] args) throws Exception {
        FileOperation.newFolder("C:\\aaa\\aaa\\aaa\\aaa\\aaa");
    }

    /**
     To new a folder
     
     @param folderPath The folder path to be created
     @throws Exception
     
     **/
    public static void newFolder(String folderPath) throws Exception {
        folderPath = Tools.convertPathToCurrentOsType(folderPath);
        File f = new File(folderPath);
        f.mkdirs();
    }

    /**
     Delete a file 
     
     @param filePath The file path to be deleted
     @throws Exception
     
     **/
    public static void delFile(String filePath) throws Exception {
        File f = new File(filePath);
        if (f.exists()) {
            f.delete();
        }
    }

    /**
     Delete a folder and all its files
     
     @param filePath The name of the folder which need be deleted 
     @throws Exception
     
     **/
    public static void delFolder(String filePath) throws Exception {
        File f = new File(filePath);
        if (!f.exists()) {
            return;
        }
        if (!f.isDirectory()) {
            return;
        }
        delFolder(f);
    }

    /**
     Delete a folder and all its files
     
     @param fleFolderName The name of the folder which need be deleted
     
     @retval true - Delete successfully
     @retval false - Delete successfully
     
     **/
    private static boolean delFolder(File fileName) throws Exception {
        boolean blnIsDeleted = true;

        File[] aryAllFiles = fileName.listFiles();

        for (int indexI = 0; indexI < aryAllFiles.length; indexI++) {
            if (blnIsDeleted) {
                if (aryAllFiles[indexI].isDirectory()) {
                    //
                    //If is a directory, recursively call this function to delete sub folders
                    //
                    blnIsDeleted = delFolder(aryAllFiles[indexI]);
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
            fileName.delete();
        }
        return blnIsDeleted;
    }

    /**
     Copy a file
     
     @param oldPath
     @param newPath
     @throws Exception
     
     **/
    public static void copyFile(String oldPath, String newPath) throws Exception {
        oldPath = Tools.convertPathToCurrentOsType(oldPath);
        newPath = Tools.convertPathToCurrentOsType(newPath);
        
        int byteCount = 0;
        File oldFile = new File(oldPath);
        
        File newFile = new File(Tools.getFilePathOnly(newPath));
        if (!newFile.exists()) {
            newFolder(Tools.getFilePathOnly(newPath));
        }

        if (oldFile.exists()) {
            InputStream is = new FileInputStream(oldPath);
            FileOutputStream fos = new FileOutputStream(newPath);
            byte[] buffer = new byte[1024];

            while ((byteCount = is.read(buffer)) != -1) {
                fos.write(buffer, 0, byteCount);
            }

            is.close();
        }
    }

    /**
     Copy a folder
     
     @param oldPath
     @param newPath
     @throws Exception
    
    **/
    public static void copyFolder(String oldPath, String newPath) throws Exception {
        File oldFile = new File(oldPath);

        //
        // Create new file path first
        //
        newFolder(newPath);

        String[] files = oldFile.list();
        File temp = null;
        for (int index = 0; index < files.length; index++) {
            if (oldPath.endsWith(DataType.FILE_SEPARATOR)) {
                temp = new File(oldPath + files[index]);
            } else {
                temp = new File(oldPath + DataType.FILE_SEPARATOR + files[index]);
            }

            if (temp.isFile()) {
                FileInputStream fis = new FileInputStream(temp);
                FileOutputStream fos = new FileOutputStream(newPath + DataType.FILE_SEPARATOR
                                                            + (temp.getName()).toString());
                byte[] b = new byte[1024 * 5];
                int len;
                while ((len = fis.read(b)) != -1) {
                    fos.write(b, 0, len);
                }
                fos.flush();
                fos.close();
                fis.close();
            }
            if (temp.isDirectory()) {
                copyFolder(oldPath + DataType.FILE_SEPARATOR + files[index], newPath + DataType.FILE_SEPARATOR
                                                                             + files[index]);
            }
        }
    }
}
