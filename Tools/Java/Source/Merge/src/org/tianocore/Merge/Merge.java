// @file
// Merge wrapper
//
//  Copyright (c) 2006, Intel Corporation    All rights reserved.
//
//  This program and the accompanying materials are licensed and made
//  available under the terms and conditions of the BSD License which
//  accompanies this distribution.  The full text of the license may 
//  be found at  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//

package org.tianocore.Merge;

public class Merge {

  public static void main(String[] args) {
      if (new MergeCmd().MergeCmdLine(args) != 0)
          System.exit(1);
      System.exit(0);
  }
}
