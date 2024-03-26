#include "BasePage.h"

#include "Pages.h"

#include "ui/model/ContextMenuModel.h"

BasePage::BasePage(PageManager &manager, PageContext &context) :
    Page(manager),
    _context(context),
    _model(context.model),
    _project(context.model.project()),
    _engine(context.engine)
{}

void BasePage::showMessage(const char *text, uint32_t duration) {
    _context.messageManager.showMessage(text, duration);
}

void BasePage::showContextMenu(const ContextMenu &contextMenu) {
    _context.contextMenu = contextMenu;
    _manager.pages().contextMenu.show(_context.contextMenu, _context.contextMenu.actionCallback());
}

void BasePage::functionShortcuts(KeyPressEvent event) {
    {
        const auto &key = event.key();
        if (key.isFunction() && key.is(Key::F0) && event.count() == 2) {
            _manager.pages().top.setMode(TopPage::Mode::Project);
        }
        if (key.isFunction() && key.is(Key::F1) && event.count() == 2) {
            _manager.pages().top.setMode(TopPage::Mode::Layout);
        }

        if (key.isFunction() && key.is(Key::F2) && event.count() == 2) {
            _manager.pages().top.setMode(TopPage::Mode::Routing);
        }

        if (key.isFunction() && key.is(Key::F3) && event.count() == 2) {
            _manager.pages().top.setMode(TopPage::Mode::MidiOutput);
        }

        if (key.isFunction() && key.is(Key::F4) && event.count() == 2) {
            _manager.pages().top.setMode(TopPage::Mode::UserScale);
        }
    }
}
