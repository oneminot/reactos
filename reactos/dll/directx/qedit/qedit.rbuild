<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="qedit" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_QEDIT}" installbase="system32" installname="qedit.dll" allowwarnings="true">
	<importlibrary definition="qedit.spec" />
	<include base="qedit">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>qedit.c</file>
	<file>qedit.rc</file>
</module>
</group>
