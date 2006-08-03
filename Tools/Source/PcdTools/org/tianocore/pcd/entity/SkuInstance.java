/** @file
  SkuInstance class.

  Sku instance contains ID and value, A pcd token maybe contains more than one Sku instance.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 
package org.tianocore.pcd.entity;

/** 
   Sku instance contains ID and value, A pcd token maybe contains more than one Sku instance.
**/
public class SkuInstance {
    ///
    /// The id number of this SKU instance
    ///
    public int              id;

    ///
    /// The value of this SKU instance
    ///
    public DynamicTokenValue value;

    /**
      Constructure function

      @param id     sku id 
      @param value  sku value for this id.
    **/
    public SkuInstance(int id, DynamicTokenValue value) {
        this.id    = id;
        this.value = value;
    }

    /**
      Default constructor function.  
    **/
    public SkuInstance() {
        this.id    = 0;
        this.value = new DynamicTokenValue();
    }
}
