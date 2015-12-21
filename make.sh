if [[ -z "$1" ]]; then
  #do not run these in parallel; there is only one object directory
  time ./make.sh accuracy
  time ./make.sh balanced
else
  set -o verbose
  make target=libretro profile=$1 clean
  make target=libretro profile=$1 pgo=instrument lto=yes -j8
  retroprofile out/bsnes_mercury_$1_libretro.so ~/smw.smc 3000
  retroprofile out/bsnes_mercury_$1_libretro.so ~/Desktop/minir/roms/kirby.sfc 1000
  retroprofile out/bsnes_mercury_$1_libretro.so ~/Desktop/minir/roms/yoshi.sfc 1000
  make target=libretro profile=$1 clean
  make target=libretro profile=$1 pgo=optimize lto=yes -j8
  rm obj/*
  mv out/bsnes_mercury_$1_libretro.so ./bsnes_$1_libretro.so
fi
