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

# options += debugger
# arch := x86
# console := true

# compiler

ifneq ($(debug),)
  flags := -I. -O0 -g
else
  flags := -I. -O3 -fomit-frame-pointer
endif

cflags := -std=gnu99 -xc
cppflags := -std=gnu++0x

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
	-@$(call delete,obj/*.o)
	-@$(call delete,obj/*.a)
	-@$(call delete,obj/*.so)
	-@$(call delete,obj/*.dylib)
	-@$(call delete,obj/*.dll)
	-@$(call delete,*.res)
	-@$(call delete,*.manifest)

archive:
	if [ -f higan.tar.xz ]; then rm higan.tar.xz; fi
	tar -cJf higan.tar.xz `ls`

sync:
ifeq ($(shell id -un),byuu)
	if [ -d ./libco ]; then rm -r ./libco; fi
	if [ -d ./nall ]; then rm -r ./nall; fi
	if [ -d ./ruby ]; then rm -r ./ruby; fi
	if [ -d ./phoenix ]; then rm -r ./phoenix; fi
	cp -r ../libco ./libco
	cp -r ../nall ./nall
	cp -r ../ruby ./ruby
	cp -r ../phoenix ./phoenix
	rm -r libco/doc
	rm -r libco/.test
	rm -r nall/.test
	rm -r ruby/.test
	rm -r phoenix/.test
endif

help:;

#this must be last because other things may have altered $(flags)
ifneq ($(lto),)
  flags += -flto
  link += $(flags)
endif
