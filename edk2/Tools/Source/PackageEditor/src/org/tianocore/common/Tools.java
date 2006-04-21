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
	
}
