struct {
  //$420b
  bool dma_enabled;

  //$420c
  bool hdma_enabled;

  //$43x0
  bool direction;
  bool indirect;
  bool unused;
  bool reverse_transfer;
  bool fixed_transfer;
  uint3 transfer_mode;

  //$43x1
  uint8 dest_addr;

  //$43x2-$43x3
  uint16 source_addr;

  //$43x4
  uint8 source_bank;

  //$43x5-$43x6
  union {
    uint16 transfer_size;
    uint16 indirect_addr;
  };

  //$43x7
  uint8 indirect_bank;

  //$43x8-$43x9
  uint16 hdma_addr;

  //$43xa
  uint8 line_counter;

  //$43xb/$43xf
  uint8 unknown;

  //internal state
  bool hdma_completed;
  bool hdma_do_transfer;
} channel[8];

struct {
  bool valid;
  unsigned addr;
  uint8 data;
} pipe;

inline void dma_add_clocks(unsigned clocks);
inline bool dma_transfer_valid(uint8 bbus, uint32 abus);
inline bool dma_addr_valid(uint32 abus);
inline uint8 dma_read(uint32 abus);
inline void dma_write(bool valid, unsigned addr = 0, uint8 data = 0);
inline void dma_transfer(bool direction, uint8 bbus, uint32 abus);

inline uint8 dma_bbus(unsigned i, unsigned channel);
inline uint32 dma_addr(unsigned i);
inline uint32 hdma_addr(unsigned i);
inline uint32 hdma_iaddr(unsigned i);

inline uint8 dma_enabled_channels();
inline bool hdma_active(unsigned i);
inline bool hdma_active_after(unsigned i);
inline uint8 hdma_enabled_channels();
inline uint8 hdma_active_channels();

inline void dma_run();
inline void hdma_update(unsigned i);
inline void hdma_run();
inline void hdma_init_reset();
inline void hdma_init();

inline void dma_power();
inline void dma_reset();
