# $Id: app_lcd.mk 1307 2011-08-21 01:12:08Z tk $
# defines additional rules for application specific LCD driver

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/modules/app_lcd/universal

# add modules to thumb sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/app_lcd/universal/app_lcd.c

# include fonts
include $(MIOS32_PATH)/modules/glcd_font/glcd_font.mk

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/app_lcd/universal
