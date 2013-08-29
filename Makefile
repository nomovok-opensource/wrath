# Main top-level Makefile
#

# Build target names
#
# For every specified build target, files
#
#  Makefile.lib.NAME.pre
#  Makefile.demos.NAME.pre
#  Makefile.lib.NAME.post
#  Makefile.demos.NAME.post
#
# get included. See existing files for examples.
#
# This variable is also overrideable from the environment or from make
# command line.
BUILDTARGETS ?= qt sdl

include Makefile.settings
include Makefile.functions

include Makefile.common.pre

define preinclude
include Makefile.$(1).pre
endef

$(foreach target,$(BUILDTARGETS),$(eval $(call preinclude,$(target))))

include Makefile.sources

define postinclude
include Makefile.$(1).post
endef

$(foreach target,$(BUILDTARGETS),$(eval $(call postinclude,$(target))))

include Makefile.common.post
