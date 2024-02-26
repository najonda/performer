#include "OverviewPage.h"

#include "model/NoteTrack.h"

#include "ui/painters/WindowPainter.h"
#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"

static void drawNoteTrack(Canvas &canvas, int trackIndex, const NoteTrackEngine &trackEngine, NoteSequence &sequence, bool running, bool patternFollow) {
    canvas.setBlendMode(BlendMode::Set);

    int stepOffset = 16*sequence.section();
    if (patternFollow) {
        stepOffset = (std::max(0, trackEngine.currentStep()) / 16) * 16*sequence.section();
        int section_no = int((trackEngine.currentStep()) / 16);
        sequence.setSecion(section_no);
    }
    int y = trackIndex * 8;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = 68 + i * 8;

        if (trackEngine.currentStep() == stepIndex) {
            canvas.setColor(step.gate() ? Color::Bright : Color::MediumBright);
            canvas.fillRect(x + 1, y + 1, 6, 6);
        } else {
            canvas.setColor(step.gate() ? Color::Medium : Color::Low);
            canvas.fillRect(x + 1, y + 1, 6, 6);
        }
        //if (running) {
            
        //}

        // if (trackEngine.currentStep() == stepIndex) {
        //     canvas.setColor(Color::Bright);
        //     canvas.drawRect(x + 1, y + 1, 6, 6);
        // }
    }
}

static void drawCurve(Canvas &canvas, int x, int y, int w, int h, float &lastY, const Curve::Function function, float min, float max) {
    const int Step = 1;

    auto eval = [=] (float x) {
        return (1.f - (function(x) * (max - min) + min)) * h;
    };

    float fy0 = y + eval(0.f);

    if (lastY >= 0.f && lastY != fy0) {
        canvas.line(x, lastY, x, fy0);
    }

    for (int i = 0; i < w; i += Step) {
        float fy1 = y + eval((float(i) + Step) / w);
        canvas.line(x + i, fy0, x + i + Step, fy1);
        fy0 = fy1;
    }

    lastY = fy0;
}

static void drawStochasticTrack(Canvas &canvas, int trackIndex, const StochasticEngine &trackEngine, const StochasticSequence &sequence) {
    canvas.setBlendMode(BlendMode::Set);

    int stepOffset = (std::max(0, trackEngine.currentStep()) / 12) * 12;
    int y = trackIndex * 8;

    for (int i = 0; i < 12; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = 16 + (68 + i * 8);

        if (trackEngine.currentStep() == stepIndex) {
            canvas.setColor(step.gate() ? Color::Bright : Color::MediumBright);
            canvas.fillRect(x + 1, y + 1, 6, 6);
        } else {
            canvas.setColor(step.gate() ? Color::Medium : Color::Low);
            canvas.fillRect(x + 1, y + 1, 6, 6);
        }

        // if (trackEngine.currentStep() == stepIndex) {
        //     canvas.setColor(Color::Bright);
        //     canvas.drawRect(x + 1, y + 1, 6, 6);
        // }
    }

}

static void drawCurveTrack(Canvas &canvas, int trackIndex, const CurveTrackEngine &trackEngine, const CurveSequence &sequence) {
    canvas.setBlendMode(BlendMode::Add);
    canvas.setColor(Color::MediumBright);

    int stepOffset = (std::max(0, trackEngine.currentStep()) / 16) * 16;
    int y = trackIndex * 8;

    float lastY = -1.f;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);
        float min = step.minNormalized();
        float max = step.maxNormalized();
        const auto function = Curve::function(Curve::Type(std::min(Curve::Last - 1, step.shape())));

        int x = 68 + i * 8;

        drawCurve(canvas, x, y + 1, 8, 6, lastY, function, min, max);
    }

    if (trackEngine.currentStep() >= 0) {
        int x = 64 + ((trackEngine.currentStep() - stepOffset) + trackEngine.currentStepFraction()) * 8;
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Bright);
        canvas.vline(x, y + 1, 7);
    }
}


OverviewPage::OverviewPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

void OverviewPage::enter() {
}

void OverviewPage::exit() {
}

void OverviewPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    canvas.setFont(Font::Tiny);
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Medium);

    canvas.vline(68 - 3, 0, 68);
    canvas.vline(68 - 2, 0, 68);
    canvas.vline(196 + 1, 0, 68);
    canvas.vline(196 + 2, 0, 68);

    for (int trackIndex = 0; trackIndex < 8; trackIndex++) {
        auto &track = _project.track(trackIndex);
        const auto &trackState = _project.playState().trackState(trackIndex);
        const auto &trackEngine = _engine.trackEngine(trackIndex);

        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Medium);

        int y = 5 + trackIndex * 8;

        // track number / pattern number
        canvas.setColor(trackState.mute() ? Color::Medium : Color::Bright);
        switch (track.trackMode()) {
            case Track::TrackMode::Note:
                canvas.drawText(2, y, track.noteTrack().name());
                break;
            case Track::TrackMode::Curve:
                canvas.drawText(2, y, track.curveTrack().name());
                break;
            case Track::TrackMode::MidiCv:
                canvas.drawText(2, y, track.midiCvTrack().name());
                break;
            case Track::TrackMode::Stochastic:
                canvas.drawText(2, y, track.stochasticTrack().name());
                break;
            default:
                break;
        }  

        std::string s = std::to_string(trackState.pattern() + 1);
        char const *pchar = s.c_str(); 
        char const p[] = {'P'};

        canvas.fillRect(46 - 1, y - 5, canvas.textWidth(p)+canvas.textWidth(pchar) + 1, 7);
        canvas.setBlendMode(BlendMode::Sub);
        canvas.drawText(46, y, FixedStringBuilder<8>("P%d", trackState.pattern() + 1));
        canvas.setBlendMode(BlendMode::Set);

        bool gate = _engine.gateOutput() & (1 << trackIndex);
        canvas.setColor(gate ? Color::Bright : Color::Medium);
        canvas.fillRect(256 - 48 + 1, trackIndex * 8 + 1, 6, 6);

        // cv output
        canvas.setColor(Color::Bright);
        canvas.drawText(256 - 32, y, FixedStringBuilder<8>("%.2fV", _engine.cvOutput().channel(trackIndex)));

        switch (track.trackMode()) {
        case Track::TrackMode::Note: {
                bool patterFolow = false;
                if (track.noteTrack().patternFollow()==Types::PatternFollow::Display || track.noteTrack().patternFollow()==Types::PatternFollow::DispAndLP) {
                    patterFolow = true;
                    canvas.drawText(256 - 54, y, FixedStringBuilder<8>("F"));
                }
                drawNoteTrack(canvas, trackIndex, trackEngine.as<NoteTrackEngine>(), track.noteTrack().sequence(trackState.pattern()), _engine.state().running(), patterFolow);    
            }
            break;
        case Track::TrackMode::Curve:
            drawCurveTrack(canvas, trackIndex, trackEngine.as<CurveTrackEngine>(), track.curveTrack().sequence(trackState.pattern()));
            break;
        case Track::TrackMode::Stochastic:
            drawStochasticTrack(canvas, trackIndex, trackEngine.as<StochasticEngine>(), track.stochasticTrack().sequence(trackState.pattern()));
            break;
        case Track::TrackMode::MidiCv:
            break;
        case Track::TrackMode::Last:
            break;
        }
    }



    if (!_stepSelection.any() || os::ticks() > _showDetailTicks + os::time::ms(500)) {
        _showDetail = false;
    }
    if (_showDetail) {
        
        auto track = _project.selectedTrack();
        switch (track.trackMode()) {
            case Track::TrackMode::Note: {
                    auto &sequence = _project.selectedNoteSequence();
                    drawDetail(canvas, sequence.step(_stepSelection.first()));
                }
                break;
            case Track::TrackMode::Stochastic: {
                    auto &sequence = _project.selectedStochasticSequence();
                    drawStochasticDetail(canvas, sequence.step(_stepSelection.first()));
                }
                break;
            default:
                break;
        }
        
    }

}

void OverviewPage::updateLeds(Leds &leds) {

    switch (_project.selectedTrack().trackMode()) {

        case Track::TrackMode::Note: {
            const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
            auto &sequence = _project.selectedNoteSequence();
            int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

            for (int i = 0; i < 16; ++i) {
                int stepIndex = stepOffset() + i;
                bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
                bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
                leds.set(MatrixMap::fromStep(i), red, green);
            }

            LedPainter::drawSelectedSequenceSection(leds, sequence.section());
            }
            break;
        case Track::TrackMode::Stochastic: {
            const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
            auto &sequence = _project.selectedStochasticSequence();
            int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

            for (int i = 0; i < 16; ++i) {
                int stepIndex = stepOffset() + i;
                bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
                bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
                leds.set(MatrixMap::fromStep(i), red, green);
            }

            LedPainter::drawSelectedSequenceSection(leds, 0);
            }
            break;
        default:
            break;
    }
}

void OverviewPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, stepOffset());
}

void OverviewPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, stepOffset());
}

void OverviewPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    

    if (key.isGlobal()) {
        return;
    }

    if (key.isQuickEdit()) {
        if (_project.selectedTrack().trackMode() == Track::TrackMode::Note) {
            auto &track = _project.selectedTrack().noteTrack();
            if (key.is(Key::Step15)) {
                bool lpConnected = _engine.isLaunchpadConnected();
                track.togglePatternFollowDisplay(lpConnected);
            }
        }
    }

        if (key.pageModifier()) {
        return;
    }

    _stepSelection.keyPress(event, stepOffset());
     auto &track = _project.selectedTrack();
    if (key.isStep()) {
        
        switch (track.trackMode()) {
            case Track::TrackMode::Note: {
                    
                    auto &sequence = _project.selectedNoteSequence();
                    int stepIndex = stepOffset() + key.step();
                    sequence.step(stepIndex).toggleGate();
                    event.consume();
                }
                break;
            case Track::TrackMode::Stochastic: {
                    int stepIndex = stepOffset() + key.step();
                    auto &sequence = _project.selectedStochasticSequence();
                    sequence.step(stepIndex).toggleGate();
                    event.consume();
                }
                break;
            default:
                break;
        }
        
    }

     if (key.isLeft()) {
        switch (track.trackMode()) {
            case Track::TrackMode::Note: {
                auto &sequence = _project.selectedNoteSequence();
                sequence.setSecion(std::max(0, sequence.section() - 1));
                 track.noteTrack().setPatternFollowDisplay(false);
                break;
            }
            default:
                break;        
        }

        event.consume();
    }
    if (key.isRight()) {
        switch (track.trackMode()) {
            case Track::TrackMode::Note: {
                auto &sequence = _project.selectedNoteSequence();
                sequence.setSecion(std::min(3, sequence.section() + 1));
                track.noteTrack().setPatternFollowDisplay(false);
                break;
            }
            default:
                break;        
        }
        
        event.consume();
    }
}

void OverviewPage::encoder(EncoderEvent &event) {

    auto &track = _project.selectedTrack();

    if (!_stepSelection.any()) {
        return;
    }

    _showDetail = true;
    _showDetailTicks = os::ticks();

    switch (track.trackMode()) {

        case Track::TrackMode::Note: {
                auto &sequence = _project.selectedNoteSequence();
                const auto &scale = sequence.selectedScale(_project.scale());
                for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
                    if (_stepSelection[stepIndex]) {
                        auto &step = sequence.step(stepIndex);
                        bool shift = globalKeyState()[Key::Shift];
                        step.setNote(step.note() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                        updateMonitorStep();
                    }
                }
            }   
            break;
        case Track::TrackMode::Stochastic: {
             auto &sequence = _project.selectedStochasticSequence();
                for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
                    if (_stepSelection[stepIndex]) {
                        auto &step = sequence.step(stepIndex);
                        step.setNoteVariationProbability(step.noteVariationProbability() + event.value());
                    }
                }
        }
        default:
            break;
        }

    event.consume();
}

void OverviewPage::drawDetail(Canvas &canvas, const NoteSequence::Step &step) {
    const auto &sequence = _project.selectedNoteSequence();
    const auto &scale = sequence.selectedScale(_project.scale());

    FixedStringBuilder<16> str;

    WindowPainter::drawFrame(canvas, 64, 16, 128, 32);

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    canvas.vline(64 + 32, 16, 32);

    canvas.setFont(Font::Small);
    str("%d", _stepSelection.first() + 1);
    if (_stepSelection.count() > 1) {
        str("*");
    }
    canvas.drawTextCentered(64, 16, 32, 32, str);

    canvas.setFont(Font::Tiny);

    str.reset();
    scale.noteName(str, step.note(), sequence.selectedRootNote(_model.project().rootNote()), Scale::Long);
    canvas.setFont(Font::Small);
    canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
    canvas.setFont(Font::Tiny);

}


void OverviewPage::drawStochasticDetail(Canvas &canvas, const StochasticSequence::Step &step) {
    FixedStringBuilder<16> str;

    WindowPainter::drawFrame(canvas, 64, 16, 128, 32);

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    canvas.vline(64 + 32, 16, 32);

    canvas.setFont(Font::Small);
    str("%d", _stepSelection.first() + 1);
    if (_stepSelection.count() > 1) {
        str("*");
    }
    canvas.drawTextCentered(64, 16, 32, 32, str);

    canvas.setFont(Font::Tiny);

    str.reset();
    SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.noteVariationProbability() + 1, StochasticSequence::NoteVariationProbability::Range
        );
        str.reset();
    str("%.1f%%", 100.f * (step.noteVariationProbability()) / (StochasticSequence::NoteVariationProbability::Range -1));
    canvas.setColor(Color::Bright);
    canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
    canvas.setFont(Font::Tiny);


}

void OverviewPage::updateMonitorStep() {
    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    // TODO should we monitor an all layers not just note?
    if (_stepSelection.any()) {
        trackEngine.setMonitorStep(_stepSelection.first());
    }
}
