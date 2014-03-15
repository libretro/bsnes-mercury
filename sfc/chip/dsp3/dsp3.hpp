class DSP3 : public Memory {
public:
  void init();
  void load();
  void unload();
  void power();
  void reset();

  uint8 read(unsigned addr);
  void write(unsigned addr, uint8 data);
};

extern DSP3 dsp3;
