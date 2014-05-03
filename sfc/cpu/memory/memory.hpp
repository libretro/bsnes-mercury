inline void op_io();
inline uint8 op_read(uint32 addr);
inline void op_write(uint32 addr, uint8 data);
alwaysinline unsigned speed(unsigned addr) const;

inline uint8 disassembler_read(uint32 addr);
