#include "PluginEditor.h"
#include "Presets.h"

namespace
{
const juce::Colour cDyn   { 0xff4fb3a9 };
const juce::Colour cFilt  { 0xffc57be0 };
const juce::Colour cPitch { 0xfff0709a };
const juce::Colour cGain  { 0xffe8a33d };
const juce::Colour cAmp   { 0xffe0674a };
const juce::Colour cEq    { 0xff7fc47f };
const juce::Colour cMod   { 0xff5b9bd5 };
const juce::Colour cLofi  { 0xffb8b86a };
const juce::Colour cTime  { 0xff9b86e8 };
const juce::Colour cVoice { 0xffe39a6b };
const juce::Colour cGrain { 0xff56c2c2 };
const juce::Colour cGlit  { 0xffe5605f };
const juce::Colour cLoop  { 0xffd8c85a };
const juce::Colour cOut   { 0xffd8d6d0 };

const juce::Colour slotColour[NumSlots] = {
    cDyn, cDyn, cFilt, cPitch, cGain, cGain, cAmp, cAmp, cEq, cVoice, cVoice,
    cMod, cMod, cLofi, cGlit, cGrain, cGrain, cTime, cTime, cLoop };
}

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
    setColour (juce::ListBox::textColourId, text);
    setColour (juce::ScrollBar::thumbColourId, line.brighter (0.2f));
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

    if (b.getProperties().contains ("label"))   // small labelled switch
    {
        const auto col = juce::Colour (0xffd8c85a);
        g.setColour (on ? col.withAlpha (0.22f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (r.reduced (0.5f), 5.0f);
        g.setColour (on ? col : line);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (on ? text : dim);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
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
    const bool lit = b.getToggleState();
    const auto col = juce::Colour ((juce::uint32) (int) b.getProperties().getWithDefault ("accent", (int) accent.getARGB()));
    g.setColour (lit ? col.withAlpha (0.22f) : (down ? line : (hl ? line.brighter (0.1f) : juce::Colours::transparentBlack)));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (lit ? col : line);
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
void TunerView::setReading (float freqHz, float refA4)
{
    bool nowValid = freqHz > 20.0f;
    juce::String nn = "--";
    float c = 0.0f;

    if (nowValid)
    {
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const float midi = 69.0f + 12.0f * std::log2 (freqHz / refA4);
        const int n = (int) std::lround (midi);
        c = (midi - (float) n) * 100.0f;
        nn = juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
    }

    if (nowValid != valid || nn != note || std::abs (c - cents) > 0.4f)
    {
        valid = nowValid; note = nn; cents = c; hz = freqHz;
        repaint();
    }
}

void TunerView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool inTune = valid && std::abs (cents) < 5.0f;
    const auto col = ! valid ? NoirLookAndFeel::dim : (inTune ? juce::Colour (0xff7fd48a) : NoirLookAndFeel::accent);

    g.setColour (col);
    g.setFont (juce::FontOptions (r.getHeight() * 0.50f, juce::Font::bold));
    g.drawText (note, r.removeFromTop (r.getHeight() * 0.58f).toNearestInt(), juce::Justification::centred);

    auto bar = r.reduced (8.0f, 0.0f).withTrimmedBottom (4.0f);
    const float y = bar.getCentreY();
    g.setColour (NoirLookAndFeel::line);
    g.drawLine (bar.getX(), y, bar.getRight(), y, 1.5f);
    for (int i = -5; i <= 5; ++i)
    {
        const float x = bar.getCentreX() + (float) i / 5.0f * bar.getWidth() * 0.5f;
        g.drawLine (x, y - (i == 0 ? 6.0f : 3.0f), x, y + (i == 0 ? 6.0f : 3.0f), 1.0f);
    }
    if (valid)
    {
        const float x = bar.getCentreX() + juce::jlimit (-50.0f, 50.0f, cents) / 50.0f * bar.getWidth() * 0.5f;
        g.setColour (col);
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
    }
}

void LoopBar::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.0f, 2.0f);
    g.setColour (NoirLookAndFeel::line);
    g.fillRoundedRectangle (r, 2.0f);
    if (hasData)
    {
        g.setColour (juce::Colour (0xffd8c85a));
        g.fillRoundedRectangle (r.withWidth (juce::jmax (3.0f, r.getWidth() * progress)), 2.0f);
    }
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
        controls.push_back ({ &led, (Id) onId });
    }

    if (typeId >= 0)
    {
        type.addItemList (juce::StringArray::fromTokens (kParams[typeId].choices, "|", ""), 1);
        addAndMakeVisible (type);
        typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, kParams[typeId].id, type);
        controls.push_back ({ &type, (Id) typeId });
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
        controls.push_back ({ &k->slider, id });
        knobs.push_back (std::move (k));
    }

    for (auto& c : controls)
        c.first->addMouseListener (this, false);
}

int Pedal::desiredWidth() const
{
    if (fixedWidth > 0) return fixedWidth;
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

    g.setColour (isOn ? accent : NoirLookAndFeel::line.brighter (0.1f));
    g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 1.0f, 26.0f, 2.5f, 1.2f);

    const int textX = onId >= 0 ? 36 : 14;
    g.setColour (isOn ? NoirLookAndFeel::text : NoirLookAndFeel::dim);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), textX, 9, getWidth() - textX - 24, 18, juce::Justification::centredLeft);

    if (subtitle.isNotEmpty())
    {
        g.setColour (NoirLookAndFeel::dim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (subtitle, textX, 24, getWidth() - textX - 24, 12, juce::Justification::centredLeft);
    }

    if (slot >= 0)   // grip: six dots, top right
    {
        g.setColour (NoirLookAndFeel::dim.withAlpha (0.55f));
        for (int ix = 0; ix < 2; ++ix)
            for (int iy = 0; iy < 3; ++iy)
                g.fillEllipse ((float) getWidth() - 20.0f + (float) ix * 5.0f, 11.0f + (float) iy * 5.0f, 2.2f, 2.2f);
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
        body = onLayoutBody (body);

    if (knobs.empty() || body.isEmpty()) return;

    const int w = body.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto c = body.removeFromLeft (w);
        k->label.setBounds (c.removeFromBottom (15));
        k->slider.setBounds (c);
    }
}

void Pedal::mouseMove (const juce::MouseEvent& e)
{
    if (e.eventComponent != this) return;
    setMouseCursor (slot >= 0 && e.y < 38 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void Pedal::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        for (auto& c : controls)
            if (e.eventComponent == c.first || e.originalComponent == c.first)
            {
                if (onContextMenu) onContextMenu (c.second);
                return;
            }
        return;
    }

    if (e.eventComponent != this) return;   // left-clicks on child controls are handled by the controls themselves
    dragArmed = slot >= 0 && e.y < 38;
    if (dragArmed && onDragStart) onDragStart (*this, e);
}

void Pedal::mouseDrag (const juce::MouseEvent& e)
{
    if (e.eventComponent == this && dragArmed && onDragMove) onDragMove (*this, e);
}

void Pedal::mouseUp (const juce::MouseEvent& e)
{
    if (e.eventComponent == this && dragArmed)
    {
        dragArmed = false;
        if (onDragEnd) onDragEnd (*this, e);
    }
}

//==============================================================================
ChainStrip::ChainStrip (NoirProcessor& p, std::vector<juce::Colour> c) : proc (p), colours (std::move (c)) {}

juce::Rectangle<float> ChainStrip::boxFor (int i) const
{
    const float gap = 9.0f;
    const float n = (float) (NumSlots + 2);
    const float w = ((float) getWidth() - gap * (n - 1.0f)) / n;
    return { (w + gap) * (float) i, 3.0f, w, (float) getHeight() - 6.0f };
}

int ChainStrip::indexAt (juce::Point<int> pt) const
{
    for (int i = 1; i <= NumSlots; ++i)
        if (boxFor (i).contains (pt.toFloat()))
            return i;
    return -1;
}

void ChainStrip::paint (juce::Graphics& g)
{
    auto isOn = [this] (int idx)
    {
        if (idx == 0 || idx == NumSlots + 1) return true;
        return proc.apvts.getRawParameterValue (kParams[kSlotOn[order[(size_t) idx - 1]]].id)->load() > 0.5f;
    };

    for (int i = 0; i < NumSlots + 2; ++i)
    {
        const bool endpoint = i == 0 || i == NumSlots + 1;
        const int slot = endpoint ? -1 : order[(size_t) i - 1];
        const auto b = boxFor (i);
        const bool on = isOn (i);

        if (i + 1 < NumSlots + 2)
        {
            const auto nb = boxFor (i + 1);
            const float y = b.getCentreY();
            g.setColour (on && isOn (i + 1) ? NoirLookAndFeel::dim : NoirLookAndFeel::line);
            g.drawLine (b.getRight() + 0.5f, y, nb.getX() - 0.5f, y, 1.2f);
        }

        const auto col = endpoint ? NoirLookAndFeel::dim : colours[(size_t) slot];
        g.setColour (on && ! endpoint ? col.withAlpha (hover == i ? 0.32f : 0.20f) : NoirLookAndFeel::panel);
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (on ? col.withAlpha (endpoint ? 0.5f : 0.9f) : NoirLookAndFeel::line);
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

        g.setColour (on ? NoirLookAndFeel::text : NoirLookAndFeel::dim.withAlpha (0.7f));
        g.setFont (juce::FontOptions (9.5f, endpoint ? juce::Font::plain : juce::Font::bold));
        g.drawText (endpoint ? (i == 0 ? "IN" : "OUT") : juce::String (kSlotShort[slot]).toUpperCase(),
                    b.toNearestInt(), juce::Justification::centred);
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
    if (auto* p = proc.param (kSlotOn[order[(size_t) i - 1]]))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
    repaint();
}

//==============================================================================
Pedal* NoirEditor::makePedal (int slot)
{
    auto make = [this] (const char* title, juce::Colour c, int on, int type, std::initializer_list<Id> k, const char* sub) -> Pedal*
    {
        auto ped = std::make_unique<Pedal> (proc.apvts, title, c, on, type, k, sub);
        auto* p = ped.get();
        addAndMakeVisible (p);
        pedals.push_back (std::move (ped));
        return p;
    };

    switch (slot)
    {
        case SGate:   return make ("Noise Gate", cDyn,   GateOn,  -1,        { GateThr },                          "Dynamics");
        case SComp:   return make ("Compressor", cDyn,   CompOn,  -1,        { CompAmt, CompLvl, CompAtk },        "Dynamics / Sustain");
        case SWah:    return make ("Wah",        cFilt,  WahOn,   WahMode,   { WahPos, WahQ },                     "Filter");
        case SPitch:  return make ("Pitch",      cPitch, PitchOn, PitchType, { PitchShift, PitchMix },             "Pitch");
        case SBoost:  return make ("Boost",      cGain,  BoostOn, BoostType, { BoostGain },                        "Gain / EQ boost");
        case SDrive:  return make ("Drive",      cGain,  DrvOn,   DrvType,   { DrvDrive, DrvTone, DrvLevel },      "Gain");
        case SAmp:    return make ("Amp",        cAmp,   AmpOn,   -1,        { AmpGain, AmpBass, AmpMid, AmpTreble, AmpPres }, "Preamp + power stage");
        case SCab:    return make ("Cabinet",    cAmp,   CabOn,   CabType,   {},                                   "Speaker / IR");
        case SEq:     return make ("EQ",         cEq,    EqOn,    -1,        { EqLow, EqLowMid, EqHiMid, EqHigh }, "4-band parametric");
        case STalk:   return make ("Talk Box",   cVoice, TbOn,    -1,        { TbVowel, TbAuto, TbRes, TbMix },    "Vowel formants");
        case SVoc:    return make ("Vocoder",    cVoice, VocOn,   -1,        { VocSens, VocBright, VocMix },       "Mic on input 2");
        case SMod1:   return make ("Mod 1",      cMod,   M1On,    M1Type,    { M1Rate, M1Depth, M1Mix },           "Modulation");
        case SMod2:   return make ("Mod 2",      cMod,   M2On,    M2Type,    { M2Rate, M2Depth, M2Mix },           "Modulation");
        case SLofi:   return make ("Lo-Fi",      cLofi,  LoOn,    -1,        { LoBits, LoRate, LoMix },            "Bitcrusher / Sample rate");
        case SGlitch: return make ("Glitch",     cGlit,  GltOn,   GltMode,   { GltLen, GltAmt, GltMix },           "Stutter / Glitch");
        case SGran:   return make ("Granular",   cGrain, GrOn,    -1,        { GrSize, GrDens, GrPitch, GrSpread, GrMix }, "Grain cloud");
        case SFreeze: return make ("Freeze",     cGrain, FrzOn,   -1,        { FrzSize, FrzMix },                  "Hold / infinite sustain");
        case SDelay:  return make ("Delay",      cTime,  DlyOn,   DlyType,   { DlyTime, DlyFb, DlyMix },           "Echo");
        case SReverb: return make ("Reverb",     cTime,  RevOn,   RevType,   { RevSize, RevMix },                  "Space");
        case SLooper: return make ("Looper",     cLoop,  LoopOn,  LoopSpeed, { LoopLevel },                        "Record / overdub");
        default:      return nullptr;
    }
}

NoirEditor::NoirEditor (NoirProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    for (int s = 0; s < NumSlots; ++s)
    {
        auto* ped = makePedal (s);
        ped->slot = s;
        bySlot[s] = ped;
        ped->onDragStart = [this] (Pedal& pd, const juce::MouseEvent& e)
        {
            dragging = &pd;
            grabOffset = e.getEventRelativeTo (this).getPosition() - pd.getPosition();
            pd.toFront (false);
            pd.setAlpha (0.92f);
            repaint();
        };
        ped->onDragMove = [this] (Pedal& pd, const juce::MouseEvent& e)
        {
            if (dragging != &pd) return;
            const auto pos = e.getEventRelativeTo (this).getPosition();
            juce::Desktop::getInstance().getAnimator().cancelAnimation (&pd, false);
            pd.setTopLeftPosition (pos - grabOffset);

            for (auto* c : cards)
            {
                if (c == &pd || ! c->targetBounds.contains (pos)) continue;
                const auto from = std::find (cards.begin(), cards.end(), &pd);
                const auto to = std::find (cards.begin(), cards.end(), c);
                const auto idx = (size_t) std::distance (cards.begin(), to);
                cards.erase (from);
                cards.insert (cards.begin() + (long) idx, &pd);
                layoutCards (true);
                break;
            }
        };
        ped->onDragEnd = [this] (Pedal& pd, const juce::MouseEvent&)
        {
            pd.setAlpha (1.0f);
            dragging = nullptr;
            layoutCards (true);
            commitOrder();
            repaint();
        };
    }

    // Utility pedals (fixed position, not part of the chain)
    pTuner = new Pedal (proc.apvts, "Tuner", cOut, TunerOn, -1, { TunerRef }, "Mutes the output while on");
    pOut   = new Pedal (proc.apvts, "Output", cOut, -1, -1, { InGain, Master, OutGain }, "Levels");
    for (auto* u : { pTuner, pOut })
    {
        addAndMakeVisible (u);
        pedals.emplace_back (u);
    }
    pTuner->setDesiredWidth (250);

    // Tuner display
    pTuner->addAndMakeVisible (tunerView);
    pTuner->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        tunerView.setBounds (body.removeFromTop ((int) ((float) body.getHeight() * 0.56f)));
        return body;
    };

    // Cabinet: IR loader lives inside the cab pedal
    auto* cabPedal = bySlot[SCab];
    cabPedal->addAndMakeVisible (irButton);
    cabPedal->addChildComponent (irClear);
    irButton.onClick = [this] { chooseIR(); };
    irClear.onClick = [this] { proc.clearIR(); timerCallback(); };
    cabPedal->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        auto row = body.withTrimmedTop (4).removeFromTop (28).reduced (6, 0);
        irClear.setBounds (row.removeFromRight (28));
        row.removeFromRight (4);
        irButton.setBounds (row);
        return juce::Rectangle<int>();
    };

    // Looper: transport buttons
    auto* lp = bySlot[SLooper];
    lp->setDesiredWidth (330);
    for (auto* b : { &loopRec, &loopPlay, &loopDub, &loopStop, &loopClearBtn, &loopUndoBtn })
    {
        lp->addAndMakeVisible (b);
        b->getProperties().set ("accent", (int) cLoop.getARGB());
    }
    loopRec.getProperties().set ("accent", (int) cGlit.getARGB());
    lp->addAndMakeVisible (loopBar);
    loopRev.setButtonText ("REV");
    loopRev.getProperties().set ("label", true);
    lp->addAndMakeVisible (loopRev);
    loopRevAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[LoopRev].id, loopRev);
    loopRec.onClick  = [this] { setLoopMode (noir::Looper::Record); };
    loopPlay.onClick = [this] { setLoopMode (noir::Looper::Play); };
    loopDub.onClick  = [this] { setLoopMode (noir::Looper::Overdub); };
    loopStop.onClick = [this] { setLoopMode (noir::Looper::Stop); };
    loopClearBtn.onClick = [this] { proc.loopClear(); proc.setParamValue (LoopMode, 0.0f); };
    loopUndoBtn.onClick  = [this] { proc.loopUndo(); };
    lp->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        auto left = body.removeFromLeft ((int) ((float) body.getWidth() * 0.66f));
        auto row1 = left.removeFromTop (26);
        left.removeFromTop (4);
        auto row2 = left.removeFromTop (26);
        left.removeFromTop (8);
        loopBar.setBounds (left.removeFromTop (10));

        auto place = [] (juce::Rectangle<int> row, juce::Button& a, juce::Button& b, juce::Button& c)
        {
            const int w = row.getWidth() / 3;
            a.setBounds (row.removeFromLeft (w).reduced (2, 0));
            b.setBounds (row.removeFromLeft (w).reduced (2, 0));
            c.setBounds (row.reduced (2, 0));
        };
        place (row1, loopRec, loopPlay, loopDub);
        place (row2, loopStop, loopClearBtn, loopUndoBtn);

        loopRev.setBounds (body.removeFromTop (24).reduced (8, 0));
        return body;
    };

    for (int s = 0; s < NumSlots; ++s) cards.push_back (bySlot[s]);

    strip = std::make_unique<ChainStrip> (proc, std::vector<juce::Colour> (std::begin (slotColour), std::end (slotColour)));
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
    resetOrder.onClick = [this] { proc.setChainOrder (NoirProcessor::defaultOrder()); syncOrderFromProcessor(); };
    for (auto* b : { &prevPreset, &nextPreset, &allOn, &allOff, &resetOrder })
        addAndMakeVisible (b);

    power.getProperties().set ("power", true);
    power.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (power);
    powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[Power].id, power);

    for (auto& pd : pedals)
        pd->onContextMenu = [this] (Id id) { showParamMenu (id); };

    // DAW tab + header tabs
    daw = std::make_unique<DawPanel> (proc);
    addChildComponent (*daw);
    tabBoard.setClickingTogglesState (false);
    tabDaw.setClickingTogglesState (false);
    tabBoard.getProperties().set ("accent", (int) cGain.getARGB());
    tabDaw.getProperties().set ("accent", (int) cMod.getARGB());
    tabBoard.onClick = [this] { setTab (0); };
    tabDaw.onClick   = [this] { setTab (1); };
    cancelLearn.onClick = [this] { proc.cancelMidiLearn(); };
    addAndMakeVisible (tabBoard);
    addAndMakeVisible (tabDaw);
    addChildComponent (cancelLearn);
    tabBoard.setToggleState (true, juce::dontSendNotification);

    // Apply the stored chain order before the first layout
    {
        const auto o = proc.getChainOrder();
        cards.clear();
        for (int s : o) cards.push_back (bySlot[s]);
        strip->setOrder (o);
    }

    setResizable (true, true);
    setResizeLimits (1350, 700, 2400, 1300);
    setSize (1500, 880);
    startTimerHz (30);
    timerCallback();
}

NoirEditor::~NoirEditor()
{
    juce::Desktop::getInstance().getAnimator().cancelAllAnimations (false);
    setLookAndFeel (nullptr);
}

void NoirEditor::setAllEffects (bool on)
{
    for (int s = 0; s < NumSlots; ++s)
        if (auto* p = proc.param (kSlotOn[s]))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
}

void NoirEditor::setLoopMode (int mode)
{
    const int current = (int) proc.apvts.getRawParameterValue (kParams[LoopMode].id)->load();
    proc.setParamValue (LoopOn, 1.0f);
    // pressing REC / DUB again closes the pass and starts playback
    if (mode == current && (mode == noir::Looper::Record || mode == noir::Looper::Overdub))
        mode = noir::Looper::Play;
    proc.setParamValue (LoopMode, (float) mode);
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

void NoirEditor::setTab (int tab)
{
    currentTab = tab;
    const bool board = tab == 0;
    tabBoard.setToggleState (board, juce::dontSendNotification);
    tabDaw.setToggleState (! board, juce::dontSendNotification);

    strip->setVisible (board);
    for (auto& p : pedals) p->setVisible (board);
    for (auto* b : { &allOn, &allOff, &resetOrder }) b->setVisible (board);
    daw->setVisible (! board);
    repaint();
}

void NoirEditor::showParamMenu (Id id)
{
    juce::PopupMenu menu, macroMenu;
    menu.addSectionHeader (paramLabel (id));

    const int cc = proc.getCcFor (id);
    menu.addItem (1, "MIDI Learn  (then move a CC / footswitch)");
    if (cc >= 0) menu.addItem (2, "Clear MIDI mapping  (CC " + juce::String (cc) + ")");

    for (int m = 0; m < NoirProcessor::NumMacros; ++m)
        macroMenu.addItem (100 + m, "Macro " + juce::String (m + 1));
    menu.addSubMenu ("Assign to Macro", macroMenu, id < Macro1);

    menu.addSeparator();
    menu.addItem (3, "Reset to default");

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, id] (int result)
    {
        if (result == 1) proc.startMidiLearn (id);
        else if (result == 2) proc.clearMidiFor (id);
        else if (result == 3) proc.setParamValue (id, kParams[id].def);
        else if (result >= 100) proc.assignMacro (result - 100, id);
    });
}

void NoirEditor::commitOrder()
{
    NoirProcessor::Order o {};
    for (size_t i = 0; i < cards.size(); ++i) o[i] = cards[i]->slot;
    proc.setChainOrder (o);
    strip->setOrder (o);
}

void NoirEditor::syncOrderFromProcessor()
{
    if (dragging != nullptr) return;

    const auto o = proc.getChainOrder();
    bool same = true;
    for (size_t i = 0; i < cards.size(); ++i)
        if (cards[i]->slot != o[i]) { same = false; break; }

    strip->setOrder (o);
    if (same) return;

    cards.clear();
    for (int s : o) cards.push_back (bySlot[s]);
    layoutCards (true);
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

    // Looper
    const int mode = (int) proc.apvts.getRawParameterValue (kParams[LoopMode].id)->load();
    const bool loopOn = proc.apvts.getRawParameterValue (kParams[LoopOn].id)->load() > 0.5f;
    loopRec.setToggleState (loopOn && mode == 1, juce::dontSendNotification);
    loopPlay.setToggleState (loopOn && mode == 2, juce::dontSendNotification);
    loopDub.setToggleState (loopOn && mode == 3, juce::dontSendNotification);
    loopStop.setToggleState (mode == 0, juce::dontSendNotification);
    loopUndoBtn.setEnabled (proc.loopCanUndo.load());
    loopBar.set (proc.loopProgress.load(), proc.loopHasData.load());

    // Tuner
    if (proc.apvts.getRawParameterValue (kParams[TunerOn].id)->load() > 0.5f)
    {
        const auto r = proc.getTuner().analyse();
        tunerView.setReading (r.freq, proc.apvts.getRawParameterValue (kParams[TunerRef].id)->load());
    }
    else
    {
        tunerView.setReading (0.0f, 440.0f);
    }

    const int learning = proc.getLearnTarget();
    if (cancelLearn.isVisible() != (learning >= 0)) { cancelLearn.setVisible (learning >= 0); resized(); }

    syncOrderFromProcessor();
    for (auto& p : pedals) p->refresh();
    strip->repaint();
    repaint (headerArea.getRight() - 260, 8, 260, 44);
    if (proc.getLearnTarget() >= 0 || cancelLearn.isVisible()) repaint (headerArea);
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

    // drop slot while dragging a pedal
    if (dragging != nullptr)
    {
        const auto r = dragging->targetBounds.toFloat().reduced (1.0f);
        g.setColour (NoirLookAndFeel::accent.withAlpha (0.07f));
        g.fillRoundedRectangle (r, 9.0f);
        juce::Path outline;
        outline.addRoundedRectangle (r, 9.0f);
        juce::Path dashed;
        const float dashes[] = { 5.0f, 4.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (NoirLookAndFeel::accent.withAlpha (0.7f));
        g.fillPath (dashed);
    }

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

    if (const int learning = proc.getLearnTarget(); learning >= 0)
    {
        g.setColour (NoirLookAndFeel::accent);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("MIDI LEARN  >  " + paramLabel (learning) + "   -   move a knob or press a footswitch",
                    headerArea.withTrimmedLeft (380).withTrimmedRight (700), juce::Justification::centredLeft);
    }
}

void NoirEditor::paintOverChildren (juce::Graphics&) {}

void NoirEditor::layoutCards (bool animate)
{
    const int gap = 10;
    std::vector<Pedal*> flow (cards.begin(), cards.end());
    flow.push_back (pTuner);
    flow.push_back (pOut);

    std::vector<std::vector<Pedal*>> rows;
    int sum = 0;
    for (auto* p : flow)
    {
        const int w = p->desiredWidth();
        if (! rows.empty() && sum + gap + w <= gridArea.getWidth()) { rows.back().push_back (p); sum += gap + w; }
        else { rows.push_back ({ p }); sum = w; }
    }

    const int nRows = (int) rows.size();
    const int rowH = juce::jlimit (150, 215, (gridArea.getHeight() - gap * (nRows - 1)) / juce::jmax (1, nRows));

    int y = gridArea.getY();
    for (int r = 0; r < nRows; ++r)
    {
        auto& row = rows[(size_t) r];
        int total = 0;
        for (auto* p : row) total += p->desiredWidth();
        const int avail = gridArea.getWidth() - gap * ((int) row.size() - 1);
        const bool last = r == nRows - 1;
        const float scale = last ? juce::jmin (1.15f, (float) avail / (float) total) : (float) avail / (float) total;

        int x = gridArea.getX();
        for (size_t i = 0; i < row.size(); ++i)
        {
            const bool lastInRow = i + 1 == row.size();
            const int w = (! last && lastInRow) ? gridArea.getRight() - x
                                                : (int) std::floor ((float) row[i]->desiredWidth() * scale);
            row[i]->targetBounds = { x, y, w, rowH };
            x += w + gap;
        }
        y += rowH + gap;
    }

    auto& animator = juce::Desktop::getInstance().getAnimator();
    for (auto* p : flow)
    {
        if (p == dragging) continue;
        if (p->getBounds() == p->targetBounds) continue;

        if (animate && p->getWidth() > 0)
            animator.animateComponent (p, p->targetBounds, 1.0f, 150, false, 1.0, 1.0);
        else
        {
            animator.cancelAnimation (p, false);
            p->setBounds (p->targetBounds);
        }
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
        tabBoard.setBounds (h.removeFromLeft (76).withSizeKeepingCentre (72, 28));
        h.removeFromLeft (4);
        tabDaw.setBounds (h.removeFromLeft (76).withSizeKeepingCentre (72, 28));
        h.removeFromLeft (12);

        auto right = h.removeFromRight (640);
        power.setBounds (right.removeFromRight (110).withSizeKeepingCentre (110, 28));
        right.removeFromRight (10);
        resetOrder.setBounds (right.removeFromRight (104).withSizeKeepingCentre (104, 28));
        right.removeFromRight (6);
        allOff.setBounds (right.removeFromRight (74).withSizeKeepingCentre (74, 28));
        right.removeFromRight (6);
        allOn.setBounds (right.removeFromRight (64).withSizeKeepingCentre (64, 28));
        right.removeFromRight (14);

        auto meters = right.withSizeKeepingCentre (right.getWidth() - 20, 22);
        meterIn  = meters.removeFromTop (8).withTrimmedLeft (26);
        meters.removeFromTop (6);
        meterOut = meters.removeFromTop (8).withTrimmedLeft (26);

        cancelLearn.setBounds (h.removeFromRight (80).withSizeKeepingCentre (76, 28));
        auto mid = h.withSizeKeepingCentre (juce::jmin (h.getWidth(), 340), 28);
        prevPreset.setBounds (mid.removeFromLeft (30));
        mid.removeFromLeft (6);
        nextPreset.setBounds (mid.removeFromRight (30));
        mid.removeFromRight (6);
        presetBox.setBounds (mid);
    }

    area.removeFromTop (6);
    daw->setBounds (area);
    strip->setBounds (area.removeFromTop (32));
    area.removeFromTop (12);

    gridArea = area;
    layoutCards (false);
}
