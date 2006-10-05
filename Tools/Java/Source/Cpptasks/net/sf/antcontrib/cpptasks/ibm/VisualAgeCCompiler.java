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
package net.sf.antcontrib.cpptasks.ibm;
import java.io.File;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.compiler.LinkType;
import net.sf.antcontrib.cpptasks.compiler.Linker;
import net.sf.antcontrib.cpptasks.gcc.GccCompatibleCCompiler;
import net.sf.antcontrib.cpptasks.OptimizationEnum;

import org.apache.tools.ant.types.Environment;
/**
 * Adapter for the IBM(r) Visual Age(tm) C++ compiler for AIX(tm)
 * 
 * @author Curt Arnold
 */
public final class VisualAgeCCompiler extends GccCompatibleCCompiler {
    private static final VisualAgeCCompiler instance = new VisualAgeCCompiler(
            "xlC", false, null);
    /**
     * Gets singleton instance of this class
     */
    public static VisualAgeCCompiler getInstance() {
        return instance;
    }
    private String identifier;
    private File[] includePath;
    /**
     * Private constructor. Use getInstance() to get singleton instance of this
     * class.
     */
    private VisualAgeCCompiler(String command, boolean newEnvironment,
            Environment env) {
        super(command, "-help", false, null, newEnvironment, env);
    }
    public void addImpliedArgs(final Vector args, 
    		final boolean debug,
            final boolean multithreaded, 
			final boolean exceptions, 
			final LinkType linkType,
			final Boolean rtti,
			final OptimizationEnum optimization) {
        args.addElement("-c");
        if (debug) {
            args.addElement("-g");
        }
        if (linkType.isSharedLibrary()) {
            args.addElement("-fpic");
        }
        if (rtti != null) {
        	if (rtti.booleanValue()) {
        		args.addElement("-qrtti=all");
        	} else {
        		args.addElement("-qnortti");
        	}
        }
    }
    public void addWarningSwitch(Vector args, int level) {
        switch (level) {
            case 0 :
                args.addElement("-w");
                break;
            case 1 :
                args.addElement("-qflag=s:s");
                break;
            case 2 :
                args.addElement("-qflag=e:e");
                break;
            case 3 :
                args.addElement("-qflag=w:w");
                break;
            case 4 :
                args.addElement("-qflag=i:i");
                break;
            case 5 :
                args.addElement("-qhalt=w:w");
                break;
        }
    }
    public Linker getLinker(LinkType linkType) {
        return VisualAgeLinker.getInstance().getLinker(linkType);
    }
    public int getMaximumCommandLength() {
        return Integer.MAX_VALUE;
    }
    /**
     * Gets identifier for the compiler.
     * 
     * Initial attempt at extracting version information
     * would lock up.  Using a stock response.
     */
    public String getIdentifier() {
    	return "VisualAge compiler - unidentified version";
    }
    
}
