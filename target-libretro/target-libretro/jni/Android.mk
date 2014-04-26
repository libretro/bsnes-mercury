LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),arm)
  LOCAL_CFLAGS += -DANDROID_ARM
  LOCAL_ARM_MODE := arm
endif

ifeq ($(TARGET_ARCH),x86)
  LOCAL_CFLAGS += -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
  LOCAL_CFLAGS += -DANDROID_MIPS
endif

SRCDIR := $(LOCAL_PATH)/../..

ifeq ($(profile),)
  profile := performance
endif

ifeq ($(profile),performance)
  LOCAL_CFLAGS += -DPROFILE_PERFORMANCE
  LOCAL_SRC_FILES += $(SRCDIR)/sfc/alt/cpu/cpu.cpp \
							$(SRCDIR)/sfc/alt/smp/smp.cpp \
							$(SRCDIR)/sfc/alt/dsp/dsp.cpp \
							$(SRCDIR)/sfc/alt/ppu-performance/ppu.cpp
  LOCAL_MODULE := retro_bsnes_performance
else ifeq ($(profile),balanced)
  LOCAL_CFLAGS += -DPROFILE_BALANCED
  LOCAL_SRC_FILES += $(SRCDIR)/sfc/cpu/cpu.cpp \
							$(SRCDIR)/sfc/smp/smp.cpp \
							$(SRCDIR)/sfc/alt/dsp/dsp.cpp \
							$(SRCDIR)/sfc/alt/ppu-balanced/ppu.cpp
  LOCAL_MODULE := retro_bsnes_balanced
else ifeq ($(profile),accuracy) # If you're batshit insane
  LOCAL_CFLAGS += -DPROFILE_ACCURACY
  LOCAL_SRC_FILES += $(SRCDIR)/sfc/cpu/cpu.cpp \
							$(SRCDIR)/sfc/smp/smp.cpp \
							$(SRCDIR)/sfc/dsp/dsp.cpp \
							$(SRCDIR)/sfc/ppu/ppu.cpp
  LOCAL_MODULE := retro_bsnes_accuracy
endif

LOCAL_SRC_FILES += $(SRCDIR)/libco/libco.c \
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

LOCAL_CPPFLAGS += -std=gnu++11 -fexceptions -frtti -Wno-literal-suffix 
LOCAL_CFLAGS += -O3 -fomit-frame-pointer -ffast-math -D__LIBRETRO__
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../..

include $(BUILD_SHARED_LIBRARY)

