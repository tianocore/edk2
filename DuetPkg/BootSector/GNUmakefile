## @file
#  Just use host GCC to compile boot sector image.
#
#  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#
##

ASM=gcc
DLINK=ld

ASSEMBLY_CODE_FILE_LIST = $(MODULE_DIR)/bootsect.S \
                          $(MODULE_DIR)/bs16.S \
                          $(MODULE_DIR)/bs32.S \
                          $(MODULE_DIR)/efi32.S \
                          $(MODULE_DIR)/efi64.S \
                          $(MODULE_DIR)/Gpt.S \
                          $(MODULE_DIR)/Mbr.S \
                          $(MODULE_DIR)/start.S \
                          $(MODULE_DIR)/start16.S \
                          $(MODULE_DIR)/start32.S \
                          $(MODULE_DIR)/start64.S \
                          $(MODULE_DIR)/st16_64.S \
                          $(MODULE_DIR)/st32_64.S

TARGET_FILES = $(OUTPUT_DIR)/bootsect.com \
               $(OUTPUT_DIR)/bs16.com \
               $(OUTPUT_DIR)/bs32.com \
               $(OUTPUT_DIR)/Gpt.com \
               $(OUTPUT_DIR)/Mbr.com \
               $(OUTPUT_DIR)/start.com \
               $(OUTPUT_DIR)/start16.com \
               $(OUTPUT_DIR)/start32.com \
               $(OUTPUT_DIR)/efi32.com2 \
               $(OUTPUT_DIR)/start64.com \
               $(OUTPUT_DIR)/st16_64.com \
               $(OUTPUT_DIR)/st32_64.com \
               $(OUTPUT_DIR)/efi64.com2

.PHONY : all
all: $(TARGET_FILES)

# bootsect.S
$(OUTPUT_DIR)/bootsect.o: $(MODULE_DIR)/bootsect.S
	$(ASM) -c -o $(OUTPUT_DIR)/bootsect.o $(MODULE_DIR)/bootsect.S
$(OUTPUT_DIR)/bootsect.com: $(OUTPUT_DIR)/bootsect.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/bootsect.com $(OUTPUT_DIR)/bootsect.o -Ttext 0 -Map $(OUTPUT_DIR)/bootsect.map

# bs16.S
$(OUTPUT_DIR)/bs16.o: $(MODULE_DIR)/bs16.S
	$(ASM) -c -o $(OUTPUT_DIR)/bs16.o $(MODULE_DIR)/bs16.S
$(OUTPUT_DIR)/bs16.com: $(OUTPUT_DIR)/bs16.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/bs16.com $(OUTPUT_DIR)/bs16.o -Ttext 0 -Map $(OUTPUT_DIR)/bs16.map

# bs32.S
$(OUTPUT_DIR)/bs32.o: $(MODULE_DIR)/bs32.S
	$(ASM) -c -o $(OUTPUT_DIR)/bs32.o $(MODULE_DIR)/bs32.S
$(OUTPUT_DIR)/bs32.com: $(OUTPUT_DIR)/bs32.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/bs32.com $(OUTPUT_DIR)/bs32.o -Ttext 0 -Map $(OUTPUT_DIR)/bs32.map

# Gpt.S
$(OUTPUT_DIR)/Gpt.o: $(MODULE_DIR)/Gpt.S
	$(ASM) -c -o $(OUTPUT_DIR)/Gpt.o $(MODULE_DIR)/Gpt.S
$(OUTPUT_DIR)/Gpt.com: $(OUTPUT_DIR)/Gpt.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/Gpt.com $(OUTPUT_DIR)/Gpt.o -Ttext 0 -Map $(OUTPUT_DIR)/Gpt.map

# Mbr.S
$(OUTPUT_DIR)/Mbr.o: $(MODULE_DIR)/Mbr.S
	$(ASM) -c -o $(OUTPUT_DIR)/Mbr.o $(MODULE_DIR)/Mbr.S
$(OUTPUT_DIR)/Mbr.com: $(OUTPUT_DIR)/Mbr.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/Mbr.com $(OUTPUT_DIR)/Mbr.o -Ttext 0 -Map $(OUTPUT_DIR)/Mbr.map

# start.S
$(OUTPUT_DIR)/start.o: $(MODULE_DIR)/start.S
	$(ASM) -c -o $(OUTPUT_DIR)/start.o $(MODULE_DIR)/start.S
$(OUTPUT_DIR)/start.com: $(OUTPUT_DIR)/start.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/start.com $(OUTPUT_DIR)/start.o -Ttext 0 -Map $(OUTPUT_DIR)/start.map

# start16.S
$(OUTPUT_DIR)/start16.o: $(MODULE_DIR)/start16.S
	$(ASM) -c -o $(OUTPUT_DIR)/start16.o $(MODULE_DIR)/start16.S
$(OUTPUT_DIR)/start16.com: $(OUTPUT_DIR)/start16.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/start16.com $(OUTPUT_DIR)/start16.o -Ttext 0 -Map $(OUTPUT_DIR)/start16.map

# start32.S
$(OUTPUT_DIR)/start32.o: $(MODULE_DIR)/start32.S
	$(ASM) -c -o $(OUTPUT_DIR)/start32.o $(MODULE_DIR)/start32.S
$(OUTPUT_DIR)/start32.com: $(OUTPUT_DIR)/start32.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/start32.com $(OUTPUT_DIR)/start32.o -Ttext 0 -Map $(OUTPUT_DIR)/start32.map

# start64.S
$(OUTPUT_DIR)/start64.o: $(MODULE_DIR)/start64.S
	$(ASM) -c -o $(OUTPUT_DIR)/start64.o $(MODULE_DIR)/start64.S
$(OUTPUT_DIR)/start64.com: $(OUTPUT_DIR)/start64.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/start64.com $(OUTPUT_DIR)/start64.o -Ttext 0 -Map $(OUTPUT_DIR)/start64.map

# start16_64.S
$(OUTPUT_DIR)/st16_64.o: $(MODULE_DIR)/st16_64.S
	$(ASM) -c -o $(OUTPUT_DIR)/st16_64.o $(MODULE_DIR)/st16_64.S
$(OUTPUT_DIR)/st16_64.com: $(OUTPUT_DIR)/st16_64.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/st16_64.com $(OUTPUT_DIR)/st16_64.o -Ttext 0 -Map $(OUTPUT_DIR)/st16_64.map

# start32_64.S
$(OUTPUT_DIR)/st32_64.o: $(MODULE_DIR)/st32_64.S
	$(ASM) -c -o $(OUTPUT_DIR)/st32_64.o $(MODULE_DIR)/st32_64.S
$(OUTPUT_DIR)/st32_64.com: $(OUTPUT_DIR)/st32_64.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/st32_64.com $(OUTPUT_DIR)/st32_64.o -Ttext 0 -Map $(OUTPUT_DIR)/st32_64.map

# efi32.S
$(OUTPUT_DIR)/efi32.o: $(MODULE_DIR)/efi32.S
	$(ASM) -c -o $(OUTPUT_DIR)/efi32.o $(MODULE_DIR)/efi32.S
$(OUTPUT_DIR)/efi32.com: $(OUTPUT_DIR)/efi32.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/efi32.com $(OUTPUT_DIR)/efi32.o -Ttext 0 -Map $(OUTPUT_DIR)/efi32.map
$(OUTPUT_DIR)/efi32.com2: $(OUTPUT_DIR)/efi32.com
	Split -f $(OUTPUT_DIR)/efi32.com -p $(OUTPUT_DIR) -o efi32.com1 -p $(OUTPUT_DIR) -t efi32.com2 -s 135168

# efi64.S
$(OUTPUT_DIR)/efi64.o: $(MODULE_DIR)/efi64.S
	$(ASM) -c -o $(OUTPUT_DIR)/efi64.o $(MODULE_DIR)/efi64.S
$(OUTPUT_DIR)/efi64.com: $(OUTPUT_DIR)/efi64.o
	$(DLINK) --oformat binary -o $(OUTPUT_DIR)/efi64.com $(OUTPUT_DIR)/efi64.o -Ttext 0 -Map $(OUTPUT_DIR)/efi64.map
$(OUTPUT_DIR)/efi64.com2: $(OUTPUT_DIR)/efi64.com
	Split -f $(OUTPUT_DIR)/efi64.com -p $(OUTPUT_DIR) -o efi64.com1 -p $(OUTPUT_DIR) -t efi64.com2 -s 135168

clean:
ifneq ($(OUTPUT_DIR), )
	rm -r $(OUTPUT_DIR)
endif
ifneq ($(DEBUG_DIR), )
	rm -r $(DEBUG_DIR)
endif


