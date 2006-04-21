/**@file
 AutogenLibOrder class.

 This class is to reorder library instance sequence according to library 
 dependence.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.build.autogen;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.LibraryClassDocument.LibraryClass;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;

/**
  This class This class is to reorder library instance sequence according to
  library dependence.
**/
public class AutogenLibOrder {
    ///
    /// The map of library class and its library instance.
    ///
    private Map<String, String> libClassMap = new HashMap<String, String>();

    ///
    /// The map of library instance and its implemet instance.
    ///
    private Map<String, String[]> libInstanceMap = new HashMap<String, String[]>();

    ///
    /// List of library instance. It is String[3] list, String[0] is libraryName,
    /// String[1] is libraryConstructor name, String[2] is libDestructor name.
    ///
    private List<String[]> libInstanceList = new ArrayList<String[]>();
    
    /**
      Constructor function
    
      This function mainly initialize some member variable.
     
      @param  libraryList   List of the library instance.
      @throws Exception
    **/
    AutogenLibOrder(List<String> libraryList) throws Exception {
        String[]       libInstance = new String[3];
        LibraryClass[] libClassDeclList = null;
        LibraryClass[] libClassConsmList = null;
        
        for (int i = 0; i < libraryList.size(); i++) {
            //
            // Add libraryInstance in to libInstanceList.
            //
            libInstance[0] = libraryList.get(i);
            Map<String, XmlObject> libDoc = GlobalData.getDoc(libInstance[0]);
            SurfaceAreaQuery.push(libDoc);
            libInstance[1] = SurfaceAreaQuery.getLibConstructorName();
            libInstance[2] = SurfaceAreaQuery.getLibDestructorName();
            libInstanceList.add(libInstance.clone());
            
            //
            // Add library instance and consumed library class list to
            // libInstanceMap.
            //
            libClassConsmList = SurfaceAreaQuery
                    .getLibraryClassArray(CommonDefinition.AlwaysConsumed);
            if (libClassConsmList != null) {
                String[] classStr = new String[libClassConsmList.length];
                for (int k = 0; k < libClassConsmList.length; k++) {
                    classStr[k] = libClassConsmList[k].getStringValue();
                }
                if (this.libInstanceMap.containsKey(libInstance[0])) {
                    throw new Exception(
                            libInstance[0]
                                    + "this library instance is already exist, please check you library instance list!");
                } else {
                    this.libInstanceMap.put(libInstance[0], classStr);
                }
            }

            //
            // Add library class and library instance map.
            //
            libClassDeclList = SurfaceAreaQuery
                    .getLibraryClassArray(CommonDefinition.AlwaysProduced);
            if (libClassDeclList != null) {
                for (int j = 0; j < libClassDeclList.length; j++) {
                    if (this.libClassMap.containsKey(libClassDeclList[j]
                            .getStringValue())) {
                        System.out.println(libClassDeclList[j].getStringValue()
                                + " class is already implement by "
                                + this.libClassMap.get(libClassDeclList[j]
                                        .getStringValue()));
                        throw new Exception(libClassDeclList
                                + " is already have library instance!");
                    } else {
                        this.libClassMap.put(libClassDeclList[j]
                                .getStringValue(), libInstance[0]);
                    }
                }
            }
            SurfaceAreaQuery.pop();
        }

        //
        // Check is the library instance list meet the require;
        //
        for (int s = 0; s < this.libInstanceList.size(); s++) {
            String[] libClass = this.libInstanceMap.get(this.libInstanceList
                    .get(s));
            if (libClass != null) {
                for (int t = 0; t < libClass.length; t++) {
                    if (this.libClassMap.get(libClass[t]) == null) {
                        //
                        // Note: There exist a kind of module which depend on 
                        // library class with no instance or whose instance will
                        // never be linked into the module. 
                        // For this satuation, the module has the description of 
                        // library class in MSA file but no description of 
                        // corresponding library instance in MBD file. There 
                        // will be a warnig message given here after a standard 
                        // log way has been decided.
                        //
                    }
                }
            }
        }
    }

    /**
      orderLibInstance
      
      This function reorder the library instance according the library class 
      dependency.
      
      @return     List which content the ordered library instance.
    **/
    List orderLibInstance() {
        List<String> orderList = new ArrayList<String>();
        //
        // Stack of node which track the library instance name ant its visiting
        // flag.
        //
        List<Node> stackList = new ArrayList<Node>();
        int stackSize = 0;
        String libInstance = null;
        if (libInstanceList.size() < 0) {
            return null;
        }

        //
        // Reorder the library instance.
        //
        for (int i = 0; i < libInstanceList.size(); i++) {
            //
            // If library instance is already in the order list skip it.
            //
            if (isInLibInstance(orderList, libInstanceList.get(i)[0])) {
                continue;
            }
            
            Node node = new Node(libInstanceList.get(i)[0], false);
            //
            // Use stack to reorder library instance.
            // Push node to stack.
            //
            stackList.add(node);
            while (stackList.size() > 0) {
                stackSize = stackList.size() - 1;
                //
                // Pop the first node in stack. If the node flag has been visited
                // add this node to orderlist and remove it from stack.
                //
                if (stackList.get(stackSize).isVisit) {
                    if (!isInLibInstance(orderList,
                            stackList.get(stackSize).nodeName)) {
                        orderList.add(stackList.get(stackSize).nodeName);
                        stackList.remove(stackSize);
                    }
                    
                } else {
                    //
                    // Get the node value and set visit flag as true.
                    //
                    stackList.get(stackList.size() - 1).isVisit = true;
                    String[] libClassList = this.libInstanceMap.get(stackList
                            .get(stackSize).nodeName);
                    //
                    // Push the node dependence library instance to the stack.
                    //
                    if (libClassList != null) {
                        for (int j = 0; j < libClassList.length; j++) {
                            libInstance = this.libClassMap.get(libClassList[j]);
                            if (libInstance != null
                                    && !isInLibInstance(orderList, libInstance)) {
                                //
                                // If and only if the currently library instance
                                // is not in stack and it have constructor or 
                                // destructor function, push this library 
                                // instacne in stack.
                                //
                                if (!isInStackList(stackList, this.libClassMap
                                        .get(libClassList[j])) && isHaveConsDestructor(libInstance)) {
                                    stackList.add(new Node(this.libClassMap
                                            .get(libClassList[j]), false));
                                }
                            }
                        }
                    }
                }
            }
        }
        return orderList;
    }

    /**
      isInLibInstance
    
      This function check does the library instance already in the list.
    
      @param list             List of the library instance.
      @param instanceName     Name of library instance.
      @return                 "true" the library instance in list |
                              "false" the library instance is not in list.
    **/
    private boolean isInLibInstance(List list, String instanceName) {
        for (int i = 0; i < list.size(); i++) {
            if (instanceName.equalsIgnoreCase(list.get(i).toString())) {
                return true;
            }
        }
        return false;
    }

    /**
      isInStackList 
      
      This function check if the node already in the stack.
       
      @param list        Stack.
      @param nodeName    Name of node.
      @return            "true" if node have in stack |
                         "false" if node don't in stack.
    **/ 
    private boolean isInStackList(List<Node> list, String nodeName) {
        for (int i = 0; i < list.size(); i++) {
            if (nodeName.equalsIgnoreCase(list.get(i).nodeName)) {
                return true;
            }
        }
        return false;
    }
    
    /**
      isHaveConsDestructor
      
      This function check if the library have constructor or destructor 
      function.
      
      @param  libName    Name of library
      @return            "true" if library have constructor or desconstructor |
                         "false" if library don't have constructor 
                         and desconstructor.
    **/
    private boolean isHaveConsDestructor (String libName){
        for (int i = 0; i < libInstanceList.size(); i++){
            if (libInstanceList.get(i)[0].equalsIgnoreCase(libName)){
                if (libInstanceList.get(i)[1] != null || libInstanceList.get(i)[2] != null){
                    return true;
                }
            }
        }
        return false;
    }
}

/**
  Node 
 
  This class is used as stack node.
 
 **/
class Node {
    String nodeName;

    boolean isVisit;

    Node(String name, boolean isVisit) {
        this.nodeName = name;
        this.isVisit = false;
    }
}