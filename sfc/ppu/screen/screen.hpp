struct Screen {
  uint32* output;

  struct Regs {
    bool addsub_mode;
    bool direct_color;

    bool color_mode;
    bool color_halve;
    bool bg1_color_enable;
    bool bg2_color_enable;
    bool bg3_color_enable;
    bool bg4_color_enable;
    bool oam_color_enable;
    bool back_color_enable;

    uint5 color_b;
    uint5 color_g;
    uint5 color_r;
  } regs;

  struct Math {
    struct Layer {
      uint16 color;
      bool color_enable;
    } main, sub;
    bool transparent;
    bool addsub_mode;
    bool color_halve;
  } math;

  inline void scanline();
  inline void run();
  inline void reset();

  inline uint16 get_pixel_sub(bool hires);
  inline uint16 get_pixel_main();
  inline uint16 addsub(unsigned x, unsigned y);
  inline uint16 get_color(unsigned palette);
  inline uint16 get_direct_color(unsigned palette, unsigned tile);
  inline uint16 fixed_color() const;

  inline void serialize(serializer&);
  inline Screen(PPU& self);

  PPU& self;
  friend class PPU;
};
