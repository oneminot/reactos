<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cabinet" type="win32dll" baseaddress="${BASEADDRESS_CABINET}" installbase="system32" installname="cabinet.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="cabinet.spec" />
	<include base="cabinet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>cabinet_main.c</file>
	<file>fci.c</file>
	<file>fdi.c</file>
	<file>stubs.c</file>
	<file>cabinet.rc</file>
	<pch>cabinet.h</pch>
	<library>wine</library>
	<library>ntdll</library>
</module>
</group>
