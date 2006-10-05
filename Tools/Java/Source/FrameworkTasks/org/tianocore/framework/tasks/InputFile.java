/** @file
This file is used to nest elements which is meant for specifying files

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
   InputFile is always used to pass arguments separated by space
 **/
public class InputFile extends NestElement {
    public String toString() {
        return super.toString(" ");
    }
}

