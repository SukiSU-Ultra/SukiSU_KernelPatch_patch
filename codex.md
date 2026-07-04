# Codex Notes

This repo has a persistent on-disk preset layout. Treat changes in `kernel/include/preset.h` and `tools/preset.h` as ABI changes, not normal refactors.

## Rules

- Do not change `MAP_SYMBOL_NUM` casually.
- `map_symbol_t` sits before `header_backup` inside `setup_preset_t`, so changing `MAP_SYMBOL_NUM` moves `header_backup` and every field after it.
- That offset drift is what broke upgrade parsing across `0.13.1`, `0.13.2`, and newer builds.
- `tools/preset.h` is the tools-side mirror of `kernel/include/preset.h`; keep them in sync when layout changes.

## Known Header Backup Offsets

- Official upstream `0.13.0` / `0.13.1`: `current_header_backup_offset - 16`
- Official upstream `0.13.2` and current `main`: `current_header_backup_offset`
- `current_header_backup_offset - 8` is kept only as a fallback for non-official intermediate/local images

`tools/patch.c` resolves this through `preset_header_backup()` and validates candidates with the saved primary-entry header.

## Safe Future Changes

- Prefer fixed-capacity storage for persistent structs.
- Prefer using reserved space like `__[]` for small new metadata.
- If a serialized struct must change, add explicit versioned parsing instead of relying on current `offsetof(...)`.

## Validation

- Rebuild tools with `wsl -d Ubuntu bash -lc "cd /mnt/e/Project/self/KernelPatch/tools && make"`.
- Re-test upgrade flow with:
  - old patched boot -> unpack
  - repatch extracted kernel
  - repack boot
  - unpack repacked boot
  - repatch again
