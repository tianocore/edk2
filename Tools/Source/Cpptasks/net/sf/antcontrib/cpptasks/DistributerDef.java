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


import org.apache.tools.ant.types.DataType;
import org.apache.tools.ant.types.Reference;
import java.util.Vector;

/**
 * Distributed build information (Non-functional prototype).
 *
 */
public final class DistributerDef
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
   * hosts.
   *
   */
  private String hosts;

  /**
   * Protocol.
   *
   */
  private DistributerProtocolEnum protocol;

  /**
   * Not sure what this is.
   */
  private int tcpCork;

  /**
   * user name.
   */
  private String user;

  /**
   *  local to remote file name maps.
   */
  private final Vector maps = new Vector();

  /**
   * Constructor.
   *
   */
  public DistributerDef() {
  }

  /**
   * Required by documentation generator.
   */
  public void execute() {
    throw new org.apache.tools.ant.BuildException(
        "Not an actual task, but looks like one for documentation purposes");
  }

  /**
   * Returns true if the if and unless conditions (if any) are
   * satisfied.
   * @return true if definition is active.
   */
  public boolean isActive() {
    return CUtil.isActive(getProject(), ifCond, unlessCond);
  }

  /**
   * Sets an id that can be used to reference this element.
   *
   * @param id
   *            id
   */
  public void setId(final String id) {
    //
    //  this is actually accomplished by a different
    //     mechanism, but we can document it
    //
  }

  /**
   * Sets the property name for the 'if' condition.
   *
   * The define will be ignored unless the property is defined.
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
   * Specifies that this element should behave as if the content of the
   * element with the matching id attribute was inserted at this location. If
   * specified, no other attributes should be specified.
   * @param r reference name
   */
  public void setRefid(final Reference r)  {
    super.setRefid(r);
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
   * Gets hosts.
   * @return hosts, may be null.
   *
   */
  public String getHosts() {
    if (isReference()) {
      DistributerDef refDistributer = (DistributerDef)
          getCheckedRef(DistributerDef.class,
                        "DistributerDef");
      return refDistributer.getHosts();
    }
    return hosts;
  }

  /**
   * Gets tcp cork.
   * @return TCP_CORK value.
   *
   */
  public int getTcpcork() {
    if (isReference()) {
      DistributerDef refDistributer = (DistributerDef)
          getCheckedRef(DistributerDef.class,
                        "DistributerDef");
      return refDistributer.getTcpcork();
    }
    return tcpCork;
  }

  /**
   * Gets protocol.
   * @return protocol, may be null.
   *
   */
  public DistributerProtocolEnum getProtocol() {
    if (isReference()) {
      DistributerDef refDistributer = (DistributerDef)
          getCheckedRef(DistributerDef.class,
                        "DistributerDef");
      return refDistributer.getProtocol();
    }
    return protocol;
  }

  /**
   * Sets hosts.
   * @param value new value
   */
  public void setHosts(final String value) {
    if (isReference()) {
      throw tooManyAttributes();
    }
    hosts = value;
  }

  /**
   * Sets TCP_CORK value.
   * @param value new value
   */
  public void setTcpcork(final int value) {
    if (isReference()) {
      throw tooManyAttributes();
    }
    tcpCork = value;
  }

  /**
   * Sets protocol.
   * @param value new value
   */
  public void setProtocol(final DistributerProtocolEnum value) {
    if (isReference()) {
      throw tooManyAttributes();
    }
    protocol = value;
  }

  /**
   *  Local to remote filename maps.
   * @return new map
   */
  public DistributerMap createMap() {
    DistributerMap map = new DistributerMap();
    map.setProject(getProject());
    maps.addElement(map);
    return map;
  }

  /**
   *  Sets remote user name.
   * @param value user name
   */
  public void setUser(final String value) {
    user = value;
  }

}
