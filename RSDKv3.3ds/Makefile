ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

BUILDDIR=objects_3ds

LIBS_ALL = -lSDL_mixer -lSDL -lmikmod -lmad -ltheoradec -lvorbisidec -logg -lcitro2d -lcitro3d -lctru -lm
LIBDIRS = $(CTRULIB) $(PORTLIBS)
export LIBPATHS	= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#export COMMIT = $(shell git rev-parse --short HEAD)

ARCH	     = -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS_ALL   = $(INCLUDE) $(ARCH) -DARM_ASM_CLIP15=1 -O2 -g -DARM11 -D_3DS  -D__3DS__ -DCOMMIT=\"$(COMMIT)\" -DFORCE_CASE_INSENSITIVE
CXXFLAGS_ALL = $(CFLAGS_ALL) $(LIBS_ALL) -fno-rtti -std=gnu++17 -Idependencies/all/theoraplay
LDFLAGS	     = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LDFLAGS_ALL  = $(LDFLAGS)

TARGET = SonicCD

APP_TITLE := Sonic CD
APP_DESCRIPTION := Sonic CD (2011)
APP_AUTHOR := Rubberduckycooly, SaturnSH2x2, ultra0
APP_ICON := resources/48x48.png

export INCLUDE = $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                 -I$(CURDIR)/$(BUILD) -I../dependencies/all/theoraplay -I../dependencies/all/tinyxml2

export LD = $(CXX)

SOURCES = ../dependencies/all/tinyxml2/tinyxml2.cpp \
		../dependencies/all/theoraplay/theoraplay.c \
	  	../RSDKv3/3ds/debug_3ds.cpp \
		../RSDKv3/3ds/audio_3ds.cpp \
		../RSDKv3/3ds/render_3ds.cpp \
		../RSDKv3/3ds/video_3ds.cpp \
		../RSDKv3/3ds/3ds-theoraplayer/source/video.c \
		../RSDKv3/3ds/3ds-theoraplayer/source/frame.c \
		../RSDKv3/Animation.cpp \
		../RSDKv3/Audio.cpp \
		../RSDKv3/Collision.cpp \
		../RSDKv3/Debug.cpp \
		../RSDKv3/Drawing.cpp \
		../RSDKv3/fcaseopen.c \
		../RSDKv3/Ini.cpp \
		../RSDKv3/Input.cpp \
		../RSDKv3/main.cpp \
		../RSDKv3/Math.cpp \
		../RSDKv3/ModAPI.cpp \
		../RSDKv3/Object.cpp \
		../RSDKv3/Palette.cpp \
		../RSDKv3/Player.cpp \
		../RSDKv3/Reader.cpp \
		../RSDKv3/RetroEngine.cpp \
		../RSDKv3/Scene.cpp \
		../RSDKv3/Scene3D.cpp \
		../RSDKv3/Script.cpp \
		../RSDKv3/Sprite.cpp \
		../RSDKv3/String.cpp \
		../RSDKv3/Text.cpp \
		../RSDKv3/Userdata.cpp \
		../RSDKv3/Video.cpp

OBJS = dependencies/all/tinyxml2/tinyxml2.cpp.o \
		dependencies/all/theoraplay/theoraplay.c.o \
	  	RSDKv3/3ds/debug_3ds.cpp.o \
		RSDKv3/3ds/audio_3ds.cpp.o \
		RSDKv3/3ds/render_3ds.cpp.o \
		RSDKv3/3ds/video_3ds.cpp.o \
		RSDKv3/3ds/3ds-theoraplayer/source/video.c.o \
		RSDKv3/3ds/3ds-theoraplayer/source/frame.c.o \
		RSDKv3/Animation.cpp.o \
		RSDKv3/Audio.cpp.o \
		RSDKv3/Collision.cpp.o \
		RSDKv3/Debug.cpp.o \
		RSDKv3/Drawing.cpp.o \
		RSDKv3/fcaseopen.c.o \
		RSDKv3/Ini.cpp.o \
		RSDKv3/Input.cpp.o \
		RSDKv3/main.cpp.o \
		RSDKv3/Math.cpp.o \
		RSDKv3/ModAPI.cpp.o \
		RSDKv3/Object.cpp.o \
		RSDKv3/Palette.cpp.o \
		RSDKv3/Player.cpp.o \
		RSDKv3/Reader.cpp.o \
		RSDKv3/RetroEngine.cpp.o \
		RSDKv3/Scene.cpp.o \
		RSDKv3/Scene3D.cpp.o \
		RSDKv3/Script.cpp.o \
		RSDKv3/Sprite.cpp.o \
		RSDKv3/String.cpp.o \
		RSDKv3/Text.cpp.o \
		RSDKv3/Userdata.cpp.o \
		RSDKv3/Video.cpp.o

export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(TARGET).smdh)

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

$(BUILDDIR)/%.o: %
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS_ALL) $^ -o $@ -c

$(TARGET).3dsx: $(TARGET).elf $(_3DSXDEPS)

$(TARGET).elf: $(SOURCES:%=objects_3ds/%.o)
	$(SILENTMSG) linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OBJS)  $(LIBPATHS) $(LIBS_ALL) -o $@

clean:
	@echo "cleaning..."
	@rm -rf RSDKv3
	@rm -f SonicCD.3dsx
	@rm -f SonicCD.smdh
	@rm -f SonicCD.elf
	@rm -f SonicCD.cia

cia: $(TARGET).elf
	@bannertool makebanner -i resources/banner.png -a resources/jingle.wav -o "$(BUILDDIR)/banner.bnr"
	@bannertool makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$(APP_ICON)" -o "$(BUILDDIR)/icon.icn"
	@makerom -f cia -o "$(TARGET).cia" -DAPP_ENCRYPTED=false -elf "$(TARGET).elf" -rsf resources/SonicCD.rsf -exefslogo -target t -icon "$(BUILDDIR)/icon.icn" -banner "$(BUILDDIR)/banner.bnr"
	@echo "built cia"
