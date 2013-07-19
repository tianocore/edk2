#
# Makefile - build a UEFI boot image for booting from different exception levels.
#
# Copyright (C) 2011-2013 ARM Limited.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of ARM nor the names of its contributors may be
#       used to endorse or promote products derived from this software
#       without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This license can also be found in the LICENSE.TXT file.


# VE
PHYS_OFFSET	:= 0x80000000
UART_BASE	:= 0x1c090000
GIC_DIST_BASE	:= 0x2c001000
GIC_CPU_BASE	:= 0x2c002000
CNTFRQ		:= 0x01800000	# 24Mhz

BOOTLOADER	:= boot.S
LD_SCRIPT	:= model.lds.S
IMAGE_1		:= uefi-bootstrap-el1.axf
IMAGE_2		:= uefi-bootstrap-el2.axf
IMAGE_3		:= uefi-bootstrap-el3.axf
IMAGE_3F	:= uefi-bootstrap-el3-foundation.axf


CROSS_COMPILE	?= aarch64-none-elf-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld

all: $(IMAGE_1) $(IMAGE_2) $(IMAGE_3) $(IMAGE_3F)

clean:
	rm -f *.axf *.o *.lds

$(IMAGE_1): boot1.o model1.lds
	$(LD) -o $@ --script=model1.lds

$(IMAGE_2): boot2.o model2.lds
	$(LD) -o $@ --script=model2.lds

$(IMAGE_3): boot3.o model3.lds
	$(LD) -o $@ --script=model3.lds

$(IMAGE_3F): boot3f.o model3f.lds
	$(LD) -o $@ --script=model3f.lds

boot1.o: $(BOOTLOADER) Makefile
	$(CC) $(CPPFLAGS) -DUART_BASE=$(UART_BASE) -DCNTFRQ=$(CNTFRQ) -DGIC_DIST_BASE=$(GIC_DIST_BASE) -DGIC_CPU_BASE=$(GIC_CPU_BASE) -DSTART_EL2=1 -DSTART_EL1=1 -c -o $@ $(BOOTLOADER)

boot2.o: $(BOOTLOADER) Makefile
	$(CC) $(CPPFLAGS) -DUART_BASE=$(UART_BASE) -DCNTFRQ=$(CNTFRQ) -DGIC_DIST_BASE=$(GIC_DIST_BASE) -DGIC_CPU_BASE=$(GIC_CPU_BASE) -DSTART_EL2=1 -c -o $@ $(BOOTLOADER)

boot3.o: $(BOOTLOADER) Makefile
	$(CC) $(CPPFLAGS) -DUART_BASE=$(UART_BASE) -DCNTFRQ=$(CNTFRQ) -DGIC_DIST_BASE=$(GIC_DIST_BASE) -DGIC_CPU_BASE=$(GIC_CPU_BASE) -c -o $@ $(BOOTLOADER)

boot3f.o: $(BOOTLOADER) Makefile
	$(CC) $(CPPFLAGS) -DUART_BASE=$(UART_BASE) -DCNTFRQ=$(CNTFRQ) -DGIC_DIST_BASE=$(GIC_DIST_BASE) -DGIC_CPU_BASE=$(GIC_CPU_BASE) -DFOUNDATION_MODEL=1 -c -o $@ $(BOOTLOADER)

model1.lds: $(LD_SCRIPT) Makefile boot1.o
	$(CC) $(CPPFLAGS) -DPHYS_OFFSET=$(PHYS_OFFSET) -DBOOT1 -E -P -C -o $@ $<

model2.lds: $(LD_SCRIPT) Makefile boot2.o
	$(CC) $(CPPFLAGS) -DPHYS_OFFSET=$(PHYS_OFFSET) -DBOOT2 -E -P -C -o $@ $<

model3.lds: $(LD_SCRIPT) Makefile boot3.o
	$(CC) $(CPPFLAGS) -DPHYS_OFFSET=$(PHYS_OFFSET) -DBOOT3 -E -P -C -o $@ $<

model3f.lds: $(LD_SCRIPT) Makefile boot3f.o
	$(CC) $(CPPFLAGS) -DPHYS_OFFSET=$(PHYS_OFFSET) -DBOOT3F -E -P -C -o $@ $<

.PHONY: all clean
