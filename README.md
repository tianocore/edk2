# VgaShim
This little project was started when a client brought a MacBookAir7,2 (13" Early 2015) and requested Windows 7 to be installed. Turned out this is not possible by default due to W7's insistence on using old VGA mode graphics configured using the 10h interrupt. 

This program attempts to install a shim where VGA ROM would normally reside. It fakes int10h support and configures a Windows 7-compatible video mode that can directly use the framebuffer before a robust driver is loaded. 

For more information on how to use please see this MacRumors thread.