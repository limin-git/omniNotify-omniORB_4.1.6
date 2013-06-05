@ECHO OFF

ECHO making COSNotify4d.def

CALL "C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat"

CD C:\_perforce\COTS\omniORB\omniORB_4.1.6\src\services\omniNotify\lib

ECHO LIBRARY shareddebug/COSNotify416_vc9_rtd> tmp.def
ECHO VERSION 1.6>> tmp.def
ECHO EXPORTS>> tmp.def

DUMPBIN.EXE /SYMBOLS debug/COSNotify4d.lib | egrep "^^[^^ ]+ +[^^ ]+ +SECT[^^ ]+ +[^^ ]+ +\(\) +External +\| +\?[^^ ]*|^^[^^ ]+ +[^^ ]+ +SECT[^^ ]+ +[^^ ]+ +External +\| +\?[^^?][^^ ]*" | ^
egrep "^^[^^ ]+ +[^^ ]+ +SECT[^^ ]+ +[^^ ]+ +\(\) +External +\| +\?[^^ ]*|^^[^^ ]+ +[^^ ]+ +SECT[^^ ]+ +[^^ ]+ +External +\| +\?[^^?][^^ ]*" | ^
egrep -v "deleting destructor[^^(]+\(unsigned int\)" | ^
egrep -v "anonymous namespace" | ^
egrep -v "@std@" | ^
cut -d"|" -f2 | ^
cut -d" " -f2 | ^
C:\gnuwin32\bin\sort.exe -u >> tmp.def

IF %ERRORLEVEL% == 0 (
    DEL /Q COSNotify4d_old.def
    REN COSNotify4d.def COSNotify4d_old.def
    REN tmp.def COSNotify4d.def
) ELSE (
    ECHO failed.
)

ECHO DONE.
