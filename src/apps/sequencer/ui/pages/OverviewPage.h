#pragma once

#include "BasePage.h"
#include "ui/StepSelection.h"

class OverviewPage : public BasePage {
public:
    OverviewPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

private:

    void drawDetail(Canvas &canvas, const NoteSequence::Step &step);
    void drawStochasticDetail(Canvas &canvas, const StochasticSequence::Step &step);
    void drawCurveDetail(Canvas &canvas, const CurveSequence::Step &step);
    void updateMonitorStep();

    static const int StepCount = 16;

    int stepOffset() const { 

        switch (_project.selectedTrack().trackMode()) {

            case Track::TrackMode::Note:
                return _project.selectedNoteSequence().section() * StepCount; 
            case Track::TrackMode::Stochastic:
                return 0;
            case Track::TrackMode::Curve:
                return _project.selectedCurveSequence().section() * StepCount; 
            case Track::TrackMode::MidiCv:
                return 0;
            default:
                return 0;
        
        }
    }

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;
    bool _showDetail;
    uint32_t _showDetailTicks;
};
