<module name="roscalc" type="win32gui" installbase="system32" installname="roscalc.exe" allowwarnings="true">
	<include base="roscalc">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<file>about.c</file>
	<file>function.c</file>
	<file>rpn.c</file>
	<file>utl.c</file>
	<file>winmain.c</file>
	<file>resource.rc</file>
</module>
