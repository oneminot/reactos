<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="bootsect">
		<xi:include href="bootsect/bootsect.rbuild" />
	</directory>
	<directory name="freeldr">
		<xi:include href="freeldr/freeldr_startup.rbuild" />
		<xi:include href="freeldr/freeldr_base64k.rbuild" />
		<xi:include href="freeldr/freeldr_base.rbuild" />
		<xi:include href="freeldr/freeldr_arch.rbuild" />
		<xi:include href="freeldr/freeldr_main.rbuild" />
		<xi:include href="freeldr/freeldr.rbuild" />
		<if property="ARCH" value="i386">
		    <xi:include href="freeldr/setupldr_main.rbuild" />
		    <xi:include href="freeldr/setupldr.rbuild" />		
		</if>
		<if property="ARCH" value="ppc">
		    <xi:include href="freeldr/setupldr_main.rbuild" />
		    <xi:include href="freeldr/setupldr.rbuild" />		
		</if>
		<if property="ARCH" value="amd64">
		    <xi:include href="freeldr/setupldr_main.rbuild" />
		    <xi:include href="freeldr/setupldr.rbuild" />		
		</if>
	</directory>
</group>
