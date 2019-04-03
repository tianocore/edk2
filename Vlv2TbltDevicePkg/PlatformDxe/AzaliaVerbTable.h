/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

--*/

UINT32 mAzaliaVerbTableData12[] = {
  //
  // Audio Verb Table - 0x80862805
  //
  // Pin Widget 5 - PORT B
  0x20471C10,
  0x20471D00,
  0x20471E56,
  0x20471F18,

  // Pin Widget 6 - PORT C
  0x20571C20,
  0x20571D00,
  0x20571E56,
  0x20571F18,

  // Pin Widget 7 - PORT D
  0x20671C30,
  0x20671D00,
  0x20671E56,
  0x20671F58
};


PCH_AZALIA_VERB_TABLE mAzaliaVerbTable[] = {
  {
    //
    // VerbTable:
    //  Revision ID = 0xFF, support all steps
    //  Codec Verb Table For AZALIA
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor:  0x10EC0880
    //
    {
      0x10EC0880,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0xFF,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000A,         // Number of Rear Jacks = 10
      0x0002          // Number of Front Jacks = 2
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    // Revision ID >= 0x03
    // Codec Verb Table For AZALIA
    // Codec Address: CAd value (0/1/2)
    // Codec Vendor: 0x434D4980
    //
    {
      0x434D4980,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0x00,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x0009,         // Number of Rear Jacks = 9
      0x0002          // Number of Front Jacks = 2
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    // Lawndale Azalia Audio Codec Verb Table
    // Revision ID = 0x00
    // Codec Address: CAd value (0/1/2)
    // Codec Vendor: 0x11D41984
    //
    {
      0x11D41984,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0x04,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x0009,         // Number of Rear Jacks = 9
      0x0002          // Number of Front Jacks = 2
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    // VerbTable:
    //  Revision ID = 0xFF, support all steps
    //  Codec Verb Table For AZALIA
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor: 0x11D41986
    //
    {
      0x11D41986,     // Vendor ID/Device ID
      0x0001,         // SubSystem ID
      0xFF,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000A,         // Number of Rear Jacks = 8
      0x0002          // Number of Front Jacks = 2
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    // VerbTable: (for Slim River, FFDS3)
    //  Revision ID = 0x00
    //  Codec Verb Table For AZALIA
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor: 0x10EC0272
    //
    {
      0x10EC0272,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0x00,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000E,         // Number of Rear Jacks
      0x0002          // Number of Front Jacks
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    //  VerbTable: (for Buffalo Trail)
    //  Revision ID = 0x00
    //  Codec Verb Table For AZALIA
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor: 0x10EC0269
    //
    {
      0x10EC0269,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0x00,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000A,         // Number of Rear Jacks
      0x0002          // Number of Front Jacks
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    //  VerbTable: (RealTek ALC888)
    //  Revision ID = 0xFF
    //  Codec Verb Table For Redfort
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor: 0x10EC0888
    //
    {
      0x10EC0888,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0xFF,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000B,         // Number of Rear Jacks
      0x0002          // Number of Front Jacks
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    //  VerbTable: (RealTek ALC885)
    //  Revision ID = 0xFF
    //  Codec Verb Table For Redfort
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor: 0x10EC0885
    //
    {
      0x10EC0885,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0xFF,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000B,         // Number of Rear Jacks
      0x0002          // Number of Front Jacks
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    //  VerbTable: (IDT 92HD81)
    //  Revision ID = 0xFF
    //  Codec Vendor: 0x111D7605
    //
    {
      0x111D76d5,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0xFF,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x0008,         // Number of Rear Jacks
      0x0002          // Number of Front Jacks
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    //  VerbTable: (Intel VLV HDMI)
    //  Revision ID = 0xFF
    //  Codec Verb Table For EmeraldLake/LosLunas
    //  Codec Vendor: 0x80862804
    //
    {
      0x80862882,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0xFF,           // Revision ID
      0x02,           // Front panel support (1=yes, 2=no)
      0x0003,         // Number of Rear Jacks
      0x0000          // Number of Front Jacks
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    // VerbTable: (RealTek ALC262)
    //  Revision ID = 0xFF, support all steps
    //  Codec Verb Table For AZALIA
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor:  0x10EC0262
    //
    {
      0x10EC0262,     // Vendor ID/Device ID
      0x0000,         // SubSystem ID
      0xFF,           // Revision ID
      0x01,           // Front panel support (1=yes, 2=no)
      0x000B,         // Number of Rear Jacks = 11
      0x0002          // Number of Front Jacks = 2
    },
    0                 // Pointer to verb table data, need to be inited in the code.
  },
  {
    //
    //  VerbTable: (RealTek ALC282)
    //  Revision ID = 0xff
    //  Codec Verb Table For Azalia on SharkBay-WhiteBluff refresh and Haswell ULT FFRD Harris Beach, WTM1, WTM2iCRB
    //  Codec Address: CAd value (0/1/2)
    //  Codec Vendor: 0x10EC0282
    //
    {
      0x10EC0282, // Vendor ID/Device ID
      0x0000,     // SubSystem ID
      0xff,       // Revision ID
      0x01,       // Front panel support (1=yes, 2=no)
      0x000C,     // Number of Rear Jacks, 0x0010 for Harris Beach, 0x000B for WTM1 & WTM2iCRB
      0x0002      // Number of Front Jacks
    },
    0             // Pointer to verb table data, need to be inited in the code.
  }
};
