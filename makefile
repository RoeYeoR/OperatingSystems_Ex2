# Define the subdirectories
SUBDIRS := prog1 prog2 prog3 prog3.5 prog4  prog6

# Define the phony targets
.PHONY: all clean $(SUBDIRS)

# Default target
all: $(SUBDIRS)

# Target to build all subdirectories
$(SUBDIRS):
	$(MAKE) -C $@

# Clean all subdirectories
clean:
	for dir in $(SUBDIRS); do \
    	$(MAKE) -C $$dir clean; \
	done