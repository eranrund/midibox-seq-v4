# $Id: eeprom.mk 1253 2011-07-14 22:45:50Z tk $

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/eeprom


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/eeprom/$(MIOS32_FAMILY)/eeprom.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/eeprom
