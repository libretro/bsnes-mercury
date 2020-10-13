LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/../..

ifeq ($(PROFILE),)
  PROFILE := performance
endif

COREFLAGS := -fomit-frame-pointer -ffast-math -D__LIBRETRO__

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

ifeq ($(PROFILE),performance)
  COREFLAGS += -DPROFILE_PERFORMANCE
else ifeq ($(PROFILE),balanced)
  COREFLAGS += -DPROFILE_BALANCED
else ifeq ($(PROFILE),accuracy) # If you're batshit insane
  COREFLAGS += -DPROFILE_ACCURACY
endif

include $(CORE_DIR)/Makefile.common

COREFLAGS += $(INCFLAGS)

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CPPFLAGS     := -std=c++11 $(COREFLAGS)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/target-libretro/link.T
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_SHARED_LIBRARY)
