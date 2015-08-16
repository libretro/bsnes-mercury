#ifdef __LIBRETRO__
#include "../../target-libretro/libretro.h"
#endif

struct Memory {
  virtual inline unsigned size() const;
  virtual uint8 read(unsigned addr) = 0;
  virtual void write(unsigned addr, uint8 data) = 0;
  virtual uint8* data() { return NULL; }
};

struct StaticRAM : Memory {
  inline uint8* data();
  inline unsigned size() const;

  inline uint8 read(unsigned addr);
  inline void write(unsigned addr, uint8 n);
  inline uint8& operator[](unsigned addr);
  inline const uint8& operator[](unsigned addr) const;

  inline StaticRAM(unsigned size);
  inline ~StaticRAM();

private:
  uint8* data_;
  unsigned size_;
};

struct MappedRAM : Memory {
  inline void reset();
  inline void map(uint8*, unsigned);
  inline void copy(const stream& memory);
  inline void read(const stream& memory);

  inline void write_protect(bool status);
  inline uint8* data();
  inline unsigned size() const;

  inline uint8 read(unsigned addr);
  inline void write(unsigned addr, uint8 n);
  inline const uint8& operator[](unsigned addr) const;
  inline MappedRAM();

private:
  uint8* data_;
  unsigned size_;
  bool write_protect_;
};

struct Bus {
  alwaysinline static unsigned mirror(unsigned addr, unsigned size);
  alwaysinline static unsigned reduce(unsigned addr, unsigned mask);

  alwaysinline uint8 read(unsigned addr);
  alwaysinline void write(unsigned addr, uint8 data);

  unsigned idcount;
  function<uint8 (unsigned)> reader[256];
  function<void (unsigned, uint8)> writer[256];

  static const uint32 fast_page_size_bits = 13;//keep at 13 or lower so the RAM mirrors can be on the fast path
  static const uint32 fast_page_size = (1 << fast_page_size_bits);
  static const uint32 fast_page_size_mask = (fast_page_size - 1);
  uint8* fast_read[0x1000000>>fast_page_size_bits];
  uint8* fast_write[0x1000000>>fast_page_size_bits];

  void map(
    const function<uint8 (unsigned)>& reader,
    const function<void (unsigned, uint8)>& writer,
    unsigned banklo, unsigned bankhi,
    unsigned addrlo, unsigned addrhi,
    unsigned size = 0, unsigned base = 0, unsigned mask = 0,
    unsigned fastmode = 0, uint8* fast_ptr = NULL
  );

  void map_reset();
  void map_xml();

#ifdef __LIBRETRO__
  vector<retro_memory_descriptor> libretro_mem_map;
#endif

  uint8 lookup[16 * 1024 * 1024];
  uint32 target[16 * 1024 * 1024];
};

extern Bus bus;
