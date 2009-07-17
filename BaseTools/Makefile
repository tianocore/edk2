
SUBDIRS = Source\C Source\Python

all: c python

c :
	@Source\C\Makefiles\NmakeSubdirs.bat all Source\C

python:
	@Source\C\Makefiles\NmakeSubdirs.bat all Source\Python

subdirs: $(SUBDIRS)
	@Source\C\Makefiles\NmakeSubdirs.bat all $**

.PHONY: clean
clean:
	@Source\C\Makefiles\NmakeSubdirs.bat clean $(SUBDIRS)

.PHONY: cleanall
cleanall:
	@Source\C\Makefiles\NmakeSubdirs.bat cleanall $(SUBDIRS)

