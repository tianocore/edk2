/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  DefaultLogger.java

Abstract:

--*/

package org.tianocore.common.logger;

import java.io.File;
import java.util.logging.Level;
import java.util.logging.Logger;

class DefaultLogger implements LogMethod {
    private Logger logger = Logger.global;
    private static Level[] levelMap = {
        Level.SEVERE, Level.WARNING, Level.INFO, Level.FINE, Level.ALL
    };

    public DefaultLogger() {
    }

    public void putMessage(Object msgSource, int msgLevel, String msg) {
        if (msgLevel < 0 || msgLevel > levelMap.length) {
            msgLevel = 2;
        }
        logger.log(levelMap[msgLevel], msg);
    }
    
    public void flushToFile(File file){
        
    }
}