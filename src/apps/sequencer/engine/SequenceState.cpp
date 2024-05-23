#include "SequenceState.h"

#include "core/Debug.h"
#include "core/math/Math.h"

static int randomStep(int firstStep, int lastStep, Random &rng) {
    return rng.nextRange(lastStep - firstStep + 1) + firstStep;
}

void SequenceState::reset() {
    _nextStep = -1;
    _step = -1;
    _prevStep = -1;
    _direction = 1;
    _iteration = 0;
    _nextIteration = 0;
}

void SequenceState::advanceFree(Types::RunMode runMode, int firstStep, int lastStep, Random &rng) {
     ASSERT(firstStep <= lastStep, "invalid first/last step");

    if (_nextStep < 0) {
        calculateNextStepFree(runMode, firstStep, lastStep, rng);
    }

    _iteration = _nextIteration;
    _prevStep = _step;
    _step = _nextStep;
    _nextStep = -1;
}

void SequenceState::calculateNextStepFree(Types::RunMode runMode, int firstStep, int lastStep, Random &rng) {
    if (_step == -1) {
        // first step
        switch (runMode) {
        case Types::RunMode::Forward:
        case Types::RunMode::Pendulum:
        case Types::RunMode::PingPong:
            _nextStep = firstStep;
            break;
        case Types::RunMode::Backward:
            _nextStep = lastStep;
            break;
        case Types::RunMode::Random:
        case Types::RunMode::RandomWalk:
        case Types::RunMode::DrunkenWalk:
            _nextStep = randomStep(firstStep, lastStep, rng);
            break;
        case Types::RunMode::Last:
            break;
        }
    } else {
        // advance step
        _nextStep = clamp(int(_step), firstStep, lastStep);

        switch (runMode) {
        case Types::RunMode::Forward:
            if (_step >= lastStep) {
                _nextStep = firstStep;
                _nextIteration = _iteration + 1;
            } else {
                _nextStep = _step + 1;
            }
            break;
        case Types::RunMode::Backward:
            if (_step <= firstStep) {
                _nextStep = lastStep;
                _nextIteration = _iteration + 1;
            } else {
                _nextStep = _step - 1;
            }
            break;
        case Types::RunMode::Pendulum:
        case Types::RunMode::PingPong:
            if (_direction > 0 && _step >= lastStep) {
                _direction = -1;
            } else if (_direction < 0 && _step <= firstStep) {
                _direction = 1;
                _nextIteration = _iteration + 1;
            } else {
                if (runMode == Types::RunMode::Pendulum) {
                    _nextStep = _step + _direction;
                }
            }
            if (runMode == Types::RunMode::PingPong) {
                _nextStep = _step + _direction;
            }
            break;
        case Types::RunMode::Random:
            _nextStep = randomStep(firstStep, lastStep, rng);
            break;
        case Types::RunMode::RandomWalk:
            advanceRandomWalk(firstStep, lastStep, rng);
            break;
        case Types::RunMode::DrunkenWalk:
            advanceDrunkenWalk(firstStep, lastStep, rng);
            break;
        case Types::RunMode::Last:
            break;
        }
    }
}


void SequenceState::advanceAligned(int absoluteStep, Types::RunMode runMode, int firstStep, int lastStep, Random &rng) {

    if (_nextStep < 0) {
        calculateNextStepAligned(absoluteStep, runMode, firstStep, lastStep, rng);
    }

    //_iteration = _nextIteration;
    _prevStep = _step;
    _step = _nextStep;
    _nextStep = -1;
}

void SequenceState::calculateNextStepAligned(int absoluteStep, Types::RunMode runMode, int firstStep, int lastStep, Random &rng) {
    ASSERT(firstStep <= lastStep, "invalid first/last step");

    int stepCount = lastStep - firstStep + 1;

    switch (runMode) {
    case Types::RunMode::Forward:
        _nextStep = firstStep + absoluteStep % stepCount;
        _iteration = absoluteStep / stepCount;
        _direction = 1;
        break;
    case Types::RunMode::Backward:
        _nextStep = lastStep - absoluteStep % stepCount;
        _iteration = absoluteStep / stepCount;
        _direction = -1;
        break;
    case Types::RunMode::Pendulum:
        _iteration = absoluteStep / (2 * stepCount);
        absoluteStep %= (2 * stepCount);
        _nextStep = (absoluteStep < stepCount) ? (firstStep + absoluteStep) : (lastStep - (absoluteStep - stepCount));
        if (_direction > 0 && _step >= lastStep) {
            _direction = -1;
        } else if (_direction < 0 && _step <= firstStep) {
            _direction = 1;
            ++_nextIteration;
        }
        break;
    case Types::RunMode::PingPong:
        _iteration = absoluteStep / (2 * stepCount - 2);
        absoluteStep %= (2 * stepCount - 2);
        _nextStep = (absoluteStep < stepCount) ? (firstStep + absoluteStep) : (lastStep - (absoluteStep - stepCount) - 1);
        if (_direction > 0 && _step >= lastStep) {
            _direction = -1;
        } else if (_direction < 0 && _step <= firstStep) {
            _direction = 1;
            ++_nextIteration;
        }
        break;
    case Types::RunMode::Random:
        _nextStep = firstStep + rng.nextRange(stepCount);
        break;
    case Types::RunMode::RandomWalk:
        advanceRandomWalk(firstStep, lastStep, rng);
        break;
    case Types::RunMode::DrunkenWalk:
        advanceDrunkenWalk(firstStep, lastStep, rng);
        break;
    case Types::RunMode::Last:
        break;
    }
}


void SequenceState::advanceRandomWalk(int firstStep, int lastStep, Random &rng) {
    if (_step == -1) {
        _nextStep = randomStep(firstStep, lastStep, rng);
    } else {
        int dir = rng.nextRange(2);
        if (dir == 0) {
            _nextStep = _step == firstStep ? lastStep : _step - 1;
            _direction = -1;
        } else {
            _nextStep = _step == lastStep ? firstStep : _step + 1;
            _direction = 1;
        }
    }
}

void SequenceState::advanceDrunkenWalk(int firstStep, int lastStep, Random &rng) {
    if (_step == -1) {
        _nextStep = randomStep(firstStep, lastStep, rng);
    } else {
        int randValue = rng.nextRange(4); // Generates a value between 0 and 3
        if (randValue == 0) { // 25% chance to move backward
            _nextStep = _step == firstStep ? lastStep : _step - 1;
            _direction = -1;
        } else if (randValue == 1) { // 25% chance to stay at the same step
            _nextStep = _step;
            _direction = 0;
        } else { // 50% chance to move forward
            _nextStep = _step == lastStep ? firstStep : _step + 1;
            _direction = 1;
        }
    }
}
