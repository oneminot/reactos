<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<project name="ReactOS" makefile="makefile.auto" xmlns:xi="http://www.w3.org/2001/XInclude">
  <xi:include href="config.rbuild">
    <xi:fallback>
      <xi:include href="config.template.rbuild" />
    </xi:fallback>
  </xi:include>

  <xi:include href="baseaddress.rbuild" />

  <define name="_M_IX86" />
  <define name="_X86_" />
  <define name="__i386__" />
  <define name="_REACTOS_" />
  <if property="MP" value="1">
    <define name="CONFIG_SMP" value="1" />
  </if>
  <if property="DBG" value="1">
    <define name="DBG" value="1" />
    <property name="DBG_OR_KDBG" value="true" />
  </if>
  <if property="KDBG" value="1">
    <define name="KDBG" value="1" />
    <property name="DBG_OR_KDBG" value="true" />
  </if>
  <compilerflag>-Os</compilerflag>
  <compilerflag>-Wno-strict-aliasing</compilerflag>
  <compilerflag>-ftracer</compilerflag>
  <compilerflag>-momit-leaf-frame-pointer</compilerflag>
  <compilerflag>-mpreferred-stack-boundary=2</compilerflag>
  <compilerflag>-Wpointer-arith</compilerflag>

  <include>.</include>
  <include>include</include>
  <include>include/reactos</include>
  <include>include/libs</include>
  <include>include/drivers</include>
  <include>include/subsys</include>
  <include>include/ndk</include>
  <include>w32api/include</include>
  <include>w32api/include/crt</include>
  <include>w32api/include/ddk</include>

<!-- TODO
  <directory name="apps">
    <xi:include href="apps/directory.rbuild" />
  </directory>
  <directory name="boot">
    <xi:include href="boot/boot.rbuild" />
  </directory>
-->
  <directory name="boot">
    <xi:include href="boot/boot.rbuild" />
  </directory>
  <directory name="base">
    <xi:include href="base/base.rbuild" />
  </directory>
<!-- TODO
  <directory name="dll">
    <xi:include href="dll/dll.rbuild" />
  </directory>
  <directory name="drivers">
    <xi:include href="drivers/directory.rbuild" />
  </directory>
  <directory name="hal">
    <xi:include href="hal/directory.rbuild" />
  </directory>
  <directory name="include">
    <xi:include href="include/directory.rbuild" />
  </directory>
-->
  <directory name="lib">
    <xi:include href="lib/lib.rbuild" />
  </directory>
  <directory name="media">
    <xi:include href="media/media.rbuild" />
  </directory>
<!-- TODO
  <directory name="modules">
    <xi:include href="modules/directory.rbuild" />
  </directory>
  <directory name="ntoskrnl">
    <xi:include href="ntoskrnl/ntoskrnl.rbuild" />
  </directory>
  <directory name="regtests">
    <xi:include href="regtests/directory.rbuild" />
  </directory>
-->
  <directory name="subsys">
    <xi:include href="subsys/subsys.rbuild" />
  </directory>
<!-- NOT DONE YET
  <directory name="subsystems">
    <xi:include href="subsystems/subsystems.rbuild" />
  </directory>
-->

</project>
