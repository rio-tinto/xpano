# Xpano - Claude Notes

## Changes made

### Full resolution keypoint detection (`algorithm/image.cc`, `cli/pano_cli.cc`)
- `PreviewSize()` now treats `preview_longer_side == 0` as "no downsampling" (was computing a 0x0 size)
- CLI loading passes `preview_longer_side = 0` instead of `kMaxImageSizeForCLI` (2048), so SIFT keypoint detection and pano-grouping feature matching run on full-resolution images

### New CLI flags for bundle adjuster tuning (`cli/args.h`, `cli/args.cc`, `cli/pano_cli.cc`, `algorithm/options.h`, `algorithm/algorithm.cc`, `constants.h`)
- `--match-conf=<F>` (default 0.25, range 0.1–0.4): Lowe ratio test threshold for the stitcher's internal pairwise feature matching. Lower = more (weaker) matches retained, giving the bundle adjuster more constraints. Primary knob for `ERR_CAMERA_PARAMS_ADJUST_FAIL`.
- `--conf-thresh=<F>` (default 1.0, range 0.1–2.0): Confidence threshold used in `leaveBiggestComponent` and `bundle_adjuster_->setConfThresh()`. Lower = more image pairs included in bundle adjustment.
- Both are wired into `StitchUserOptions` and applied in `algorithm::Stitch()` via `SetPanoConfidenceThresh()`.

## Pipeline resolution overview (CLI)

| Stage | What runs | Resolution |
|---|---|---|
| Load + keypoint detect | SIFT on `preview_` | Full res (after fix) |
| Pano grouping match | SIFT descriptors from above | Full res (after fix) |
| Camera estimation (inside stitcher) | SIFT on `registr_resol_=0.6 Mpx` downscaled images | ~0.6 Mpx (intentional, not worth changing) |
| Warp + composite | Full res images via `GetFullRes()` | Full res (`full_res=true` default) |
| Output cap | `max_pano_mpx` downscale | 100 Mpx default, use `--max-pano-mpx` |

## Key constants (constants.h)
- `kMaxImageSizeForCLI = 2048` — no longer used after full-res fix
- `kDefaultConfThresh = 1.0`, range 0.1–2.0
- `kDefaultMatchConf = 0.25`, range 0.1–0.4
- `kMaxPanoMpx = 100` — raise with `--max-pano-mpx` for large panoramas
