<module name="msports" type="win32dll" baseaddress="${BASEADDRESS_MSPORTS}" installbase="system32" installname="msports.dll" unicode="yes">
	<include base="msports">.</include>
	<importlibrary definition="msports.spec" />
	<library>wine</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>user32</library>
	<library>setupapi</library>
	<file>classinst.c</file>
	<file>comdb.c</file>
	<file>msports.c</file>
	<file>serial.c</file>
	<file>msports.rc</file>
	<pch>precomp.h</pch>
</module>
