	.arch armv8-a
	.file	"devicetable-offsets.c"
// GNU C89 (Linaro GCC 7.2-2017.11-rc1) version 7.2.1 20171011 (aarch64-linux-gnu)
//	compiled by GNU C version 6.2.0, GMP version 6.0.0, MPFR version 3.1.3, MPC version 1.0.3, isl version none
// GGC heuristics: --param ggc-min-expand=30 --param ggc-min-heapsize=4096
// options passed:  -nostdinc -I ./arch/arm64/include
// -I ./arch/arm64/include/generated -I ./include
// -I ./arch/arm64/include/uapi -I ./arch/arm64/include/generated/uapi
// -I ./include/uapi -I ./include/generated/uapi
// -I /home/chinkey/git_stuff/opencpi/runtime/hdl/include
// -I /home/chinkey/git_stuff/opencpi/os/interfaces/include
// -imultiarch aarch64-linux-gnu
// -iprefix /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/../lib/gcc/aarch64-linux-gnu/7.2.1/
// -isysroot /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/../aarch64-linux-gnu/libc
// -D __KERNEL__ -D CONFIG_AS_LSE=1 -D CC_HAVE_ASM_GOTO
// -D KBUILD_BASENAME="devicetable_offsets"
// -D KBUILD_MODNAME="devicetable_offsets"
// -isystem /opt/Xilinx/SDK/2018.2/gnu/aarch64/lin/aarch64-linux/bin/../lib/gcc/aarch64-linux-gnu/7.2.1/include
// -include ./include/linux/kconfig.h
// -MD scripts/mod/.devicetable-offsets.s.d
// scripts/mod/devicetable-offsets.c -mlittle-endian -mgeneral-regs-only
// -mpc-relative-literal-loads -mabi=lp64 -march=armv8-a
// -auxbase-strip scripts/mod/devicetable-offsets.s -g -O2 -Wformat=2 -Wall
// -Wundef -Wstrict-prototypes -Wno-trigraphs
// -Werror=implicit-function-declaration -Wno-format-security
// -Wno-frame-address -Wformat-truncation=0 -Wformat-overflow=0
// -Wno-int-in-bool-context -Wframe-larger-than=2048
// -Wno-unused-but-set-variable -Wunused-const-variable=0
// -Wdeclaration-after-statement -Wno-pointer-sign -Werror=implicit-int
// -Werror=strict-prototypes -Werror=date-time
// -Werror=incompatible-pointer-types -Werror=designated-init -std=gnu90
// -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE
// -fno-asynchronous-unwind-tables -fno-delete-null-pointer-checks
// -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls
// -fno-var-tracking-assignments -fno-strict-overflow -fconserve-stack
// -fverbose-asm --param allow-store-data-races=0
// options enabled:  -faggressive-loop-optimizations -falign-labels
// -fauto-inc-dec -fbranch-count-reg -fcaller-saves
// -fchkp-check-incomplete-type -fchkp-check-read -fchkp-check-write
// -fchkp-instrument-calls -fchkp-narrow-bounds -fchkp-optimize
// -fchkp-store-bounds -fchkp-use-static-bounds
// -fchkp-use-static-const-bounds -fchkp-use-wrappers -fcode-hoisting
// -fcombine-stack-adjustments -fcompare-elim -fcprop-registers
// -fcrossjumping -fcse-follow-jumps -fdefer-pop -fdevirtualize
// -fdevirtualize-speculatively -fdwarf2-cfi-asm -fearly-inlining
// -feliminate-unused-debug-types -fexpensive-optimizations
// -fforward-propagate -ffp-int-builtin-inexact -ffunction-cse -fgcse
// -fgcse-lm -fgnu-runtime -fgnu-unique -fguess-branch-probability
// -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
// -findirect-inlining -finline -finline-atomics
// -finline-functions-called-once -finline-small-functions -fipa-bit-cp
// -fipa-cp -fipa-icf -fipa-icf-functions -fipa-icf-variables -fipa-profile
// -fipa-pure-const -fipa-ra -fipa-reference -fipa-sra -fipa-vrp
// -fira-hoist-pressure -fira-share-save-slots -fira-share-spill-slots
// -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
// -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
// -fmath-errno -fmerge-constants -fmerge-debug-strings
// -fmove-loop-invariants -fomit-frame-pointer -foptimize-strlen
// -fpartial-inlining -fpeephole -fpeephole2 -fplt -fprefetch-loop-arrays
// -free -freg-struct-return -freorder-blocks -freorder-functions
// -frerun-cse-after-loop -fsched-critical-path-heuristic
// -fsched-dep-count-heuristic -fsched-group-heuristic -fsched-interblock
// -fsched-last-insn-heuristic -fsched-pressure -fsched-rank-heuristic
// -fsched-spec -fsched-spec-insn-heuristic -fsched-stalled-insns-dep
// -fschedule-fusion -fschedule-insns -fschedule-insns2 -fsection-anchors
// -fsemantic-interposition -fshow-column -fshrink-wrap
// -fshrink-wrap-separate -fsigned-zeros -fsplit-ivs-in-unroller
// -fsplit-wide-types -fssa-backprop -fssa-phiopt -fstdarg-opt
// -fstore-merging -fstrict-volatile-bitfields -fsync-libcalls
// -fthread-jumps -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp
// -ftree-builtin-call-dce -ftree-ccp -ftree-ch -ftree-coalesce-vars
// -ftree-copy-prop -ftree-cselim -ftree-dce -ftree-dominator-opts
// -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-if-convert
// -ftree-loop-im -ftree-loop-ivcanon -ftree-loop-optimize
// -ftree-parallelize-loops= -ftree-phiprop -ftree-pre -ftree-pta
// -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr -ftree-sra
// -ftree-switch-conversion -ftree-tail-merge -ftree-ter -ftree-vrp
// -funit-at-a-time -fvar-tracking -fverbose-asm -fzero-initialized-in-bss
// -mfix-cortex-a53-835769 -mfix-cortex-a53-843419 -mgeneral-regs-only
// -mglibc -mlittle-endian -momit-leaf-frame-pointer
// -mpc-relative-literal-loads

	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.startup,"ax",@progbits
	.align	2
	.p2align 3,,7
	.global	main
	.type	main, %function
main:
.LFB19:
	.file 1 "scripts/mod/devicetable-offsets.c"
	.loc 1 10 0
	.cfi_startproc
// scripts/mod/devicetable-offsets.c:11: 	DEVID(usb_device_id);
	.loc 1 11 0
#APP
// 11 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_usb_device_id 32 sizeof(struct usb_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:12: 	DEVID_FIELD(usb_device_id, match_flags);
	.loc 1 12 0
// 12 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_match_flags 0 offsetof(struct usb_device_id, match_flags)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:13: 	DEVID_FIELD(usb_device_id, idVendor);
	.loc 1 13 0
// 13 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_idVendor 2 offsetof(struct usb_device_id, idVendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:14: 	DEVID_FIELD(usb_device_id, idProduct);
	.loc 1 14 0
// 14 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_idProduct 4 offsetof(struct usb_device_id, idProduct)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:15: 	DEVID_FIELD(usb_device_id, bcdDevice_lo);
	.loc 1 15 0
// 15 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bcdDevice_lo 6 offsetof(struct usb_device_id, bcdDevice_lo)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:16: 	DEVID_FIELD(usb_device_id, bcdDevice_hi);
	.loc 1 16 0
// 16 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bcdDevice_hi 8 offsetof(struct usb_device_id, bcdDevice_hi)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:17: 	DEVID_FIELD(usb_device_id, bDeviceClass);
	.loc 1 17 0
// 17 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bDeviceClass 10 offsetof(struct usb_device_id, bDeviceClass)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:18: 	DEVID_FIELD(usb_device_id, bDeviceSubClass);
	.loc 1 18 0
// 18 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bDeviceSubClass 11 offsetof(struct usb_device_id, bDeviceSubClass)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:19: 	DEVID_FIELD(usb_device_id, bDeviceProtocol);
	.loc 1 19 0
// 19 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bDeviceProtocol 12 offsetof(struct usb_device_id, bDeviceProtocol)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:20: 	DEVID_FIELD(usb_device_id, bInterfaceClass);
	.loc 1 20 0
// 20 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bInterfaceClass 13 offsetof(struct usb_device_id, bInterfaceClass)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:21: 	DEVID_FIELD(usb_device_id, bInterfaceSubClass);
	.loc 1 21 0
// 21 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bInterfaceSubClass 14 offsetof(struct usb_device_id, bInterfaceSubClass)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:22: 	DEVID_FIELD(usb_device_id, bInterfaceProtocol);
	.loc 1 22 0
// 22 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bInterfaceProtocol 15 offsetof(struct usb_device_id, bInterfaceProtocol)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:23: 	DEVID_FIELD(usb_device_id, bInterfaceNumber);
	.loc 1 23 0
// 23 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_usb_device_id_bInterfaceNumber 16 offsetof(struct usb_device_id, bInterfaceNumber)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:25: 	DEVID(hid_device_id);
	.loc 1 25 0
// 25 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_hid_device_id 24 sizeof(struct hid_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:26: 	DEVID_FIELD(hid_device_id, bus);
	.loc 1 26 0
// 26 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hid_device_id_bus 0 offsetof(struct hid_device_id, bus)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:27: 	DEVID_FIELD(hid_device_id, group);
	.loc 1 27 0
// 27 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hid_device_id_group 2 offsetof(struct hid_device_id, group)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:28: 	DEVID_FIELD(hid_device_id, vendor);
	.loc 1 28 0
// 28 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hid_device_id_vendor 4 offsetof(struct hid_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:29: 	DEVID_FIELD(hid_device_id, product);
	.loc 1 29 0
// 29 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hid_device_id_product 8 offsetof(struct hid_device_id, product)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:31: 	DEVID(ieee1394_device_id);
	.loc 1 31 0
// 31 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_ieee1394_device_id 32 sizeof(struct ieee1394_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:32: 	DEVID_FIELD(ieee1394_device_id, match_flags);
	.loc 1 32 0
// 32 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ieee1394_device_id_match_flags 0 offsetof(struct ieee1394_device_id, match_flags)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:33: 	DEVID_FIELD(ieee1394_device_id, vendor_id);
	.loc 1 33 0
// 33 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ieee1394_device_id_vendor_id 4 offsetof(struct ieee1394_device_id, vendor_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:34: 	DEVID_FIELD(ieee1394_device_id, model_id);
	.loc 1 34 0
// 34 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ieee1394_device_id_model_id 8 offsetof(struct ieee1394_device_id, model_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:35: 	DEVID_FIELD(ieee1394_device_id, specifier_id);
	.loc 1 35 0
// 35 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ieee1394_device_id_specifier_id 12 offsetof(struct ieee1394_device_id, specifier_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:36: 	DEVID_FIELD(ieee1394_device_id, version);
	.loc 1 36 0
// 36 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ieee1394_device_id_version 16 offsetof(struct ieee1394_device_id, version)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:38: 	DEVID(pci_device_id);
	.loc 1 38 0
// 38 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_pci_device_id 32 sizeof(struct pci_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:39: 	DEVID_FIELD(pci_device_id, vendor);
	.loc 1 39 0
// 39 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pci_device_id_vendor 0 offsetof(struct pci_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:40: 	DEVID_FIELD(pci_device_id, device);
	.loc 1 40 0
// 40 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pci_device_id_device 4 offsetof(struct pci_device_id, device)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:41: 	DEVID_FIELD(pci_device_id, subvendor);
	.loc 1 41 0
// 41 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pci_device_id_subvendor 8 offsetof(struct pci_device_id, subvendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:42: 	DEVID_FIELD(pci_device_id, subdevice);
	.loc 1 42 0
// 42 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pci_device_id_subdevice 12 offsetof(struct pci_device_id, subdevice)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:43: 	DEVID_FIELD(pci_device_id, class);
	.loc 1 43 0
// 43 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pci_device_id_class 16 offsetof(struct pci_device_id, class)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:44: 	DEVID_FIELD(pci_device_id, class_mask);
	.loc 1 44 0
// 44 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pci_device_id_class_mask 20 offsetof(struct pci_device_id, class_mask)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:46: 	DEVID(ccw_device_id);
	.loc 1 46 0
// 46 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_ccw_device_id 16 sizeof(struct ccw_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:47: 	DEVID_FIELD(ccw_device_id, match_flags);
	.loc 1 47 0
// 47 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ccw_device_id_match_flags 0 offsetof(struct ccw_device_id, match_flags)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:48: 	DEVID_FIELD(ccw_device_id, cu_type);
	.loc 1 48 0
// 48 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ccw_device_id_cu_type 2 offsetof(struct ccw_device_id, cu_type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:49: 	DEVID_FIELD(ccw_device_id, cu_model);
	.loc 1 49 0
// 49 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ccw_device_id_cu_model 6 offsetof(struct ccw_device_id, cu_model)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:50: 	DEVID_FIELD(ccw_device_id, dev_type);
	.loc 1 50 0
// 50 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ccw_device_id_dev_type 4 offsetof(struct ccw_device_id, dev_type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:51: 	DEVID_FIELD(ccw_device_id, dev_model);
	.loc 1 51 0
// 51 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ccw_device_id_dev_model 7 offsetof(struct ccw_device_id, dev_model)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:53: 	DEVID(ap_device_id);
	.loc 1 53 0
// 53 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_ap_device_id 16 sizeof(struct ap_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:54: 	DEVID_FIELD(ap_device_id, dev_type);
	.loc 1 54 0
// 54 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ap_device_id_dev_type 2 offsetof(struct ap_device_id, dev_type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:56: 	DEVID(css_device_id);
	.loc 1 56 0
// 56 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_css_device_id 16 sizeof(struct css_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:57: 	DEVID_FIELD(css_device_id, type);
	.loc 1 57 0
// 57 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_css_device_id_type 1 offsetof(struct css_device_id, type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:59: 	DEVID(serio_device_id);
	.loc 1 59 0
// 59 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_serio_device_id 4 sizeof(struct serio_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:60: 	DEVID_FIELD(serio_device_id, type);
	.loc 1 60 0
// 60 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_serio_device_id_type 0 offsetof(struct serio_device_id, type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:61: 	DEVID_FIELD(serio_device_id, proto);
	.loc 1 61 0
// 61 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_serio_device_id_proto 3 offsetof(struct serio_device_id, proto)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:62: 	DEVID_FIELD(serio_device_id, id);
	.loc 1 62 0
// 62 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_serio_device_id_id 2 offsetof(struct serio_device_id, id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:63: 	DEVID_FIELD(serio_device_id, extra);
	.loc 1 63 0
// 63 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_serio_device_id_extra 1 offsetof(struct serio_device_id, extra)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:65: 	DEVID(acpi_device_id);
	.loc 1 65 0
// 65 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_acpi_device_id 32 sizeof(struct acpi_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:66: 	DEVID_FIELD(acpi_device_id, id);
	.loc 1 66 0
// 66 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_acpi_device_id_id 0 offsetof(struct acpi_device_id, id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:67: 	DEVID_FIELD(acpi_device_id, cls);
	.loc 1 67 0
// 67 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_acpi_device_id_cls 24 offsetof(struct acpi_device_id, cls)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:68: 	DEVID_FIELD(acpi_device_id, cls_msk);
	.loc 1 68 0
// 68 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_acpi_device_id_cls_msk 28 offsetof(struct acpi_device_id, cls_msk)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:70: 	DEVID(pnp_device_id);
	.loc 1 70 0
// 70 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_pnp_device_id 16 sizeof(struct pnp_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:71: 	DEVID_FIELD(pnp_device_id, id);
	.loc 1 71 0
// 71 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pnp_device_id_id 0 offsetof(struct pnp_device_id, id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:73: 	DEVID(pnp_card_device_id);
	.loc 1 73 0
// 73 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_pnp_card_device_id 80 sizeof(struct pnp_card_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:74: 	DEVID_FIELD(pnp_card_device_id, devs);
	.loc 1 74 0
// 74 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pnp_card_device_id_devs 16 offsetof(struct pnp_card_device_id, devs)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:76: 	DEVID(pcmcia_device_id);
	.loc 1 76 0
// 76 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_pcmcia_device_id 80 sizeof(struct pcmcia_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:77: 	DEVID_FIELD(pcmcia_device_id, match_flags);
	.loc 1 77 0
// 77 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_match_flags 0 offsetof(struct pcmcia_device_id, match_flags)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:78: 	DEVID_FIELD(pcmcia_device_id, manf_id);
	.loc 1 78 0
// 78 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_manf_id 2 offsetof(struct pcmcia_device_id, manf_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:79: 	DEVID_FIELD(pcmcia_device_id, card_id);
	.loc 1 79 0
// 79 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_card_id 4 offsetof(struct pcmcia_device_id, card_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:80: 	DEVID_FIELD(pcmcia_device_id, func_id);
	.loc 1 80 0
// 80 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_func_id 6 offsetof(struct pcmcia_device_id, func_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:81: 	DEVID_FIELD(pcmcia_device_id, function);
	.loc 1 81 0
// 81 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_function 7 offsetof(struct pcmcia_device_id, function)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:82: 	DEVID_FIELD(pcmcia_device_id, device_no);
	.loc 1 82 0
// 82 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_device_no 8 offsetof(struct pcmcia_device_id, device_no)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:83: 	DEVID_FIELD(pcmcia_device_id, prod_id_hash);
	.loc 1 83 0
// 83 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_pcmcia_device_id_prod_id_hash 12 offsetof(struct pcmcia_device_id, prod_id_hash)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:85: 	DEVID(of_device_id);
	.loc 1 85 0
// 85 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_of_device_id 200 sizeof(struct of_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:86: 	DEVID_FIELD(of_device_id, name);
	.loc 1 86 0
// 86 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_of_device_id_name 0 offsetof(struct of_device_id, name)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:87: 	DEVID_FIELD(of_device_id, type);
	.loc 1 87 0
// 87 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_of_device_id_type 32 offsetof(struct of_device_id, type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:88: 	DEVID_FIELD(of_device_id, compatible);
	.loc 1 88 0
// 88 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_of_device_id_compatible 64 offsetof(struct of_device_id, compatible)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:90: 	DEVID(vio_device_id);
	.loc 1 90 0
// 90 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_vio_device_id 64 sizeof(struct vio_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:91: 	DEVID_FIELD(vio_device_id, type);
	.loc 1 91 0
// 91 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_vio_device_id_type 0 offsetof(struct vio_device_id, type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:92: 	DEVID_FIELD(vio_device_id, compat);
	.loc 1 92 0
// 92 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_vio_device_id_compat 32 offsetof(struct vio_device_id, compat)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:94: 	DEVID(input_device_id);
	.loc 1 94 0
// 94 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_input_device_id 200 sizeof(struct input_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:95: 	DEVID_FIELD(input_device_id, flags);
	.loc 1 95 0
// 95 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_flags 0 offsetof(struct input_device_id, flags)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:96: 	DEVID_FIELD(input_device_id, bustype);
	.loc 1 96 0
// 96 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_bustype 8 offsetof(struct input_device_id, bustype)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:97: 	DEVID_FIELD(input_device_id, vendor);
	.loc 1 97 0
// 97 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_vendor 10 offsetof(struct input_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:98: 	DEVID_FIELD(input_device_id, product);
	.loc 1 98 0
// 98 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_product 12 offsetof(struct input_device_id, product)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:99: 	DEVID_FIELD(input_device_id, version);
	.loc 1 99 0
// 99 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_version 14 offsetof(struct input_device_id, version)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:100: 	DEVID_FIELD(input_device_id, evbit);
	.loc 1 100 0
// 100 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_evbit 16 offsetof(struct input_device_id, evbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:101: 	DEVID_FIELD(input_device_id, keybit);
	.loc 1 101 0
// 101 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_keybit 24 offsetof(struct input_device_id, keybit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:102: 	DEVID_FIELD(input_device_id, relbit);
	.loc 1 102 0
// 102 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_relbit 120 offsetof(struct input_device_id, relbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:103: 	DEVID_FIELD(input_device_id, absbit);
	.loc 1 103 0
// 103 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_absbit 128 offsetof(struct input_device_id, absbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:104: 	DEVID_FIELD(input_device_id, mscbit);
	.loc 1 104 0
// 104 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_mscbit 136 offsetof(struct input_device_id, mscbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:105: 	DEVID_FIELD(input_device_id, ledbit);
	.loc 1 105 0
// 105 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_ledbit 144 offsetof(struct input_device_id, ledbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:106: 	DEVID_FIELD(input_device_id, sndbit);
	.loc 1 106 0
// 106 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_sndbit 152 offsetof(struct input_device_id, sndbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:107: 	DEVID_FIELD(input_device_id, ffbit);
	.loc 1 107 0
// 107 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_ffbit 160 offsetof(struct input_device_id, ffbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:108: 	DEVID_FIELD(input_device_id, swbit);
	.loc 1 108 0
// 108 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_input_device_id_swbit 176 offsetof(struct input_device_id, swbit)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:110: 	DEVID(eisa_device_id);
	.loc 1 110 0
// 110 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_eisa_device_id 16 sizeof(struct eisa_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:111: 	DEVID_FIELD(eisa_device_id, sig);
	.loc 1 111 0
// 111 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_eisa_device_id_sig 0 offsetof(struct eisa_device_id, sig)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:113: 	DEVID(parisc_device_id);
	.loc 1 113 0
// 113 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_parisc_device_id 8 sizeof(struct parisc_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:114: 	DEVID_FIELD(parisc_device_id, hw_type);
	.loc 1 114 0
// 114 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_parisc_device_id_hw_type 0 offsetof(struct parisc_device_id, hw_type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:115: 	DEVID_FIELD(parisc_device_id, hversion);
	.loc 1 115 0
// 115 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_parisc_device_id_hversion 2 offsetof(struct parisc_device_id, hversion)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:116: 	DEVID_FIELD(parisc_device_id, hversion_rev);
	.loc 1 116 0
// 116 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_parisc_device_id_hversion_rev 1 offsetof(struct parisc_device_id, hversion_rev)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:117: 	DEVID_FIELD(parisc_device_id, sversion);
	.loc 1 117 0
// 117 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_parisc_device_id_sversion 4 offsetof(struct parisc_device_id, sversion)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:119: 	DEVID(sdio_device_id);
	.loc 1 119 0
// 119 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_sdio_device_id 16 sizeof(struct sdio_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:120: 	DEVID_FIELD(sdio_device_id, class);
	.loc 1 120 0
// 120 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_sdio_device_id_class 0 offsetof(struct sdio_device_id, class)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:121: 	DEVID_FIELD(sdio_device_id, vendor);
	.loc 1 121 0
// 121 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_sdio_device_id_vendor 2 offsetof(struct sdio_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:122: 	DEVID_FIELD(sdio_device_id, device);
	.loc 1 122 0
// 122 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_sdio_device_id_device 4 offsetof(struct sdio_device_id, device)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:124: 	DEVID(ssb_device_id);
	.loc 1 124 0
// 124 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_ssb_device_id 6 sizeof(struct ssb_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:125: 	DEVID_FIELD(ssb_device_id, vendor);
	.loc 1 125 0
// 125 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ssb_device_id_vendor 0 offsetof(struct ssb_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:126: 	DEVID_FIELD(ssb_device_id, coreid);
	.loc 1 126 0
// 126 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ssb_device_id_coreid 2 offsetof(struct ssb_device_id, coreid)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:127: 	DEVID_FIELD(ssb_device_id, revision);
	.loc 1 127 0
// 127 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ssb_device_id_revision 4 offsetof(struct ssb_device_id, revision)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:129: 	DEVID(bcma_device_id);
	.loc 1 129 0
// 129 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_bcma_device_id 6 sizeof(struct bcma_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:130: 	DEVID_FIELD(bcma_device_id, manuf);
	.loc 1 130 0
// 130 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_bcma_device_id_manuf 0 offsetof(struct bcma_device_id, manuf)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:131: 	DEVID_FIELD(bcma_device_id, id);
	.loc 1 131 0
// 131 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_bcma_device_id_id 2 offsetof(struct bcma_device_id, id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:132: 	DEVID_FIELD(bcma_device_id, rev);
	.loc 1 132 0
// 132 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_bcma_device_id_rev 4 offsetof(struct bcma_device_id, rev)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:133: 	DEVID_FIELD(bcma_device_id, class);
	.loc 1 133 0
// 133 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_bcma_device_id_class 5 offsetof(struct bcma_device_id, class)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:135: 	DEVID(virtio_device_id);
	.loc 1 135 0
// 135 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_virtio_device_id 8 sizeof(struct virtio_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:136: 	DEVID_FIELD(virtio_device_id, device);
	.loc 1 136 0
// 136 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_virtio_device_id_device 0 offsetof(struct virtio_device_id, device)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:137: 	DEVID_FIELD(virtio_device_id, vendor);
	.loc 1 137 0
// 137 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_virtio_device_id_vendor 4 offsetof(struct virtio_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:139: 	DEVID(hv_vmbus_device_id);
	.loc 1 139 0
// 139 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_hv_vmbus_device_id 24 sizeof(struct hv_vmbus_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:140: 	DEVID_FIELD(hv_vmbus_device_id, guid);
	.loc 1 140 0
// 140 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hv_vmbus_device_id_guid 0 offsetof(struct hv_vmbus_device_id, guid)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:142: 	DEVID(i2c_device_id);
	.loc 1 142 0
// 142 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_i2c_device_id 32 sizeof(struct i2c_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:143: 	DEVID_FIELD(i2c_device_id, name);
	.loc 1 143 0
// 143 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_i2c_device_id_name 0 offsetof(struct i2c_device_id, name)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:145: 	DEVID(spi_device_id);
	.loc 1 145 0
// 145 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_spi_device_id 40 sizeof(struct spi_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:146: 	DEVID_FIELD(spi_device_id, name);
	.loc 1 146 0
// 146 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_spi_device_id_name 0 offsetof(struct spi_device_id, name)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:148: 	DEVID(dmi_system_id);
	.loc 1 148 0
// 148 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_dmi_system_id 344 sizeof(struct dmi_system_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:149: 	DEVID_FIELD(dmi_system_id, matches);
	.loc 1 149 0
// 149 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_dmi_system_id_matches 16 offsetof(struct dmi_system_id, matches)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:151: 	DEVID(platform_device_id);
	.loc 1 151 0
// 151 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_platform_device_id 32 sizeof(struct platform_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:152: 	DEVID_FIELD(platform_device_id, name);
	.loc 1 152 0
// 152 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_platform_device_id_name 0 offsetof(struct platform_device_id, name)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:154: 	DEVID(mdio_device_id);
	.loc 1 154 0
// 154 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_mdio_device_id 8 sizeof(struct mdio_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:155: 	DEVID_FIELD(mdio_device_id, phy_id);
	.loc 1 155 0
// 155 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_mdio_device_id_phy_id 0 offsetof(struct mdio_device_id, phy_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:156: 	DEVID_FIELD(mdio_device_id, phy_id_mask);
	.loc 1 156 0
// 156 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_mdio_device_id_phy_id_mask 4 offsetof(struct mdio_device_id, phy_id_mask)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:158: 	DEVID(zorro_device_id);
	.loc 1 158 0
// 158 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_zorro_device_id 16 sizeof(struct zorro_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:159: 	DEVID_FIELD(zorro_device_id, id);
	.loc 1 159 0
// 159 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_zorro_device_id_id 0 offsetof(struct zorro_device_id, id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:161: 	DEVID(isapnp_device_id);
	.loc 1 161 0
// 161 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_isapnp_device_id 16 sizeof(struct isapnp_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:162: 	DEVID_FIELD(isapnp_device_id, vendor);
	.loc 1 162 0
// 162 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_isapnp_device_id_vendor 4 offsetof(struct isapnp_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:163: 	DEVID_FIELD(isapnp_device_id, function);
	.loc 1 163 0
// 163 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_isapnp_device_id_function 6 offsetof(struct isapnp_device_id, function)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:165: 	DEVID(ipack_device_id);
	.loc 1 165 0
// 165 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_ipack_device_id 12 sizeof(struct ipack_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:166: 	DEVID_FIELD(ipack_device_id, format);
	.loc 1 166 0
// 166 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ipack_device_id_format 0 offsetof(struct ipack_device_id, format)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:167: 	DEVID_FIELD(ipack_device_id, vendor);
	.loc 1 167 0
// 167 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ipack_device_id_vendor 4 offsetof(struct ipack_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:168: 	DEVID_FIELD(ipack_device_id, device);
	.loc 1 168 0
// 168 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ipack_device_id_device 8 offsetof(struct ipack_device_id, device)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:170: 	DEVID(amba_id);
	.loc 1 170 0
// 170 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_amba_id 16 sizeof(struct amba_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:171: 	DEVID_FIELD(amba_id, id);
	.loc 1 171 0
// 171 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_amba_id_id 0 offsetof(struct amba_id, id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:172: 	DEVID_FIELD(amba_id, mask);
	.loc 1 172 0
// 172 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_amba_id_mask 4 offsetof(struct amba_id, mask)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:174: 	DEVID(mips_cdmm_device_id);
	.loc 1 174 0
// 174 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_mips_cdmm_device_id 1 sizeof(struct mips_cdmm_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:175: 	DEVID_FIELD(mips_cdmm_device_id, type);
	.loc 1 175 0
// 175 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_mips_cdmm_device_id_type 0 offsetof(struct mips_cdmm_device_id, type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:177: 	DEVID(x86_cpu_id);
	.loc 1 177 0
// 177 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_x86_cpu_id 16 sizeof(struct x86_cpu_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:178: 	DEVID_FIELD(x86_cpu_id, feature);
	.loc 1 178 0
// 178 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_x86_cpu_id_feature 6 offsetof(struct x86_cpu_id, feature)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:179: 	DEVID_FIELD(x86_cpu_id, family);
	.loc 1 179 0
// 179 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_x86_cpu_id_family 2 offsetof(struct x86_cpu_id, family)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:180: 	DEVID_FIELD(x86_cpu_id, model);
	.loc 1 180 0
// 180 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_x86_cpu_id_model 4 offsetof(struct x86_cpu_id, model)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:181: 	DEVID_FIELD(x86_cpu_id, vendor);
	.loc 1 181 0
// 181 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_x86_cpu_id_vendor 0 offsetof(struct x86_cpu_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:183: 	DEVID(cpu_feature);
	.loc 1 183 0
// 183 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_cpu_feature 2 sizeof(struct cpu_feature)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:184: 	DEVID_FIELD(cpu_feature, feature);
	.loc 1 184 0
// 184 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_cpu_feature_feature 0 offsetof(struct cpu_feature, feature)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:186: 	DEVID(mei_cl_device_id);
	.loc 1 186 0
// 186 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_mei_cl_device_id 64 sizeof(struct mei_cl_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:187: 	DEVID_FIELD(mei_cl_device_id, name);
	.loc 1 187 0
// 187 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_mei_cl_device_id_name 0 offsetof(struct mei_cl_device_id, name)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:188: 	DEVID_FIELD(mei_cl_device_id, uuid);
	.loc 1 188 0
// 188 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_mei_cl_device_id_uuid 32 offsetof(struct mei_cl_device_id, uuid)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:189: 	DEVID_FIELD(mei_cl_device_id, version);
	.loc 1 189 0
// 189 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_mei_cl_device_id_version 48 offsetof(struct mei_cl_device_id, version)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:191: 	DEVID(rio_device_id);
	.loc 1 191 0
// 191 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_rio_device_id 8 sizeof(struct rio_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:192: 	DEVID_FIELD(rio_device_id, did);
	.loc 1 192 0
// 192 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_rio_device_id_did 0 offsetof(struct rio_device_id, did)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:193: 	DEVID_FIELD(rio_device_id, vid);
	.loc 1 193 0
// 193 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_rio_device_id_vid 2 offsetof(struct rio_device_id, vid)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:194: 	DEVID_FIELD(rio_device_id, asm_did);
	.loc 1 194 0
// 194 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_rio_device_id_asm_did 4 offsetof(struct rio_device_id, asm_did)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:195: 	DEVID_FIELD(rio_device_id, asm_vid);
	.loc 1 195 0
// 195 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_rio_device_id_asm_vid 6 offsetof(struct rio_device_id, asm_vid)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:197: 	DEVID(ulpi_device_id);
	.loc 1 197 0
// 197 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_ulpi_device_id 16 sizeof(struct ulpi_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:198: 	DEVID_FIELD(ulpi_device_id, vendor);
	.loc 1 198 0
// 198 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ulpi_device_id_vendor 0 offsetof(struct ulpi_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:199: 	DEVID_FIELD(ulpi_device_id, product);
	.loc 1 199 0
// 199 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_ulpi_device_id_product 2 offsetof(struct ulpi_device_id, product)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:201: 	DEVID(hda_device_id);
	.loc 1 201 0
// 201 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_hda_device_id 32 sizeof(struct hda_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:202: 	DEVID_FIELD(hda_device_id, vendor_id);
	.loc 1 202 0
// 202 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hda_device_id_vendor_id 0 offsetof(struct hda_device_id, vendor_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:203: 	DEVID_FIELD(hda_device_id, rev_id);
	.loc 1 203 0
// 203 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hda_device_id_rev_id 4 offsetof(struct hda_device_id, rev_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:204: 	DEVID_FIELD(hda_device_id, api_version);
	.loc 1 204 0
// 204 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_hda_device_id_api_version 8 offsetof(struct hda_device_id, api_version)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:206: 	DEVID(fsl_mc_device_id);
	.loc 1 206 0
// 206 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->SIZE_fsl_mc_device_id 18 sizeof(struct fsl_mc_device_id)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:207: 	DEVID_FIELD(fsl_mc_device_id, vendor);
	.loc 1 207 0
// 207 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_fsl_mc_device_id_vendor 0 offsetof(struct fsl_mc_device_id, vendor)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:208: 	DEVID_FIELD(fsl_mc_device_id, obj_type);
	.loc 1 208 0
// 208 "scripts/mod/devicetable-offsets.c" 1
	
.ascii "->OFF_fsl_mc_device_id_obj_type 2 offsetof(struct fsl_mc_device_id, obj_type)"	//
// 0 "" 2
// scripts/mod/devicetable-offsets.c:211: }
	.loc 1 211 0
#NO_APP
	mov	w0, 0	//,
	ret
	.cfi_endproc
.LFE19:
	.size	main, .-main
	.text
.Letext0:
	.file 2 "./include/uapi/asm-generic/int-ll64.h"
	.file 3 "./include/uapi/linux/uuid.h"
	.file 4 "./include/linux/uuid.h"
	.file 5 "./include/asm-generic/int-ll64.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x148
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF18
	.byte	0x1
	.4byte	.LASF19
	.4byte	.LASF20
	.4byte	.Ldebug_ranges0+0
	.8byte	0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF0
	.uleb128 0x3
	.4byte	.LASF7
	.byte	0x2
	.byte	0x15
	.4byte	0x3b
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF1
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF3
	.uleb128 0x4
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF4
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF6
	.uleb128 0x5
	.string	"u8"
	.byte	0x5
	.byte	0x11
	.4byte	0x3b
	.uleb128 0x6
	.4byte	0x6c
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF9
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x7
	.byte	0x10
	.byte	0x3
	.byte	0x18
	.4byte	0xaa
	.uleb128 0x8
	.string	"b"
	.byte	0x3
	.byte	0x19
	.4byte	0xaa
	.byte	0
	.byte	0
	.uleb128 0x9
	.4byte	0x30
	.4byte	0xba
	.uleb128 0xa
	.4byte	0x7b
	.byte	0xf
	.byte	0
	.uleb128 0x3
	.4byte	.LASF12
	.byte	0x3
	.byte	0x1a
	.4byte	0x97
	.uleb128 0x6
	.4byte	0xba
	.uleb128 0x7
	.byte	0x10
	.byte	0x4
	.byte	0x17
	.4byte	0xdd
	.uleb128 0x8
	.string	"b"
	.byte	0x4
	.byte	0x18
	.4byte	0xaa
	.byte	0
	.byte	0
	.uleb128 0x3
	.4byte	.LASF13
	.byte	0x4
	.byte	0x19
	.4byte	0xca
	.uleb128 0x6
	.4byte	0xdd
	.uleb128 0xb
	.4byte	.LASF14
	.byte	0x4
	.byte	0x28
	.4byte	0xc5
	.uleb128 0xb
	.4byte	.LASF15
	.byte	0x4
	.byte	0x29
	.4byte	0xe8
	.uleb128 0x9
	.4byte	0x76
	.4byte	0x113
	.uleb128 0xa
	.4byte	0x7b
	.byte	0xf
	.byte	0
	.uleb128 0x6
	.4byte	0x103
	.uleb128 0xb
	.4byte	.LASF16
	.byte	0x4
	.byte	0x50
	.4byte	0x113
	.uleb128 0xb
	.4byte	.LASF17
	.byte	0x4
	.byte	0x51
	.4byte	0x113
	.uleb128 0xc
	.4byte	.LASF21
	.byte	0x1
	.byte	0x9
	.4byte	0x50
	.8byte	.LFB19
	.8byte	.LFE19-.LFB19
	.uleb128 0x1
	.byte	0x9c
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.LFB19
	.8byte	.LFE19-.LFB19
	.8byte	0
	.8byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.8byte	.LFB19
	.8byte	.LFE19
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF5:
	.string	"long long int"
.LASF7:
	.string	"__u8"
.LASF4:
	.string	"unsigned int"
.LASF1:
	.string	"unsigned char"
.LASF21:
	.string	"main"
.LASF13:
	.string	"uuid_t"
.LASF20:
	.string	"/home/chinkey/git_stuff/opencpi/projects/core/rcc/platforms/xilinx18_2/release/kernel-source"
.LASF8:
	.string	"long unsigned int"
.LASF6:
	.string	"long long unsigned int"
.LASF14:
	.string	"guid_null"
.LASF16:
	.string	"guid_index"
.LASF15:
	.string	"uuid_null"
.LASF9:
	.string	"char"
.LASF10:
	.string	"long int"
.LASF17:
	.string	"uuid_index"
.LASF11:
	.string	"_Bool"
.LASF3:
	.string	"short unsigned int"
.LASF0:
	.string	"signed char"
.LASF19:
	.string	"scripts/mod/devicetable-offsets.c"
.LASF2:
	.string	"short int"
.LASF12:
	.string	"guid_t"
.LASF18:
	.ascii	"GNU C89 7.2.1 20171011 -mlittle-endian -mgeneral-regs-only -"
	.ascii	"mpc-relative-literal-loads -mabi=lp64 -march=armv8-a -g -O2 "
	.ascii	"-std=gnu90 -fno-strict-aliasing -fno-common -fsh"
	.string	"ort-wchar -fno-PIE -fno-asynchronous-unwind-tables -fno-delete-null-pointer-checks -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -fno-strict-overflow -fconserve-stack --param allow-store-data-races=0"
	.ident	"GCC: (Linaro GCC 7.2-2017.11-rc1) 7.2.1 20171011"
	.section	.note.GNU-stack,"",@progbits
