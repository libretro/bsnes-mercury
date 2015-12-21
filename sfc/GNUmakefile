sfc_objects := sfc-interface sfc-system sfc-controller
sfc_objects += sfc-cartridge sfc-cheat
sfc_objects += sfc-memory sfc-cpu sfc-smp sfc-dsp sfc-ppu
sfc_objects += sfc-satellaview sfc-eboot
sfc_objects += sfc-icd2 sfc-mcc sfc-nss sfc-event
sfc_objects += sfc-sa1 sfc-superfx
sfc_objects += sfc-armdsp sfc-hitachidsp sfc-necdsp
sfc_objects += sfc-epsonrtc sfc-sharprtc
sfc_objects += sfc-spc7110 sfc-sdd1 sfc-obc1
sfc_objects += sfc-msu1
sfc_objects += sfc-bsmemory sfc-sufamiturbo
sfc_objects += sfc-hledsp1 sfc-hledsp2 sfc-hledsp3 sfc-hledsp4
sfc_objects += sfc-hlecx4 sfc-hlest0010

ifeq ($(profile),accuracy)
  profflags += -DPROFILE_ACCURACY
  sfccpu := $(sfc)/cpu
  sfcsmp := $(sfc)/smp
  sfcdsp := $(sfc)/dsp
  sfcppu := $(sfc)/ppu
else ifeq ($(profile),balanced)
  profflags += -DPROFILE_BALANCED
  sfccpu := $(sfc)/cpu
  sfcsmp := $(sfc)/smp
  sfcdsp := $(sfc)/alt/dsp
  sfcppu := $(sfc)/alt/ppu-balanced
else ifeq ($(profile),performance)
  profflags += -DPROFILE_PERFORMANCE
  sfccpu := $(sfc)/alt/cpu
  sfcsmp := $(sfc)/alt/smp
  sfcdsp := $(sfc)/alt/dsp
  sfcppu := $(sfc)/alt/ppu-performance
else
  $(error unknown Super Famicom profile)
endif

obj/sfc-interface-$(profile).o:   $(sfc)/interface/interface.cpp $(call rwildcard,$(sfc)/interface)
obj/sfc-system-$(profile).o:      $(sfc)/system/system.cpp $(call rwildcard,$(sfc)/system/)
obj/sfc-controller-$(profile).o:  $(sfc)/controller/controller.cpp $(call rwildcard,$(sfc)/controller/)
obj/sfc-cartridge-$(profile).o:   $(sfc)/cartridge/cartridge.cpp $(call rwildcard,$(sfc)/cartridge/)
obj/sfc-cheat-$(profile).o:       $(sfc)/cheat/cheat.cpp $(call rwildcard,$(sfc)/cheat/)
obj/sfc-memory-$(profile).o:      $(sfc)/memory/memory.cpp $(call rwildcard,$(sfc)/memory/)
obj/sfc-cpu-$(profile).o:         $(sfccpu)/cpu.cpp $(call rwildcard,$(sfccpu)/)
obj/sfc-smp-$(profile).o:         $(sfcsmp)/smp.cpp $(call rwildcard,$(sfcsmp)/)
obj/sfc-dsp-$(profile).o:         $(sfcdsp)/dsp.cpp $(call rwildcard,$(sfcdsp)/)
obj/sfc-ppu-$(profile).o:         $(sfcppu)/ppu.cpp $(call rwildcard,$(sfcppu)/)

obj/sfc-satellaview-$(profile).o: $(sfc)/expansion/satellaview/satellaview.cpp $(call rwildcard,$(sfc)/expansion/satellaview/)
obj/sfc-eboot-$(profile).o:       $(sfc)/expansion/eboot/eboot.cpp $(call rwildcard,$(sfc)/expansion/eboot/)

obj/sfc-icd2-$(profile).o:        $(sfc)/coprocessor/icd2/icd2.cpp $(call rwildcard,$(sfc)/coprocessor/icd2/)
obj/sfc-mcc-$(profile).o:         $(sfc)/coprocessor/mcc/mcc.cpp $(call rwildcard,$(sfc)/coprocessor/mcc/)
obj/sfc-nss-$(profile).o:         $(sfc)/coprocessor/nss/nss.cpp $(call rwildcard,$(sfc)/coprocessor/nss/)
obj/sfc-event-$(profile).o:       $(sfc)/coprocessor/event/event.cpp $(call rwildcard,$(sfc)/coprocessor/event/)

obj/sfc-sa1-$(profile).o:         $(sfc)/coprocessor/sa1/sa1.cpp $(call rwildcard,$(sfc)/coprocessor/sa1/)
obj/sfc-superfx-$(profile).o:     $(sfc)/coprocessor/superfx/superfx.cpp $(call rwildcard,$(sfc)/coprocessor/superfx/)

obj/sfc-armdsp-$(profile).o:      $(sfc)/coprocessor/armdsp/armdsp.cpp $(call rwildcard,$(sfc)/coprocessor/armdsp/)
obj/sfc-hitachidsp-$(profile).o:  $(sfc)/coprocessor/hitachidsp/hitachidsp.cpp $(call rwildcard,$(sfc)/coprocessor/hitachidsp/)
obj/sfc-necdsp-$(profile).o:      $(sfc)/coprocessor/necdsp/necdsp.cpp $(call rwildcard,$(sfc)/coprocessor/necdsp/)

obj/sfc-epsonrtc-$(profile).o:    $(sfc)/coprocessor/epsonrtc/epsonrtc.cpp $(call rwildcard,$(sfc)/coprocessor/epsonrtc/)
obj/sfc-sharprtc-$(profile).o:    $(sfc)/coprocessor/sharprtc/sharprtc.cpp $(call rwildcard,$(sfc)/coprocessor/sharprtc/)

obj/sfc-spc7110-$(profile).o:     $(sfc)/coprocessor/spc7110/spc7110.cpp $(call rwildcard,$(sfc)/coprocessor/spc7110/)
obj/sfc-sdd1-$(profile).o:        $(sfc)/coprocessor/sdd1/sdd1.cpp $(call rwildcard,$(sfc)/coprocessor/sdd1/)
obj/sfc-obc1-$(profile).o:        $(sfc)/coprocessor/obc1/obc1.cpp $(call rwildcard,$(sfc)/coprocessor/obc1/)

obj/sfc-msu1-$(profile).o:        $(sfc)/coprocessor/msu1/msu1.cpp $(call rwildcard,$(sfc)/coprocessor/msu1/)

obj/sfc-bsmemory-$(profile).o:    $(sfc)/slot/bsmemory/bsmemory.cpp $(call rwildcard,$(sfc)/slot/bsmemory/)
obj/sfc-sufamiturbo-$(profile).o: $(sfc)/slot/sufamiturbo/sufamiturbo.cpp $(call rwildcard,$(sfc)/slot/sufamiturbo/)

obj/sfc-hledsp1-$(profile).o:     $(sfc)/coprocessor/dsp1/dsp1.cpp $(sfc)/coprocessor/dsp1/*
obj/sfc-hledsp2-$(profile).o:     $(sfc)/coprocessor/dsp2/dsp2.cpp $(sfc)/coprocessor/dsp2/*
obj/sfc-hledsp3-$(profile).o:     $(sfc)/coprocessor/dsp3/dsp3.cpp $(sfc)/coprocessor/dsp3/*
obj/sfc-hledsp4-$(profile).o:     $(sfc)/coprocessor/dsp4/dsp4.cpp $(sfc)/coprocessor/dsp4/*

obj/sfc-hlecx4-$(profile).o:      $(sfc)/coprocessor/cx4/cx4.cpp $(sfc)/coprocessor/cx4/*
obj/sfc-hlest0010-$(profile).o:   $(sfc)/coprocessor/st0010/st0010.cpp $(sfc)/coprocessor/st0010/*