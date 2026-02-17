// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "xpano/algorithm/options.h"
#include "xpano/pipeline/options.h"

namespace xpano::cli {

struct Args {
  bool run_gui = false;
  bool print_help = false;
  bool print_version = false;
  std::vector<std::filesystem::path> input_paths;
  std::optional<std::filesystem::path> output_path;

  // Projection
  std::optional<algorithm::ProjectionType> projection;

  // Matching
  std::optional<pipeline::MatchingType> matching_type;
  std::optional<int> match_threshold;
  std::optional<float> min_shift;

  // Export
  std::optional<int> jpeg_quality;
  std::optional<int> png_compression;
  std::optional<bool> copy_metadata;

  // Stitching
  std::optional<algorithm::WaveCorrectionType> wave_correction;
  std::optional<int> max_pano_mpx;
};

std::optional<Args> ParseArgs(int argc, char** argv);

void PrintHelp();

}  // namespace xpano::cli
