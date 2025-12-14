MAKEFILES = $(wildcard */*/Makefile)
SKETCHES_ALL := $(sort $(dir $(MAKEFILES)))
ROOTDIR := $(abspath $(CURDIR)/..)
RELEASE_DIR := $(abspath $(CURDIR)/../release)

# Optional: build only some sketches
# Examples:
#   make all ONLY=hornet/ufc/
#   make all ONLY="hornet/ufc/ hornet/apu/"
#   make all ONLY=hornet   (prefix match)
ONLY ?=

# If ONLY is set, keep entries that match any token in ONLY (substring/prefix match).
ifneq ($(strip $(ONLY)),)
SKETCHES := $(sort $(foreach s,$(SKETCHES_ALL),$(if $(filter %$(ONLY)%,$(s)),$(s),)))
else
SKETCHES := $(SKETCHES_ALL)
endif

export

all: $(SKETCHES)

$(SKETCHES):
	$(MAKE) -C $@ $(MAKECMDGOALS)

prep_release:
	mkdir -p $(RELEASE_DIR)

release: prep_release $(SKETCHES)

clean:
	@for d in $(SKETCHES_ALL); do \
		$(MAKE) -C $$d clean; \
	done

.PHONY: all release clean prep_release $(SKETCHES)
