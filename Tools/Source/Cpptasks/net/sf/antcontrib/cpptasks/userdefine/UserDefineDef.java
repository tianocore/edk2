/*
 * 
 * Copyright 2002-2006 The Ant-Contrib project
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
package net.sf.antcontrib.cpptasks.userdefine;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.Vector;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.types.FileList;
import org.apache.tools.ant.types.FileSet;

import sun.nio.cs.ext.TIS_620;

import net.sf.antcontrib.cpptasks.ProcessorDef;
import net.sf.antcontrib.cpptasks.types.AslcompilerArgument;
import net.sf.antcontrib.cpptasks.types.ConditionalPath;
import net.sf.antcontrib.cpptasks.types.IncludePath;
import net.sf.antcontrib.cpptasks.types.LibrarySet;

public class UserDefineDef extends ProcessorDef{
    
    public UserDefineDef () {}
    
    private String type = "CC";
    private String includepathDelimiter;
    
    private File outdir;
    private File workdir;

    private String inputSuffix;
    private String outputSuffix;
    
    private Vector<IncludePath> includePaths= new Vector<IncludePath>();
    private Vector<FileList> fileSetList = new Vector<FileList>();
    
    /**
     * New adding for support GCC toolchain.
     * Most of those only have one value for example :
     * entryPoint, mapFile, pdbFile, define those as element because 
     * if attribut too much the command line is not good-lookinng.
     */
    
    private Vector<UserDefineElement> includeFiles = new Vector<UserDefineElement>();
    private Vector<UserDefineElement> outPutFiles = new Vector<UserDefineElement>();
    private Vector<UserDefineElement> subSystem = new Vector<UserDefineElement>();
    private Vector<UserDefineElement> entryPoint = new Vector<UserDefineElement>();
    private Vector<UserDefineElement> map = new Vector<UserDefineElement>();
    private Vector<UserDefineElement> pdb = new Vector<UserDefineElement>();
    private Vector<LibrarySet> libSet = new Vector<LibrarySet>();
    
    public void execute() throws org.apache.tools.ant.BuildException {
        throw new org.apache.tools.ant.BuildException(
                        "Not an actual task, but looks like one for documentation purposes");
    }


    public void addConfiguredArgument(UserDefineArgument arg) {
        if (isReference()) {
            throw noChildrenAllowed();
        }
        addConfiguredProcessorArg(arg);
    }
    
    /**
     * Creates an include path.
     */
    public IncludePath createIncludePath() {
        Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException("project must be set");
        }
        if (isReference()) {
            throw noChildrenAllowed();
        }
        IncludePath path = new IncludePath(p);
        includePaths.addElement(path);
        return path;
    }
    
    
    /**
     * Add a <includepath> if specify the file attribute
     * 
     * @throws BuildException
     *             if the specify file not exist
     */
    protected void loadFile(Vector activePath, File file) throws BuildException {
        FileReader fileReader;
        BufferedReader in;
        String str;
        if (!file.exists()) {
            throw new BuildException("The file " + file + " is not existed");
        }
        try {
            fileReader = new FileReader(file);
            in = new BufferedReader(fileReader);
            while ((str = in.readLine()) != null) {
                if (str.trim() == "") {
                    continue;
                }
                str = getProject().replaceProperties(str);
                activePath.addElement(str.trim());
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }
    
    /**
     * Returns the specific include path.
     */
    public String[] getActiveIncludePaths() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getActiveIncludePaths();
        }
        return getActivePaths(includePaths);
    }
    
    private String[] getActivePaths(Vector paths) {
        Project p = getProject();
        if (p == null) {
            throw new java.lang.IllegalStateException("project not set");
        }
        Vector activePaths = new Vector(paths.size());
        for (int i = 0; i < paths.size(); i++) {
            ConditionalPath path = (ConditionalPath) paths.elementAt(i);
            if (path.isActive(p)) {
                if (path.getFile() == null) {
                    String[] pathEntries = path.list();
                    for (int j = 0; j < pathEntries.length; j++) {
                        activePaths.addElement(pathEntries[j]);
                    }
                } else {
                    loadFile(activePaths, path.getFile());
                }
            }
        }
        String[] pathNames = new String[activePaths.size()];
        activePaths.copyInto(pathNames);
        return pathNames;
    }
    
    public String getIncludepathDelimiter() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getIncludepathDelimiter();
        }
        return includepathDelimiter;
    }

    public void setIncludepathDelimiter(String includepathDelimiter) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.includepathDelimiter = includepathDelimiter;
    }

    public String getInputSuffix() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getInputSuffix();
        }
        return inputSuffix;
    }

    public void setInputSuffix(String inputSuffix) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.inputSuffix = inputSuffix;
    }

    public File getOutdir() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getOutdir();
        }
        return outdir;
    }

    public void setOutdir(File outdir) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.outdir = outdir;
    }

    public String getOutputSuffix() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getOutputSuffix();
        }
        return outputSuffix;
    }

    public void setOutputSuffix(String outputSuffix) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.outputSuffix = outputSuffix;
    }

    public String getType() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getType();
        }
        return type;
    }

    public void setType(String type) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.type = type;
    }

    public File getWorkdir() {
        if (isReference()) {
            return ((UserDefineDef) getCheckedRef(UserDefineDef.class,
                            "UserDefineDef")).getWorkdir();
        }
        return workdir;
    }

    public void setWorkdir(File workdir) {
        if (isReference()) {
            throw tooManyAttributes();
        }
        this.workdir = workdir;
    }
    
    /**
     * Add an libSet.
     */
    public LibrarySet createLibset() {
        if (isReference()){
            throw noChildrenAllowed();
        }
        LibrarySet lib = new LibrarySet();
        libSet.addElement(lib);
        return lib;        
    }
    
    public String getLibSetString(){
        String libString = null;
        for (int i = 0; i < libSet.size(); i++){
            String[] libList = libSet.get(i).getLibs();
            for (int j = 0; j < libList.length; j++){
                libString = libString + libList[j] + " ";
            }
        }
        return libString;
    }
    
    public Vector<LibrarySet> getLibSet(){
        return this.libSet;        
    }
    
    /**
     * Add map element
     */
    public void addMap(UserDefineElement mapElement){
        if (isReference()){
            throw noChildrenAllowed();
        }else{
            this.map.addElement(mapElement);
        }
    }
    
    public Vector<UserDefineElement> getMap (){
        return this.map;
    }   
    
    public String getMapvalue (){
        if (this.map.size() > 0){
            /*
             * If user set more than one map use the first one. 
             */
            return this.map.get(0).value;
        }
        return null;
        
    }
    public String getMapFlag(){
        if (this.map.size() > 0){
            /*
             * If user set more than one map use the first one. 
             */
            return this.map.get(0).flag;
        }
        return null;
    }
    /**
     *  Add pdb element
     */
    public void addPdb(UserDefineElement pdbElement){
        if (isReference()){
            throw noChildrenAllowed();
        }
        this.pdb.addElement(pdbElement);
    }
    
    public Vector<UserDefineElement> getPdb(){
        return this.pdb;
    }
    public String getPdbvalue (){
        if (this.pdb.size() > 0){
            /*
             * If user set more than one pdb use the first one. 
             * 
             */
            return this.pdb.get(0).value;
        }
        return null;
        
    }
    public String getPdbFlag(){
        if (this.pdb.size() > 0){
            /*
             * If user set more than one pdb use the first one. 
             */
            return this.pdb.get(0).flag;
        }
        return null;
    }
    
    /**
     * add entryPoint element.
     */
    public void addEntryPoint(UserDefineElement entryPointElement){
        if (isReference()){
            throw noChildrenAllowed();
        }
        this.entryPoint.addElement(entryPointElement);
    }
    
    public Vector<UserDefineElement> getEntryPoint(){
        return this.entryPoint;
    }
    
    public String getEntryPointvalue (){
        if (this.entryPoint.size() > 0){
            /*
             * If user set more than one entryPoint use the first one. 
             */
            return this.entryPoint.get(0).value;
        }
        return null;
        
    }
    public String getEntryPointFlag(){
        if (this.entryPoint.size() > 0){
            /*
             * If user set more than one entry point use the first one. 
             */
            return this.entryPoint.get(0).flag;
        }
        return null;
    }
    
    /**
     * Add subSystem element.
     */
    public void addSubSystem (UserDefineElement subSystem){
        if (isReference()){
            throw noChildrenAllowed();
        }
        this.subSystem.addElement(subSystem);
    }
    public Vector<UserDefineElement> getSubSystem (){
        return this.subSystem;
    }
    
    public String getSubSystemvalue (){
        if (this.subSystem.size() > 0){
            /*
             * If user set more than one subsystem use the first one. 
             */
            return this.subSystem.get(0).value;
        }
        return null;
        
    }
    public String getSubSystemFlag(){
        if (this.subSystem.size() > 0){
            /*
             * If user set more than one subsystem use the first one. 
             */
            return this.subSystem.get(0).flag;
        }
        return null;
    }
    /**
     * Add includeFile element
     */
    public void addIncludeFile (UserDefineElement includeFile){
        if (isReference()){
            throw noChildrenAllowed();
        }
        this.includeFiles.addElement(includeFile);
    }
    public Vector<UserDefineElement> getIncludeFiles(){
        return this.includeFiles;
    }
    
    public String getIncludeFile (){
        if (this.includeFiles.size() > 0){
            /*
             * If user set more than one map use the first one. 
             */
            return this.includeFiles.get(0).value;
        }
        return null;
        
    }
    public String getIncludeFileFlag(){
        if (this.includeFiles.size() > 0){
            /*
             * If user set more than one map use the first one. 
             */
            return this.includeFiles.get(0).flag;
        }
        return null;
    }
    
    /**
     * Add OutputFile element
     */
    public void addOutputFile (UserDefineElement outPutFile){
        if (isReference()){
            throw noChildrenAllowed();
        }
        this.outPutFiles.addElement(outPutFile);
    }
    
    public Vector<UserDefineElement> getOutputFiles(){
        return this.outPutFiles;
    }
    
    public String getOutputFile (){
        if (this.outPutFiles.size() > 0){
            /*
             * If user set more than one map use the first one. 
             */
            return this.outPutFiles.get(0).value;
        }
        return null;
        
    }
    public String getOutPutFlag(){
        if (this.outPutFiles.size() > 0){
            /*
             * If user set more than one map use the first one. 
             */
            return this.outPutFiles.get(0).flag;
        }
        return null;
    }
    
    /**
     * Add fileSet list
     */
    public void addFileList(FileList fileSet){
        this.fileSetList.addElement(fileSet);
    }
    
    public Vector<String> getFileList(){
        Project p = getProject();
        Vector<String> fileListVector = new Vector<String>();
        for (int i = 0; i < this.fileSetList.size(); i++){
            String[] tempStrList = this.fileSetList.get(i).getFiles(p);
            for (int j = 0; j < tempStrList.length; j++){
                fileListVector .addElement(tempStrList[j]);
            }
        }
        return fileListVector;
    }
}
