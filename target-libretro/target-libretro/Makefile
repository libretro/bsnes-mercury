processors := arm hg51b upd96050 gsu r65816 spc700 lr35902
include processor/Makefile

include sfc/Makefile
include gb/Makefile
output := libretro

ifeq ($(platform),unix)
  flags += -fPIC
else ifeq ($(platform),osx)
  flags += -fPIC
else ifeq ($(platform),ios)
  flags += -arch armv7 -marm -fPIC -isysroot $(IOSSDK) -DHAVE_POSIX_MEMALIGN=1 -w
endif

flags += -D__LIBRETRO__

#rules
objects += libretro
objects := $(patsubst %,obj/%.o,$(objects))

obj/libretro.o: $(ui)/libretro.cpp $(ui)/*

#targets
build: $(objects)
ifeq ($(platform),unix)
	$(compiler) -o out/bsnes_libretro.so -shared $(objects) -ldl -Wl,--no-undefined -Wl,--version-script=$(ui)/link.T
else ifeq ($(platform),ios)
	$(compiler) -o out/bsnes_libretro_ios.dylib -dynamiclib $(objects) -isysroot $(IOSSDK) -arch armv7
else ifeq ($(platform),osx)
	$(compiler) -o out/bsnes_libretro.dylib -dynamiclib $(objects)
else ifeq ($(platform),win)
	$(compiler) -o out/bsnes_libretro.dll -shared $(objects) -Wl,--no-undefined -Wl,--version-script=$(ui)/link.T -static-libgcc -static-libstdc++ -lws2_32
endif

install:
ifeq ($(platform),unix)
	install -D -m 755 out/bsnes_libretro.so $(DESTDIR)$(prefix)/lib/bsnes_libretro.so
else ifeq ($(platform),osx)
	cp out/bsnes_libretro.dylib $(DESTDIR)$(prefix)/lib/bsnes_libretro.dylib
else ifeq ($(platform),ios)
	cp out/bsnes_libretro_ios.dylib $(DESTDIR)$(prefix)/lib/bsnes_libretro_ios.dylib
endif

uninstall:
ifeq ($(platform),x)
	rm $(DESTDIR)$(prefix)/lib/bsnes_libretro.so
else ifeq ($(platform),osx)
	rm /usr/local/lib/bsnes_libretro.dylib
else ifeq ($(platform),ios)
	rm /usr/local/lib/bsnes_libretro_ios.dylib
endif

