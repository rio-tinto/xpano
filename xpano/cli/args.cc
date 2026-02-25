// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/cli/args.h"

#include <algorithm>
#include <charconv>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

#include "xpano/algorithm/options.h"
#include "xpano/constants.h"
#include "xpano/utils/fmt.h"
#include "xpano/utils/path.h"

namespace xpano::cli {

namespace {

const std::string kGuiFlag = "--gui";
const std::string kOutputFlag = "--output=";
const std::string kHelpFlag = "--help";
const std::string kVersionFlag = "--version";
const std::string kProjectionFlag = "--projection=";
const std::string kMatchingTypeFlag = "--matching-type=";
const std::string kMatchThresholdFlag = "--match-threshold=";
const std::string kMinShiftFlag = "--min-shift=";
const std::string kJpegQualityFlag = "--jpeg-quality=";
const std::string kPngCompressionFlag = "--png-compression=";
const std::string kCopyMetadataFlag = "--copy-metadata";
const std::string kNoCopyMetadataFlag = "--no-copy-metadata";
const std::string kWaveCorrectionFlag = "--wave-correction=";
const std::string kMaxPanoMpxFlag = "--max-pano-mpx=";

std::optional<int> ParseInt(const std::string& str) {
  int value;
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ec == std::errc() && result.ptr == str.data() + str.size()) {
    return value;
  }
  return std::nullopt;
}

std::optional<float> ParseFloat(const std::string& str) {
  try {
    size_t pos;
    float value = std::stof(str, &pos);
    if (pos == str.size()) {
      return value;
    }
  } catch (...) {
  }
  return std::nullopt;
}

std::optional<algorithm::ProjectionType> ParseProjectionType(
    const std::string& str) {
  if (str == "perspective") return algorithm::ProjectionType::kPerspective;
  if (str == "cylindrical") return algorithm::ProjectionType::kCylindrical;
  if (str == "spherical") return algorithm::ProjectionType::kSpherical;
  if (str == "fisheye") return algorithm::ProjectionType::kFisheye;
  if (str == "stereographic") return algorithm::ProjectionType::kStereographic;
  if (str == "rectilinear")
    return algorithm::ProjectionType::kCompressedRectilinear;
  if (str == "panini") return algorithm::ProjectionType::kPanini;
  if (str == "mercator") return algorithm::ProjectionType::kMercator;
  if (str == "transverse-mercator")
    return algorithm::ProjectionType::kTransverseMercator;
  return std::nullopt;
}

std::optional<algorithm::WaveCorrectionType> ParseWaveCorrectionType(
    const std::string& str) {
  if (str == "off") return algorithm::WaveCorrectionType::kOff;
  if (str == "auto") return algorithm::WaveCorrectionType::kAuto;
  if (str == "horizontal") return algorithm::WaveCorrectionType::kHorizontal;
  if (str == "vertical") return algorithm::WaveCorrectionType::kVertical;
  return std::nullopt;
}

std::optional<pipeline::MatchingType> ParseMatchingType(
    const std::string& str) {
  if (str == "auto") return pipeline::MatchingType::kAuto;
  if (str == "single") return pipeline::MatchingType::kSinglePano;
  if (str == "none") return pipeline::MatchingType::kNone;
  return std::nullopt;
}

void ParseArg(Args* result, const std::string& arg) {
  if (arg == kGuiFlag) {
    result->run_gui = true;
  } else if (arg == kHelpFlag) {
    result->print_help = true;
  } else if (arg == kVersionFlag) {
    result->print_version = true;
  } else if (arg.starts_with(kOutputFlag)) {
    auto substr = arg.substr(kOutputFlag.size());
    result->output_path = std::filesystem::path(substr);
  } else if (arg.starts_with(kProjectionFlag)) {
    auto substr = arg.substr(kProjectionFlag.size());
    result->projection = ParseProjectionType(substr);
  } else if (arg.starts_with(kMatchingTypeFlag)) {
    auto substr = arg.substr(kMatchingTypeFlag.size());
    result->matching_type = ParseMatchingType(substr);
    if (!result->matching_type) {
      spdlog::warn("Invalid --matching-type '{}', using default (auto). Valid: auto, single, none", substr);
    }
  } else if (arg.starts_with(kMatchThresholdFlag)) {
    auto substr = arg.substr(kMatchThresholdFlag.size());
    result->match_threshold = ParseInt(substr);
  } else if (arg.starts_with(kMinShiftFlag)) {
    auto substr = arg.substr(kMinShiftFlag.size());
    result->min_shift = ParseFloat(substr);
  } else if (arg.starts_with(kJpegQualityFlag)) {
    auto substr = arg.substr(kJpegQualityFlag.size());
    result->jpeg_quality = ParseInt(substr);
  } else if (arg.starts_with(kPngCompressionFlag)) {
    auto substr = arg.substr(kPngCompressionFlag.size());
    result->png_compression = ParseInt(substr);
  } else if (arg == kCopyMetadataFlag) {
    result->copy_metadata = true;
  } else if (arg == kNoCopyMetadataFlag) {
    result->copy_metadata = false;
  } else if (arg.starts_with(kWaveCorrectionFlag)) {
    auto substr = arg.substr(kWaveCorrectionFlag.size());
    result->wave_correction = ParseWaveCorrectionType(substr);
  } else if (arg.starts_with(kMaxPanoMpxFlag)) {
    auto substr = arg.substr(kMaxPanoMpxFlag.size());
    result->max_pano_mpx = ParseInt(substr);
  } else {
    result->input_paths.emplace_back(arg);
  }
}

Args ParseArgsRaw(int argc, char** argv) {
  Args result;
  const std::vector<std::filesystem::path> input_paths;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    ParseArg(&result, arg);
  }
  return result;
}

bool ValidateArgs(const Args& args) {
  if (args.output_path && args.input_paths.empty()) {
    spdlog::error("No supported images provided");
    return false;
  }
  if (args.output_path &&
      !utils::path::IsExtensionSupported(*args.output_path)) {
    spdlog::error("Unsupported output file extension: \"{}\"",
                  args.output_path->extension().string());
    return false;
  }
  if (args.output_path && args.run_gui) {
    spdlog::error(
        "Specifying --gui and --output together is not yet supported.");
    return false;
  }
  if (args.match_threshold.has_value() && !args.match_threshold.value()) {
    spdlog::error("Invalid value for --match-threshold");
    return false;
  }
  if (args.match_threshold.has_value()) {
    int val = *args.match_threshold;
    if (val < kMinMatchThreshold || val > kMaxMatchThreshold) {
      spdlog::error("--match-threshold must be between {} and {}",
                    kMinMatchThreshold, kMaxMatchThreshold);
      return false;
    }
  }
  if (args.min_shift.has_value()) {
    float val = *args.min_shift;
    if (val < kMinShiftInPano || val > kMaxShiftInPano) {
      spdlog::error("--min-shift must be between {} and {}", kMinShiftInPano,
                    kMaxShiftInPano);
      return false;
    }
  }
  if (args.jpeg_quality.has_value()) {
    int val = *args.jpeg_quality;
    if (val < 0 || val > kMaxJpegQuality) {
      spdlog::error("--jpeg-quality must be between 0 and {}", kMaxJpegQuality);
      return false;
    }
  }
  if (args.png_compression.has_value()) {
    int val = *args.png_compression;
    if (val < 0 || val > kMaxPngCompression) {
      spdlog::error("--png-compression must be between 0 and {}",
                    kMaxPngCompression);
      return false;
    }
  }
  if (args.max_pano_mpx.has_value()) {
    int val = *args.max_pano_mpx;
    if (val < 1 || val > 5000) {
      spdlog::error("--max-pano-mpx must be between 1 and 5000");
      return false;
    }
  }
  return true;
}

}  // namespace

std::vector<std::filesystem::path> ExpandDirectories(
    const std::vector<std::filesystem::path>& paths) {
  std::vector<std::filesystem::path> result;
  for (const auto& path : paths) {
    if (std::filesystem::is_directory(path)) {
      spdlog::info("Expanding directory: {}", path.string());
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file()) {
          result.push_back(entry.path());
        }
      }
    } else {
      result.push_back(path);
    }
  }
  return result;
}

std::optional<Args> ParseArgs(int argc, char** argv) {
  Args args;
  try {
    args = ParseArgsRaw(argc, argv);
  } catch (const std::exception& e) {
    spdlog::error("Error parsing arguments: {}", e.what());
    return {};
  }

  // Expand any directories to their contained files
  args.input_paths = ExpandDirectories(args.input_paths);

  auto supported_inputs = utils::path::KeepSupported(args.input_paths);
  if (supported_inputs.empty() && !args.input_paths.empty()) {
    spdlog::error("No supported images provided!");
    return {};
  }
  args.input_paths = supported_inputs;
  std::sort(args.input_paths.begin(), args.input_paths.end());

  if (!ValidateArgs(args)) {
    return {};
  }

  return args;
}

void PrintHelp() {
  spdlog::info("Xpano v1.3 - added matching type flag");
  spdlog::info("");
  spdlog::info("Usage: Xpano [<input files or directories>] [options]");
  spdlog::info("");
  spdlog::info("Options:");
  spdlog::info("  --output=<path>          Output file path");
  spdlog::info("  --gui                    Launch GUI mode");
  spdlog::info("  --help                   Show this help message");
  spdlog::info("  --version                Show version");
  spdlog::info("");
  spdlog::info("Projection:");
  spdlog::info("  --projection=<type>      Projection type (default: spherical)");
  spdlog::info("                           Types: perspective, cylindrical, spherical,");
  spdlog::info("                           fisheye, stereographic, rectilinear, panini,");
  spdlog::info("                           mercator, transverse-mercator");
  spdlog::info("");
  spdlog::info("Matching:");
  spdlog::info("  --matching-type=<type>   Matching mode (default: auto)");
  spdlog::info("                           Types: auto, single, none");
  spdlog::info("                           auto: pairwise matching, recommended");
  spdlog::info("                           single: assume all images form one pano");
  spdlog::info("                           none: skip matching");
  spdlog::info("  --match-threshold=<N>    Match threshold, {} - {} (default: {})",
               kMinMatchThreshold, kMaxMatchThreshold, kDefaultMatchThreshold);
  spdlog::info("  --min-shift=<F>          Min shift filter, {} - {} (default: {})",
               kMinShiftInPano, kMaxShiftInPano, kDefaultShiftInPano);
  spdlog::info("");
  spdlog::info("Export:");
  spdlog::info("  --jpeg-quality=<N>       JPEG quality, 0 - {} (default: {})",
               kMaxJpegQuality, kDefaultJpegQuality);
  spdlog::info("  --png-compression=<N>    PNG compression, 0 - {} (default: {})",
               kMaxPngCompression, kDefaultPngCompression);
  spdlog::info("  --copy-metadata          Copy EXIF from first image");
  spdlog::info("  --no-copy-metadata       Don't copy EXIF metadata");
  spdlog::info("");
  spdlog::info("Stitching:");
  spdlog::info("  --wave-correction=<type> Wave correction (default: auto)");
  spdlog::info("                           Types: off, auto, horizontal, vertical");
  spdlog::info("  --max-pano-mpx=<N>       Max panorama size in megapixels (default: {})",
               kMaxPanoMpx);
  spdlog::info("");
  spdlog::info("Supported formats: {}", fmt::join(kSupportedExtensions, ", "));
}

}  // namespace xpano::cli
