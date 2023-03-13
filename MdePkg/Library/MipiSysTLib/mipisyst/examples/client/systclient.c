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

/* Example SyS-T client code using various instrumentation call types */

const struct mipi_syst_origin origin =
MIPI_SYST_GEN_ORIGIN_GUID(0x494E5443, 0x8A9C, 0x4014, 0xA65A, 0x2F36A36D96E4, 1);
struct mipi_syst_handle * systh;

void banner();
void sinewave();
void raw();
void printf_format_test();
void catalog_format_test();
void short_messages();

int main(int argc, char* argv[])
{
#if defined(MIPI_SYST_STATIC)
	MIPI_SYST_INIT(mipi_syst_platform_init, 0);
#endif

	systh = MIPI_SYST_ALLOC_HANDLE(&origin);

	/* add optional fields */
	MIPI_SYST_ENABLE_HANDLE_CHECKSUM(systh, 1);
	MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(systh, 1);

	MIPI_SYST_BUILD(systh, MIPI_SYST_SEVERITY_MAX, 0x00010000, "version banner string", sizeof("version banner string"));

	MIPI_SYST_CATALOG32_3(systh, MIPI_SYST_SEVERITY_INFO,
		MIPI_SYST_HASH("SyS-T Library version %d.%d.%d",0),
		MIPI_SYST_VERSION_MAJOR,
		MIPI_SYST_VERSION_MINOR,
		MIPI_SYST_VERSION_PATCH);

	banner();
	catalog_format_test();
	printf_format_test();
	sinewave();
	raw();
	short_messages();

	/* Different catalog calls */
	MIPI_SYST_CATPRINTF64_0(systh, MIPI_SYST_SEVERITY_INFO, 0x1122334455667788, "Hello world\n");
	MIPI_SYST_CATPRINTF32(systh, MIPI_SYST_SEVERITY_INFO, 2, "%s=%d\n",
		MIPI_SYST_PARAM_CSTR("state"),
		MIPI_SYST_PARAM_INT(10)
	);
	MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO, MIPI_SYST_HASH("%s=%d\n", 0),
		MIPI_SYST_PARAM_CSTR("state"),
		MIPI_SYST_PARAM_INT(10)
	);

	/* Release any resources associated with this SyS-T handle.
	*/
	MIPI_SYST_DELETE_HANDLE(systh);

#if defined(MIPI_SYST_STATIC)
	MIPI_SYST_SHUTDOWN(mipi_syst_platform_destroy);
#endif

	return 0;
}