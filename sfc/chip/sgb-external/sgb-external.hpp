struct SGBExternal : Coprocessor, library {
  unsigned revision;

  static void Enter();
  void enter();

  bool load_library(const char * name);

  void init();
  void load();
  void unload();
  void power();
  void reset();

  uint8 read(unsigned addr);
  void write(unsigned addr, uint8 data);

  void serialize(serializer&);

private:
  function<void (uint8_t*, unsigned)> sgb_rom;
  function<void (uint8_t*, unsigned)> sgb_ram;
  function<void (uint8_t*, unsigned)> sgb_rtc;
  function<bool (bool)> sgb_init;
  function<void ()> sgb_term;
  function<void ()> sgb_power;
  function<void ()> sgb_reset;
  function<void (unsigned)> sgb_row;
  function<uint8 (uint16)> sgb_read;
  function<void (uint16, uint8)> sgb_write;
  function<unsigned (uint32_t*, unsigned)> sgb_run;
  function<void ()> sgb_save;
  function<void (serializer&)> sgb_serialize;

  unsigned row;
  uint32_t samplebuffer[4096];
};

extern SGBExternal sgbExternal;
