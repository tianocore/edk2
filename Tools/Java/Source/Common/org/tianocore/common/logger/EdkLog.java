/*++

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 Module Name:
 EdkLogger.java

 Abstract:

 --*/
package org.tianocore.common.logger;

import java.io.File;

public class EdkLog {
    public static final String always = "ALWAYS";

    public static final String error = "ERROR";

    public static final String warning = "WARNING";

    public static final String info = "INFO";

    public static final String verbose = "VERBOSE";

    public static final String debug = "DEBUG";

    public static final int EDK_ALWAYS = -1;

    public static final int EDK_ERROR = 0;

    public static final int EDK_WARNING = 1;

    public static final int EDK_INFO = 2;

    public static final int EDK_VERBOSE = 3;

    public static final int EDK_DEBUG = 4;

    private static int logLevel = EDK_INFO;

    private static LogMethod logger = new DefaultLogger();

    public static void log(int level, String message) {
        if (level <= logLevel) {
            logger.putMessage(null, level, message);
        }
    }

    public static void log(String message) {
        if (EDK_INFO <= logLevel) {
            logger.putMessage(null, EDK_INFO, message);
        }
    }
    
    public static void log(Object o, int level, String message) {
        if (level <= logLevel) {
            logger.putMessage(o, level, message);
        }
    }

    public static void log(Object o, String message) {
        if (EDK_INFO <= logLevel) {
            logger.putMessage(o, EDK_INFO, message);
        }
    }

    public static void flushLogToFile(File file) {
        logger.flushToFile(file);
    }

    public static void setLogger(LogMethod l) {
        logger = l;
    }

    public static void setLogLevel(int level) {
        logLevel = level;
    }

    public static void setLogLevel(String level) {
        if (level == null) {
            return;
        }
        String levelStr = level.trim();
        if (levelStr.equalsIgnoreCase(error)) {
            logLevel = EDK_ERROR;
        }
        if (levelStr.equalsIgnoreCase(debug)) {
            logLevel = EDK_DEBUG;
        }
        if (levelStr.equalsIgnoreCase(info)) {
            logLevel = EDK_INFO;
        }
        if (levelStr.equalsIgnoreCase(verbose)) {
            logLevel = EDK_VERBOSE;
        }
        if (levelStr.equalsIgnoreCase(warning)) {
            logLevel = EDK_WARNING;
        }
    }

    public static int getLogLevel() {
        return logLevel;
    }
}
