package org.tianocore.build;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.toolchain.ToolChainInfo;

public class FrameworkBuildTask extends Task{

    private Set<File> buildFiles = new LinkedHashSet<File>();
    
    private Set<File> fpdFiles = new LinkedHashSet<File>();
    
    private Set<File> msaFiles = new LinkedHashSet<File>();
    
    ///
    /// there are three type: all (build), clean and cleanall
    ///
    private String type = "all";
    
    public void execute() throws BuildException {
        //
        // Seach build.xml -> .FPD -> .MSA file
        //
        try {
            //
            // Gen Current Working Directory
            //
            File dummyFile = new File(".");
            File cwd = dummyFile.getCanonicalFile();
            File[] files = cwd.listFiles();
            for (int i = 0; i < files.length; i++) {
                if (files[i].isFile()) {
                    if (files[i].getName().equalsIgnoreCase("build.xml")) {
                        //
                        // First, search build.xml, if found, ANT call it
                        //
                        buildFiles.add(files[i]);

                    } else if (files[i].getName().endsWith(".fpd")) {
                        //
                        // Second, search FPD file, if found, build it
                        //
                        fpdFiles.add(files[i]);
                    } else if (files[i].getName().endsWith(".msa")) {
                        //
                        // Third, search MSA file, if found, build it
                        //
                        msaFiles.add(files[i]);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            throw new BuildException(e.getMessage());
        }
        
        //
        // If there is no build files or FPD files or MSA files, stop build
        //
        if (fpdFiles.size() == 0 && msaFiles.size() == 0) {
            throw new BuildException("Can't find any build.xml file or FPD files or MSA files in current directory. ");
        }
        
        File buildFile = intercommuniteWithUser();
        System.out.println("Start to build file [" + buildFile.getPath() + "] ..>> ");
        
        //
        // Deal with all environment variable (Add them to properties)
        //
        backupSystemProperties();
        
        //
        // Get ToolChain Info from environment
        //
        ToolChainInfo envToolChainInfo = new ToolChainInfo(); 
        envToolChainInfo.addTargets(getProject().getProperty("TARGET")); 
        envToolChainInfo.addTagnames(getProject().getProperty("TAGNAME")); 
        envToolChainInfo.addArchs(getProject().getProperty("ARCH")); 
        GlobalData.setToolChainEnvInfo(envToolChainInfo);
        
        //
        // Global Data initialization
        //
        String toolsDefFilename = "tools_def.txt";
        if (getProject().getProperty("TOOLS_DEF") != null) {
            toolsDefFilename = getProject().getProperty("TOOLS_DEF");
        }
        
        GlobalData.initInfo("Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db",
                            getProject().getProperty("WORKSPACE_DIR"), toolsDefFilename);
        
        //
        // Build every FPD files (PLATFORM build)
        //
        if (buildFile.getName().endsWith(".fpd")) {
            FpdParserTask fpdParserTask = new FpdParserTask();
            fpdParserTask.setType(type);
            fpdParserTask.setProject(getProject());
            fpdParserTask.setFpdFile(buildFile);
            fpdParserTask.execute();
        }
        
        //
        // Build every MSA files (SINGLE MODULE BUILD)
        //
        else if (buildFile.getName().endsWith(".msa")) {
            GenBuildTask genBuildTask = new GenBuildTask();
            genBuildTask.setType(type);
            genBuildTask.setProject(getProject());
            genBuildTask.setMsaFile(buildFile);
            genBuildTask.execute();
        }
    }
    
    /**
      Transfer system environment variables to ANT properties. If system variable 
      already exiests in ANT properties, skip it.
      
    **/
    private void backupSystemProperties() {
        Map<String, String> sysProperties = System.getenv();
        Set<String> keys = sysProperties.keySet();
        Iterator<String> iter = keys.iterator();
        while (iter.hasNext()) {
            String name = iter.next();
            
            //
            // If system environment variable is not in ANT properties, add it
            //
            if (getProject().getProperty(name) == null) {
                getProject().setProperty(name, sysProperties.get(name));
            }
        }
    }

    private File intercommuniteWithUser(){
        File file = null;
        if (fpdFiles.size() + msaFiles.size() > 1) {
            File[] allFiles = new File[fpdFiles.size() + msaFiles.size()];
            int index = 0;
            Iterator<File> iter = fpdFiles.iterator();
            while (iter.hasNext()) {
                allFiles[index] = iter.next();
                index++;
            }
            iter = msaFiles.iterator();
            while (iter.hasNext()) {
                allFiles[index] = iter.next();
                index++;
            }
            System.out.println("Find " + allFiles.length + " FPD and MSA files: ");
            for (int i = 0; i < allFiles.length; i++) {
                System.out.println("[" + (i + 1) + "]: " + allFiles[i].getName());
            }
            
            boolean flag = true;
            System.out.print("Please select one file to build:[1] ");
            do{
                BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
                try {
                     String str = br.readLine();
                     if (str.trim().length() == 0) {
                         file = allFiles[0];
                         flag = false;
                         continue ;
                     }
                     int indexSelect = Integer.parseInt(str);
                     if (indexSelect <=0 || indexSelect > allFiles.length) {
                         System.out.print("Please enter a number between [1.." + allFiles.length + "]:[1] ");
                         continue ;
                     } else {
                         file = allFiles[indexSelect - 1];
                         flag = false;
                         continue ;
                     }
                } catch (Exception e) {
                    System.out.print("Please enter a valid number:[1] ");
                    flag = true;
                }
            } while (flag);
        }
        else if (fpdFiles.size() == 1) {
            file = fpdFiles.toArray(new File[1])[0];
        }
        else if (msaFiles.size() == 1) {
            file = msaFiles.toArray(new File[1])[0];
        }
        return file;
    }
    
    
    public void setType(String type) {
        if (type.equalsIgnoreCase("clean") || type.equalsIgnoreCase("cleanall")) {
            this.type = type.toLowerCase();
        }
        else {
            this.type = "all";
        }
    }
}
