
BASETOOLS_DIR = $(EDK_TOOLS_PATH)\Bin\Win32

#
# Build Macro
#
ASSEMBLY_CODE_FILE_LIST = $(MODULE_DIR)\bootsect.asm \
                          $(MODULE_DIR)\bs16.asm \
                          $(MODULE_DIR)\bs32.asm \
                          $(MODULE_DIR)\efi32.asm \
                          $(MODULE_DIR)\Gpt.asm \
                          $(MODULE_DIR)\Mbr.asm \
                          $(MODULE_DIR)\start.asm \
                          $(MODULE_DIR)\start16.asm \
                          $(MODULE_DIR)\start32.asm 

TARGET_FILES = $(OUTPUT_DIR)\bootsect.com \
               $(OUTPUT_DIR)\bs16.com \
               $(OUTPUT_DIR)\bs32.com \
               $(OUTPUT_DIR)\Gpt.com \
               $(OUTPUT_DIR)\Mbr.com \
               $(OUTPUT_DIR)\Start.com \
               $(OUTPUT_DIR)\Start16.com \
               $(OUTPUT_DIR)\Start32.com \
               $(OUTPUT_DIR)\efi32.com2

INC = 
      

#OBJECTS = 

LIBS = 

COMMON_DEPS = 

all: $(TARGET_FILES)
          
#=============                
$(OUTPUT_DIR)\bootsect.obj:$(MODULE_DIR)\bootsect.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\bootsect.obj" /FR"$(OUTPUT_DIR)\bootsect.txt" "$(MODULE_DIR)\bootsect.asm"
             
$(OUTPUT_DIR)\bootsect.com:$(OUTPUT_DIR)\bootsect.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\bootsect.obj,$(OUTPUT_DIR)\bootsect.com,$(OUTPUT_DIR)\bootsect.map,,,

#=============                

$(OUTPUT_DIR)\bs16.obj:$(MODULE_DIR)\bs16.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\bs16.obj" "$(MODULE_DIR)\bs16.asm"
             
$(OUTPUT_DIR)\bs16.com:$(OUTPUT_DIR)\bs16.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\bs16.obj,$(OUTPUT_DIR)\bs16.com,$(OUTPUT_DIR)\bs16.map,,,

#=============                

$(OUTPUT_DIR)\bs32.obj:$(MODULE_DIR)\bs32.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\bs32.obj" "$(MODULE_DIR)\bs32.asm"
             
$(OUTPUT_DIR)\bs32.com:$(OUTPUT_DIR)\bs32.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\bs32.obj,$(OUTPUT_DIR)\bs32.com,$(OUTPUT_DIR)\bs32.map,,,

#=============                

$(OUTPUT_DIR)\Gpt.obj:$(MODULE_DIR)\Gpt.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\Gpt.obj" "$(MODULE_DIR)\Gpt.asm"
             
$(OUTPUT_DIR)\Gpt.com:$(OUTPUT_DIR)\Gpt.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Gpt.obj,$(OUTPUT_DIR)\Gpt.com,$(OUTPUT_DIR)\Gpt.map,,,

#=============                

$(OUTPUT_DIR)\Mbr.obj:$(MODULE_DIR)\Mbr.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\Mbr.obj" "$(MODULE_DIR)\Mbr.asm"
             
$(OUTPUT_DIR)\Mbr.com:$(OUTPUT_DIR)\Mbr.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Mbr.obj,$(OUTPUT_DIR)\Mbr.com,$(OUTPUT_DIR)\Mbr.map,,,

#============

$(OUTPUT_DIR)\Start.obj:$(MODULE_DIR)\Start.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\Start.obj" "$(MODULE_DIR)\Start.asm"
             
$(OUTPUT_DIR)\Start.com:$(OUTPUT_DIR)\Start.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Start.obj,$(OUTPUT_DIR)\Start.com,$(OUTPUT_DIR)\Start.map,,,
  
#=============     

$(OUTPUT_DIR)\Start16.obj:$(MODULE_DIR)\Start16.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\Start16.obj" "$(MODULE_DIR)\Start16.asm"
             
$(OUTPUT_DIR)\Start16.com:$(OUTPUT_DIR)\Start16.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Start16.obj,$(OUTPUT_DIR)\Start16.com,$(OUTPUT_DIR)\Start16.map,,,

#=============

$(OUTPUT_DIR)\Start32.obj:$(MODULE_DIR)\Start32.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\Start32.obj" "$(MODULE_DIR)\Start32.asm"
             
$(OUTPUT_DIR)\Start32.com:$(OUTPUT_DIR)\Start32.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Start32.obj,$(OUTPUT_DIR)\Start32.com,$(OUTPUT_DIR)\Start32.map,,,

#=============

$(OUTPUT_DIR)\efi32.obj:$(MODULE_DIR)\efi32.asm
  "$(ASM)" /c /omf /Fo"$(OUTPUT_DIR)\efi32.obj" "$(MODULE_DIR)\efi32.asm"
             
$(OUTPUT_DIR)\efi32.com:$(OUTPUT_DIR)\efi32.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\efi32.obj,$(OUTPUT_DIR)\efi32.com,$(OUTPUT_DIR)\efi32.map,,,

#=============

$(OUTPUT_DIR)\efi32.com2:$(OUTPUT_DIR)\efi32.com
  $(BASETOOLS_DIR)\Split.exe -f $(OUTPUT_DIR)\efi32.com -t $(OUTPUT_DIR)\efi32.com2 -s 135168
#
# clean all generated files
#

loader:$(BUILD_DIR)\FV\Efildr

$(BUILD_DIR)\FV\DUETEFIMAINFV.z:$(BUILD_DIR)\FV\DUETEFIMAINFV.Fv
  $(BASETOOLS_DIR)\TianoCompress -e -o $(BUILD_DIR)\FV\DUETEFIMAINFV.z $(BUILD_DIR)\FV\DUETEFIMAINFV.Fv
  
$(BUILD_DIR)\FV\DxeMain.z:$(BUILD_DIR)\IA32\DxeMain.efi  
  $(BASETOOLS_DIR)\TianoCompress -e -o $(BUILD_DIR)\FV\DxeMain.z $(BUILD_DIR)\IA32\DxeMain.efi
  
$(BUILD_DIR)\FV\DxeIpl.z:$(BUILD_DIR)\IA32\DxeIpl.efi  
  $(BASETOOLS_DIR)\TianoCompress -e -o $(BUILD_DIR)\FV\DxeIpl.z $(BUILD_DIR)\IA32\DxeIpl.efi
    
$(BUILD_DIR)\FV\Efildr32:$(BUILD_DIR)\IA32\EfiLoader.efi $(BUILD_DIR)\FV\DxeIpl.z $(BUILD_DIR)\FV\DUETEFIMAINFV.z
  $(BASETOOLS_DIR)\EfiLdrImage.exe -o $(BUILD_DIR)\FV\Efildr32 $(BUILD_DIR)\IA32\EfiLoader.efi $(BUILD_DIR)\FV\DxeIpl.z $(BUILD_DIR)\FV\DUETEFIMAINFV.z
  
$(BUILD_DIR)\FV\Efildr:$(OUTPUT_DIR)\Start.com $(OUTPUT_DIR)\Efi32.com2 $(BUILD_DIR)\FV\Efildr32
  copy /b $(OUTPUT_DIR)\Start.com+$(OUTPUT_DIR)\Efi32.com2+$(BUILD_DIR)\FV\Efildr32 $(BUILD_DIR)\FV\Efildr
    
clean:
	if exist $(DEBUG_DIR) rmdir /s /q $(DEBUG_DIR)
	if exist $(OUTPUT_DIR) rmdir /s /q $(OUTPUT_DIR)


