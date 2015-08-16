#include <sfc/sfc.hpp>

#define MEMORY_CPP

namespace SuperFamicom {

Bus bus;

void Bus::map(
  const function<uint8 (unsigned)>& reader,
  const function<void (unsigned, uint8)>& writer,
  unsigned banklo, unsigned bankhi,
  unsigned addrlo, unsigned addrhi,
  unsigned size, unsigned base, unsigned mask,
  unsigned fastmode, uint8* fastptr
) {
  assert(banklo <= bankhi && banklo <= 0xff);
  assert(addrlo <= addrhi && addrlo <= 0xffff);
  assert(idcount < 255);

  bool do_fast=(size%(addrhi+1-addrlo)==0 && !((mask|addrlo|addrhi|size)&fast_page_size_mask));
  bool do_fast_read =(fastmode!=Cartridge::Mapping::fastmode_slow      && do_fast);
  bool do_fast_write=(fastmode==Cartridge::Mapping::fastmode_readwrite && do_fast);
  for(unsigned bank = banklo; bank <= bankhi; bank++) {
    for(unsigned addr = addrlo&~fast_page_size_mask; addr<=addrhi; addr+=fast_page_size) {
      unsigned origpos = (bank << 16 | addr);
      unsigned fastoffset = origpos >> fast_page_size_bits;

      unsigned accesspos = reduce(origpos, mask);
      if(size) accesspos = base + mirror(accesspos, size - base);
      if(do_fast_read)  fast_read[fastoffset] = fastptr - origpos + accesspos;
      else fast_read[fastoffset] = NULL;
      if(do_fast_write) fast_write[fastoffset] = fastptr - origpos + accesspos;
      else fast_write[fastoffset] = NULL;
    }
  }

#ifdef __LIBRETRO__
  if (((banklo&-banklo)|0x01000000)>(bankhi-banklo) && (bankhi&(bankhi+1)&~banklo)==0 &&
      ((addrlo&-addrlo)|0x01000000)>(addrhi-addrlo) && (addrhi&(addrhi+1)&~addrlo)==0)
  {
    retro_memory_descriptor desc;
    desc.flags=(fastmode==Cartridge::Mapping::fastmode_readwrite ? 0 : RETRO_MEMDESC_CONST);
    desc.ptr=fastptr;
    desc.offset=0;
    desc.start=banklo<<16 | addrlo;
    desc.select=(banklo<<16 | addrlo) ^ (bankhi<<16 | addrhi) ^ 0xFFFFFF;
    desc.disconnect=mask;
    desc.len=size;
    if (!size) desc.len=0x01000000;
    desc.addrspace=NULL;
    libretro_mem_map.append(desc);
  }
#endif

  unsigned id = idcount++;
  this->reader[id] = reader;
  this->writer[id] = writer;

  if (!(mask & (addrlo^addrhi)) && size%(addrhi+1-addrlo)==0) {
    //fastpath for common cases
    for(unsigned bank = banklo; bank <= bankhi; bank++) {
      unsigned offset = reduce(bank << 16 | addrlo, mask);
      if (size) offset = base + mirror(offset, size - base);
      unsigned pos = (bank<<16 | addrlo);
      unsigned end = (bank<<16 | addrhi);
      while (pos <= end) {
        lookup[pos] = id;
        target[pos] = offset++;
        pos++;
      }
    }
  } else {
    for(unsigned bank = banklo; bank <= bankhi; bank++) {
      for(unsigned addr = addrlo; addr <= addrhi; addr++) {
        unsigned offset = reduce(bank << 16 | addr, mask);
        if(size) offset = base + mirror(offset, size - base);
        lookup[bank << 16 | addr] = id;
        target[bank << 16 | addr] = offset;
      }
    }
  }
}

void Bus::map_reset() {
  function<uint8 (unsigned)> reader = [](unsigned) { return cpu.regs.mdr; };
  function<void (unsigned, uint8)> writer = [](unsigned, uint8) {};

#ifdef __LIBRETRO__
  libretro_mem_map.reset();
#endif

  idcount = 0;
  map(reader, writer, 0x00, 0xff, 0x0000, 0xffff);
}

void Bus::map_xml() {
  for(auto& m : cartridge.mapping) {
    lstring part = m.addr.split<1>(":");
    lstring banks = part(0).split(",");
    lstring addrs = part(1).split(",");
    for(auto& bank : banks) {
      for(auto& addr : addrs) {
        lstring bankpart = bank.split<1>("-");
        lstring addrpart = addr.split<1>("-");
        unsigned banklo = hex(bankpart(0));
        unsigned bankhi = hex(bankpart(1, bankpart(0)));
        unsigned addrlo = hex(addrpart(0));
        unsigned addrhi = hex(addrpart(1, addrpart(0)));
        map(m.reader, m.writer, banklo, bankhi, addrlo, addrhi, m.size, m.base, m.mask, m.fastmode, m.fastptr);
      }
    }
  }
}

}
