<module name="ntprint_winetest" type="win32cui" installbase="bin" installname="ntprint_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="ntprint_winetest">.</include>
	<file>ntprint.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
