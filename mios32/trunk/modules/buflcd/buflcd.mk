# $Id: buflcd.mk 1500 2012-09-01 21:24:29Z tk $

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/buflcd


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/buflcd/buflcd.c




# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/buflcd
