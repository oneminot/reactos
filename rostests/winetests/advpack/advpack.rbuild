<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="advpack_winetest" type="win32cui" installbase="bin" installname="advpack_winetest.exe" allowwarnings="true">
	<include base="advpack_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>advpack.c</file>
	<file>files.c</file>
	<file>install.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>cabinet</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
