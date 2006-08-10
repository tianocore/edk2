package org.tianocore.migration;

import java.io.*;
import java.util.*;
import java.util.regex.*;

/*
	Class ModuleInfo is built for scanning the source files, it contains all the needed
information and all the temporary data.
*/
public class ModuleInfo {
	ModuleInfo(String modulepath, UI ui, Database db) throws Exception {
		this.modulepath = modulepath;
		this.ui = ui;
		this.db = db;
		moduleScan();
	}
	
	private String modulepath = null;
	private Database db = null;
	private UI ui = null;
	
	public String modulename = null;
	public String guidvalue = null;
	public String moduletype = null;
	public String entrypoint = null;
	
	public Set<String> localmodulesources = new HashSet<String>();		//contains both .c and .h
	public Set<String> localmoduleheaders = new HashSet<String>();
	public Set<String> preprocessedccodes = new HashSet<String>();
	
	public Set<String> hashfuncc = new HashSet<String>();
	public Set<String> hashfuncd = new HashSet<String>();
	public Set<String> hashnonlocalfunc = new HashSet<String>();
	public Set<String> hashnonlocalmacro = new HashSet<String>();
	public Set<String> hashEFIcall = new HashSet<String>();
	public Set<String> hashr8only = new HashSet<String>();
	
	public Set<String> hashrequiredr9libs = new HashSet<String>();	// hashrequiredr9libs is now all added in SourceFileReplacer 
	public Set<String> guid = new HashSet<String>();
	public Set<String> protocol = new HashSet<String>();
	public Set<String> ppi = new HashSet<String>();
	
	private static String migrationcomment = "//%$//";
	
	private void moduleScan() throws Exception {
		String[] list = new File(modulepath).list();
		boolean hasInf = false;
		String infname = null;
		boolean hasMsa = false;
		String msaname = null;
		
		for (int i = 0 ; i < list.length ; i++) {
			if (new File(list[i]).isDirectory()) {
				;
			} else {
				if (list[i].contains(".c") || list[i].contains(".C")) {
					localmodulesources.add(list[i]);
				} else if (list[i].contains(".h") || list[i].contains(".H")) {
					localmodulesources.add(list[i]);
					localmoduleheaders.add(list[i]);	//the case that several .inf or .msa found is not concerned
				} else if (list[i].contains(".inf")) {
					if (ui.yesOrNo("Found .inf file : " + list[i] + "\nUse this file as this module's .inf ?")) {
						hasInf = true;
						infname = list[i];
					} else {
						continue;
					}
				} else if (list[i].contains(".msa")) {
					if (ui.yesOrNo("Found .msa file : " + list[i] + "\nUse this file as this module's .msa ?")) {
						hasMsa = true;
						msaname = list[i];
					} else {
						continue;
					}
				}
			}
		}
		
		ModuleReader mr = new ModuleReader(modulepath, this, db);
		if (hasInf) {							// this sequence shows using .inf as default
			mr.readInf(infname);
		} else if (hasMsa) {
			mr.readMsa(msaname);
		} else {
			ui.println("No Inf Nor Msa Found");
		}
		
		CommentOutNonLocalHFile();
		parsePreProcessedSourceCode();
		
		new SourceFileReplacer(modulepath, this, db, ui).flush();	// some adding library actions are taken here,so it must be put before "MsaWriter"
		
		// show result
		if (ui.yesOrNo("Parse Module Information Complete . See details ?")) {
			ui.println("\nModule Information : ");
			ui.println("Entrypoint : " + entrypoint);
			show(protocol, "Protocol : ");
			show(ppi, "Ppi : ");
			show(guid, "Guid : ");
			show(hashfuncc, "call : ");
			show(hashfuncd, "def : ");
			show(hashEFIcall, "EFIcall : ");
			show(hashnonlocalmacro, "macro : ");
			show(hashnonlocalfunc, "nonlocal : ");
			show(hashr8only, "hashr8only : ");
		}
		
		new MsaWriter(modulepath, this, db).flush();
		
		// remove temp directory
		//File tempdir = new File(modulepath + File.separator + "temp");
		//System.out.println("Deleting Dir");
		//if (tempdir.exists()) tempdir.d;
		
		ui.println("Errors Left : " + db.error);
		ui.println("Complete!");
		ui.println("Your R9 module is placed at " + modulepath + File.separator + "result");
		ui.println("Your logfile is placed at " + modulepath);
	}
	
	private void show(Set<String> hash, String show) {
		ui.println(show + hash.size());
		ui.println(hash);
	}

	// add '//' to all non-local include lines
	private void CommentOutNonLocalHFile() throws IOException {
		BufferedReader rd;
		String line;
		String curFile;
		PrintWriter outfile;

		Pattern ptninclude = Pattern.compile("[\"<](.*[.]h)[\">]");
		Matcher mtcinclude;
		
		File tempdir = new File(modulepath + File.separator + "temp" + File.separator);
		if (!tempdir.exists()) tempdir.mkdir();
		
		Iterator<String> ii = localmodulesources.iterator();
		while ( ii.hasNext() ) {
			curFile = ii.next();
			rd = new BufferedReader(new FileReader(modulepath + File.separator + curFile));
			outfile = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "temp" + File.separator + curFile)));
			while ((line = rd.readLine()) != null) {
				if (line.contains("#include")) {
					mtcinclude = ptninclude.matcher(line);
					if (mtcinclude.find() && localmoduleheaders.contains(mtcinclude.group(1))) {
					} else {
						line = migrationcomment + line;
					}
				}
				outfile.append(line + '\n');
			}
			outfile.flush();
			outfile.close();
		}
	}
	/*
	private void search(String line, Pattern ptn, Method md) {
		matmacro = Func.ptntmacro.matcher(line);
		while (matmacro.find()) {
			if ((temp = Func.registerMacro(matmacro, this, db)) != null) {
			}
		}
	}
	*/
	private void parsePreProcessedSourceCode() throws Exception {
		//Cl cl = new Cl(modulepath);
		//cl.execute("Fat.c");
		//cl.generateAll(preprocessedccodes);
		//
		//System.out.println("Note!!!! The CL is not implemented now , pls do it manually!!! RUN :");
		//System.out.println("cl " + modulepath + "\\temp\\*.c" + " -P");
		//String[] list = new File(modulepath + File.separator + "temp").list();	// without CL , add
		String[] list = new File(modulepath).list();
		for (int i = 0 ; i < list.length ; i++) {
			if (list[i].contains(".c")) {											// without CL , change to .i
				preprocessedccodes.add(list[i]);
			}
		}
		//
		Iterator<String> ii = preprocessedccodes.iterator();
		BufferedReader rd = null;
		String ifile = null;
		String line = null;
		String temp = null;
		//StringBuffer result = new StringBuffer();
		
		Pattern patefifuncc = Pattern.compile("g?(BS|RT)\\s*->\\s*([a-zA-Z_]\\w*)",Pattern.MULTILINE);
		Pattern patentrypoint = Pattern.compile("EFI_([A-Z]*)_ENTRY_POINT\\s*\\(([^\\(\\)]*)\\)",Pattern.MULTILINE);
		Matcher matguid;
		Matcher matfuncc;
		Matcher matfuncd;
		Matcher matenclosereplace;
		Matcher matefifuncc;
		Matcher matentrypoint;
		Matcher matmacro;
		
		while (ii.hasNext()) {
			StringBuffer wholefile = new StringBuffer();
			ifile = ii.next();
			rd = new BufferedReader(new FileReader(modulepath + File.separator + "temp" + File.separator + ifile));
			while ((line = rd.readLine()) != null) {
				wholefile.append(line + '\n');
			}
			line = wholefile.toString();
			
			// if this is a Pei phase module , add these library class to .msa
			matentrypoint = patentrypoint.matcher(line);
			if (matentrypoint.find()) {
				entrypoint = matentrypoint.group(2);
				if (matentrypoint.group(1).matches("PEIM")) {
					hashrequiredr9libs.add("PeimEntryPoint");
				} else {
					hashrequiredr9libs.add("UefiDriverEntryPoint");
				}
			}
			
			// find guid
			matguid = Guid.ptnguid.matcher(line);										// several ways to implement this , which one is faster ? :
			while (matguid.find()) {													// 1.currently , find once , then call to identify which is it
				if ((temp = Guid.register(matguid, this, db)) != null) {			// 2.use 3 different matchers , search 3 times to find each
					//matguid.appendReplacement(result, db.getR9Guidname(temp));		// search the database for all 3 kinds of guids , high cost
				}
			}
			//matguid.appendTail(result);
			//line = result.toString();

			// find EFI call in form of '->' , many 'gUnicodeCollationInterface->' like things are not changed
			// This item is not simply replaced , special operation is required.
			matefifuncc = patefifuncc.matcher(line);
			while (matefifuncc.find()) {
				hashEFIcall.add(matefifuncc.group(2));
			}

			// find function call
			matfuncc = Func.ptnfuncc.matcher(line);
			while (matfuncc.find()) {
				if ((temp = Func.register(matfuncc, this, db)) != null) {
					//ui.println(ifile + "  dofunc  " + temp);
					//matfuncc.appendReplacement(result, db.getR9Func(temp));
				}
			}
			//matfuncc.appendTail(result);
			//line = result.toString();

			// find macro
			matmacro = Macro.ptntmacro.matcher(line);
			while (matmacro.find()) {
				if ((temp = Macro.register(matmacro, this, db)) != null) {
				}
			}
			
			// find function definition
			// replace all {} to @
			while ((matenclosereplace = Func.ptnbrace.matcher(line)).find()) {
				line = matenclosereplace.replaceAll("@");
			}

			matfuncd = Func.ptnfuncd.matcher(line);
			while (matfuncd.find()) {
				if ((temp = Func.register(matfuncd, this, db)) != null) {
				}
			}
		}
		
		// op on hash
		Iterator<String> funcci = hashfuncc.iterator();
		while (funcci.hasNext()) {
			if (!hashfuncd.contains(temp = funcci.next()) && !hashEFIcall.contains(temp)) {
				hashnonlocalfunc.add(temp);					// this set contains both changed and not changed items
			}
		}
	}
	
	public static void main(String[] args) throws Exception {
		FirstPanel.init();
	}
}