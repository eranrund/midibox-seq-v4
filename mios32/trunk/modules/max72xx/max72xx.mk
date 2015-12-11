# $Id: max72xx.mk 1686 2013-02-07 22:11:42Z tk $

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/max72xx


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/max72xx/max72xx.c




# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/max72xx
