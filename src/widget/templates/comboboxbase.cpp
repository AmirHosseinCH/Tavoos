#include <tavoos/widget/templates/comboboxbase.h>

#include <tavoos/builder.h>
#include <tavoos/events/key.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace Tavoos {

ComboBoxBase::ComboBoxBase(Object* parent) : ButtonBase{parent} {
    m_popupMaxHeight.onChange([this](const float&) { updateListHeight(); });
    m_currentIndex.onChange([this](const int& index) { applyCurrentIndex(index); });
    m_placeholder.onChange([this](const std::string&) { refreshDisplay(); });
}

ComboBoxBase::~ComboBoxBase() {
    detachModel();
}

void ComboBoxBase::runInBuilderScope(Widget& slot, const std::function<void()>& call) {
    Builder::Scope scope{&slot};
    call();
}

void ComboBoxBase::wirePopup() {
    m_popupOpened.setIfChanged(false);
    m_popup->openedState().onChange([this](const bool& opened) { m_popupOpened.setIfChanged(opened); });
}

void ComboBoxBase::wireList() {
    m_list->currentIndexState().onChange([this](const int&) { syncFromList(); });
    m_list->rowsHeightState().onChange([this](const float&) { updateListHeight(); });
    if (m_model) {
        m_list->model(*m_model);
        if (m_installRows)
            m_installRows();
    }
    if (built())
        m_list->currentIndex(m_currentIndexState.get());
    updateListHeight();
}

void ComboBoxBase::wireRow(ColumnWidget& row, State<int>& index) {
    row.onMouseEnter([this, &index](MouseEvent&) { m_list->currentIndex(index.get()); })
        .onClick([this, &index](MouseEvent&) { pick(index.get()); });
}

void ComboBoxBase::attachModel(ListModelBase& model) {
    detachModel();
    m_model = &model;
    m_customRows = false;
    m_textOf = nullptr;
    m_installRows = nullptr;
    if (m_list)
        m_list->model(model);
    m_modelSubscription = model.onChange([this](const ListChange& change) { modelChanged(change); });
    refreshDisplay();
}

void ComboBoxBase::detachModel() {
    if (m_model)
        m_model->removeOnChange(m_modelSubscription);
    m_model = nullptr;
    m_modelSubscription = 0;
}

void ComboBoxBase::reportModelMismatch() const {
    if (m_model)
        spdlog::error("ComboBox: the item type does not match the model");
    else
        spdlog::error("ComboBox: call model() before text() or delegate()");
}

void ComboBoxBase::reportMissingPopup() const {
    spdlog::error("ComboBox: call popup() before list()");
}

void ComboBoxBase::modelChanged(const ListChange& change) {
    using Kind = ListChange::Kind;

    if (change.kind == Kind::Destroyed) {
        m_model = nullptr;
        m_modelSubscription = 0;
        m_textOf = nullptr;
        m_installRows = nullptr;
        m_remembered = -1;
        setChosen(-1);
        return;
    }

    if (popupOpened()) {
        if (change.kind == Kind::Reset)
            m_remembered = -1;
        else if (change.kind != Kind::Changed)
            m_remembered = indexAfterChange(m_remembered, change);
        setChosen(m_remembered);
    } else if (!m_list && built()) {
        if (change.kind == Kind::Reset)
            setChosen(-1);
        else if (change.kind != Kind::Changed)
            setChosen(indexAfterChange(m_currentIndexState.get(), change));
    }
    refreshDisplay();
}

int ComboBoxBase::validIndex(int index) const {
    const int total = m_model ? m_model->size() : 0;
    return index >= 0 && index < total ? index : -1;
}

void ComboBoxBase::applyCurrentIndex(int index) {
    if (m_applying || !built())
        return;
    if (popupOpened()) {
        m_remembered = validIndex(index);
        setChosen(m_remembered);
        return;
    }
    if (!m_list) {
        setChosen(validIndex(index));
        return;
    }
    m_list->currentIndex(index);
    syncFromList();
}

void ComboBoxBase::setChosen(int index) {
    m_currentIndexState.setIfChanged(index);
    if (m_currentIndex.get() != index) {
        m_applying = true;
        m_currentIndex.set(index);
        m_applying = false;
    }
    refreshDisplay();
}

void ComboBoxBase::syncFromList() {
    if (!built() || !m_list || popupOpened())
        return;
    setChosen(m_list->currentIndex());
}

std::string ComboBoxBase::currentText() const {
    const int index = m_currentIndexState.get();
    if (!m_textOf || validIndex(index) < 0)
        return {};
    return m_textOf(index);
}

void ComboBoxBase::refreshDisplay() {
    const int index = m_currentIndexState.get();
    const bool hasText = m_textOf && validIndex(index) >= 0;
    m_displayText.setIfChanged(hasText ? m_textOf(index) : m_placeholder.get());
}

void ComboBoxBase::updateListHeight() {
    if (!m_list)
        return;
    const float fit = std::min(m_list->rowsHeight(), std::max(0.0f, m_popupMaxHeight.get()));
    m_list->height(std::max(1, static_cast<int>(std::ceil(fit))));
}

void ComboBoxBase::onBuilt() {
    const int requested = m_currentIndex.get();
    if (m_list) {
        m_list->currentIndex(requested);
        setChosen(m_list->currentIndex());
    } else {
        setChosen(validIndex(requested));
    }
    ButtonBase::onBuilt();
}

bool ComboBoxBase::popupOpened() const {
    return m_popup && m_popup->opened();
}

void ComboBoxBase::openPopup() {
    if (popupOpened() || !m_popup || !m_list || !m_model || !enabled())
        return;

    m_remembered = m_currentIndexState.get();
    m_popup->width(static_cast<int>(std::lround(displayedWidth())));
    updateListHeight();
    if (m_remembered >= 0)
        m_list->scrollToIndex(m_remembered);
    m_popup->open();
    m_list->focus();
}

void ComboBoxBase::closePopup() {
    if (popupOpened())
        m_popup->close();
}

void ComboBoxBase::pick(int index) {
    if (!popupOpened())
        return;

    m_picking = true;
    m_list->currentIndex(index);
    m_popup->close();
    m_picking = false;
    setChosen(m_list->currentIndex());
    if (m_onActivated)
        m_onActivated(currentIndex());
}

void ComboBoxBase::popupClosed() {
    if (!m_list)
        return;
    if (!m_picking)
        m_list->currentIndex(m_remembered);
    syncFromList();
    if (m_list->focused())
        focus();
}

void ComboBoxBase::stepChosen(int delta) {
    const int total = m_model ? m_model->size() : 0;
    if (total == 0)
        return;

    const int current = currentIndex();
    const int next = std::clamp(current < 0 ? 0 : current + delta, 0, total - 1);
    if (next == current)
        return;
    if (m_list)
        m_list->currentIndex(next);
    else
        setChosen(next);
    if (m_onActivated)
        m_onActivated(currentIndex());
}

void ComboBoxBase::handleClick(MouseEvent& event) {
    if (popupOpened())
        closePopup();
    else
        openPopup();
    ButtonBase::handleClick(event);
}

void ComboBoxBase::triggerKeyPress(KeyEvent& event) {
    const int key = event.keyCode();
    const bool enter = key == static_cast<int>(Key::Enter) || key == static_cast<int>(Key::KpEnter);

    if (popupOpened()) {
        if (enter) {
            const int index = m_list->currentIndex();
            if (index >= 0)
                pick(index);
            else
                closePopup();
        } else {
            event.ignore();
        }
        Widget::triggerKeyPress(event);
        return;
    }

    if (enabled() && (key == static_cast<int>(Key::Up) || key == static_cast<int>(Key::Down))) {
        stepChosen(key == static_cast<int>(Key::Up) ? -1 : 1);
        Widget::triggerKeyPress(event);
        return;
    }
    ButtonBase::triggerKeyPress(event);
}

}
