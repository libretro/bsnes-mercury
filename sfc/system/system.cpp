#include <sfc/sfc.hpp>

namespace SuperFamicom {

System system;
Configuration configuration;
Random random;

#include "video.cpp"
#include "audio.cpp"
#include "device.cpp"
#include "serialization.cpp"

#include <sfc/scheduler/scheduler.cpp>

System::System() {
  region = Region::Autodetect;
  expansionPort = Device::ID::eBoot;
}

auto System::run() -> void {
  scheduler.sync = Scheduler::SynchronizeMode::None;

  scheduler.enter();
  if(scheduler.exit_reason == Scheduler::ExitReason::FrameEvent) {
    video.update();
  }
}

auto System::runToSave() -> void {
  if(CPU::Threaded == true) {
    scheduler.sync = Scheduler::SynchronizeMode::CPU;
    runThreadToSave();
  }

  if(SMP::Threaded == true) {
    scheduler.thread = smp.thread;
    runThreadToSave();
  }

  if(PPU::Threaded == true) {
    scheduler.thread = ppu.thread;
    runThreadToSave();
  }

  if(DSP::Threaded == true) {
    scheduler.thread = dsp.thread;
    runThreadToSave();
  }

  for(unsigned i = 0; i < cpu.coprocessors.size(); i++) {
    auto& chip = *cpu.coprocessors[i];
    scheduler.thread = chip.thread;
    runThreadToSave();
  }
}

auto System::runThreadToSave() -> void {
  while(true) {
    scheduler.enter();
    if(scheduler.exit_reason == Scheduler::ExitReason::SynchronizeEvent) break;
    if(scheduler.exit_reason == Scheduler::ExitReason::FrameEvent) {
      video.update();
    }
  }
}

auto System::init() -> void {
  assert(interface != nullptr);

  satellaview.init();
  eboot.init();

  icd2.init();
  mcc.init();
  nss.init();
  event.init();
  sa1.init();
  superfx.init();
  armdsp.init();
  hitachidsp.init();
  necdsp.init();
  epsonrtc.init();
  sharprtc.init();
  spc7110.init();
  sdd1.init();
  obc1.init();
  msu1.init();

  bsmemory.init();

  dsp1.init();
  dsp2.init();
  dsp3.init();
  dsp4.init();
  cx4.init();
  st0010.init();

  video.init();
  audio.init();

  device.connect(0, configuration.controllerPort1);
  device.connect(1, configuration.controllerPort2);
}

auto System::term() -> void {
}

auto System::load() -> void {
  interface->loadRequest(ID::SystemManifest, "manifest.bml", true);
#ifdef __LIBRETRO__
  interface->loadRequest(ID::IPLROM, "ipl.rom", true);
#else
  auto document = BML::unserialize(information.manifest);

  if(auto iplrom = document["system/smp/rom/name"].text()) {
    interface->loadRequest(ID::IPLROM, iplrom, true);
  }
#endif

  region = configuration.region;
  if(region == Region::Autodetect) {
    region = (cartridge.region() == Cartridge::Region::NTSC ? Region::NTSC : Region::PAL);
  }
  expansionPort = configuration.expansionPort;
  cpuFrequency = region() == Region::NTSC ? 21477272 : 21281370;
  apuFrequency = 24606720;

  audio.coprocessor_enable(false);

  bus.reset();
  bus.map();

  cpu.enable();
  ppu.enable();

  if(expansionPort() == Device::ID::Satellaview) satellaview.load();
  if(expansionPort() == Device::ID::eBoot) eboot.load();

  if(cartridge.hasICD2()) icd2.load();
  if(cartridge.hasMCC()) mcc.load();
  if(cartridge.hasNSSDIP()) nss.load();
  if(cartridge.hasEvent()) event.load();
  if(cartridge.hasSA1()) sa1.load();
  if(cartridge.hasSuperFX()) superfx.load();
  if(cartridge.hasARMDSP()) armdsp.load();
  if(cartridge.hasHitachiDSP()) hitachidsp.load();
  if(cartridge.hasNECDSP()) necdsp.load();
  if(cartridge.hasEpsonRTC()) epsonrtc.load();
  if(cartridge.hasSharpRTC()) sharprtc.load();
  if(cartridge.hasSPC7110()) spc7110.load();
  if(cartridge.hasSDD1()) sdd1.load();
  if(cartridge.hasOBC1()) obc1.load();
  if(cartridge.hasMSU1()) msu1.load();

  if(cartridge.hasBSMemorySlot()) bsmemory.load();
  if(cartridge.hasSufamiTurboSlots()) sufamiturboA.load(), sufamiturboB.load();

  if(cartridge.hasDSP1()) dsp1.load();
  if(cartridge.hasDSP2()) dsp2.load();
  if(cartridge.hasDSP3()) dsp3.load();
  if(cartridge.hasDSP4()) dsp4.load();
  if(cartridge.hasCX4()) cx4.load();
  if(cartridge.hasST0010()) st0010.load();

  serializeInit();
}

auto System::unload() -> void {
  if(expansionPort() == Device::ID::Satellaview) satellaview.unload();
  if(expansionPort() == Device::ID::eBoot) eboot.unload();

  if(cartridge.hasICD2()) icd2.unload();
  if(cartridge.hasMCC()) mcc.unload();
  if(cartridge.hasNSSDIP()) nss.unload();
  if(cartridge.hasEvent()) event.unload();
  if(cartridge.hasSA1()) sa1.unload();
  if(cartridge.hasSuperFX()) superfx.unload();
  if(cartridge.hasARMDSP()) armdsp.unload();
  if(cartridge.hasHitachiDSP()) hitachidsp.unload();
  if(cartridge.hasNECDSP()) necdsp.unload();
  if(cartridge.hasEpsonRTC()) epsonrtc.unload();
  if(cartridge.hasSharpRTC()) sharprtc.unload();
  if(cartridge.hasSPC7110()) spc7110.unload();
  if(cartridge.hasSDD1()) sdd1.unload();
  if(cartridge.hasOBC1()) obc1.unload();
  if(cartridge.hasMSU1()) msu1.unload();

  if(cartridge.hasBSMemorySlot()) bsmemory.unload();
  if(cartridge.hasSufamiTurboSlots()) sufamiturboA.unload(), sufamiturboB.unload();

  if(cartridge.hasDSP1()) dsp1.unload();
  if(cartridge.hasDSP2()) dsp2.unload();
  if(cartridge.hasDSP3()) dsp3.unload();
  if(cartridge.hasDSP4()) dsp4.unload();
  if(cartridge.hasCX4()) cx4.unload();
  if(cartridge.hasST0010()) st0010.unload();
}

auto System::power() -> void {
  random.seed((uint)time(0));

  cpu.power();
  smp.power();
  dsp.power();
  ppu.power();

  if(expansionPort() == Device::ID::Satellaview) satellaview.power();
  if(expansionPort() == Device::ID::eBoot) eboot.power();

  if(cartridge.hasICD2()) icd2.power();
  if(cartridge.hasMCC()) mcc.power();
  if(cartridge.hasNSSDIP()) nss.power();
  if(cartridge.hasEvent()) event.power();
  if(cartridge.hasSA1()) sa1.power();
  if(cartridge.hasSuperFX()) superfx.power();
  if(cartridge.hasARMDSP()) armdsp.power();
  if(cartridge.hasHitachiDSP()) hitachidsp.power();
  if(cartridge.hasNECDSP()) necdsp.power();
  if(cartridge.hasEpsonRTC()) epsonrtc.power();
  if(cartridge.hasSharpRTC()) sharprtc.power();
  if(cartridge.hasSPC7110()) spc7110.power();
  if(cartridge.hasSDD1()) sdd1.power();
  if(cartridge.hasOBC1()) obc1.power();
  if(cartridge.hasMSU1()) msu1.power();

  if(cartridge.hasBSMemorySlot()) bsmemory.power();

  if(cartridge.hasDSP1()) dsp1.power();
  if(cartridge.hasDSP2()) dsp2.power();
  if(cartridge.hasDSP3()) dsp3.power();
  if(cartridge.hasDSP4()) dsp4.power();
  if(cartridge.hasCX4()) cx4.power();
  if(cartridge.hasST0010()) st0010.power();

  reset();
}

auto System::reset() -> void {
  cpu.reset();
  smp.reset();
  dsp.reset();
  ppu.reset();

  if(expansionPort() == Device::ID::Satellaview) satellaview.reset();
  if(expansionPort() == Device::ID::eBoot) eboot.reset();

  if(cartridge.hasICD2()) icd2.reset();
  if(cartridge.hasMCC()) mcc.reset();
  if(cartridge.hasNSSDIP()) nss.reset();
  if(cartridge.hasEvent()) event.reset();
  if(cartridge.hasSA1()) sa1.reset();
  if(cartridge.hasSuperFX()) superfx.reset();
  if(cartridge.hasARMDSP()) armdsp.reset();
  if(cartridge.hasHitachiDSP()) hitachidsp.reset();
  if(cartridge.hasNECDSP()) necdsp.reset();
  if(cartridge.hasEpsonRTC()) epsonrtc.reset();
  if(cartridge.hasSharpRTC()) sharprtc.reset();
  if(cartridge.hasSPC7110()) spc7110.reset();
  if(cartridge.hasSDD1()) sdd1.reset();
  if(cartridge.hasOBC1()) obc1.reset();
  if(cartridge.hasMSU1()) msu1.reset();

  if(cartridge.hasBSMemorySlot()) bsmemory.reset();

  if(cartridge.hasDSP1()) dsp1.reset();
  if(cartridge.hasDSP2()) dsp2.reset();
  if(cartridge.hasDSP3()) dsp3.reset();
  if(cartridge.hasDSP4()) dsp4.reset();
  if(cartridge.hasCX4()) cx4.reset();
  if(cartridge.hasST0010()) st0010.reset();

  if(cartridge.hasICD2()) cpu.coprocessors.append(&icd2);
  if(cartridge.hasEvent()) cpu.coprocessors.append(&event);
  if(cartridge.hasSA1()) cpu.coprocessors.append(&sa1);
  if(cartridge.hasSuperFX()) cpu.coprocessors.append(&superfx);
  if(cartridge.hasARMDSP()) cpu.coprocessors.append(&armdsp);
  if(cartridge.hasHitachiDSP()) cpu.coprocessors.append(&hitachidsp);
  if(cartridge.hasNECDSP()) cpu.coprocessors.append(&necdsp);
  if(cartridge.hasEpsonRTC()) cpu.coprocessors.append(&epsonrtc);
  if(cartridge.hasSharpRTC()) cpu.coprocessors.append(&sharprtc);
  if(cartridge.hasSPC7110()) cpu.coprocessors.append(&spc7110);
  if(cartridge.hasMSU1()) cpu.coprocessors.append(&msu1);

  scheduler.init();
  device.connect(0, configuration.controllerPort1);
  device.connect(1, configuration.controllerPort2);
}

auto System::scanline() -> void {
  video.scanline();
  if(cpu.vcounter() == 241) scheduler.exit(Scheduler::ExitReason::FrameEvent);
}

auto System::frame() -> void {
}

}
