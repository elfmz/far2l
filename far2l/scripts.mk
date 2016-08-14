
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
DIRBIT = 64
else
DIRBIT = 32
endif

RM = rm
GAWK = gawk
M4 = m4 -P -DFARBIT=$(DIRBIT)

MYDIR = $(shell pwd)

BUILD = $(MYDIR)/../Build
TOOLS = $(MYDIR)/../tools
BOOTSTRAP = $(MYDIR)/bootstrap
SCRIPTS = $(MYDIR)/scripts


all: lng

lng: $(BOOTSTRAP)/farlang.templ $(BUILD)/FarEng.hlf $(BUILD)/FarRus.hlf $(BUILD)/FarHun.hlf $(BUILD)/File_id.diz $(BOOTSTRAP)/far.rc $(BOOTSTRAP)/copyright.inc $(BOOTSTRAP)/farversion.inc
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(TOOLS)/farlng generator -nc -i $(BOOTSTRAP)/lang.ini -ol $(BUILD) $(BOOTSTRAP)/farlang.templ

$(BOOTSTRAP)/Far.exe.manifest: Far.exe.manifest.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(M4) Far.exe.manifest.m4 > $@

$(BOOTSTRAP)/far.rc: far.rc.m4 farversion.m4 tools.m4 vbuild.m4 res.hpp $(BOOTSTRAP)/Far.exe.manifest
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(M4) far.rc.m4 > $@

$(BOOTSTRAP)/copyright.inc: copyright.inc.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(M4) copyright.inc.m4 | $(GAWK) -f $(SCRIPTS)/enc.awk > $@

$(BOOTSTRAP)/farversion.inc: farversion.inc.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(M4) farversion.inc.m4 > $@

$(BOOTSTRAP)/farlang.templ: farlang.templ.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(M4) farlang.templ.m4 > $@

$(BUILD)/FarEng.hlf: FarEng.hlf.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(GAWK) -f $(SCRIPTS)/mkhlf.awk FarEng.hlf.m4 | $(M4) > $@

$(BUILD)/FarRus.hlf: FarRus.hlf.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(GAWK) -f $(SCRIPTS)/mkhlf.awk FarRus.hlf.m4 | $(M4) > $@

$(BUILD)/FarHun.hlf: FarHun.hlf.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(GAWK) -f $(SCRIPTS)/mkhlf.awk FarHun.hlf.m4 | $(M4) > $@

$(BUILD)/File_id.diz: File_id.diz.m4 farversion.m4 tools.m4 vbuild.m4
	mkdir -p $(BOOTSTRAP) $(BUILD); \
	$(M4) File_id.diz.m4 > $@


clean:
	$(RM) $(BUILD)/*.lng $(BUILD)/*.hlf $(BUILD)/File_id.diz $(BOOTSTRAP)/* || echo "Already clean"

