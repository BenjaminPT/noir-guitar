#include "MidiPanel.h"
#include "PluginEditor.h"

using L = NoirLookAndFeel;

class MidiPanel::DeviceModel : public juce::ListBoxModel
{
public:
    explicit DeviceModel (MidiPanel& o) : owner (o) {}
    int getNumRows() override { return owner.deviceNames.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= owner.deviceNames.size()) return;
        if (sel) { g.setColour (L::line); g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f); }
        const auto col = juce::Colour (0xff7fd48a);
        const float glow = 0.35f + 0.65f * owner.activity;
        g.setColour (col.withAlpha (0.25f * glow));
        g.fillEllipse (8.0f, (float) h * 0.5f - 7.0f, 14.0f, 14.0f);
        g.setColour (col.withAlpha (0.55f + 0.45f * owner.activity));
        g.fillEllipse (11.0f, (float) h * 0.5f - 4.0f, 8.0f, 8.0f);
        g.setColour (L::text);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (owner.deviceNames[row], 32, 0, w - 40, h, juce::Justification::centredLeft, true);
    }
private:
    MidiPanel& owner;
};

class MidiPanel::BindingModel : public juce::ListBoxModel
{
public:
    explicit BindingModel (MidiPanel& o) : owner (o) {}
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        if (sel) { g.setColour (L::line); g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f); }
        g.setColour (L::text);
        g.setFont (juce::FontOptions (12.5f));
        g.drawText (rows[(size_t) row].second, 10, 0, w - 16, h, juce::Justification::centredLeft, true);
    }
    std::vector<std::pair<int, juce::String>> rows;   // slot index + text
private:
    MidiPanel& owner;
};

//==============================================================================
MidiPanel::MidiPanel (NoirProcessor& p) : proc (p)
{
    deviceModel = std::make_unique<DeviceModel> (*this);
    bindingModel = std::make_unique<BindingModel> (*this);

    deviceList.setModel (deviceModel.get());
    deviceList.setRowHeight (30);
    deviceList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0f0f12));
    deviceList.setColour (juce::ListBox::outlineColourId, L::line);
    deviceList.setOutlineThickness (1);
    addAndMakeVisible (deviceList);

    deviceNote.setFont (juce::FontOptions (11.0f));
    deviceNote.setColour (juce::Label::textColourId, L::dim);
    deviceNote.setJustificationType (juce::Justification::topLeft);
    deviceNote.setText ("Every MIDI controller connected to this computer is listed here. In the standalone app, turn devices on or off "
                        "under Options > Audio/MIDI Settings. Inside a DAW, MIDI arrives through the host: route the controller's "
                        "track MIDI to this plugin (Reaper: receive MIDI; Ableton Live only forwards MIDI to instruments).",
                        juce::dontSendNotification);
    addAndMakeVisible (deviceNote);

    for (auto* l : { &actionLabel, &targetLabel, &modeLabel, &typeLabel, &numberLabel })
    {
        l->setColour (juce::Label::textColourId, L::dim);
        l->setFont (juce::FontOptions (11.0f));
        addAndMakeVisible (l);
    }
    actionLabel.setText ("WHAT TO CONTROL", juce::dontSendNotification);
    targetLabel.setText ("EFFECT / KNOB", juce::dontSendNotification);
    modeLabel.setText ("BUTTON BEHAVIOUR", juce::dontSendNotification);
    typeLabel.setText ("MESSAGE", juce::dontSendNotification);
    numberLabel.setText ("NUMBER", juce::dontSendNotification);

    for (int a = 0; a < NoirProcessor::NumActions; ++a)
        actionBox.addItem (NoirProcessor::actionName (a), a + 1);
    actionBox.setSelectedId (1, juce::dontSendNotification);
    actionBox.onChange = [this] { updateTargetEnabled(); };
    addAndMakeVisible (actionBox);

    // every parameter, grouped by pedal
    juce::String group;
    for (int i = 0; i < Macro1; ++i)
    {
        if (i >= (int) BpmManual && i < (int) LoopTrack) continue;
        const auto full = paramLabel (i);
        const auto g = full.upToFirstOccurrenceOf (" / ", false, false);
        if (g != group) { group = g; targetBox.addSectionHeading (group); }
        targetBox.addItem (full.fromFirstOccurrenceOf (" / ", false, false), i + 1);
    }
    targetBox.setSelectedId (DrvOn + 1, juce::dontSendNotification);
    addAndMakeVisible (targetBox);

    modeBox.addItem ("Auto (recommended)", 1);
    modeBox.addItem ("Toggle  (press = on / off)", 2);
    modeBox.addItem ("Momentary  (on while held)", 3);
    modeBox.addItem ("Absolute  (knob / fader)", 4);
    modeBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (modeBox);

    typeBox.addItem ("Control Change (CC)", 1);
    typeBox.addItem ("Note", 2);
    typeBox.addItem ("Program Change", 3);
    typeBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (typeBox);

    numberSlider.setSliderStyle (juce::Slider::IncDecButtons);
    numberSlider.setRange (0, 127, 1);
    numberSlider.setValue (20, juce::dontSendNotification);
    numberSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 24);
    addAndMakeVisible (numberSlider);

    learnBtn.getProperties().set ("accent", (int) juce::Colour (0xffe8a33d).getARGB());
    learnBtn.onClick = [this] { startLearn(); };
    addBtn.onClick = [this] { addManual(); };
    addAndMakeVisible (learnBtn);
    addAndMakeVisible (addBtn);

    help.setFont (juce::FontOptions (11.0f));
    help.setColour (juce::Label::textColourId, L::dim);
    help.setJustificationType (juce::Justification::topLeft);
    help.setText ("1) Pick what to control.  2) Press LEARN, then press the footswitch / pad or move the knob on your controller.  "
                  "Or choose the message type and number yourself and press ADD.  Tip: you can also right-click any knob or "
                  "button on the BOARD tab and choose MIDI Learn.",
                  juce::dontSendNotification);
    addAndMakeVisible (help);

    bindingList.setModel (bindingModel.get());
    bindingList.setRowHeight (28);
    bindingList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0f0f12));
    bindingList.setColour (juce::ListBox::outlineColourId, L::line);
    bindingList.setOutlineThickness (1);
    addAndMakeVisible (bindingList);

    removeBtn.onClick = [this]
    {
        const int r = bindingList.getSelectedRow();
        if (r >= 0 && r < (int) bindingModel->rows.size()) proc.removeBinding (bindingModel->rows[(size_t) r].first);
        refreshBindings();
    };
    clearBtn.onClick = [this] { proc.clearAllMidi(); refreshBindings(); };
    addAndMakeVisible (removeBtn);
    addAndMakeVisible (clearBtn);

    updateTargetEnabled();
    refreshDevices();
    refreshBindings();
    startTimerHz (20);
}

MidiPanel::~MidiPanel() = default;

void MidiPanel::visibilityChanged()
{
    if (isVisible()) { refreshDevices(); refreshBindings(); }
}

void MidiPanel::updateTargetEnabled()
{
    const bool isParam = actionBox.getSelectedId() == 1;
    targetBox.setEnabled (isParam);
    modeBox.setEnabled (isParam);
    targetBox.setAlpha (isParam ? 1.0f : 0.4f);
    modeBox.setAlpha (isParam ? 1.0f : 0.4f);
}

void MidiPanel::startLearn()
{
    if (proc.isLearning()) { proc.cancelMidiLearn(); return; }
    const int action = actionBox.getSelectedId() - 1;
    const int target = action == NoirProcessor::ActParam ? targetBox.getSelectedId() - 1 : -1;
    proc.startLearn (action, target, modeBox.getSelectedId() - 1);
}

void MidiPanel::addManual()
{
    NoirProcessor::MidiBinding b;
    b.type = typeBox.getSelectedId() - 1;
    b.number = (int) numberSlider.getValue();
    b.action = actionBox.getSelectedId() - 1;
    b.target = b.action == NoirProcessor::ActParam ? targetBox.getSelectedId() - 1 : -1;
    b.mode = modeBox.getSelectedId() - 1;
    proc.addBinding (b);
    refreshBindings();
}

void MidiPanel::refreshDevices()
{
    deviceNames.clear();
    for (const auto& d : juce::MidiInput::getAvailableDevices())
        deviceNames.add (d.name);
    if (deviceNames.isEmpty())
        deviceNames.add ("No MIDI device found  -  plug in your controller");
    deviceList.updateContent();
    deviceList.repaint();
}

void MidiPanel::refreshBindings()
{
    bindingModel->rows.clear();
    for (int i = 0; i < NoirProcessor::MaxBindings; ++i)
        if (NoirProcessor::MidiBinding b; proc.getBinding (i, b))
            bindingModel->rows.push_back ({ i, proc.describeBinding (b) });
    bindingList.updateContent();
    bindingList.repaint();
    lastVersion = proc.mapVersion.load();
}

void MidiPanel::timerCallback()
{
    if (! isVisible()) return;

    activity *= 0.88f;
    const int count = proc.midiCount.load();
    if (count != lastCount)
    {
        static const char* typeName[3] = { "CC", "Note", "PC" };
        const int from = juce::jmax (lastCount, count - 16);
        for (int c = from; c < count; ++c)
        {
            const uint32_t m = proc.midiLog[c & 15].load();
            logLines.insert (0, juce::String (typeName[juce::jlimit (0, 2, (int) (m >> 24))]) + " " + juce::String ((int) ((m >> 8) & 255))
                                    + "   value " + juce::String ((int) (m & 255)) + "   ch " + juce::String ((int) ((m >> 16) & 255)));
        }
        while (logLines.size() > 10) logLines.remove (logLines.size() - 1);
        lastCount = count;
        activity = 1.0f;
        repaint (monCard);
    }

    if (proc.mapVersion.load() != lastVersion) refreshBindings();

    const bool learning = proc.isLearning();
    learnBtn.setButtonText (learning ? "WAITING FOR MIDI...  (click to cancel)" : "LEARN");
    learnBtn.setToggleState (learning, juce::dontSendNotification);

    static int tick = 0;
    if (++tick % 20 == 0) refreshDevices();
    deviceList.repaint();
}

void MidiPanel::card (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (L::panel);
    g.fillRoundedRectangle (r.toFloat(), 9.0f);
    g.setColour (L::line);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 9.0f, 1.0f);
    g.setColour (L::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title, r.getX() + 16, r.getY() + 10, r.getWidth() - 32, 18, juce::Justification::centredLeft);
}

void MidiPanel::paint (juce::Graphics& g)
{
    card (g, devCard, "CONNECTED DEVICES");
    card (g, monCard, "LIVE MONITOR");
    card (g, asgCard, "ASSIGN A BUTTON, PEDAL OR KNOB");
    card (g, mapCard, "ASSIGNMENTS");

    auto m = monCard.reduced (16, 0).withTrimmedTop (36);
    g.setFont (juce::FontOptions (12.5f, juce::Font::plain));
    if (logLines.isEmpty())
    {
        g.setColour (L::dim);
        g.drawText ("Move a knob or press a button on your controller...", m.removeFromTop (20), juce::Justification::centredLeft);
    }
    for (int i = 0; i < logLines.size() && m.getHeight() >= 18; ++i)
    {
        g.setColour (i == 0 ? L::accent : L::dim.withAlpha (1.0f - (float) i * 0.07f));
        g.drawText (logLines[i], m.removeFromTop (18), juce::Justification::centredLeft);
    }
}

void MidiPanel::resized()
{
    auto area = getLocalBounds();
    const int gap = 12;
    auto left = area.removeFromLeft (juce::jmin (440, area.getWidth() * 2 / 5));
    area.removeFromLeft (gap);
    auto right = area;

    devCard = left.removeFromTop (juce::jmax (240, left.getHeight() * 55 / 100));
    left.removeFromTop (gap);
    monCard = left;

    asgCard = right.removeFromTop (268);
    right.removeFromTop (gap);
    mapCard = right;

    {
        auto d = devCard.reduced (14, 0).withTrimmedTop (36).withTrimmedBottom (12);
        deviceList.setBounds (d.removeFromTop (juce::jmax (60, d.getHeight() - 96)));
        d.removeFromTop (6);
        deviceNote.setBounds (d);
    }
    {
        auto a = asgCard.reduced (16, 0).withTrimmedTop (38);
        const int colW = (a.getWidth() - 12) / 2;
        auto row1 = a.removeFromTop (46);
        auto c1 = row1.removeFromLeft (colW); row1.removeFromLeft (12);
        actionLabel.setBounds (c1.removeFromTop (16)); actionBox.setBounds (c1.removeFromTop (26));
        targetLabel.setBounds (row1.removeFromTop (16)); targetBox.setBounds (row1.removeFromTop (26));
        a.removeFromTop (6);
        auto row2 = a.removeFromTop (46);
        auto c2 = row2.removeFromLeft (colW); row2.removeFromLeft (12);
        modeLabel.setBounds (c2.removeFromTop (16)); modeBox.setBounds (c2.removeFromTop (26));
        learnBtn.setBounds (row2.withTrimmedTop (16).removeFromTop (28));
        a.removeFromTop (8);
        auto row3 = a.removeFromTop (46);
        auto c3 = row3.removeFromLeft (colW); row3.removeFromLeft (12);
        typeLabel.setBounds (c3.removeFromTop (16)); typeBox.setBounds (c3.removeFromTop (26));
        auto c4 = row3.removeFromLeft (colW - 90);
        numberLabel.setBounds (c4.removeFromTop (16)); numberSlider.setBounds (c4.removeFromTop (26));
        row3.removeFromLeft (8);
        addBtn.setBounds (row3.withTrimmedTop (16).removeFromTop (28));
        a.removeFromTop (6);
        help.setBounds (a);
    }
    {
        auto m = mapCard.reduced (14, 0).withTrimmedTop (36).withTrimmedBottom (12);
        auto bar = m.removeFromBottom (28);
        m.removeFromBottom (8);
        clearBtn.setBounds (bar.removeFromRight (100));
        bar.removeFromRight (8);
        removeBtn.setBounds (bar.removeFromRight (140));
        bindingList.setBounds (m);
    }
}
