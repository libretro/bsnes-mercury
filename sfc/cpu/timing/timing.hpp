//timing.cpp
inline unsigned dma_counter();

alwaysinline void add_clocks(unsigned clocks);
inline void scanline();

alwaysinline void alu_edge();
alwaysinline void dma_edge();
alwaysinline void last_cycle();

inline void timing_power();
inline void timing_reset();

//irq.cpp
alwaysinline void poll_interrupts();
inline void nmitimen_update(uint8 data);
inline bool rdnmi();
inline bool timeup();

alwaysinline bool nmi_test();
alwaysinline bool irq_test();

//joypad.cpp
inline void step_auto_joypad_poll();
