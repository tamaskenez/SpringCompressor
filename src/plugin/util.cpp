#include "util.h"

#include <juce_audio_formats/juce_audio_formats.h>

std::optional<std::pair<std::vector<float>, double>> load_audio_file_first_channel(const std::filesystem::path& path)
{
    juce::AudioFormatManager format_manager;
    format_manager.registerBasicFormats();

    const std::unique_ptr<juce::AudioFormatReader> reader(format_manager.createReaderFor(juce::File(path.string())));
    if (!reader)
        return std::nullopt;

    const int num_samples = iicast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> buffer(1, num_samples);
    reader->read(&buffer, 0, num_samples, 0, true, false);

    const float* data = buffer.getReadPointer(0);
    return std::make_pair(std::vector<float>(data, data + num_samples), reader->sampleRate);
}
