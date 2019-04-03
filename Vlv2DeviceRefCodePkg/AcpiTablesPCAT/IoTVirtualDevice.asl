/** @file
Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

Device(IOTD) { 
  Name(_HID, "MSFT8000")
  Name(_CID, "MSFT8000")
  
  Name(_CRS, ResourceTemplate() {  
    // Index 0 
    SPISerialBus(            // Pin 5, 7, 9 , 11 of JP1 for SIO_SPI
      1,                     // Device selection
      PolarityLow,           // Device selection polarity
      FourWireMode,          // wiremode
      8,                     // databit len
      ControllerInitiated,   // slave mode
      8000000,               // Connection speed
      ClockPolarityLow,      // Clock polarity
      ClockPhaseSecond,      // clock phase
      "\\_SB.SPI1",          // ResourceSource: SPI bus controller name
      0,                     // ResourceSourceIndex
      ResourceConsumer,      // Resource usage
      JSPI,                  // DescriptorName: creates name for offset of resource descriptor
      )                      // Vendor Data  
    
    // Index 1     
    I2CSerialBus(            // Pin 13, 15 of JP1, for SIO_I2C5 (signal)
      0x00,                  // SlaveAddress: bus address (TBD)
      ,                      // SlaveMode: default to ControllerInitiated
      400000,                // ConnectionSpeed: in Hz
      ,                      // Addressing Mode: default to 7 bit
      "\\_SB.I2C6",          // ResourceSource: I2C bus controller name (For MinnowBoard Max, hardware I2C5(0-based) is reported as ACPI I2C6(1-based))
      ,
      ,
      JI2C,                  // Descriptor Name: creates name for offset of resource descriptor
      )                      // VendorData
    
    // Index 2
    UARTSerialBus(           // Pin 17, 19 of JP1, for SIO_UART2
      115200,                // InitialBaudRate: in bits ber second
      ,                      // BitsPerByte: default to 8 bits
      ,                      // StopBits: Defaults to one bit
      0xfc,                  // LinesInUse: 8 1-bit flags to declare line enabled
      ,                      // IsBigEndian: default to LittleEndian
      ,                      // Parity: Defaults to no parity
      ,                      // FlowControl: Defaults to no flow control
      32,                    // ReceiveBufferSize
      32,                    // TransmitBufferSize
      "\\_SB.URT2",          // ResourceSource: UART bus controller name
      ,
      ,
      UAR2,                  // DescriptorName: creates name for offset of resource descriptor
      )                      
    
    // Index 3
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO2",) {0}  // Pin 21 of JP1 (GPIO_S5[00])
    // Index 4
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO2",) {0} 
    
    // Index 5
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO2",) {1}  // Pin 23 of JP1 (GPIO_S5[01])
    // Index 6
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO2",) {1}
    
    // Index 7
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO2",) {2}  // Pin 25 of JP1 (GPIO_S5[02])
    // Index 8
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO2",) {2} 
    
    // Index 9
    UARTSerialBus(           // Pin 6, 8, 10, 12 of JP1, for SIO_UART1
      115200,                // InitialBaudRate: in bits ber second
      ,                      // BitsPerByte: default to 8 bits
      ,                      // StopBits: Defaults to one bit
      0xfc,                  // LinesInUse: 8 1-bit flags to declare line enabled
      ,                      // IsBigEndian: default to LittleEndian
      ,                      // Parity: Defaults to no parity
      FlowControlHardware,   // FlowControl: Defaults to no flow control
      32,                    // ReceiveBufferSize
      32,                    // TransmitBufferSize
      "\\_SB.URT1",          // ResourceSource: UART bus controller name
      ,
      ,
      UAR1,              // DescriptorName: creates name for offset of resource descriptor
      )  
    
    // Index 10
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {62}  // Pin 14 of JP1 (GPIO_SC[62])
    // Index 11
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {62} 
    
    // Index 12
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {63}  // Pin 16 of JP1 (GPIO_SC[63])
    // Index 13
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {63} 
    
    // Index 14
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {65}  // Pin 18 of JP1 (GPIO_SC[65])
    // Index 15
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {65} 
    
    // Index 16
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {64}  // Pin 20 of JP1 (GPIO_SC[64])
    // Index 17
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {64} 
    
    // Index 18
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {94}  // Pin 22 of JP1 (GPIO_SC[94])
    // Index 19
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {94} 
    
    // Index 20
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {95}  // Pin 24 of JP1 (GPIO_SC[95])
    // Index 21
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {95} 
    
    // Index 22
    GpioIo (Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPO0",) {54}  // Pin 26 of JP1 (GPIO_SC[54])
    // Index 23
    GpioInt(Edge, ActiveBoth, SharedAndWake, PullNone, 0,"\\_SB.GPO0",) {54}
  })

  Name(_DSD, Package() {
    ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
    Package(1) {	  // Just one Property for IOT (at this time) 
      Package(2) {	//The “symbolic-identifiers” property
        "symbolic-identifiers", 
        Package() {	//Contains all the <resource index, symbolic-identifier> pairs       
          0, "SPI0",	  
          1, "I2C5",		
          2, "UART2",
          3, 21,       // Pin 21 of JP1 (GPIO_S5[00])
          4, 21,       // Pin 21 for separate resource. 
          5, 23,       // Pin 23 of JP1 (GPIO_S5[01])
          6, 23,
          7, 25,       // Pin 25 of JP1 (GPIO_S5[02])
          8, 25,
          9, "UART1",
          10, 14,      // Pin 14 of JP1 (GPIO_SC[62])
          11, 14,
          12, 16,      // Pin 16 of JP1 (GPIO_SC[63])
          13, 16,
          14, 18,      // Pin 18 of JP1 (GPIO_SC[65])
          15, 18,
          16, 20,      // Pin 20 of JP1 (GPIO_SC[64])
          17, 20,
          18, 22,      // Pin 22 of JP1 (GPIO_SC[94])
          19, 22,
          20, 24,      // Pin 24 of JP1 (GPIO_SC[95])
          21, 24,
          22, 26,      // Pin 26 of JP1 (GPIO_SC[54])
          23, 26
        }
      } 
    }
  })
  
  Method(_STA,0,Serialized) {
    
    //
    // Only report IoT virtual device when all pins' configuration follows MSFT's datasheet.
    //
    If (LEqual(IOT, 1)) {
      Return (0xF)
    }
    
    Return (0x0)
  }
}
