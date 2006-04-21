/*
 *
 * Copyright 2004 The Ant-Contrib project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.sf.antcontrib.cpptasks;

import org.apache.tools.ant.types.EnumeratedAttribute;

/**
 * Enumeration of cpu types.
 *
 * @author Curt Arnold
 *
 */
public final class OSFamilyEnum
    extends EnumeratedAttribute {
  /**
   * Constructor.
   *
   * Set by default to "pentium3"
   *
   * @see java.lang.Object#Object()
   */
  public OSFamilyEnum() {
    setValue("windows");
  }

  /**
   * Gets list of acceptable values.
   *
   * @see org.apache.tools.ant.types.EnumeratedAttribute#getValues()
   */
  public String[] getValues() {
    return new String[] {
        "windows",
        "dos",
        "mac",
        "unix",
        "netware",
        "os/2",
        "tandem",
        "win9x",
        "z/os",
        "os/400",
        "openvms"};
  }
}
