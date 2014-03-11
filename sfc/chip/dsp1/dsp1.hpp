#include "dsp1emu.hpp"

class DSP1 {
public:
  void init();
  void load();
  void unload();
  void power();
  void reset();

  uint8 read(unsigned addr);
  void write(unsigned addr, uint8 data);

  void serialize(serializer&);

  unsigned Select;

private:
  Dsp1 dsp1;
};

extern DSP1 dsp1;
