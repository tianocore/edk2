
=== FTDI USB SERIAL OVERVIEW ===

This is a bus driver that enables the EfiSerialIoProtocol interface
for FTDI8U232AM based USB-to-Serial adapters.

=== STATUS ===

Serial Input: Functional on real hardware.
Serial Output: Functional on real hardware.

Operating Modes: Currently the user is able to change all operating modes
except timeout and FIFO depth.
The default operating mode is:
	Baudrate:     115200
	Parity:       None
	Flow Control: None
	Data Bits:    8
	Stop Bits:    1
Notes: 
	Data Bits setting of 6,7,8 can not be combined with a Stop Bits setting of 1.5

        At baudrates less than 9600 some of the characters may be transmitted incorrectly.

=== COMPATIBILITY ===

Tested with:
An FTDI8U232AM based USB-To-Serial adapter, the UEFI Shell, and the SerialTest application 
using a PuTTY Terminal

See CompatibleDevices.txt for a list of devices which have been confirmed to work with this 
driver.