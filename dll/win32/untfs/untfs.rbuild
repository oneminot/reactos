<module name="untfs" type="win32dll" baseaddress="${BASEADDRESS_UNTFS}" installbase="system32" installname="untfs.dll">
	<importlibrary definition="untfs.spec" />
	<include base="untfs">.</include>
	<library>ntfslib</library>
	<library>ntdll</library>
	<file>untfs.c</file>
	<file>untfs.rc</file>
</module>
