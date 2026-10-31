#include "PluginEditor.h"
#include "Presets.h"

//==============================================================================
NoirLookAndFeel::NoirLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, accent);
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
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto fill = s.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (line);
    g.strokePath (track, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? startAngle + 0.5f * (endAngle - startAngle) : startAngle;

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (fill);
    g.strokePath (value, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path tick;
    tick.startNewSubPath (centre.x + std::sin (angle) * radius * 0.30f, centre.y - std::cos (angle) * radius * 0.30f);
    tick.lineTo (centre.x + std::sin (angle) * radius * 0.74f, centre.y - std::cos (angle) * radius * 0.74f);
    g.setColour (text);
    g.strokePath (tick, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void NoirLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();

    if (b.getProperties().contains ("power"))
    {
        const auto c = r.reduced (1.0f);
        g.setColour (on ? accent.withAlpha (0.16f) : panel);
        g.fillRoundedRectangle (c, c.getHeight() * 0.5f);
        g.setColour (on ? accent : line);
        g.drawRoundedRectangle (c, c.getHeight() * 0.5f, 1.2f);
        g.setColour (on ? accent : (hl ? text : dim));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (on ? "POWER ON" : "BYPASSED", c.toNearestInt(), juce::Justification::centred);
        return;
    }

    // Pedal LED
    const auto col = juce::Colour ((juce::uint32) (int) b.getProperties().getWithDefault ("accent", (int) accent.getARGB()));
    const auto dot = juce::Rectangle<float> (10.0f, 10.0f).withCentre (r.getCentre());
    if (on)
    {
        g.setColour (col.withAlpha (0.22f));
        g.fillEllipse (dot.expanded (5.0f));
        g.setColour (col.withAlpha (0.35f));
        g.fillEllipse (dot.expanded (2.5f));
    }
    g.setColour (on ? col : line.brighter (0.15f));
    g.fillEllipse (dot);
    g.setColour (on ? col.brighter (0.4f) : dim.withAlpha (hl ? 0.9f : 0.5f));
    g.drawEllipse (dot, 1.0f);
}

void NoirLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? line : (hl ? line.brighter (0.1f) : juce::Colours::transparentBlack));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void NoirLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (juce::Colour (0xff0f0f12));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 3.5f, cy - 1.5f, cx + 3.5f, cy - 1.5f, cx, cy + 2.5f);
    g.setColour (dim);
    g.fillPath (arrow);
}

//==============================================================================
Pedal::Pedal (juce::AudioProcessorValueTreeState& s, const juce::String& t, juce::Colour acc,
              int onParam, int typeParam, std::initializer_list<Id> knobIds, const juce::String& sub)
    : state (s), title (t), subtitle (sub), accent (acc), onId (onParam), typeId (typeParam)
{
    if (onId >= 0)
    {
        led.getProperties().set ("accent", (int) accent.getARGB());
        led.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (led);
        ledAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, kParams[onId].id, led);
    }

    if (typeId >= 0)
    {
        type.addItemList (juce::StringArray::fromTokens (kParams[typeId].choices, "|", ""), 1);
        addAndMakeVisible (type);
        typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, kParams[typeId].id, type);
    }

    for (auto id : knobIds)
    {
        const auto& info = kParams[id];
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 15);
        k->slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        k->slider.setMouseDragSensitivity (180);
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        k->slider.setDoubleClickReturnValue (true, info.def);
        if (info.lo < 0.0f && info.hi > 0.0f && std::abs (info.lo + info.hi) < 0.001f)
            k->slider.getProperties().set ("bipolar", true);
        k->label.setText (info.name, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, info.id, k->slider);
        knobs.push_back (std::move (k));
    }
}

int Pedal::desiredWidth() const
{
    int w = (int) knobs.size() * 72 + 24;
    if (typeId >= 0) w = juce::jmax (w, 164);
    return juce::jmax (w, 110);
}

void Pedal::refresh()
{
    bool nowOn = true;
    if (onId >= 0)
        if (auto* v = state.getRawParameterValue (kParams[onId].id))
            nowOn = v->load() > 0.5f;

    if (nowOn == isOn) return;
    isOn = nowOn;

    const float a = isOn ? 1.0f : 0.40f;
    type.setAlpha (a);
    for (auto& k : knobs) { k->slider.setAlpha (a); k->label.setAlpha (a); }
    repaint();
}

void Pedal::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (NoirLookAndFeel::panel);
    g.fillRoundedRectangle (r, 9.0f);

    if (isOn)
    {
        juce::ColourGradient glow (accent.withAlpha (0.13f), r.getX(), r.getY(),
                                   accent.withAlpha (0.0f), r.getX(), r.getY() + 70.0f, false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r, 9.0f);
    }

    g.setColour (isOn ? accent.withAlpha (0.45f) : NoirLookAndFeel::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);

    // accent bar
    g.setColour (isOn ? accent : NoirLookAndFeel::line.brighter (0.1f));
    g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 1.0f, 26.0f, 2.5f, 1.2f);

    const int textX = onId >= 0 ? 36 : 14;
    g.setColour (isOn ? NoirLookAndFeel::text : NoirLookAndFeel::dim);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), textX, 9, getWidth() - textX - 10, 18, juce::Justification::centredLeft);

    if (subtitle.isNotEmpty())
    {
        g.setColour (NoirLookAndFeel::dim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (subtitle, textX, 24, getWidth() - textX - 10, 12, juce::Justification::centredLeft);
    }
}

void Pedal::resized()
{
    auto a = getLocalBounds();
    led.setBounds (10, 9, 20, 18);
    a.removeFromTop (38);

    if (typeId >= 0)
    {
        type.setBounds (a.removeFromTop (24).reduced (12, 0));
        a.removeFromTop (6);
    }

    auto body = a.reduced (6, 2).withTrimmedBottom (4);

    if (onLayoutBody)
        onLayoutBody (body);

    if (knobs.empty()) return;

    const int w = body.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto c = body.removeFromLeft (w);
        k->label.setBounds (c.removeFromBottom (15));
        k->slider.setBounds (c);
    }
}

//==============================================================================
ChainStrip::ChainStrip (NoirProcessor& p, std::vector<Item> it) : proc (p), items (std::move (it)) {}

juce::Rectangle<float> ChainStrip::boxFor (int i) const
{
    const float gap = 14.0f;
    const float n = (float) items.size();
    const float w = ((float) getWidth() - gap * (n - 1.0f)) / n;
    return { (w + gap) * (float) i, 4.0f, w, (float) getHeight() - 8.0f };
}

int ChainStrip::indexAt (juce::Point<int> pt) const
{
    for (int i = 0; i < (int) items.size(); ++i)
        if (boxFor (i).contains (pt.toFloat()) && items[(size_t) i].onId >= 0)
            return i;
    return -1;
}

void ChainStrip::paint (juce::Graphics& g)
{
    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto& it = items[(size_t) i];
        const auto b = boxFor (i);
        const bool endpoint = it.onId < 0;
        const bool on = endpoint || proc.apvts.getRawParameterValue (kParams[it.onId].id)->load() > 0.5f;

        if (i + 1 < (int) items.size())
        {
            const auto nb = boxFor (i + 1);
            const float y = b.getCentreY();
            const bool lit = on && (items[(size_t) i + 1].onId < 0 ||
                             proc.apvts.getRawParameterValue (kParams[items[(size_t) i + 1].onId].id)->load() > 0.5f);
            g.setColour (lit ? NoirLookAndFeel::dim : NoirLookAndFeel::line);
            g.drawLine (b.getRight() + 1.0f, y, nb.getX() - 1.0f, y, 1.2f);
        }

        const auto col = endpoint ? NoirLookAndFeel::dim : it.colour;
        g.setColour (on && ! endpoint ? col.withAlpha (hover == i ? 0.30f : 0.20f) : NoirLookAndFeel::panel);
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (on ? col.withAlpha (endpoint ? 0.5f : 0.9f) : NoirLookAndFeel::line);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

        g.setColour (on ? NoirLookAndFeel::text : NoirLookAndFeel::dim.withAlpha (0.7f));
        g.setFont (juce::FontOptions (10.5f, endpoint ? juce::Font::plain : juce::Font::bold));
        g.drawText (it.name.toUpperCase(), b.toNearestInt(), juce::Justification::centred);
    }
}

void ChainStrip::mouseMove (const juce::MouseEvent& e)
{
    const int h = indexAt (e.getPosition());
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void ChainStrip::mouseDown (const juce::MouseEvent& e)
{
    const int i = indexAt (e.getPosition());
    if (i < 0) return;
    if (auto* p = proc.param ((Id) items[(size_t) i].onId))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
    repaint();
}

//==============================================================================
NoirEditor::NoirEditor (NoirProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    const auto cDyn   = juce::Colour (0xff4fb3a9);
    const auto cFilt  = juce::Colour (0xffc57be0);
    const auto cPitch = juce::Colour (0xfff0709a);
    const auto cGain  = juce::Colour (0xffe8a33d);
    const auto cAmp   = juce::Colour (0xffe0674a);
    const auto cEq    = juce::Colour (0xff7fc47f);
    const auto cMod   = juce::Colour (0xff5b9bd5);
    const auto cLofi  = juce::Colour (0xffb8b86a);
    const auto cTime  = juce::Colour (0xff9b86e8);
    const auto cOut   = juce::Colour (0xffd8d6d0);

    auto make = [this] (const char* title, juce::Colour c, int on, int type, std::initializer_list<Id> k, const char* sub = "") -> Pedal*
    {
        auto ped = std::make_unique<Pedal> (proc.apvts, title, c, on, type, k, sub);
        auto* raw = ped.get();
        addAndMakeVisible (raw);
        pedals.push_back (std::move (ped));
        return raw;
    };

    pGate   = make ("Noise Gate", cDyn,   GateOn,  -1,        { GateThr },                         "Dynamics");
    pComp   = make ("Compressor", cDyn,   CompOn,  -1,        { CompAmt, CompLvl, CompAtk },       "Dynamics / Sustain");
    pWah    = make ("Wah",        cFilt,  WahOn,   WahMode,   { WahPos, WahQ },                    "Filter");
    pPitch  = make ("Pitch",      cPitch, PitchOn, PitchType, { PitchShift, PitchMix },            "Pitch");
    pBoost  = make ("Boost",      cGain,  BoostOn, BoostType, { BoostGain },                       "Gain / EQ boost");
    pDrive  = make ("Drive",      cGain,  DrvOn,   DrvType,   { DrvDrive, DrvTone, DrvLevel },     "Gain");
    pAmp    = make ("Amp",        cAmp,   AmpOn,   -1,        { AmpGain, AmpBass, AmpMid, AmpTreble, AmpPres }, "Preamp + power stage");
    pCab    = make ("Cabinet",    cAmp,   CabOn,   CabType,   {},                                  "Speaker / IR");
    pEq     = make ("EQ",         cEq,    EqOn,    -1,        { EqLow, EqLowMid, EqHiMid, EqHigh }, "4-band parametric");
    pMod1   = make ("Mod 1",      cMod,   M1On,    M1Type,    { M1Rate, M1Depth, M1Mix },          "Modulation");
    pMod2   = make ("Mod 2",      cMod,   M2On,    M2Type,    { M2Rate, M2Depth, M2Mix },          "Modulation");
    pLofi   = make ("Lo-Fi",      cLofi,  LoOn,    -1,        { LoBits, LoRate, LoMix },           "Bitcrusher / Sample rate");
    pDelay  = make ("Delay",      cTime,  DlyOn,   DlyType,   { DlyTime, DlyFb, DlyMix },          "Echo");
    pReverb = make ("Reverb",     cTime,  RevOn,   RevType,   { RevSize, RevMix },                 "Space");
    pOut    = make ("Output",     cOut,   -1,      -1,        { InGain, Master, OutGain },         "Levels");

    // Cabinet: IR loader lives inside the cab pedal.
    pCab->addAndMakeVisible (irButton);
    pCab->addChildComponent (irClear);
    irButton.onClick = [this] { chooseIR(); };
    irClear.onClick = [this] { proc.clearIR(); timerCallback(); };
    pCab->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        auto row = body.withTrimmedTop (14).removeFromTop (28).reduced (6, 0);
        irClear.setBounds (row.removeFromRight (28));
        row.removeFromRight (4);
        irButton.setBounds (row);
    };
    pCab->resized();

    strip = std::make_unique<ChainStrip> (proc, std::vector<ChainStrip::Item> {
        { "In", -1, cOut },
        { "Gate", GateOn, cDyn },   { "Comp", CompOn, cDyn },   { "Wah", WahOn, cFilt },
        { "Pitch", PitchOn, cPitch }, { "Boost", BoostOn, cGain }, { "Drive", DrvOn, cGain },
        { "Amp", AmpOn, cAmp },     { "Cab", CabOn, cAmp },     { "EQ", EqOn, cEq },
        { "Mod 1", M1On, cMod },    { "Mod 2", M2On, cMod },    { "Lo-Fi", LoOn, cLofi },
        { "Delay", DlyOn, cTime },  { "Reverb", RevOn, cTime }, { "Out", -1, cOut } });
    addAndMakeVisible (*strip);

    // Header
    for (const auto& pr : factoryPresets())
        presetBox.addItem (pr.name, presetBox.getNumItems() + 1);
    presetBox.setTextWhenNothingSelected ("Chains / Presets");
    presetBox.onChange = [this] { applyPreset (proc, presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    auto step = [this] (int d)
    {
        const int n = presetBox.getNumItems();
        if (n == 0) return;
        const int cur = presetBox.getSelectedItemIndex();
        presetBox.setSelectedItemIndex (((cur < 0 ? (d > 0 ? -1 : 0) : cur) + d + n) % n, juce::sendNotificationSync);
    };
    prevPreset.onClick = [step] { step (-1); };
    nextPreset.onClick = [step] { step (1); };
    allOn.onClick  = [this] { setAllEffects (true); };
    allOff.onClick = [this] { setAllEffects (false); };
    for (auto* b : { &prevPreset, &nextPreset, &allOn, &allOff })
        addAndMakeVisible (b);

    power.getProperties().set ("power", true);
    power.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (power);
    powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[Power].id, power);

    setSize (1260, 760);
    startTimerHz (30);
    timerCallback();
}

NoirEditor::~NoirEditor()
{
    setLookAndFeel (nullptr);
}

void NoirEditor::setAllEffects (bool on)
{
    static const Id switches[] = { GateOn, CompOn, WahOn, PitchOn, BoostOn, DrvOn, AmpOn, CabOn, EqOn,
                                   M1On, M2On, LoOn, DlyOn, RevOn };
    for (auto id : switches)
        if (auto* p = proc.param (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
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
    const float ip = proc.inPeak.exchange (0.0f);
    const float op = proc.outPeak.exchange (0.0f);
    inLevel  = juce::jmax (ip, inLevel * 0.84f);
    outLevel = juce::jmax (op, outLevel * 0.84f);

    const bool ir = proc.hasIR();
    irButton.setButtonText (ir ? proc.getIRName() : "Load IR");
    irClear.setVisible (ir);

    for (auto& p : pedals) p->refresh();
    strip->repaint();
    repaint (headerArea.getRight() - 230, 10, 230, 40);
}

//==============================================================================
void NoirEditor::paint (juce::Graphics& g)
{
    g.fillAll (NoirLookAndFeel::bg);

    juce::ColourGradient vignette (juce::Colour (0xff17171b), (float) getWidth() * 0.5f, 0.0f,
                                   NoirLookAndFeel::bg, (float) getWidth() * 0.5f, 220.0f, false);
    g.setGradientFill (vignette);
    g.fillRect (0, 0, getWidth(), 220);

    g.setColour (NoirLookAndFeel::text);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("NOIR", 24, 14, 80, 30, juce::Justification::centredLeft);
    g.setColour (NoirLookAndFeel::accent);
    g.fillRect (24, 46, 22, 2);
    g.setColour (NoirLookAndFeel::dim);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("GUITAR  /  PEDALBOARD", 24, 50, 160, 12, juce::Justification::centredLeft);

    auto drawMeter = [&] (juce::Rectangle<int> r, float level, const juce::String& name)
    {
        g.setColour (NoirLookAndFeel::dim);
        g.setFont (juce::FontOptions (9.5f));
        g.drawText (name, r.getX() - 24, r.getY() - 4, 22, r.getHeight() + 8, juce::Justification::centredRight);
        g.setColour (NoirLookAndFeel::line);
        g.fillRoundedRectangle (r.toFloat(), 2.0f);
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float f = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour (db > -1.0f ? juce::Colour (0xffd9534f) : NoirLookAndFeel::accent);
        g.fillRoundedRectangle (r.toFloat().withWidth ((float) r.getWidth() * f), 2.0f);
    };
    drawMeter (meterIn, inLevel, "IN");
    drawMeter (meterOut, outLevel, "OUT");
}

void NoirEditor::layoutRow (juce::Rectangle<int> row, std::vector<Pedal*> row_pedals)
{
    const int gap = 10;
    int total = 0;
    for (auto* p : row_pedals) total += p->desiredWidth();
    const float scale = (float) (row.getWidth() - gap * ((int) row_pedals.size() - 1)) / (float) total;

    int x = row.getX();
    for (size_t i = 0; i < row_pedals.size(); ++i)
    {
        const int w = (i + 1 == row_pedals.size()) ? row.getRight() - x
                                                    : (int) std::floor ((float) row_pedals[i]->desiredWidth() * scale);
        row_pedals[i]->setBounds (x, row.getY(), w, row.getHeight());
        x += w + gap;
    }
}

void NoirEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 14);

    // Header
    headerArea = area.removeFromTop (54);
    {
        auto h = headerArea;
        h.removeFromLeft (200);

        auto right = h.removeFromRight (440);
        power.setBounds (right.removeFromRight (110).withSizeKeepingCentre (110, 28));
        right.removeFromRight (10);
        allOff.setBounds (right.removeFromRight (74).withSizeKeepingCentre (74, 28));
        right.removeFromRight (6);
        allOn.setBounds (right.removeFromRight (64).withSizeKeepingCentre (64, 28));
        right.removeFromRight (14);

        auto meters = right.withSizeKeepingCentre (right.getWidth() - 20, 22);
        meterIn  = meters.removeFromTop (8).withTrimmedLeft (26);
        meters.removeFromTop (6);
        meterOut = meters.removeFromTop (8).withTrimmedLeft (26);

        auto mid = h.withSizeKeepingCentre (juce::jmin (h.getWidth(), 340), 28);
        prevPreset.setBounds (mid.removeFromLeft (30));
        mid.removeFromLeft (6);
        nextPreset.setBounds (mid.removeFromRight (30));
        mid.removeFromRight (6);
        presetBox.setBounds (mid);
    }

    area.removeFromTop (6);
    strip->setBounds (area.removeFromTop (34));
    area.removeFromTop (12);

    const int gap = 10;
    const int rowH = (area.getHeight() - 2 * gap) / 3;
    layoutRow (area.removeFromTop (rowH), { pGate, pComp, pWah, pPitch, pBoost, pDrive });
    area.removeFromTop (gap);
    layoutRow (area.removeFromTop (rowH), { pAmp, pCab, pEq, pMod1 });
    area.removeFromTop (gap);
    layoutRow (area, { pMod2, pLofi, pDelay, pReverb, pOut });
}
