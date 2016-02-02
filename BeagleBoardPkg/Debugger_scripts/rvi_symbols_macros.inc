//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//

define /R int compare_guid(guid1, guid2)
    unsigned char *guid1;
    unsigned char *guid2;
{
    return strncmp(guid1, guid2, 16);
}
.

define /R unsigned char * find_system_table(mem_start, mem_size)
    unsigned char *mem_start;
    unsigned long mem_size;
{
    unsigned char *mem_ptr;

    mem_ptr = mem_start + mem_size;

    do
    {
        mem_ptr -= 0x400000; // 4 MB

        if (strncmp(mem_ptr, "IBI SYST", 8) == 0)
        {
            return *(unsigned long *)(mem_ptr + 8); // EfiSystemTableBase
        }

    } while (mem_ptr > mem_start);

    return 0;
}
.

define /R unsigned char * find_debug_info_table_header(system_table)
    unsigned char *system_table;
{
    unsigned long configuration_table_entries;
    unsigned char *configuration_table;
    unsigned long index;
    unsigned char debug_table_guid[16];

    // Fill in the debug table's guid
    debug_table_guid[ 0] = 0x77;
    debug_table_guid[ 1] = 0x2E;
    debug_table_guid[ 2] = 0x15;
    debug_table_guid[ 3] = 0x49;
    debug_table_guid[ 4] = 0xDA;
    debug_table_guid[ 5] = 0x1A;
    debug_table_guid[ 6] = 0x64;
    debug_table_guid[ 7] = 0x47;
    debug_table_guid[ 8] = 0xB7;
    debug_table_guid[ 9] = 0xA2;
    debug_table_guid[10] = 0x7A;
    debug_table_guid[11] = 0xFE;
    debug_table_guid[12] = 0xFE;
    debug_table_guid[13] = 0xD9;
    debug_table_guid[14] = 0x5E;
    debug_table_guid[15] = 0x8B;

    configuration_table_entries = *(unsigned long *)(system_table + 64);
    configuration_table         = *(unsigned long *)(system_table + 68);

    for (index = 0; index < configuration_table_entries; index++)
    {
        if (compare_guid(configuration_table, debug_table_guid) == 0)
        {
            return *(unsigned long *)(configuration_table + 16);
        }

        configuration_table += 20;
    }

    return 0;
}
.

define /R int valid_pe_header(header)
        unsigned char *header;
{
    if ((header[0x00] == 'M') &&
        (header[0x01] == 'Z') &&
        (header[0x80] == 'P') &&
        (header[0x81] == 'E'))
    {
        return 1;
    }

    return 0;
}
.

define /R unsigned long pe_headersize(header)
        unsigned char *header;
{
    unsigned long *size;

    size = header + 0x00AC;

    return *size;
}
.

define /R unsigned char *pe_filename(header)
        unsigned char *header;
{
    unsigned long *debugOffset;
    unsigned char *stringOffset;

    if (valid_pe_header(header))
    {
        debugOffset  = header + 0x0128;
        stringOffset = header + *debugOffset + 0x002C;

        return stringOffset;
    }

    return 0;
}
.

define /R int char_is_valid(c)
        unsigned char c;
{
    if (c >= 32 && c < 127)
    	return 1;

    return 0;
}
.

define /R write_symbols_file(filename, mem_start, mem_size)
    unsigned char *filename;
    unsigned char *mem_start;
    unsigned long mem_size;
{
    unsigned char *system_table;
    unsigned char *debug_info_table_header;
    unsigned char *debug_info_table;
    unsigned long debug_info_table_size;
    unsigned long index;
    unsigned char *debug_image_info;
    unsigned char *loaded_image_protocol;
    unsigned char *image_base;
    unsigned char *debug_filename;
    unsigned long header_size;
    int           status;

    system_table = find_system_table(mem_start, mem_size);
    if (system_table == 0)
    {
        return;
    }

    status = fopen(88, filename, "w");

    debug_info_table_header = find_debug_info_table_header(system_table);

    debug_info_table      = *(unsigned long *)(debug_info_table_header + 8);
    debug_info_table_size = *(unsigned long *)(debug_info_table_header + 4);

    for (index = 0; index < (debug_info_table_size * 4); index += 4)
    {
        debug_image_info = *(unsigned long *)(debug_info_table + index);

        if (debug_image_info == 0)
        {
            break;
        }

        loaded_image_protocol = *(unsigned long *)(debug_image_info + 4);

        image_base = *(unsigned long *)(loaded_image_protocol + 32);

        debug_filename = pe_filename(image_base);
        header_size    = pe_headersize(image_base);

        $fprintf 88, "%s 0x%08x\n", debug_filename, image_base + header_size$;
    }


    fclose(88);
}
.

