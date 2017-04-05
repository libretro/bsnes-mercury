#include <gb/gb.hpp>
#include <iostream>

namespace GameBoy {

Cheat cheat;

void Cheat::reset() {
  codes.reset();
}

void Cheat::append(unsigned addr, unsigned data) {
  codes.append({addr, Unused, data});
}

void Cheat::append(unsigned addr, unsigned comp, unsigned data) {
  codes.append({addr, comp, data});
}

optional<unsigned> Cheat::find(unsigned addr, unsigned comp) {
  for(auto& code : codes) {
    if(code.addr == addr && (code.comp == Unused || code.comp == comp)) {
      return {true, code.data};
    }
  }
  return false;
}

bool Cheat::decode(const char *part, unsigned &addr, unsigned &comp, unsigned &data) {
  /* Code Types
   * RAW:
   *   AAAA:DD
   * Game Genie: http://gamehacking.org/library/114
   * GameShark: 0BDDAAAA - Byteswapped Address, bank+1
   * Codebreaker: 0BAAAA-DD
   * Xploder: Unknown
   */
  std::cerr << "[bsnes]: Decoding cheats not implemented." << std::endl;
  return false;
}

void Cheat::synchronize() {
  std::cerr << "[bsnes]: Synchronizing cheats not implemented." << std::endl;
  return;
}

}
