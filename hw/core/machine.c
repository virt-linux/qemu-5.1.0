/*
 * QEMU Machine
 *
 * Copyright (C) 2014 Red Hat Inc
 *
 * Authors:
 *   Marcel Apfelbaum <marcel.a@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/option.h"
#include "qapi/qmp/qerror.h"
#include "sysemu/replay.h"
#include "qemu/units.h"
#include "hw/boards.h"
#include "qapi/error.h"
#include "qapi/qapi-visit-common.h"
#include "qapi/visitor.h"
#include "hw/sysbus.h"
#include "sysemu/sysemu.h"
#include "sysemu/numa.h"
#include "qemu/error-report.h"
#include "sysemu/qtest.h"
#include "hw/pci/pci.h"
#include "hw/mem/nvdimm.h"
#include "migration/vmstate.h"

/*
 * The same as hw_compat_4_2 + hw_compat_5_0
 */
GlobalProperty hw_compat_rhel_8_2[] = {
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "virtio-blk-device", "queue-size", "128"},
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "virtio-scsi-device", "virtqueue_size", "128"},
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "virtio-blk-device", "x-enable-wce-if-config-wce", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "virtio-blk-device", "seg-max-adjust", "off"},
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "virtio-scsi-device", "seg_max_adjust", "off"},
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "vhost-blk-device", "seg_max_adjust", "off"},
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "usb-host", "suppress-remote-wake", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "usb-redir", "suppress-remote-wake", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "qxl", "revision", "4" },
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "qxl-vga", "revision", "4" },
    /* hw_compat_rhel_8_2 from hw_compat_4_2 */
    { "fw_cfg", "acpi-mr-restore", "false" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "pci-host-bridge", "x-config-reg-migration-enabled", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "virtio-balloon-device", "page-poison", "false" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "vmport", "x-read-set-eax", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "vmport", "x-signal-unsupported-cmd", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "vmport", "x-report-vmx-type", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "vmport", "x-cmds-v2", "off" },
    /* hw_compat_rhel_8_2 from hw_compat_5_0 */
    { "virtio-device", "x-disable-legacy-check", "true" },
};
const size_t hw_compat_rhel_8_2_len = G_N_ELEMENTS(hw_compat_rhel_8_2);

/*
 * The same as hw_compat_4_1
 */
GlobalProperty hw_compat_rhel_8_1[] = {
    /* hw_compat_rhel_8_1 from hw_compat_4_1 */
    { "virtio-pci", "x-pcie-flr-init", "off" },
};
const size_t hw_compat_rhel_8_1_len = G_N_ELEMENTS(hw_compat_rhel_8_1);

/* The same as hw_compat_3_1
 * format of array has been changed by:
 *     6c36bddf5340 ("machine: Use shorter format for GlobalProperty arrays")
 */
GlobalProperty hw_compat_rhel_8_0[] = {
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "pcie-root-port", "x-speed", "2_5" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "pcie-root-port", "x-width", "1" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "memory-backend-file", "x-use-canonical-path-for-ramblock-id", "true" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "memory-backend-memfd", "x-use-canonical-path-for-ramblock-id", "true" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "tpm-crb", "ppi", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "tpm-tis", "ppi", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "usb-kbd", "serial", "42" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "usb-mouse", "serial", "42" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "usb-tablet", "serial", "42" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "virtio-blk-device", "discard", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 */
    { "virtio-blk-device", "write-zeroes", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_4_0 */
    { "VGA",            "edid", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_4_0 */
    { "secondary-vga",  "edid", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_4_0 */
    { "bochs-display",  "edid", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_4_0 */
    { "virtio-vga",     "edid", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_4_0 */
    { "virtio-gpu-device", "edid", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_4_0 */
    { "virtio-device", "use-started", "false" },
    /* hw_compat_rhel_8_0 from hw_compat_3_1 - that was added in 4.1 */
    { "pcie-root-port-base", "disable-acs", "true" },
};
const size_t hw_compat_rhel_8_0_len = G_N_ELEMENTS(hw_compat_rhel_8_0);

/* The same as hw_compat_3_0 + hw_compat_2_12
 * except that
 *   there's nothing in 3_0
 *   migration.decompress-error-check=off was in 7.5 from bz 1584139
 */
GlobalProperty hw_compat_rhel_7_6[] = {
    /* hw_compat_rhel_7_6 from hw_compat_2_12 */
    { "hda-audio", "use-timer", "false" },
    /* hw_compat_rhel_7_6 from hw_compat_2_12 */
    { "cirrus-vga", "global-vmstate", "true" },
    /* hw_compat_rhel_7_6 from hw_compat_2_12 */
    { "VGA", "global-vmstate", "true" },
    /* hw_compat_rhel_7_6 from hw_compat_2_12 */
    { "vmware-svga", "global-vmstate", "true" },
    /* hw_compat_rhel_7_6 from hw_compat_2_12 */
    { "qxl-vga", "global-vmstate",  "true" },
};
const size_t hw_compat_rhel_7_6_len = G_N_ELEMENTS(hw_compat_rhel_7_6);

/* The same as hw_compat_2_11 + hw_compat_2_10 */
GlobalProperty hw_compat_rhel_7_5[] = {
    /* hw_compat_rhel_7_5 from hw_compat_2_11 */
    { "hpet", "hpet-offset-saved", "false" },
    /* hw_compat_rhel_7_5 from hw_compat_2_11 */
    { "virtio-blk-pci", "vectors", "2" },
    /* hw_compat_rhel_7_5 from hw_compat_2_11 */
    { "vhost-user-blk-pci", "vectors", "2" },
    /* hw_compat_rhel_7_5 from hw_compat_2_11
       bz 1608778 modified for our naming */
    { "e1000-82540em", "migrate_tso_props", "off" },
    /* hw_compat_rhel_7_5 from hw_compat_2_10 */
    { "virtio-mouse-device", "wheel-axis", "false" },
    /* hw_compat_rhel_7_5 from hw_compat_2_10 */
    { "virtio-tablet-device", "wheel-axis", "false" },
    { "cirrus-vga", "vgamem_mb", "16" },
    { "migration", "decompress-error-check", "off" },
};
const size_t hw_compat_rhel_7_5_len = G_N_ELEMENTS(hw_compat_rhel_7_5);

/* Mostly like hw_compat_2_9 except
 * x-mtu-bypass-backend, x-migrate-msix has already been
 * backported to RHEL7.4. shpc was already on in 7.4.
 */
GlobalProperty hw_compat_rhel_7_4[] = {
    { "intel-iommu", "pt", "off" },
};

const size_t hw_compat_rhel_7_4_len = G_N_ELEMENTS(hw_compat_rhel_7_4);
/* Mostly like HW_COMPAT_2_6 + HW_COMPAT_2_7 + HW_COMPAT_2_8 except
 * disable-modern, disable-legacy, page-per-vq have already been
 * backported to RHEL7.3
 */
GlobalProperty hw_compat_rhel_7_3[] = {
    { "virtio-mmio", "format_transport_address", "off" },
    { "virtio-serial-device", "emergency-write", "off" },
    { "ioapic", "version", "0x11" },
    { "intel-iommu", "x-buggy-eim", "true" },
    { "virtio-pci", "x-ignore-backend-features", "on" },
    { "fw_cfg_mem", "x-file-slots", stringify(0x10) },
    { "fw_cfg_io", "x-file-slots", stringify(0x10) },
    { "pflash_cfi01", "old-multiple-chip-handling", "on" },
    { TYPE_PCI_DEVICE, "x-pcie-extcap-init", "off" },
    { "virtio-pci", "x-pcie-deverr-init", "off" },
    { "virtio-pci", "x-pcie-lnkctl-init", "off" },
    { "virtio-pci", "x-pcie-pm-init", "off" },
    { "virtio-net-device", "x-mtu-bypass-backend", "off" },
    { "e1000e", "__redhat_e1000e_7_3_intr_state", "on" },
};
const size_t hw_compat_rhel_7_3_len = G_N_ELEMENTS(hw_compat_rhel_7_3);

/* Mostly like hw_compat_2_4 + 2_3 but:
 *  we don't need "any_layout" as it has been backported to 7.2
 */
GlobalProperty hw_compat_rhel_7_2[] = {
        { "virtio-blk-device", "scsi", "true" },
        { "e1000-82540em", "extra_mac_registers", "off" },
        { "virtio-pci", "x-disable-pcie", "on" },
        { "virtio-pci", "migrate-extra", "off" },
        { "fw_cfg_mem", "dma_enabled", "off" },
        { "fw_cfg_io", "dma_enabled", "off" },
        { "isa-fdc", "fallback", "144" },
        /* Optional because not all virtio-pci devices support legacy mode */
        { "virtio-pci", "disable-modern", "on", .optional = true },
        { "virtio-pci", "disable-legacy", "off", .optional = true },
        { TYPE_PCI_DEVICE, "x-pcie-lnksta-dllla", "off" },
        { "virtio-pci", "page-per-vq", "on" },
        /* hw_compat_rhel_7_2 - introduced with 2.10.0 */
        { "migration", "send-section-footer", "off" },
        /* hw_compat_rhel_7_2 - introduced with 2.10.0 */
        { "migration", "store-global-state", "off",
        },
};
const size_t hw_compat_rhel_7_2_len = G_N_ELEMENTS(hw_compat_rhel_7_2);

/* Mostly like hw_compat_2_1 but:
 *    we don't need virtio-scsi-pci since 7.0 already had that on
 *
 * RH: Note, qemu-extended-regs should have been enabled in the 7.1
 * machine type, but was accidentally turned off in 7.2 onwards.
 */
GlobalProperty hw_compat_rhel_7_1[] = {
    { "intel-hda-generic", "old_msi_addr", "on" },
    { "VGA", "qemu-extended-regs", "off" },
    { "secondary-vga", "qemu-extended-regs", "off" },
    { "usb-mouse", "usb_version", stringify(1) },
    { "usb-kbd", "usb_version", stringify(1) },
    { "virtio-pci",  "virtio-pci-bus-master-bug-migration", "on" },
    { "virtio-blk-pci", "any_layout", "off" },
    { "virtio-balloon-pci", "any_layout", "off" },
    { "virtio-serial-pci", "any_layout", "off" },
    { "virtio-9p-pci", "any_layout", "off" },
    { "virtio-rng-pci", "any_layout", "off" },
    /* HW_COMPAT_RHEL7_1 - introduced with 2.10.0 */
    { "migration", "send-configuration", "off" },
};
const size_t hw_compat_rhel_7_1_len = G_N_ELEMENTS(hw_compat_rhel_7_1);

GlobalProperty hw_compat_5_0[] = {
    { "pci-host-bridge", "x-config-reg-migration-enabled", "off" },
    { "virtio-balloon-device", "page-poison", "false" },
    { "vmport", "x-read-set-eax", "off" },
    { "vmport", "x-signal-unsupported-cmd", "off" },
    { "vmport", "x-report-vmx-type", "off" },
    { "vmport", "x-cmds-v2", "off" },
    { "virtio-device", "x-disable-legacy-check", "true" },
};
const size_t hw_compat_5_0_len = G_N_ELEMENTS(hw_compat_5_0);

GlobalProperty hw_compat_4_2[] = {
    { "virtio-blk-device", "queue-size", "128"},
    { "virtio-scsi-device", "virtqueue_size", "128"},
    { "virtio-blk-device", "x-enable-wce-if-config-wce", "off" },
    { "virtio-blk-device", "seg-max-adjust", "off"},
    { "virtio-scsi-device", "seg_max_adjust", "off"},
    { "vhost-blk-device", "seg_max_adjust", "off"},
    { "usb-host", "suppress-remote-wake", "off" },
    { "usb-redir", "suppress-remote-wake", "off" },
    { "qxl", "revision", "4" },
    { "qxl-vga", "revision", "4" },
    { "fw_cfg", "acpi-mr-restore", "false" },
};
const size_t hw_compat_4_2_len = G_N_ELEMENTS(hw_compat_4_2);

GlobalProperty hw_compat_4_1[] = {
    { "virtio-pci", "x-pcie-flr-init", "off" },
    { "virtio-device", "use-disabled-flag", "false" },
};
const size_t hw_compat_4_1_len = G_N_ELEMENTS(hw_compat_4_1);

GlobalProperty hw_compat_4_0[] = {
    { "VGA",            "edid", "false" },
    { "secondary-vga",  "edid", "false" },
    { "bochs-display",  "edid", "false" },
    { "virtio-vga",     "edid", "false" },
    { "virtio-gpu-device", "edid", "false" },
    { "virtio-device", "use-started", "false" },
    { "virtio-balloon-device", "qemu-4-0-config-size", "true" },
    { "pl031", "migrate-tick-offset", "false" },
};
const size_t hw_compat_4_0_len = G_N_ELEMENTS(hw_compat_4_0);

GlobalProperty hw_compat_3_1[] = {
    { "pcie-root-port", "x-speed", "2_5" },
    { "pcie-root-port", "x-width", "1" },
    { "memory-backend-file", "x-use-canonical-path-for-ramblock-id", "true" },
    { "memory-backend-memfd", "x-use-canonical-path-for-ramblock-id", "true" },
    { "tpm-crb", "ppi", "false" },
    { "tpm-tis", "ppi", "false" },
    { "usb-kbd", "serial", "42" },
    { "usb-mouse", "serial", "42" },
    { "usb-tablet", "serial", "42" },
    { "virtio-blk-device", "discard", "false" },
    { "virtio-blk-device", "write-zeroes", "false" },
    { "virtio-balloon-device", "qemu-4-0-config-size", "false" },
    { "pcie-root-port-base", "disable-acs", "true" }, /* Added in 4.1 */
};
const size_t hw_compat_3_1_len = G_N_ELEMENTS(hw_compat_3_1);

GlobalProperty hw_compat_3_0[] = {};
const size_t hw_compat_3_0_len = G_N_ELEMENTS(hw_compat_3_0);

GlobalProperty hw_compat_2_12[] = {
    { "migration", "decompress-error-check", "off" },
    { "hda-audio", "use-timer", "false" },
    { "cirrus-vga", "global-vmstate", "true" },
    { "VGA", "global-vmstate", "true" },
    { "vmware-svga", "global-vmstate", "true" },
    { "qxl-vga", "global-vmstate", "true" },
};
const size_t hw_compat_2_12_len = G_N_ELEMENTS(hw_compat_2_12);

GlobalProperty hw_compat_2_11[] = {
    { "hpet", "hpet-offset-saved", "false" },
    { "virtio-blk-pci", "vectors", "2" },
    { "vhost-user-blk-pci", "vectors", "2" },
    { "e1000", "migrate_tso_props", "off" },
};
const size_t hw_compat_2_11_len = G_N_ELEMENTS(hw_compat_2_11);

GlobalProperty hw_compat_2_10[] = {
    { "virtio-mouse-device", "wheel-axis", "false" },
    { "virtio-tablet-device", "wheel-axis", "false" },
};
const size_t hw_compat_2_10_len = G_N_ELEMENTS(hw_compat_2_10);

GlobalProperty hw_compat_2_9[] = {
    { "pci-bridge", "shpc", "off" },
    { "intel-iommu", "pt", "off" },
    { "virtio-net-device", "x-mtu-bypass-backend", "off" },
    { "pcie-root-port", "x-migrate-msix", "false" },
};
const size_t hw_compat_2_9_len = G_N_ELEMENTS(hw_compat_2_9);

GlobalProperty hw_compat_2_8[] = {
    { "fw_cfg_mem", "x-file-slots", "0x10" },
    { "fw_cfg_io", "x-file-slots", "0x10" },
    { "pflash_cfi01", "old-multiple-chip-handling", "on" },
    { "pci-bridge", "shpc", "on" },
    { TYPE_PCI_DEVICE, "x-pcie-extcap-init", "off" },
    { "virtio-pci", "x-pcie-deverr-init", "off" },
    { "virtio-pci", "x-pcie-lnkctl-init", "off" },
    { "virtio-pci", "x-pcie-pm-init", "off" },
    { "cirrus-vga", "vgamem_mb", "8" },
    { "isa-cirrus-vga", "vgamem_mb", "8" },
};
const size_t hw_compat_2_8_len = G_N_ELEMENTS(hw_compat_2_8);

GlobalProperty hw_compat_2_7[] = {
    { "virtio-pci", "page-per-vq", "on" },
    { "virtio-serial-device", "emergency-write", "off" },
    { "ioapic", "version", "0x11" },
    { "intel-iommu", "x-buggy-eim", "true" },
    { "virtio-pci", "x-ignore-backend-features", "on" },
};
const size_t hw_compat_2_7_len = G_N_ELEMENTS(hw_compat_2_7);

GlobalProperty hw_compat_2_6[] = {
    { "virtio-mmio", "format_transport_address", "off" },
    /* Optional because not all virtio-pci devices support legacy mode */
    { "virtio-pci", "disable-modern", "on",  .optional = true },
    { "virtio-pci", "disable-legacy", "off", .optional = true },
};
const size_t hw_compat_2_6_len = G_N_ELEMENTS(hw_compat_2_6);

GlobalProperty hw_compat_2_5[] = {
    { "isa-fdc", "fallback", "144" },
    { "pvscsi", "x-old-pci-configuration", "on" },
    { "pvscsi", "x-disable-pcie", "on" },
    { "vmxnet3", "x-old-msi-offsets", "on" },
    { "vmxnet3", "x-disable-pcie", "on" },
};
const size_t hw_compat_2_5_len = G_N_ELEMENTS(hw_compat_2_5);

GlobalProperty hw_compat_2_4[] = {
    /* Optional because the 'scsi' property is Linux-only */
    { "virtio-blk-device", "scsi", "true", .optional = true },
    { "e1000", "extra_mac_registers", "off" },
    { "virtio-pci", "x-disable-pcie", "on" },
    { "virtio-pci", "migrate-extra", "off" },
    { "fw_cfg_mem", "dma_enabled", "off" },
    { "fw_cfg_io", "dma_enabled", "off" }
};
const size_t hw_compat_2_4_len = G_N_ELEMENTS(hw_compat_2_4);

GlobalProperty hw_compat_2_3[] = {
    { "virtio-blk-pci", "any_layout", "off" },
    { "virtio-balloon-pci", "any_layout", "off" },
    { "virtio-serial-pci", "any_layout", "off" },
    { "virtio-9p-pci", "any_layout", "off" },
    { "virtio-rng-pci", "any_layout", "off" },
    { TYPE_PCI_DEVICE, "x-pcie-lnksta-dllla", "off" },
    { "migration", "send-configuration", "off" },
    { "migration", "send-section-footer", "off" },
    { "migration", "store-global-state", "off" },
};
const size_t hw_compat_2_3_len = G_N_ELEMENTS(hw_compat_2_3);

GlobalProperty hw_compat_2_2[] = {};
const size_t hw_compat_2_2_len = G_N_ELEMENTS(hw_compat_2_2);

GlobalProperty hw_compat_2_1[] = {
    { "intel-hda", "old_msi_addr", "on" },
    { "VGA", "qemu-extended-regs", "off" },
    { "secondary-vga", "qemu-extended-regs", "off" },
    { "virtio-scsi-pci", "any_layout", "off" },
    { "usb-mouse", "usb_version", "1" },
    { "usb-kbd", "usb_version", "1" },
    { "virtio-pci", "virtio-pci-bus-master-bug-migration", "on" },
};
const size_t hw_compat_2_1_len = G_N_ELEMENTS(hw_compat_2_1);

static char *machine_get_kernel(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->kernel_filename);
}

static void machine_set_kernel(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->kernel_filename);
    ms->kernel_filename = g_strdup(value);
}

static char *machine_get_initrd(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->initrd_filename);
}

static void machine_set_initrd(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->initrd_filename);
    ms->initrd_filename = g_strdup(value);
}

static char *machine_get_append(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->kernel_cmdline);
}

static void machine_set_append(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->kernel_cmdline);
    ms->kernel_cmdline = g_strdup(value);
}

static char *machine_get_dtb(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->dtb);
}

static void machine_set_dtb(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->dtb);
    ms->dtb = g_strdup(value);
}

static char *machine_get_dumpdtb(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->dumpdtb);
}

static void machine_set_dumpdtb(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->dumpdtb);
    ms->dumpdtb = g_strdup(value);
}

static void machine_get_phandle_start(Object *obj, Visitor *v,
                                      const char *name, void *opaque,
                                      Error **errp)
{
    MachineState *ms = MACHINE(obj);
    int64_t value = ms->phandle_start;

    visit_type_int(v, name, &value, errp);
}

static void machine_set_phandle_start(Object *obj, Visitor *v,
                                      const char *name, void *opaque,
                                      Error **errp)
{
    MachineState *ms = MACHINE(obj);
    int64_t value;

    if (!visit_type_int(v, name, &value, errp)) {
        return;
    }

    ms->phandle_start = value;
}

static char *machine_get_dt_compatible(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->dt_compatible);
}

static void machine_set_dt_compatible(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->dt_compatible);
    ms->dt_compatible = g_strdup(value);
}

static bool machine_get_dump_guest_core(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->dump_guest_core;
}

static void machine_set_dump_guest_core(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->dump_guest_core = value;
}

static bool machine_get_mem_merge(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->mem_merge;
}

static void machine_set_mem_merge(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->mem_merge = value;
}

static bool machine_get_usb(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->usb;
}

static void machine_set_usb(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->usb = value;
    ms->usb_disabled = !value;
}

static bool machine_get_graphics(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->enable_graphics;
}

static void machine_set_graphics(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->enable_graphics = value;
}

static char *machine_get_firmware(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->firmware);
}

static void machine_set_firmware(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->firmware);
    ms->firmware = g_strdup(value);
}

static void machine_set_suppress_vmdesc(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->suppress_vmdesc = value;
}

static bool machine_get_suppress_vmdesc(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->suppress_vmdesc;
}

static void machine_set_enforce_config_section(Object *obj, bool value,
                                             Error **errp)
{
    MachineState *ms = MACHINE(obj);

    warn_report("enforce-config-section is deprecated, please use "
                "-global migration.send-configuration=on|off instead");

    ms->enforce_config_section = value;
}

static bool machine_get_enforce_config_section(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->enforce_config_section;
}

static char *machine_get_memory_encryption(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->memory_encryption);
}

static void machine_set_memory_encryption(Object *obj, const char *value,
                                        Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->memory_encryption);
    ms->memory_encryption = g_strdup(value);

    /*
     * With memory encryption, the host can't see the real contents of RAM,
     * so there's no point in it trying to merge areas.
     */
    if (value) {
        machine_set_mem_merge(obj, false, errp);
    }
}

static bool machine_get_nvdimm(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->nvdimms_state->is_enabled;
}

static void machine_set_nvdimm(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->nvdimms_state->is_enabled = value;
}

static bool machine_get_hmat(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->numa_state->hmat_enabled;
}

static void machine_set_hmat(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->numa_state->hmat_enabled = value;
}

static char *machine_get_nvdimm_persistence(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->nvdimms_state->persistence_string);
}

static void machine_set_nvdimm_persistence(Object *obj, const char *value,
                                           Error **errp)
{
    MachineState *ms = MACHINE(obj);
    NVDIMMState *nvdimms_state = ms->nvdimms_state;

    if (strcmp(value, "cpu") == 0) {
        nvdimms_state->persistence = 3;
    } else if (strcmp(value, "mem-ctrl") == 0) {
        nvdimms_state->persistence = 2;
    } else {
        error_setg(errp, "-machine nvdimm-persistence=%s: unsupported option",
                   value);
        return;
    }

    g_free(nvdimms_state->persistence_string);
    nvdimms_state->persistence_string = g_strdup(value);
}

void machine_class_allow_dynamic_sysbus_dev(MachineClass *mc, const char *type)
{
    strList *item = g_new0(strList, 1);

    item->value = g_strdup(type);
    item->next = mc->allowed_dynamic_sysbus_devices;
    mc->allowed_dynamic_sysbus_devices = item;
}

static void validate_sysbus_device(SysBusDevice *sbdev, void *opaque)
{
    MachineState *machine = opaque;
    MachineClass *mc = MACHINE_GET_CLASS(machine);
    bool allowed = false;
    strList *wl;

    for (wl = mc->allowed_dynamic_sysbus_devices;
         !allowed && wl;
         wl = wl->next) {
        allowed |= !!object_dynamic_cast(OBJECT(sbdev), wl->value);
    }

    if (!allowed) {
        error_report("Option '-device %s' cannot be handled by this machine",
                     object_class_get_name(object_get_class(OBJECT(sbdev))));
        exit(1);
    }
}

static char *machine_get_memdev(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return g_strdup(ms->ram_memdev_id);
}

static void machine_set_memdev(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->ram_memdev_id);
    ms->ram_memdev_id = g_strdup(value);
}


static void machine_init_notify(Notifier *notifier, void *data)
{
    MachineState *machine = MACHINE(qdev_get_machine());

    /*
     * Loop through all dynamically created sysbus devices and check if they are
     * all allowed.  If a device is not allowed, error out.
     */
    foreach_dynamic_sysbus_device(validate_sysbus_device, machine);
}

HotpluggableCPUList *machine_query_hotpluggable_cpus(MachineState *machine)
{
    int i;
    HotpluggableCPUList *head = NULL;
    MachineClass *mc = MACHINE_GET_CLASS(machine);

    /* force board to initialize possible_cpus if it hasn't been done yet */
    mc->possible_cpu_arch_ids(machine);

    for (i = 0; i < machine->possible_cpus->len; i++) {
        Object *cpu;
        HotpluggableCPUList *list_item = g_new0(typeof(*list_item), 1);
        HotpluggableCPU *cpu_item = g_new0(typeof(*cpu_item), 1);

        cpu_item->type = g_strdup(machine->possible_cpus->cpus[i].type);
        cpu_item->vcpus_count = machine->possible_cpus->cpus[i].vcpus_count;
        cpu_item->props = g_memdup(&machine->possible_cpus->cpus[i].props,
                                   sizeof(*cpu_item->props));

        cpu = machine->possible_cpus->cpus[i].cpu;
        if (cpu) {
            cpu_item->has_qom_path = true;
            cpu_item->qom_path = object_get_canonical_path(cpu);
        }
        list_item->value = cpu_item;
        list_item->next = head;
        head = list_item;
    }
    return head;
}

/**
 * machine_set_cpu_numa_node:
 * @machine: machine object to modify
 * @props: specifies which cpu objects to assign to
 *         numa node specified by @props.node_id
 * @errp: if an error occurs, a pointer to an area to store the error
 *
 * Associate NUMA node specified by @props.node_id with cpu slots that
 * match socket/core/thread-ids specified by @props. It's recommended to use
 * query-hotpluggable-cpus.props values to specify affected cpu slots,
 * which would lead to exact 1:1 mapping of cpu slots to NUMA node.
 *
 * However for CLI convenience it's possible to pass in subset of properties,
 * which would affect all cpu slots that match it.
 * Ex for pc machine:
 *    -smp 4,cores=2,sockets=2 -numa node,nodeid=0 -numa node,nodeid=1 \
 *    -numa cpu,node-id=0,socket_id=0 \
 *    -numa cpu,node-id=1,socket_id=1
 * will assign all child cores of socket 0 to node 0 and
 * of socket 1 to node 1.
 *
 * On attempt of reassigning (already assigned) cpu slot to another NUMA node,
 * return error.
 * Empty subset is disallowed and function will return with error in this case.
 */
void machine_set_cpu_numa_node(MachineState *machine,
                               const CpuInstanceProperties *props, Error **errp)
{
    MachineClass *mc = MACHINE_GET_CLASS(machine);
    NodeInfo *numa_info = machine->numa_state->nodes;
    bool match = false;
    int i;

    if (!mc->possible_cpu_arch_ids) {
        error_setg(errp, "mapping of CPUs to NUMA node is not supported");
        return;
    }

    /* disabling node mapping is not supported, forbid it */
    assert(props->has_node_id);

    /* force board to initialize possible_cpus if it hasn't been done yet */
    mc->possible_cpu_arch_ids(machine);

    for (i = 0; i < machine->possible_cpus->len; i++) {
        CPUArchId *slot = &machine->possible_cpus->cpus[i];

        /* reject unsupported by board properties */
        if (props->has_thread_id && !slot->props.has_thread_id) {
            error_setg(errp, "thread-id is not supported");
            return;
        }

        if (props->has_core_id && !slot->props.has_core_id) {
            error_setg(errp, "core-id is not supported");
            return;
        }

        if (props->has_socket_id && !slot->props.has_socket_id) {
            error_setg(errp, "socket-id is not supported");
            return;
        }

        if (props->has_die_id && !slot->props.has_die_id) {
            error_setg(errp, "die-id is not supported");
            return;
        }

        /* skip slots with explicit mismatch */
        if (props->has_thread_id && props->thread_id != slot->props.thread_id) {
                continue;
        }

        if (props->has_core_id && props->core_id != slot->props.core_id) {
                continue;
        }

        if (props->has_die_id && props->die_id != slot->props.die_id) {
                continue;
        }

        if (props->has_socket_id && props->socket_id != slot->props.socket_id) {
                continue;
        }

        /* reject assignment if slot is already assigned, for compatibility
         * of legacy cpu_index mapping with SPAPR core based mapping do not
         * error out if cpu thread and matched core have the same node-id */
        if (slot->props.has_node_id &&
            slot->props.node_id != props->node_id) {
            error_setg(errp, "CPU is already assigned to node-id: %" PRId64,
                       slot->props.node_id);
            return;
        }

        /* assign slot to node as it's matched '-numa cpu' key */
        match = true;
        slot->props.node_id = props->node_id;
        slot->props.has_node_id = props->has_node_id;

        if (machine->numa_state->hmat_enabled) {
            if ((numa_info[props->node_id].initiator < MAX_NODES) &&
                (props->node_id != numa_info[props->node_id].initiator)) {
                error_setg(errp, "The initiator of CPU NUMA node %" PRId64
                        " should be itself", props->node_id);
                return;
            }
            numa_info[props->node_id].has_cpu = true;
            numa_info[props->node_id].initiator = props->node_id;
        }
    }

    if (!match) {
        error_setg(errp, "no match found");
    }
}

static void smp_parse(MachineState *ms, QemuOpts *opts)
{
    if (opts) {
        unsigned cpus    = qemu_opt_get_number(opts, "cpus", 0);
        unsigned sockets = qemu_opt_get_number(opts, "sockets", 0);
        unsigned cores   = qemu_opt_get_number(opts, "cores", 0);
        unsigned threads = qemu_opt_get_number(opts, "threads", 0);

        /* compute missing values, prefer sockets over cores over threads */
        if (cpus == 0 || sockets == 0) {
            cores = cores > 0 ? cores : 1;
            threads = threads > 0 ? threads : 1;
            if (cpus == 0) {
                sockets = sockets > 0 ? sockets : 1;
                cpus = cores * threads * sockets;
            } else {
                ms->smp.max_cpus =
                        qemu_opt_get_number(opts, "maxcpus", cpus);
                sockets = ms->smp.max_cpus / (cores * threads);
            }
        } else if (cores == 0) {
            threads = threads > 0 ? threads : 1;
            cores = cpus / (sockets * threads);
            cores = cores > 0 ? cores : 1;
        } else if (threads == 0) {
            threads = cpus / (cores * sockets);
            threads = threads > 0 ? threads : 1;
        } else if (sockets * cores * threads < cpus) {
            error_report("cpu topology: "
                         "sockets (%u) * cores (%u) * threads (%u) < "
                         "smp_cpus (%u)",
                         sockets, cores, threads, cpus);
            exit(1);
        }

        ms->smp.max_cpus =
                qemu_opt_get_number(opts, "maxcpus", cpus);

        if (ms->smp.max_cpus < cpus) {
            error_report("maxcpus must be equal to or greater than smp");
            exit(1);
        }

        if (sockets * cores * threads > ms->smp.max_cpus) {
            error_report("cpu topology: "
                         "sockets (%u) * cores (%u) * threads (%u) > "
                         "maxcpus (%u)",
                         sockets, cores, threads,
                         ms->smp.max_cpus);
            exit(1);
        }

        if (sockets * cores * threads != ms->smp.max_cpus) {
            warn_report("Invalid CPU topology deprecated: "
                        "sockets (%u) * cores (%u) * threads (%u) "
                        "!= maxcpus (%u)",
                        sockets, cores, threads,
                        ms->smp.max_cpus);
        }

        ms->smp.cpus = cpus;
        ms->smp.cores = cores;
        ms->smp.threads = threads;
        ms->smp.sockets = sockets;
    }

    if (ms->smp.cpus > 1) {
        Error *blocker = NULL;
        error_setg(&blocker, QERR_REPLAY_NOT_SUPPORTED, "smp");
        replay_add_blocker(blocker);
    }
}

static void machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    /* Default 128 MB as guest ram size */
    mc->default_ram_size = 128 * MiB;
    mc->rom_file_has_mr = true;
    mc->smp_parse = smp_parse;

    /* numa node memory size aligned on 8MB by default.
     * On Linux, each node's border has to be 8MB aligned
     */
    mc->numa_mem_align_shift = 23;
    mc->numa_auto_assign_ram = numa_default_auto_assign_ram;

    object_class_property_add_str(oc, "kernel",
        machine_get_kernel, machine_set_kernel);
    object_class_property_set_description(oc, "kernel",
        "Linux kernel image file");

    object_class_property_add_str(oc, "initrd",
        machine_get_initrd, machine_set_initrd);
    object_class_property_set_description(oc, "initrd",
        "Linux initial ramdisk file");

    object_class_property_add_str(oc, "append",
        machine_get_append, machine_set_append);
    object_class_property_set_description(oc, "append",
        "Linux kernel command line");

    object_class_property_add_str(oc, "dtb",
        machine_get_dtb, machine_set_dtb);
    object_class_property_set_description(oc, "dtb",
        "Linux kernel device tree file");

    object_class_property_add_str(oc, "dumpdtb",
        machine_get_dumpdtb, machine_set_dumpdtb);
    object_class_property_set_description(oc, "dumpdtb",
        "Dump current dtb to a file and quit");

    object_class_property_add(oc, "phandle-start", "int",
        machine_get_phandle_start, machine_set_phandle_start,
        NULL, NULL);
    object_class_property_set_description(oc, "phandle-start",
        "The first phandle ID we may generate dynamically");

    object_class_property_add_str(oc, "dt-compatible",
        machine_get_dt_compatible, machine_set_dt_compatible);
    object_class_property_set_description(oc, "dt-compatible",
        "Overrides the \"compatible\" property of the dt root node");

    object_class_property_add_bool(oc, "dump-guest-core",
        machine_get_dump_guest_core, machine_set_dump_guest_core);
    object_class_property_set_description(oc, "dump-guest-core",
        "Include guest memory in a core dump");

    object_class_property_add_bool(oc, "mem-merge",
        machine_get_mem_merge, machine_set_mem_merge);
    object_class_property_set_description(oc, "mem-merge",
        "Enable/disable memory merge support");

    object_class_property_add_bool(oc, "usb",
        machine_get_usb, machine_set_usb);
    object_class_property_set_description(oc, "usb",
        "Set on/off to enable/disable usb");

    object_class_property_add_bool(oc, "graphics",
        machine_get_graphics, machine_set_graphics);
    object_class_property_set_description(oc, "graphics",
        "Set on/off to enable/disable graphics emulation");

    object_class_property_add_str(oc, "firmware",
        machine_get_firmware, machine_set_firmware);
    object_class_property_set_description(oc, "firmware",
        "Firmware image");

    object_class_property_add_bool(oc, "suppress-vmdesc",
        machine_get_suppress_vmdesc, machine_set_suppress_vmdesc);
    object_class_property_set_description(oc, "suppress-vmdesc",
        "Set on to disable self-describing migration");

    object_class_property_add_bool(oc, "enforce-config-section",
        machine_get_enforce_config_section, machine_set_enforce_config_section);
    object_class_property_set_description(oc, "enforce-config-section",
        "Set on to enforce configuration section migration");

    object_class_property_add_str(oc, "memory-encryption",
        machine_get_memory_encryption, machine_set_memory_encryption);
    object_class_property_set_description(oc, "memory-encryption",
        "Set memory encryption object to use");
}

static void machine_class_base_init(ObjectClass *oc, void *data)
{
    if (!object_class_is_abstract(oc)) {
        MachineClass *mc = MACHINE_CLASS(oc);
        const char *cname = object_class_get_name(oc);
        assert(g_str_has_suffix(cname, TYPE_MACHINE_SUFFIX));
        mc->name = g_strndup(cname,
                            strlen(cname) - strlen(TYPE_MACHINE_SUFFIX));
        mc->compat_props = g_ptr_array_new();
    }
}

static void machine_initfn(Object *obj)
{
    MachineState *ms = MACHINE(obj);
    MachineClass *mc = MACHINE_GET_CLASS(obj);

    ms->dump_guest_core = true;
    ms->mem_merge = true;
    ms->enable_graphics = true;

    if (mc->nvdimm_supported) {
        Object *obj = OBJECT(ms);

        ms->nvdimms_state = g_new0(NVDIMMState, 1);
        object_property_add_bool(obj, "nvdimm",
                                 machine_get_nvdimm, machine_set_nvdimm);
        object_property_set_description(obj, "nvdimm",
                                        "Set on/off to enable/disable "
                                        "NVDIMM instantiation");

        object_property_add_str(obj, "nvdimm-persistence",
                                machine_get_nvdimm_persistence,
                                machine_set_nvdimm_persistence);
        object_property_set_description(obj, "nvdimm-persistence",
                                        "Set NVDIMM persistence"
                                        "Valid values are cpu, mem-ctrl");
    }

    if (mc->cpu_index_to_instance_props && mc->get_default_cpu_node_id) {
        ms->numa_state = g_new0(NumaState, 1);
        object_property_add_bool(obj, "hmat",
                                 machine_get_hmat, machine_set_hmat);
        object_property_set_description(obj, "hmat",
                                        "Set on/off to enable/disable "
                                        "ACPI Heterogeneous Memory Attribute "
                                        "Table (HMAT)");
    }

    object_property_add_str(obj, "memory-backend",
                            machine_get_memdev, machine_set_memdev);
    object_property_set_description(obj, "memory-backend",
                                    "Set RAM backend"
                                    "Valid value is ID of hostmem based backend");

    /* Register notifier when init is done for sysbus sanity checks */
    ms->sysbus_notifier.notify = machine_init_notify;
    qemu_add_machine_init_done_notifier(&ms->sysbus_notifier);
}

static void machine_finalize(Object *obj)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->kernel_filename);
    g_free(ms->initrd_filename);
    g_free(ms->kernel_cmdline);
    g_free(ms->dtb);
    g_free(ms->dumpdtb);
    g_free(ms->dt_compatible);
    g_free(ms->firmware);
    g_free(ms->device_memory);
    g_free(ms->nvdimms_state);
    g_free(ms->numa_state);
}

bool machine_usb(MachineState *machine)
{
    return machine->usb;
}

int machine_phandle_start(MachineState *machine)
{
    return machine->phandle_start;
}

bool machine_dump_guest_core(MachineState *machine)
{
    return machine->dump_guest_core;
}

bool machine_mem_merge(MachineState *machine)
{
    return machine->mem_merge;
}

static char *cpu_slot_to_string(const CPUArchId *cpu)
{
    GString *s = g_string_new(NULL);
    if (cpu->props.has_socket_id) {
        g_string_append_printf(s, "socket-id: %"PRId64, cpu->props.socket_id);
    }
    if (cpu->props.has_die_id) {
        g_string_append_printf(s, "die-id: %"PRId64, cpu->props.die_id);
    }
    if (cpu->props.has_core_id) {
        if (s->len) {
            g_string_append_printf(s, ", ");
        }
        g_string_append_printf(s, "core-id: %"PRId64, cpu->props.core_id);
    }
    if (cpu->props.has_thread_id) {
        if (s->len) {
            g_string_append_printf(s, ", ");
        }
        g_string_append_printf(s, "thread-id: %"PRId64, cpu->props.thread_id);
    }
    return g_string_free(s, false);
}

static void numa_validate_initiator(NumaState *numa_state)
{
    int i;
    NodeInfo *numa_info = numa_state->nodes;

    for (i = 0; i < numa_state->num_nodes; i++) {
        if (numa_info[i].initiator == MAX_NODES) {
            error_report("The initiator of NUMA node %d is missing, use "
                         "'-numa node,initiator' option to declare it", i);
            exit(1);
        }

        if (!numa_info[numa_info[i].initiator].present) {
            error_report("NUMA node %" PRIu16 " is missing, use "
                         "'-numa node' option to declare it first",
                         numa_info[i].initiator);
            exit(1);
        }

        if (!numa_info[numa_info[i].initiator].has_cpu) {
            error_report("The initiator of NUMA node %d is invalid", i);
            exit(1);
        }
    }
}

static void machine_numa_finish_cpu_init(MachineState *machine)
{
    int i;
    bool default_mapping;
    GString *s = g_string_new(NULL);
    MachineClass *mc = MACHINE_GET_CLASS(machine);
    const CPUArchIdList *possible_cpus = mc->possible_cpu_arch_ids(machine);

    assert(machine->numa_state->num_nodes);
    for (i = 0; i < possible_cpus->len; i++) {
        if (possible_cpus->cpus[i].props.has_node_id) {
            break;
        }
    }
    default_mapping = (i == possible_cpus->len);

    for (i = 0; i < possible_cpus->len; i++) {
        const CPUArchId *cpu_slot = &possible_cpus->cpus[i];

        if (!cpu_slot->props.has_node_id) {
            /* fetch default mapping from board and enable it */
            CpuInstanceProperties props = cpu_slot->props;

            props.node_id = mc->get_default_cpu_node_id(machine, i);
            if (!default_mapping) {
                /* record slots with not set mapping,
                 * TODO: make it hard error in future */
                char *cpu_str = cpu_slot_to_string(cpu_slot);
                g_string_append_printf(s, "%sCPU %d [%s]",
                                       s->len ? ", " : "", i, cpu_str);
                g_free(cpu_str);

                /* non mapped cpus used to fallback to node 0 */
                props.node_id = 0;
            }

            props.has_node_id = true;
            machine_set_cpu_numa_node(machine, &props, &error_fatal);
        }
    }

    if (machine->numa_state->hmat_enabled) {
        numa_validate_initiator(machine->numa_state);
    }

    if (s->len && !qtest_enabled()) {
        warn_report("CPU(s) not present in any NUMA nodes: %s",
                    s->str);
        warn_report("All CPU(s) up to maxcpus should be described "
                    "in NUMA config, ability to start up with partial NUMA "
                    "mappings is obsoleted and will be removed in future");
    }
    g_string_free(s, true);
}

MemoryRegion *machine_consume_memdev(MachineState *machine,
                                     HostMemoryBackend *backend)
{
    MemoryRegion *ret = host_memory_backend_get_memory(backend);

    if (memory_region_is_mapped(ret)) {
        error_report("memory backend %s can't be used multiple times.",
                     object_get_canonical_path_component(OBJECT(backend)));
        exit(EXIT_FAILURE);
    }
    host_memory_backend_set_mapped(backend, true);
    vmstate_register_ram_global(ret);
    return ret;
}

void machine_run_board_init(MachineState *machine)
{
    MachineClass *machine_class = MACHINE_GET_CLASS(machine);

    if (machine->ram_memdev_id) {
        Object *o;
        o = object_resolve_path_type(machine->ram_memdev_id,
                                     TYPE_MEMORY_BACKEND, NULL);
        machine->ram = machine_consume_memdev(machine, MEMORY_BACKEND(o));
    }

    if (machine->numa_state) {
        numa_complete_configuration(machine);
        if (machine->numa_state->num_nodes) {
            machine_numa_finish_cpu_init(machine);
        }
    }

    /* If the machine supports the valid_cpu_types check and the user
     * specified a CPU with -cpu check here that the user CPU is supported.
     */
    if (machine_class->valid_cpu_types && machine->cpu_type) {
        ObjectClass *class = object_class_by_name(machine->cpu_type);
        int i;

        for (i = 0; machine_class->valid_cpu_types[i]; i++) {
            if (object_class_dynamic_cast(class,
                                          machine_class->valid_cpu_types[i])) {
                /* The user specificed CPU is in the valid field, we are
                 * good to go.
                 */
                break;
            }
        }

        if (!machine_class->valid_cpu_types[i]) {
            /* The user specified CPU is not valid */
            error_report("Invalid CPU type: %s", machine->cpu_type);
            error_printf("The valid types are: %s",
                         machine_class->valid_cpu_types[0]);
            for (i = 1; machine_class->valid_cpu_types[i]; i++) {
                error_printf(", %s", machine_class->valid_cpu_types[i]);
            }
            error_printf("\n");

            exit(1);
        }
    }

    machine_class->init(machine);
}

static const TypeInfo machine_info = {
    .name = TYPE_MACHINE,
    .parent = TYPE_OBJECT,
    .abstract = true,
    .class_size = sizeof(MachineClass),
    .class_init    = machine_class_init,
    .class_base_init = machine_class_base_init,
    .instance_size = sizeof(MachineState),
    .instance_init = machine_initfn,
    .instance_finalize = machine_finalize,
};

static void machine_register_types(void)
{
    type_register_static(&machine_info);
}

type_init(machine_register_types)
