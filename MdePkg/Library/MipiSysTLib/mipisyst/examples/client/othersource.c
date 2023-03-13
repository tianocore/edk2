/*
 Copyright (c) 2018, MIPI Alliance, Inc. 
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.

 * Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "mipi_syst.h"

extern struct mipi_syst_handle * systh;

void banner()
{
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("+-------------------------------------------------------+", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|               ____         _____   _______            |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|              / ___|       / ____| |__   __|           |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|             | |___  __  _| |___ _____| |              |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|              \\___ \\| | | |\\___ \\_____| |              |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|              ____| | |_| |____| |    | |              |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|             |_____/ \\__| |_____/     |_|              |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|                      _/ /                             |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|                     |__/                              |", 0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("+-------------------------------------------------------+", 1));
}

/*
* Use dynamic width settings with %*s formats to draw a sinus pattern
*/
static int sin_wave[30] = {
	0, 5,11,16,20,24,26,27,27,26,24,20,16,11, 5, 0,-5,-11,-16,-20,-24,-26,-27,-27,-26,-24,-20,-16,-11,-5
};

void  sinewave()
{
	int i, ii, y;

	/* Show printf formatting capabilties of catalog and printf messages
	*/
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_DEBUG, "|-------------------------------------------------------|");

	for (i = 0; i < 1; ++i) {
		for (ii = 0; ii < 30; ++ii) {
			y = sin_wave[ii];
			MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_DEBUG,
				MIPI_SYST_HASH("| '%%*so%%*s           | '%*so%*s ' |", 0),
				MIPI_SYST_PARAM_INT(14 - y / 2),
				MIPI_SYST_PARAM_CSTR(""),
				MIPI_SYST_PARAM_INT(14 + y / 2),
				MIPI_SYST_PARAM_CSTR(""));
		}
	}
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_DEBUG, "|-------------------------------------------------------|");
}

/* each client can use the 6-bit subtype value for tagging the write content */
#define WRITE_DATA_TAG 0x1

/* raw data outputting
 */
void raw()
{
	unsigned char data[10];
	int i;

	for (i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
		data[i] = i;
	}
	MIPI_SYST_WRITE(systh, MIPI_SYST_SEVERITY_MAX, WRITE_DATA_TAG, data, sizeof(data));
}

void short_messages()
{
	/* simple values
	 */
	MIPI_SYST_SHORT32(systh, 0x1234567);
	MIPI_SYST_SHORT64(systh, 0x112233445566778);

	/* Example using a mask to classify values
	 * see collateral xml where mask is defined
	 */
#define E_MASK 0x0E000000    /* 0xE in bits 24..28 means error, the lower bits error number */
#define E_ONE  E_MASK|1
#define E_TWO  E_MASK|2

	MIPI_SYST_SHORT32(systh, E_ONE);
	MIPI_SYST_SHORT32(systh, E_TWO);

#define W_MASK 0x0F000000   /* 0xF in bits 24..28 means warning, the lower bits warning number */
#define W_ONE  W_MASK|1
#define W_TWO  W_MASK|2

	MIPI_SYST_SHORT32(systh, W_ONE);
	MIPI_SYST_SHORT32(systh, W_TWO);
}

void foo()
{
	MIPI_SYST_CATALOG32_0(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("in function foo",0));
}

void bar()
{
	MIPI_SYST_CATALOG32_0(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("in function bar",0));
}

void printf_format_test()
{
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|-------------------------------------------------------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|    printf Format   |         printf Result            |");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|---------------------------------strings---------------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30s'             | '%30s' |", "right-justified");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30s'            | '%-30s' |", "left-justified");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|---------------------------------ASCII-----------------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%s'               | '%s'  |", "!\"#$%&'()*+-./0123456789:;<=>");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%s'               | '%s'  |", "?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%s'               | '%s'  |", "\\]^_`qbcdefghijklmnopqrstuvwx");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%s'               | '%s'  |", "yz{|}~                       ");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|---------------------------------decimals--------------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30d'             | '%30d' |", 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30d'             | '%30d' |", 10);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30d'             | '%30d' |", 100);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30d'             | '%30d' |", 1000);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30d'             | '%30d' |", 10000);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30d'             | '%30d' |", 100000);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30d'            | '%-30d' |", 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30d'            | '%-30d' |", 10);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30d'            | '%-30d' |", 100);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30d'            | '%-30d' |", 1000);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30d'            | '%-30d' |", 10000);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30d'            | '%-30d' |", 100000);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|----------------------------------hexadecimals---------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30x'             | '%30x' |", 0xa);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30x'             | '%30x' |", 0xab);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30x'             | '%30x' |", 0xabc);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30x'             | '%30x' |", 0xabcd);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30x'             | '%30x' |", 0xabcde);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30x'             | '%30x' |", 0xabcdef);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30x'            | '%-30x' |", 0xa);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30x'            | '%-30x' |", 0xab);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30x'            | '%-30x' |", 0xabc);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30x'            | '%-30x' |", 0xabcd);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30x'            | '%-30x' |", 0xabcde);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30x'            | '%-30x' |", 0xabcdef);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30X'             | '%30X' |", 0xa);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30X'             | '%30X' |", 0xab);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30X'             | '%30X' |", 0xabc);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30X'             | '%30X' |", 0xabcd);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30X'             | '%30X' |", 0xabcde);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30X'             | '%30X' |", 0xabcdef);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30X'            | '%-30X' |", 0xa);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30X'            | '%-30X' |", 0xab);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30X'            | '%-30X' |", 0xabc);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30X'            | '%-30X' |", 0xabcd);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30X'            | '%-30X' |", 0xabcde);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30X'            | '%-30X' |", 0xabcdef);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|-----------------------------------double--------------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.1f'           | '%30.1f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.2f'           | '%30.2f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.3f'           | '%30.3f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.4f'           | '%30.4f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.5f'           | '%30.5f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.6f'           | '%30.6f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.7f'           | '%30.7f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.8f'           | '%30.8f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.9f'           | '%30.9f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.10f'          | '%30.10f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.1f'          | '%-30.1f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.2f'          | '%-30.2f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.3f'          | '%-30.3f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.4f'          | '%-30.4f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.5f'          | '%-30.5f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.6f'          | '%-30.6f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.7f'          | '%-30.7f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.8f'          | '%-30.8f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.9f'          | '%-30.9f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.10f'         | '%-30.10f' |", 3.14159265359);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|-----------------------------------scientific----------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.1e'           | '%30.1e' |", 1.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.2e'           | '%30.2e' |", 10.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.3e'           | '%30.3e' |", 10.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.4e'           | '%30.4e' |", 100.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.5e'           | '%30.5e' |", 1000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.6e'           | '%30.6e' |", 10000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.7e'           | '%30.7e' |", 10000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.8e'           | '%30.8e' |", 10000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.9e'           | '%30.9e' |", 100000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%30.10e'          | '%30.10e' |", 1000000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.1e'          | '%-30.1e' |", 1.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.2e'          | '%-30.2e' |", 10.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.3e'          | '%-30.3e' |", 10.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.4e'          | '%-30.4e' |", 100.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.5e'          | '%-30.5e' |", 1000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.6e'          | '%-30.6e' |", 10000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.7e'          | '%-30.7e' |", 10000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.8e'          | '%-30.8e' |", 10000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.9e'          | '%-30.9e' |", 100000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30.10e'         | '%-30.10e' |", 1000000.5);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "|-----------------------------------exotics-------------|");
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030hhu'          | '%030hhu' |", (unsigned char)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030hu'           | '%030hu' |", (unsigned int)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030lu'           | '%030lu' |", (unsigned long int) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030llu'          | '%030llu' |", (unsigned long long) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030hho'          | '%030hho' |", (unsigned char)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030ho'           | '%030ho' |", (unsigned int)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030lo'           | '%030lo' |", (unsigned long int) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030llo'          | '%030llo' |", (unsigned long long) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030hhx'          | '%030hhx' |", (unsigned char)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030hx'           | '%030hx' |", (unsigned int)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030lx'           | '%030lx' |", (unsigned long int) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%030llx'          | '%030llx' |", (unsigned long long) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30hhu'          | '%-30hhu' |", (unsigned char)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%-30hu'           | '%-30hu' |", (unsigned int)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30lu'          | '%#-30lu' |", (unsigned long int) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30llu'         | '%#-30llu' |", (unsigned long long) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30hho'         | '%#-30hho' |", (unsigned char)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30ho'          | '%#-30ho' |", (unsigned int)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30lo'          | '%#-30lo' |", (unsigned long int) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30llo'         | '%#-30llo' |", (unsigned long long) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30hhx'         | '%#-30hhx' |", (unsigned char)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30hx'          | '%#-30hx' |", (unsigned int)-1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30lx'          | '%#-30lx' |", (unsigned long int) - 1);
	MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO, "| '%%#-30llx'         | '%#-30llx' |", (unsigned long long) - 1);
}

/* same as printf, but now using catalog messaging*/
void catalog_format_test()
{
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|    catalog Format  |         Printed Result           |",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|---------------------------------strings---------------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30s'             | '%30s' |", 0), MIPI_SYST_PARAM_CSTR("right-justified"));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30s'            | '%-30s' |", 0), MIPI_SYST_PARAM_CSTR("left-justified"));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|---------------------------------ASCII-----------------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%s'               | '%s'  |",0), MIPI_SYST_PARAM_CSTR("!\"#$%&'()*+-./0123456789:;<=>"));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%s'               | '%s'  |",1), MIPI_SYST_PARAM_CSTR("?@ABCDEFGHIJKLMNOPQRSTUVWXYZ["));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%s'               | '%s'  |",2), MIPI_SYST_PARAM_CSTR("\\]^_`qbcdefghijklmnopqrstuvwx"));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%s'               | '%s'  |",3), MIPI_SYST_PARAM_CSTR("yz{|}~                       "));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|---------------------------------decimals--------------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30d'             | '%30d' |",0), MIPI_SYST_PARAM_INT(1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30d'             | '%30d' |",1), MIPI_SYST_PARAM_INT(10));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30d'             | '%30d' |",2), MIPI_SYST_PARAM_INT(100));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30d'             | '%30d' |",3), MIPI_SYST_PARAM_INT(1000));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30d'             | '%30d' |",4), MIPI_SYST_PARAM_INT(10000));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30d'             | '%30d' |",5), MIPI_SYST_PARAM_INT(100000));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30d'            | '%-30d' |",0), MIPI_SYST_PARAM_INT(1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30d'            | '%-30d' |",1), MIPI_SYST_PARAM_INT(10));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30d'            | '%-30d' |",2), MIPI_SYST_PARAM_INT(100));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30d'            | '%-30d' |",3), MIPI_SYST_PARAM_INT(1000));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30d'            | '%-30d' |",4), MIPI_SYST_PARAM_INT(10000));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30d'            | '%-30d' |",5), MIPI_SYST_PARAM_INT(100000));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|----------------------------------hexadecimals---------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30x'             | '%30x' |",0), MIPI_SYST_PARAM_INT(0xa));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30x'             | '%30x' |",1), MIPI_SYST_PARAM_INT(0xab));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30x'             | '%30x' |",2), MIPI_SYST_PARAM_INT(0xabc));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30x'             | '%30x' |",3), MIPI_SYST_PARAM_INT(0xabcd));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30x'             | '%30x' |",4), MIPI_SYST_PARAM_INT(0xabcde));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30x'             | '%30x' |",5), MIPI_SYST_PARAM_INT(0xabcdef));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30x'            | '%-30x' |",0), MIPI_SYST_PARAM_INT(0xa));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30x'            | '%-30x' |",1), MIPI_SYST_PARAM_INT(0xab));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30x'            | '%-30x' |",2), MIPI_SYST_PARAM_INT(0xabc));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30x'            | '%-30x' |",3), MIPI_SYST_PARAM_INT(0xabcd));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30x'            | '%-30x' |",4), MIPI_SYST_PARAM_INT(0xabcde));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30x'            | '%-30x' |",5), MIPI_SYST_PARAM_INT(0xabcdef));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30X'             | '%30X' |",0), MIPI_SYST_PARAM_INT(0xa));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30X'             | '%30X' |",1), MIPI_SYST_PARAM_INT(0xab));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30X'             | '%30X' |",2), MIPI_SYST_PARAM_INT(0xabc));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30X'             | '%30X' |",3), MIPI_SYST_PARAM_INT(0xabcd));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30X'             | '%30X' |",4), MIPI_SYST_PARAM_INT(0xabcde));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30X'             | '%30X' |",5), MIPI_SYST_PARAM_INT(0xabcdef));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30X'            | '%-30X' |",0), MIPI_SYST_PARAM_INT(0xa));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30X'            | '%-30X' |",1), MIPI_SYST_PARAM_INT(0xab));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30X'            | '%-30X' |",2), MIPI_SYST_PARAM_INT(0xabc));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30X'            | '%-30X' |",3), MIPI_SYST_PARAM_INT(0xabcd));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30X'            | '%-30X' |",4), MIPI_SYST_PARAM_INT(0xabcde));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30X'            | '%-30X' |",5), MIPI_SYST_PARAM_INT(0xabcdef));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|-----------------------------------double--------------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.1f'           | '%30.1f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.2f'           | '%30.2f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.3f'           | '%30.3f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.4f'           | '%30.4f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.5f'           | '%30.5f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.6f'           | '%30.6f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.7f'           | '%30.7f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.8f'           | '%30.8f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.9f'           | '%30.9f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.10f'          | '%30.10f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.1f'          | '%-30.1f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.2f'          | '%-30.2f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.3f'          | '%-30.3f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.4f'          | '%-30.4f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.5f'          | '%-30.5f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.6f'          | '%-30.6f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.7f'          | '%-30.7f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.8f'          | '%-30.8f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.9f'          | '%-30.9f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.10f'         | '%-30.10f' |",0), MIPI_SYST_PARAM_FLOAT(3.14159265359));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|-----------------------------------scientific----------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.1e'           | '%30.1e' |",0), MIPI_SYST_PARAM_DOUBLE(1.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.2e'           | '%30.2e' |",0), MIPI_SYST_PARAM_DOUBLE(10.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.3e'           | '%30.3e' |",0), MIPI_SYST_PARAM_DOUBLE(10.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.4e'           | '%30.4e' |",0), MIPI_SYST_PARAM_DOUBLE(100.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.5e'           | '%30.5e' |",0), MIPI_SYST_PARAM_DOUBLE(1000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.6e'           | '%30.6e' |",0), MIPI_SYST_PARAM_DOUBLE(10000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.7e'           | '%30.7e' |",0), MIPI_SYST_PARAM_DOUBLE(10000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.8e'           | '%30.8e' |",0), MIPI_SYST_PARAM_DOUBLE(10000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.9e'           | '%30.9e' |",0), MIPI_SYST_PARAM_DOUBLE(100000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%30.10e'          | '%30.10e' |",0), MIPI_SYST_PARAM_DOUBLE(1000000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.1e'          | '%-30.1e' |",0), MIPI_SYST_PARAM_DOUBLE(1.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.2e'          | '%-30.2e' |",0), MIPI_SYST_PARAM_DOUBLE(10.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.3e'          | '%-30.3e' |",0), MIPI_SYST_PARAM_DOUBLE(10.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.4e'          | '%-30.4e' |",0), MIPI_SYST_PARAM_DOUBLE(100.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.5e'          | '%-30.5e' |",0), MIPI_SYST_PARAM_DOUBLE(1000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.6e'          | '%-30.6e' |",0), MIPI_SYST_PARAM_DOUBLE(10000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.7e'          | '%-30.7e' |",0), MIPI_SYST_PARAM_DOUBLE(10000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.8e'          | '%-30.8e' |",0), MIPI_SYST_PARAM_DOUBLE(10000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.9e'          | '%-30.9e' |",0), MIPI_SYST_PARAM_DOUBLE(100000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30.10e'         | '%-30.10e' |",0), MIPI_SYST_PARAM_DOUBLE(1000000.5));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("|-----------------------------------exotics-------------|",0));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030hhu'          | '%030hhu' |",0), MIPI_SYST_PARAM_CHAR((unsigned char)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030hu'           | '%030hu' |",0), MIPI_SYST_PARAM_INT((unsigned int)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030lu'           | '%030lu' |",0), MIPI_SYST_PARAM_LONG((unsigned long int) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030llu'          | '%030llu' |",0), MIPI_SYST_PARAM_LONGLONG((unsigned long long) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030hho'          | '%030hho' |",0), MIPI_SYST_PARAM_CHAR((unsigned char)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030ho'           | '%030ho' |",0), MIPI_SYST_PARAM_INT((unsigned int)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030lo'           | '%030lo' |",0), MIPI_SYST_PARAM_LONG((unsigned long int) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030llo'          | '%030llo' |",0), MIPI_SYST_PARAM_LONGLONG((unsigned long long) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030hhx'          | '%030hhx' |",0), MIPI_SYST_PARAM_CHAR((unsigned char)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030hx'           | '%030hx' |",0), MIPI_SYST_PARAM_INT((unsigned int)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030lx'           | '%030lx' |",0), MIPI_SYST_PARAM_LONG((unsigned long int) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%030llx'          | '%030llx' |",0), MIPI_SYST_PARAM_LONGLONG((unsigned long long) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30hhu'          | '%-30hhu' |",0), MIPI_SYST_PARAM_CHAR((unsigned char)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%-30hu'           | '%-30hu' |",0), MIPI_SYST_PARAM_INT((unsigned int)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30lu'          | '%#-30hu' |",0), MIPI_SYST_PARAM_LONG((unsigned long int) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30llu'         | '%#-30llu' |",0), MIPI_SYST_PARAM_LONGLONG((unsigned long long) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30hho'         | '%#-30hho' |",0), MIPI_SYST_PARAM_CHAR((unsigned char)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30ho'          | '%#-30ho' |",0), MIPI_SYST_PARAM_INT((unsigned int)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30lo'          | '%#-30lo' |",0), MIPI_SYST_PARAM_LONG((unsigned long int) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30llo'         | '%#-30llo' |",0), MIPI_SYST_PARAM_LONGLONG((unsigned long long) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30hhx'         | '%#-30hhx' |",0), MIPI_SYST_PARAM_CHAR((unsigned char)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30hx'          | '%#-30hx' |",0), MIPI_SYST_PARAM_INT((unsigned int)-1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30lx'          | '%#-30lx' |",0), MIPI_SYST_PARAM_LONG((unsigned long int) - 1));
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("| '%%#-30llx'         | '%#-30llx' |",0), MIPI_SYST_PARAM_LONGLONG((unsigned long long) - 1));
}