public:
  inline auto mmio_read(uint addr, uint8 data) -> uint8;
  inline auto mmio_write(uint addr, uint8 data) -> void;

privileged:
struct {
  uint8 ppu1_mdr;
  uint8 ppu2_mdr;

  uint16 vram_readbuffer;
  uint8 oam_latchdata;
  uint8 cgram_latchdata;
  uint8 bgofs_latchdata;
  uint8 mode7_latchdata;
  bool counters_latched;
  bool latch_hcounter;
  bool latch_vcounter;

  uint10 oam_iaddr;
  uint9 cgram_iaddr;

  //$2100  INIDISP
  bool display_disable;
  uint4 display_brightness;

  //$2102  OAMADDL
  //$2103  OAMADDH
  uint10 oam_baseaddr;
  uint10 oam_addr;
  bool oam_priority;

  //$2105  BGMODE
  bool bg3_priority;
  uint8 bgmode;

  //$210d  BG1HOFS
  uint16 mode7_hoffset;

  //$210e  BG1VOFS
  uint16 mode7_voffset;

  //$2115  VMAIN
  bool vram_incmode;
  uint2 vram_mapping;
  uint8 vram_incsize;

  //$2116  VMADDL
  //$2117  VMADDH
  uint16 vram_addr;

  //$211a  M7SEL
  uint2 mode7_repeat;
  bool mode7_vflip;
  bool mode7_hflip;

  //$211b  M7A
  uint16 m7a;

  //$211c  M7B
  uint16 m7b;

  //$211d  M7C
  uint16 m7c;

  //$211e  M7D
  uint16 m7d;

  //$211f  M7X
  uint16 m7x;

  //$2120  M7Y
  uint16 m7y;

  //$2121  CGADD
  uint9 cgram_addr;

  //$2133  SETINI
  bool mode7_extbg;
  bool pseudo_hires;
  bool overscan;
  bool interlace;

  //$213c  OPHCT
  uint16 hcounter;

  //$213d  OPVCT
  uint16 vcounter;
} regs;

alwaysinline auto get_vram_address() -> uint16;
alwaysinline auto vram_read(uint addr) -> uint8;
alwaysinline auto vram_write(uint addr, uint8 data) -> void;
alwaysinline auto oam_read(uint addr) -> uint8;
alwaysinline auto oam_write(uint addr, uint8 data) -> void;
alwaysinline auto cgram_read(uint addr) -> uint8;
alwaysinline auto cgram_write(uint addr, uint8 data) -> void;

inline auto mmio_update_video_mode() -> void;

inline auto mmio_w2100(uint8) -> void;  //INIDISP
inline auto mmio_w2101(uint8) -> void;  //OBSEL
inline auto mmio_w2102(uint8) -> void;  //OAMADDL
inline auto mmio_w2103(uint8) -> void;  //OAMADDH
inline auto mmio_w2104(uint8) -> void;  //OAMDATA
inline auto mmio_w2105(uint8) -> void;  //BGMODE
inline auto mmio_w2106(uint8) -> void;  //MOSAIC
inline auto mmio_w2107(uint8) -> void;  //BG1SC
inline auto mmio_w2108(uint8) -> void;  //BG2SC
inline auto mmio_w2109(uint8) -> void;  //BG3SC
inline auto mmio_w210a(uint8) -> void;  //BG4SC
inline auto mmio_w210b(uint8) -> void;  //BG12NBA
inline auto mmio_w210c(uint8) -> void;  //BG34NBA
inline auto mmio_w210d(uint8) -> void;  //BG1HOFS
inline auto mmio_w210e(uint8) -> void;  //BG1VOFS
inline auto mmio_w210f(uint8) -> void;  //BG2HOFS
inline auto mmio_w2110(uint8) -> void;  //BG2VOFS
inline auto mmio_w2111(uint8) -> void;  //BG3HOFS
inline auto mmio_w2112(uint8) -> void;  //BG3VOFS
inline auto mmio_w2113(uint8) -> void;  //BG4HOFS
inline auto mmio_w2114(uint8) -> void;  //BG4VOFS
inline auto mmio_w2115(uint8) -> void;  //VMAIN
inline auto mmio_w2116(uint8) -> void;  //VMADDL
inline auto mmio_w2117(uint8) -> void;  //VMADDH
inline auto mmio_w2118(uint8) -> void;  //VMDATAL
inline auto mmio_w2119(uint8) -> void;  //VMDATAH
inline auto mmio_w211a(uint8) -> void;  //M7SEL
inline auto mmio_w211b(uint8) -> void;  //M7A
inline auto mmio_w211c(uint8) -> void;  //M7B
inline auto mmio_w211d(uint8) -> void;  //M7C
inline auto mmio_w211e(uint8) -> void;  //M7D
inline auto mmio_w211f(uint8) -> void;  //M7X
inline auto mmio_w2120(uint8) -> void;  //M7Y
inline auto mmio_w2121(uint8) -> void;  //CGADD
inline auto mmio_w2122(uint8) -> void;  //CGDATA
inline auto mmio_w2123(uint8) -> void;  //W12SEL
inline auto mmio_w2124(uint8) -> void;  //W34SEL
inline auto mmio_w2125(uint8) -> void;  //WOBJSEL
inline auto mmio_w2126(uint8) -> void;  //WH0
inline auto mmio_w2127(uint8) -> void;  //WH1
inline auto mmio_w2128(uint8) -> void;  //WH2
inline auto mmio_w2129(uint8) -> void;  //WH3
inline auto mmio_w212a(uint8) -> void;  //WBGLOG
inline auto mmio_w212b(uint8) -> void;  //WOBJLOG
inline auto mmio_w212c(uint8) -> void;  //TM
inline auto mmio_w212d(uint8) -> void;  //TS
inline auto mmio_w212e(uint8) -> void;  //TMW
inline auto mmio_w212f(uint8) -> void;  //TSW
inline auto mmio_w2130(uint8) -> void;  //CGWSEL
inline auto mmio_w2131(uint8) -> void;  //CGADDSUB
inline auto mmio_w2132(uint8) -> void;  //COLDATA
inline auto mmio_w2133(uint8) -> void;  //SETINI
inline auto mmio_r2134() -> uint8;  //MPYL
inline auto mmio_r2135() -> uint8;  //MPYM
inline auto mmio_r2136() -> uint8;  //MPYH
inline auto mmio_r2137() -> uint8;  //SLHV
inline auto mmio_r2138() -> uint8;  //OAMDATAREAD
inline auto mmio_r2139() -> uint8;  //VMDATALREAD
inline auto mmio_r213a() -> uint8;  //VMDATAHREAD
inline auto mmio_r213b() -> uint8;  //CGDATAREAD
inline auto mmio_r213c() -> uint8;  //OPHCT
inline auto mmio_r213d() -> uint8;  //OPVCT
inline auto mmio_r213e() -> uint8;  //STAT77
inline auto mmio_r213f() -> uint8;  //STAT78

inline auto mmio_reset() -> void;
