#include "misc.h"

juce::String to_juce_string(std::string_view sv)
{
    return juce::String(sv.data(), sv.size());
}

juce::StringArray choices_for(juce::AudioProcessorValueTreeState& apvts, const char* parameter_id)
{
    return dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameter_id))->choices;
}

static juce::ComboBox& populate_combo(juce::ComboBox& cb, const juce::StringArray& items)
{
    cb.addItemList(items, 1);
    return cb;
}

ComboBoxWithAttachment::ComboBoxWithAttachment(
  juce::AudioProcessorValueTreeState& apvts, const juce::String& parameter_id, const juce::StringArray& items
)
    : combo(parameter_id)
    , attachment(apvts, parameter_id, populate_combo(combo, items))
{
}
