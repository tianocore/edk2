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
 * Enumeration of cpu architecture types.
 *
 * @author Curt Arnold
 *
 */
public final class ArchEnum
    extends EnumeratedAttribute {
  /**
   * Constructor.
   *
   * Set by default to "pentium3"
   *
   * @see java.lang.Object#Object()
   */
  public ArchEnum() {
    setValue("pentium3");
  }

  /**
   * Gets list of acceptable values.
   *
   * @see org.apache.tools.ant.types.EnumeratedAttribute#getValues()
   */
  public String[] getValues() {
    /**
     * Class initializer.
     */
     return new String[] {
          "i386",
          "i486",
          "i586",
          "i686",
          "pentium",
          "pentium-mmx",
          "pentiumpro",
          "pentium2",
          "pentium3",
          "pentium4",
          "k6",
          "k6-2",
          "k6-3",
          "athlon",
          "athlon-tbird",
          "athlon-4",
          "athlon-xp",
          "athlon-mp",
          "winchip-c6",
          "winchip2",
          "c3"};
  }
}
