# Microsoft Developer Studio Generated NMAKE File, Based on ftp.dsp
!IF "$(CFG)" == ""
CFG=ftp - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ftp - Win32 Debug.
!ENDIF

!IF "$(CFG)" != "ftp - Win32 Release" && "$(CFG)" != "ftp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "ftp.mak" CFG="ftp - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "ftp - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ftp - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE
!ERROR An invalid configuration is specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

!IF  "$(CFG)" == "ftp - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0"

ALL : "$(OUTDIR)\ftp.exe"

!ELSE

ALL : "$(OUTDIR)\ftp.exe"

!ENDIF

CLEAN :
	-@erase "$(INTDIR)\cmds.obj"
	-@erase "$(INTDIR)\cmdtab.obj"
	-@erase "$(INTDIR)\domacro.obj"
	-@erase "$(INTDIR)\fake.obj"
	-@erase "$(INTDIR)\ftp.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\ruserpass.obj"
	-@erase "$(INTDIR)\vc*.idb"
	-@erase "$(OUTDIR)\ftp.exe"
	-@erase "$(OUTDIR)\ftp.pch"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2  /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" \
 /D "HAVE_TIMEVAL" /Fp"$(INTDIR)\ftp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ftp.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\ftp.pdb" /machine:I386 /out:"$(OUTDIR)\ftp.exe"
LINK32_OBJS= \
	"$(INTDIR)\cmds.obj" \
	"$(INTDIR)\cmdtab.obj" \
	"$(INTDIR)\domacro.obj" \
	"$(INTDIR)\fake.obj" \
	"$(INTDIR)\ftp.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\ruserpass.obj"

"$(OUTDIR)\ftp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\ftp.exe
InputPath=.\Release\ftp.exe
SOURCE=$(InputPath)

!ELSEIF  "$(CFG)" == "ftp - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0"

ALL : "$(OUTDIR)\ftp.exe"

!ELSE

ALL : "$(OUTDIR)\ftp.exe"

!ENDIF

CLEAN :
	-@erase "$(INTDIR)\cmds.obj"
	-@erase "$(INTDIR)\cmdtab.obj"
	-@erase "$(INTDIR)\domacro.obj"
	-@erase "$(INTDIR)\fake.obj"
	-@erase "$(INTDIR)\ftp.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\ruserpass.obj"
	-@erase "$(INTDIR)\vc*.idb"
	-@erase "$(INTDIR)\vc*.pdb"
	-@erase "$(OUTDIR)\ftp.exe"
	-@erase "$(OUTDIR)\ftp.ilk"
	-@erase "$(OUTDIR)\ftp.pdb"
	-@erase "$(OUTDIR)\ftp.pch"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS"\
 /Fp"$(INTDIR)\ftp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ftp.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ftp.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ftp.exe"\
 /pdbtype:sept
LINK32_OBJS= \
	"$(INTDIR)\cmds.obj" \
	"$(INTDIR)\cmdtab.obj" \
	"$(INTDIR)\domacro.obj" \
	"$(INTDIR)\fake.obj" \
	"$(INTDIR)\ftp.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\ruserpass.obj"

"$(OUTDIR)\ftp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Debug\ftp.exe
InputPath=.\Debug\ftp.exe
SOURCE=$(InputPath)

!ENDIF


!IF "$(CFG)" == "ftp - Win32 Release" || "$(CFG)" == "ftp - Win32 Debug"
SOURCE=.\cmds.c

!IF  "$(CFG)" == "ftp - Win32 Release"

DEP_CPP_CMDS_=\
	".\fake.h"\
	".\ftp_var.h"\
	".\pathnames.h"\
	".\prototypes.h"\


"$(INTDIR)\cmds.obj" : $(SOURCE) $(DEP_CPP_CMDS_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ftp - Win32 Debug"

DEP_CPP_CMDS_=\
	".\fake.h"\
	".\ftp_var.h"\
	".\pathnames.h"\
	".\prototypes.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\


"$(INTDIR)\cmds.obj" : $(SOURCE) $(DEP_CPP_CMDS_) "$(INTDIR)"


!ENDIF

SOURCE=.\cmdtab.c
DEP_CPP_CMDTA=\
	".\fake.h"\
	".\ftp_var.h"\


"$(INTDIR)\cmdtab.obj" : $(SOURCE) $(DEP_CPP_CMDTA) "$(INTDIR)"


SOURCE=.\domacro.c
DEP_CPP_DOMAC=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\


"$(INTDIR)\domacro.obj" : $(SOURCE) $(DEP_CPP_DOMAC) "$(INTDIR)"


SOURCE=.\fake.c
DEP_CPP_FAKE_=\
	".\prototypes.h"\


"$(INTDIR)\fake.obj" : $(SOURCE) $(DEP_CPP_FAKE_) "$(INTDIR)"


SOURCE=.\ftp.c

!IF  "$(CFG)" == "ftp - Win32 Release"

DEP_CPP_FTP_C=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\


"$(INTDIR)\ftp.obj" : $(SOURCE) $(DEP_CPP_FTP_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ftp - Win32 Debug"

DEP_CPP_FTP_C=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\


"$(INTDIR)\ftp.obj" : $(SOURCE) $(DEP_CPP_FTP_C) "$(INTDIR)"


!ENDIF

SOURCE=.\main.c

!IF  "$(CFG)" == "ftp - Win32 Release"

DEP_CPP_MAIN_=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\


"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ftp - Win32 Debug"

DEP_CPP_MAIN_=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\
	{$(INCLUDE)}"sys\types.h"\


"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF

SOURCE=.\ruserpass.c

!IF  "$(CFG)" == "ftp - Win32 Release"

DEP_CPP_RUSER=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\


"$(INTDIR)\ruserpass.obj" : $(SOURCE) $(DEP_CPP_RUSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ftp - Win32 Debug"

DEP_CPP_RUSER=\
	".\fake.h"\
	".\ftp_var.h"\
	".\prototypes.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\


"$(INTDIR)\ruserpass.obj" : $(SOURCE) $(DEP_CPP_RUSER) "$(INTDIR)"


!ENDIF


!ENDIF
