#pragma once

#include "Config.h"
#include "Types.h"
#include "NoteSequence.h"
#include "Serialize.h"
#include "Routing.h"
#include "FileDefs.h"
#include "core/utils/StringUtils.h"


class BaseTrackPatternFollow {
public:
    
    // patternFollow
    Types::PatternFollow patternFollow() const { return _patternFollow; }
    void setPatternFollow(const Types::PatternFollow patternFollow) {
        _patternFollow = ModelUtils::clampedEnum(patternFollow);
    }

    void setPatternFollow(bool trackDisplay, bool trackLP) {

        if (trackDisplay && trackLP) {
            setPatternFollow(Types::PatternFollow::DispAndLP);
            return;
        }

        else if (trackDisplay) {
            setPatternFollow(Types::PatternFollow::Display);
            return;
        }

        else if (trackLP) {
            setPatternFollow(Types::PatternFollow::LaunchPad);
            return;
        }

        setPatternFollow(Types::PatternFollow::Off);

        return;
    }

    void editPatternFollow(int value, bool shift) {
        setPatternFollow(ModelUtils::adjustedEnum(patternFollow(), value));
    }

    void printPatternFollow(StringBuilder &str) const {
        str(Types::patternFollowName(patternFollow()));
    }

    protected:
        Types::PatternFollow _patternFollow;

};