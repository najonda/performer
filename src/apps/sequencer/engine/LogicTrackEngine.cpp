#include "LogicTrackEngine.h"

#include "Engine.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/LogicSequence.h"
#include "model/Scale.h"
#include "ui/MatrixMap.h"
#include <climits>
#include <iostream>
#include <ctime>

static Random rng;

// evaluate if step gate is active
static bool evalStepGate(const LogicSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.gateProbability() + probabilityBias, -1, LogicSequence::GateProbability::Max);
    return step.gate() && int(rng.nextRange(LogicSequence::GateProbability::Range)) <= probability;
}

// evaluate step condition
static bool evalStepCondition(const LogicSequence::Step &step, int iteration, bool fill, bool &prevCondition) {
    auto condition = step.condition();
    switch (condition) {
    case Types::Condition::Off:                                         return true;
    case Types::Condition::Fill:        prevCondition = fill;           return prevCondition;
    case Types::Condition::NotFill:     prevCondition = !fill;          return prevCondition;
    case Types::Condition::Pre:                                         return prevCondition;
    case Types::Condition::NotPre:                                      return !prevCondition;
    case Types::Condition::First:       prevCondition = iteration == 0; return prevCondition;
    case Types::Condition::NotFirst:    prevCondition = iteration != 0; return prevCondition;
    default:
        int index = int(condition);
        if (index >= int(Types::Condition::Loop) && index < int(Types::Condition::Last)) {
            auto loop = Types::conditionLoop(condition);
            prevCondition = iteration % loop.base == loop.offset;
            if (loop.invert) prevCondition = !prevCondition;
            return prevCondition;
        }
    }
    return true;
}

// evaluate step retrigger count
static int evalStepRetrigger(const LogicSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.retriggerProbability() + probabilityBias, -1, LogicSequence::RetriggerProbability::Max);
    return int(rng.nextRange(LogicSequence::RetriggerProbability::Range)) <= probability ? step.retrigger() + 1 : 1;
}

// evaluate step length
static int evalStepLength(const LogicSequence::Step &step, int lengthBias) {
    int length = LogicSequence::Length::clamp(step.length() + lengthBias) + 1;
    int probability = step.lengthVariationProbability();
    if (int(rng.nextRange(LogicSequence::LengthVariationProbability::Range)) <= probability) {
        int offset = step.lengthVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.lengthVariationRange()) + 1);
        if (step.lengthVariationRange() < 0) {
            offset = -offset;
        }
        length = clamp(length + offset, 0, LogicSequence::Length::Range);
    }
    return length;
}

// evaluate transposition
static int evalTransposition(const Scale &scale, int octave, int transpose) {
    return octave * scale.notesPerOctave() + transpose;
}

// evaluate note voltage
static float evalStepNote(const LogicSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, bool useVariation = true) {


    if (step.bypassScale()) {
        const Scale &bypassScale = Scale::get(0);
        int note = step.note() + evalTransposition(bypassScale, octave, transpose);
        int probability = clamp(step.noteVariationProbability() + probabilityBias, -1, LogicSequence::NoteVariationProbability::Max);
        if (useVariation && int(rng.nextRange(LogicSequence::NoteVariationProbability::Range)) <= probability) {
            int offset = step.noteVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.noteVariationRange()) + 1);
            if (step.noteVariationRange() < 0) {
                offset = -offset;
            }
            note = LogicSequence::Note::clamp(note + offset);
        }
        return bypassScale.noteToVolts(note) + (bypassScale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
    }
    int note = step.note() + evalTransposition(scale, octave, transpose);
    int probability = clamp(step.noteVariationProbability() + probabilityBias, -1, LogicSequence::NoteVariationProbability::Max);
    if (useVariation && int(rng.nextRange(LogicSequence::NoteVariationProbability::Range)) <= probability) {
        int offset = step.noteVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.noteVariationRange()) + 1);
        if (step.noteVariationRange() < 0) {
            offset = -offset;
        }
        note = LogicSequence::Note::clamp(note + offset);
    }
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}

void LogicTrackEngine::reset() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
    _prevCondition = false;
    _activity = false;
    _gateOutput = false;
    //_cvOutput = 0.f;
    //_cvOutputTarget = 0.f;
    _slideActive = false;
    _gateQueue.clear();
    _cvQueue.clear();
    _recordHistory.clear();

    changePattern();
}

void LogicTrackEngine::restart() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
}

TrackEngine::TickResult LogicTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick % linkData->divisor == 0) {
            recordStep(tick, linkData->divisor);
            triggerStep(tick, linkData->divisor);
        }
    } else {
        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
        uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;

        if (int(_model.project().stepsToStop()) != 0 && int(relativeTick / divisor) == int(_model.project().stepsToStop())) {
            _engine.clockStop();
        }

        // handle reset measure
        if (relativeTick == 0) {
            reset();
            _currentStageRepeat = 1;
        }
        const auto &sequence = *_sequence;

        // advance sequence
        switch (_logicTrack.playMode()) {
        case Types::PlayMode::Aligned:
            if (relativeTick % divisor == 0) {
                _sequenceState.advanceAligned(relativeTick / divisor, sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
                recordStep(tick, divisor);
                triggerStep(tick, divisor);

                _sequenceState.calculateNextStepAligned(
                        (relativeTick + divisor) / divisor,
                        sequence.runMode(),
                        sequence.firstStep(),
                        sequence.lastStep(),
                        rng
                    );
                triggerStep(tick + divisor, divisor, true);
            }
            break;
        case Types::PlayMode::Free:
            relativeTick = _freeRelativeTick;
            if (++_freeRelativeTick >= divisor) {
                _freeRelativeTick = 0;
            }
            if (relativeTick == 0) {
                if (_currentStageRepeat == 1) {
                     _sequenceState.advanceFree(sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
                     _sequenceState.calculateNextStepFree(
                        sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
                }

                recordStep(tick, divisor);
                const auto &step = sequence.step(_sequenceState.step());
                bool isLastStageStep = ((int) step.stageRepeats() - (int) _currentStageRepeat) <= 0;
            
                if (step.gateOffset() >= 0) {
                    triggerStep(tick, divisor);
                }

                if (!isLastStageStep && step.gateOffset() < 0) {
                    triggerStep(tick + divisor, divisor, false);
                }

                if (isLastStageStep 
                        && sequence.step(_sequenceState.nextStep()).gateOffset() < 0) {
                    triggerStep(tick + divisor, divisor, true);
                }
               
                if (isLastStageStep) {
                   _currentStageRepeat = 1; 
                } else {
                    _currentStageRepeat++;
                }
            }
            break;
        case Types::PlayMode::Last:
            break;
        }

        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = &_sequenceState;
    }

    auto &midiOutputEngine = _engine.midiOutputEngine();

    TickResult result = TickResult::NoUpdate;

    while (!_gateQueue.empty() && tick >= _gateQueue.front().tick) {
        if (!_monitorOverrideActive) {
            result |= TickResult::GateUpdate;
            _activity = _gateQueue.front().gate;
            _gateOutput = (!mute() || fill()) && _activity;
            midiOutputEngine.sendGate(_track.trackIndex(), _gateOutput);
        }
        _gateQueue.pop();

    }

    while (!_cvQueue.empty() && tick >= _cvQueue.front().tick) {
        if (!mute() || _logicTrack.cvUpdateMode() == LogicTrack::CvUpdateMode::Always) {
            if (!_monitorOverrideActive) {
                result |= TickResult::CvUpdate;
                _cvOutputTarget = _cvQueue.front().cv;
                _slideActive = _cvQueue.front().slide;
                midiOutputEngine.sendCv(_track.trackIndex(), _cvOutputTarget);
                midiOutputEngine.sendSlide(_track.trackIndex(), _slideActive);
            }
        }
        _cvQueue.pop();
    }

    return result;
}

void LogicTrackEngine::update(float dt) {
    bool running = _engine.state().running();
    bool recording = _engine.state().recording();

    const auto &sequence = *_sequence;
    const auto &scale = sequence.selectedScale(_model.project().scale());
    int rootNote = sequence.selectedRootNote(_model.project().rootNote());
    int octave = _logicTrack.octave();
    int transpose = _logicTrack.transpose();

    // enable/disable step recording mode
    /*if (recording && _model.project().recordMode() == Types::RecordMode::StepRecord) {
        if (!_stepRecorder.enabled()) {
            _stepRecorder.start(sequence);
        }
    } else {
        if (_stepRecorder.enabled()) {
            _stepRecorder.stop();
        }
    }*/

    // helper to send gate/cv from monitoring to midi output engine
    auto sendToMidiOutputEngine = [this] (bool gate, float cv = 0.f) {
        auto &midiOutputEngine = _engine.midiOutputEngine();
        midiOutputEngine.sendGate(_track.trackIndex(), gate);
        if (gate) {
            midiOutputEngine.sendCv(_track.trackIndex(), cv);
            midiOutputEngine.sendSlide(_track.trackIndex(), false);
        }
    };

    // set monitor override
    auto setOverride = [&] (float cv) {
        _cvOutputTarget = cv;
        _activity = _gateOutput = true;
        _monitorOverrideActive = true;
        // pass through to midi engine
        sendToMidiOutputEngine(true, cv);
    };

    // clear monitor override
    auto clearOverride = [&] () {
        if (_monitorOverrideActive) {
            _activity = _gateOutput = false;
            _monitorOverrideActive = false;
            sendToMidiOutputEngine(false);
        }
    };

    // check for step monitoring
    bool stepMonitoring = (!running && _monitorStepIndex >= 0);

    // check for live monitoring
    auto monitorMode = _model.project().monitorMode();
    bool liveMonitoring =
        (monitorMode == Types::MonitorMode::Always) ||
        (monitorMode == Types::MonitorMode::Stopped && !running);

    if (stepMonitoring) {
        const auto &step = sequence.step(_monitorStepIndex);
        setOverride(evalStepNote(step, 0, scale, rootNote, octave, transpose, false));
    } else if (liveMonitoring && _recordHistory.isNoteActive()) {
        int note = noteFromMidiNote(_recordHistory.activeNote()) + evalTransposition(scale, octave, transpose);
        setOverride(scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f));
    } else {
        clearOverride();
    }

    if (_slideActive && _logicTrack.slideTime() > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget, _logicTrack.slideTime(), dt);
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void LogicTrackEngine::changePattern() {
    _sequence = &_logicTrack.sequence(pattern());
    _fillSequence = &_logicTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
}

void LogicTrackEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _recordHistory.write(tick, message);

    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord) {
       // _stepRecorder.process(message, *_sequence, [this] (int midiNote) { return noteFromMidiNote(midiNote); });
    }
}

void LogicTrackEngine::clearMidiMonitoring() {
    _recordHistory.clear();
}

void LogicTrackEngine::setMonitorStep(int index) {
    _monitorStepIndex = (index >= 0 && index < CONFIG_STEP_COUNT) ? index : -1;

    // in step record mode, select step to start recording recording from
    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord &&
        index >= _sequence->firstStep() && index <= _sequence->lastStep()) {
        _stepRecorder.setStepIndex(index);
    }
}
void LogicTrackEngine::triggerStep(uint32_t tick, uint32_t divisor, bool forNextStep) {
    int octave = _logicTrack.octave();
    int transpose = _logicTrack.transpose();
    int rotate = _logicTrack.rotate();
    bool fillStep = fill() && (rng.nextRange(100) < uint32_t(fillAmount()));
    bool useFillGates = fillStep && _logicTrack.fillMode() == LogicTrack::FillMode::Gates;
    bool useFillSequence = fillStep && _logicTrack.fillMode() == LogicTrack::FillMode::NextPattern;
    bool useFillCondition = fillStep && _logicTrack.fillMode() == LogicTrack::FillMode::Condition;

    const auto &sequence = *_sequence;
    const auto &evalSequence = useFillSequence ? *_fillSequence : *_sequence;

    // TODO do we need to encounter rotate?
    _currentStep = SequenceUtils::rotateStep(_sequenceState.step(), sequence.firstStep(), sequence.lastStep(), rotate);

    int stepIndex;

    if (forNextStep) {
        stepIndex = _sequenceState.nextStep();
    } else {
        stepIndex = _currentStep;
    }

    if (stepIndex < 0) return;

    const auto &step = evalSequence.step(stepIndex);

    int gateOffset = ((int) divisor * step.gateOffset()) / (LogicSequence::GateOffset::Max + 1);
    uint32_t stepTick = (int) tick + gateOffset;

    bool stepGate = evalStepGate(step, _logicTrack.gateProbabilityBias()) || useFillGates;
    if (stepGate) {
        stepGate = evalStepCondition(step, _sequenceState.iteration(), useFillCondition, _prevCondition);
    }
    switch (step.stageRepeatMode()) {
        case LogicSequence::StageRepeatMode::Each:
            break;
        case LogicSequence::StageRepeatMode::First:
            stepGate = stepGate && _currentStageRepeat == 1;
            break;
        case LogicSequence::StageRepeatMode::Last:
            stepGate = stepGate && _currentStageRepeat == step.stageRepeats()+1;
            break;
        case LogicSequence::StageRepeatMode::Middle:
            stepGate = stepGate && _currentStageRepeat == (step.stageRepeats()+1)/2;
            break;
        case LogicSequence::StageRepeatMode::Odd:
            stepGate = stepGate && _currentStageRepeat % 2 != 0;
            break;
        case LogicSequence::StageRepeatMode::Even:
            stepGate = stepGate && _currentStageRepeat % 2 == 0;
            break;
        case LogicSequence::StageRepeatMode::Triplets:
            stepGate = stepGate && (_currentStageRepeat - 1) % 3 == 0;
            break;
        case LogicSequence::StageRepeatMode::Random:
                srand((unsigned int)time(NULL));
                int rndMode = rng.nextRange(6);
                switch (rndMode) {
                    case 0:
                        break;
                    case 1:
                        stepGate = stepGate && _currentStageRepeat == 1;
                        break;
                    case 2:
                        stepGate = stepGate && _currentStageRepeat == step.stageRepeats()+1;
                        break;
                    case 3:
                        stepGate = stepGate && _currentStageRepeat % ((step.stageRepeats()+1)/2)+1 == 0;
                        break;
                    case 4:
                        stepGate = stepGate && _currentStageRepeat % 2 != 0;
                        break;
                    case 5:
                        stepGate = stepGate && _currentStageRepeat % 2 == 0;
                        break;
                    case 6:
                        stepGate = stepGate && (_currentStageRepeat - 1) % 3 == 0;
                        break;

                }
                break;
    }

    if (stepGate) {
        uint32_t stepLength = (divisor * evalStepLength(step, _logicTrack.lengthBias())) / LogicSequence::Length::Range;
        int stepRetrigger = evalStepRetrigger(step, _logicTrack.retriggerProbabilityBias());
        if (stepRetrigger > 1) {
            uint32_t retriggerLength = divisor / stepRetrigger;
            uint32_t retriggerOffset = 0;
            while (stepRetrigger-- > 0 && retriggerOffset <= stepLength) {
                _gateQueue.pushReplace({ Groove::applySwing(stepTick + retriggerOffset, swing()), true });
                _gateQueue.pushReplace({ Groove::applySwing(stepTick + retriggerOffset + retriggerLength / 2, swing()), false });
                retriggerOffset += retriggerLength;
            }
        } else {
            _gateQueue.pushReplace({ Groove::applySwing(stepTick, swing()), true });
            _gateQueue.pushReplace({ Groove::applySwing(stepTick + stepLength, swing()), false });
        }
    }

    if (stepGate || _logicTrack.cvUpdateMode() == LogicTrack::CvUpdateMode::Always) {
        const auto &scale = evalSequence.selectedScale(_model.project().scale());
        int rootNote = evalSequence.selectedRootNote(_model.project().rootNote());
        _cvQueue.push({ Groove::applySwing(stepTick, swing()), evalStepNote(step, _logicTrack.noteProbabilityBias(), scale, rootNote, octave, transpose), step.slide() });
    }
}

void LogicTrackEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    triggerStep(tick, divisor, false);
}

void LogicTrackEngine::recordStep(uint32_t tick, uint32_t divisor) {
    if (!_engine.state().recording() || _model.project().recordMode() == Types::RecordMode::StepRecord || _sequenceState.prevStep() < 0) {
        return;
    }

    bool stepWritten = false;

    auto writeStep = [this, divisor, &stepWritten] (int stepIndex, int note, int lengthTicks) {
        auto &step = _sequence->step(stepIndex);
        int length = (lengthTicks * LogicSequence::Length::Range) / divisor;

        step.setGate(true);
        step.setGateProbability(LogicSequence::GateProbability::Max);
        step.setRetrigger(0);
        step.setRetriggerProbability(LogicSequence::RetriggerProbability::Max);
        step.setLength(length);
        step.setLengthVariationRange(0);
        step.setLengthVariationProbability(LogicSequence::LengthVariationProbability::Max);
        step.setNote(noteFromMidiNote(note));
        step.setNoteVariationRange(0);
        step.setNoteVariationProbability(LogicSequence::NoteVariationProbability::Max);
        step.setCondition(Types::Condition::Off);


        stepWritten = true;
    };

    auto clearStep = [this] (int stepIndex) {
        auto &step = _sequence->step(stepIndex);

        step.clear();
    };

    uint32_t stepStart = tick - divisor;
    uint32_t stepEnd = tick;
    uint32_t margin = divisor / 2;

    for (size_t i = 0; i < _recordHistory.size(); ++i) {
        if (_recordHistory[i].type != RecordHistory::Type::NoteOn) {
            continue;
        }

        int note = _recordHistory[i].note;
        uint32_t noteStart = _recordHistory[i].tick;
        uint32_t noteEnd = i + 1 < _recordHistory.size() ? _recordHistory[i + 1].tick : tick;

        if (noteStart >= stepStart - margin && noteStart < stepStart + margin) {
            // note on during step start phase
            if (noteEnd >= stepEnd) {
                // note hold during step
                int length = std::min(noteEnd, stepEnd) - stepStart;
                writeStep(_sequenceState.prevStep(), note, length);
            } else {
                // note released during step
                int length = noteEnd - noteStart;
                writeStep(_sequenceState.prevStep(), note, length);
            }
        } else if (noteStart < stepStart && noteEnd > stepStart) {
            // note on during previous step
            int length = std::min(noteEnd, stepEnd) - stepStart;
            writeStep(_sequenceState.prevStep(), note, length);
        }
    }

    if (isSelected() && !stepWritten && _model.project().recordMode() == Types::RecordMode::Overwrite) {
        clearStep(_sequenceState.prevStep());
    }
}

int LogicTrackEngine::noteFromMidiNote(uint8_t midiNote) const {
    const auto &scale = _sequence->selectedScale(_model.project().scale());
    int rootNote = _sequence->selectedRootNote(_model.project().rootNote());

    if (scale.isChromatic()) {
        return scale.noteFromVolts((midiNote - 60 - rootNote) * (1.f / 12.f));
    } else {
        return scale.noteFromVolts((midiNote - 60) * (1.f / 12.f));
    }
}
