/*
 * 
 * Copyright 2001-2004 The Ant-Contrib project
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

/**
 * The equivalent of a Help About
 *    run "java -jar cpptasks.jar" to read
 *
 *  @author Curt Arnold
 */
public class AboutCCTask {
    /**
     * display identification message and exit
     *
     * @param args ignored
     */
    public static void main(String args[]) {
      System.out.println("CCTask: Compile and link task for Apache Ant 1.5 or later\n");
      System.out.println("Copyright (c) 2002-2004, The Ant-Contrib project.\n");
      System.out.println("http://sf.net/projects/ant-contrib\n");
      System.out.println("Licensed under the Apache Software License 2.0");
      System.out.println("available at http://www.apache.org/licenses/LICENSE-2.0\n");
      System.out.println("This software is not a product of the");
      System.out.println("of the Apache Software Foundation and no");
      System.out.println("endorsement or promotion is implied.\n");
      System.out.println("THIS SOFTWARE IS PROVIDED 'AS-IS', See");
      System.out.println("http://www.apache.org/LICENSE for additional");
      System.out.println("disclaimers.\n");
      System.out.println("To use:");
      System.out.println("\tPlace cpptasks.jar into lib directory of Ant 1.5 or later.");
      System.out.println("\tAdd <taskdef resource=\"cpptasks.tasks\"/> and");
      System.out.println("\t\t<typedef resource=\"cpptasks.types\"/> to build.xml");
      System.out.println("Add <cc/>, <compiler/>, <linker/>, <assembler/> and <aslcompiler> elements.");
    }
}
