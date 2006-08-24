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
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.ModuleIdentification;

/**
  This class This class is to reorder library instance sequence according to
  library dependence.
**/
public class AutogenLibOrder {
    ///
    /// The map of library class and its library instance.
    ///
    private Map<String, ModuleIdentification> libClassMap = new HashMap<String, ModuleIdentification>();

    ///
    /// The map of library instance and its implemet libraryClass.
    ///
    private Map<ModuleIdentification, String[]> libInstanceMap = new HashMap<ModuleIdentification, String[]>();

    ///
    /// List of library instance. It is String[3] list, String[0] is libraryName,
    /// String[1] is libraryConstructor name, String[2] is libDestructor name.
    ///
    private List<LibraryInstanceNode> libInstanceList = new ArrayList<LibraryInstanceNode>();
    
    /**
      Constructor function
    
      This function mainly initialize some member variable.
     
      @param  libraryList   List of the library instance.
      @throws Exception
    **/
    AutogenLibOrder(ModuleIdentification[] libraryList, String arch) throws Exception {
        LibraryInstanceNode libInstanceNode;
        String[]       libClassDeclList = null;
        String[]       libClassConsmList = null;
        
        for (int i = 0; i < libraryList.length; i++) {
            //
            // Add libraryInstance in to libInstanceList.
            // 
            Map<String, XmlObject> libDoc = GlobalData.getDoc(libraryList[i], arch);
            SurfaceAreaQuery.push(libDoc);
            libInstanceNode = new LibraryInstanceNode (libraryList[i],SurfaceAreaQuery.getLibConstructorName(), SurfaceAreaQuery.getLibDestructorName());
            libInstanceList.add(libInstanceNode);
            
            //
            // Add library instance and consumed library class list to
            // libInstanceMap.
            //
            libClassConsmList = SurfaceAreaQuery
                    .getLibraryClasses(CommonDefinition.ALWAYSCONSUMED, arch);
            if (libClassConsmList != null) {
                String[] classStr = new String[libClassConsmList.length];
                for (int k = 0; k < libClassConsmList.length; k++) {
                    classStr[k] = libClassConsmList[k];
                }
                if (this.libInstanceMap.containsKey(libraryList[i])) {
                    throw new Exception(
                            libraryList[i].getName()
                                    + "this library instance already exists, please check the library instance list!");
                } else {
                    this.libInstanceMap.put(libraryList[i], classStr);
                }
            }

            //
            // Add library class and library instance map.
            //
            libClassDeclList = SurfaceAreaQuery
                    .getLibraryClasses(CommonDefinition.ALWAYSPRODUCED, arch);
            if (libClassDeclList != null) {
                for (int j = 0; j < libClassDeclList.length; j++) {
                    if (this.libClassMap.containsKey(libClassDeclList[j])) {
                        System.out.println(libClassDeclList[j]
                                + " class is already implement by "
                                + this.libClassMap.get(libClassDeclList[j]));
                        throw new Exception("Library Class: " + libClassDeclList
                                + " already has a library instance!");
                    } else {
                        this.libClassMap.put(libClassDeclList[j], libraryList[i]);
                    }
                }
            }
            SurfaceAreaQuery.pop();
        }

        //
        // Check is the library instance list meet the require;
        //
        //for (int s = 0; s < this.libInstanceList.size(); s++) {
        //    String[] libClass = this.libInstanceMap.get(this.libInstanceList
        //            .get(s));
        //    if (libClass != null) {
        //        for (int t = 0; t < libClass.length; t++) {
        //            if (this.libClassMap.get(libClass[t]) == null) {
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
        //           }
        //       }
        //   }
        //}
    }

    /**
      orderLibInstance
      
      This function reorder the library instance according the library class 
      dependency.
      
      @return     List which content the ordered library instance.
    **/
    List<ModuleIdentification> orderLibInstance() {
        List<ModuleIdentification> orderList = new ArrayList<ModuleIdentification>();
        //
        // Stack of node which track the library instance name ant its visiting
        // flag.
        //
        List<Node> stackList = new ArrayList<Node>();
        int stackSize = 0;
        ModuleIdentification libInstanceId = null;
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
            if (isInLibInstance(orderList, libInstanceList.get(i).libId)) {
                continue;
            }
            
            Node node = new Node(libInstanceList.get(i).libId, false);
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
                            stackList.get(stackSize).nodeId)) {
                        orderList.add(stackList.get(stackSize).nodeId);
                        stackList.remove(stackSize);
                    }
                    
                } else {
                    //
                    // Get the node value and set visit flag as true.
                    //
                    stackList.get(stackList.size() - 1).isVisit = true;
                    String[] libClassList = this.libInstanceMap.get(stackList
                            .get(stackSize).nodeId);
                    //
                    // Push the node dependence library instance to the stack.
                    //
                    if (libClassList != null) {
                        for (int j = 0; j < libClassList.length; j++) {
                            libInstanceId = this.libClassMap.get(libClassList[j]);
                            if (libInstanceId != null
                                    && !isInLibInstance(orderList, libInstanceId)) {
                                //
                                // If and only if the currently library instance
                                // is not in stack and it have constructor or 
                                // destructor function, push this library 
                                // instacne in stack.
                                //
                                if (!isInStackList(stackList, this.libClassMap
                                        .get(libClassList[j])) && isHaveConsDestructor(libInstanceId)) {
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
    private boolean isInLibInstance(List<ModuleIdentification> list, ModuleIdentification instanceId) {
        for (int i = 0; i < list.size(); i++) {
            
            if (instanceId.equals(list.get(i))) {
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
    private boolean isInStackList(List<Node> list, ModuleIdentification instanceId) {
        for (int i = 0; i < list.size(); i++) {
            if (instanceId.equals(list.get(i).nodeId)) {
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
    private boolean isHaveConsDestructor (ModuleIdentification libNode){
        for (int i = 0; i < libInstanceList.size(); i++){
            if (libInstanceList.get(i).libId.equals(libNode)){
                if (libInstanceList.get(i).constructorName != null || libInstanceList.get(i).deconstructorName != null){
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
    ModuleIdentification nodeId;

    boolean isVisit;

    Node(ModuleIdentification nodeId, boolean isVisit) {
        this.nodeId = nodeId;
        this.isVisit = false;
    }
}  
/**
  LibraryInstance Node   
  
  This class is used to store LibrayInstance and it's deconstructor and constructor
**/
    
class LibraryInstanceNode {
    ModuleIdentification libId;
    String deconstructorName;
    String constructorName;
    
    LibraryInstanceNode (ModuleIdentification libId, String deconstructor, String constructor){
        this.libId = libId;
        this.deconstructorName = deconstructor;
        this.constructorName   = constructor;
    }
}
