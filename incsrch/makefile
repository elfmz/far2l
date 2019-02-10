.ERASE
.SILENT
.EXTENSIONS: $(EXTENSIONS) .res .rc

Name=IncSrch
Target = $(Name).dll
DllPath = C:\TOOLS\Far\Plugins\Editor\IncSrch
bt = NT
O = OBJ
.obj: $(O)
.res: $(O)

Objs =             &
      IncSrch.obj  &
      Search.obj   &
      locale.obj   &
      control.obj  &
      clip.obj     &

Ress = $(O)\$(Name).res

Lnk = $(Name).lnk

!ifndef map
map = 0
!endif
!ifndef debug
debug = 0
!endif
!ifndef p5profile
p5profile = 0
!endif

Compile_Flags = /zq/wx/we/bt=$(bt)/mf/6r/s/wx/we/zq/zc/zl
#/zc
Compile_Flags_C   = $(Compile_Flags)/fhq=$(O)\c.pch
Compile_Flags_Cpp = $(Compile_Flags)/zv/fhq=$(O)\cpp.pch
Msm6Compile = /c /w /nologo /D$(bt)

!ifeq debug 1
Compile_Flags += /d2
__LINK_DEB__ = D All
Optimize_Flags = /od
Msm6Compile +=  /Zi
!else
Optimize_Flags = /oe$(Opt_inline)athbmikl+r/fp6
!endif

!ifeq p5profile 1
Compile_Flags += /et/d__SW_ET
Linker_Flags = ref __DLLstart_
Opt_inline = 0
!endif

!ifeq map 1
__LINK_MAP__ = op map
!endif

!ifdef __LOADDLL__
!  loaddll wcc      wccd
!  loaddll wccaxp   wccdaxp
!  loaddll wcc386   wccd386
!  loaddll wpp      wppdi86
!  loaddll wppaxp   wppdaxp
!  loaddll wpp386   wppd386
!  loaddll wlink    wlink
!  loaddll wlib     wlibd
!endif

$(Target) : $(O) $(Objs) $(Ress) $(Lnk)
    -echo .linking $@
    *wlink @$(Lnk) op res=$(Ress) name $@ $(__LINK_DEB__) p $(O) file {$(Objs)} $(Linker_Flags) $(__LINK_MAP__)

$(O) :
    -if not exist $(O)\nul mkdir $(O)

clean : .SYMBOLIC
    -del $(O)\*.obj
    -del $(O)\*.res
    -del $(O)\*.pch
    -rmdir $(O)
    -del $(Target)

test : .SYMBOLIC
    %make $(Target)
    -if exist $(DllPath)\IncSrch.1 del $(DllPath)\IncSrch.1
    -ren $(DllPath)\IncSrch.dll *.1
    -copy $(Target) $(DllPath)\*.*

.cpp.obj : .AUTODEPEND
    -echo .compiling $]@
    *wpp386 $]@ /fo=$(O)\$^. $(Compile_Flags_Cpp)$(Optimize_flags)

.c.obj : .AUTODEPEND
    -echo .compiling $]@
    *wcc386 $]@ /fo=$(O)\$^. $(Compile_Flags_C)$(Optimize_flags)

.asm.obj : .AUTODEPEND
    -echo .assembing $]@
    ml $(Msm6Compile) /Fo$(O)\$^. $]@

.rc.res : .AUTODEPEND
    -echo .invoke resource compiler on $]@
    wrc /r /q /bt=$(bt) /fo=$(O)\$^. $]@
