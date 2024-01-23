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

    void setPatternFollowDisplay(bool trackDisplay) {
        const auto pattern_follow = patternFollow();

        const bool lp_tracking =
            (pattern_follow == Types::PatternFollow::LaunchPad ||
             pattern_follow == Types::PatternFollow::DispAndLP);

        setPatternFollow(trackDisplay, lp_tracking);
    }

    bool isPatternFollowDisplayOn() {
        const auto pattern_follow = patternFollow();

        return (pattern_follow == Types::PatternFollow::Display ||
                pattern_follow == Types::PatternFollow::DispAndLP);
    }


    void togglePatternFollowDisplay() {
        const auto pattern_follow = patternFollow();

        const bool disp_tracking = isPatternFollowDisplayOn();

        const bool lp_tracking =
            (pattern_follow == Types::PatternFollow::LaunchPad ||
             pattern_follow == Types::PatternFollow::DispAndLP);

        setPatternFollow(not disp_tracking, lp_tracking);
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
