
all: subdirs

LANGUAGES = C Python

SOURCE_SUBDIRS := $(patsubst %,Source/%,$(sort $(LANGUAGES)))
SUBDIRS := $(SOURCE_SUBDIRS) Tests
CLEAN_SUBDIRS := $(patsubst %,%-clean,$(sort $(SUBDIRS)))

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: $(CLEAN_SUBDIRS)
$(CLEAN_SUBDIRS):
	-$(MAKE) -C $(@:-clean=) clean

clean:  $(CLEAN_SUBDIRS)

test:
	@$(MAKE) -C Tests

