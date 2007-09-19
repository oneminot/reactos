<module name="audiosrv" type="win32cui" installbase="system32" 
installname="audiosrv.exe" allowwarnings="true">
	<include base="audiosrv">.</include>
	<define name="UNICODE" />
	<define name="__USE_W32API" />
	<define name="__REACTOS__" />
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="WINVER">0x0501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>setupapi</library>
	<file>main.c</file>
	<file>pnp_list_manager.c</file>
	<file>pnp_list_lock.c</file>
	<file>pnp.c</file>
	<file>audiosrv.rc</file>
</module>
