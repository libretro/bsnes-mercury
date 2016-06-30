sfc_objects := sfc-interface sfc-system sfc-controller
sfc_objects += sfc-cartridge sfc-cheat
sfc_objects += sfc-memory sfc-cpu sfc-smp sfc-dsp sfc-ppu
sfc_objects += sfc-satellaviewbase
sfc_objects += sfc-icd2 sfc-bsx sfc-nss sfc-event
sfc_objects += sfc-sa1 sfc-superfx
sfc_objects += sfc-armdsp sfc-hitachidsp sfc-necdsp
sfc_objects += sfc-epsonrtc sfc-sharprtc
sfc_objects += sfc-spc7110 sfc-sdd1 sfc-obc1
sfc_objects += sfc-hsu1 sfc-msu1
sfc_objects += sfc-satellaviewcart sfc-sufamiturbo
sfc_objects += sfc-hledsp1 sfc-hledsp2 sfc-hledsp3 sfc-hledsp4
sfc_objects += sfc-hlecx4 sfc-hlest0010
sfc_objects += sfc-sgbexternal

ifeq ($(profile),accuracy)
  profflags := -DPROFILE_ACCURACY
  sfccpu := $(sfc)/cpu
  sfcsmp := $(sfc)/smp
  sfcdsp := $(sfc)/dsp
  sfcppu := $(sfc)/ppu
else ifeq ($(profile),balanced)
  profflags := -DPROFILE_BALANCED
  sfccpu := $(sfc)/cpu
  sfcsmp := $(sfc)/smp
  sfcdsp := $(sfc)/alt/dsp
  sfcppu := $(sfc)/alt/ppu-balanced
else ifeq ($(profile),performance)
  profflags := -DPROFILE_PERFORMANCE
  sfccpu := $(sfc)/alt/cpu
  sfcsmp := $(sfc)/alt/smp
  sfcdsp := $(sfc)/alt/dsp
  sfcppu := $(sfc)/alt/ppu-performance
else
  $(error unknown profile.)
endif

ifeq ($(sfc_lagfix),1)
	flags += -DSFC_LAGFIX
endif

obj/sfc-interface-$(profile).o:       $(sfc)/interface/interface.cpp $(call rwildcard,$(sfc)/interface)
obj/sfc-system-$(profile).o:          $(sfc)/system/system.cpp $(call rwildcard,$(sfc)/system/)
obj/sfc-controller-$(profile).o:      $(sfc)/controller/controller.cpp $(call rwildcard,$(sfc)/controller/)
obj/sfc-cartridge-$(profile).o:       $(sfc)/cartridge/cartridge.cpp $(sfc)/cartridge/*
obj/sfc-cheat-$(profile).o:           $(sfc)/cheat/cheat.cpp $(sfc)/cheat/*
obj/sfc-memory-$(profile).o:          $(sfc)/memory/memory.cpp $(call rwildcard,$(sfc)/memory/)
obj/sfc-cpu-$(profile).o:             $(sfccpu)/cpu.cpp $(call rwildcard,$(sfccpu)/)
obj/sfc-smp-$(profile).o:             $(sfcsmp)/smp.cpp $(call rwildcard,$(sfcsmp)/)
obj/sfc-dsp-$(profile).o:             $(sfcdsp)/dsp.cpp $(call rwildcard,$(sfcdsp)/)
obj/sfc-ppu-$(profile).o:             $(sfcppu)/ppu.cpp $(call rwildcard,$(sfcppu)/)

obj/sfc-satellaviewbase-$(profile).o: $(sfc)/base/satellaview/satellaview.cpp $(call rwildcard,$(sfc)/base/satellaview/)

obj/sfc-icd2-$(profile).o:            $(sfc)/chip/icd2/icd2.cpp $(call rwildcard,$(sfc)/chip/icd2/)
obj/sfc-bsx-$(profile).o:             $(sfc)/chip/bsx/bsx.cpp $(call rwildcard,$(sfc)/chip/bsx/)
obj/sfc-nss-$(profile).o:             $(sfc)/chip/nss/nss.cpp $(call rwildcard,$(sfc)/chip/nss/)
obj/sfc-event-$(profile).o:           $(sfc)/chip/event/event.cpp $(call rwildcard,$(sfc)/chip/event/)

obj/sfc-sa1-$(profile).o:             $(sfc)/chip/sa1/sa1.cpp $(call rwildcard,$(sfc)/chip/sa1/)
obj/sfc-superfx-$(profile).o:         $(sfc)/chip/superfx/superfx.cpp $(call rwildcard,$(sfc)/chip/superfx/)

obj/sfc-armdsp-$(profile).o:          $(sfc)/chip/armdsp/armdsp.cpp $(call rwildcard,$(sfc)/chip/armdsp/)
obj/sfc-hitachidsp-$(profile).o:      $(sfc)/chip/hitachidsp/hitachidsp.cpp $(call rwildcard,$(sfc)/chip/hitachidsp/)
obj/sfc-necdsp-$(profile).o:          $(sfc)/chip/necdsp/necdsp.cpp $(call rwildcard,$(sfc)/chip/necdsp/)

obj/sfc-epsonrtc-$(profile).o:        $(sfc)/chip/epsonrtc/epsonrtc.cpp $(call rwildcard,$(sfc)/chip/epsonrtc/)
obj/sfc-sharprtc-$(profile).o:        $(sfc)/chip/sharprtc/sharprtc.cpp $(call rwildcard,$(sfc)/chip/sharprtc/)

obj/sfc-spc7110-$(profile).o:         $(sfc)/chip/spc7110/spc7110.cpp $(sfc)/chip/spc7110/*
obj/sfc-sdd1-$(profile).o:            $(sfc)/chip/sdd1/sdd1.cpp $(sfc)/chip/sdd1/*
obj/sfc-obc1-$(profile).o:            $(sfc)/chip/obc1/obc1.cpp $(sfc)/chip/obc1/*

obj/sfc-hsu1-$(profile).o:            $(sfc)/chip/hsu1/hsu1.cpp $(sfc)/chip/hsu1/*
obj/sfc-msu1-$(profile).o:            $(sfc)/chip/msu1/msu1.cpp $(sfc)/chip/msu1/*

obj/sfc-satellaviewcart-$(profile).o: $(sfc)/slot/satellaview/satellaview.cpp $(call rwildcard,$(sfc)/slot/satellaview/)
obj/sfc-sufamiturbo-$(profile).o:     $(sfc)/slot/sufamiturbo/sufamiturbo.cpp $(call rwildcard,$(sfc)/slot/sufamiturbo/)

obj/sfc-hledsp1-$(profile).o:         $(sfc)/chip/dsp1/dsp1.cpp $(sfc)/chip/dsp1/*
obj/sfc-hledsp2-$(profile).o:         $(sfc)/chip/dsp2/dsp2.cpp $(sfc)/chip/dsp2/*
obj/sfc-hledsp3-$(profile).o:         $(sfc)/chip/dsp3/dsp3.cpp $(sfc)/chip/dsp3/*
obj/sfc-hledsp4-$(profile).o:         $(sfc)/chip/dsp4/dsp4.cpp $(sfc)/chip/dsp4/*

obj/sfc-hlecx4-$(profile).o:          $(sfc)/chip/cx4/cx4.cpp $(sfc)/chip/cx4/*
obj/sfc-hlest0010-$(profile).o:       $(sfc)/chip/st0010/st0010.cpp $(sfc)/chip/st0010/*

obj/sfc-sgbexternal-$(profile).o:     $(sfc)/chip/sgb-external/sgb-external.cpp $(sfc)/chip/sgb-external/*
