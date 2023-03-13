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

#include "mipi_syst.h"   /* SyS-T definitions     */


/* Define origin used by this example as the message client ID
 */
static const struct mipi_syst_origin origin =
MIPI_SYST_GEN_ORIGIN_GUID(0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35, 1);


int main(int argc, char* argv[])
{
    struct mipi_syst_handle* systh;

#if defined(MIPI_SYST_STATIC)
    /* Initialize SyS-T subsystem when using static linking to SyS-T.
     * Note: The call to MIPI_SYS_INIT() is done by the shared library
     * loader initialization function when dyanmic linking is used.
     */
    MIPI_SYST_INIT(mipi_syst_platform_init, 0);
#endif

    /* Initialize a SyS-T output handle structure
     */
    systh = MIPI_SYST_ALLOC_HANDLE( &origin );

    /* Send a string message with payload "Hello World!"
     */
    MIPI_SYST_DEBUG(systh, MIPI_SYST_SEVERITY_INFO, "Hello SyS-T!" ,sizeof("Hello SyS-T!"));

    /* Release any resources associated with this SyS-T handle.
     */
    MIPI_SYST_DELETE_HANDLE(systh);

#if defined(MIPI_SYST_STATIC)
    /* Perform SyS-T subsystem cleanup when using static linking.
     */
    MIPI_SYST_SHUTDOWN(mipi_syst_platform_destroy);
#endif
    return 0;
}