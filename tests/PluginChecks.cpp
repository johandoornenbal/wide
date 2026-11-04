#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    if(p->getParameters().size()!=3 || !p->hasEditor()) return 1;
    p->setRateAndBufferSizeDetails(48000,512); p->prepareToPlay(48000,512);
    p->getParameters()[0]->setValueNotifyingHost(.7f);
    juce::MemoryBlock state; p->getStateInformation(state);
    p->getParameters()[0]->setValueNotifyingHost(.1f);
    p->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    if(std::abs(p->getParameters()[0]->getValue()-.7f)>1e-6f) return 2;
    juce::AudioBuffer<float> audio(2,512);
    for(int ch=0;ch<2;++ch) for(int i=0;i<512;++i) audio.setSample(ch,i,.4f*std::sin(i*.05f));
    juce::MidiBuffer midi; p->processBlock(audio,midi);
    if(!std::isfinite(audio.getMagnitude(0,512)) || audio.getMagnitude(0,512)<.01f) return 3;
    // Exercise both meters with a sustained stereo signal before rendering.
    for(int block=0;block<280;++block) {
        for(int i=0;i<512;++i) {
            const auto phase=static_cast<float>(block*512+i);
            audio.setSample(0,i,.9f*std::sin(phase*.05f));
            audio.setSample(1,i,.22f*std::sin(phase*.031f));
        }
        p->processBlock(audio,midi);
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
    juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true);
    juce::Graphics g(image); editor->paintEntireComponent(g,true);
    juce::File target=juce::File::getCurrentWorkingDirectory().getChildFile("ui-preview.png");
    if(auto out=target.createOutputStream()) {
        out->setPosition(0); out->truncate();
        if(!juce::PNGImageFormat().writeImageToStream(image,*out)) return 8;
    }
    else return 4;
    auto saved=juce::ImageFileFormat::loadFrom(target);
    if(saved.getWidth()!=editor->getWidth() || saved.getHeight()!=editor->getHeight()) return 9;
    if(editor->getWidth()!=840 || editor->getHeight()!=650) return 5;
    int meters=0;
    for(auto* child:editor->getChildren()) {
        if(child->getTitle()=="Output correlation") {
            ++meters;
            if(!editor->getLocalBounds().contains(child->getBounds())) return 6;
        }
    }
    if(meters!=1) return 7;
    int sliders=0;
    for(auto* child:editor->getChildren()) if(auto* slider=dynamic_cast<juce::Slider*>(child)) {
        ++sliders;
        const bool mix=slider->getName()=="Dry / Wet";
        if(std::abs(slider->getMinimum()-(mix?0.:-1.))>1.e-6 || slider->getMaximum()!=1.) return 10;
        slider->setValue(mix?.25:-.4,juce::sendNotificationSync);
        const auto expected=mix?.25f:.3f;
        const int index=slider->getName()=="Width"?0:slider->getName()=="Center"?1:2;
        if(std::abs(p->getParameters()[index]->getValue()-expected)>1.e-6f) return 11;
    }
    if(sliders!=3) return 12;
    for(auto* child:editor->getChildren()) if(auto* button=dynamic_cast<juce::TextButton*>(child)) button->onClick();
    for(auto* parameter:p->getParameters()) if(parameter->getValue()!=parameter->getDefaultValue()) return 13;
    p->setProcessingPrecision(juce::AudioProcessor::doublePrecision);
    p->prepareToPlay(48000,64);
    juce::AudioBuffer<double> doubleAudio(2,64);
    for(int i=0;i<64;++i) { doubleAudio.setSample(0,i,.2); doubleAudio.setSample(1,i,-.1); }
    p->processBlock(doubleAudio,midi);
    if(std::abs(doubleAudio.getSample(0,63)-.3)>1.e-12) return 14;
    editor.reset(); p->releaseResources();
    std::cout<<"Parameter state round-trip, stereo processing, correlation meter and editor render passed\n";
}
