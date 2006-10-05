/** @file
This file is to define the FileTimeStamp class for speeding up the dependency check.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.common.cache;

import java.io.File;
import java.util.Map;
import java.util.TreeMap;
import java.util.HashMap;

/**
   FileTimeStamp class is used to cache the time stamp of accessing file, which
   will speed up the dependency check for build
 **/
public class FileTimeStamp {
    //
    // cache the modified timestamp of files accessed, to speed up the depencey check
    // 
    private static Map<String, Long> timeStampCache = new HashMap<String, Long>();

    /**
       Get the time stamp of given file. It will try the cache first and then
       get from file system if no time stamp of the file is cached.

       @param file      File name
       
       @return long     The time stamp of the file
     **/
    synchronized public static long get(String file) {
        long timeStamp = 0;

        Long value = timeStampCache.get(file);
        if (value != null) {
            timeStamp = value.longValue();
        } else {
            timeStamp = new File(file).lastModified();
            timeStampCache.put(file, new Long(timeStamp));
        }

        return timeStamp;
    }

    /**
       Force update the time stamp for the given file

       @param file          File name
       @param timeStamp     The time stamp of the file
     **/
    synchronized public static void update(String file, long timeStamp) {
        timeStampCache.put(file, new Long(timeStamp));
    }

    /**
       Force update the time stamp for the given file

       @param file          File name
     **/
    synchronized public static void update(String file) {
        long timeStamp = new File(file).lastModified();
        timeStampCache.put(file, new Long(timeStamp));
    }

    /**
       Check if the time stamp of given file has been cached for not

       @param file      The file name
       
       @return boolean
     **/
    synchronized public static boolean containsKey(String file) {
        return timeStampCache.containsKey(file);
    }
}
