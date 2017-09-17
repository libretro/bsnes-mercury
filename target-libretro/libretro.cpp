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

#define SAMPLE_FREQ_PAL 14750000.0
#define SAMPLE_FREQ_NTSC (135000000.0/11.0)

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
static unsigned previous_height = 224; // SNES boots in 256x224 display mode

static const char * read_opt(const char * name, const char * defval);
static void update_system_geometry();

struct Callbacks : Emulator::Interface::Bind {
  retro_video_refresh_t pvideo_refresh;
  retro_audio_sample_batch_t paudio;
  retro_input_poll_t pinput_poll;
  retro_input_state_t pinput_state;
  retro_environment_t penviron;
  bool crop_overscan;
  bool gamma_ramp;
  unsigned short region_mode;
  unsigned short aspect_ratio_mode;
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

  bool input_polled;

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

  enum {
    video_fmt_32,
    video_fmt_16,
    video_fmt_15
  } video_fmt;
  union {
    uint32_t video_buffer[512 * 480];
    uint16_t video_buffer_16[512 * 480];
  };

  void videoRefresh(const uint32_t* palette, const uint32_t* data, unsigned pitch, unsigned width, unsigned height) override {
    if (crop_overscan || !SuperFamicom::ppu.overscan()) {
      data += 8 * 1024;

      if (height == 240)
        height = 224;
      else if (height == 480)
        height = 448;
    }
    else
    {
      /* The data contains always 240/480 lines, but the top row/two rows are always blank and 
         the actual data is in the bottom 239/478 lines. We want to display only the bottom 239/478 
         active lines so move the data pointer and update the height accordingly. */
      data += 1 * 1024;
      
      if (height == 240)
        height = 239;
      else if (height == 480)
        height = 478;
    }

    if (height != previous_height)
    {
      output(RETRO_LOG_DEBUG, "Display height: %u\n", height);
      output(RETRO_LOG_DEBUG, "Previous display height: %u\n", previous_height);
      previous_height = height;
      update_system_geometry();
    }

    if (video_fmt == video_fmt_32)
    {
      uint32_t *ptr = video_buffer;
      for (unsigned y = 0; y < height; y++, data += pitch >> 2, ptr += width)
         for (unsigned x = 0; x < width; x++)
            ptr[x] = palette[data[x]];
      
      pvideo_refresh(video_buffer, width, height, width*sizeof(uint32_t));
    }
    else
    {
      uint16_t *ptr = video_buffer_16;
      for (unsigned y = 0; y < height; y++, data += pitch >> 2, ptr += width)
         for (unsigned x = 0; x < width; x++)
            ptr[x] = palette[data[x]];
      
      pvideo_refresh(video_buffer_16, width, height, width*sizeof(uint16_t));
    }
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
    if (!input_polled)
    {
      pinput_poll();
      input_polled=true;
    }
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
    if (video_fmt == video_fmt_32) return (r << 16) | (g << 8) | (b << 0);
    if (video_fmt == video_fmt_16) return (r>>3 << 11) | (g>>2 << 5) | (b>>3 << 0);
    if (video_fmt == video_fmt_15) return (r>>3 << 10) | (g>>3 << 5) | (b>>3 << 0);
    return 0;
  }

  void notify(string text) override {
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
	struct retro_variable allowvar = { "bsnes_violate_accuracy", "disabled" };
	core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&allowvar);
	if (!strcmp(allowvar.value, "enabled"))
	{
		struct retro_variable var = {name, defval};
		core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&var);
		return var.value;
	}
	else return defval;
}

struct Interface : public SuperFamicom::Interface {
  SuperFamicomCartridge::Mode mode;

  Interface();

  void init() {
     if (core_bind.gamma_ramp)
        { SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Emulation); }
     else
        { SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Standard); }
  }
};

struct GBInterface : public GameBoy::Interface {
  GBInterface() { bind = &core_bind; }
  void init() {
     if (core_bind.gamma_ramp)
        { SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Emulation); }
     else
        { SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Standard); }
  }
};

static Interface core_interface;
static GBInterface core_gb_interface;

Interface::Interface() {
  bind = &core_bind;
  core_bind.iface = &core_interface;
}

unsigned retro_api_version(void) {
  return RETRO_API_VERSION;
}

static unsigned superfx_freq_orig;

void retro_set_environment(retro_environment_t environ_cb)
{
   core_bind.penviron = environ_cb;

   static const struct retro_variable vars[] = {
      { "bsnes_violate_accuracy", "Allow settings to reduce accuracy; disabled|enabled" },
      { "bsnes_chip_hle", "Special chip accuracy; LLE|HLE" },
      { "bsnes_superfx_overclock", "SuperFX speed; 100%|150%|200%|300%|400%|500%|1000%" },
         //Any integer is usable here, but there is no such thing as "any integer" in core options.
      { "bsnes_region", "System region; auto|ntsc|pal" },
      { "bsnes_aspect_ratio", "Preferred aspect ratio; auto|ntsc|pal" },
      { "bsnes_crop_overscan", "Crop overscan; disabled|enabled" }, 
      { "bsnes_gamma_ramp", "Gamma ramp (requires restart); disabled|enabled" },
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

   struct retro_variable overscan_var = { "bsnes_crop_overscan", "disabled" };
   core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&overscan_var);
   if (strcmp(overscan_var.value, "enabled") == 0)
     core_bind.crop_overscan = true;
   else
     core_bind.crop_overscan = false;
     
   struct retro_variable gamma_ramp_var = { "bsnes_gamma_ramp", "disabled" };
   core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&gamma_ramp_var);
   if (strcmp(gamma_ramp_var.value, "enabled") == 0)
     core_bind.gamma_ramp = true;
   else
     core_bind.gamma_ramp = false;

   struct retro_variable region_var = { "bsnes_region", "auto" };
   core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&region_var);
   if (strcmp(region_var.value, "ntsc") == 0)
     core_bind.region_mode = 1;
   else if (strcmp(region_var.value, "pal") == 0)
     core_bind.region_mode = 2;
   else
     core_bind.region_mode = 0;

   if (core_bind.region_mode == 1)
     SuperFamicom::configuration.region = SuperFamicom::System::Region::NTSC;
   else if (core_bind.region_mode == 2)
     SuperFamicom::configuration.region = SuperFamicom::System::Region::PAL;
   else
     SuperFamicom::configuration.region = SuperFamicom::System::Region::Autodetect;

   unsigned short old_aspect_ratio_mode = core_bind.aspect_ratio_mode;
   struct retro_variable aspect_ratio_var = { "bsnes_aspect_ratio", "auto" };
   core_bind.penviron(RETRO_ENVIRONMENT_GET_VARIABLE, (void*)&aspect_ratio_var);
   if (strcmp(aspect_ratio_var.value, "ntsc") == 0)
     core_bind.aspect_ratio_mode = 1;
   else if (strcmp(aspect_ratio_var.value, "pal") == 0)
     core_bind.aspect_ratio_mode = 2;
   else
     core_bind.aspect_ratio_mode = 0;

   if (old_aspect_ratio_mode != core_bind.aspect_ratio_mode)
   {
     update_system_geometry();
   }

   output(RETRO_LOG_DEBUG, "superfx_freq_orig: %u\n", superfx_freq_orig);
   output(RETRO_LOG_DEBUG, "SuperFamicom::superfx.frequency: %u\n", SuperFamicom::superfx.frequency);
   output(RETRO_LOG_DEBUG, "Overscan mode: %u\n", core_bind.crop_overscan);
   output(RETRO_LOG_DEBUG, "Region mode: %u\n", core_bind.region_mode);
   output(RETRO_LOG_DEBUG, "Aspect ratio mode: %u\n", core_bind.aspect_ratio_mode);
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
  update_variables();
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
  core_bind.input_polled=false;
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

void retro_cheat_reset(void) {
  SuperFamicom::cheat.reset();
}

static char genie_replace(char input){
  switch (input){
    case '0':
      return '4';
    case '1':
      return '6';
    case '2':
      return 'D';
    case '3':
      return 'E';
    case '4':
      return '2';
    case '5':
      return '7';
    case '6':
      return '8';
    case '7':
      return '3';
    case '8':
      return 'B';
    case '9':
      return '5';
    case 'A':
    case 'a':
      return 'C';
    case 'B':
    case 'b':
      return '9';
    case 'C':
    case 'c':
      return 'A';
    case 'D':
    case 'd':
      return '0';
    case 'E':
    case 'e':
      return 'F';
    case 'F':
    case 'f':
      break;
  }

  return '1';
}

void retro_cheat_set(unsigned index, bool enable, const char *code) {
  char codeCopy[256];
  char *part;
  unsigned addr, data;
  char addr_str[7], data_str[7];
  char *nulstr = (char *)'\0';

  if (code == nulstr) return;
  strcpy(codeCopy,code);
  part=strtok(codeCopy,"+,;._ ");
  
  while (part!=NULL)
  {

    addr_str[6]=0;
    data_str[2]=0;
    data_str[6]=0;
    addr=data=0;

    //RAW
    if (strlen(part)>=9 && part[6]==':') {
      strncpy(addr_str,part,6);
      strncpy(data_str,part+7,2);
      addr=strtoul(addr_str,&nulstr,16);
      data=strtoul(data_str,&nulstr,16);
      SuperFamicom::cheat.append(addr,data);
    }

    //Game Genie
    else if (strlen(part)>=9 && part[4]=='-') {
      strncpy(data_str,part,2);
      strncpy(addr_str,part+2,2);
      strncpy(addr_str+2,part+5,4);
      for (int i=0;i<2;i++)
        data_str[i]=genie_replace(data_str[i]);
      for (int i=0;i<6;i++)
        addr_str[i]=genie_replace(addr_str[i]);
      data=strtoul(data_str,&nulstr,16);
      int addr_scrambled=strtoul(addr_str,&nulstr,16);
      addr=(addr_scrambled&0x003C00)<<10;
      addr|=(addr_scrambled&0x00003C)<<14;
      addr|=(addr_scrambled&0xF00000)>>8;
      addr|=(addr_scrambled&0x000003)<<10;
      addr|=(addr_scrambled&0x00C000)>>6;
      addr|=(addr_scrambled&0x0F0000)>>12;
      addr|=(addr_scrambled&0x0003C0)>>6;
      SuperFamicom::cheat.append(addr,data);
    }

    //PAR & X-Terminator
    else if (strlen(part)==8) {
      strncpy(addr_str,part,6);
      strncpy(data_str,part+6,2);
      addr=strtoul(addr_str,&nulstr,16);
      data=strtoul(data_str,&nulstr,16);
      SuperFamicom::cheat.append(addr,data);
    }

    //Gold Finger
    else if (strlen(part)==14) {
      if (part[13]=='1'){
        retro_log_default(RETRO_LOG_INFO,"CHEAT: Goldfinger SRAM cheats not supported: %s\n",part);
        goto nextLine;
      }
      addr_str[0]='0';
      strncpy(addr_str+1,part,5);

      addr=strtoul(addr_str,&nulstr,16);
      addr=(addr&0x7FFF)|((addr&0x7F8000)<<1)|0x8000;

      strncpy(data_str,part+5,6);

      char pair_str[3];
      pair_str[2]=0;
      int csum_calc=0,csum_code;
      for (int i=0;i<6;i++){
        if (i<3){
          strncpy(pair_str,addr_str+2*i,2);
        } else {
          strncpy(pair_str,part+2*i-1,2);
        }
        csum_calc+=strtoul(pair_str,&nulstr,16);
      }
      csum_calc-=0x160;
      csum_calc&=0xFF;
      strncpy(pair_str,part+11,2);
      csum_code=strtoul(pair_str,&nulstr,16);
      if (csum_calc!=csum_code){
        retro_log_default(RETRO_LOG_INFO,"CHEAT: Goldfinger calculated checksum '%X' doesn't match code: %s\n", csum_calc, part);
        goto nextLine;
      }

      for (int i=0;i<3;i++){
        strncpy(pair_str,data_str+2*i,2);
        if (pair_str[0]!='x' && pair_str[0]!='X'){
          data=strtoul(pair_str,&nulstr,16);
          SuperFamicom::cheat.append(addr+i,data);
        }
      }
    }

    //Unknown
    else {
      retro_log_default(RETRO_LOG_INFO, "CHEAT: Unrecognized code type: %s\n",part);
    }

    if (addr == 0 || data == 0){
      retro_log_default(RETRO_LOG_INFO,"CHEAT: Decoding failed: %s\n",part);
    }

    nextLine:
      part=strtok(NULL,"+,;._ ");

  }
  
}

void retro_get_system_info(struct retro_system_info *info) {
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
  static string version("v", Emulator::Version, " (", Emulator::Profile, ")", GIT_VERSION);
  info->library_name     = "bsnes-mercury";
  info->library_version  = version;
  info->valid_extensions = "sfc|smc|bml";
  info->need_fullpath    = false;
}

static double get_aspect_ratio(unsigned width, unsigned height)
{
  bool is_pal = retro_get_region() == RETRO_REGION_PAL;
  if (core_bind.aspect_ratio_mode == 1)
    is_pal = false; 
  else if (core_bind.aspect_ratio_mode == 2)
    is_pal = true;

  double par = (is_pal ? SAMPLE_FREQ_PAL : SAMPLE_FREQ_NTSC) / (SuperFamicom::ppu.frequency / 2.0);
  return (double)width * par / (double)height; 
}

static void get_system_av_info(struct retro_system_av_info *info)
{
  struct retro_system_timing timing = { 0.0, 32040.5 };
  timing.fps = retro_get_region() == RETRO_REGION_NTSC ? 21477272.0 / 357366.0 : 21281370.0 / 425568.0;
  unsigned max_width = 512;
  unsigned max_height = core_bind.crop_overscan ? 448 : 478;
  unsigned base_width = 256;
  unsigned base_height = core_bind.crop_overscan || !SuperFamicom::ppu.overscan() ? 224 : 239;

  double aspect_ratio = get_aspect_ratio(base_width, base_height);
  output(RETRO_LOG_DEBUG, "Base height: %u\n", base_height);
  output(RETRO_LOG_DEBUG, "Base width: %u\n", base_width);
  output(RETRO_LOG_DEBUG, "Aspect ratio: %f\n", aspect_ratio);
  output(RETRO_LOG_DEBUG, "FPS: %f\n", timing.fps);
  struct retro_game_geometry geom = { base_width, base_height, max_width, max_height, (float)aspect_ratio };

  info->timing = timing;
  info->geometry = geom;
}

static void update_system_geometry()
{
  struct retro_system_av_info info;
  get_system_av_info(&info);
  core_bind.penviron(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
  get_system_av_info(info);
  
  enum retro_pixel_format fmt;
  fmt = RETRO_PIXEL_FORMAT_XRGB8888;
  if (core_bind.penviron(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    core_bind.video_fmt = Callbacks::video_fmt_32;
  else
  {
    fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (core_bind.penviron(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      core_bind.video_fmt = Callbacks::video_fmt_16;
    else
    {
      //this one is always supported
      core_bind.video_fmt = Callbacks::video_fmt_15;
    }
    if (core_bind.gamma_ramp)
        { SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Emulation); }
     else
        { SuperFamicom::video.generate_palette(Emulator::Interface::PaletteMode::Standard); }
  }
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

static void init_descriptors(void)
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 0 },
   };

   core_bind.penviron(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

bool retro_load_game(const struct retro_game_info *info) {
  // Support loading a manifest directly.
  core_bind.manifest = info->path && string(info->path).endsWith(".bml");
  init_descriptors();

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
  if (ret) {
    SuperFamicom::bus.libretro_mem_map.reverse();
    retro_memory_map map={SuperFamicom::bus.libretro_mem_map.data(), SuperFamicom::bus.libretro_mem_map.size()};
    core_bind.penviron(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void*)&map);
    
    if (SuperFamicom::cartridge.has_superfx())
      superfx_freq_orig=SuperFamicom::superfx.frequency;
  }
  
  return ret;
}


bool retro_load_game_special(unsigned game_type,
      const struct retro_game_info *info, size_t num_info) {
  core_bind.manifest = false;
  init_descriptors();
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

