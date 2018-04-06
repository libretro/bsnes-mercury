LOCAL_PATH := $(call my-dir)

SRCDIR := $(LOCAL_PATH)/../..

ifeq ($(profile),)
  profile := performance
endif

INCFLAGS  := -I$(SRCDIR)
COREFLAGS := -fomit-frame-pointer -ffast-math -D__LIBRETRO__ $(INCFLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

SRCFILES := $(SRCDIR)/libco/libco.c \
            $(SRCDIR)/processor/arm/arm.cpp \
            $(SRCDIR)/processor/gsu/gsu.cpp \
            $(SRCDIR)/processor/hg51b/hg51b.cpp \
            $(SRCDIR)/processor/lr35902/lr35902.cpp \
            $(SRCDIR)/processor/r65816/r65816.cpp \
            $(SRCDIR)/processor/spc700/spc700.cpp \
            $(SRCDIR)/processor/upd96050/upd96050.cpp \
            $(SRCDIR)/sfc/interface/interface.cpp \
            $(SRCDIR)/sfc/system/system.cpp \
            $(SRCDIR)/sfc/controller/controller.cpp \
            $(SRCDIR)/sfc/cartridge/cartridge.cpp \
            $(SRCDIR)/sfc/cheat/cheat.cpp \
            $(SRCDIR)/sfc/memory/memory.cpp \
            $(SRCDIR)/sfc/chip/icd2/icd2.cpp \
            $(SRCDIR)/sfc/chip/bsx/bsx.cpp \
            $(SRCDIR)/sfc/slot/sufamiturbo/sufamiturbo.cpp \
            $(SRCDIR)/sfc/base/satellaview/satellaview.cpp \
            $(SRCDIR)/sfc/slot/satellaview/satellaview.cpp \
            $(SRCDIR)/sfc/chip/nss/nss.cpp \
            $(SRCDIR)/sfc/chip/event/event.cpp \
            $(SRCDIR)/sfc/chip/sa1/sa1.cpp \
            $(SRCDIR)/sfc/chip/superfx/superfx.cpp \
            $(SRCDIR)/sfc/chip/armdsp/armdsp.cpp \
            $(SRCDIR)/sfc/chip/hitachidsp/hitachidsp.cpp \
            $(SRCDIR)/sfc/chip/necdsp/necdsp.cpp \
            $(SRCDIR)/sfc/chip/epsonrtc/epsonrtc.cpp \
            $(SRCDIR)/sfc/chip/sharprtc/sharprtc.cpp \
            $(SRCDIR)/sfc/chip/spc7110/spc7110.cpp \
            $(SRCDIR)/sfc/chip/dsp1/dsp1.cpp \
            $(SRCDIR)/sfc/chip/dsp2/dsp2.cpp \
            $(SRCDIR)/sfc/chip/dsp3/dsp3.cpp \
            $(SRCDIR)/sfc/chip/dsp4/dsp4.cpp \
            $(SRCDIR)/sfc/chip/cx4/cx4.cpp \
            $(SRCDIR)/sfc/chip/st0010/st0010.cpp \
            $(SRCDIR)/sfc/chip/sgb-external/sgb-external.cpp \
            $(SRCDIR)/sfc/chip/sdd1/sdd1.cpp \
            $(SRCDIR)/sfc/chip/obc1/obc1.cpp \
            $(SRCDIR)/sfc/chip/hsu1/hsu1.cpp \
            $(SRCDIR)/sfc/chip/msu1/msu1.cpp \
            $(SRCDIR)/gb/interface/interface.cpp \
            $(SRCDIR)/gb/system/system.cpp \
            $(SRCDIR)/gb/scheduler/scheduler.cpp \
            $(SRCDIR)/gb/memory/memory.cpp \
            $(SRCDIR)/gb/cartridge/cartridge.cpp \
            $(SRCDIR)/gb/cpu/cpu.cpp \
            $(SRCDIR)/gb/ppu/ppu.cpp \
            $(SRCDIR)/gb/apu/apu.cpp \
            $(SRCDIR)/gb/cheat/cheat.cpp \
            $(SRCDIR)/gb/video/video.cpp \
            $(SRCDIR)/target-libretro/libretro.cpp

ifeq ($(profile),performance)
  COREFLAGS += -DPROFILE_PERFORMANCE
  SRCFILES += $(SRCDIR)/sfc/alt/cpu/cpu.cpp \
              $(SRCDIR)/sfc/alt/smp/smp.cpp \
              $(SRCDIR)/sfc/alt/dsp/dsp.cpp \
              $(SRCDIR)/sfc/alt/ppu-performance/ppu.cpp
else ifeq ($(profile),balanced)
  COREFLAGS += -DPROFILE_BALANCED
  SRCFILES += $(SRCDIR)/sfc/cpu/cpu.cpp \
              $(SRCDIR)/sfc/smp/smp.cpp \
              $(SRCDIR)/sfc/alt/dsp/dsp.cpp \
              $(SRCDIR)/sfc/alt/ppu-balanced/ppu.cpp
else ifeq ($(profile),accuracy) # If you're batshit insane
  COREFLAGS += -DPROFILE_ACCURACY
  SRCFILES += $(SRCDIR)/sfc/cpu/cpu.cpp \
              $(SRCDIR)/sfc/smp/smp.cpp \
              $(SRCDIR)/sfc/dsp/dsp.cpp \
              $(SRCDIR)/sfc/ppu/ppu.cpp
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro_bsnes_mercury_$(profile)
LOCAL_SRC_FILES    := $(SRCFILES)
LOCAL_CPPFLAGS     := -std=gnu++11 $(COREFLAGS)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(SRCDIR)/target-libretro/link.T
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_SHARED_LIBRARY)
