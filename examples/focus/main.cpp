// Focus - a minimal Tavoos sample.
//
// A quiet, light focus timer with today's tasks: lots of whitespace, one typeface, one accent,
// hairline borders. The only effect is a single soft glass card over a faint blurred glow.

#include <tavoos/animation/animation.h>
#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/templates.h>
#include <tavoos/widget/widgets.h>
#include <tavoos/window.h>

#include <array>
#include <cstdio>
#include <ctime>
#include <deque>
#include <string>

using TB = Tavoos::Builder;
using namespace Tavoos;

namespace {

// ---------------------------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------------------------

const Color kPaper   = Color::rgba(247, 246, 243);
const Color kInk     = Color::rgba(24, 24, 27);
const Color kSoft    = Color::rgba(113, 113, 122);
const Color kFaint   = Color::rgba(161, 161, 170);
const Color kLine    = Color::rgba(228, 228, 231);
const Color kAccent  = Color::rgba(234, 88, 12);

const char* kFont = "Inter";

// ---------------------------------------------------------------------------------------------
// Model
// ---------------------------------------------------------------------------------------------

std::string today() {
    const std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof buf, "%A, %e %B", std::localtime(&now));
    std::string s = buf;
    if (auto pos = s.find("  "); pos != std::string::npos) s.erase(pos, 1);
    return s;
}

std::string mmss(int seconds) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d:%02d", seconds / 60, seconds % 60);
    return buf;
}

enum class Mode { Focus, ShortBreak, LongBreak };

struct Task {
    std::string title;
    std::string tag;
    State<Paint> ink{kInk};
};

struct Model {
    Mode mode{Mode::Focus};
    int focusMinutes{25};
    int total{25 * 60};
    int remaining{25 * 60};
    bool running{false};

    State<std::string> clock{mmss(25 * 60)};
    State<std::string> caption{"Session 2 of 4 \xC2\xB7 ready when you are"};
    State<std::string> startLabel{"Start"};
    State<int> elapsed{0};
    State<int> length{25 * 60};
    State<std::string> focusLengthText{"25 min"};

    std::deque<Task> tasks;
    int done{0};
    State<std::string> doneText{""};
    ColumnWidget* taskList{nullptr};
    ScrollAreaWidget* taskScroll{nullptr};

    int minutesFor(Mode m) const {
        switch (m) {
        case Mode::Focus: return focusMinutes;
        case Mode::ShortBreak: return 5;
        case Mode::LongBreak: return 15;
        }
        return focusMinutes;
    }

    void reset() {
        running = false;
        total = minutesFor(mode) * 60;
        remaining = total;
        length.set(total);
        elapsed.set(0);
        clock.set(mmss(remaining));
        startLabel.set("Start");
        caption.set(mode == Mode::Focus ? "Session 2 of 4 \xC2\xB7 ready when you are" : "Step away from the screen");
    }

    void setMode(Mode m) {
        mode = m;
        reset();
    }

    void toggle() {
        if (remaining == 0) reset();
        running = !running;
        startLabel.set(running ? "Pause" : "Resume");
        if (running) caption.set(mode == Mode::Focus ? "Deep work \xC2\xB7 notifications muted" : "On a break");
    }

    void tick() {
        if (!running || remaining == 0) return;
        --remaining;
        clock.set(mmss(remaining));
        elapsed.set(total - remaining);
        if (remaining == 0) {
            running = false;
            startLabel.set("Start again");
            caption.set(mode == Mode::Focus ? "Nice work. Take a short break." : "Break's over.");
        }
    }

    void updateDone() {
        doneText.set(std::to_string(done) + " of " + std::to_string(tasks.size()) + " done");
    }
} model;

class Ticker : public AnimatableBase {
    bool tick(float dt) override {
        m_acc += dt;
        while (m_acc >= 1.0f) {
            m_acc -= 1.0f;
            model.tick();
        }
        return true;
    }
    float m_acc{0.0f};
} ticker;

// ---------------------------------------------------------------------------------------------
// Custom controls
// ---------------------------------------------------------------------------------------------

// A text tab with a short accent underline when selected; join tabs to one ButtonGroup.
class Tab : public ButtonBase {
public:
    Tab(Object* parent) : ButtonBase{parent} {
        checkable(true).exclusive(true);
        height(32);
        checkedState().onChange([this](const bool& on) {
            m_ink.animateTo(on ? kInk : kFaint, 0.15f, Easing::easeOutQuad);
            m_bar.animateTo(on ? 1.0f : 0.0f, 0.2f, Easing::easeOutQuad);
            if (on) model.setMode(m_mode);
        });
        hoveredState().onChange([this](const bool& h) {
            if (!checked()) m_ink.animateTo(h ? kSoft : kFaint, 0.1f);
        });
        background([](RectangleWidget& box) { box.color(Color::Transparent); });
        content<TextWidget>([this](TextWidget& t) {
            t.text(m_caption).family(kFont).weight(FontWeight::Medium).fontSize(14).color(m_ink)
                .alignment(Alignment::Left | Alignment::Top);
        });
        // The underline is one more child of the control, sized by its own state.
        addChild<RectangleWidget>([this](RectangleWidget& bar) {
            bar.height(2).width(18).radius(1).alignment(Alignment::Left | Alignment::Bottom)
                .color(kAccent).opacity(m_bar);
        });
    }

    Tab& item(const std::string& caption, Mode mode, int width) {
        m_caption.set(caption);
        m_mode = mode;
        this->width(width);
        return *this;
    }

private:
    Mode m_mode{Mode::Focus};
    State<std::string> m_caption{""};
    AnimatedState<Paint> m_ink{kFaint};
    AnimatedState<float> m_bar{0.0f};
};

// A plain pill button: solid ink or a hairline outline.
class Pill : public ButtonBase {
public:
    Pill(Object* parent) : ButtonBase{parent} {
        height(44);
        hoveredState().onChange([this](const bool&) { restyle(); });
        pressedState().onChange([this](const bool&) { restyle(); });
        background([this](RectangleWidget& box) {
            box.radius(22).color(m_bg).borderWidth(m_borderW).borderColor(kLine);
        });
        content<TextWidget>([this](TextWidget& t) {
            t.text(m_caption).family(kFont).weight(FontWeight::Medium).fontSize(14).color(m_ink).alignment(Alignment::Center);
        });
    }

    Pill& solid(State<std::string>& caption) {
        m_solid = true;
        m_caption.set(caption.get());
        caption.onChange([this](const std::string& s) { m_caption.set(s); });
        m_ink.set(Color::White);
        m_borderW.set(0.0f);
        restyle();
        return *this;
    }

    Pill& outline(const std::string& caption) {
        m_solid = false;
        m_caption.set(caption);
        m_ink.set(kInk);
        m_borderW.set(1.0f);
        restyle();
        return *this;
    }

private:
    void restyle() {
        if (m_solid)
            m_bg.animateTo(pressed() ? Color::rgba(0, 0, 0) : hovered() ? Color::rgba(52, 52, 58) : kInk, 0.1f);
        else
            m_bg.animateTo(pressed() ? Color::rgba(232, 231, 228) : hovered() ? Color::rgba(240, 239, 236) : Color::rgba(255, 255, 255, 0), 0.1f);
    }

    bool m_solid{true};
    State<std::string> m_caption{""};
    State<Paint> m_ink{Color::White};
    State<float> m_borderW{0.0f};
    AnimatedState<Paint> m_bg{kInk};
};

// ---------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------

void text(const std::string& value, float size, FontWeight weight = FontWeight::Regular, Paint color = kInk) {
    TB::Text([=](TextWidget& t) {
        t.text(value).family(kFont).weight(weight).fontSize(size).color(color).alignment(Alignment::CenterVertical);
    });
}

void boundText(State<std::string>& value, float size, FontWeight weight = FontWeight::Regular, Paint color = kInk) {
    TB::Text([&value, size, weight, color](TextWidget& t) {
        t.text(value).family(kFont).weight(weight).fontSize(size).color(color).alignment(Alignment::CenterVertical);
    });
}

void spacer() {
    TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(Color::Transparent); });
}

void hairline() {
    TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(kLine); });
}

// One task row. Built with addChild so it also works for rows added after build().
void addTaskRow(ColumnWidget& list, Task& task, bool checked) {
    list.addChild<RowWidget>([&task, checked](RowWidget& row) {
        row.fill(Fill::Width).height(42).spacing(14);

        row.addChild<CheckboxWidget>([&task, checked](CheckboxWidget& c) {
            c.width(18).height(18).radius(5).checked(checked).alignment(Alignment::CenterVertical)
                .uncheckedColor(Color::White).uncheckedBorderColor(Color::rgba(212, 212, 216))
                .checkedColor(kInk).checkedBorderColor(kInk).checkColor(Color::White);
            c.checkedState().onChange([&task](const bool& on) {
                task.ink.set(on ? kFaint : kInk);
                model.done += on ? 1 : -1;
                model.updateDone();
            });
        });

        row.addChild<TextWidget>([&task](TextWidget& t) {
            t.text(task.title).family(kFont).fontSize(14).color(task.ink).alignment(Alignment::CenterVertical);
        });

        row.addChild<RectangleWidget>([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(Color::Transparent); });

        if (!task.tag.empty()) {
            row.addChild<TextWidget>([&task](TextWidget& t) {
                t.text(task.tag).family(kFont).fontSize(12).color(kFaint).alignment(Alignment::CenterVertical);
            });
        }
    });

    list.addChild<RectangleWidget>([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(kLine); });
}

void addTask(const std::string& title, const std::string& tag, bool checked) {
    Task& task = model.tasks.emplace_back();
    task.title = title;
    task.tag = tag;
    task.ink.set(checked ? kFaint : kInk);
    if (checked) ++model.done;
    if (model.taskList) addTaskRow(*model.taskList, task, checked);
    model.updateDone();
}

ButtonGroup modeGroup;

// ---------------------------------------------------------------------------------------------
// Sections
// ---------------------------------------------------------------------------------------------

void timerPane() {
    TB::Column([](ColumnWidget& pane) {
        pane.fill(Fill::Both).spacing(0);

        // Wordmark row.
        TB::Row([](RowWidget& row) {
            row.fill(Fill::Width).height(28).spacing(10);
            TB::Rectangle([](RectangleWidget& dot) {
                dot.width(10).height(10).radius(5).color(kAccent).alignment(Alignment::CenterVertical);
            });
            text("Focus", 15, FontWeight::SemiBold);
            spacer();
            text(today(), 13, FontWeight::Regular, kSoft);
        });

        TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(36).color(Color::Transparent); });

        // The timer card: the one glass surface, over a faint warm glow.
        TB::Rectangle([](RectangleWidget& stage) {
            stage.fill(Fill::Both).color(Color::Transparent);

            TB::Rectangle([](RectangleWidget& glow) {
                glow.width(300).height(300).radius(150).x(250).y(40).color(kAccent).opacity(0.18f).layerBlur(80);
            });

            TB::Rectangle([](RectangleWidget& card) {
                card.fill(Fill::Both).radius(28).padding(40)
                    .color(Color::rgba(255, 255, 255, 150))
                    .borderWidth(1).borderColor(Color::rgba(255, 255, 255, 220))
                    .glass({ .lightAngle = -30, .lightIntensity = 0.35f, .refraction = 0.25f, .depth = 24,
                             .dispersion = 0.0f, .frost = 40 })
                    .dropShadow({ .offsetY = 18, .radius = 48, .color = Color::rgba(24, 24, 27, 18) })
                    .dropShadow({ .offsetY = 1, .radius = 2, .color = Color::rgba(24, 24, 27, 14) });

                TB::Column([](ColumnWidget& c) {
                    c.fill(Fill::Both).spacing(0);

                    TB::Row([](RowWidget& tabs) {
                        tabs.spacing(28).height(32);
                        TB::Create<Tab>([](Tab& t) { t.item("Focus", Mode::Focus, 46).group(modeGroup).checked(true); });
                        TB::Create<Tab>([](Tab& t) { t.item("Short break", Mode::ShortBreak, 84).group(modeGroup); });
                        TB::Create<Tab>([](Tab& t) { t.item("Long break", Mode::LongBreak, 80).group(modeGroup); });
                    });

                    TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Both).color(Color::Transparent); });

                    TB::Text([](TextWidget& t) {
                        t.text(model.clock).family(kFont).weight(FontWeight::Light).fontSize(124).color(kInk).height(130);
                    });
                    TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(8).color(Color::Transparent); });
                    boundText(model.caption, 15, FontWeight::Regular, kSoft);

                    TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Both).color(Color::Transparent); });

                    TB::ProgressBar([](ProgressBarWidget& p) {
                        p.fill(Fill::Width).height(3).radius(2).minValue(0).maxValue(model.length).value(model.elapsed)
                            .trackColor(Color::rgba(24, 24, 27, 18)).fillColor(kAccent).transition(0.4f);
                    });

                    TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(28).color(Color::Transparent); });

                    TB::Row([](RowWidget& actions) {
                        actions.spacing(12).height(44);
                        TB::Create<Pill>([](Pill& p) {
                            p.solid(model.startLabel).width(132).onClick([](MouseEvent&) { model.toggle(); });
                        });
                        TB::Create<Pill>([](Pill& p) {
                            p.outline("Reset").width(96).onClick([](MouseEvent&) { model.reset(); });
                        });
                    });
                });
            });
        });
    });
}

void sidePane() {
    TB::Column([](ColumnWidget& side) {
        side.width(360).fill(Fill::Height).spacing(0).paddingTop(64);

        TB::Row([](RowWidget& head) {
            head.fill(Fill::Width).height(28);
            text("Today", 20, FontWeight::SemiBold);
            spacer();
            boundText(model.doneText, 13, FontWeight::Regular, kSoft);
        });

        TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(16).color(Color::Transparent); });
        hairline();

        // Five rows are visible; longer lists scroll.
        TB::ScrollArea([](ScrollAreaWidget& area) {
            model.taskScroll = &area;
            area.fill(Fill::Width).height(5 * 43).flickDirection(FlickDirection::Vertical)
                .backgroundColor(Color::Transparent).thumbColor(Color::rgba(24, 24, 27, 60)).barThickness(4);
            TB::Column([](ColumnWidget& list) {
                list.fill(Fill::Width).spacing(0);
                model.taskList = &list;
            });
        });

        // New tasks are typed here and appended to the list on Enter.
        TB::Row([](RowWidget& row) {
            row.fill(Fill::Width).height(42).spacing(14);
            TB::Text([](TextWidget& t) {
                t.text("+").family(kFont).weight(FontWeight::Light).fontSize(20).color(kFaint)
                    .width(18).alignment(Alignment::CenterVertical);
            });
            TB::TextField([](TextFieldWidget& f) {
                f.placeholder("Add a task").font(Font{kFont, 14.0f}).fill(Fill::Width).height(40)
                    .alignment(Alignment::CenterVertical).borderWidth(0.0f).radius(0).innerPadding(0)
                    .backgroundColor(Color::Transparent).textColor(kInk).placeholderColor(kFaint).caretColor(kAccent);
                f.onSubmit([&f](const std::string& value) {
                    if (value.empty()) return;
                    addTask(value, "", false);
                    f.text("");
                    if (model.taskScroll) model.taskScroll->scrollToBottom();
                });
            });
        });

        TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Both).color(Color::Transparent); });
        TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(20).color(Color::Transparent); });

        // Settings.
        text("SETTINGS", 11, FontWeight::Medium, kFaint);
        TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(14).color(Color::Transparent); });

        TB::Row([](RowWidget& row) {
            row.fill(Fill::Width).height(20);
            text("Focus length", 14);
            spacer();
            boundText(model.focusLengthText, 13, FontWeight::Regular, kSoft);
        });
        TB::Slider([](SliderWidget& s) {
            s.fill(Fill::Width).height(28).minValue(15).maxValue(60).value(25)
                .trackColor(kLine).fillColor(kInk).thumbColor(kInk);
            s.valueState().onChange([](const int& v) {
                model.focusMinutes = v;
                model.focusLengthText.set(std::to_string(v) + " min");
                if (model.mode == Mode::Focus && !model.running) model.reset();
            });
        });

        TB::Rectangle([](RectangleWidget& gap) { gap.fill(Fill::Width).height(10).color(Color::Transparent); });

        const std::array<std::pair<const char*, bool>, 2> switches{{ { "Chime when done", true }, { "Auto-start breaks", false } }};
        for (const auto& [name, on] : switches) {
            TB::Row([name, on](RowWidget& row) {
                row.fill(Fill::Width).height(40);
                text(name, 14);
                spacer();
                TB::Switch([on](SwitchWidget& s) {
                    s.width(36).height(20).checked(on).alignment(Alignment::CenterVertical)
                        .uncheckedColor(Color::rgba(228, 228, 231)).checkedColor(kInk)
                        .thumbColor(Color::White).checkedThumbColor(Color::White);
                });
            });
        }
    });
}

}  // namespace

class FocusWindow : public Window {
public:
    void build() override {
        TB::Row([](RowWidget& layout) {
            layout.fill(Fill::Both).paddingLeft(56).paddingRight(56).paddingTop(40).paddingBottom(44).spacing(56);
            timerPane();
            sidePane();
        });

        addTask("Outline the layout chapter", "Writing", true);
        addTask("Reply to design feedback", "Mail", true);
        addTask("Profile the blur pass", "Engine", false);
        addTask("Review text shaping PR", "Code", false);
        addTask("Plan next week", "", false);
    }
};

int main() {
    Application app;
    app.registerFont(kFont, "resource:/assets/fonts/Inter-Light.ttf", FontWeight::Light);
    app.registerFont(kFont, "resource:/assets/fonts/Inter-Regular.ttf", FontWeight::Regular);
    app.registerFont(kFont, "resource:/assets/fonts/Inter-Medium.ttf", FontWeight::Medium);
    app.registerFont(kFont, "resource:/assets/fonts/Inter-SemiBold.ttf", FontWeight::SemiBold);

    AnimationManager::instance().registerAnimation(&ticker);

    TB::createWindowOf<FocusWindow>([](Window& window) {
        window.width(1080).height(640).title("Focus").color(kPaper);
    });

    return app.run();
}
