mv libco libretro-sdk
cd ..
diff -rup higan_v096-source bsneshle096 > bsneshle096/changes.diff
cd bsneshle096
mv libretro-sdk libco
diff -up ../bsnes-libretro/target-libretro/libretro.cpp target-libretro/libretro.cpp > changes-libretro.diff
