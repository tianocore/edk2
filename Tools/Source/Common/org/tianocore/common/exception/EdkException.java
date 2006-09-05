/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  EdkException.java

Abstract:

--*/

package org.tianocore.common.exception;

public class EdkException extends Exception {
    static final long serialVersionUID = -8494188017252114029L;

    public static boolean isPrintStack = false;

    public EdkException(String message) {
        super("[EdkException]:" + message);
    }

    public EdkException(String message, boolean traceStack) {
        super(message);

    }

    public EdkException(){
        super();
    }

    public EdkException(Exception e, String message){
        super("[EdkException]:" + message);
        if (isPrintStack){
            this.setStackTrace(e.getStackTrace());
        }
        e.printStackTrace();
    }
    public static void setIsprintStack (boolean flag){
        isPrintStack = flag;
    }
}
