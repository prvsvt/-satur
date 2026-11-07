#include "PluginEditor.h"
#include "BinaryData.h"

static juce::String RU (const char* s) { return juce::String::fromUTF8 (s); }

namespace
{
    struct KnobInfo { const char* id; const char* title; const char* suffix; juce::uint32 colour; int decimals; };

    const KnobInfo infos[7] = {
        { "thresh",  "ТРЕЗВОСТЬ", " dB", 0xff39ff88, 1 },  // threshold
        { "ratio",   "ГРАДУС",    " :1", 0xffff2e93, 1 },  // ratio
        { "attack",  "РЕАКЦИЯ",   " ms", 0xff00e5ff, 1 },  // attack
        { "release", "ОТХОДНЯК",  " ms", 0xffffe14d, 0 },  // release
        { "drive",   "НАКАТИЛ",   " dB", 0xffb26bff, 1 },  // drive / saturation
        { "mix",     "ЗАКУСКА",   " %",  0xffff7a1a, 0 },  // dry/wet
        { "output",  "ГРОМКОСТЬ", " dB", 0xffffffff, 1 },  // output
    };

    const juce::Colour bgTop (0xff1a0b2e), bgBottom (0xff05020c);
    const juce::Colour neonPink (0xffff2e93), neonGreen (0xff39ff88), neonCyan (0xff00e5ff);
}

//==============================================================================
void KonLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                float pos, float startAngle, float endAngle,
                                juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const bool hover = s.isMouseOverOrDragging();
    const bool down  = s.isMouseButtonDown();
    const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float ang = startAngle + pos * (endAngle - startAngle);

    // свечение: сильнее когда крутишь и чем больше значение
    const float glowAmt = (down ? 0.55f : hover ? 0.35f : 0.15f) + pos * 0.15f;
    juce::ColourGradient glow (col.withAlpha (glowAmt), c.x, c.y,
                               col.withAlpha (0.0f), c.x, c.y - r * 1.5f, true);
    g.setGradientFill (glow);
    g.fillEllipse (c.x - r * 1.5f, c.y - r * 1.5f, r * 3.0f, r * 3.0f);

    // тело
    juce::ColourGradient body (juce::Colour (0xff2a1a44), c.x, c.y - r,
                               juce::Colour (0xff0b0614), c.x, c.y + r, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - r * 0.75f, c.y - r * 0.75f, r * 1.5f, r * 1.5f);
    g.setColour (col.withAlpha (0.5f));
    g.drawEllipse (c.x - r * 0.75f, c.y - r * 0.75f, r * 1.5f, r * 1.5f, down ? 2.5f : 1.5f);

    // дорожка
    const float ar = r * 0.9f;
    juce::Path track;
    track.addCentredArc (c.x, c.y, ar, ar, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // значение (толстая дуга + светящийся ореол)
    juce::Path val;
    val.addCentredArc (c.x, c.y, ar, ar, 0.0f, startAngle, ang, true);
    g.setColour (col.withAlpha (0.25f));
    g.strokePath (val, juce::PathStrokeType (down ? 11.0f : 8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col);
    g.strokePath (val, juce::PathStrokeType (down ? 5.5f : 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // стрелка
    juce::Path p;
    p.addRoundedRectangle (-2.0f, -r * 0.68f, 4.0f, r * 0.34f, 2.0f);
    g.setColour (juce::Colours::white);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));

    // точка по центру
    g.setColour (col);
    g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
}

//==============================================================================
KonEditor::KonEditor (KonProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    setSize (700, 470);

    horseImg = juce::ImageCache::getFromMemory (BinaryData::horse_png, BinaryData::horse_pngSize);

    for (int i = 0; i < numKnobs; ++i)
    {
        auto& s = sliders[(size_t) i];
        auto& l = labels[(size_t) i];
        const auto& k = infos[i];

        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        s.setTextValueSuffix (k.suffix);
        s.setNumDecimalPlacesToDisplay (k.decimals);
        s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (k.colour));
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setMouseDragSensitivity (180);
        addAndMakeVisible (s);

        l.setText (RU (k.title), juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (13.5f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, juce::Colour (k.colour));
        addAndMakeVisible (l);

        attachments[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, k.id, s);
    }

    startTimerHz (30);
}

KonEditor::~KonEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void KonEditor::timerCallback()
{
    const float gr = proc.grMeter.load();
    grSmooth += (gr - grSmooth) * 0.25f;

    drive01 = proc.apvts.getRawParameterValue ("drive")->load() / 24.0f;

    // чем больше "накатил", тем сильнее конь качается
    phase += 0.12f + drive01 * 0.35f;
    tilt = std::sin (phase) * (0.012f + drive01 * 0.05f);

    // на сжатии конь "надувается" и дрожит
    zoom = 1.0f + juce::jmin (grSmooth, 20.0f) * 0.006f + std::sin (phase * 0.5f) * 0.006f;
    shakeX = (rng.nextFloat() - 0.5f) * grSmooth * 0.5f;
    shakeY = (rng.nextFloat() - 0.5f) * grSmooth * 0.5f;

    repaint();
}

void KonEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // фон
    g.setGradientFill (juce::ColourGradient (bgTop, 0, 0, bgBottom, 0, bounds.getHeight(), false));
    g.fillAll();

    // неоновая сетка
    g.setColour (neonPink.withAlpha (0.05f));
    for (float gx = 0; gx < bounds.getWidth(); gx += 28.0f)
        g.drawVerticalLine ((int) gx, 0.0f, bounds.getHeight());
    for (float gy = 0; gy < bounds.getHeight(); gy += 28.0f)
        g.drawHorizontalLine ((int) gy, 0.0f, bounds.getWidth());

    // заголовок с "глитч" эффектом
    const auto title = RU ("ПЬЯНЫЙ КОНЬ");
    g.setFont (juce::Font (34.0f, juce::Font::bold));
    const float gl = 1.5f + drive01 * 3.0f;
    g.setColour (neonCyan.withAlpha (0.8f));
    g.drawText (title, 20 - (int) gl, 12, 400, 44, juce::Justification::centredLeft);
    g.setColour (neonPink.withAlpha (0.8f));
    g.drawText (title, 20 + (int) gl, 12, 400, 44, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white);
    g.drawText (title, 20, 12, 400, 44, juce::Justification::centredLeft);

    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.setColour (neonGreen);
    g.drawText (RU ("КОМПРЕССОР + САТУРАТОР  //  v1.0"), 22, 50, 400, 16, juce::Justification::centredLeft);

    // --- конь ---
    horseArea = juce::Rectangle<float> (20.0f, 80.0f, 250.0f, 300.0f);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle (horseArea, 22.0f);
    g.reduceClipRegion (clip, {});
    const auto ctr = horseArea.getCentre();
    g.addTransform (juce::AffineTransform::rotation (tilt, ctr.x, ctr.y)
                        .followedBy (juce::AffineTransform::scale (zoom, zoom, ctr.x, ctr.y))
                        .translated (shakeX, shakeY));
    g.drawImage (horseImg, horseArea.expanded (24.0f),
                 juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
    g.restoreState();

    // розовая "пьяная" вуаль
    g.saveState();
    g.reduceClipRegion (clip, {});
    g.setColour (neonPink.withAlpha (drive01 * 0.28f));
    g.fillRect (horseArea);
    g.restoreState();

    // рамка пульсирует при сжатии
    const float borderGlow = juce::jlimit (0.0f, 1.0f, grSmooth / 12.0f);
    g.setColour (neonGreen.withAlpha (0.25f + borderGlow * 0.6f));
    g.drawRoundedRectangle (horseArea, 22.0f, 2.0f + borderGlow * 4.0f);

    // подпись по уровню "накатил"
    juce::String phrase = drive01 < 0.17f ? RU ("ЕЩЁ ТРЕЗВЫЙ")
                        : drive01 < 0.45f ? RU ("НАВЕСЕЛЕ")
                        : drive01 < 0.75f ? RU ("КАЧАЕТ")
                                          : RU ("В ХЛАМ");
    auto tag = juce::Rectangle<float> (horseArea.getX() + 30.0f, horseArea.getBottom() - 44.0f,
                                       horseArea.getWidth() - 60.0f, 30.0f);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (tag, 15.0f);
    g.setColour (neonPink);
    g.drawRoundedRectangle (tag, 15.0f, 1.5f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (15.0f, juce::Font::bold));
    g.drawText (phrase, tag, juce::Justification::centred);

    // --- измеритель сжатия ---
    auto meter = juce::Rectangle<float> (284.0f, 80.0f, 16.0f, 300.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (meter, 8.0f);
    const float fill = juce::jlimit (0.0f, 1.0f, grSmooth / 20.0f);
    auto bar = meter.withHeight (meter.getHeight() * fill);
    g.setGradientFill (juce::ColourGradient (neonGreen, 0, meter.getY(), neonPink, 0, meter.getBottom(), false));
    g.fillRoundedRectangle (bar, 8.0f);
    g.setColour (neonGreen);
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText (RU ("СЖАТИЕ"), (int) meter.getX() - 14, (int) meter.getBottom() + 4, 44, 14,
                juce::Justification::centred);
    g.drawText (juce::String (grSmooth, 1) + " dB", (int) meter.getX() - 18, (int) meter.getBottom() + 18, 52, 14,
                juce::Justification::centred);

    // подсказка
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.setFont (juce::Font (12.0f, juce::Font::plain));
    g.drawText (RU ("крути ручки • двойной клик = сброс"), 340, 400, 340, 40,
                juce::Justification::centredRight);
}

void KonEditor::resized()
{
    const int startX = 325, startY = 72;
    const int cellW = 124, cellH = 128;

    for (int i = 0; i < numKnobs; ++i)
    {
        const int col = i % 3, row = i / 3;
        juce::Rectangle<int> cell (startX + col * cellW, startY + row * cellH, cellW, cellH);
        labels[(size_t) i].setBounds (cell.removeFromTop (18));
        sliders[(size_t) i].setBounds (cell);
    }
}
