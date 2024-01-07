#include "StochasticSequenceEditPage.h"

#include "Pages.h"

#include "model/StochasticSequence.h"
#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "model/Scale.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,    Generate,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
    { "GEN" },
};

enum class Function {
    Gate        = 0,
    Retrigger   = 1,
    Length      = 2,
    Note        = 3,
    Condition     = 4,
};

static const char *functionNames[] = { "GATE", "RETRIG", "LENGTH", "NOTE", "COND" };

static const StochasticSequenceListModel::Item quickEditItems[8] = {
    StochasticSequenceListModel::Item::FirstStep,
    StochasticSequenceListModel::Item::LastStep,
    StochasticSequenceListModel::Item::RunMode,
    StochasticSequenceListModel::Item::Divisor,
    StochasticSequenceListModel::Item::ResetMeasure,
    StochasticSequenceListModel::Item::Scale,
    StochasticSequenceListModel::Item::RootNote,
    StochasticSequenceListModel::Item::Last
};

StochasticSequenceEditPage::StochasticSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        auto layer = _project.selectedStochasticSequenceLayer();
        const auto &sequence = _project.selectedStochasticSequence();
        return sequence.step(a).layerValue(layer) == sequence.step(b).layerValue(layer);
    });
}

void StochasticSequenceEditPage::enter() {
    updateMonitorStep();

    _showDetail = false;
}

void StochasticSequenceEditPage::exit() {
    _engine.selectedTrackEngine().as<StochasticEngine>().setMonitorStep(-1);
}

void StochasticSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    /* Prepare flags shown before mode name (top right header) */
    const char *mode_flags = NULL;
    if (_sectionTracking) {
        const char *st_flag = "F";
        mode_flags = st_flag;
    }

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", mode_flags);

    WindowPainter::drawActiveFunction(canvas, StochasticSequence::layerName(layer()));
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
    const auto &sequence = _project.selectedStochasticSequence();
    const auto &scale = sequence.selectedScale(_project.scale());
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    int currentRecordStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentRecordStep() : -1;

    const int stepWidth = Width / StepCount;
    const int stepOffset = this->stepOffset();

    const int loopY = 16;


    // Track Pattern Section on the UI
    if (_sectionTracking && _engine.state().running()) {
        bool section_change = bool((currentStep) % StepCount == 0); // StepCount is relative to screen
        int section_no = int((currentStep) / StepCount);
        if (section_change && section_no != _section) {
            _section = section_no;
        }
    }

    // draw loop points
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    SequencePainter::drawLoopStart(canvas, (sequence.firstStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);
    SequencePainter::drawLoopEnd(canvas, (sequence.lastStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);

    for (int i = 0; i < 12; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = i * stepWidth;
        int y = 20;

        // loop
        if (stepIndex > sequence.firstStep() && stepIndex <= sequence.lastStep()) {
            canvas.setColor(Color::Bright);
            canvas.point(x, loopY);
        }

        // step index
        {
            canvas.setColor(_stepSelection[stepIndex] ? Color::Bright : Color::Medium);
            FixedStringBuilder<8> str("%d", stepIndex + 1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y - 2, str);
        }

        // step gate
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (step.gate()) {
            canvas.setColor(_context.model.settings().userSettings().get<DimSequenceSetting>(SettingDimSequence)->getValue() ? Color::Low : Color::Bright);
            canvas.fillRect(x + 4, y + 4, stepWidth - 8, stepWidth - 8);
        }

        // record step
        if (stepIndex == currentRecordStep) {
            // draw circle
            canvas.setColor(step.gate() ? Color::None : Color::Bright);
            canvas.fillRect(x + 6, y + 6, stepWidth - 12, stepWidth - 12);
            canvas.setColor(Color::Medium);
            canvas.hline(x + 7, y + 5, 2);
            canvas.hline(x + 7, y + 10, 2);
            canvas.vline(x + 5, y + 7, 2);
            canvas.vline(x + 10, y + 7, 2);
        }

        switch (layer()) {
        case Layer::Gate:
            break;
        case Layer::GateProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateProbability() + 1, StochasticSequence::GateProbability::Range
            );
            break;
        case Layer::GateOffset:
            SequencePainter::drawOffset(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.gateOffset(), StochasticSequence::GateOffset::Min - 1, StochasticSequence::GateOffset::Max + 1
            );
            break;
        case Layer::Retrigger:
            SequencePainter::drawRetrigger(
                canvas,
                x, y + 18, stepWidth, 2,
                step.retrigger() + 1, StochasticSequence::Retrigger::Range
            );
            break;
        case Layer::RetriggerProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.retriggerProbability() + 1, StochasticSequence::RetriggerProbability::Range
            );
            break;
        case Layer::Length:
            SequencePainter::drawLength(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, StochasticSequence::Length::Range
            );
            break;
        case Layer::LengthVariationRange:
            SequencePainter::drawLengthRange(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.length() + 1, step.lengthVariationRange(), StochasticSequence::Length::Range
            );
            break;
        case Layer::LengthVariationProbability:
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.lengthVariationProbability() + 1, StochasticSequence::LengthVariationProbability::Range
            );
            break;
        case Layer::Note: {
            int rootNote = sequence.selectedRootNote(_model.project().rootNote());
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            str.reset();
            scale.noteName(str, step.note(), rootNote, Scale::Short2);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::NoteVariationRange: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str("%d", step.noteVariationRange());
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::NoteVariationProbability: {
            SequencePainter::drawProbability(
                canvas,
                x + 2, y + 18, stepWidth - 4, 2,
                step.noteVariationProbability() + 1, StochasticSequence::NoteVariationProbability::Range
            );
            int rootNote = sequence.selectedRootNote(_model.project().rootNote());
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;
            scale.noteName(str, step.note(), rootNote, Scale::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::Slide:
            SequencePainter::drawSlide(
                canvas,
                x + 4, y + 18, stepWidth - 8, 4,
                step.slide()
            );
            break;
        case Layer::Condition: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;
            Types::printCondition(str, step.condition(), Types::ConditionFormat::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            str.reset();
            Types::printCondition(str, step.condition(), Types::ConditionFormat::Short2);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::StageRepeats: {
            canvas.setColor(Bright);
            FixedStringBuilder<8> str("x%d", step.stageRepeats()+1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::StageRepeatsMode: {
            SequencePainter::drawStageRepeatMode(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.stageRepeatMode()
            );
            break;
        }
        case Layer::Last:
            break;
        }
    }

    // handle detail display

    if (_showDetail) {
        if (layer() == Layer::Gate || layer() == Layer::Slide || _stepSelection.none()) {
            _showDetail = false;
        }
        if (_stepSelection.isPersisted() && os::ticks() > _showDetailTicks + os::time::ms(500)) {
            _showDetail = false;
        }
    }

    if (_showDetail) {
        drawDetail(canvas, sequence.step(_stepSelection.first()));
    }



}

void StochasticSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
    const auto &sequence = _project.selectedStochasticSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset() + i;
        bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
        bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
        leds.set(MatrixMap::fromStep(i), red, green);
    }

    LedPainter::drawSelectedSequenceSection(leds, _section);

    // show quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != StochasticSequenceListModel::Item::Last);
            leds.mask(index);
        }
        int index = MatrixMap::fromStep(15);
        leds.unmask(index);
        leds.set(index, false, true);
        leds.mask(index);
    }
}

void StochasticSequenceEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, stepOffset());
    updateMonitorStep();
}

void StochasticSequenceEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, stepOffset());
    updateMonitorStep();
}

void StochasticSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedStochasticSequence();

    if (key.isQuickEdit()) {
        if (key.is(Key::Step15)) {
            tieNotes();
        }
    }

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }

    if (key.isQuickEdit()) {
        quickEdit(key.quickEdit());
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        // XXX Added here, but should we move it to pageModifier structure?
        if (key.is(Key::Step5)) {
            setSectionTracking(not _sectionTracking);
            event.consume();
        }

        if (key.is(Key::Step4)) {
            sequence.setReseed(true);
            event.consume();
        }
        return;
    }

    _stepSelection.keyPress(event, stepOffset());
    updateMonitorStep();

    if (!key.shiftModifier() && key.isStep()) {
        int stepIndex = stepOffset() + key.step();
        switch (layer()) {
        case Layer::Gate:
            sequence.step(stepIndex).toggleGate();
            event.consume();
            break;
        default:
            break;
        }
    }

    KeyPressEvent keyPressEvent =_keyPressEventTracker.process(key);

    if (!key.shiftModifier() && key.isStep() && keyPressEvent.count() == 2) {
        int stepIndex = stepOffset() + key.step();
        if (layer() != Layer::Gate) {
            sequence.step(stepIndex).toggleGate();
            event.consume();
        }
    }

    if (key.isFunction()) {
        switchLayer(key.function(), key.shiftModifier());
        event.consume();
    }

    if (key.isEncoder()) {
        setSectionTracking(false);

        if (!_showDetail && _stepSelection.any() && allSelectedStepsActive()) {
            setSelectedStepsGate(false);
        } else {
            setSelectedStepsGate(true);
        }
    }


    if (key.isLeft()) {
        if (key.shiftModifier()) {
            sequence.shiftSteps(_stepSelection.selected(), -1);
        } else {
            setSectionTracking(false);
            _section = std::max(0, _section - 1);
        }
        event.consume();
    }
    if (key.isRight()) {
        if (key.shiftModifier()) {
            sequence.shiftSteps(_stepSelection.selected(), 1);
        } else {
            setSectionTracking(false);
            _section = std::min(3, _section + 1);
        }
        event.consume();
    }
}

void StochasticSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedStochasticSequence();
    const auto &scale = sequence.selectedScale(_project.scale());

    if (!_stepSelection.any())
    {
        switch (layer())
        {
        case Layer::Gate:
            setLayer(event.value() > 0 ? Layer::GateOffset : Layer::GateProbability);
            break;
        case Layer::GateOffset:
            setLayer(event.value() > 0 ? Layer::GateProbability : Layer::Gate);
            break;
        case Layer::GateProbability:
            setLayer(event.value() > 0 ? Layer::Gate : Layer::GateOffset);
            break;
        case Layer::Retrigger:
            setLayer(event.value() > 0 ? Layer::RetriggerProbability : Layer::StageRepeatsMode);
            break;
        case Layer::RetriggerProbability:
            setLayer(event.value() > 0 ? Layer::StageRepeats : Layer::Retrigger);
            break;
        case Layer::StageRepeats:
            setLayer(event.value() > 0 ? Layer::StageRepeatsMode : Layer::RetriggerProbability);
            break;
        case Layer::StageRepeatsMode:
            setLayer(event.value() > 0 ? Layer::Retrigger : Layer::StageRepeats);
            break;
        case Layer::Length:
            setLayer(event.value() > 0 ? Layer::LengthVariationRange : Layer::LengthVariationProbability);
            break;
        case Layer::LengthVariationRange:
            setLayer(event.value() > 0 ? Layer::LengthVariationProbability : Layer::Length);
            break;
        case Layer::LengthVariationProbability:
            setLayer(event.value() > 0 ? Layer::Length : Layer::LengthVariationRange);
            break;
        case Layer::Note:
            setLayer(event.value() > 0 ? Layer::NoteVariationRange : Layer::Slide);
            break;
        case Layer::NoteVariationRange:
            setLayer(event.value() > 0 ? Layer::NoteVariationProbability : Layer::Note);
            break;
        case Layer::NoteVariationProbability:
            setLayer(event.value() > 0 ? Layer::Slide : Layer::NoteVariationRange);
            break;
        case Layer::Slide:
            setLayer(event.value() > 0 ? Layer::Note : Layer::NoteVariationProbability);
            break;
        default:
            break;
        }
        return;
    }
    else
    {
        _showDetail = true;
        _showDetailTicks = os::ticks();
    }

    for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            auto &step = sequence.step(stepIndex);
            bool shift = globalKeyState()[Key::Shift];
            switch (layer()) {
            case Layer::Gate:
                step.setGate(event.value() > 0);
                break;
            case Layer::GateProbability:
                step.setGateProbability(step.gateProbability() + event.value());
                break;
            case Layer::GateOffset:
                step.setGateOffset(step.gateOffset() + event.value());
                break;
            case Layer::Retrigger:
                step.setRetrigger(step.retrigger() + event.value());
                break;
            case Layer::RetriggerProbability:
                step.setRetriggerProbability(step.retriggerProbability() + event.value());
                break;
            case Layer::Length:
                step.setLength(step.length() + event.value());
                break;
            case Layer::LengthVariationRange:
                step.setLengthVariationRange(step.lengthVariationRange() + event.value());
                break;
            case Layer::LengthVariationProbability:
                step.setLengthVariationProbability(step.lengthVariationProbability() + event.value());
                break;
            case Layer::Note:
                step.setNote(step.note() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                updateMonitorStep();
                break;
            case Layer::NoteVariationRange:
                step.setNoteVariationRange(step.noteVariationRange() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                updateMonitorStep();
                break;
            case Layer::NoteVariationProbability:
                step.setNoteVariationProbability(step.noteVariationProbability() + event.value());
                break;
            case Layer::Slide:
                step.setSlide(event.value() > 0);
                break;
            case Layer::Condition:
                step.setCondition(ModelUtils::adjustedEnum(step.condition(), event.value()));
                break;
            case Layer::StageRepeats:
                step.setStageRepeats(step.stageRepeats() + event.value());
                break;
            case Layer::StageRepeatsMode:
                step.setStageRepeatsMode(
                    static_cast<StochasticSequence::StageRepeatMode>(
                        step.stageRepeatMode() + event.value()
                    )
                );
                break;
            case Layer::Last:
                break;
            }
        }
    }

    event.consume();
}

void StochasticSequenceEditPage::midi(MidiEvent &event) {
    if (!_engine.recording() && layer() == Layer::Note && _stepSelection.any()) {
        auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
        auto &sequence = _project.selectedStochasticSequence();
        const auto &scale = sequence.selectedScale(_project.scale());
        const auto &message = event.message();

        if (message.isNoteOn()) {
            float volts = (message.note() - 60) * (1.f / 12.f);
            int note = scale.noteFromVolts(volts);

            for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
                if (_stepSelection[stepIndex]) {
                    auto &step = sequence.step(stepIndex);
                    step.setNote(note);
                    step.setGate(true);
                }
            }

            trackEngine.setMonitorStep(_stepSelection.first());
            updateMonitorStep();
        }
    }
}

void StochasticSequenceEditPage::switchLayer(int functionKey, bool shift) {
    if (shift) {
        switch (Function(functionKey)) {
        case Function::Gate:
            setLayer(Layer::GateProbability);
            break;
        case Function::Retrigger:
            setLayer(Layer::StageRepeats);
            break;
        case Function::Length:
            setLayer(Layer::StageRepeatsMode);
            break;
        case Function::Note:
            setLayer(Layer::Slide);
            break;
        case Function::Condition:
            setLayer(Layer::Condition);
            break;
        }
        return;
    }

    switch (Function(functionKey)) {
    case Function::Gate:
        switch (layer()) {
        case Layer::GateProbability:
            setLayer(Layer::GateOffset);
            break;
        case Layer::GateOffset:
            setLayer(Layer::GateProbability);
            break;
        default:
            setLayer(Layer::GateProbability);
            break;
        }
        break;
    case Function::Retrigger:
        switch (layer()) {
        case Layer::Retrigger:
            setLayer(Layer::RetriggerProbability);
            break;
        case Layer::RetriggerProbability:
            setLayer(Layer::StageRepeats);
            break;
        case Layer::StageRepeats:
            setLayer(Layer::StageRepeatsMode);
            break;
        default:
            setLayer(Layer::Retrigger);
            break;
        }
        break;
    case Function::Length:
        switch (layer()) {
        case Layer::Length:
            setLayer(Layer::LengthVariationRange);
            break;
        case Layer::LengthVariationRange:
            setLayer(Layer::LengthVariationProbability);
            break;
        default:
            setLayer(Layer::Length);
            break;
        }
        break;
    case Function::Note:
        switch (layer()) {
        case Layer::Note:
            //setLayer(Layer::NoteVariationRange);
            break;
        case Layer::NoteVariationRange:
            //setLayer(Layer::NoteVariationProbability);
            break;
        case Layer::NoteVariationProbability:
            setLayer(Layer::Slide);
            break;
        default:
            setLayer(Layer::NoteVariationProbability);
            break;
        }
        break;
    case Function::Condition:
        setLayer(Layer::Condition);
        break;
    }
}

int StochasticSequenceEditPage::activeFunctionKey() {
    switch (layer()) {
    case Layer::Gate:
    case Layer::GateProbability:
    case Layer::GateOffset:
        return 0;
    case Layer::Retrigger:
    case Layer::RetriggerProbability:
    case Layer::StageRepeats:
    case Layer::StageRepeatsMode:
        return 1;
    case Layer::Length:
    case Layer::LengthVariationRange:
    case Layer::LengthVariationProbability:
        return 2;
    case Layer::Note:
    case Layer::NoteVariationRange:
    case Layer::NoteVariationProbability:
    case Layer::Slide:
        return 3;
    case Layer::Condition:
        return 4;
    case Layer::Last:
        break;
    }

    return -1;
}

void StochasticSequenceEditPage::updateMonitorStep() {
    auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();

    // TODO should we monitor an all layers not just note?
    if (layer() == Layer::Note && !_stepSelection.isPersisted() && _stepSelection.any()) {
        trackEngine.setMonitorStep(_stepSelection.first());
    } else {
        trackEngine.setMonitorStep(-1);
    }
}

void StochasticSequenceEditPage::drawDetail(Canvas &canvas, const StochasticSequence::Step &step) {

    const auto &sequence = _project.selectedStochasticSequence();
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

    switch (layer()) {
    case Layer::Gate:
    case Layer::Slide:
        break;
    case Layer::GateProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateProbability() + 1, StochasticSequence::GateProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.gateProbability() + 1.f) / StochasticSequence::GateProbability::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::GateOffset:
        SequencePainter::drawOffset(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.gateOffset(), StochasticSequence::GateOffset::Min - 1, StochasticSequence::GateOffset::Max + 1
        );
        str.reset();
        str("%.1f%%", 100.f * step.gateOffset() / float(StochasticSequence::GateOffset::Max + 1));
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Retrigger:
        SequencePainter::drawRetrigger(
            canvas,
            64+ 32 + 8, 32 - 4, 64 - 16, 8,
            step.retrigger() + 1, StochasticSequence::Retrigger::Range
        );
        str.reset();
        str("%d", step.retrigger() + 1);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::RetriggerProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.retriggerProbability() + 1, StochasticSequence::RetriggerProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.retriggerProbability() + 1.f) / StochasticSequence::RetriggerProbability::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Length:
        SequencePainter::drawLength(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, StochasticSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.length() + 1.f) / StochasticSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationRange:
        SequencePainter::drawLengthRange(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.length() + 1, step.lengthVariationRange(), StochasticSequence::Length::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationRange()) / StochasticSequence::Length::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::LengthVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.lengthVariationProbability() + 1, StochasticSequence::LengthVariationProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.lengthVariationProbability() + 1.f) / StochasticSequence::LengthVariationProbability::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Note:
        str.reset();
        scale.noteName(str, step.note(), sequence.selectedRootNote(_model.project().rootNote()), Scale::Long);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::NoteVariationRange:
        str.reset();
        str("%d", step.noteVariationRange());
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::NoteVariationProbability:
        SequencePainter::drawProbability(
            canvas,
            64 + 32 + 8, 32 - 4, 64 - 16, 8,
            step.noteVariationProbability() + 1, StochasticSequence::NoteVariationProbability::Range
        );
        str.reset();
        str("%.1f%%", 100.f * (step.noteVariationProbability() + 1.f) / StochasticSequence::NoteVariationProbability::Range);
        canvas.setColor(Color::Bright);
        canvas.drawTextCentered(64 + 32 + 64, 32 - 4, 32, 8, str);
        break;
    case Layer::Condition:
        str.reset();
        Types::printCondition(str, step.condition(), Types::ConditionFormat::Long);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 96, 32, str);
        break;
    case Layer::StageRepeats:
        str.reset();
        str("x%d", step.stageRepeats()+1);
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
     case Layer::StageRepeatsMode:
        str.reset();
        switch (step.stageRepeatMode()) {
            case StochasticSequence::Each:
                str("EACH");
                break;
            case StochasticSequence::First:
                str("FIRST");
                break;
            case StochasticSequence::Middle:
                str("MIDDLE");
                break;
            case StochasticSequence::Last:
                str("LAST");
                break;
            case StochasticSequence::Odd:
                str("ODD");
                break;
            case StochasticSequence::Even:
                str("EVEN");
                break;
            case StochasticSequence::Triplets:
                str("TRIPLET");
                break;
            case StochasticSequence::Random:
                str("RANDOM");
                break;

            default:
                break;
        }
        canvas.setFont(Font::Small);
        canvas.drawTextCentered(64 + 32, 16, 64, 32, str);
        break;
    case Layer::Last:
        break;
    }
}

void StochasticSequenceEditPage::contextShow() {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }
    ));
}

void StochasticSequenceEditPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::Init:
        initSequence();
        break;
    case ContextAction::Copy:
        copySequence();
        break;
    case ContextAction::Paste:
        pasteSequence();
        break;
    case ContextAction::Duplicate:
        duplicateSequence();
        break;
    case ContextAction::Generate:
        generateSequence();
        break;
    case ContextAction::Last:
        break;
    }
}

bool StochasticSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteNoteSequenceSteps();
    default:
        return true;
    }
}

void StochasticSequenceEditPage::initSequence() {
    _project.selectedStochasticSequence().clearSteps();
    showMessage("STEPS INITIALIZED");
}

void StochasticSequenceEditPage::copySequence() {
    _model.clipBoard().copyNoteSequenceSteps(_project.selectedNoteSequence(), _stepSelection.selected());
    showMessage("STEPS COPIED");
}

void StochasticSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteNoteSequenceSteps(_project.selectedNoteSequence(), _stepSelection.selected());
    showMessage("STEPS PASTED");
}

void StochasticSequenceEditPage::duplicateSequence() {
    _project.selectedStochasticSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}

/*
 * Makes the UI track the current step section
 */
void StochasticSequenceEditPage::setSectionTracking(bool track) {
    _sectionTracking = track;
}

void StochasticSequenceEditPage::tieNotes() {

    auto &sequence = _project.selectedStochasticSequence();

    if (_stepSelection.any()) {
        int first=-1;
        int last=-1;

        for (size_t i = 0; i < sequence.steps().size(); ++i) {
            if (_stepSelection[i]) {
                if (first == -1 ) {
                    first = i;
                }
                last = i;
            }
        }

        for (int i = first; i <= last; i++) {
            sequence.step(i).setGate(true);
            if (i != last) {
                sequence.step(i).setLength(StochasticSequence::Length::Max);
                showMessage("NOTES TIED");
            }
            sequence.step(i).setNote(sequence.step(first).note());
            std::cerr << _stepSelection[i];
        }
    }
}

void StochasticSequenceEditPage::generateSequence() {
    _manager.pages().generatorSelect.show([this] (bool success, Generator::Mode mode) {
        if (success) {
            auto builder = _builderContainer.create<StochasticSequenceBuilder>(_project.selectedStochasticSequence(), layer());
            auto generator = Generator::execute(mode, *builder);
            if (generator) {
                _manager.pages().generator.show(generator);
            }
        }
    });
}

void StochasticSequenceEditPage::quickEdit(int index) {
    _listModel.setSequence(&_project.selectedStochasticSequence());
    if (quickEditItems[index] != StochasticSequenceListModel::Item::Last) {
        _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
    }
}

bool StochasticSequenceEditPage::allSelectedStepsActive() const {
    const auto &sequence = _project.selectedStochasticSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex] && !sequence.step(stepIndex).gate()) {
            return false;
        }
    }
    return true;
}

void StochasticSequenceEditPage::setSelectedStepsGate(bool gate) {
    auto &sequence = _project.selectedStochasticSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            sequence.step(stepIndex).setGate(gate);
        }
    }
}