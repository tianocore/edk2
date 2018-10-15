Prerequisite Tools:
1. Install Python 2.7.3 from https://www.python.org/download/releases/2.7.3/
2. Install wxPython 2.8.12.1 from https://sourceforge.net/projects/wxpython/files/wxPython/2.8.12.1/
   generally the libraries will be installed at python's subfolder, for example in windows: c:\python27\Lib\site-packages\
3. Install DoxyGen 1.8.6 from https://sourceforge.net/projects/doxygen/files/rel-1.8.6/
4. (Windows only) Install Htmlhelp tool from https://msdn.microsoft.com/en-us/library/windows/desktop/ms669985(v=vs.85).aspx

Limitation:
1. Current tool doesn't work on latest wxPython and DoxyGen tool. Please use the sepecific version in above.

Run the Tool:
a) Run with GUI:
  1. Enter src folder, double click "packagedocapp.pyw" or run command "python packagedocapp.pyw" to open the GUI.
  2. Make sure all the information in blank are correct.
  3. Click "Generate Package Document!"
b) Run with command line:
  1. Open command line window
  2. Enter src folder, for example: "cd C:\PackageDocumentTools\src"
  3. Run "python packagedoc_cli.py --help" for detail command.
