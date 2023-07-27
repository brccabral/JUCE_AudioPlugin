/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void LookAndFeel::drawRotarySlider(juce::Graphics &g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider &slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    // * fill circle
    g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey);
    g.fillEllipse(bounds);

    // * border
    g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    // * check if we can cast "slider" to "RotarySliderWithLabels"
    if (auto *rswl = dynamic_cast<RotarySliderWithLabels *>(&slider))
    {
        // * draw nob indicator
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5); // * give space for the text

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        // * draw text
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;

    if (auto *pb = dynamic_cast<PowerButton *>(&toggleButton))
    {
        Path powerButton;

        auto bounds = toggleButton.getLocalBounds();

#ifdef DEBUG
        g.setColour(Colours::red);
        g.drawRect(bounds);
#endif

        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - JUCE_LIVE_CONSTANT(6);
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = JUCE_LIVE_CONSTANT(30.f);

        size -= JUCE_LIVE_CONSTANT(6);

        powerButton.addCentredArc(r.getCentreX(),
                                  r.getCentreY(),
                                  size * 0.5,
                                  size * 0.5,
                                  0.f,
                                  degreesToRadians(ang),
                                  degreesToRadians(360.f - ang),
                                  true);

        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
    else if (auto *analyzerButton = dynamic_cast<AnalyzerButton *>(&toggleButton))
    {
        auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(color);

        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);

        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    // * if we can convert to AudioParameterChoice, return the name
    if (auto *choiceParam = dynamic_cast<juce::AudioParameterChoice *>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false; // * if kHz

    if (auto *floatParam = dynamic_cast<juce::AudioParameterFloat *>(param))
    {
        float val = getValue();

        if (val > 999.f)
        {
            val /= 1000.f; // 1001 / 1000 = 1.001
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0)); // * val, 2 decimal places
    }
    else
    {
        // * if not AudioParameterFloat, throw an error to make sure we implement this
        jassertfalse; // this shouldn't happen!
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";

        str << suffix;
    }

    return str;
}

//==============================================================================
RotarySliderWithLabels::RotarySliderWithLabels(juce::RangedAudioParameter &rap, const juce::String &unitSuffix)
    : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                   juce::Slider::TextEntryBoxPosition::NoTextBox),
      param(&rap),
      suffix(unitSuffix)
{
    setLookAndFeel(&lnf);
}

RotarySliderWithLabels::~RotarySliderWithLabels()
{
    setLookAndFeel(nullptr);
}

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();
    auto sliderBounds = getSliderBounds();

#ifdef DEBUG
    // * full component border
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds());

    // * lookandfeel border
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);
#endif

    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), // * normalize
                                      startAng,
                                      endAng,
                                      *this);

    // * show ranges (min/max)
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        // * give some space away from circle to draw text
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

// * put circle inside a square
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(AudioPlugin_JUCEAudioProcessor &p) : audioProcessor(p),
                                                                                    leftPathProducer(audioProcessor.leftChannelFifo),
                                                                                    rightPathProducer(audioProcessor.rightChannelFifo)
{

    const auto &params = audioProcessor.getParameters();
    for (auto param : params)
        param->addListener(this);

    updateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto &params = audioProcessor.getParameters();
    for (auto param : params)
        param->removeListener(this);
}

std::vector<float> ResponseCurveComponent::getFrequencies()
{
    return std::vector<float>{
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000};
}

std::vector<float> ResponseCurveComponent::getGains()
{
    return std::vector<float>{
        -24, -12, 0, 12, 24};
}

std::vector<float> ResponseCurveComponent::getXs(const std::vector<float> &freqs, float left, float width)
{
    std::vector<float> xs;
    for (auto f : freqs)
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back(left + width * normX);
    }

    return xs;
}

// * update background Grid lines if window gets resized
void ResponseCurveComponent::drawBackgroundGrid(juce::Graphics &g)
{
    using namespace juce;
    // * vertical lines
    auto freqs = getFrequencies();

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto xs = getXs(freqs, left, width);

    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    // * horizontal lines
    auto gain = getGains();

    for (auto gDB : gain)
    {
        auto y = jmap(gDB, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDB == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
}

void ResponseCurveComponent::drawTextLabels(juce::Graphics &g)
{
    using namespace juce;

    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();

    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);

        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);

        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void ResponseCurveComponent::updateResponseCurve()
{
    using namespace juce;

    auto responseArea = getAnalysisArea();
    auto w = responseArea.getWidth();

    auto &lowCut = monoChain.get<ChainPositions::LowCut>();
    auto &peak = monoChain.get<ChainPositions::Peak>();
    auto &highCut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    // * calculate Magnitude for each filter, combine them and store in the vector
    std::vector<double> mags; // * magnitudes
    mags.resize(w);

    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10((double(i) / double(w)), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::LowCut>())
        {
            if (!lowCut.isBypassed<0>())
                mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<1>())
                mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<2>())
                mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<3>())
                mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!monoChain.isBypassed<ChainPositions::HighCut>())
        {
            if (!highCut.isBypassed<0>())
                mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<1>())
                mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<2>())
                mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<3>())
                mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    // * chart limits
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        // * -24 to 24 comes from "Peak Gain"
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    // * draw chart
    responseCurve.clear();
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
}

void ResponseCurveComponent::paint(juce::Graphics &g)
{
    using namespace juce;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    // * background = grid lines
    drawBackgroundGrid(g);

    auto responseArea = getAnalysisArea();

    // * draw FFT
    if (shouldShowFFTAnalysis)
    {
        // * start drawing from the bottom of chart area
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colours::skyblue);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colour(215u, 201u, 134u));
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }

    // * chart border
    Path border;
    border.setUsingNonZeroWinding(false);
    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());

    g.setColour(Colours::darkgrey);
    g.fillPath(border);

    // * draw chart box
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    drawTextLabels(g);

    // * draw filter curve
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
    using namespace juce;

    responseCurve.preallocateSpace(getWidth() * 3);
    updateResponseCurve();
}

//==============================================================================
AudioPlugin_JUCEAudioProcessorEditor::AudioPlugin_JUCEAudioProcessorEditor(
    AudioPlugin_JUCEAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      // * sliders attachments
      peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
      peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
      peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
      lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
      highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
      lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
      highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
      // * chart
      responseCurveComponent(audioProcessor),
      // * sliders components
      peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
      peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
      peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
      lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
      highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
      lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
      highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "db/Oct"),
      // * bypass
      lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowcutBypassButton),
      peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
      highcutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highcutBypassButton),
      analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(480, 500);

    peakFreqSlider.labels.add({0.f, "20Hz"});
    peakFreqSlider.labels.add({1.f, "20kHz"});

    peakGainSlider.labels.add({0.f, "-24dB"});
    peakGainSlider.labels.add({1.f, "+24dB"});

    peakQualitySlider.labels.add({0.f, "0.1"});
    peakQualitySlider.labels.add({1.f, "10.0"});

    lowCutFreqSlider.labels.add({0.f, "20Hz"});
    lowCutFreqSlider.labels.add({1.f, "20kHz"});

    highCutFreqSlider.labels.add({0.f, "20Hz"});
    highCutFreqSlider.labels.add({1.f, "20kHz"});

    lowCutSlopeSlider.labels.add({0.0f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});

    highCutSlopeSlider.labels.add({0.0f, "12"});
    highCutSlopeSlider.labels.add({1.f, "48"});

    for (auto *comp : getComps())
        addAndMakeVisible(comp);

    peakBypassButton.setLookAndFeel(&lnf);
    lowcutBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);
    analyzerEnabledButton.setLookAndFeel(&lnf);

    // * use Bypass buttons to disable Sliders
    auto safePtr = juce::Component::SafePointer<AudioPlugin_JUCEAudioProcessorEditor>(this);
    peakBypassButton.onClick = [safePtr]()
    {
        if (auto *comp = safePtr.getComponent())
        {
            auto bypassed = comp->peakBypassButton.getToggleState();

            comp->peakFreqSlider.setEnabled(!bypassed);
            comp->peakGainSlider.setEnabled(!bypassed);
            comp->peakQualitySlider.setEnabled(!bypassed);
        }
    };

    lowcutBypassButton.onClick = [safePtr]()
    {
        if (auto *comp = safePtr.getComponent())
        {
            auto bypassed = comp->lowcutBypassButton.getToggleState();

            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    highcutBypassButton.onClick = [safePtr]()
    {
        if (auto *comp = safePtr.getComponent())
        {
            auto bypassed = comp->highcutBypassButton.getToggleState();

            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    analyzerEnabledButton.onClick = [safePtr]()
    {
        if (auto *comp = safePtr.getComponent())
        {
            auto enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
        }
    };
}

AudioPlugin_JUCEAudioProcessorEditor::~AudioPlugin_JUCEAudioProcessorEditor()
{
    peakBypassButton.setLookAndFeel(nullptr);
    lowcutBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPlugin_JUCEAudioProcessorEditor::paint(juce::Graphics &g)
{
    using namespace juce;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    // * Title bar
    Path curve;

    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    g.setFont(Font("Iosevka Term Slab", 30, 0)); // https://github.com/be5invis/Iosevka

    String title{"PFM::C++ FOR MUSICIANS"};
    g.setFont(30);
    auto titleWidth = g.getCurrentFont().getStringWidth(title);

    curve.startNewSubPath(center.x, 32);
    curve.lineTo(center.x - titleWidth * 0.45f, 32);

    auto cornerSize = 20;
    auto curvePos = curve.getCurrentPosition();
    curve.quadraticTo(curvePos.getX() - cornerSize, curvePos.getY(),
                      curvePos.getX() - cornerSize, curvePos.getY() - 16);
    curvePos = curve.getCurrentPosition();
    curve.quadraticTo(curvePos.getX(), 2,
                      curvePos.getX() - cornerSize, 2);

    curve.lineTo({0.f, 2.f});
    curve.lineTo(0.f, 0.f);
    curve.lineTo(center.x, 0.f);
    curve.closeSubPath();

    g.setColour(Colour(97u, 18u, 167u));
    g.fillPath(curve);

    curve.applyTransform(AffineTransform().scaled(-1, 1));
    curve.applyTransform(AffineTransform().translated(getWidth(), 0));
    g.fillPath(curve);

    g.setColour(Colour(255u, 154u, 1u));
    g.drawFittedText(title, bounds, juce::Justification::centredTop, 1);
}

void AudioPlugin_JUCEAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    bounds.removeFromTop(4);

    auto analyzerEnabledArea = bounds.removeFromTop(25);
    analyzerEnabledArea.setWidth(50);
    analyzerEnabledArea.setX(5);
    analyzerEnabledArea.removeFromTop(2);

    analyzerEnabledButton.setBounds(analyzerEnabledArea);

    bounds.removeFromTop(5);

    // * to use JUCE_LIVE_CONSTANT, compile with DEBUG and run the standalone program
    // * from the same folder as Makefile
    float responseHeigh = JUCE_LIVE_CONSTANT(25);
    float hRatio = responseHeigh / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);
    DBG(__FILE__);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5); // * 50% of the rest (1 - 0.33)

    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakBypassButton.setBounds(bounds.removeFromTop(25));
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5)); // * 50% of the rest (1 - 0.33)
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component *> AudioPlugin_JUCEAudioProcessorEditor::getComps()
{
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        &lowcutBypassButton,
        &peakBypassButton,
        &highcutBypassButton,
        &analyzerEnabledButton};
}

// * we added Editor as listener for any param change
// * this sets "parametersChanged" which is checked in "timerCallback()" every 60Hz
void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    // * tempIncomingBuffer has random size, we copy to monoBuffer in a fixed size
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (channelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (channelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);

            channelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    /*
     if there are FFT data buffers to pull
        if we can pull a buffer
            generate a path
     */
    const auto fftSize = channelFFTDataGenerator.getFFTSize();

    /*
     48000 / 2048 = 23hz  <- this is the bin width
     */
    const auto binWidth = sampleRate / (double)fftSize;

    while (channelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (channelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    /*
     while there are paths that can be pull
        pull as many as we can
            display the most recent path
     */

    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(channelFFTPath);
    }
}

// * called every 60Hz, checks if "parametersChanged" has been set by any param as
// * we added Editor as listener for param change
void ResponseCurveComponent::timerCallback()
{
    if (shouldShowFFTAnalysis)
    {
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();

        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    if (parametersChanged.compareAndSetBool(false, true))
    {
        DBG("ResponseCurveComponent::timerCallback() parametersChanged");
        updateChain();
        updateResponseCurve();
    }

    // * signal a repaint
    // * now we need to repaint all the time because the FFT gets updated all the time
    repaint();
}

void ResponseCurveComponent::updateChain()
{
    // * update the monochain
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);

    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
