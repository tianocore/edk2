/** @file
  Java class Tools contains common use procedures.
 
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
import java.util.Calendar;
import java.util.Date;
import java.util.UUID;

/**
 This class contains static methods for some common operations
  
 @since PackageEditor 1.0
**/
public class Tools {
	
        public static final String guidArrayPat = "0x[a-fA-F0-9]{1,8},( )*0x[a-fA-F0-9]{1,4},( )*0x[a-fA-F0-9]{1,4}(,( )*\\{)?(,?( )*0x[a-fA-F0-9]{1,2}){8}( )*(\\})?";
	public static final String guidRegistryPat = "[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}";
	/**
	  get current date and time, then return
	  @return String
	 **/
	public static String getCurrentDateTime() {
		Date now = new Date(System.currentTimeMillis());
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
	    return sdf.format(now);
	}
	
	/**
	  Delete a folder and all its files
	  @param strFolderName
	  @return boolean
	 **/
	public static boolean deleteFolder(File fleFolderName) {
		boolean blnIsDeleted = true;
		File[] aryAllFiles = fleFolderName.listFiles();
		
		for (int indexI = 0; indexI < aryAllFiles.length; indexI++) {
			if (blnIsDeleted) {
				if (aryAllFiles[indexI].isDirectory()) {
					blnIsDeleted  = deleteFolder(aryAllFiles[indexI]);
				} else if (aryAllFiles[indexI].isFile()) {
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
	 Get a new GUID
     
	 @return String
	**/
	public static String generateUuidString() {
		return UUID.randomUUID().toString();
	}
	
    public static String formatGuidString (String guidNameConv) {
        String[] strList;
        String guid = "";
        int index = 0;
        if (guidNameConv
                        .matches(Tools.guidRegistryPat)) {
            strList = guidNameConv.split("-");
            guid = "0x" + strList[0] + ", ";
            guid = guid + "0x" + strList[1] + ", ";
            guid = guid + "0x" + strList[2] + ", ";
//            guid = guid + "{";
            guid = guid + "0x" + strList[3].substring(0, 2) + ", ";
            guid = guid + "0x" + strList[3].substring(2, 4);

            while (index < strList[4].length()) {
                guid = guid + ", ";
                guid = guid + "0x" + strList[4].substring(index, index + 2);
                index = index + 2;
            }
//            guid = guid + "}";
            return guid;
        }
        else if (guidNameConv
                        .matches(Tools.guidArrayPat)) {
            strList = guidNameConv.split(",");
            
            //
            // chang ANSI c form to registry form
            //
            for (int i = 0; i < strList.length; i++){
                strList[i] = strList[i].substring(strList[i].lastIndexOf("x") + 1);
            }
            if (strList[strList.length - 1].endsWith("}")) {
                strList[strList.length -1] = strList[strList.length-1].substring(0, strList[strList.length-1].length()-1); 
            }
            //
            //inserting necessary leading zeros
            //
            
            int segLen = strList[0].length();
            if (segLen < 8){
                for (int i = 0; i < 8 - segLen; ++i){
                    strList[0] = "0" + strList[0];
                }
            }
            
            segLen = strList[1].length();
            if (segLen < 4){
                for (int i = 0; i < 4 - segLen; ++i){
                    strList[1] = "0" + strList[1];
                }
            }
            segLen = strList[2].length();
            if (segLen < 4){
                for (int i = 0; i < 4 - segLen; ++i){
                    strList[2] = "0" + strList[2];
                }
            }
            for (int i = 3; i < 11; ++i) {
                segLen = strList[i].length();
                if (segLen < 2){
                    strList[i] = "0" + strList[i];
                }
            }
            
            for (int i = 0; i < 3; i++){
                guid += strList[i] + "-";
            }
            
            guid += strList[3];
            guid += strList[4] + "-";
            
            for (int i = 5; i < strList.length; ++i){
                guid += strList[i];
            }
            
            
            return guid;
        } else {
            
            return "0";

        }
    }
}
