/** @file
  File is ContextMain class . 
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.context;

public class ContextMain {
    
    public static void main(String[] args) {

        if (TargetFile.validateFilename("target.txt") == false) {
            System.out.printf("%n%s", "Target.txt can't be found in WorkSpace. Please check it!");
            System.exit(0);
        }
        
        if(ParseParameter.checkParameter(args) == false){
            System.exit(0);
        }
        
        if (TargetFile.readFile() == false){
            System.exit(0);
        }
        
        if (ParseParameter.standardizeParameter(args) > 0){
            System.exit(0);
        }
        
        if (TargetFile.createTempFile("target.txt") == false){
            System.exit(0);
        }
        
        if (TargetFile.readwriteFile() == false){
            System.exit(0);
        }
        
        System.out.printf("%n%s", "Target.txt generate successfully!");
    }
}
