mv libco libretro-sdk
cd ..
diff -ru higan_v094-source bsneshle094 > bsneshle094/changes.diff
cd bsneshle094
mv libretro-sdk libco
diff -u ../bsnes-libretro/target-libretro/libretro.cpp target-libretro/libretro.cpp > changes-libretro.diff
