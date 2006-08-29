<module name="imm32" type="win32dll" baseaddress="${BASEADDRESS_IMM32}" installbase="system32" installname="imm32.dll">
	<importlibrary definition="imm32.spec.def" />
	<include base="imm32">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>imm.c</file>
	<file>imm32.spec</file>
</module>
