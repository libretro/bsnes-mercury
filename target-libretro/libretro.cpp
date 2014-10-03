#include "libretro.h"
#include <sfc/sfc.hpp>
#include <nall/stream/mmap.hpp>
#include <nall/stream/file.hpp>
#include "../ananke/heuristics/super-famicom.hpp"
#include "../ananke/heuristics/game-boy.hpp"
#include <string>

// Special memory types.
#define RETRO_MEMORY_SNES_BSX_RAM             ((1 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_SNES_BSX_PRAM            ((2 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM  ((3 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM  ((4 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_SNES_GAME_BOY_RAM        ((5 << 8) | RETRO_MEMORY_SAVE_RAM)
#define RETRO_MEMORY_SNES_GAME_BOY_RTC        ((6 << 8) | RETRO_MEMORY_RTC)

// Special game types passed into retro_load_game_special().
// Only used when multiple ROMs are required.
#define RETRO_GAME_TYPE_BSX             0x101
#define RETRO_GAME_TYPE_BSX_SLOTTED     0x102
#define RETRO_GAME_TYPE_SUFAMI_TURBO    0x103
#define RETRO_GAME_TYPE_SUPER_GAME_BOY  0x104

#define RETRO_DEVICE_JOYPAD_MULTITAP       RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 0)
#define RETRO_DEVICE_LIGHTGUN_JUSTIFIER    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 1)
#define RETRO_DEVICE_LIGHTGUN_JUSTIFIERS   RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2)

using namespace nall;

const uint8 iplrom[64] = {
/*ffc0*/  0xcd, 0xef,        //mov   x,#$ef
/*ffc2*/  0xbd,              //mov   sp,x
/*ffc3*/  0xe8, 0x00,        //mov   a,#$00
/*ffc5*/  0xc6,              //mov   (x),a
/*ffc6*/  0x1d,              //dec   x
/*ffc7*/  0xd0, 0xfc,        //bne   $ffc5
/*ffc9*/  0x8f, 0xaa, 0xf4,  //mov   $f4,#$aa
/*ffcc*/  0x8f, 0xbb, 0xf5,  //mov   $f5,#$bb
/*ffcf*/  0x78, 0xcc, 0xf4,  //cmp   $f4,#$cc
/*ffd2*/  0xd0, 0xfb,        //bne   $ffcf
/*ffd4*/  0x2f, 0x19,        //bra   $ffef
/*ffd6*/  0xeb, 0xf4,        //mov   y,$f4
/*ffd8*/  0xd0, 0xfc,        //bne   $ffd6
/*ffda*/  0x7e, 0xf4,        //cmp   y,$f4
/*ffdc*/  0xd0, 0x0b,        //bne   $ffe9
/*ffde*/  0xe4, 0xf5,        //mov   a,$f5
/*ffe0*/  0xcb, 0xf4,        //mov   $f4,y
/*ffe2*/  0xd7, 0x00,        //mov   ($00)+y,a
/*ffe4*/  0xfc,              //inc   y
/*ffe5*/  0xd0, 0xf3,        //bne   $ffda
/*ffe7*/  0xab, 0x01,        //inc   $01
/*ffe9*/  0x10, 0xef,        //bpl   $ffda
/*ffeb*/  0x7e, 0xf4,        //cmp   y,$f4
/*ffed*/  0x10, 0xeb,        //bpl   $ffda
/*ffef*/  0xba, 0xf6,        //movw  ya,$f6
/*fff1*/  0xda, 0x00,        //movw  $00,ya
/*fff3*/  0xba, 0xf4,        //movw  ya,$f4
/*fff5*/  0xc4, 0xf4,        //mov   $f4,a
/*fff7*/  0xdd,              //mov   a,y
/*fff8*/  0x5d,              //mov   x,a
/*fff9*/  0xd0, 0xdb,        //bne   $ffd6
/*fffb*/  0x1f, 0x00, 0x00,  //jmp   ($0000+x)
/*fffe*/  0xc0, 0xff         //reset vector location ($ffc0)
};

static void retro_log_default(enum retro_log_level level, const char *fmt, ...)
{
  fprintf(stderr, "[bsnes]: ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}
static retro_log_printf_t output;

static const char * read_opt(const char * name, const char * defval);

struct Callbacks : Emulator::Interface::Bind {
  retro_video_refresh_t pvideo_refresh;
  retro_audio_sample_batch_t paudio;
  retro_input_poll_t pinput_poll;
  retro_input_state_t pinput_state;
  retro_environment_t penviron;
  bool overscan;
  bool manifest;

  bool load_request_error;
  const uint8_t *rom_data;
  unsigned rom_size;
  const uint8_t *gb_rom_data;
  unsigned gb_rom_size;
  string xmlrom;
  string xmlrom_gb;

  void *sram;
  unsigned sram_size;

  Emulator::Interface *iface;
  string basename;

  static unsigned snes_to_retro(unsigned device) {
    switch ((SuperFamicom::Input::Device)device) {
       default:
       case SuperFamicom::Input::Device::None:       return RETRO_DEVICE_NONE;
       case SuperFamicom::Input::Device::Joypad:     return RETRO_DEVICE_JOYPAD;
       case SuperFamicom::Input::Device::Multitap:   return RETRO_DEVICE_JOYPAD_MULTITAP;
       case SuperFamicom::Input::Device::Mouse:      return RETRO_DEVICE_MOUSE;
       case SuperFamicom::Input::Device::SuperScope: return RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE;
       case SuperFamicom::Input::Device::Justifier:  return RETRO_DEVICE_LIGHTGUN_JUSTIFIER;
       case SuperFamicom::Input::Device::Justifiers: return RETRO_DEVICE_LIGHTGUN_JUSTIFIERS;
    }
  }

  // TODO: Properly map Mouse/Lightguns.
  static unsigned snes_to_retro(unsigned, unsigned id) {
    return id;
  }

  static SuperFamicom::Input::Device retro_to_snes(unsigned device) {
    switch (device) {
       default:
       case RETRO_DEVICE_NONE:                 return SuperFamicom::Input::Device::None;
       case RETRO_DEVICE_JOYPAD:               return SuperFamicom::Input::Device::Joypad;
       case RETRO_DEVICE_ANALOG:               return SuperFamicom::Input::Device::Joypad;
       case RETRO_DEVICE_JOYPAD_MULTITAP:      return SuperFamicom::Input::Device::Multitap;
       case RETRO_DEVICE_MOUSE:                return SuperFamicom::Input::Device::Mouse;
       case RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE: return SuperFamicom::Input::Device::SuperScope;
       case RETRO_DEVICE_LIGHTGUN_JUSTIFIER:   return SuperFamicom::Input::Device::Justifier;
       case RETRO_DEVICE_LIGHTGUN_JUSTIFIERS:  return SuperFamicom::Input::Device::Justifiers;
    }
  }

  uint32_t video_buffer[512 * 480];

  void videoRefresh(const uint32_t* palette, const uint32_t* data, unsigned pitch, unsigned width, unsigned height) override {
    if (!overscan) {
      data += 8 * 1024;

      if (height == 240)
        height = 224;
      else if (height == 480)
        height = 448;
    }

    uint32_t *ptr = video_buffer;
    for (unsigned y = 0; y < height; y++, data += pitch >> 2, ptr += width)
       for (unsigned x = 0; x < width; x++)
          ptr[x] = palette[data[x]];

    pvideo_refresh(video_buffer, width, height, width*sizeof(uint32_t));
    pinput_poll();
  }

  int16_t sampleBuf[128];
  unsigned int sampleBufPos;

  void audioSample(int16_t left, int16_t right) override {
    sampleBuf[sampleBufPos++] = left;
    sampleBuf[sampleBufPos++] = right;
    if(sampleBufPos==128) {
      paudio(sampleBuf, 64);
      sampleBufPos = 0;
    }
  }

  int16_t inputPoll(unsigned port, unsigned device, unsigned id) override {
    if(id > 11) return 0;
    return pinput_state(port, snes_to_retro(device), 0, snes_to_retro(device, id));
  }

  void saveRequest(unsigned id, string p) override {
    if (manifest) {
      output(RETRO_LOG_INFO, "[Save]: ID %u, Request \"%s\".\n", id, (const char*)p);
      string save_path = {path(0), p};
      filestream stream(save_path, file::mode::write);
      iface->save(id, stream);
    }
  }

  void loadFile(unsigned id, string p) {
    // Look for BIOS in system directory as well.
    const char *dir = 0;
    penviron(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir);

    string load_path = {path(0), p};
    if(manifest || file::exists(load_path)) {
      filestream stream(load_path, file::mode::read);
      iface->load(id, stream);
    } else if(dir) {
      load_path = {dir, "/", p};
      if(file::exists(load_path)) {
        mmapstream stream(load_path);
        iface->load(id, stream);
      } else {
        output(RETRO_LOG_ERROR, "Cannot find requested file in system directory: \"%s\".\n", (const char*)load_path);
        load_request_error = true;
      }
    } else {
      output(RETRO_LOG_ERROR, "Cannot find requested file: \"%s\" in ROM directory nor system directory.\n", (const char*)p);
      load_request_error = true;
    }
  }

  void loadROM(unsigned id) {
    memorystream stream(rom_data, rom_size);
    iface->load(id, stream);
  }

  void loadManifest(unsigned id) {
    memorystream stream((const uint8_t*)(const char*)xmlrom, xmlrom.length());
    iface->load(id, stream);
  }

  void loadSGBROMManifest(unsigned id) {
    memorystream stream((const uint8_t*)(const char*)xmlrom_gb, xmlrom_gb.length());
    iface->load(SuperFamicom::ID::SuperGameBoyManifest, stream);
  }

  void loadSGBROM(unsigned id) {
    memorystream stream(gb_rom_data, gb_rom_size);
    iface->load(id, stream);
  }

  void loadIPLROM(unsigned id) {
    memorystream stream(iplrom, sizeof(iplrom));
    iface->load(id, stream);
  }

  void loadRequestManifest(unsigned id, const string& p) {
    output(RETRO_LOG_INFO, "[Manifest]: ID %u, Request \"%s\".\n", id, (const char*)p);
    switch(id) {
      case SuperFamicom::ID::IPLROM:
        loadIPLROM(id);
        break;

      case SuperFamicom::ID::Manifest:
        loadManifest(id);
        break;

      default:
        loadFile(id, p);
        break;
    }
  }

  void loadRequestMemory(unsigned id, const string& p) {
    output(RETRO_LOG_INFO, "[Memory]: ID %u, Request \"%s\".\n", id, (const char*)p);
    switch(id) {
      case SuperFamicom::ID::Manifest:
        loadManifest(id);
        break;

      case SuperFamicom::ID::SuperGameBoyManifest:
        loadSGBROMManifest(id);
        break;

      case SuperFamicom::ID::ROM:
      case SuperFamicom::ID::SuperFXROM:
      case SuperFamicom::ID::SA1ROM:
      case SuperFamicom::ID::SDD1ROM:
      case SuperFamicom::ID::HitachiDSPROM:
      case SuperFamicom::ID::SPC7110PROM:
        output(RETRO_LOG_INFO, "Load ROM.\n");
        loadROM(id);
        break;

      case SuperFamicom::ID::SuperGameBoyROM:
        loadSGBROM(id);
        break;

      // SRAM. Have to special case for all chips ...
      case SuperFamicom::ID::RAM:
        sram = SuperFamicom::cartridge.ram.data();
        sram_size = SuperFamicom::cartridge.ram.size();
        break;
      case SuperFamicom::ID::SuperFXRAM:
        sram = SuperFamicom::superfx.ram.data();
        sram_size = SuperFamicom::superfx.ram.size();
        break;
      case SuperFamicom::ID::SA1BWRAM:
        sram = SuperFamicom::sa1.bwram.data();
        sram_size = SuperFamicom::sa1.bwram.size();
        break;
      case SuperFamicom::ID::SDD1RAM:
        sram = SuperFamicom::sdd1.ram.data();
        sram_size = SuperFamicom::sdd1.ram.size();
        break;
      case SuperFamicom::ID::OBC1RAM:
        sram = SuperFamicom::obc1.ram.data();
        sram_size = SuperFamicom::obc1.ram.size();
        break;
      case SuperFamicom::ID::HitachiDSPRAM:
        sram = SuperFamicom::hitachidsp.ram.data();
        sram_size = SuperFamicom::hitachidsp.ram.size();
        break;
      case SuperFamicom::ID::ArmDSPRAM:
        sram = SuperFamicom::armdsp.programRAM;
        sram_size = 16 * 1024 * sizeof(SuperFamicom::armdsp.programRAM[0]);
        break;
      case SuperFamicom::ID::Nec96050DSPRAM:
        sram = SuperFamicom::necdsp.dataRAM;
        sram_size = sizeof(SuperFamicom::necdsp.dataRAM);
        break;
      case SuperFamicom::ID::SPC7110RAM:
        sram = SuperFamicom::spc7110.ram.data();
        sram_size = SuperFamicom::spc7110.ram.size();
        break;

      // SGB RAM is handled explicitly.
      case SuperFamicom::ID::SuperGameBoyRAM:
        break;

      case SuperFamicom::ID::IPLROM:
        loadIPLROM(id);
        break;

      default:
        output(RETRO_LOG_INFO, "Load BIOS.\n");
        loadFile(id, p);
        break;
    }
  }

  void loadRequest(unsigned id, string p) override {
    if (manifest)
       loadRequestManifest(id, p);
    else
       loadRequestMemory(id, p);
    output(RETRO_LOG_INFO, "Complete load request.\n");
  }

  void loadRequest(unsigned id, string p, string manifest) override {
    switch (id) {
      case SuperFamicom::ID::SuperGameBoy:
        output(RETRO_LOG_INFO, "Loading GB ROM.\n");
        loadSGBROMManifest(id);
        break;

      default:
        output(RETRO_LOG_INFO, "Didn't do anything with loadRequest (3 arg).\n");
    }
  }

  string path(unsigned) override {
    return string(basename);
  }

  uint32_t videoColor(unsigned, uint16_t, uint16_t r, uint16_t g, uint16_t b) override {
    r >>= 8;
    g >>= 8;
    b >>= 8;
    return (r << 16) | (g << 8) | (b << 0);
  }

  void notify(string text) {
    output(RETRO_LOG_ERROR, "%s\n", (const char*)text);
  }

  unsigned altImplementation(unsigned item) override {
    if (item==SuperFamicom::Alt::ForDSP)
    {
      if (!strcmp(read_opt("bsnes_chip_hle", "LLE"), "HLE")) return SuperFamicom::Alt::DSP::HLE;
      else return SuperFamicom::Alt::DSP::LLE;
    }
#ifdef EXPERIMENTAL_FEATURES
    if (item==SuperFamicom::Alt::ForSuperGameBoy)
    {
      if (!strcmp(read_opt("bsnes_sgb_core", "Internal"), "Gambatte")) return SuperFamicom::Alt::SuperGameBoy::External;
      else return SuperFamicom::Alt::SuperGameBoy::Internal;
    }
#endif
    return 0;
  }
};

static Callbacks core_bind;

static const char * read_opt(const char * name, const char * defval)
{
	struct retro_variable allowvar = { "bsnes_violate_accuracy", "No" };
	core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&allowvar);
	if (!strcmp(allowvar.value, "Yes"))
	{
		struct retro_variable var = {name, defval};
		core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&var);
		return var.value;
	}
	else return defval;
}

struct Interface : public SuperFamicom::Interface {
  SuperFamicomCartridge::Mode mode;

  void setCheats(const lstring &list = lstring());

  Interface();

  void init() {
     SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Standard);
  }
};

struct GBInterface : public GameBoy::Interface {
  GBInterface() { bind = &core_bind; }
  void init() {
     SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Standard);
  }
};

static Interface core_interface;
static GBInterface core_gb_interface;

Interface::Interface() {
  bind = &core_bind;
  core_bind.iface = &core_interface;
}

void Interface::setCheats(const lstring &) {
#if 0
  if(core_interface.mode == SuperFamicomCartridge::ModeSuperGameBoy) {
    GameBoy::cheat.reset();
    for(auto &code : list) {
      lstring codelist;
      codelist.split("+", code);
      for(auto &part : codelist) {
        unsigned addr, data, comp;
        if(GameBoy::Cheat::decode(part, addr, data, comp)) {
          GameBoy::cheat.append({addr, data, comp});
        }
      }
    }
    GameBoy::cheat.synchronize();
    return;
  }

  SuperFamicom::cheat.reset();
  for(auto &code : list) {
    lstring codelist;
    codelist.split("+", code);
    for(auto &part : codelist) {
      unsigned addr, data;
      if(SuperFamicom::Cheat::decode(part, addr, data)) {
        SuperFamicom::cheat.append({addr, data});
      }
    }
  }

  SuperFamicom::cheat.synchronize();
#endif
}

unsigned retro_api_version(void) {
  return RETRO_API_VERSION;
}

static unsigned superfx_freq_orig;

void retro_set_environment(retro_environment_t environ_cb)
{
   core_bind.penviron = environ_cb;

   static const struct retro_variable vars[] = {
      { "bsnes_violate_accuracy", "Respect accuracy-impacting settings; No|Yes" },
      { "bsnes_chip_hle", "Special chip accuracy; LLE|HLE" },
      { "bsnes_superfx_overclock", "SuperFX speed; 100%|150%|200%|300%|400%|500%|1000%" },
         //Any integer is usable here, but there is no such thing as "any integer" in core options.
#ifdef EXPERIMENTAL_FEATURES
      { "bsnes_sgb_core", "Super Game Boy core; Internal|Gambatte" },
#endif
      { NULL, NULL },
   };
   core_bind.penviron(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);

   static struct retro_log_callback log={retro_log_default};
   core_bind.penviron(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, (void*)&log);
   output=log.log;

   static const retro_subsystem_memory_info sgb_memory[] = {
      { "srm", RETRO_MEMORY_SNES_GAME_BOY_RAM },
      { "rtc", RETRO_MEMORY_SNES_GAME_BOY_RTC },
   };

   static const struct retro_subsystem_rom_info sgb_roms[] = {
      { "GameBoy", "gb|gbc", false, false, true, sgb_memory, 2 },
      { "Super GameBoy BIOS", "sfc|smc", false, false, true, NULL, 0 },
   };

   static const retro_subsystem_memory_info sufami_a_memory[] = {
      { "srm", RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM },
   };

   static const retro_subsystem_memory_info sufami_b_memory[] = {
      { "srm", RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM },
   };

   static const struct retro_subsystem_rom_info sufami_roms[] = {
      { "Sufami A", "sfc|smc", false, false, false, sufami_a_memory, 1 },
      { "Sufami B", "sfc|smc", false, false, false, sufami_b_memory, 1 },
      { "Sufami BIOS", "sfc|smc", false, false, true, NULL, 0 },
   };

   static const retro_subsystem_memory_info bsx_memory[] = {
      { "srm", RETRO_MEMORY_SNES_BSX_RAM },
      { "psrm", RETRO_MEMORY_SNES_BSX_PRAM },
   };

   static const struct retro_subsystem_rom_info bsx_roms[] = {
      { "BSX ROM", "bs", false, false, true, bsx_memory, 2 },
      { "BSX BIOS", "sfc|smc", false, false, true, NULL, 0 },
   };

   // OR in 0x1000 on types to remain ABI compat with the older-style retro_load_game_special().
   static const struct retro_subsystem_info subsystems[] = {
      { "Super GameBoy", "sgb", sgb_roms, 2, RETRO_GAME_TYPE_SUPER_GAME_BOY | 0x1000 }, // Super Gameboy
      { "Sufami Turbo", "sufami", sufami_roms, 3, RETRO_GAME_TYPE_SUFAMI_TURBO | 0x1000 }, // Sufami Turbo
      { "BSX", "bsx", bsx_roms, 2, RETRO_GAME_TYPE_BSX | 0x1000 }, // BSX
      { "BSX slotted", "bsxslot", bsx_roms, 2, RETRO_GAME_TYPE_BSX_SLOTTED | 0x1000 }, // BSX slotted
      { NULL },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)subsystems);

   static const struct retro_controller_description port_1[] = {
      { "SNES Joypad", RETRO_DEVICE_JOYPAD },
      { "SNES Mouse", RETRO_DEVICE_MOUSE },
   };

   static const struct retro_controller_description port_2[] = {
      { "SNES Joypad", RETRO_DEVICE_JOYPAD },
      { "SNES Mouse", RETRO_DEVICE_MOUSE },
      { "Multitap", RETRO_DEVICE_JOYPAD_MULTITAP },
      { "SuperScope", RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE },
      { "Justifier", RETRO_DEVICE_LIGHTGUN_JUSTIFIER },
      { "Justifiers", RETRO_DEVICE_LIGHTGUN_JUSTIFIERS },
   };

   static const struct retro_controller_info ports[] = {
      { port_1, 2 },
      { port_2, 6 },
      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

static void update_variables(void) {
   if (SuperFamicom::cartridge.has_superfx()) {
      const char * speed=read_opt("bsnes_superfx_overclock", "100%");
      unsigned percent=strtoul(speed, NULL, 10);//we can assume that the input is one of our advertised options
      SuperFamicom::superfx.frequency=(uint64)superfx_freq_orig*percent/100;
   }
}

void retro_set_video_refresh(retro_video_refresh_t cb)           { core_bind.pvideo_refresh = cb; }
void retro_set_audio_sample(retro_audio_sample_t)                { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { core_bind.paudio         = cb; }
void retro_set_input_poll(retro_input_poll_t cb)                 { core_bind.pinput_poll    = cb; }
void retro_set_input_state(retro_input_state_t cb)               { core_bind.pinput_state   = cb; }

void retro_set_controller_port_device(unsigned port, unsigned device) {
  if (port < 2)
    SuperFamicom::input.connect(port, Callbacks::retro_to_snes(device));
}

void retro_init(void) {
  SuperFamicom::interface = &core_interface;
  GameBoy::interface = &core_gb_interface;

  core_interface.init();
  core_gb_interface.init();
  
  core_bind.sampleBufPos = 0;

  SuperFamicom::system.init();
  SuperFamicom::input.connect(SuperFamicom::Controller::Port1, SuperFamicom::Input::Device::Joypad);
  SuperFamicom::input.connect(SuperFamicom::Controller::Port2, SuperFamicom::Input::Device::Joypad);
}

void retro_deinit(void) {
  SuperFamicom::system.term();
}

void retro_reset(void) {
  SuperFamicom::system.reset();
}

void retro_run(void) {
  bool updated = false;
  if (core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
    update_variables();
  SuperFamicom::system.run();
  if(core_bind.sampleBufPos) {
    core_bind.paudio(core_bind.sampleBuf, core_bind.sampleBufPos/2);
    core_bind.sampleBufPos = 0;
  }
}

size_t retro_serialize_size(void) {
  return SuperFamicom::system.serialize_size();
}

bool retro_serialize(void *data, size_t size) {
  SuperFamicom::system.runtosave();
  serializer s = SuperFamicom::system.serialize();
  if(s.size() > size) return false;
  memcpy(data, s.data(), s.size());
  return true;
}

bool retro_unserialize(const void *data, size_t size) {
  serializer s((const uint8_t*)data, size);
  return SuperFamicom::system.unserialize(s);
}

#if 0
struct CheatList {
  bool enable;
  string code;
  CheatList() : enable(false) {}
};

static vector<CheatList> cheatList;
#endif

void retro_cheat_reset(void) {
#if 0
  cheatList.reset();
  core_interface.setCheats();
#endif
}

void retro_cheat_set(unsigned index, bool enable, const char *code) {
#if 0
  cheatList.reserve(index+1);
  cheatList[index].enable = enable;
  cheatList[index].code = code;
  lstring list;
  
  for(unsigned n = 0; n < cheatList.size(); n++) {
    if(cheatList[n].enable) list.append(cheatList[n].code);
  }
  
  core_interface.setCheats(list);
#endif
}

void retro_get_system_info(struct retro_system_info *info) {
  static string version("v", Emulator::Version, " (", Emulator::Profile, ")");
  info->library_name     = "bsnes-mercury";
  info->library_version  = version;
  info->valid_extensions = "sfc|smc|bml";
  info->need_fullpath    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
  struct retro_system_timing timing = { 0.0, 32040.5 };
  timing.fps = retro_get_region() == RETRO_REGION_NTSC ? 21477272.0 / 357366.0 : 21281370.0 / 425568.0;

  if (!core_bind.penviron(RETRO_ENVIRONMENT_GET_OVERSCAN, &core_bind.overscan))
     core_bind.overscan = false;

  unsigned base_width = 256;
  unsigned base_height = core_bind.overscan ? 240 : 224;
  struct retro_game_geometry geom = { base_width, base_height, base_width << 1, base_height << 1, 4.0 / 3.0 };

  info->timing   = timing;
  info->geometry = geom;

  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
  core_bind.penviron(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
}

static void output_multiline(enum retro_log_level level, char * data)
{
  while (true)
  {
    char* data_linebreak=strchr(data, '\n');
    if (data_linebreak) *data_linebreak='\0';
    if (*data) output(level, "%s\n", data);
    if (!data_linebreak) break;
    *data_linebreak='\n';
    data=data_linebreak+1;
  }
}

static bool snes_load_cartridge_normal(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size
) {
  string xmlrom = (rom_xml && *rom_xml) ? string(rom_xml) : SuperFamicomCartridge(rom_data, rom_size).markup;

  core_bind.rom_data = rom_data;
  core_bind.rom_size = rom_size;
  core_bind.xmlrom   = xmlrom;
  output(RETRO_LOG_INFO, "BML map:\n");
  output_multiline(RETRO_LOG_INFO, xmlrom.data());
  core_bind.iface->load(SuperFamicom::ID::SuperFamicom);
  SuperFamicom::system.power();
  return !core_bind.load_request_error;
}

static bool snes_load_cartridge_bsx_slotted(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *bsx_xml, const uint8_t *bsx_data, unsigned bsx_size
) {
#if 0
  string xmlrom = (rom_xml && *rom_xml) ? string(rom_xml) : SuperFamicomCartridge(rom_data, rom_size).markup;
  string xmlbsx = (bsx_xml && *bsx_xml) ? string(bsx_xml) : SuperFamicomCartridge(bsx_data, bsx_size).markup;

  SuperFamicom::bsxflash.memory.copy(memorystream(bsx_data, bsx_size));
  SuperFamicom::cartridge.load(xmlrom, memorystream(rom_data, rom_size));

  SuperFamicom::system.power();
  return !core_bind.load_request_error;
#endif
  return false;
}

static bool snes_load_cartridge_bsx(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *bsx_xml, const uint8_t *bsx_data, unsigned bsx_size
) {
#if 0
  string xmlrom = (rom_xml && *rom_xml) ? string(rom_xml) : SuperFamicomCartridge(rom_data, rom_size).markup;
  string xmlbsx = (bsx_xml && *bsx_xml) ? string(bsx_xml) : SuperFamicomCartridge(bsx_data, bsx_size).markup;

  SuperFamicom::bsxflash.memory.copy(memorystream(bsx_data, bsx_size));
  SuperFamicom::cartridge.load(xmlrom, memorystream(rom_data, rom_size));

  SuperFamicom::system.power();
  return !core_bind.load_request_error;
#endif
  return false;
}

static bool snes_load_cartridge_sufami_turbo(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *sta_xml, const uint8_t *sta_data, unsigned sta_size,
  const char *stb_xml, const uint8_t *stb_data, unsigned stb_size
) {
#if 0
  string xmlrom = (rom_xml && *rom_xml) ? string(rom_xml) : SuperFamicomCartridge(rom_data, rom_size).markup;
  string xmlsta = (sta_xml && *sta_xml) ? string(sta_xml) : SuperFamicomCartridge(sta_data, sta_size).markup;
  string xmlstb = (stb_xml && *stb_xml) ? string(stb_xml) : SuperFamicomCartridge(stb_data, stb_size).markup;

  if(sta_data) SuperFamicom::sufamiturbo.slotA.rom.copy(memorystream(sta_data, sta_size));
  if(stb_data) SuperFamicom::sufamiturbo.slotB.rom.copy(memorystream(stb_data, stb_size));
  SuperFamicom::cartridge.load(xmlrom, memorystream(rom_data, rom_size));

  SuperFamicom::system.power();
  return !core_bind.load_request_error;
#endif
  return false;
}

static bool snes_load_cartridge_super_game_boy(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *dmg_xml, const uint8_t *dmg_data, unsigned dmg_size
) {
  string xmlrom_sgb = (rom_xml && *rom_xml) ? string(rom_xml) : SuperFamicomCartridge(rom_data, rom_size).markup;
  string xmlrom_gb  = (dmg_xml && *dmg_xml) ? string(dmg_xml) : GameBoyCartridge((uint8_t*)dmg_data, dmg_size).markup;
  output(RETRO_LOG_INFO, "Markup SGB:\n");
  output_multiline(RETRO_LOG_INFO, xmlrom_sgb.data());
  output(RETRO_LOG_INFO, "Markup GB:\n");
  output_multiline(RETRO_LOG_INFO, xmlrom_gb.data());

  core_bind.rom_data    = rom_data;
  core_bind.rom_size    = rom_size;
  core_bind.gb_rom_data = dmg_data;
  core_bind.gb_rom_size = dmg_size;
  core_bind.xmlrom      = xmlrom_sgb;
  core_bind.xmlrom_gb   = xmlrom_gb;

  core_bind.iface->load(SuperFamicom::ID::SuperFamicom);
  core_bind.iface->load(SuperFamicom::ID::SuperGameBoy);
  SuperFamicom::system.power();
  return !core_bind.load_request_error;
}

bool retro_load_game(const struct retro_game_info *info) {
  // Support loading a manifest directly.
  core_bind.manifest = info->path && string(info->path).endsWith(".bml");

  const uint8_t *data = (const uint8_t*)info->data;
  size_t size = info->size;
  if ((size & 0x7ffff) == 512) {
    size -= 512;
    data += 512;
  }
  retro_cheat_reset();
  if (info->path) {
    core_bind.load_request_error = false;
    core_bind.basename = info->path;

    char *posix_slash = (char*)strrchr(core_bind.basename, '/');
    char *win_slash = (char*)strrchr(core_bind.basename, '\\');
    if (posix_slash && !win_slash)
       posix_slash[1] = '\0';
    else if (win_slash && !posix_slash)
       win_slash[1] = '\0';
    else if (posix_slash && win_slash)
       max(posix_slash, win_slash)[1] = '\0';
    else
      core_bind.basename = "./";
  }

  core_interface.mode = SuperFamicomCartridge::ModeNormal;
  std::string manifest;
  if (core_bind.manifest)
    manifest = std::string((const char*)info->data, info->size); // Might not be 0 terminated.
  
  bool ret=snes_load_cartridge_normal(core_bind.manifest ? manifest.data() : info->meta, data, size);
  SuperFamicom::bus.libretro_mem_map.reverse();
  retro_memory_map map={SuperFamicom::bus.libretro_mem_map.data(), SuperFamicom::bus.libretro_mem_map.size()};
  core_bind.penviron(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void*)&map);
  
  if (SuperFamicom::cartridge.has_superfx())
    superfx_freq_orig=SuperFamicom::superfx.frequency;
  
  return ret;
}


bool retro_load_game_special(unsigned game_type,
      const struct retro_game_info *info, size_t num_info) {
  core_bind.manifest = false;
  const uint8_t *data = (const uint8_t*)info[0].data;
  size_t size = info[0].size;
  if ((size & 0x7ffff) == 512) {
    size -= 512;
    data += 512;
  }

  retro_cheat_reset();
  if (info[0].path) {
    core_bind.load_request_error = false;
    core_bind.basename = info[0].path;

    char *posix_slash = (char*)strrchr(core_bind.basename, '/');
    char *win_slash = (char*)strrchr(core_bind.basename, '\\');
    if (posix_slash && !win_slash)
       posix_slash[1] = '\0';
    else if (win_slash && !posix_slash)
       win_slash[1] = '\0';
    else if (posix_slash && win_slash)
       max(posix_slash, win_slash)[1] = '\0';
    else
      core_bind.basename = "./";
  }

  switch (game_type) {
     case RETRO_GAME_TYPE_BSX:
        core_interface.mode = SuperFamicomCartridge::ModeBsx;
        return num_info == 2 && snes_load_cartridge_bsx(info[0].meta, data, size,
              info[1].meta, (const uint8_t*)info[1].data, info[1].size);

     case RETRO_GAME_TYPE_BSX | 0x1000:
        core_interface.mode = SuperFamicomCartridge::ModeBsx;
        return num_info == 2 && snes_load_cartridge_bsx(info[1].meta, (const uint8_t*)info[1].data, info[1].size,
              info[0].meta, (const uint8_t*)info[0].data, info[0].size);

     case RETRO_GAME_TYPE_BSX_SLOTTED:
        core_interface.mode = SuperFamicomCartridge::ModeBsxSlotted;
        return num_info == 2 && snes_load_cartridge_bsx_slotted(info[0].meta, data, size,
              info[1].meta, (const uint8_t*)info[1].data, info[1].size);

     case RETRO_GAME_TYPE_BSX_SLOTTED | 0x1000:
        core_interface.mode = SuperFamicomCartridge::ModeBsxSlotted;
        return num_info == 2 && snes_load_cartridge_bsx(info[1].meta, (const uint8_t*)info[1].data, info[1].size,
              info[0].meta, (const uint8_t*)info[0].data, info[0].size);

     case RETRO_GAME_TYPE_SUPER_GAME_BOY:
        core_interface.mode = SuperFamicomCartridge::ModeSuperGameBoy;
        return num_info == 2 && snes_load_cartridge_super_game_boy(info[0].meta, data, size,
              info[1].meta, (const uint8_t*)info[1].data, info[1].size);

     case RETRO_GAME_TYPE_SUPER_GAME_BOY | 0x1000:
        core_interface.mode = SuperFamicomCartridge::ModeSuperGameBoy;
        return num_info == 2 && snes_load_cartridge_super_game_boy(info[1].meta, (const uint8_t*)info[1].data, info[1].size,
              info[0].meta, (const uint8_t*)info[0].data, info[0].size);

     case RETRO_GAME_TYPE_SUFAMI_TURBO:
        core_interface.mode = SuperFamicomCartridge::ModeSufamiTurbo;
        return num_info == 3 && snes_load_cartridge_sufami_turbo(info[0].meta, (const uint8_t*)info[0].data, info[0].size,
              info[1].meta, (const uint8_t*)info[1].data, info[1].size,
              info[2].meta, (const uint8_t*)info[2].data, info[2].size);

     case RETRO_GAME_TYPE_SUFAMI_TURBO | 0x1000:
        core_interface.mode = SuperFamicomCartridge::ModeSufamiTurbo;
        return num_info == 3 && snes_load_cartridge_sufami_turbo(info[2].meta, (const uint8_t*)info[2].data, info[2].size,
              info[0].meta, (const uint8_t*)info[0].data, info[0].size,
              info[1].meta, (const uint8_t*)info[1].data, info[1].size);

     default:
        return false;
  }
}

void retro_unload_game(void) {
  core_bind.iface->save();
  SuperFamicom::cartridge.unload();
  core_bind.sram = nullptr;
  core_bind.sram_size = 0;
}

unsigned retro_get_region(void) {
  return SuperFamicom::system.region() == SuperFamicom::System::Region::NTSC ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

void* retro_get_memory_data(unsigned id) {
  if(SuperFamicom::cartridge.loaded() == false) return 0;
  if(core_bind.manifest) return 0;

  switch(id) {
    case RETRO_MEMORY_SAVE_RAM:
      return core_bind.sram;
    case RETRO_MEMORY_RTC:
      return nullptr;
    case RETRO_MEMORY_SNES_BSX_RAM:
      return nullptr;
    case RETRO_MEMORY_SNES_BSX_PRAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeBsx) break;
      return SuperFamicom::bsxcartridge.psram.data();
    case RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeSufamiTurbo) break;
      return SuperFamicom::sufamiturboA.ram.data();
    case RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeSufamiTurbo) break;
      return SuperFamicom::sufamiturboB.ram.data();
    case RETRO_MEMORY_SNES_GAME_BOY_RAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeSuperGameBoy) break;
      return GameBoy::cartridge.ramdata;

    case RETRO_MEMORY_SYSTEM_RAM:
      return SuperFamicom::cpu.wram;
    case RETRO_MEMORY_VIDEO_RAM:
      return SuperFamicom::ppu.vram;
  }

  return nullptr;
}

size_t retro_get_memory_size(unsigned id) {
  if(SuperFamicom::cartridge.loaded() == false) return 0;
  if(core_bind.manifest) return 0;
  size_t size = 0;

  switch(id) {
    case RETRO_MEMORY_SAVE_RAM:
      size = core_bind.sram_size;
      output(RETRO_LOG_INFO, "SRAM memory size: %u.\n", (unsigned)size);
      break;
    case RETRO_MEMORY_RTC:
      size = 0;
      break;
    case RETRO_MEMORY_SNES_BSX_RAM:
      size = 0;
      break;
    case RETRO_MEMORY_SNES_BSX_PRAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeBsx) break;
      size = SuperFamicom::bsxcartridge.psram.size();
      break;
    case RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeSufamiTurbo) break;
      size = SuperFamicom::sufamiturboA.ram.size();
      break;
    case RETRO_MEMORY_SNES_SUFAMI_TURBO_B_RAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeSufamiTurbo) break;
      size = SuperFamicom::sufamiturboB.ram.size();
      break;
    case RETRO_MEMORY_SNES_GAME_BOY_RAM:
      if(core_interface.mode != SuperFamicomCartridge::ModeSuperGameBoy) break;
      size = GameBoy::cartridge.ramsize;
      break;

    case RETRO_MEMORY_SYSTEM_RAM:
      size = 128 * 1024;
      break;
    case RETRO_MEMORY_VIDEO_RAM:
      size = 64 * 1024;
      break;
  }

  if(size == -1U) size = 0;
  return size;
}

