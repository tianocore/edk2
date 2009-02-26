
ASSEMBLY_CODE_FILE_LIST = $(MODULE_DIR)/bootsect.S \
                          $(MODULE_DIR)/bs16.S \
                          $(MODULE_DIR)/bs32.S \
                          $(MODULE_DIR)/efi32.S \
                          $(MODULE_DIR)/Gpt.S \
                          $(MODULE_DIR)/Mbr.S \
                          $(MODULE_DIR)/start.S \
                          $(MODULE_DIR)/start16.S \
                          $(MODULE_DIR)/start32.S

TARGET_FILES = $(OUTPUT_DIR)/bootsect.bin \
               $(OUTPUT_DIR)/bs16.bin \
               $(OUTPUT_DIR)/bs32.bin \
               $(OUTPUT_DIR)/Gpt.bin \
               $(OUTPUT_DIR)/Mbr.bin \
               $(OUTPUT_DIR)/start.bin \
               $(OUTPUT_DIR)/start16.bin \
               $(OUTPUT_DIR)/start32.bin \
            #  $(OUTPUT_DIR)/start64.bin \
            #  $(OUTPUT_DIR)/st16_64.bin \
            #  $(OUTPUT_DIR)/st32_64.bin \
               $(OUTPUT_DIR)/efi32.bin \
            #  $(OUTPUT_DIR)/efi64.bin2 

.PHONY : all
all: $(TARGET_FILES)

# bootsect.S
$(OUTPUT_DIR)/bootsect.o: $(MODULE_DIR)/bootsect.S
	$(ASM) -c -o $(OUTPUT_DIR)/bootsect.o $(MODULE_DIR)/bootsect.S
$(OUTPUT_DIR)/bootsect.bin: $(OUTPUT_DIR)/bootsect.o
	$(DLINK) -o $(OUTPUT_DIR)/bootsect.bin $(OUTPUT_DIR)/bootsect.o -Ttext 0

# bs16.S
$(OUTPUT_DIR)/bs16.o: $(MODULE_DIR)/bs16.S
	$(ASM) -c -o $(OUTPUT_DIR)/bs16.o $(MODULE_DIR)/bs16.S
$(OUTPUT_DIR)/bs16.bin: $(OUTPUT_DIR)/bs16.o
	$(DLINK) -o $(OUTPUT_DIR)/bs16.bin $(OUTPUT_DIR)/bs16.o -Ttext 0

# bs32.S
$(OUTPUT_DIR)/bs32.o: $(MODULE_DIR)/bs32.S
	$(ASM) -c -o $(OUTPUT_DIR)/bs32.o $(MODULE_DIR)/bs32.S
$(OUTPUT_DIR)/bs32.bin: $(OUTPUT_DIR)/bs32.o
	$(DLINK) -o $(OUTPUT_DIR)/bs32.bin $(OUTPUT_DIR)/bs32.o -Ttext 0

# Gpt.S
$(OUTPUT_DIR)/Gpt.o: $(MODULE_DIR)/Gpt.S
	$(ASM) -c -o $(OUTPUT_DIR)/Gpt.o $(MODULE_DIR)/Gpt.S
$(OUTPUT_DIR)/Gpt.bin: $(OUTPUT_DIR)/Gpt.o
	$(DLINK) -o $(OUTPUT_DIR)/Gpt.bin $(OUTPUT_DIR)/Gpt.o -Ttext 0

# Mbr.S
$(OUTPUT_DIR)/Mbr.o: $(MODULE_DIR)/Mbr.S
	$(ASM) -c -o $(OUTPUT_DIR)/Mbr.o $(MODULE_DIR)/Mbr.S
$(OUTPUT_DIR)/Mbr.bin: $(OUTPUT_DIR)/Mbr.o
	$(DLINK) -o $(OUTPUT_DIR)/Mbr.bin $(OUTPUT_DIR)/Mbr.o -Ttext 0

# start.S
$(OUTPUT_DIR)/start.o: $(MODULE_DIR)/start.S
	$(ASM) -c -o $(OUTPUT_DIR)/start.o $(MODULE_DIR)/start.S
$(OUTPUT_DIR)/start.bin: $(OUTPUT_DIR)/start.o
	$(DLINK) -o $(OUTPUT_DIR)/start.bin $(OUTPUT_DIR)/start.o -Ttext 0

# start16.S
$(OUTPUT_DIR)/start16.o: $(MODULE_DIR)/start16.S
	$(ASM) -c -o $(OUTPUT_DIR)/start16.o $(MODULE_DIR)/start16.S
$(OUTPUT_DIR)/start16.bin: $(OUTPUT_DIR)/start16.o
	$(DLINK) -o $(OUTPUT_DIR)/start16.bin $(OUTPUT_DIR)/start16.o -Ttext 0

# start32.S
$(OUTPUT_DIR)/start32.o: $(MODULE_DIR)/start32.S
	$(ASM) -c -o $(OUTPUT_DIR)/start32.o $(MODULE_DIR)/start32.S
$(OUTPUT_DIR)/start32.bin: $(OUTPUT_DIR)/start32.o
	$(DLINK) -o $(OUTPUT_DIR)/start32.bin $(OUTPUT_DIR)/start32.o -Ttext 0

# efi32.S
$(OUTPUT_DIR)/efi32.o: $(MODULE_DIR)/efi32.S
	$(ASM) -c -o $(OUTPUT_DIR)/efi32.o $(MODULE_DIR)/efi32.S
$(OUTPUT_DIR)/efi32.bin: $(OUTPUT_DIR)/efi32.o
	$(DLINK) -o $(OUTPUT_DIR)/efi32.bin $(OUTPUT_DIR)/efi32.o -Ttext 0


clean:
ifneq ($(OUTPUT_DIR), )
	rm -r $(OUTPUT_DIR)
endif
ifneq ($(DEBUG_DIR), )
	rm -r $(DEBUG_DIR) 
endif



