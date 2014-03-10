#include <sfc/sfc.hpp>

#define DSP1_CPP
namespace SuperFamicom {

DSP1 dsp1;

#include "serialization.cpp"
#include "dsp1emu.cpp"

void DSP1::init() {
}

void DSP1::load() {
}

void DSP1::unload() {
}

void DSP1::power() {
}

void DSP1::reset() {
}

uint8 DSP1::read(unsigned addr) {
}

void DSP1::write(unsigned addr, uint8 data) {
  
}

}
