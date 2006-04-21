/*++

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 Module Name:
 ShareObject.java

 Abstract:

 --*/
package org.tianocore.build.global;

import java.util.*;

import org.apache.tools.ant.*;
import org.apache.tools.ant.types.DataType;

public class GlobalShare extends DataType implements DynamicConfigurator {
    private static final HashMap<String, Object> objStorage = new HashMap<String, Object>();

    private DataObjectOp op;

    private String objName;

    private Object objInst;

    private String objClassPackage = "org.tianocore";

    public GlobalShare () {

    }

    public GlobalShare (String objName) {
        this.objName = objName;
        this.objInst = objStorage.get(this.objName);
    }

    public GlobalShare (String objName, Object obj) {
        this.objName = objName;
        this.objInst = obj;
        objStorage.put(this.objName, this.objInst);
    }

    public Object createDynamicElement(String name) throws BuildException {
        String className = objClassPackage + "." + name;
        log("GlobalShare.createDynamicElement(" + name + ")",
                        Project.MSG_VERBOSE);
        try {
            objInst = Class.forName(className).newInstance();
        } catch (ClassNotFoundException e) {
            throw new BuildException("class name is not found");
        } catch (InstantiationException e) {
            throw new BuildException("the class cannnot be instantiated");
        } catch (IllegalAccessException e) {
            throw new BuildException("cannot access the class");
        }

        return objInst;
    }

    public void setDynamicAttribute(String name, String value)
                    throws BuildException {
        log("name = " + name + " value = " + value, Project.MSG_VERBOSE);
        throw new BuildException();
    }

    public void setName(String name) {
        this.objName = name;
        if (this.op != null) {
            issueOperation();
        }
    }

    public String getName() {
        return this.objName;
    }

    public void setPackage(String name) {
        log("ShareObject.setPackage(" + name + ")", Project.MSG_VERBOSE);
        this.objClassPackage = name;
    }

    public String getPackage() {
        return this.objClassPackage;
    }

    public void setOperation(String opName) {
        log("ShareObject.setOperation(" + opName + ")", Project.MSG_VERBOSE);
        this.op = DataObjectOp.formString(opName);

        if (this.objName != null) {
            issueOperation();
        }
    }

    public String getOperation() {
        return this.op.toString();
    }

    public void issueOperation() {
        if (this.op == DataObjectOp.ADD) {

            log("ShareObject: adding ... " + this.objName, Project.MSG_VERBOSE);
            objStorage.put(this.objName, this.objInst);

        } else if (this.op == DataObjectOp.GET) {

            log("ShareObject: fetching ... " + this.objName,
                            Project.MSG_VERBOSE);
            objInst = objStorage.get(objName);

        } else if (this.op == DataObjectOp.DEL) {

            log("ShareObject: removing ... " + this.objName,
                            Project.MSG_VERBOSE);
            objInst = objStorage.remove(objName);

        } else {
            throw new BuildException("not supported operation");
        }
    }

    public Object get() {
        return this.objInst;
    }

    public static int getObjectNum() {
        return objStorage.size();
    }

    public static Object add(String objName, Object obj) {
        return objStorage.put(objName, obj);
    }

    public static Object retrieve(String objName) {
        return objStorage.get(objName);
    }

    public static Object remove(String objName) {
        return objStorage.remove(objName);
    }

    public static void empty() {
        objStorage.clear();
    }
}

class DataObjectOp {
    private static final HashMap<String, DataObjectOp> opMap = new HashMap<String, DataObjectOp>();

    private final String opName;

    private DataObjectOp (String name) {
        this.opName = name;
        opMap.put(this.opName, this);
    }

    public String toString() {
        return opName;
    }

    public static DataObjectOp formString(String opName) {
        return opMap.get(opName);
    }

    public static final DataObjectOp ADD = new DataObjectOp("ADD");

    public static final DataObjectOp GET = new DataObjectOp("GET");

    public static final DataObjectOp DEL = new DataObjectOp("DEL");
}
