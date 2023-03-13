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

/*
 * Contributors:
 * Norbert Schulz (Intel Corporation) - Initial API and implementation
 */


/* Example platform adaptation
 * This "platform" shows how to implement a SyS-T library platform module
 * to simulate MIPI STP data protocol generation for SyS-T messages.
 */
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>

#include "mipi_syst.h"

#if defined(_WIN32)
static CRITICAL_SECTION lockPlat;
#define LOCKING_INIT()    InitializeCriticalSection(&lockPlat)
#define LOCKING_DESTROY() DeleteCriticalSection(&lockPlat)
#define LOCK()            EnterCriticalSection(&lockPlat)
#define UNLOCK()          LeaveCriticalSection(&lockPlat)

#else

#include <pthread.h>
static pthread_mutex_t lockPlat;

#define LOCKING_INIT()      pthread_mutex_init(&lockPlat, NULL)
#define LOCKING_DESTROY()   pthread_mutex_destroy(&lockPlat)
#define LOCK()              pthread_mutex_lock(&lockPlat)
#define UNLOCK()            pthread_mutex_unlock(&lockPlat)

#endif

MIPI_SYST_EXPORT mipi_syst_u64 MIPI_SYST_CALLCONV mipi_syst_get_epoch_us()
{
	mipi_syst_u64 epoch;
#if defined(MIPI_SYST_UNIT_TEST)
	epoch = 0x12345678aabbccddull;
#elif defined(_WIN32)
	// windows does not offer epoch time API directly.
	// Search for the 116444... constant below on
	// msdn for an explanation of this computation:
	//
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	epoch = ft.dwHighDateTime;
	epoch = ((epoch<<32) | ft.dwLowDateTime) / 10 - 11644473600000000ULL;
#else
	struct timeval    tv;
	gettimeofday(&tv, NULL);
	epoch = tv.tv_sec;
	epoch *= 1000000;
	epoch += tv.tv_usec;
#endif
	return epoch;
}

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
MIPI_SYST_EXPORT
void * MIPI_SYST_CALLCONV  mipi_syst_platform_alloc(size_t s)
{
	return malloc(s);
}

MIPI_SYST_EXPORT
void MIPI_SYST_CALLCONV mipi_syst_platform_free(void * p)
{
	free(p);
}

#endif /* defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY) */

/* helper class for generating STP protocol data
*/
struct stp_writer_data {
	FILE *         fp;        /* output		   */
	mipi_syst_u8   byteDone;  /* true = byte complete  */
	mipi_syst_u8   current;   /* current byte value	   */
	mipi_syst_u16  master;    /* current master	   */
	mipi_syst_u16  channel;   /* current channel	   */
	mipi_syst_u64  recordCount; /* count stp packets   */

	mipi_syst_u64  timestamp;  /* first timestamp      */
};


static struct stp_writer_data writer_state;

void stp_write_putNibble(struct stp_writer_data* p, mipi_syst_u8 n)
{
	p->current |= (n<<4);
	p->byteDone = ! p->byteDone;

	if (p->byteDone) {    /* push it out .. */
		fputc(p->current, p->fp);
		p->current = 0;
	} else {          /* first nibble, shift it down to b0..3 */
		p->current >>= 4;
	}
}

void stp_write_flush(struct stp_writer_data* p) {
	if (!p->byteDone) {
		stp_write_putNibble(p, 0);
	}

	fflush(p->fp);
}
void stp_write_d4(struct stp_writer_data* p, mipi_syst_u8 v) {
	stp_write_putNibble(p, v);
}

void stp_write_payload8(struct stp_writer_data* p, mipi_syst_u8 v) {
	stp_write_d4(p, v);
	stp_write_d4(p, v>>4);
}

void stp_write_payload16(struct stp_writer_data* p, mipi_syst_u16 v) {
	stp_write_payload8(p, (mipi_syst_u8)v);
	stp_write_payload8(p, (mipi_syst_u8)(v>>8));
}

void stp_write_payload32(struct stp_writer_data* p, mipi_syst_u32 v) {
	stp_write_payload16(p, (mipi_syst_u16)v);
	stp_write_payload16(p, (mipi_syst_u16)(v>>16));
}

void stp_write_payload64(struct stp_writer_data* p, mipi_syst_u64 v) {
	stp_write_payload32(p, (mipi_syst_u32)v);
	stp_write_payload32(p, (mipi_syst_u32)(v>>32));
}

mipi_syst_u64 deltaTime(struct stp_writer_data* p)
{
	mipi_syst_u64 delta;

	delta = mipi_syst_get_epoch_us() - p->timestamp;
	return delta * 60; /* simluate 60Mhz clock */
}
void stp_write_d32mts(struct stp_writer_data* p, mipi_syst_u32 v) {
	stp_write_d4(p, 0xA);
	stp_write_payload32(p, v);

	stp_write_d4(p, 0xE);
	stp_write_payload64(p, deltaTime(p));
}

void stp_write_d64mts(struct stp_writer_data* p, mipi_syst_u64 v) {
	stp_write_d4(p, 0xB);
	stp_write_payload64(p, v);

	stp_write_d4(p, 0xE);
	stp_write_payload64(p, deltaTime(p));
}
void stp_write_d32ts(struct stp_writer_data* p, mipi_syst_u32 v) {
	stp_write_d4(p, 0xF);
	stp_write_d4(p, 0x6);

	stp_write_payload32(p, v);

	stp_write_d4(p, 0xE);
	stp_write_payload64(p, deltaTime(p));
}

void stp_write_d8(struct stp_writer_data* p, mipi_syst_u8 v) {
	stp_write_d4(p, 0x4);
	stp_write_payload8(p, v);
}

void stp_write_d16(struct stp_writer_data* p, mipi_syst_u16 v) {
	stp_write_d4(p, 0x5);
	stp_write_payload16(p, v);
}


void stp_write_d32(struct stp_writer_data* p, mipi_syst_u32 v) {
	stp_write_d4(p, 0x6);
	stp_write_payload32(p, v);
}


void stp_write_d64(struct stp_writer_data* p, mipi_syst_u64 v)
{
	stp_write_d4(p, 0x7);
	stp_write_payload64(p, v);
}

void stp_write_flag(struct stp_writer_data* p)
{
	stp_write_d4(p, 0xF);
	stp_write_d4(p, 0xE);
}

void stp_write_async(struct stp_writer_data* p)
{
	int i;
	for(i=0; i < 21; ++i) {
		stp_write_d4(p, 0xF);
	}
	stp_write_d4(p, 0x0);
}

void stp_write_version(struct stp_writer_data* p)
{
	stp_write_d4(p, 0xF);
	stp_write_d4(p, 0x0);
	stp_write_d4(p, 0x0);

	stp_write_d4(p, 0x3);      /* STPv2NAT */

	p->master = p->channel = 0;
}

void stp_write_freq(struct stp_writer_data* p)
{
	stp_write_d4(p, 0xF);
	stp_write_d4(p, 0x0);
	stp_write_d4(p, 0x8);
	stp_write_payload32(p,  60 * 1000 * 1000 );  // 60 Mhz
}

void stp_write_setMC(struct stp_writer_data* p,
		     mipi_syst_u16 master,
		     mipi_syst_u16 channel)
{
	/* re-send async after 20 packets */
	if (!(p->recordCount++ % 20) ) {
		stp_write_async(p);
		stp_write_version(p);
		stp_write_freq(p);
	}

	if (p->master != master ) {
		stp_write_d4(p, 0xF);
		stp_write_d4(p, 0x1);
		stp_write_payload16(p, master);

		p->master = master;
		p->channel = 0;
	}

	if (p->channel != channel) {
		stp_write_d4(p, 0xF);
		stp_write_d4(p, 0x3);
		stp_write_payload16(p, channel);

		p->channel = channel;
	}
}



/* output stream for STP data
*/
static FILE * fp;
static mipi_syst_u16 master = 128;  /* Default MIPI STP master:channel */
static mipi_syst_u16 channel = 1;


/* low level "driver" output routines */
static void sth_write_d32ts(struct mipi_syst_handle* systh, mipi_syst_u32 v);
static void sth_write_d32mts(struct mipi_syst_handle* systh, mipi_syst_u32 v);
static void sth_write_d64mts(struct mipi_syst_handle* systh, mipi_syst_u64 v);
static void sth_write_d8(struct mipi_syst_handle* systh, mipi_syst_u8 v);
static void sth_write_d16(struct mipi_syst_handle* systh, mipi_syst_u16 v);
static void sth_write_d32(struct mipi_syst_handle* systh, mipi_syst_u32 v);
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void sth_write_d64(struct mipi_syst_handle* systh, mipi_syst_u64 vp);
#endif
static void sth_write_user8ts(struct mipi_syst_handle* systh, mipi_syst_u8 v);
static void sth_write_flag(struct mipi_syst_handle* systh);

/*
* Platform specific SyS-T handle initialization hook function
*
* @param systh pointer to the new SyS-T handle structure
*/
static void platform_handle_init(struct mipi_syst_handle* systh)
{
	LOCK();

	/* Simply increment channels on each handle request and advance
	* to next master if all consumed.
	*/
	if (channel > 127) {
		++master;
		channel = 1;
	}

	systh->systh_platform.channel = channel++;
	systh->systh_platform.master  = master;

	UNLOCK();
}

/**
* Platform specific SyS-T handle initialization hook function
*
* @param systh pointer to the new SyS-T handle structure
*/
static void platform_handle_release(struct mipi_syst_handle* systh)
{
	LOCK();

	/* add any race protected cleanup code here
	*/

	UNLOCK();
}

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_platform_init(struct mipi_syst_header* systh, const void * platform_data)
{
	const char * filename;

	LOCKING_INIT();

	/* Platform data is expected to be UTF-8 string with the STP output
	* file name.
	*/
	filename = (const char *)platform_data;

	if (filename == NULL ||!strcmp(filename, "-"))
	{
		fp = stdout;
	} else {
		fp = fopen((char*)platform_data, "wb");

		if (fp == NULL) {
			fprintf(stderr, "Unable to open file '%s'.\n", (char*)platform_data);
			exit(-1);
		}
	}

	/* create the STP output writer
	*/
	writer_state.byteDone = 0;
	writer_state.fp = fp;
	writer_state.current = 0;
	writer_state.master = 0;
	writer_state.channel = 0;
	writer_state.recordCount=0;
	writer_state.timestamp = mipi_syst_get_epoch_us();

	systh->systh_platform.stpWriter = & writer_state;

	/* Set handle init hook that performs per SyS-T handle initialization
	* and destruction
	*/
	systh->systh_inith    = platform_handle_init;
	systh->systh_releaseh = platform_handle_release;

	/* Initialize platform specific data in global SyS-T state
	* This platform example puts its low level output function
	* pointers here. A real implementation may have these "inlined"
	* for performance reasons.
	*/
	systh->systh_platform.write_d32ts  = sth_write_d32ts;
	systh->systh_platform.write_d32mts = sth_write_d32mts;
	systh->systh_platform.write_d64mts = sth_write_d64mts;
	systh->systh_platform.write_d8     = sth_write_d8;
	systh->systh_platform.write_d16    = sth_write_d16;
	systh->systh_platform.write_d32    = sth_write_d32;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
	systh->systh_platform.write_d64    = sth_write_d64;
#endif
	systh->systh_platform.write_flag   = sth_write_flag;
}

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_platform_destroy(struct mipi_syst_header* systh)
{
	if (systh->systh_platform.stpWriter != NULL &&
	    systh->systh_platform.stpWriter->fp != NULL)
	{
		fflush(systh->systh_platform.stpWriter->fp);

		if (systh->systh_platform.stpWriter->fp != stdout)
		{
			fclose(systh->systh_platform.stpWriter->fp);
			systh->systh_platform.stpWriter->fp = NULL;
		}
	}

	LOCKING_DESTROY();
}

#if !defined(MIPI_SYST_STATIC)

/*
* This example platform uses SyS-T as a shared library inside an
* application. The platform init hook is called during a shared library
* constructor call.
*/
static MIPI_SYST_SHAREDLIB_CONSTRUCTOR
	void shared_library_init()
{
	const char * filename;

	filename = getenv("SYSTCAT_OUTPUT");
	if (filename == NULL) {
		filename = "syst_stp_data.bin";
	}

	MIPI_SYST_INIT(mipi_syst_platform_init, filename);

	if (filename[0] != '-' && filename[1] != 0  ) {
		printf("writing STP data into '%s' ...\n", filename);
	}
}

/*
* This example platform  uses SyS-T as a shared library inside an
* application. The platform destroy hook is called during a shared library
* destructor call.
*/
static MIPI_SYST_SHAREDLIB_DESTRUCTOR void shared_library_exit()
{
	MIPI_SYST_SHUTDOWN(mipi_syst_platform_destroy);
}

#if defined(_WIN32)
/*
* Windows DLL main routine, needed to run the global initialization and
* destruction handlers.
*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
	switch(fdwReason) {
	case DLL_PROCESS_ATTACH:
		shared_library_init();
		break;
	case DLL_PROCESS_DETACH:
		shared_library_exit();
		break;
	}
	return TRUE;
}
#endif

#endif /* #if !defined(MIPI_SYST_STATIC) */


/*  C code low level driver output routine wrappers for SyS-T
*/
static void sth_write_d32ts(struct mipi_syst_handle* systh, mipi_syst_u32 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;

	/*  Each message starts with d32ts. We use this as an		*/
	/*  indicator to lock the output writer until we wrote the	*/
	/*  end of record flag pattern.					*/
	LOCK();


	stp_write_setMC(writer,
		systh->systh_platform.master, systh->systh_platform.channel );
	stp_write_d32ts(writer, v);
}

/* short single 32 bit payload message */
static void sth_write_d32mts(struct mipi_syst_handle* systh, mipi_syst_u32 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;

	LOCK();

	stp_write_setMC(writer,
		systh->systh_platform.master,systh->systh_platform.channel );
	stp_write_d32mts(writer, v);

	UNLOCK();
}
/* short single 64 bit payload message */
static void sth_write_d64mts(struct mipi_syst_handle* systh, mipi_syst_u64 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;

	LOCK();

	stp_write_setMC(writer,
		systh->systh_platform.master,systh->systh_platform.channel );
	stp_write_d64mts(writer, v);

	UNLOCK();
}
static void sth_write_d8(struct mipi_syst_handle* systh, mipi_syst_u8 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;
	stp_write_d8(writer, v);
}

static void sth_write_d16(struct mipi_syst_handle* systh, mipi_syst_u16 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;
	stp_write_d16(writer, v);
}

static void sth_write_d32(struct mipi_syst_handle* systh, mipi_syst_u32 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;
	stp_write_d32(writer, v);
}

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void sth_write_d64(struct mipi_syst_handle* systh, mipi_syst_u64 v)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;
	stp_write_d64(writer, v);
}
#endif

static void sth_write_flag(struct mipi_syst_handle* systh)
{
	struct stp_writer_data * writer = systh->systh_header->systh_platform.stpWriter;

	stp_write_flag(writer);
	stp_write_flush(writer);

	/* atomic record write done - matching lock was in sth_write_d32ts() */
	UNLOCK();
}