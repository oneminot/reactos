<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="cmidriver" type="kernelmodedriver" installbase="system32/drivers" installname="cmipci.sys" allowwarnings="true">

	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>

	<group compilerset="msc">
		<compilerflag compiler="cxx">/GR-</compilerflag>
	</group>

	<include base="mpu401">.</include>
	<library>ntoskrnl</library>
	<library>portcls</library>
	<library>hal</library>
	<library>ks</library>
	<file>adapter.cpp</file>
	<file>common.cpp</file>
	<file>mintopo.cpp</file>
	<file>minwave.cpp</file>
	<file>cmipci.rc</file>
</module>