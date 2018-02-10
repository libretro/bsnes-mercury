include nall/Makefile

ifeq ($(platform),win)
  override platform = windows
else ifeq ($(platform),mingw)
  override platform = windows
else ifeq ($(platform),osx)
  override platform = macosx
else ifeq ($(platform),unix)
  override platform = linux
else ifeq ($(platform),x)
  override platform = linux
endif

fc  := fc
sfc := sfc
gb  := gb
gba := gba

profile := accuracy
target  := libretro

# SFC input lag fix
sfc_lagfix := 1

# options += debugger
# arch := x86
# console := true

# compiler

ifeq ($(DEBUG), 1)
  flags := -I. -O0 -g
else
  flags := -I. -O3 -fomit-frame-pointer
endif

cflags := $(CPPFLAGS) $(CFLAGS) -std=gnu99 -xc
cppflags := $(CPPFLAGS) $(CXXFLAGS) -std=gnu++0x

objects := libco

# profile-guided optimization mode
# pgo := instrument
# pgo := optimize

ifeq ($(pgo),instrument)
  flags += -fprofile-generate
  link += -lgcov
else ifeq ($(pgo),optimize)
  flags += -fprofile-use
endif

ifeq ($(compiler),)
  ifneq ($(CXX),)
    compiler := $(CXX)
  else
    compiler := g++
  endif
endif

# platform
ui := target-$(target)

ifeq ($(findstring libretro,$(ui)),)
  ifeq ($(platform),windows)
    ifeq ($(arch),x86)
      flags += -m32
      link += -m32
    endif
    ifeq ($(console),true)
      link += -mconsole
    else
      link += -mwindows
    endif
    link += -mthreads -luuid -lkernel32 -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lole32 -lws2_32
    link += -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc
  else ifeq ($(platform),macosx)
    flags += -march=native
  else ifeq ($(platform),linux)
    flags += -march=native
    link += -Wl,-export-dynamic -lX11 -lXext -ldl
  else ifeq ($(platform),bsd)
    flags += -march=native
    link += -Wl,-export-dynamic -lX11 -lXext
  else ifeq ($(platform),haiku)
    flags += -march=native
    link += -Wl,-export-dynamic -lX11 -lXext -lroot
  else
    $(error unsupported platform.)
  endif
endif

ifeq ($(platform),macosx)
   ifndef ($(NOUNIVERSAL))
      flags += $(ARCHFLAGS)
      link += $(ARCHFLAGS)
   endif
endif


# implicit rules
compile-profile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(compiler) $(cflags) $(flags) $(profflags) $1 -c $< -o $@, \
      $(if $(filter %.cpp,$<), \
        $(compiler) $(cppflags) $(flags) $(profflags) $1 -c $< -o $@ \
      ) \
    ) \
  )

%-$(profile).o: $<; $(call compile-profile)

compile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(compiler) $(cflags) $(flags) $1 -c $< -o $@, \
      $(if $(filter %.cpp,$<), \
        $(compiler) $(cppflags) $(flags) $1 -c $< -o $@ \
      ) \
    ) \
  )

%.o: $<; $(call compile)

all: build;

obj/libco.o: libco/libco.c libco/*

include $(ui)/Makefile
flags := $(flags) $(foreach o,$(call strupper,$(options)),-D$o)

# targets
clean:
	-$(call delete,obj/*.o)
	-$(call delete,obj/*.a)
	-$(call delete,out/*.so)
	-$(call delete,out/*.dylib)
	-$(call delete,out/*.dll)

help:;

#this must be last because other things may have altered $(flags)
ifneq ($(lto),)
  flags += -flto
  link += $(flags)
endif
