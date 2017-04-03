#include <sfc/sfc.hpp>
#include <iostream>

#define CHEAT_CPP
namespace SuperFamicom {

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
  //WRAM mirroring: $00-3f,80-bf:0000-1fff -> $7e:0000-1fff
  if((addr & 0x40e000) == 0x000000) addr = 0x7e0000 | (addr & 0x1fff);

  for(auto& code : codes) {
    if(code.addr == addr && (code.comp == Unused || code.comp == comp)) {
      return {true, code.data};
    }
  }
  return false;
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

bool Cheat::decode(const char *part, unsigned &addr, unsigned &data) {
  char addr_str[7], data_str[3];
  char *nulstr = '\0';
  addr_str[6]=0;
  data_str[2]=0;
  addr=data=0;

  //RAW
  if (strlen(part)>=9 && part[6]==':') {
    strncpy(addr_str,part,6);
    strncpy(data_str,part+7,2);
    addr=strtoul(addr_str,&nulstr,16);
    data=strtoul(data_str,&nulstr,16);
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
  }

  //PAR & X-Terminator
  else if (strlen(part)==8) {
    strncpy(addr_str,part,6);
    strncpy(data_str,part+6,2);
    addr=strtoul(addr_str,&nulstr,16);
    data=strtoul(data_str,&nulstr,16);
  }

  //Gold Finger
  else if (strlen(part)==14) {
    if (part[13]=='1'){
      std::cout << "[bsnes]: CHEAT: Goldfinger SRAM cheats not supported: " << part << std::endl;
      return false;
    }
    addr_str[0]='0';
    strncpy(addr_str+1,part,5);
    strncpy(data_str,part+5,2);
    if (part[7]!='X' && part[7]!='x')
      std::cout << "[bsnes]: CHEAT: Goldfinger multibyte cheats not supported: " << part << std::endl;

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
      std::cout << "[bsnes]: CHEAT: Goldfinger calculated checksum '" << std::hex << csum_calc <<  "' doesn't match code: " << part << std::endl;
      return false;
    }

    int addr_scrambled=strtoul(addr_str,&nulstr,16);
    addr=(addr_scrambled&0x7FFF)|((addr_scrambled&0x7F8000)<<1)|0x8000;
    data=strtoul(data_str,&nulstr,16);
  }

  //Unknown
  else {
    std::cout << "[bsnes]: CHEAT: Unrecognized code type: " << part << std::endl;
    return false;
  }

  if (addr == 0 || data == 0){
    std::cout << "[bsnes]: CHEAT: Decoding failed: " << part << std::endl;
    return false;
  }

  return true;
}

void Cheat::synchronize() {
  return;
}

}
