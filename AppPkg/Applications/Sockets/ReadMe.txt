Users of the socket library need to do the following:

•	Use the development branch: https://edk2.svn.sourceforge.net/svnroot/edk2/branches/EADK/
•	Create an Efi\StdLib\etc directory on their system or USB flash device
•	Copy the files from StdLib\Efi\StdLib\etc into that directory
•	Edit the files appropriately
	o	Set the correct DNS servers in resolv.conf
	o	Set the search order in host.conf
•	At the EFI shell
	o	Set the device containing the \Efi directory as the default device
	o	Run the socket application
