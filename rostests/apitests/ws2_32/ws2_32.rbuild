<module name="ws2_32_apitests" type="win32cui" unicode="yes">
	<define name="_WIN32_WINNT">0x0501</define>
	<library>apitest</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>shell32</library>
	<library>ws2_32</library>
	<file>helpers.c</file>
	<file>testlist.c</file>
	<file>ws2_32.c</file>
</module>
