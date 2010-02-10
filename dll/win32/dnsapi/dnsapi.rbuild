<module name="dnsapi" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_DNSAPI}" installbase="system32" installname="dnsapi.dll" crt="msvcrt">
	<importlibrary definition="dnsapi.spec" />
	<include base="dnsapi">include</include>
	<include base="adns">src</include>
	<include base="adns">adns_win32</include>
	<define name="ADNS_JGAA_WIN32" />
	<library>adns</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<directory name="dnsapi">
		<file>adns.c</file>
		<file>context.c</file>
		<file>memory.c</file>
		<file>names.c</file>
		<file>query.c</file>
		<file>stubs.c</file>
		<pch>precomp.h</pch>
	</directory>
	<file>dnsapi.rc</file>
</module>
