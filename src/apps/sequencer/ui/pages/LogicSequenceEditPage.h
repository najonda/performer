#pragma once

#include "BasePage.h"

#include "ui/StepSelection.h"
#include "ui/model/LogicSequenceListModel.h"

#include "engine/generators/SequenceBuilder.h"
#include "ui/KeyPressEventTracker.h"

#include "core/utils/Container.h"

class LogicSequenceEditPage : public BasePage {
public:
    LogicSequenceEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;
    virtual void midi(MidiEvent &event) override;

private:
    typedef LogicSequence::Layer Layer;

    static const int StepCount = 16;

    int stepOffset() const { return _project.selectedLogicSequence().section() * StepCount; }

    void switchLayer(int functionKey, bool shift);
    int activeFunctionKey();

    void updateMonitorStep();
    void drawDetail(Canvas &canvas, const LogicSequence::Step &step);

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void initSequence();
    void copySequence();
    void pasteSequence();
    void duplicateSequence();
    void tieNotes();
    void generateSequence();

    void quickEdit(int index);

    bool allSelectedStepsActive() const;
    void setSelectedStepsGate(bool gate);

    void setSectionTracking(bool track);
    bool isSectionTracking();
    void toggleSectionTracking();

    LogicSequence::Layer layer() const { return _project.selectedLogicSequenceLayer(); };
    void setLayer(LogicSequence::Layer layer) { _project.setSelectedLogicSequenceLayer(layer); }

    bool _showDetail;
    uint32_t _showDetailTicks;

    KeyPressEventTracker _keyPressEventTracker;


    LogicSequenceListModel _listModel;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;

    Container<LogicSequenceBuilder> _builderContainer;

    LogicSequence _inMemorySequence;
};
