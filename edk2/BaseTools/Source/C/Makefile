ARCH = IA32

!INCLUDE Makefiles\ms.common

LIBRARIES = Common
APPLICATIONS = \
  BootSectImage \
  EfiLdrImage \
  EfiRom \
  GenBootSector \
  GenCrc32 \
  GenFfs \
  GenFv \
  GenFw \
  GenPage \
  GenSec \
  GenVtf \
  LzmaCompress \
  Split \
  TianoCompress \
  VolInfo \
  VfrCompile

all: libs apps install

libs: $(LIBRARIES)
	@echo.
	@echo ######################
	@echo # Build libraries
	@echo ######################
	@if not exist $(LIB_PATH) mkdir $(LIB_PATH)
	@Makefiles\NmakeSubdirs.bat all $**

apps: $(APPLICATIONS)
	@echo.
	@echo ######################
	@echo # Build executables
	@echo ######################
	@if not exist $(BIN_PATH) mkdir $(BIN_PATH)
	@Makefiles\NmakeSubdirs.bat all $**

install: $(LIB_PATH) $(BIN_PATH)
	@echo.
	@echo ######################
	@echo # Install to $(SYS_LIB_PATH)
	@echo # Install to $(SYS_BIN_PATH)
	@echo ######################
	@-xcopy $(LIB_PATH)\*.lib $(SYS_LIB_PATH) /I /D /E /F /Y > NUL 2>&1
	@-xcopy $(BIN_PATH)\*.exe $(SYS_BIN_PATH) /I /D /E /F /Y > NUL 2>&1

.PHONY: clean
clean:
  @Makefiles\NmakeSubdirs.bat clean $(LIBRARIES) $(APPLICATIONS)

.PHONY: cleanall
cleanall:
  @Makefiles\NmakeSubdirs.bat cleanall $(LIBRARIES) $(APPLICATIONS)
  @del /f /q $(BIN_PATH)\*.pdb $(BIN_PATH)\*.ilk

!INCLUDE Makefiles\ms.rule

