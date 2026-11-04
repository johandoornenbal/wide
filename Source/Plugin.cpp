#include <JuceHeader.h>
#include "WiderEngine.h"
#include <atomic>

class WiderProcessor final : public juce::AudioProcessor {
public:
    WiderProcessor() : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        state(*this, nullptr, "WiderState", layout()) {}
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout p;
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"width",1}, "Width", -1.f, 1.f, 0.f));
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"center",1}, "Center", -1.f, 1.f, 0.f));
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix",1}, "Mix", 0.f, 1.f, 1.f));
        return p;
    }
    void prepareToPlay(double rate, int) override { engine.reset(rate); correlation=0; cross=energyL=energyR=0; coefficient=std::exp(-1.0/(rate*.3)); }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& b) const override {
        return b.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            && b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    template<class T> void process(juce::AudioBuffer<T>& b) {
        juce::ScopedNoDenormals guard;
        engine.parameters((state.getRawParameterValue("width")->load()+1.f)*.5f,
                          (state.getRawParameterValue("center")->load()+1.f)*.5f,
                          state.getRawParameterValue("mix")->load());
        T* data[]={b.getWritePointer(0), b.getWritePointer(1)};
        if constexpr (std::is_same_v<T,float>) engine.processReplacing(data,data,b.getNumSamples());
        else engine.processDoubleReplacing(data,data,b.getNumSamples());
        for(int i=0;i<b.getNumSamples();++i) {
            const double l=data[0][i], r=data[1][i];
            cross=coefficient*cross+(1-coefficient)*l*r;
            energyL=coefficient*energyL+(1-coefficient)*l*l;
            energyR=coefficient*energyR+(1-coefficient)*r*r;
        }
        const double denominator=std::sqrt(energyL*energyR);
        correlation.store(denominator>1.e-12 ? static_cast<float>(std::clamp(cross/denominator,-1.,1.)) : 0.f);
        meterSequence.fetch_add(1, std::memory_order_relaxed);
    }
    void processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override { process(b); }
    void processBlock(juce::AudioBuffer<double>& b, juce::MidiBuffer&) override { process(b); }
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override { clearMeter(); }
    void processBlockBypassed(juce::AudioBuffer<double>&, juce::MidiBuffer&) override { clearMeter(); }
    void clearMeter() { cross=energyL=energyR=0; correlation=0; ++meterSequence; }
    bool supportsDoublePrecisionProcessing() const override { return true; }
    const juce::String getName() const override { return "Wider Studio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock& dst) override {
        if(auto xml=state.copyState().createXml()) copyXmlToBinary(*xml,dst);
    }
    void setStateInformation(const void* data,int size) override {
        if(auto xml=getXmlFromBinary(data,size); xml && xml->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
    }
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> correlation{0};
    std::atomic<uint32_t> meterSequence{0};
private:
    WiderEngine engine;
    double cross=0, energyL=0, energyR=0, coefficient=0;
};

namespace style {
const juce::Colour cream{0xfff4eddb}, ink{0xff252b29}, gold{0xffd7ac61}, muted{0xff858b80};
class Look final : public juce::LookAndFeel_V4 {
public:
    Look() {
        setColour(juce::Slider::textBoxTextColourId, cream);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TextEditor::highlightColourId, gold.withAlpha(.4f));
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
        auto r=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(16);
        auto radius=juce::jmin(r.getWidth(),r.getHeight())*.5f;
        auto cx=r.getCentreX(), cy=r.getCentreY();
        juce::Path track, active;
        track.addCentredArc(cx,cy,radius,radius,0,start,end,true);
        active.addCentredArc(cx,cy,radius,radius,0,start,start+pos*(end-start),true);
        g.setColour(cream.withAlpha(.12f)); g.strokePath(track,juce::PathStrokeType(3));
        g.setColour(gold); g.strokePath(active,juce::PathStrokeType(3));
        auto dial=juce::Rectangle<float>(radius*1.58f,radius*1.58f).withCentre({cx,cy});
        g.setColour(juce::Colours::black.withAlpha(.3f)); g.fillEllipse(dial.translated(0,5));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff535a52),cx,cy-radius,ink,cx,cy+radius,false));
        g.fillEllipse(dial); g.setColour(cream.withAlpha(.15f)); g.drawEllipse(dial,1);
        const auto angle=start+pos*(end-start);
        g.setColour(gold);
        g.drawLine(cx+std::sin(angle)*radius*.48f,cy-std::cos(angle)*radius*.48f,
                   cx+std::sin(angle)*radius*.68f,cy-std::cos(angle)*radius*.68f,3);
    }
};
}


class CorrelationMeter final : public juce::Component {
public:
    CorrelationMeter() { setTitle("Output correlation"); setDescription("Output stereo correlation, minus one to plus one; zero at silence"); }
    void setValue(float v) { value=juce::jlimit(-1.f,1.f,v); repaint(); }
    void paint(juce::Graphics& g) override {
        auto b=getLocalBounds().toFloat();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff777d70),0,0,juce::Colour(0xff151b18),0,b.getHeight(),false));
        g.fillRoundedRectangle(b,12);
        auto face=b.reduced(7);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffff4cf),face.getCentreX(),face.getCentreY(),juce::Colour(0xffcfbb8b),face.getRight(),face.getBottom(),true));
        g.fillRoundedRectangle(face,7);
        const float cx=b.getCentreX(), cy=b.getHeight()-28, radius=178;
        auto point=[=](float v,float r) { const float a=v*.95f; return juce::Point<float>{cx+std::sin(a)*r,cy-std::cos(a)*r}; };
        juce::Path arc; arc.addCentredArc(cx,cy,radius,radius,0,-.95f,.95f,true);
        g.setColour(juce::Colour(0xff5a5141)); g.strokePath(arc,juce::PathStrokeType(1));
        for(int i=-10;i<=10;++i) {
            const float v=static_cast<float>(i)/10;
            g.setColour(i<0?juce::Colour(0xffa44f3e):juce::Colour(0xff514a3d));
            g.drawLine({point(v,radius),point(v,radius+(i%5==0?10.f:5.f))},1);
            if(i%5==0) {
                g.setFont(juce::FontOptions(11.f));
                g.drawText(i==0?"0":juce::String(v,1),juce::Rectangle<float>(36,16).withCentre(point(v,radius+22)),juce::Justification::centred);
            }
        }
        g.setColour(juce::Colour(0xff706249)); g.setFont(juce::FontOptions(11.f));
        g.drawText("OUTPUT CORRELATION",0,143,getWidth(),20,juce::Justification::centred);
        g.setFont(juce::FontOptions(10.f));
        g.drawText("OUT OF PHASE",20,183,120,20,juce::Justification::left);
        g.drawText("IN PHASE",getWidth()-140,183,120,20,juce::Justification::right);
        auto tip=point(value,radius+6);
        g.setColour(juce::Colours::black.withAlpha(.16f)); g.drawLine(cx+2,cy+2,tip.x+2,tip.y+2,3);
        g.setColour(juce::Colour(0xffad4234)); g.drawLine(cx,cy,tip.x,tip.y,2);
        g.setColour(juce::Colour(0xff484639)); g.fillEllipse(cx-6,cy-6,12,12);
        g.setColour(juce::Colour(0xffb6aa88)); g.fillEllipse(cx-2,cy-2,4,4);
        juce::Path reflection; reflection.addTriangle(face.getX(),face.getY(),face.getRight(),face.getY(),face.getX(),face.getY()+face.getHeight()*.42f);
        g.setColour(juce::Colours::white.withAlpha(.14f)); g.fillPath(reflection);
    }
private:
    float value=0;
};

class WiderEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit WiderEditor(WiderProcessor& p) : AudioProcessorEditor(p), processor(p) {
        setLookAndFeel(&look);
        const char* ids[]={"width","center","mix"};
        const char* names[]={"Width","Center","Dry / Wet"};
        const char* tips[]={"Side density: -1 to +1. Zero is neutral. Double-click to reset.",
            "Mid density: -1 to +1. Zero is neutral. Double-click to reset.",
            "Original Airwindows mix law. Double-click for 100%."};
        for(size_t i=0;i<3;++i) {
            auto& s=knobs[i]; addAndMakeVisible(s);
            s.setName(names[i]); s.setTooltip(tips[i]);
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,110,26);
            attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);
            s.textFromValueFunction=[i](double v) { return i==2?juce::String(v*100,0)+" %":(v>0?"+":"")+juce::String(v,2); };
            s.valueFromTextFunction=[i](const juce::String& t) { return t.getDoubleValue()/(i==2?100.:1.); };
            s.setDoubleClickReturnValue(true,i==2?1.:0.); s.updateText();
        }
        addAndMakeVisible(meter); meter.setValue(p.correlation.load());
        addAndMakeVisible(reset); reset.setButtonText("RESET TO DEFAULT");
        reset.setColour(juce::TextButton::buttonColourId,style::ink);
        reset.setColour(juce::TextButton::textColourOffId,style::gold);
        reset.onClick=[this] { for(auto* parameter:processor.getParameters()) { parameter->beginChangeGesture(); parameter->setValueNotifyingHost(parameter->getDefaultValue()); parameter->endChangeGesture(); } };
        setSize(840,650); startTimerHz(30);
    }
    ~WiderEditor() override { stopTimer(); setLookAndFeel(nullptr); }
    void resized() override {
        meter.setBounds(190,138,460,246);
        knobs[0].setBounds(68,437,220,160);
        knobs[1].setBounds(310,437,220,160);
        knobs[2].setBounds(552,437,220,160);
        reset.setBounds(630,51,174,30);
    }
    void paint(juce::Graphics& g) override {
        using namespace style;
        g.fillAll(cream); g.setColour(ink); g.setFont(juce::FontOptions(11.f));
        g.drawText("YODO AUDIO    /    AIRWINDOWS DSP",32,20,600,20,juce::Justification::left);
        g.setFont(juce::FontOptions(39.f)); g.drawText("Wider",30,45,300,54,juce::Justification::left);
        g.setFont(juce::FontOptions(12.f)); g.drawText("S T U D I O  /  S T E R E O",240,65,300,22,juce::Justification::left);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343d36),0,110,juce::Colour(0xff1e2722),0,615,false));
        g.fillRoundedRectangle(16,110,808,505,16);
        g.setColour(gold.withAlpha(.2f)); g.drawRoundedRectangle(juce::Rectangle<float>(17,111,806,503),15,1);
        for(float x:{30.f,810.f}) for(float y:{124.f,601.f}) {
            g.setColour(juce::Colour(0xff111a15)); g.fillEllipse(x-4,y-4,8,8);
            g.setColour(juce::Colour(0xff646c5b)); g.drawLine(x-2,y+1,x+2,y-1,1);
        }
        g.setColour(gold); g.setFont(juce::FontOptions(10.f));
        g.drawText("NONLINEAR MID / SIDE SHAPING",190,389,460,20,juce::Justification::centred);
        g.setColour(cream.withAlpha(.12f)); g.drawHorizontalLine(418,44,796);
        const char* labels[]={"WIDTH","CENTER","DRY / WET"};
        const char* subtitles[]={"SIDE DENSITY","MID DENSITY","ORIGINAL BLEND"};
        for(int i=0;i<3;++i) {
            g.setColour(gold); g.setFont(juce::FontOptions(12.f));
            g.drawText(labels[i],68+i*242,428,220,20,juce::Justification::centred);
            g.setColour(muted); g.setFont(juce::FontOptions(9.f));
            g.drawText(subtitles[i],68+i*242,594,220,16,juce::Justification::centred);
        }
        g.setColour(ink.withAlpha(.65f)); g.setFont(juce::FontOptions(10.f));
        g.drawText("Wider by Chris Johnson  /  Independent studio edition",32,623,540,20,juce::Justification::left);
        g.drawText("STEREO  /  FLOAT / DOUBLE",580,623,224,20,juce::Justification::right);
    }
private:
    void timerCallback() override {
        auto seq=processor.meterSequence.load();
        idle=seq==lastSequence?juce::jmin(idle+1,16):0; lastSequence=seq;
        const float target=idle>15?0.f:processor.correlation.load();
        shown+=(target-shown)*.25f; meter.setValue(shown);
    }
    WiderProcessor& processor;
    style::Look look;
    CorrelationMeter meter;
    juce::TextButton reset;
    std::array<juce::Slider,3> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,3> attachments;
    juce::TooltipWindow tooltip{this,600};
    uint32_t lastSequence=0;
    int idle=0;
    float shown=0;
};
juce::AudioProcessorEditor* WiderProcessor::createEditor() { return new WiderEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WiderProcessor; }
