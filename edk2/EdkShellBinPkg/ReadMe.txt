The binaries of EdkShellBinPkg are directly retrieved from EDK 1.03 release with the following steps:
1.Download Edk1.03.zip from EDK official release site: ://edk.tianocore.org/servlets/ProjectDocumentList?folderID=6&expandFolder=6&folderID=0
2.Unzip it to a local folder, e.g. c:\Edk.
3.The EDK II prime binaries are mapped as follows:
EDKII prime                                        Edk\
EdkShellBinPkg\FullShell\ia32\Shell_Full.efi       Other\Maintained\Application\UefiShell\bin\ia32\Shell_Full.efi	
EdkShellBinPkg\FullShell\x64\Shell_Full.efi        Other\Maintained\Application\UefiShell\bin\x64\Shell_Full.efi	
EdkShellBinPkg\FullShell\ipf\Shell_Full.efi        Other\Maintained\Application\UefiShell\bin\ipf\Shell_Full.efi	
EdkShellBinPkg\MinimumShell\ia32\Shell.efi         Other\Maintained\Application\UefiShell\bin\ipf\Shell.efi	
EdkShellBinPkg\MinimumShell\x64\Shell.efi          Other\Maintained\Application\UefiShell\bin\x64\Shell.efi
EdkShellBinPkg\MinimumShell\ipf\Shell.efi          Other\Maintained\Application\UefiShell\bin\ipf\Shell.efi
EdkShellBinPkg\bin\ia32\Shell_Full.efi             Other\Maintained\Application\UefiShell\bin\ia32\Shell_Full.efi	
EdkShellBinPkg\bin\x64\Shell_Full.efi              Other\Maintained\Application\UefiShell\bin\x64\Shell_Full.efi	
EdkShellBinPkg\bin\ipf\Shell_Full.efi              Other\Maintained\Application\UefiShell\bin\ipf\Shell_Full.efi	
EdkShellBinPkg\bin\ia32\Shell.efi                  Other\Maintained\Application\UefiShell\bin\ipf\Shell.efi	
EdkShellBinPkg\bin\x64\Shell.efi                   Other\Maintained\Application\UefiShell\bin\x64\Shell.efi
EdkShellBinPkg\bin\ipf\Shell.efi                   Other\Maintained\Application\UefiShell\bin\ipf\Shell.efi
EdkShellBinPkg\bin\ia32\Apps\*                     Other\Maintained\Application\UefiShell\bin\ia32\Apps\*
EdkShellBinPkg\bin\x64\Apps\*                      Other\Maintained\Application\UefiShell\bin\x64\Apps\*
EdkShellBinPkg\bin\ipf\Apps\*                      Other\Maintained\Application\UefiShell\bin\ipf\Apps\*
