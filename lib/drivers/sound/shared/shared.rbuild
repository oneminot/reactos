<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="audio" type="staticlibrary" allowwarnings="true">
    <define name="__NTDRIVER__"/>
    <define name="KERNEL"/>
    <include base="soundblaster">.</include>
    <include base="ReactOS">include/reactos/libs/sound</include>
    <file>time.c</file>
</module>
