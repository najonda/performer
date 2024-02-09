#include "StochasticEngine.h"

#include "Engine.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/StochasticSequence.h"
#include "model/Scale.h"
#include "ui/MatrixMap.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <ctime>
#include <iterator>
#include <random>
#include <vector>

static Random rng;

bool sortTaskByProbRev(const StochasticStep& lhs, const StochasticStep& rhs) {
    return lhs.probability() > rhs.probability();
}

// evaluate if step gate is active
static bool evalStepGate(const StochasticSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.gateProbability() + probabilityBias, -1, StochasticSequence::GateProbability::Max);
    return step.gate() && int(rng.nextRange(StochasticSequence::GateProbability::Range)) <= probability;
}

// evaluate if step gate is active
 int StochasticEngine::evalRestProbability(StochasticSequence &sequence) {
    int sum = 0;
    std::vector<StochasticStep> probability;
    for (int i = 0; i < 4; i++) {

        switch (i) {
            case 0: {
                probability.insert(probability.end(), StochasticStep(i, clamp(sequence.restProbability(), -1, StochasticSequence::NoteVariationProbability::Max)));
            }
                break;
            case 1: {
                probability.insert(probability.end(), StochasticStep(i, clamp(sequence.restProbability2(), -1, StochasticSequence::NoteVariationProbability::Max)));
                break;
            }
            case 2: {
                probability.insert(probability.end(), StochasticStep(i, clamp(sequence.restProbability4(), -1, StochasticSequence::NoteVariationProbability::Max)));
                break;
            }
            case 3: {
                probability.insert(probability.end(), StochasticStep(i, clamp(sequence.restProbability8(), -1, StochasticSequence::NoteVariationProbability::Max)));
                break;
            }
            default:
                break;
        }

        sum = sum + probability.at(i).probability();
    }
    if (sum==0) { return -1;}
    std::sort (std::begin(probability), std::end(probability), sortTaskByProbRev);
    int stepIndex = getNextWeightedPitch(probability, 0, probability.size());
    switch (stepIndex) {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 3;
        case 3:
            return 7;
    }
    return -1;
}

// evaluate step condition
static bool evalStepCondition(const StochasticSequence::Step &step, int iteration, bool fill, bool &prevCondition) {
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
static int evalStepRetrigger(const StochasticSequence::Step &step, int probabilityBias) {
    int probability = clamp(step.retriggerProbability() + probabilityBias, -1, StochasticSequence::RetriggerProbability::Max);
    return int(rng.nextRange(StochasticSequence::RetriggerProbability::Range)) <= probability ? step.retrigger() + 1 : 1;
}

// evaluate step length
static int evalStepLength(const StochasticSequence::Step &step, int lengthBias) {
    int length = StochasticSequence::Length::clamp(step.length() + lengthBias) + 1;
    int probability = step.lengthVariationProbability();
    if (int(rng.nextRange(StochasticSequence::LengthVariationProbability::Range)) <= probability) {
        int offset = step.lengthVariationRange() == 0 ? 0 : rng.nextRange(std::abs(step.lengthVariationRange()) + 1);
        if (step.lengthVariationRange() < 0) {
            offset = -offset;
        }
        length = clamp(length + offset, 0, StochasticSequence::Length::Range);
    }
    return length;
}

// evaluate transposition
static int evalTransposition(const Scale &scale, int octave, int transpose) {
    return octave * scale.notesPerOctave() + transpose;
}

// evaluate note voltage
static float evalStepNote(const StochasticSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, StochasticSequence sequence, bool useVariation = true) {
    if (step.bypassScale()) {
        const Scale &bypassScale = Scale::get(0);
        int note = step.note() + evalTransposition(bypassScale, octave, transpose);
        int probability = clamp(step.noteOctaveProbability() + probabilityBias, -1, StochasticSequence::NoteOctaveProbability::Max);
        if (step.noteOctaveProbability()==0) {
            probability = 0;
        }
        if (useVariation && int(rng.nextRange(StochasticSequence::NoteOctaveProbability::Range)) <= probability && probability!= 0) {
            int oct = step.noteOctave() + sequence.lowOctaveRange() + ( std::rand() % ( sequence.highOctaveRange() - sequence.lowOctaveRange() + 1 ) );
            note = StochasticSequence::Note::clamp(note + (bypassScale.notesPerOctave()*oct));
        }
        return bypassScale.noteToVolts(note) + (bypassScale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
    }
    int note = step.note() + evalTransposition(scale, octave, transpose);
    int probability = clamp(step.noteOctaveProbability() + probabilityBias, -1, StochasticSequence::NoteOctaveProbability::Max);
    if (useVariation && int(rng.nextRange(StochasticSequence::NoteOctaveProbability::Range)) <= probability && probability != 0) {
        int oct = step.noteOctave() + sequence.lowOctaveRange() + ( std::rand() % ( sequence.highOctaveRange() - sequence.lowOctaveRange() + 1 ) );
        note = StochasticSequence::Note::clamp(note + (scale.notesPerOctave()*oct));
    }
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}

void StochasticEngine::reset() {
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

void StochasticEngine::restart() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
}

TrackEngine::TickResult StochasticEngine::tick(uint32_t tick) {
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

        // handle reset measure
        if (relativeTick == 0) {
            reset();
            _currentStageRepeat = 1;
        }
        const auto &sequence = *_sequence;

        // advance sequence
        switch (_stochasticTrack.playMode()) {
        case Types::PlayMode::Aligned:
            if (relativeTick % divisor == 0) {
                triggerStep(tick, divisor);

                _sequenceState.calculateNextStepAligned(
                        (relativeTick + divisor) / divisor,
                        sequence.runMode(),
                        sequence.sequenceFirstStep(),
                        sequence.sequenceLastStep(),
                        rng
                    );
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
                }

                const auto &step = sequence.step(_sequenceState.step());
                bool isLastStageStep = ((int) (step.stageRepeats()+1) - (int) _currentStageRepeat) <= 0;
            
                triggerStep(tick+divisor, divisor);
                               
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
        if (!mute() || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {
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

void StochasticEngine::update(float dt) {
    bool running = _engine.state().running();
    const auto &sequence = *_sequence;
    const auto &scale = sequence.selectedScale(_model.project().scale());
    int rootNote = sequence.selectedRootNote(_model.project().rootNote());
    int octave = _stochasticTrack.octave();
    int transpose = _stochasticTrack.transpose();

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
    //if (button.row == 1) {
    //    _project.selectedStochasticSequence().setRestProbability(button.gridIndex());
    //}
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
        setOverride(evalStepNote(step, 0, scale, rootNote, octave, transpose, sequence, false));
    } else if (liveMonitoring && _recordHistory.isNoteActive()) {
        int note = noteFromMidiNote(_recordHistory.activeNote()) + evalTransposition(scale, octave, transpose);
        setOverride(scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f));
    } else {
        clearOverride();
    }

    if (_slideActive && _stochasticTrack.slideTime() > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget, _stochasticTrack.slideTime(), dt);
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void StochasticEngine::changePattern() {
    _sequence = &_stochasticTrack.sequence(pattern());
    _fillSequence = &_stochasticTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
}

void StochasticEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _recordHistory.write(tick, message);
}

void StochasticEngine::clearMidiMonitoring() {
    _recordHistory.clear();
}

void StochasticEngine::setMonitorStep(int index) {
    _monitorStepIndex = (index >= 0 && index < CONFIG_STEP_COUNT) ? index : -1;

    // in step record mode, select step to start recording recording from
    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord &&
        index >= _sequence->firstStep() && index <= _sequence->lastStep()) {
        _stepRecorder.setStepIndex(index);
    }
}


std::vector<StochasticLoopStep> inMemSteps;
std::vector<StochasticLoopStep> lockedSteps;

bool start = 0;

int skips = 0;

void StochasticEngine::triggerStep(uint32_t tick, uint32_t divisor, bool forNextStep) {
    int octave = _stochasticTrack.octave();
    int transpose = _stochasticTrack.transpose();
    bool fillStep = fill() && (rng.nextRange(100) < uint32_t(fillAmount()));
    bool useFillGates = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::Gates;
    bool useFillSequence = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::NextPattern;
    bool useFillCondition = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::Condition;

    auto &sequence = *_sequence;
    auto &evalSequence = useFillSequence ? *_fillSequence : *_sequence;
    
    int stepIndex;

    uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
    uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;
    auto abstoluteStep = ((relativeTick + divisor) / divisor) -1;
    
    auto index = abstoluteStep % sequence.sequenceLength();

    StochasticSequence::Step step;
    uint32_t stepTick = 0;
    bool stepGate = false;
    float noteValue = 0;
    uint32_t stepLength = 0;
    int stepRetrigger = 0;

    if (sequence.reseed()) {

        srand((unsigned int)time(NULL));
        int rnd = -StochasticSequence::NoteVariationProbability::Range + ( std::rand() % ( (StochasticSequence::NoteVariationProbability::Range) - (-StochasticSequence::NoteVariationProbability::Range)) + 1 );

        _stochasticTrack.setNoteProbabilityBias(rnd);
        sequence.setReseed(false);
    }

    // fill in memory step when sequence is running or when the in memory loop is not full filled
    if (!sequence.useLoop() || (sequence.useLoop() && int(inMemSteps.size()) < sequence.bufferLoopLength())) { 


        
        if (skips != 0) {
            skips--;
            inMemSteps.insert(inMemSteps.end(), StochasticLoopStep(-1, false, step, 0, 0, 0));
            return;
        }
        if (index % 2 == 0) {
            int rest = evalRestProbability(sequence);
            if (rest != -1) {
                skips = rest;
            }
        }

        std::vector<StochasticStep> probability;
        int sum =0;
        for (int i = 0; i < 12; i++) {
            if (sequence.step(i).gate()) {
                probability.insert(probability.end(), StochasticStep(i, clamp(sequence.step(i).noteVariationProbability() + _stochasticTrack.noteProbabilityBias(), -1, StochasticSequence::NoteVariationProbability::Max)));
            } else {
                probability.insert(probability.end(), StochasticStep(i, 0));
            }
            sum = sum + probability.at(i).probability();
        }
        if (sum==0) { return;}
        std::sort (std::begin(probability), std::end(probability), sortTaskByProbRev);
        stepIndex = getNextWeightedPitch(probability, sequence.reseed(), probability.size());

        step = sequence.step(stepIndex); 
        _currentStep = stepIndex;   
        
        int gateOffset = ((int) divisor * step.gateOffset()) / (StochasticSequence::GateOffset::Max + 1);
        stepTick = (int) tick + gateOffset;

        stepGate = evalStepGate(step, _stochasticTrack.gateProbabilityBias()) || useFillGates;
        if (stepGate) {
            stepGate = evalStepCondition(step, _sequenceState.iteration(), useFillCondition, _prevCondition);
        }
        switch (step.stageRepeatMode()) {
            case StochasticSequence::StageRepeatMode::Each:
                break;
            case StochasticSequence::StageRepeatMode::First:
                stepGate = stepGate && _currentStageRepeat == 1;
                break;
            case StochasticSequence::StageRepeatMode::Last:
                stepGate = stepGate && _currentStageRepeat == step.stageRepeats()+1;
                break;
            case StochasticSequence::StageRepeatMode::Middle:
                stepGate = stepGate && _currentStageRepeat == (step.stageRepeats()+1)/2;
                break;
            case StochasticSequence::StageRepeatMode::Odd:
                stepGate = stepGate && _currentStageRepeat % 2 != 0;
                break;
            case StochasticSequence::StageRepeatMode::Even:
                stepGate = stepGate && _currentStageRepeat % 2 == 0;
                break;
            case StochasticSequence::StageRepeatMode::Triplets:
                stepGate = stepGate && (_currentStageRepeat - 1) % 3 == 0;
                break;
            case StochasticSequence::StageRepeatMode::Random:
                    srand((unsigned int)time(NULL));
                    int rndMode = 0 + ( std::rand() % ( 6 - 0 + 1 ) );
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
        const auto &scale = evalSequence.selectedScale(_model.project().scale());
        int rootNote = evalSequence.selectedRootNote(_model.project().rootNote());
        noteValue = evalStepNote(step, _stochasticTrack.noteProbabilityBias(), scale, rootNote, octave, transpose, sequence);
        stepLength = (divisor * evalStepLength(step, _stochasticTrack.lengthBias())) / StochasticSequence::Length::Range;

        int rnd = 0;
        if (sequence.lengthModifier()!= 0) {
            std::random_device r;
            int mean = sequence.lengthModifier();
            std::seed_seq seed2{r(), r(), r(), r(), r(), r(), r(), r()};
            std::mt19937 e2(seed2);
            std::normal_distribution<> normal_dist(mean, 2);

            rnd = std::round(normal_dist(e2));
            
        }
        stepLength = stepLength + rnd;
        stepRetrigger = evalStepRetrigger(step, _stochasticTrack.retriggerProbabilityBias());
        if (int(inMemSteps.size()) < sequence.bufferLoopLength()) {
            inMemSteps.insert(inMemSteps.end(), StochasticLoopStep(stepIndex, stepGate, step, noteValue, stepLength, stepRetrigger));
        }
    }

    // clear the in memory sequence when reaches the max size
    if (!sequence.useLoop() && int(inMemSteps.size()) >= sequence.bufferLoopLength()) {
        inMemSteps.clear();
    }

    // clear the locked memory sequence and reset it to the in memory sequence
    if (sequence.clearLoop()) {
        lockedSteps = inMemSteps;
        sequence.setClearLoop(false);
        sequence.setUseLoop(true);
    }

    // use the locked loop to retrieve steps data
    if (sequence.useLoop() && int(inMemSteps.size()) >= sequence.bufferLoopLength()) {

        if (int(lockedSteps.size()) != int(inMemSteps.size())) {
            lockedSteps = inMemSteps;
        }

        auto subArray = slicing(lockedSteps, sequence.sequenceFirstStep(), sequence.sequenceLastStep());
        stepIndex = subArray.at(index).index();
        if (stepIndex == -1) {
            return;
        }
        _currentStep = stepIndex;

        stepGate = subArray.at(index).gate();
        step = subArray.at(index).step();
        int gateOffset = ((int) divisor * step.gateOffset()) / (StochasticSequence::GateOffset::Max + 1);
        stepTick = (int) tick + gateOffset;
        noteValue = subArray.at(index).noteValue();
        stepLength = subArray.at(index).stepLength();
        stepRetrigger = lockedSteps.at(index).stepRetrigger();
    
    }

    if (stepGate) {
        sequence.setStepBounds(stepIndex);
        
        //stepLength = (divisor * evalStepLength(step, _stochasticTrack.lengthBias())) / StochasticSequence::Length::Range;
        //stepRetrigger = evalStepRetrigger(step, _stochasticTrack.retriggerProbabilityBias());
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

    if (stepGate || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {

        _cvQueue.push({ Groove::applySwing(stepTick, swing()), noteValue, step.slide() });
    }
}


int StochasticEngine::getNextWeightedPitch(std::vector<StochasticStep> distr, bool reseed, int notesPerOctave) {
        int total_weights = 0;

        for(int i = 0; i < notesPerOctave; i++) {
            total_weights += distr.at(i % notesPerOctave).probability();
        }


        if (reseed) { 
            srand((unsigned int)time(NULL));
        }
        int rnd = 1 + ( std::rand() % ( (total_weights) - 1 + 1 ) );

        for(int i = 0; i < notesPerOctave; i++) {
            int weight = distr.at(i % notesPerOctave).probability();
            if (rnd <= weight && weight > 0) {
                return distr.at(i % notesPerOctave).index();
            }
            rnd -= weight;
        }
        return -1;
    }



void StochasticEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    triggerStep(tick, divisor, false);
}

void StochasticEngine::recordStep(uint32_t tick, uint32_t divisor) {
    if (!_engine.state().recording() || _model.project().recordMode() == Types::RecordMode::StepRecord || _sequenceState.prevStep() < 0) {
        return;
    }

    bool stepWritten = false;

    auto writeStep = [this, divisor, &stepWritten] (int stepIndex, int note, int lengthTicks) {
        auto &step = _sequence->step(stepIndex);
        int length = (lengthTicks * StochasticSequence::Length::Range) / divisor;

        step.setGate(true);
        step.setGateProbability(StochasticSequence::GateProbability::Max);
        step.setRetrigger(0);
        step.setRetriggerProbability(StochasticSequence::RetriggerProbability::Max);
        step.setLength(length);
        step.setLengthVariationRange(0);
        step.setLengthVariationProbability(StochasticSequence::LengthVariationProbability::Max);
        step.setNote(noteFromMidiNote(note));
        step.setNoteOctave(0);
        step.setNoteVariationProbability(StochasticSequence::NoteVariationProbability::Max);
        step.setCondition(Types::Condition::Off);
        step.setStageRepeats(1);

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

int StochasticEngine::noteFromMidiNote(uint8_t midiNote) const {
    const auto &scale = _sequence->selectedScale(_model.project().scale());
    int rootNote = _sequence->selectedRootNote(_model.project().rootNote());

    if (scale.isChromatic()) {
        return scale.noteFromVolts((midiNote - 60 - rootNote) * (1.f / 12.f));
    } else {
        return scale.noteFromVolts((midiNote - 60) * (1.f / 12.f));
    }
}
