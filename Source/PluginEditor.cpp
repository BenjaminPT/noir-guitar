#include "PluginEditor.h"

//==============================================================================
NoirLookAndFeel::NoirLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, dim);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, line);
    setColour (juce::PopupMenu::highlightedTextColourId, accent);
    setColour (juce::TextButton::textColourOffId, dim);
    setColour (juce::TextButton::textColourOnId, text);
}

void NoirLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (line);
    g.strokePath (track, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Bipolar-looking controls (centre detent) fill from the middle.
    const bool bipolar = s.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? startAngle + 0.5f * (endAngle - startAngle) : startAngle;

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (s.isEnabled() ? accent : dim);
    g.strokePath (value, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path tick;
    tick.startNewSubPath (centre.x + std::sin (angle) * radius * 0.35f, centre.y - std::cos (angle) * radius * 0.35f);
    tick.lineTo (centre.x + std::sin (angle) * radius * 0.78f, centre.y - std::cos (angle) * radius * 0.78f);
    g.setColour (text);
    g.strokePath (tick, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void NoirLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const auto dot = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ 9.0f, r.getCentreY() });
    g.setColour (b.getToggleState() ? accent : line);
    g.fillEllipse (dot);
    g.setColour (hl ? text : (b.getToggleState() ? text : dim));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (b.getButtonText(), b.getLocalBounds().withTrimmedLeft (20), juce::Justification::centredLeft);
}

void NoirLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? line : (hl ? line.brighter (0.1f) : juce::Colours::transparentBlack));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void NoirLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 3.5f, cy - 1.5f, cx + 3.5f, cy - 1.5f, cx, cy + 2.5f);
    g.setColour (dim);
    g.fillPath (arrow);
}

//==============================================================================
NoirEditor::NoirEditor (NoirProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    sInput.title = "INPUT";   sDrive.title = "DRIVE";   sAmp.title = "AMP";
    sCab.title = "CAB";       sDelay.title = "DELAY";   sReverb.title = "REVERB";
    sOut.title = "OUTPUT";

    addKnob (ids::in,       "Input",    sInput);
    addKnob (ids::gate,     "Gate",     sInput);

    addKnob (ids::odDrive,  "Drive",    sDrive);
    addKnob (ids::odTone,   "Tone",     sDrive);
    addKnob (ids::odLevel,  "Level",    sDrive).slider.getProperties().set ("bipolar", true);

    addKnob (ids::gain,     "Gain",     sAmp);
    addKnob (ids::bass,     "Bass",     sAmp).slider.getProperties().set ("bipolar", true);
    addKnob (ids::mid,      "Mid",      sAmp).slider.getProperties().set ("bipolar", true);
    addKnob (ids::treble,   "Treble",   sAmp).slider.getProperties().set ("bipolar", true);
    addKnob (ids::presence, "Presence", sAmp);
    addKnob (ids::master,   "Master",   sAmp);

    addKnob (ids::dlyTime,  "Time",     sDelay);
    addKnob (ids::dlyFb,    "Repeats",  sDelay);
    addKnob (ids::dlyMix,   "Mix",      sDelay);

    addKnob (ids::revSize,  "Size",     sReverb);
    addKnob (ids::revMix,   "Mix",      sReverb);

    addKnob (ids::out,      "Level",    sOut);

    driveOn.setButtonText ("On");
    cabOn.setButtonText ("On");
    addAndMakeVisible (driveOn);
    addAndMakeVisible (cabOn);
    driveOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::odOn, driveOn);
    cabOnAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::cabOn, cabOn);

    cabType.addItemList ({ "1x12 Open", "2x12 Combo", "4x12 Closed" }, 1);
    addAndMakeVisible (cabType);
    cabTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::cabType, cabType);

    addAndMakeVisible (irButton);
    addChildComponent (irClear);
    irButton.onClick = [this] { chooseIR(); };
    irClear.onClick = [this] { proc.clearIR(); };

    setSize (860, 440);
    startTimerHz (30);
    timerCallback();
}

NoirEditor::~NoirEditor()
{
    setLookAndFeel (nullptr);
}

NoirEditor::Knob& NoirEditor::addKnob (const char* paramId, const juce::String& name, Section& s)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k->slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    k->slider.setDoubleClickReturnValue (true, 0.0);
    k->slider.setMouseDragSensitivity (180);
    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k->slider);

    // Double click resets to the parameter default.
    if (auto* par = proc.apvts.getParameter (paramId))
        k->slider.setDoubleClickReturnValue (true, par->convertFrom0to1 (par->getDefaultValue()));

    auto& ref = *k;
    s.knobs.push_back (&ref);
    knobs.push_back (std::move (k));
    return ref;
}

void NoirEditor::chooseIR()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a cabinet impulse response (WAV)",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  proc.loadIR (f);
                              timerCallback();
                          });
}

void NoirEditor::timerCallback()
{
    // Fast attack, smooth fall for the thin meters.
    const float ip = proc.inPeak.exchange (0.0f);
    const float op = proc.outPeak.exchange (0.0f);
    inLevel  = juce::jmax (ip, inLevel * 0.82f);
    outLevel = juce::jmax (op, outLevel * 0.82f);

    const bool ir = proc.hasIR();
    irButton.setButtonText (ir ? proc.getIRName() : "Load IR");
    irClear.setVisible (ir);
    cabType.setEnabled (! ir);
    cabType.setAlpha (ir ? 0.4f : 1.0f);

    repaint (meterIn.expanded (2));
    repaint (meterOut.expanded (2));
}

//==============================================================================
void NoirEditor::paint (juce::Graphics& g)
{
    g.fillAll (NoirLookAndFeel::bg);

    // Wordmark
    g.setColour (NoirLookAndFeel::text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::plain));
    g.drawText ("NOIR", 28, 18, 100, 26, juce::Justification::centredLeft);
    g.setColour (NoirLookAndFeel::dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("GUITAR", 82, 18, 80, 26, juce::Justification::centredLeft);

    auto drawSection = [&] (const Section& s)
    {
        g.setColour (NoirLookAndFeel::panel);
        g.fillRoundedRectangle (s.area.toFloat(), 8.0f);
        g.setColour (NoirLookAndFeel::line);
        g.drawRoundedRectangle (s.area.toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.setColour (NoirLookAndFeel::dim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (s.title, s.area.getX() + 14, s.area.getY() + 8, 80, 16, juce::Justification::centredLeft);
    };

    for (auto* s : { &sInput, &sDrive, &sAmp, &sCab, &sDelay, &sReverb, &sOut })
        drawSection (*s);

    auto drawMeter = [&] (juce::Rectangle<int> r, float level)
    {
        g.setColour (NoirLookAndFeel::line);
        g.fillRoundedRectangle (r.toFloat(), 1.5f);
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float f = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour (db > -1.0f ? juce::Colour (0xffd9534f) : NoirLookAndFeel::accent);
        g.fillRoundedRectangle (r.toFloat().withTrimmedTop (r.getHeight() * (1.0f - f)), 1.5f);
    };

    drawMeter (meterIn, inLevel);
    drawMeter (meterOut, outLevel);
}

void NoirEditor::resized()
{
    const int margin = 20, gap = 10;
    auto area = getLocalBounds().reduced (margin);
    area.removeFromTop (34);

    const int rowH = (area.getHeight() - gap) / 2;
    auto rowA = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto rowB = area;

    auto layoutKnobs = [] (Section& s, juce::Rectangle<int> r)
    {
        if (s.knobs.empty()) return;
        const int w = r.getWidth() / (int) s.knobs.size();
        for (auto* k : s.knobs)
        {
            auto c = r.removeFromLeft (w);
            k->label.setBounds (c.removeFromBottom (16));
            k->slider.setBounds (c);
        }
    };

    // Row A: input | drive | amp   (2 : 3 : 6 knobs)
    {
        const int unit = rowA.getWidth() - 2 * gap;
        sInput.area = rowA.removeFromLeft (unit * 2 / 11);  rowA.removeFromLeft (gap);
        sDrive.area = rowA.removeFromLeft (unit * 3 / 11);  rowA.removeFromLeft (gap);
        sAmp.area   = rowA;

        driveOn.setBounds (sDrive.area.getRight() - 62, sDrive.area.getY() + 6, 52, 20);
    }

    // Row B: cab | delay | reverb | output
    {
        const int unit = rowB.getWidth() - 3 * gap;
        sCab.area    = rowB.removeFromLeft (unit * 3 / 10);  rowB.removeFromLeft (gap);
        sDelay.area  = rowB.removeFromLeft (unit * 3 / 10);  rowB.removeFromLeft (gap);
        sReverb.area = rowB.removeFromLeft (unit * 2 / 10);  rowB.removeFromLeft (gap);
        sOut.area    = rowB;

        cabOn.setBounds (sCab.area.getRight() - 62, sCab.area.getY() + 6, 52, 20);
    }

    for (auto* s : { &sInput, &sDrive, &sAmp, &sDelay, &sReverb })
        layoutKnobs (*s, s->area.reduced (10, 0).withTrimmedTop (28).withTrimmedBottom (8));

    // Cab controls
    {
        auto c = sCab.area.reduced (18, 0).withTrimmedTop (44).withTrimmedBottom (20);
        cabType.setBounds (c.removeFromTop (30));
        c.removeFromTop (14);
        auto row = c.removeFromTop (30);
        irClear.setBounds (row.removeFromRight (30));
        row.removeFromRight (6);
        irButton.setBounds (row);
    }

    // Output: one knob + two thin meters
    {
        auto o = sOut.area.reduced (10, 0).withTrimmedTop (28).withTrimmedBottom (8);
        auto m = o.removeFromRight (22).withTrimmedTop (6).withTrimmedBottom (14);
        meterOut = m.removeFromRight (5);
        m.removeFromRight (5);
        meterIn = m.removeFromRight (5);
        layoutKnobs (sOut, o);
    }
}
