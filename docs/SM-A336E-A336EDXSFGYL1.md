# SM-A336E A336EDXSFGYL1 port record

Status: **profile derived and building; hardware execution not yet reliable.**
Every constant below comes from this exact firmware, not from another device.
The remaining work is reclaim/root-stage tuning on hardware (see the last
section), which is the same class of work the A536E profile needed.

| Field | Value |
| --- | --- |
| Model | `SM-A336E` (Galaxy A33 5G, `a33x`, Exynos 1280 / s5e8825) |
| CSC | `XXV` (Vietnam) |
| Firmware | `A336EDXSFGYL1` / `A336EOLMFGYL1` |
| Android | 16 / API 36 |
| Fingerprint | `samsung/a33xnsdxx/a33x:16/BP2A.250605.031.A3/A336EDXSFGYL1:user/release-keys` |
| Kernel | `5.10.237-android12-9-31999025-abA336EDXSFGYL1` |
| Kernel image | 34,048,512 bytes, SHA-256 `519974B090DC8625085BEB0723CDED3EF3CB0A10BACDCDB195D18C270361C27B` |
| boot.img | 67,108,864 bytes, SHA-256 `A3DE77010DB539AFA537D7D82EDBC65C00015FAF1ACAA6358867E3E12939E755` |
| Page size | 4096 (4 KiB) |
| ELF base | `0xffffffc008000000` (vmlinux-to-elf, 113,783 symbols) |

## 1. Firmware acquisition

`samloader-rs` 2.2.0 (`topjohnwu/samloader-rs`, release SHA-256
`0132de1b79e208138f980c467f9a7eaa2ffcb72aae14afc3ed13799803c9b9ed`) reported
`A336EDXSFGYL1/A336EOLMFGYL1/A336EDXSFGYL1/A336EDXSFGYL1` as a published
version for `SM-A336E`/`XXV`, identical to the installed build. The AP archive
was extracted and `boot.img.lz4` decompressed with `lz4.frame`; the ARM64 Image
is boot header v4 with `kernel_size` 0x2079000 at offset `0x1000`,
`text_offset = 0`, `image_size = 0x2370000`, `flags = 0xa`.

## 2. Configuration

The image embeds IKCONFIG (`IKCFG_ST` at `0x1223220`); the extracted config is
8,344 lines. Relevant entries:

```text
CONFIG_ARM64_VA_BITS=39          CONFIG_PGTABLE_LEVELS=3
CONFIG_ARM64_4K_PAGES=y          CONFIG_SLUB=y
CONFIG_SLAB_FREELIST_RANDOM=y    CONFIG_SLAB_FREELIST_HARDENED=y
# CONFIG_SLUB_DEBUG_ON is not set
CONFIG_HARDENED_USERCOPY=y       CONFIG_RANDOMIZE_BASE=y
CONFIG_MODULES=y                 CONFIG_MODVERSIONS=y
CONFIG_TRIM_UNUSED_KSYMS=y       CONFIG_SHADOW_CALL_STACK=y
CONFIG_CFI_CLANG=y               CONFIG_VMAP_STACK=y
CONFIG_RT_MUTEXES=y              CONFIG_FUTEX_PI=y
CONFIG_ASHMEM=y                  CONFIG_CONFIGFS_FS=y
CONFIG_PREEMPT=y                 CONFIG_SMP=y   (NR_CPUS=32)
```

There is **no BTF** in this Samsung 5.10 image (the strict raw-BTF scan from
`PORTING.md` finds no valid blob). Layouts were therefore taken from the
target's own disassembly, as the A155N procedure does.

## 3. Symbols (image offsets from `0xffffffc008000000`)

| Use | Symbol | Offset |
| --- | --- | ---: |
| `INIT_MM_IMAGE` | `init_mm` | `0x01f64f38` |
| `INIT_TASK_BASE` | `init_task` | `0x01e0dd00` |
| `SELINUX_STATE_ALIAS` | `selinux_state` | `0x021ddb68` |
| `ASHMEM_MISC_FOPS_ALIAS` | `ashmem_misc` + `offsetof(miscdevice, fops)` | `0x01ffbbd0` + `0x10` |
| `ASHMEM_FOPS_IMAGE` | `ashmem_fops` | `0x01b08858` |
| `SYSTEM_UNBOUND_WQ_ALIAS` | `system_unbound_wq` | `0x01df9e10` |
| `PWQ_CACHE_ALIAS` | `pwq_cache` | `0x01dfcb78` |
| `CALL_USERMODEHELPER_EXEC_WORK_IMAGE` | `call_usermodehelper_exec_work` | `0x000f6be4` |
| llseek slot | `noop_llseek` | `0x003789dc` |
| read slot | `configfs_read_file` (LLVM-suffixed) | `0x0044771c` |
| write slot | `configfs_write_bin_file` (LLVM-suffixed) | `0x00447b7c` |
| splice slot | `generic_file_splice_read` | `0x003c3ad8` |

The `configfs_*` symbols carry `.llvm.<hash>` suffixes because of CFI
internalisation; they must be matched as prefixes.

`miscdevice.fops == 0x10` was confirmed from the initialised `ashmem_misc`
object in the image (`minor = 0xff`, `name -> "ashmem"`, `fops -> ashmem_fops`)
and `ashmem_fops` itself supplied the ioctl/compat_ioctl/mmap/open/release/
show_fdinfo slots used by the forged table.

## 4. Structures

| Structure | Evidence | Value |
| --- | --- | --- |
| `sizeof(struct mm_struct)` | `mm_cache_init` → `kmem_cache_create_usercopy("mm_struct", 0x3c0, 0, …, 0x168, 0x170, …)` | `0x3c0` |
| `mm_struct.pgd` | `mm_init` stores `pgd_alloc()` result at `[mm + 0x48]` | `0x48` |
| `configfs_buffer.page` | `configfs_read_file` reads `[buf + 0x10]` | `0x10` |
| `configfs_buffer.needs_read_fill` | idem, `[buf + 0x50]` | `0x50` |
| `configfs_buffer.bin_buffer` | `configfs_write_bin_file` `[buf + 0x58]` | `0x58` |
| `…bin_buffer_size` / `cb_max_size` | idem `[buf + 0x60]` / `[buf + 0x64]` | `0x60` / `0x64` |
| `kmem_cache.usersize` | `__check_heap_object` `[cache + 0xf4]` useroffset, `[cache + 0xf8]` usersize | `0xf8` |
| `workqueue_struct.dfl_pwq` | `apply_wqattrs_commit` load/store `[wq + 0xb0]` | `0xb0` |
| `pool_workqueue.pool/wq/work_color/refcnt` | `__queue_work` | `0x00/0x08/0x10/0x18` |
| `…nr_in_flight/nr_active/max_active` | `__queue_work` (`[pwq+0x1c+c*4]`, `ldp [pwq+0x58]`) | `0x1c/0x58/0x5c` |
| `worker_pool.worklist/nr_idle` | `__queue_work` (`pool+0x20` head, `idle_list` at `0x38`) | `0x20/0x34` |
| `work_struct.entry/func` | `__queue_work` | `0x08/0x18` |
| `subprocess_info` | `call_usermodehelper_exec_work` (`complete` 0x30, `wait` 0x50, `retval` 0x54, `cleanup` 0x60) | work 0x00..0x30 |
| `selinux_state.enforcing` | `sel_write_enforce` `ldarb [state]`, mask 1 | `0x00` |

`struct mutex` in this build occupies `0x30` bytes (`configfs_buffer.mutex`
spans `0x20..0x50`), which is why `needs_read_fill` is at `0x50`.

## 5. Memory map and KASLR

The bootloader injects `/memory` nodes, so the DTBs in `boot`/`vendor_boot`/
`dtbo` carry no memory ranges. The live map was read from the device:

- `/proc/device-tree/memory@80000000` and `memory@880000000`;
- `/sys/devices/system/memory/` contains blocks 16..31 (2 GiB at `0x80000000`)
  and 272..303 (4 GiB at `0x880000000`), i.e. a 6 GiB unit with a 2 GiB + 4 GiB
  split.

With `PAGE_OFFSET = 0xffffff8000000000` and `PHYS_OFFSET = 0x80000000`:

```text
DMA32 window   0xffffff8000000000 .. 0xffffff8080000000
Normal window  0xffffff8800000000 .. 0xffffff8980000000
```

**The inherited KASLR heuristic does not work on this device.** The A536E
scanner requires eight consecutive unmapped 32 KiB samples (256 KiB) below the
image before it accepts an edge. Observed offsets here are as small as
`0x18000`, so the scan finds nothing there and can then lock onto a false
transition far above the image. Addresses derived that way are wrong; the
observed consequences were a missed SELinux write (so `/dev/ashmem` open stayed
denied, `EACCES`) and, on another boot, a panic.

Measured on a live boot at 4 KiB granularity (PRFM only):

```text
offset 0x0000..0x6fff   ~137 ticks   unmapped
offset 0x7000..0x16fff  ~112 ticks   image head, not fully mapped
offset 0x18000..        ~14..27 ticks mapped
```

The `a33x` profile therefore classifies with a strict quarter-range threshold
(`mapped + (unmapped - mapped)/4`), accepts the first unmapped → mapped
transition only after four consecutive mapped 4 KiB pages, and refines the
result to 4 KiB. It reports `0x18000` and `0xf8000` on different boots, both
32 KiB aligned, consistently across repeats.

## 6. Futex/pselect write primitive

`remove_waiter` loads `current` with `mrs x20, SP_EL0` and touches
`current->pi_lock` (`0x86c`) and `current->pi_blocked_on` (`0x898`), so the bug
is present.

`struct rt_mutex_waiter` is at `sp + 0x90` in `futex_wait_requeue_pi`
(frame `0x1a0`), and `core_sys_select` puts the three fd-sets at `sp + 0x50`
(frame `0x1c0`). With `__arm64_sys_futex` frame `0x70` and `do_futex` `0x60`
versus `__arm64_sys_pselect6` frame `0xa0`, the fd-set buffer sits `0x30`
bytes below the waiter, i.e. logical fd-set qword **6** is waiter offset 0.
That is the same mapping the A536E profile hardcodes (`write_set[1]`,
`write_set[2]`, `except_set[2]`, `except_set[3]`), so the primitive carries
over unchanged.

## 7. Build

```sh
make TARGET=a33x-A336EDXSFGYL1 ANDROID_NDK_HOME=/path/to/ndk release
```

The host used for this port has no `make`; the same flags were driven directly
through NDK 29.0.14206865 `clang --target=aarch64-linux-android31`.

```text
build/a33x-A336EDXSFGYL1/cve-2026-43499-app.release.so   46,432 bytes
build/a33x-A336EDXSFGYL1/cve-2026-43499-app.so         104,128 bytes (padded)
SHA-256 4886595e7cbe2b2e5526b2c99cf50080b649f42e4de8eaf4fad7744064d48faf
```

## 8. Hardened usercopy and the root stage

`copy_to_user` from a slab object is rejected unless the owning cache's
`useroffset/usersize` window covers the range (`__check_heap_object` reads
`cache+0xf4`/`cache+0xf8`). The forged-fops read primitive is exactly such a
copy, so every slab object the chain reads must have its cache relaxed first.
This is why the shared code patches `pwq_cache->usersize` before touching a
`pool_workqueue`.

Two further objects are read by `install_umh_root`, and both needed work:

- `system_unbound_wq` itself. `alloc_workqueue` sizes the object with
  `csel x0, #0x140, #0x148` on the `WQ_UNBOUND` bit, so an unbound queue is a
  kmalloc-512 object (index 9), and reading `wq->dfl_pwq` aborts usercopy until
  that cache is relaxed. The a33x profile patches it via
  `kmalloc_caches[NORMAL][9]`, resolved at runtime from the global array
  (`kmalloc_slab` shows a 0x70-byte stride per type, 14 entries, and
  `type = (gfp >> 4) & 1`, so `GFP_KERNEL` is type 0, offset `9 * 8 = 0x48`).
- `worker_pool`. Its cache is not covered by any patch, so the pool-side
  sanity reads (empty worklist, `nr_idle`) were dropped. Inserting into a live
  pool's worklist is valid whether or not it is empty, and the gate that
  matters (`pwq->nr_active`) comes from the already-patched `pwq_cache`.

With both changes the chain reaches and completes the `pwq_cache` patch
(`PATCH_OK`) and issues the kmalloc-512 patch (`PATCH2_REQUEST`).

## 9. Hardware status

Runs from the app (`u:r:untrusted_app:s0`) and from `adb shell` both work; the
app path is the realistic one.

| Stage | Result |
| --- | --- |
| KASLR slide | works (`0x18000`, `0x78000`, `0x130000`, `0x198000` ...) |
| KernelSnitch `mm_struct` leak | works |
| 34/34 Normal-slab group | works (one run needed 67 scans) |
| SLUB/SKB reclaim + forged fops | `ARW_OK` on most runs |
| `pwq_cache` usersize patch | `PATCH_OK` |
| kmalloc-512 usersize patch | `PATCH2_REQUEST` issued; the write itself still fails intermittently |
| usermode-helper root | not reached yet |

The remaining failures are in the futex/pselect write primitive itself, not in
derived data. The chain performs several independent q0 writes (SELinux state,
two fops slots, two cache patches), and roughly one run in two panics inside
one of them, or earlier during the slab-group search. The ARW proof reads back
exactly the written values, so addressing and layouts are correct; what is
unstable is the race the primitive depends on. `dmesg`, `/proc/last_kmsg` and
`/sys/fs/pstore` are all root-only on this build, so the oops text is not
available to guide the next iteration.

Each write stage is now logged with `fsync` after every step, which located
the crash window precisely. A completed stage reads:

```text
requeue enter
requeue returned=-1 errno=35      (EAGAIN)
wait returned=-1 errno=110        (ETIMEDOUT, 2 s timeout)
pselect enter nfds=320            (fd-sets overwrite the stale waiter)
setattr enter tid=...
sched_setattr returned=0
consume done
```

The panics occur between `requeue returned` and `wait returned`, i.e. inside
the vulnerable rt-mutex chain walk, before the crafted fd-set words are placed.
That matches the bug: `remove_waiter` clears `current->pi_blocked_on` instead
of `waiter->task`'s, leaving a stale waiter pointer reachable from the requeue
path. It is a timing race, which is why the failure point moves between the
slab drain, the first write and the second write across runs.

Two operational findings:

- **Memory pressure matters.** On a 6 GiB unit the group search plus the DMA32
  holds leave very little free memory (observed 166 MiB free with ~1 GiB of
  swap in use). A heavier leak setting (6-way collision agreement, tried here)
  wedged the collision child in a userspace spin instead of failing cleanly, so
  the A536E 4/2 split was restored.
- **Freshness matters.** Runs started within ~60 s of boot are less stable;
  letting the device settle, and using `am kill-all` first, gives the most
  consistent results.

`A33X_TRIGGER_SLABS`, `A33X_PAGE_SCAN_MAX`, `MM_PARTIALS`,
`A33X_SKB_SENDS`/`SNDBUF` are inherited from the A536E profile; they are
allocator/scheduler ratios and still need per-device tuning. A trace build
(`-DA33X_TRACE=1`) prints every read/write step in `install_umh_root` and
should be used for further iterations. Each attempt needs a fresh boot; a
failure panics and returns the device to stock in about 15 seconds.

## 10. Dev-fork app integration

Because the released app resolves `support/targets-v3.json` from the upstream
repository `main` and requires a `kernelsu` artifact, this profile cannot be
exercised from a stock APK before publication. For local testing the app was
built from source with these changes (dev fork only, not upstream):

1. `PayloadRepository` reads `http://127.0.0.1:8099/targets-v3.json`;
2. `kernelsu` is optional in `SupportManifest` and the late-load stage is
   skipped when absent;
3. `InstallActivity` is exported so the flow can be driven from adb;
4. `usesCleartextTraffic` for the loopback feed, `compileSdk 36`.

The feed is served from the host and reached over `adb reverse tcp:8099
tcp:8099`. A publishable upstream entry additionally needs
`kernelsu/ksud-A336EDXSFGYL1-kdp` built per `PORTING.md` section 7 with
vermagic `5.10.237-android12-9-31999025-abA336EDXSFGYL1 SMP preempt mod_unload
modversions aarch64`.
