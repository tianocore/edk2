/** @file
 * This file is ANT task VariableTask. 
 *
 * VariableTask task implements part of ANT property task. The difference is
 * this task will override variable with same name, but ANT property task do not.
 *
 * Copyright (c) 2006, Intel Corporation
 * All rights reserved. This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */
package org.tianocore.build.global;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

/**
 * VariableTask task implements part of ANT property task. The difference is
 * this task will override variable with same name, but ANT property task do not.
 * 
 * @since GenBuild 1.0
 */
public class VariableTask extends Task {

   /**
    * property value
    */
   private String value;
   
   /**
    * property name
    */
   private String name;

   /**
    * Set property name.
    *
    * @param name property name
    */
   public void setName( String name ) {
      this.name = name;
   }


   /**
    * Set property value.
    *
    * @param value  property value
    */
   public void setValue( String value ) {
      this.value = value;
   }

   /**
    * ANT task's entry point, will be called after init(). 
    *
    * @exception BuildException
    *            If name or value is null
    */
   public void execute() throws BuildException {
       if (name == null || value == null) {
           throw new BuildException("Name or value cannot be null.");
       }
       getProject().setProperty(name, value);
   }
}

