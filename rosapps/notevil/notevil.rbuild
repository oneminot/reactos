<module name="notevil" type="win32cui" installbase="system32" installname="notevil.exe">
	<include base="notevil">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<file>notevil.c</file>
	<file>notevil.rc</file>
</module>