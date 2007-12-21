/** @file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.migration;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.StringReader;
import java.util.Iterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.FilenameDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.SourceFilesDocument;

public final class ModuleReader implements Common.ForDoAll {
	private static final ModuleReader modulereader = new ModuleReader();

	private ModuleInfo mi;

	private final CommentLaplace commentlaplace = new CommentLaplace();

	private static final Pattern ptninfequation = Pattern
			.compile("([^\\s]*)\\s*=\\s*([^\\s]*)");

	private static final Pattern ptnsection = Pattern.compile(
			"\\[([^\\[\\]]*)\\]([^\\[\\]]*)\\n", Pattern.MULTILINE);

	private static final Pattern ptnfilename = Pattern.compile("[^\\s]+");

	public final void ModuleScan() throws Exception {
		Common.toDoAll(mi.modulepath, ModuleInfo.class.getMethod("enroll",
				String.class), mi, null, Common.FILE);

		// inf&msa
		String filename = null;
		if (mi.msaorinf.isEmpty()) {
			MigrationTool.ui.println("No INF nor MSA file found!");
			System.exit(0);
		} else {
			if (mi.msaorinf.size() == 1) {
				filename = (String) mi.msaorinf.toArray()[0];
			} else {
				filename = MigrationTool.ui.choose(
						"Found .inf or .msa file for module\n" + mi.modulepath
								+ "\nChoose one Please", mi.msaorinf.toArray());
			}
		}

		if (filename.contains(".inf")) {
			readInf(filename);
		} else if (filename.contains(".msa")) {
			readMsa(filename);
		}
		// inf&msa

		preProcessModule();
	}

	private final void readMsa(String name) throws Exception {
		ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory
				.parse(new File(mi.modulepath + File.separator + name));
		ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = msadoc
				.getModuleSurfaceArea();
		MsaHeaderDocument.MsaHeader msaheader = msa.getMsaHeader();

		mi.modulename = msaheader.getModuleName();
		mi.guidvalue = msaheader.getGuidValue();
		mi.moduletype = msaheader.getModuleType().toString(); // ???

		SourceFilesDocument.SourceFiles sourcefiles = msa.getSourceFiles();

		String temp;
		Iterator<FilenameDocument.Filename> li = sourcefiles.getFilenameList()
				.iterator();
		while (li.hasNext()) {
			if (!mi.localmodulesources.contains(temp = li.next().toString())) {
				System.out.println("Source File Missing! : " + temp);
			}
		}
	}

	private final String extractLicense(String wholeline) throws Exception {
		String tempLine;
		String license = null;

		BufferedReader rd = new BufferedReader(new StringReader(wholeline));
		while ((tempLine = rd.readLine()) != null) {
			if (tempLine.contains("#")) {
				if (tempLine.contains("Copyright")) {
					//
					// Find license info.
					// 
					license = "";
					while ((tempLine = rd.readLine()) != null) {
						if (!tempLine.contains("#")
								|| tempLine.contains("Module Name:")
								|| tempLine.contains("Abstract:")) {
							//
							// We assume license ends here.
							// 
							break;
						}
						license += "      "
								+ tempLine
										.replaceAll("\\s*[#]\\s*(.*)", "$1\n");
					}
					break;
				}
			}
		}
		return license;
	}

	private final void readInf(String name) throws Exception {
		System.out.println("\nParsing INF file: " + name);
		String wholeline;
		Matcher mtrinfequation;
		Matcher mtrsection;
		Matcher mtrfilename;

		wholeline = Common.file2string(mi.modulepath + File.separator + name);
		mi.license = extractLicense(wholeline);
		mtrsection = ptnsection.matcher(wholeline);
		while (mtrsection.find()) {
			if (mtrsection.group(1).matches("defines")) {
				mtrinfequation = ptninfequation.matcher(mtrsection.group(2));
				while (mtrinfequation.find()) {
					if (mtrinfequation.group(1).matches("BASE_NAME")) {
						mi.modulename = mtrinfequation.group(2);
					}
					if (mtrinfequation.group(1).matches("FILE_GUID")) {
						mi.guidvalue = mtrinfequation.group(2);
					}
					if (mtrinfequation.group(1).matches("COMPONENT_TYPE")) {
						mi.moduletype = mtrinfequation.group(2);
						if (mi.moduletype.matches("LIBRARY")) {
							mi.isLibrary = true;
						}
					}
				}
			}
			if (mtrsection.group(1).contains("nmake.")) {
				mtrinfequation = ptninfequation.matcher(mtrsection.group(2));
				while (mtrinfequation.find()) {
					if (mtrinfequation.group(1).matches("IMAGE_ENTRY_POINT")) {
						mi.entrypoint = mtrinfequation.group(2);
					}
					if (mtrinfequation.group(1).matches("DPX_SOURCE")) {
						if (!mi.localmodulesources.contains(mtrinfequation
								.group(2))) {
							MigrationTool.ui.println("DPX File Missing! : "
									+ mtrinfequation.group(2));
						}
					}
				}
			}
			if (mtrsection.group(1).contains("sources.")) {
				mtrfilename = ptnfilename.matcher(mtrsection.group(2));
				while (mtrfilename.find()) {
					mi.infsources.add(mtrfilename.group());
					if (!mi.localmodulesources.contains(mtrfilename.group())) {
						MigrationTool.ui
								.println("Warn: Source File Missing! : "
										+ mtrfilename.group());
					}
				}
			}
			if (mtrsection.group(1).matches("includes.")) {
				mtrfilename = ptnfilename.matcher(mtrsection.group(2));
				while (mtrfilename.find()) {
					mi.infincludes.add(mtrfilename.group());
				}
			}
		}
	}

	private final void preProcessModule() throws Exception {
		// according to .inf file, add extraordinary includes and sourcefiles
		Common.dirCopy(mi.modulepath, mi.temppath); // collect all
													// Laplace.namechange to
													// here???

		if (!mi.infincludes.isEmpty()) {
			Iterator<String> it = mi.infincludes.iterator();
			String tempincludename = null;
			while (it.hasNext()) {
				tempincludename = it.next();
				if (tempincludename.contains("..")) {
					Matcher mtr = Common.PTNSEPARATER.matcher(tempincludename);
					if (mtr.find() && !mtr.group(2).matches(".")) {
						Common.oneLevelDirCopy(mi.modulepath.replaceAll(
								Common.STRSEPARATER, "$1")
								+ File.separator + mtr.group(2), mi.temppath,
								".h");
					} else {
						Common.oneLevelDirCopy(mi.modulepath.replaceAll(
								Common.STRSEPARATER, "$1"), mi.temppath, ".h");
					}
				}
			}
		}
		if (!mi.infsources.isEmpty()) {
			Iterator<String> it = mi.infsources.iterator();
			String tempsourcename = null;
			while (it.hasNext()) {
				tempsourcename = it.next();
				if (tempsourcename.contains("..")) {
					Common.ensureDir(mi.temppath + File.separator
							+ "MT_Parent_Sources");
					Matcher mtr = Common.PTNSEPARATER.matcher(tempsourcename);
					if (mtr.find()) {
						Common.fileCopy(mi.modulepath.replaceAll(
								Common.STRSEPARATER, "$1")
								+ File.separator + mtr.group(2), mi.temppath
								+ File.separator + "MT_Parent_Sources"
								+ File.separator + mtr.group(2));
					}
				}
			}
		}

		Common.toDoAll(mi.temppath, this, Common.FILE);

		parsePreProcessedSourceCode();

	}

	private final void parsePreProcessedSourceCode() throws Exception {
		BufferedReader rd = null;
		String ifile = null;
		String line = null;
		String temp = null;

		Iterator<String> ii = mi.localmodulesources.iterator();
		while (ii.hasNext()) {
			temp = ii.next();
			if (temp.contains(".c") || temp.contains(".dxs")) {
				mi.preprocessedccodes.add(temp);
			}
		}

		ii = mi.preprocessedccodes.iterator();

		Pattern patefifuncc = Pattern.compile(
				"g?(BS|RT)\\s*->\\s*([a-zA-Z_]\\w*)", Pattern.MULTILINE);
		Matcher matguid;
		Matcher matfuncc;
		Matcher matfuncd;
		Matcher matenclosereplace;
		Matcher matefifuncc;
		Matcher matmacro;

		while (ii.hasNext()) {
			StringBuffer wholefile = new StringBuffer();
			ifile = ii.next();
			rd = new BufferedReader(new FileReader(mi.temppath + File.separator
					+ ifile));
			while ((line = rd.readLine()) != null) {
				wholefile.append(line + '\n');
			}
			line = wholefile.toString();

			// find guid
			matguid = Guid.ptnguid.matcher(line); // several ways to implement
													// this , which one is
													// faster ? :
			while (matguid.find()) { // 1.currently , find once , then call
										// to identify which is it
				if ((temp = Guid.register(matguid, mi, MigrationTool.db)) != null) { // 2.use
																						// 3
																						// different
																						// matchers
																						// ,
																						// search
																						// 3
																						// times
																						// to
																						// find
																						// each
					// matguid.appendReplacement(result,
					// MigrationTool.db.getR9Guidname(temp)); // search the
					// database for all 3 kinds of guids , high cost
				}
			}
			// matguid.appendTail(result);
			// line = result.toString();

			// find EFI call in form of '->' , many
			// 'gUnicodeCollationInterface->' like things are not changed
			// This item is not simply replaced , special operation is required.
			matefifuncc = patefifuncc.matcher(line);
			while (matefifuncc.find()) {
				mi.hashEFIcall.add(matefifuncc.group(2));
			}

			// find function call
			matfuncc = Func.ptnfuncc.matcher(line);
			while (matfuncc.find()) {
				if ((temp = Func.register(matfuncc, mi, MigrationTool.db)) != null) {
					// MigrationTool.ui.println(ifile + " dofunc " + temp);
					// matfuncc.appendReplacement(result,
					// MigrationTool.db.getR9Func(temp));
				}
			}
			// matfuncc.appendTail(result);
			// line = result.toString();

			// find macro
			matmacro = Macro.ptntmacro.matcher(line);
			while (matmacro.find()) {
				if ((temp = Macro.register(matmacro, mi, MigrationTool.db)) != null) {
				}
			}

			// find function definition
			// replace all {} to @
			while ((matenclosereplace = Func.ptnbrace.matcher(line)).find()) {
				line = matenclosereplace.replaceAll("@");
			}

			matfuncd = Func.ptnfuncd.matcher(line);
			while (matfuncd.find()) {
				if ((temp = Func.register(matfuncd, mi, MigrationTool.db)) != null) {
				}
			}
		}

		// op on hash
		Iterator<String> funcci = mi.hashfuncc.iterator();
		while (funcci.hasNext()) {
			if (!mi.hashfuncd.contains(temp = funcci.next())
					&& !mi.hashEFIcall.contains(temp)) {
				mi.hashnonlocalfunc.add(temp); // this set contains both
												// changed and not changed items
			}
		}
	}

	public class CommentLaplace extends Common.Laplace {
		public String operation(String wholeline) {
			StringBuffer wholebuffer = new StringBuffer();
			String templine = null;
			Pattern ptnincludefile = Pattern.compile("[\"<](.*[.]h)[\">]");
			Pattern ptninclude = Pattern.compile("#include\\s*(.*)");
			Matcher mtrinclude = ptninclude.matcher(wholeline);
			Matcher mtrincludefile = null;
			while (mtrinclude.find()) {
				mtrincludefile = ptnincludefile.matcher(mtrinclude.group(1));
				if (mtrincludefile.find()
						&& mi.localmodulesources.contains(mtrincludefile
								.group(1))) {
					templine = mtrinclude.group();
				} else {
                    String line = mtrinclude.group().toLowerCase();
                    if (line.contains("pal.h")) {
                        templine = "#include <IndustryStandard/Pal.h>\n";
                    } else if (line.contains("sal.h")) {
                        templine = "#include <IndustryStandard/Sal.h>\n";
                    } else if (line.contains("pci22.h")) {
                        templine = "#include <IndustryStandard/Pci22.h>\n";
                    } else if (line.contains("pci23.h")) {
                        templine = "#include <IndustryStandard/Pci23.h>\n";
                    } else if (line.contains("pci30.h")) {
                        templine = "#include <IndustryStandard/Pci30.h>\n";
                    } else if (line.contains("pci.h")) {
                        templine = "#include <IndustryStandard/Pci.h>\n";
                    } else if (line.contains("acpi.h")) {
                        templine = "#include <IndustryStandard/Acpi.h>\n";
                    } else if (line.contains("scsi.h")) {
                        templine = "#include <IndustryStandard/Scsi.h>\n";
                    } else if (line.contains("usb.h")) {
                        templine = "#include <IndustryStandard/Usb.h>\n";
                    } else {
                        templine = MigrationTool.MIGRATIONCOMMENT
                                + mtrinclude.group();
                    }
                }
                mtrinclude.appendReplacement(wholebuffer, templine);
            }
			mtrinclude.appendTail(wholebuffer);
			return wholebuffer.toString();
		}

		public boolean recognize(String filename) {
			return filename.contains(".c") || filename.contains(".h")
					|| filename.contains(".dxs");
		}

		public String namechange(String oldname) {
			return oldname;
		}
	}

	// -----------------------------------ForDoAll-----------------------------------//
	public void run(String filepath) throws Exception {
		String name = mi.temppath + File.separator
				+ filepath.replace(mi.temppath + File.separator, "");
		if (commentlaplace.recognize(name)) {
			commentlaplace.transform(name, name);
		}
	}

	public boolean filter(File dir) {
		return true;
	}

	// -----------------------------------ForDoAll-----------------------------------//

	public final void setModuleInfo(ModuleInfo m) {
		mi = m;
	}

	public static final void aimAt(ModuleInfo mi) throws Exception {
		modulereader.setModuleInfo(mi);
		modulereader.ModuleScan();
	}
}
