<module name="psapi_winetest" type="win32cui" installbase="bin" installname="psapi_winetest.exe" allowwarnings="true">
    <include base="psapi_winetest">.</include>
    <define name="__USE_W32API" />
    <library>ntdll</library>
    <library>psapi</library>
    <file>testlist.c</file>
    <file>module.c</file>
</module>
