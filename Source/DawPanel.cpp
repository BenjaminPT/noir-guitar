#include "DawPanel.h"
#include "PluginEditor.h"

namespace
{
const juce::Colour kAccent { 0xff5b9bd5 };

// One macro link: target name, min / max range, remove button
class LinkRow : public juce::Component
{
public:
    LinkRow()
    {
        for (auto* s : { &lo, &hi })
        {
            s->setSliderStyle (juce::Slider::LinearHorizontal);
            s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s->setRange (0.0, 1.0, 0.001);
            s->setColour (juce::Slider::thumbColourId, kAccent);
            s->setColour (juce::Slider::trackColourId, kAccent.withAlpha (0.55f));
            s->setColour (juce::Slider::backgroundColourId, NoirLookAndFeel::line);
            addAndMakeVisible (*s);
        }
        remove.setButtonText ("x");
        addAndMakeVisible (remove);
        lo.onValueChange = [this] { if (onRange) onRange ((float) lo.getValue(), (float) hi.getValue()); };
        hi.onValueChange = [this] { if (onRange) onRange ((float) lo.getValue(), (float) hi.getValue()); };
        remove.onClick   = [this] { if (onRemove) onRemove(); };
    }

    void set (const juce::String& n, float l, float h)
    {
        name = n;
        lo.setValue (l, juce::dontSendNotification);
        hi.setValue (h, juce::dontSendNotification);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (NoirLookAndFeel::text);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (name, 6, 2, getWidth() - 40, 18, juce::Justification::centredLeft);
        g.setColour (NoirLookAndFeel::dim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("min", 6, 24, 22, 14, juce::Justification::centredLeft);
        g.drawText ("max", getWidth() / 2 + 2, 24, 22, 14, juce::Justification::centredLeft);
        g.setColour (NoirLookAndFeel::line);
        g.drawHorizontalLine (getHeight() - 1, 4.0f, (float) getWidth() - 4.0f);
    }

    void resized() override
    {
        auto a = getLocalBounds();
        remove.setBounds (a.removeFromTop (22).removeFromRight (26).reduced (0, 1));
        a.removeFromTop (2);
        auto left = a.removeFromLeft (a.getWidth() / 2).withTrimmedLeft (24).withTrimmedRight (6);
        lo.setBounds (left.removeFromTop (18));
        hi.setBounds (a.withTrimmedLeft (24).withTrimmedRight (8).removeFromTop (18));
    }

    std::function<void (float, float)> onRange;
    std::function<void()> onRemove;

private:
    juce::String name;
    juce::Slider lo, hi;
    juce::TextButton remove;
};
}

//==============================================================================
class DawPanel::MidiModel : public juce::ListBoxModel
{
public:
    explicit MidiModel (NoirProcessor& p) : proc (p) {}

    void rebuild()
    {
        rows.clear();
        const auto map = proc.midiMapSnapshot();
        for (int cc = 0; cc < 128; ++cc)
            if (map[(size_t) cc] >= 0) rows.push_back ({ cc, map[(size_t) cc] });
    }

    int getNumRows() override { return (int) rows.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        if (selected) { g.setColour (NoirLookAndFeel::line); g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f); }
        g.setColour (kAccent);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("CC " + juce::String (rows[(size_t) row].first), 8, 0, 58, h, juce::Justification::centredLeft);
        g.setColour (NoirLookAndFeel::text);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (paramLabel (rows[(size_t) row].second), 70, 0, w - 74, h, juce::Justification::centredLeft);
    }

    int selectedParam (int row) const { return row >= 0 && row < (int) rows.size() ? rows[(size_t) row].second : -1; }

private:
    NoirProcessor& proc;
    std::vector<std::pair<int, int>> rows;
};

class DawPanel::LinkModel : public juce::ListBoxModel
{
public:
    explicit LinkModel (NoirProcessor& p) : proc (p) {}

    void rebuild (int macro)
    {
        current = macro;
        slots.clear();
        for (int i = 0; i < NoirProcessor::LinksPerMacro; ++i)
            if (proc.getLink (macro, i).param >= 0) slots.push_back (i);
    }

    int getNumRows() override { return (int) slots.size(); }
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override {}

    juce::Component* refreshComponentForRow (int row, bool, juce::Component* existing) override
    {
        if (row < 0 || row >= (int) slots.size())
        {
            delete existing;
            return nullptr;
        }

        auto* r = dynamic_cast<LinkRow*> (existing);
        if (r == nullptr) { delete existing; r = new LinkRow(); }

        const int macro = current, idx = slots[(size_t) row];
        const auto l = proc.getLink (macro, idx);
        r->set (paramLabel (l.param), l.lo, l.hi);
        r->onRange = [this, macro, idx] (float lo, float hi) { proc.setLinkRange (macro, idx, lo, hi); };
        r->onRemove = [this, macro, idx] { proc.unassignMacro (macro, idx); };
        return r;
    }

private:
    NoirProcessor& proc;
    int current = 0;
    std::vector<int> slots;
};

//==============================================================================
DawPanel::DawPanel (NoirProcessor& p) : proc (p)
{
    midiModel = std::make_unique<MidiModel> (proc);
    linkModel = std::make_unique<LinkModel> (proc);

    auto styleLabel = [] (juce::Label& l, const juce::String& t, juce::Justification j = juce::Justification::centredLeft)
    {
        l.setText (t, juce::dontSendNotification);
        l.setJustificationType (j);
        l.setColour (juce::Label::textColourId, NoirLookAndFeel::dim);
    };

    // Tempo
    hostSync.getProperties().set ("label", true);
    addAndMakeVisible (hostSync);
    hostSyncAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[HostSync].id, hostSync);
    bpmSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    bpmSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 15);
    bpmSlider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    bpmSlider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    addAndMakeVisible (bpmSlider);
    bpmAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kParams[BpmManual].id, bpmSlider);
    styleLabel (bpmLabel, "Manual BPM", juce::Justification::centred);
    addAndMakeVisible (bpmLabel);
    styleLabel (hostInfo, "");
    hostInfo.setColour (juce::Label::textColourId, NoirLookAndFeel::text);
    addAndMakeVisible (hostInfo);

    // Tempo sync rows
    const char* names[4] = { "Delay time", "Mod 1 rate", "Mod 2 rate", "Glitch length" };
    const Id ids[4] = { DlySync, M1Sync, M2Sync, GltSync };
    for (int i = 0; i < 4; ++i)
    {
        styleLabel (syncRows[i].label, names[i]);
        addAndMakeVisible (syncRows[i].label);
        syncRows[i].box.addItemList (juce::StringArray::fromTokens (kParams[ids[i]].choices, "|", ""), 1);
        addAndMakeVisible (syncRows[i].box);
        syncRows[i].att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, kParams[ids[i]].id, syncRows[i].box);
    }

    // Looper
    for (auto* b : { &loopFollow, &loopQuant })
    {
        b->getProperties().set ("label", true);
        addAndMakeVisible (*b);
    }
    followAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[LoopFollow].id, loopFollow);
    quantAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[LoopQuant].id, loopQuant);

    // MIDI
    styleLabel (midiHelp, "Right-click any knob or switch on the BOARD tab and choose MIDI Learn, then move a CC on your controller "
                          "or press a footswitch. Program Change 0, 1, 2 ... loads the chains from the preset list.");
    midiHelp.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (midiHelp);
    midiList.setModel (midiModel.get());
    midiList.setRowHeight (24);
    midiList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    midiList.setColour (juce::ListBox::outlineColourId, NoirLookAndFeel::line);
    midiList.setOutlineThickness (1);
    addAndMakeVisible (midiList);
    removeMidi.onClick = [this]
    {
        const int sel = midiList.getSelectedRow();
        const int pid = midiModel->selectedParam (sel);
        if (pid >= 0) proc.clearMidiFor ((Id) pid);
    };
    clearMidi.onClick = [this] { proc.clearAllMidi(); };
    addAndMakeVisible (removeMidi);
    addAndMakeVisible (clearMidi);

    // Macros
    for (int i = 0; i < NoirProcessor::NumMacros; ++i)
    {
        auto& k = macros[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
        k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        k.slider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
        addAndMakeVisible (k.slider);
        styleLabel (k.label, "Macro " + juce::String (i + 1), juce::Justification::centred);
        addAndMakeVisible (k.label);
        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kParams[Macro1 + i].id, k.slider);
    }

    for (int i = 0; i < NoirProcessor::NumMacros; ++i) macroSel.addItem ("Macro " + juce::String (i + 1), i + 1);
    macroSel.setSelectedId (1, juce::dontSendNotification);
    macroSel.onChange = [this] { refreshLists(); };
    addAndMakeVisible (macroSel);
    styleLabel (macroHelp, "Right-click any control on the BOARD tab and choose Assign to Macro. Automate the macro in your DAW "
                           "to move many parameters at once; set the range of each link below.");
    addAndMakeVisible (macroHelp);
    linkList.setModel (linkModel.get());
    linkList.setRowHeight (46);
    linkList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    linkList.setColour (juce::ListBox::outlineColourId, NoirLookAndFeel::line);
    linkList.setOutlineThickness (1);
    addAndMakeVisible (linkList);

    refreshLists();
    startTimerHz (10);
}

DawPanel::~DawPanel()
{
    midiList.setModel (nullptr);
    linkList.setModel (nullptr);
}

void DawPanel::visibilityChanged() { if (isVisible()) refreshLists(); }

void DawPanel::refreshLists()
{
    midiModel->rebuild();
    midiList.updateContent();
    midiList.repaint();

    linkModel->rebuild (juce::jmax (0, macroSel.getSelectedId() - 1));
    linkList.updateContent();
    linkList.repaint();

    lastVersion = proc.mapVersion.load();
    lastMacroSel = macroSel.getSelectedId();
}

void DawPanel::timerCallback()
{
    if (! isVisible()) return;

    if (proc.mapVersion.load() != lastVersion)
        refreshLists();

    juce::String s;
    if (proc.hostKnown.load())
        s = juce::String (proc.hostBpmShown.load(), 1) + " BPM   |   transport " + (proc.hostPlayingShown.load() ? "playing" : "stopped");
    else
        s = "No host transport (standalone): using " + juce::String (proc.hostBpmShown.load(), 1) + " BPM";
    if (hostInfo.getText() != s) hostInfo.setText (s, juce::dontSendNotification);
}

void DawPanel::card (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (NoirLookAndFeel::panel);
    g.fillRoundedRectangle (r.toFloat(), 9.0f);
    g.setColour (NoirLookAndFeel::line);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 9.0f, 1.0f);
    g.setColour (kAccent);
    g.fillRoundedRectangle ((float) r.getX() + 14.0f, (float) r.getY() + 1.0f, 26.0f, 2.5f, 1.2f);
    g.setColour (NoirLookAndFeel::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title, r.getX() + 14, r.getY() + 9, r.getWidth() - 28, 18, juce::Justification::centredLeft);
}

void DawPanel::paint (juce::Graphics& g)
{
    card (g, tempoCard, "TEMPO");
    card (g, syncCard, "TEMPO SYNC");
    card (g, loopCard, "LOOPER  /  TRANSPORT");
    card (g, midiCard, "MIDI CONTROL");
    card (g, macroCard, "MACROS");
}

void DawPanel::resized()
{
    auto area = getLocalBounds();
    const int gap = 10;
    auto c1 = area.removeFromLeft ((int) ((float) area.getWidth() * 0.32f));
    area.removeFromLeft (gap);
    auto c2 = area.removeFromLeft ((int) ((float) area.getWidth() * 0.44f));
    area.removeFromLeft (gap);
    auto c3 = area;

    tempoCard = c1.removeFromTop (190);
    c1.removeFromTop (gap);
    loopCard = c1.removeFromBottom (110);
    c1.removeFromBottom (gap);
    syncCard = c1;
    midiCard = c2;
    macroCard = c3;

    // Tempo
    {
        auto a = tempoCard.reduced (14, 0).withTrimmedTop (34).withTrimmedBottom (10);
        hostSync.setBounds (a.removeFromTop (26));
        a.removeFromTop (6);
        hostInfo.setBounds (a.removeFromBottom (20));
        auto k = a.withSizeKeepingCentre (110, a.getHeight());
        bpmLabel.setBounds (k.removeFromBottom (14));
        bpmSlider.setBounds (k);
    }

    // Sync
    {
        auto a = syncCard.reduced (14, 0).withTrimmedTop (38).withTrimmedBottom (10);
        for (auto& row : syncRows)
        {
            auto r = a.removeFromTop (34);
            row.label.setBounds (r.removeFromLeft (110));
            row.box.setBounds (r.reduced (0, 4));
        }
    }

    // Looper
    {
        auto a = loopCard.reduced (14, 0).withTrimmedTop (38).withTrimmedBottom (10);
        loopFollow.setBounds (a.removeFromTop (26));
        a.removeFromTop (8);
        loopQuant.setBounds (a.removeFromTop (26));
    }

    // MIDI
    {
        auto a = midiCard.reduced (14, 0).withTrimmedTop (34).withTrimmedBottom (12);
        midiHelp.setBounds (a.removeFromTop (74));
        auto buttons = a.removeFromBottom (28);
        a.removeFromBottom (8);
        removeMidi.setBounds (buttons.removeFromLeft (130));
        buttons.removeFromLeft (8);
        clearMidi.setBounds (buttons.removeFromLeft (90));
        midiList.setBounds (a);
    }

    // Macros
    {
        auto a = macroCard.reduced (14, 0).withTrimmedTop (34).withTrimmedBottom (12);
        const int knobH = 92;
        for (int row = 0; row < 2; ++row)
        {
            auto r = a.removeFromTop (knobH);
            const int w = r.getWidth() / 4;
            for (int col = 0; col < 4; ++col)
            {
                auto& k = macros[row * 4 + col];
                auto c = r.removeFromLeft (w);
                k.label.setBounds (c.removeFromBottom (14));
                k.slider.setBounds (c);
            }
        }
        a.removeFromTop (6);
        macroSel.setBounds (a.removeFromTop (26).removeFromLeft (150));
        a.removeFromTop (4);
        macroHelp.setBounds (a.removeFromTop (46));
        linkList.setBounds (a);
    }
}
