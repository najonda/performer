#include "ContextMenuPage.h"

#include "ui/painters/WindowPainter.h"

#include "core/math/Math.h"
#include <cstdint>

ContextMenuPage::ContextMenuPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}    

uint32_t lastTicks;

void ContextMenuPage::show(ContextMenuModel &contextMenuModel, ResultCallback callback) {
    _contextMenuModel = &contextMenuModel;
    _callback = callback;
    lastTicks = os::ticks();
    BasePage::show();
}

void ContextMenuPage::draw(Canvas &canvas) {


    uint32_t currentTicks = os::ticks();
    uint32_t deltaTicks = currentTicks - lastTicks;
    if (deltaTicks > os::time::ms(2000) && _contextMenuModel->doubleClick()) {
        close();
    }


    canvas.setFont(Font::Tiny);
    canvas.setBlendMode(BlendMode::Set);

    const int BarHeight = 12;

    canvas.setColor(Color::None);
    canvas.fillRect(0, Height - BarHeight - 1, Width, BarHeight + 1);

    canvas.setColor(Color::Bright);
    canvas.hline(0, Height - BarHeight, Width);
    for (int i = 1; i < 5; ++i) {
        canvas.vline((Width * i) / 5, Height - BarHeight, BarHeight);
    }

    for (int i = 0; i < 5; ++i) {
        int itemIndex = i;
        if (itemIndex < _contextMenuModel->itemCount()) {
            const auto &item = _contextMenuModel->item(itemIndex);
            bool enabled = _contextMenuModel->itemEnabled(itemIndex);

            int x = (Width * i) / 5;
            int w = Width / 5;

            // icon
            // int iconSize = 16;
            // canvas.drawRect(x + (w - iconSize) / 2, 32 + 4, iconSize, iconSize);

            if (item.title) {
                canvas.setColor(enabled ? Color::Bright : Color::Medium);
                canvas.drawText(x + (w - canvas.textWidth(item.title) + 1) / 2, Height - 4, item.title);
            }
        }
    }
}

void ContextMenuPage::keyUp(KeyEvent &event) {
    const auto &key = event.key();

    if (_contextMenuModel->doubleClick()) {
        return;
    }
    if (!key.pageModifier() || !key.shiftModifier()) {
        close();
        event.consume();
    }
}

void ContextMenuPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.isFunction()) {
        int itemIndex = key.function();
        if (_contextMenuModel->itemEnabled(itemIndex)) {
            closeAndCallback(itemIndex);
        }
        event.consume();
    }
}

void ContextMenuPage::encoder(EncoderEvent &event) {
    event.consume();
}

void ContextMenuPage::closeAndCallback(int index) {
    close();
    if (_callback) {
        _callback(index);
    }
}
