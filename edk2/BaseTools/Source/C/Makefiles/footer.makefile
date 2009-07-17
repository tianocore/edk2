DEPFILES = $(OBJECTS:%.o=%.d)

$(MAKEROOT)/libs-$(ARCH):
	mkdir -p $(MAKEROOT)/libs-$(ARCH)

.PHONY: install
install: $(MAKEROOT)/libs-$(ARCH) $(LIBRARY)
	cp $(LIBRARY) $(MAKEROOT)/libs-$(ARCH)

$(LIBRARY): $(OBJECTS) 
	$(AR) crs $@ $^

%.o : %.c 
	$(CC)  -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.S
	$(AS) -c $(ASFLAGS) $< -o $@

.PHONY: clean
clean:
	@rm -f $(OBJECTS) $(LIBRARY) $(DEPFILES)

-include $(DEPFILES)
