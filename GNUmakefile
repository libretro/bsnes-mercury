include nall/GNUmakefile

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
target := libretro
# console := true

# compiler
ifneq ($(debug),)
  flags := -I. -Ilibco -O0 -g
else
  flags := -I. -Ilibco -O3 -fomit-frame-pointer
endif
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

ui := target-$(target)

# platform
ifeq ($(findstring libretro,$(ui)),)
  ifeq ($(platform),windows)
    ifeq ($(console),true)
      link += -mconsole
    else
      link += -mwindows
    endif
    link += -mthreads -luuid -lkernel32 -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32
    link += -Wl,-enable-auto-import
    link += -Wl,-enable-runtime-pseudo-reloc
  else ifeq ($(platform),macosx)
    flags += -march=native
  else ifeq ($(platform),linux)
    flags += -march=native -fopenmp
    link += -fopenmp
    link += -Wl,-export-dynamic
    link += -lX11 -lXext
  else ifeq ($(platform),bsd)
    flags += -march=native -fopenmp
    link += -fopenmp
    link += -Wl,-export-dynamic
    link += -lX11 -lXext
  else
    $(error unsupported platform.)
  endif
endif

ifeq ($(platform),osx)
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

include $(ui)/GNUmakefile
flags := $(flags) $(foreach o,$(call strupper,$(options)),-D$o)

# targets
clean:
	-@$(call delete,out/*)
	-@$(call delete,obj/*.o)
	-@$(call delete,obj/*.a)
	-@$(call delete,obj/*.so)
	-@$(call delete,obj/*.dylib)
	-@$(call delete,obj/*.dll)

archive:
	if [ -f higan.tar.xz ]; then rm higan.tar.xz; fi
	tar -cJf higan.tar.xz `ls`

sync:
ifeq ($(shell id -un),byuu)
	if [ -d ./libco ]; then rm -r ./libco; fi
	if [ -d ./nall ]; then rm -r ./nall; fi
	if [ -d ./ruby ]; then rm -r ./ruby; fi
	if [ -d ./hiro ]; then rm -r ./hiro; fi
	cp -r ../libco ./libco
	cp -r ../nall ./nall
	cp -r ../ruby ./ruby
	cp -r ../hiro ./hiro
	rm -r libco/doc
	rm -r libco/-test
	rm -r nall/-test
	rm -r ruby/-test
	rm -r hiro/-test
endif

help:;

#this must be last because other things may have altered $(flags)
ifneq ($(lto),)
  flags += -flto
  link += $(flags)
endif
