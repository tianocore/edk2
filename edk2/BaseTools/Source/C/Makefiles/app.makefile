MAKEROOT ?= ../..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)/bin/$(APPNAME)

.PHONY:all
all: $(MAKEROOT)/bin $(APPLICATION) 

$(APPLICATION): $(OBJECTS) 
	$(LINKER) -o $(APPLICATION) $(LFLAGS) $(OBJECTS) -L$(MAKEROOT)/libs $(LIBS)

include $(MAKEROOT)/Makefiles/footer.makefile
