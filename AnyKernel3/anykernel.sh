### AnyKernel3 Ramdisk Mod Script
## osm0sis @ xda-developers
## configured for Samsung Galaxy M11 (SM-M115F / SDM450) — kernel 4.9.337
## + KernelSU-Next v1.1.1 (KSU_VERSION 12851). Same RNG fix, same dtb set
## as the non-KSU release. Install the official KernelSU-Next manager app
## to grant su -- the manager signature hash baked into this build is
## upstream's default.

### AnyKernel setup
# global properties
properties() { '
kernel.string=Linux 4.9.337-rng-fix + KernelSU-Next v1.1.1 (12851), clang10
do.devicecheck=1
do.modules=0
do.systemless=1
do.cleanup=1
do.cleanuponabort=0
device.name1=m11q
device.name2=m11
device.name3=m11nsxx
device.name4=m11ltn
device.name5=m11ins
device.name6=m11tn
device.name7=m11ns
device.name8=sdm450
device.name9=a11q
supported.versions=
supported.patchlevels=
supported.vendorpatchlevels=
'; } # end properties

### AnyKernel install
## boot files attributes
boot_attributes() {
set_perm_recursive 0 0 755 644 $RAMDISK/*;
set_perm_recursive 0 0 750 750 $RAMDISK/init* $RAMDISK/sbin;
} # end attributes

# boot shell variables
BLOCK=/dev/block/bootdevice/by-name/boot;
IS_SLOT_DEVICE=0;
RAMDISK_COMPRESSION=auto;
PATCH_VBMETA_FLAG=auto;

# import functions/variables and setup patching - see for reference (DO NOT REMOVE)
. tools/ak3-core.sh;

# boot install
dump_boot; # unpack the current boot.img, keeping its ramdisk + header params

write_boot; # repack with the new kernel, reusing the on-device ramdisk/boot layout
## end boot install
