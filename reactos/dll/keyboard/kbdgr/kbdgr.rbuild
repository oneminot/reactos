<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgr" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdgr.dll" allowwarnings="true">
	<importlibrary definition="kbdgr.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdgr.c</file>
	<file>kbdgr.rc</file>
</module>
