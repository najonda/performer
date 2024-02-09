#include "StochasticSequence.h"
#include "ProjectVersion.h"

#include "ModelUtils.h"
#include "Routing.h"

Types::LayerRange StochasticSequence::layerRange(Layer layer) {
    #define CASE(_layer_) \
    case Layer::_layer_: \
        return { _layer_::Min, _layer_::Max };

    switch (layer) {
    case Layer::Gate:
        return { 0, 1 };
    case Layer::Slide:
        return { 0, 1 }; 
    CASE(GateOffset)
    CASE(GateProbability)
    CASE(Retrigger)
    CASE(RetriggerProbability)
    CASE(Length)
    CASE(LengthVariationRange)
    CASE(LengthVariationProbability)
    CASE(NoteOctave)
    CASE(NoteOctaveProbability)
    CASE(NoteVariationProbability)
    CASE(Condition)
    CASE(StageRepeats)
    CASE(StageRepeatsMode)
    case Layer::Last:
        break;
    }

    #undef CASE

    return { 0, 0 };
}

int StochasticSequence::layerDefaultValue(Layer layer)
{
    StochasticSequence::Step step;

    switch (layer) {
    case Layer::Gate:
        return step.gate();
    case Layer::GateProbability:
        return step.gateProbability();
    case Layer::GateOffset:
        return step.gateOffset();
    case Layer::Slide:
        return step.slide();
    case Layer::Retrigger:
        return step.retrigger();
    case Layer::RetriggerProbability:
        return step.retriggerProbability();
    case Layer::Length:
        return step.length();
    case Layer::LengthVariationRange:
        return step.lengthVariationRange();
    case Layer::LengthVariationProbability:
        return step.lengthVariationProbability();
    case Layer::NoteOctave:
        return step.noteOctave();
    case Layer::NoteOctaveProbability:
        return step.noteOctaveProbability();
    case Layer::NoteVariationProbability:
        return step.noteVariationProbability();
    case Layer::Condition:
        return int(step.condition());
    case Layer::StageRepeats:
        return step.stageRepeats();
    case Layer::StageRepeatsMode:
        return step.stageRepeatMode();
    case Layer::Last:
        break;
    }

    return 0;
}

int StochasticSequence::Step::layerValue(Layer layer) const {
    switch (layer) {
    case Layer::Gate:
        return gate() ? 1 : 0;
    case Layer::Slide:
        return slide() ? 1 : 0;
    case Layer::GateProbability:
        return gateProbability();
    case Layer::GateOffset:
        return gateOffset();
    case Layer::Retrigger:
        return retrigger();
    case Layer::RetriggerProbability:
        return retriggerProbability();
    case Layer::Length:
        return length();
    case Layer::LengthVariationRange:
        return lengthVariationRange();
    case Layer::LengthVariationProbability:
        return lengthVariationProbability();
    case Layer::NoteOctave:
        return noteOctave();
    case Layer::NoteOctaveProbability:
        return noteOctaveProbability();
    case Layer::NoteVariationProbability:
        return noteVariationProbability();
    case Layer::Condition:
        return int(condition());
    case Layer::StageRepeats:
        return stageRepeats();
    case Layer::StageRepeatsMode:
        return stageRepeatMode();
    case Layer::Last:
        break;
    }

    return 0;
}

void StochasticSequence::Step::setLayerValue(Layer layer, int value) {
    switch (layer) {
    case Layer::Gate:
        setGate(value);
        break;
    case Layer::Slide:
        setSlide(value);
        break;
    case Layer::GateProbability:
        setGateProbability(value);
        break;
    case Layer::GateOffset:
        setGateOffset(value);
        break;
    case Layer::Retrigger:
        setRetrigger(value);
        break;
    case Layer::RetriggerProbability:
        setRetriggerProbability(value);
        break;
    case Layer::Length:
        setLength(value);
        break;
    case Layer::LengthVariationRange:
        setLengthVariationRange(value);
        break;
    case Layer::LengthVariationProbability:
        setLengthVariationProbability(value);
        break;
    case Layer::NoteOctave:
        setNoteOctave(value);
        break;
    case Layer::NoteOctaveProbability:
        setNoteOctaveProbability(value);
        break;
    case Layer::NoteVariationProbability:
        setNoteVariationProbability(value);
        break;
    case Layer::Condition:
        setCondition(Types::Condition(value));
        break;
    case Layer::StageRepeats:
        setStageRepeats(value);
        break;
    case Layer::StageRepeatsMode:
        setStageRepeatsMode(static_cast<StochasticSequence::StageRepeatMode>(value));
        break;
    case Layer::Last:
        break;
    }
}

void StochasticSequence::Step::clear() {
    _data0.raw = 0;
    _data1.raw = 1;
    setGate(false);
    setGateProbability(GateProbability::Max);
    setGateOffset(0);
    setSlide(false);
    setBypassScale(true);
    setRetrigger(0);
    setRetriggerProbability(RetriggerProbability::Max);
    setLength(Length::Max / 2);
    setLengthVariationRange(0);
    setLengthVariationProbability(LengthVariationProbability::Max);
    setNote(0);
    setNoteOctave(0);
    setNoteOctaveProbability(NoteOctaveProbability::Min);
    setNoteVariationProbability(NoteVariationProbability::Max);
    setCondition(Types::Condition::Off);
    setStageRepeats(0);
    setStageRepeatsMode(StageRepeatMode::Each);
}

void StochasticSequence::Step::write(VersionedSerializedWriter &writer) const {
    writer.write(_data0.raw);
    writer.write(_data1.raw);
}

void StochasticSequence::Step::read(VersionedSerializedReader &reader) {
    if (reader.dataVersion() < ProjectVersion::Version27) {
        reader.read(_data0.raw);
        reader.readAs<uint16_t>(_data1.raw);
        if (reader.dataVersion() < ProjectVersion::Version5) {
            _data1.raw &= 0x1f;
        }
        if (reader.dataVersion() < ProjectVersion::Version7) {
            setGateOffset(0);
        }
        if (reader.dataVersion() < ProjectVersion::Version12) {
            setCondition(Types::Condition(0));
        }
    } else {
        reader.read(_data0.raw);
        reader.read(_data1.raw);
    }
}

void StochasticSequence::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::Scale:
        setScale(intValue, true);
        break;
    case Routing::Target::RootNote:
        setRootNote(intValue, true);
        break;
    case Routing::Target::Divisor:
        setDivisor(intValue, true);
        break;
    case Routing::Target::RunMode:
        setRunMode(Types::RunMode(intValue), true);
        break;
    case Routing::Target::FirstStep:
        setFirstStep(intValue, true);
        break;
    case Routing::Target::LastStep:
        setLastStep(intValue, true);
        break;
    case Routing::Target::Reseed:
        setReseed(intValue, true);
        break;
    case Routing::Target::RestProbability:
        setRestProbability(intValue, true);
        break;
    case Routing::Target::SequenceFirstStep:
        setSequenceFirstStep(intValue, true);
        break;
    case Routing::Target::SequenceLastStep:
        setSequenceLastStep(intValue, true);
        break;
    case Routing::Target::LowOctaveRange:
        setLowOctaveRange(intValue, true);
        break;
    case Routing::Target::HighOctaveRange:
        setHighOctaveRange(intValue, true);
        break;
    default:
        break;
    }
}

void StochasticSequence::clear() {
    setScale(-1);
    setRootNote(-1);
    setDivisor(12);
    setResetMeasure(0);
    setRunMode(Types::RunMode::Forward);
    setFirstStep(0);
    setLastStep(0);
    setRestProbability(0);
    setSequenceFirstStep(0);
    setSequenceLastStep(15);

    clearSteps();
}

void StochasticSequence::clearSteps() {
    for (auto &step : _steps) {
        step.clear();
    }
    
    for (int i = 0; i < 64; ++i) {
        _steps[i].setGate(false);
        _steps[i].setNoteVariationProbability(0);
        _steps[i].setNote(i);
    }
}

bool StochasticSequence::isEdited() const {
    auto clearStep = Step();
    for (const auto &step : _steps) {
        if (step != clearStep) {
            return true;
        }
    }
    return false;
}

void StochasticSequence::setGates(std::initializer_list<int> gates) {
    size_t step = 0;
    for (auto gate : gates) {
        if (step < _steps.size()) {
            _steps[step++].setGate(gate);
        }
    }
}

void StochasticSequence::setNotes(std::initializer_list<int> notes) {
    size_t step = 0;
    for (auto note : notes) {
        if (step < _steps.size()) {
            _steps[step++].setNote(note);
        }
    }
}

void StochasticSequence::shiftSteps(const std::bitset<CONFIG_STEP_COUNT> &selected, int direction) {
    if (selected.any()) {
        ModelUtils::shiftSteps(_steps, selected, firstStep(), lastStep(), direction);
    } else {
        ModelUtils::shiftSteps(_steps, firstStep(), lastStep(), direction);
    }
}

void StochasticSequence::duplicateSteps() {
    ModelUtils::duplicateSteps(_steps, firstStep(), lastStep());
    setLastStep(lastStep() + (lastStep() - firstStep() + 1));
}

void StochasticSequence::write(VersionedSerializedWriter &writer) const {
    writer.write(_scale.base);
    writer.write(_rootNote.base);
    writer.write(_divisor.base);
    writer.write(_resetMeasure);
    writer.write(_runMode.base);
    writer.write(_firstStep.base);
    writer.write(_lastStep.base);
    writer.write(_restProbability.base);

    writeArray(writer, _steps);
}

void StochasticSequence::read(VersionedSerializedReader &reader) {
    reader.read(_scale.base);
    reader.read(_rootNote.base);
    if (reader.dataVersion() < ProjectVersion::Version10) {
        reader.readAs<uint8_t>(_divisor.base);
    } else {
        reader.read(_divisor.base);
    }
    reader.read(_resetMeasure);
    reader.read(_runMode.base);
    reader.read(_firstStep.base);
    reader.read(_lastStep.base);
    reader.read(_restProbability.base);

    readArray(reader, _steps);
}
