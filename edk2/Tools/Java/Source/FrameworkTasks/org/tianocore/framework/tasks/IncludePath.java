/** @file
This file is used to nest elements which is meant for include path name

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

package org.tianocore.framework.tasks;

/**
   IncludePath class is generally used to pass arguments with "-i" before each
   of them.
 **/
public class IncludePath extends NestElement {
    public String toString() {
        return super.toString(" -i ");
    }
}

