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

import java.io.File;
import java.io.IOException;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.types.DataType;

/**
 * Local to remote filename mapping (Experimental).
 *
 */
public final class DistributerMap
    extends DataType {
  /**
   * if property.
   */
  private String ifCond;

  /**
   * unless property.
   */
  private String unlessCond;

  /**
   * local directory name.
   *
   */
  private File localName;

  /**
   * Canonical local file name.
   */
  private String canonicalPath;

  /**
   * remote name.
   *
   */
  private String remoteName;

  /**
   * Separator (/ or \) character on remote system.
   */
  private char remoteSeparator = File.separatorChar;

  /**
   * hosts that for which this map is valid.
   *
   */
  private String hosts;

  /**
   * Constructor.
   *
   */
  public DistributerMap() {
  }

  /**
   * Required by documentation generator.
   */
  public void execute()  {
    throw new org.apache.tools.ant.BuildException(
        "Not an actual task, but looks like one for documentation purposes");
  }

  /**
   * Returns true if the if and unless conditions (if any) are
   * satisfied.
   *
   * @return true if this object is active.
   */
  public boolean isActive()  {
    return CUtil.isActive(getProject(), ifCond, unlessCond);
  }

  /**
   * Sets the property name for the 'if' condition.
   *
   * This object will be ignored unless the property is defined.
   *
   * The value of the property is insignificant, but values that would imply
   * misinterpretation ("false", "no") will throw an exception when
   * evaluated.
   *
   * @param propName
   *            property name
   */
  public void setIf(final String propName) {
    ifCond = propName;
  }

  /**
   * Set the property name for the 'unless' condition.
   *
   * If named property is set, the define will be ignored.
   *
   * The value of the property is insignificant, but values that would imply
   * misinterpretation ("false", "no") of the behavior will throw an
   * exception when evaluated.
   *
   * @param propName
   *            name of property
   */
  public void setUnless(final String propName) {
    unlessCond = propName;
  }

  /**
   * Gets local directory.
   * @return local directory, may be null.
   *
   */
  public File getLocal() {
    return localName;
  }

  /**
   * Gets remote name for directory.
   * @return remote name, may be null.
   *
   */
  public String getRemote() {
    return remoteName;
  }

  /**
   * Converts the local file name to the remote name for the same file.
   *
   * @param host host
   * @param localFile local file
   * @return remote name for local file, null if unknown.
   */
  public String toRemote(final String host, final File localFile) {
    if (remoteName != null
        && (hosts == null || hosts.indexOf(host) >= 0)) {
      try {
        String canonical = localFile.getCanonicalPath();
        if (canonical.startsWith(canonicalPath)) {
          if (isActive()) {
            return remoteName
                + canonical.substring(canonicalPath.length()).replace(File.
                separatorChar, remoteSeparator);
          }
        }
      } catch (IOException ex) {
        return null;
      }
    }
    return null;
  }

  /**
   * Sets local directory for base of mapping.
   *
   * @param value value
   */
  public void setLocal(final File value)  {
    if (value == null) {
      throw new NullPointerException("value");
    }
    if (value.exists() && !value.isDirectory()) {
      throw new BuildException("local should be a directory");
    }
    localName = value;
    try {
      canonicalPath = localName.getCanonicalPath();
    } catch (IOException ex) {
      throw new BuildException(ex);
    }
  }

  /**
   * Sets remote name for directory.
   * @param value remote name for directory
   */
  public void setRemote(final String value) {
    remoteName = value;
  }

  /**
   * Sets the separator character (/ or \) for the remote system.
   * @param value separator character
   */
  public void setRemoteSeparator(final String value)  {
    if (value != null && value.length() != 1) {
      throw new BuildException("remote separator must be a single character");
    }
    remoteSeparator = value.charAt(0);
  }

  /**
   * Sets hosts for which this mapping is valid.
   *
   * @param value hosts
   */
  public void setHosts(final String value) {
    hosts = value;
  }

}
