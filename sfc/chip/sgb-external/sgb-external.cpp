#include <sfc/sfc.hpp>

#define SGBEXTERNAL_CPP
namespace SuperFamicom {

SGBExternal sgbExternal;

#include "serialization.cpp"

bool SGBExternal::load_library(const char * path) {
  if(open_absolute(path)) {
    void* symbol;
#define SYM(x) symbol=sym(#x); if (!symbol) return false; x=symbol;
    SYM(sgb_rom);
    SYM(sgb_ram);
    SYM(sgb_rtc);
    SYM(sgb_init);
    SYM(sgb_term);
    SYM(sgb_power);
    SYM(sgb_reset);
    SYM(sgb_row);
    SYM(sgb_read);
    SYM(sgb_write);
    SYM(sgb_run);
    SYM(sgb_save);
    SYM(sgb_serialize);
#undef SYM
    return true;
  }
  return false;
}

void SGBExternal::Enter() { sgbExternal.enter(); }

void SGBExternal::enter() {
  while(true) {
    if(scheduler.sync == Scheduler::SynchronizeMode::All) {
      scheduler.exit(Scheduler::ExitReason::SynchronizeEvent);
    }

    unsigned samples = sgb_run(samplebuffer, 16);
    for(unsigned i = 0; i < samples; i++) {
      int16 left  = samplebuffer[i] >>  0;
      int16 right = samplebuffer[i] >> 16;

      //SNES audio is notoriously quiet; lower Game Boy samples to match SGB sound effects
      audio.coprocessor_sample(left / 3, right / 3);
    }

    step(samples);
    synchronize_cpu();
  }
}

void SGBExternal::init() {
}

void SGBExternal::load() {
}

void SGBExternal::unload() {
  sgb_term();
}

void SGBExternal::power() {
  unsigned frequency = (revision == 1 ? system.cpu_frequency() / 10 : 2097152);
  create(SGBExternal::Enter, frequency);

  audio.coprocessor_enable(true);
  audio.coprocessor_frequency(revision == 1 ? 2147727.0 : 2097152.0);

  sgb_rom(GameBoy::cartridge.romdata, GameBoy::cartridge.romsize);
  sgb_ram(GameBoy::cartridge.ramdata, GameBoy::cartridge.ramsize);
  sgb_rtc(NULL, 0);

  bool version = (revision == 1) ? 0 : 1;
  if(sgb_init) sgb_init(version);
  if(sgb_power) sgb_power();
}

void SGBExternal::reset() {
  sgb_reset();
}

uint8 SGBExternal::read(unsigned addr) {
  if ((addr&0xFFFF)==0x7800)
  {
    static int x=319;
    static int y=11;
    x++;
    if (x==320)
    {
      x=0;
      sgb_row(y++);
//printf("%.6x\n",(unsigned)      cpu.status.wram_addr);
      if (y==18) y=0;
    }
  }
  return sgb_read(addr);
}

void SGBExternal::write(unsigned addr, uint8 data) {
  sgb_write(addr, data);
}

}
