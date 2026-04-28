# Boot

The DuetScreen image provides a couple of options to be set on first boot only using the user-accessible FAT32 boot partiton of a burnt SD image. Note that these options are not available when booting from the embedded NAND memory.

[Documentation](https://github.com/Duet3D/buildroot-duetscreen/blob/master/BOOT.md)

This folder contains templates for the files that can be placed on the boot partition to set these options. The files should be copied to the root of the boot partition if you want to enable their respective features, and will be removed after first boot.