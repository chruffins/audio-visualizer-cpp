#pragma once

#include <memory>
#include <vector>

#include "graphics/views/audio_vis.hpp"

namespace vis {

// Factories to create visualization implementations. They return instances
// of `ui::AudioVisualizerView::Visualization` that the view owns.
std::unique_ptr<ui::AudioVisualizerView::Visualization> createPolarWaveformVisualization();
std::unique_ptr<ui::AudioVisualizerView::Visualization> createMirrorBarsVisualization();
std::unique_ptr<ui::AudioVisualizerView::Visualization> createDualEchoWaveVisualization();

// Utility used by the view to split interleaved stereo samples into left/right buffers.
void splitInterleavedStereoSamples(const std::vector<float>& interleavedSamples, std::vector<float>& leftSamples, std::vector<float>& rightSamples);

// Sample window sizes used by the visualizations and the view.
constexpr std::size_t kPolarWaveformSampleWindow = 1024;
constexpr std::size_t kMirrorBarsSampleWindow = 768;
constexpr std::size_t kDualEchoStereoSampleWindow = 1024;
constexpr std::size_t kDualEchoMonoFallbackWindow = 512;

} // namespace vis
