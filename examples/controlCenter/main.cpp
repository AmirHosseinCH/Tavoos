// Control Center - a Tavoos showcase app.
//
// Frosted-glass panels over a blurred, colorful backdrop, with switches, sliders, a segmented
// control, accent swatches, a checkbox, a progress bar, a spin box and a text field. Picking an
// accent swatch recolors every accent-bound widget live through one shared State<Paint>.

#include <tavoos/animation/animation.h>
#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/templates.h>
#include <tavoos/widget/widgets.h>
#include <tavoos/window.h>

#include <array>
#include <cstdio>
#include <string>

using TB = Tavoos::Builder;
using namespace Tavoos;

namespace {

// ---------------------------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------------------------

const Color kBackdrop = Color::rgba(11, 13, 26);
const Color kInk      = Color::rgba(244, 244, 250);
const Color kMuted    = Color::rgba(235, 235, 245, 175);
const Color kViolet   = Color::rgba(124, 92, 255);
const Color kCoral    = Color::rgba(255, 94, 126);
const Color kTeal     = Color::rgba(34, 195, 181);
const Color kAmber    = Color::rgba(255, 181, 71);
const Color kSky      = Color::rgba(75, 163, 255);

const char* kFont = "Inter";

// ---------------------------------------------------------------------------------------------
// Icons (inline SVG, recolored by SVGWidget::color)
// ---------------------------------------------------------------------------------------------

std::string svg(const char* body, const char* extra = "fill='none' stroke='#fff' stroke-width='1.8' stroke-linecap='round' stroke-linejoin='round'") {
    return std::string("data:<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' ") + extra + ">" + body + "</svg>";
}

const std::string kIconGrid     = svg("<rect x='3' y='3' width='7' height='7' rx='2'/><rect x='14' y='3' width='7' height='7' rx='2'/><rect x='3' y='14' width='7' height='7' rx='2'/><rect x='14' y='14' width='7' height='7' rx='2'/>");
const std::string kIconMusic    = svg("<path d='M9 18V5l12-2v13'/><circle cx='6' cy='18' r='3'/><circle cx='18' cy='16' r='3'/>");
const std::string kIconBulb     = svg("<path d='M9 18h6'/><path d='M10 22h4'/><path d='M12 2a7 7 0 0 0-4 12.7V16h8v-1.3A7 7 0 0 0 12 2z'/>");
const std::string kIconMonitor  = svg("<rect x='2' y='4' width='20' height='13' rx='2'/><path d='M8 21h8'/><path d='M12 17v4'/>");
const std::string kIconGear     = svg("<circle cx='12' cy='12' r='3'/><path d='M12 2v3M12 19v3M4.2 4.2l2.1 2.1M17.7 17.7l2.1 2.1M2 12h3M19 12h3M4.2 19.8l2.1-2.1M17.7 6.3l2.1-2.1'/>");
const std::string kIconSearch   = svg("<circle cx='11' cy='11' r='7'/><path d='M20 20l-3.5-3.5'/>");
const std::string kIconShuffle  = svg("<path d='M16 3h5v5'/><path d='M4 20L21 3'/><path d='M21 16v5h-5'/><path d='M15 15l6 6'/><path d='M4 4l5 5'/>");
const std::string kIconRepeat   = svg("<path d='M17 1l4 4-4 4'/><path d='M3 11V9a4 4 0 0 1 4-4h14'/><path d='M7 23l-4-4 4-4'/><path d='M21 13v2a4 4 0 0 1-4 4H3'/>");
const std::string kIconPrev     = svg("<path d='M7 6h2v12H7z'/><path d='M19 6v12l-9-6z'/>", "fill='#fff'");
const std::string kIconNext     = svg("<path d='M15 6h2v12h-2z'/><path d='M5 6v12l9-6z'/>", "fill='#fff'");
const std::string kIconPlay     = svg("<path d='M8 5v14l11-7z'/>", "fill='#fff'");
const std::string kIconPause    = svg("<rect x='6' y='5' width='4' height='14' rx='1'/><rect x='14' y='5' width='4' height='14' rx='1'/>", "fill='#fff'");
const std::string kIconSun      = svg("<circle cx='12' cy='12' r='4'/><path d='M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4'/>");
const std::string kIconVolume   = svg("<path d='M11 5L6 9H2v6h4l5 4z'/><path d='M15.5 8.5a5 5 0 0 1 0 7'/><path d='M19 5a10 10 0 0 1 0 14'/>");
const std::string kIconThermo   = svg("<path d='M14 14.8V4a2 2 0 0 0-4 0v10.8a4 4 0 1 0 4 0z'/>");
const std::string kLogo         = svg("<circle cx='12' cy='12' r='10.5' stroke='#7C5CFF'/><circle cx='12' cy='12' r='6' stroke='#FF5E7E'/><circle cx='12' cy='12' r='2.2' fill='#22C3B5' stroke='none'/>",
                                      "fill='none' stroke-width='1.8'");

// ---------------------------------------------------------------------------------------------
// App state
// ---------------------------------------------------------------------------------------------

std::string clock(int seconds) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d:%02d", seconds / 60, seconds % 60);
    return buf;
}

std::string percent(int value) { return std::to_string(value) + "%"; }

std::string celsius(int tenths) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%.1f\xC2\xB0", tenths / 10.0);
    return buf;
}

constexpr int kTrackLength = 238;
constexpr int kLibrarySize = 2000;

ShadowStyle glowFor(Color c) {
    c.a = 0.55f;
    return { .offsetY = 8, .radius = 22, .color = c };
}

struct Model {
    State<Paint> accent{kViolet};
    State<ShadowStyle> accentGlow{glowFor(kViolet)};

    void setAccent(const Color& c) {
        accent.set(c);
        accentGlow.set(glowFor(c));
    }

    // Now playing
    SliderWidget* seek{nullptr};
    State<bool> playing{true};
    State<std::string> playIcon{kIconPause};
    State<std::string> elapsed{clock(102)};

    // Sound & light
    State<std::string> brightnessText{percent(72)};
    State<std::string> volumeText{percent(45)};
    State<std::string> temperatureText{celsius(215)};

    // Library sync
    State<int> synced{1284};
    State<std::string> syncPercent{percent(64)};
    State<std::string> syncTracks{"1,284 / 2,000 tracks"};
    State<bool> syncPaused{false};
    State<std::string> syncBadge{"Syncing"};
    State<std::string> syncButton{"Pause sync"};

    void setSynced(int value) {
        synced.set(value);
        syncPercent.set(percent(value * 100 / kLibrarySize));
        const std::string n = std::to_string(value);
        const std::string grouped = value >= 1000 ? n.substr(0, n.size() - 3) + "," + n.substr(n.size() - 3) : n;
        syncTracks.set(grouped + " / 2,000 tracks");
        if (value >= kLibrarySize) syncBadge.set("Up to date");
    }
} model;

// Advances the song while playing and the library sync while not paused.
class Ticker : public AnimatableBase {
    bool tick(float dt) override {
        m_song += dt;
        m_sync += dt;
        if (m_song >= 1.0f) {
            m_song = 0.0f;
            if (model.playing.get() && model.seek && !model.seek->pressed())
                model.seek->value((model.seek->value() + 1) % (kTrackLength + 1));
        }
        if (m_sync >= 0.25f) {
            m_sync = 0.0f;
            if (!model.syncPaused.get() && model.synced.get() < kLibrarySize)
                model.setSynced(model.synced.get() + 1);
        }
        return true;
    }
    float m_song{0.0f};
    float m_sync{0.0f};
} ticker;

// ---------------------------------------------------------------------------------------------
// Custom controls
// ---------------------------------------------------------------------------------------------

// A round icon button with a soft hover/press fill. `filled` makes it the accent-colored hero.
class IconButton : public ButtonBase {
public:
    IconButton(Object* parent) : ButtonBase{parent} {
        hoveredState().onChange([this](const bool&) { updateFill(); });
        pressedState().onChange([this](const bool&) { updateFill(); });
        background([this](RectangleWidget& disc) { disc.radius(m_radius).color(m_paint); });
        content<SVGWidget>([this](SVGWidget& icon) {
            icon.source(m_icon).width(m_iconSize).height(m_iconSize).alignment(Alignment::Center).color(m_iconColor);
        });
    }

    IconButton& icon(const std::string& source) { m_icon.set(source); return *this; }
    IconButton& size(int px, int iconPx) {
        width(px).height(px);
        m_radius.set(px / 2);
        m_iconSize.set(iconPx);
        return *this;
    }
    IconButton& ghost() { m_base = Color::Transparent; m_iconColor.set(kMuted); updateFill(); return *this; }
    IconButton& soft() { m_base = Color::rgba(255, 255, 255, 30); updateFill(); return *this; }

private:
    void updateFill() {
        Color c = m_base;
        if (pressed()) c = Color::rgba(255, 255, 255, 70);
        else if (hovered()) c = Color::rgba(255, 255, 255, 45);
        m_paint.set(c);
    }

    Color m_base{Color::rgba(255, 255, 255, 30)};
    State<Paint> m_paint{Color::rgba(255, 255, 255, 30)};
    State<Paint> m_iconColor{kInk};
    State<std::string> m_icon{""};
    State<int> m_radius{22};
    State<int> m_iconSize{18};
};

// The accent play/pause button: a filled disc with a colored glow.
class PlayButton : public ButtonBase {
public:
    PlayButton(Object* parent) : ButtonBase{parent} {
        width(56).height(56);
        hoveredState().onChange([this](const bool& h) { m_scale.animateTo(h ? 1.06f : 1.0f, 0.12f, Easing::easeOutQuad); });
        pressedState().onChange([this](const bool& p) { m_scale.animateTo(p ? 0.94f : (hovered() ? 1.06f : 1.0f), 0.08f); });
        background([this](RectangleWidget& disc) {
            disc.radius(28).color(model.accent).scale(m_scale)
                .dropShadow(model.accentGlow);
        });
        content<SVGWidget>([this](SVGWidget& icon) {
            icon.source(model.playIcon).width(20).height(20).alignment(Alignment::Center).color(Color::White).scale(m_scale);
        });
        onClick([](MouseEvent&) {
            const bool now = !model.playing.get();
            model.playing.set(now);
            model.playIcon.set(now ? kIconPause : kIconPlay);
        });
    }

private:
    AnimatedState<float> m_scale{1.0f};
};

// A sidebar navigation entry: icon + label, highlighted when checked or hovered.
class NavItem : public ButtonBase {
public:
    NavItem(Object* parent) : ButtonBase{parent} {
        checkable(true).exclusive(true);
        height(44);
        checkedState().onChange([this](const bool&) { update(); });
        hoveredState().onChange([this](const bool&) { update(); });
        background([this](RectangleWidget& box) {
            box.radius(12).color(m_paint).borderWidth(1).borderColor(m_border);
        });
        content<RowWidget>([this](RowWidget& row) {
            // Slot bodies run after build(), so children are added with addChild, not TB::.
            row.spacing(12).paddingLeft(12).fill(Fill::Both);
            row.addChild<SVGWidget>([this](SVGWidget& icon) {
                icon.source(m_icon).width(18).height(18).alignment(Alignment::CenterVertical).color(m_text);
            });
            row.addChild<TextWidget>([this](TextWidget& t) {
                t.text(m_label).family(kFont).weight(FontWeight::Medium).fontSize(14).color(m_text)
                    .alignment(Alignment::CenterVertical);
            });
        });
    }

    NavItem& item(const std::string& icon, const std::string& label) {
        m_icon.set(icon);
        m_label.set(label);
        return *this;
    }

private:
    void update() {
        const bool on = checked();
        m_paint.set(on ? Color::rgba(255, 255, 255, 42) : hovered() ? Color::rgba(255, 255, 255, 18) : Color::Transparent);
        m_border.set(on ? Color::rgba(255, 255, 255, 40) : Color::Transparent);
        m_text.set(on ? Color::White : kMuted);
    }

    State<std::string> m_icon{""};
    State<std::string> m_label{""};
    State<Paint> m_paint{Color::Transparent};
    State<Paint> m_border{Color::Transparent};
    State<Paint> m_text{kMuted};
};

// One option of a segmented control; join several to a ButtonGroup.
class Segment : public ButtonBase {
public:
    Segment(Object* parent) : ButtonBase{parent} {
        checkable(true).exclusive(true);
        height(36).fill(Fill::Width);
        checkedState().onChange([this](const bool& on) {
            m_paint.animateTo(on ? Color::rgba(255, 255, 255, 235) : Color::rgba(255, 255, 255, 0), 0.15f, Easing::easeOutQuad);
            m_text.animateTo(on ? Color::rgba(20, 20, 43) : Color::rgba(240, 240, 250, 205), 0.15f, Easing::easeOutQuad);
        });
        background([this](RectangleWidget& box) { box.radius(10).color(m_paint); });
        content<TextWidget>([this](TextWidget& t) {
            t.text(m_label).family(kFont).weight(FontWeight::SemiBold).fontSize(13).color(m_text).alignment(Alignment::Center);
        });
    }

    Segment& label(const std::string& text) { m_label.set(text); return *this; }

private:
    State<std::string> m_label{""};
    AnimatedState<Paint> m_paint{Color::rgba(255, 255, 255, 0)};
    AnimatedState<Paint> m_text{Color::rgba(240, 240, 250, 205)};
};

// A color swatch; checking it makes its color the app-wide accent.
class Swatch : public ButtonBase {
public:
    Swatch(Object* parent) : ButtonBase{parent} {
        checkable(true).exclusive(true);
        width(32).height(32);
        checkedState().onChange([this](const bool& on) {
            m_ring.animateTo(on ? Color::White : Color::rgba(255, 255, 255, 0), 0.15f);
            if (on) model.setAccent(m_swatch);
        });
        background([this](RectangleWidget& dot) {
            dot.radius(16).color(m_color).borderWidth(3).borderColor(m_ring);
        });
    }

    Swatch& color(const Color& c) { m_swatch = c; m_color.set(c); return *this; }

private:
    Color m_swatch{Color::White};
    State<Paint> m_color{Color::White};
    AnimatedState<Paint> m_ring{Color::rgba(255, 255, 255, 0)};
};

// A tall, pill-shaped "fill" slider in the style of a control-center brightness bar.
class FillSlider : public SliderBase {
public:
    FillSlider(Object* parent) : SliderBase{parent} {
        height(28).fill(Fill::Width);
        background([this](RectangleWidget& track) {
            track.radius(14).color(Color::rgba(255, 255, 255, 36)).clip(true);
            track.addChild<RectangleWidget>([this](RectangleWidget& level) {
                level.fill(Fill::Height).alignment(Alignment::Left).widthFraction(positionState()).color(m_paint);
            });
        });
        handle<RectangleWidget>([this](RectangleWidget& grip) {
            grip.width(4).height(14).radius(2).marginLeft(-10)
                .alignment(Alignment::CenterVertical).xFraction(positionState())
                .color(Color::rgba(255, 255, 255, 220));
        });
    }

    FillSlider& fillColor(const Color& c) { m_paint.set(c); return *this; }
    FillSlider& fillColor(State<Paint>& c) { m_paint.set(c.get()); c.onChange([this](const Paint& p) { m_paint.set(p); }); return *this; }

private:
    State<Paint> m_paint{Color::White};
};

// ---------------------------------------------------------------------------------------------
// Building blocks
// ---------------------------------------------------------------------------------------------

// The shared look of every panel: a pane of frosted glass with a soft lift.
void glass(RectangleWidget& r, int radius = 24) {
    r.radius(radius)
        .color(Color::rgba(255, 255, 255, 22))
        .borderWidth(1).borderColor(Color::rgba(255, 255, 255, 38))
        .glass({ .lightAngle = -45, .lightIntensity = 0.55f, .refraction = 0.45f, .depth = 16,
                 .dispersion = 0.15f, .frost = 28, .saturation = 1.35f })
        .dropShadow({ .offsetY = 20, .radius = 40, .color = Color::rgba(4, 4, 16, 90) });
}

void spacer() {
    TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(Color::Transparent); });
}

void label(const std::string& text, float size, FontWeight weight = FontWeight::Regular, Paint color = kInk) {
    TB::Text([=](TextWidget& t) {
        t.text(text).family(kFont).weight(weight).fontSize(size).color(color).alignment(Alignment::CenterVertical);
    });
}

void cardTitle(const std::string& text) { label(text, 15, FontWeight::SemiBold); }

void icon(const std::string& source, int size, Paint color = kInk) {
    TB::SVG([=](SVGWidget& s) {
        s.source(source).width(size).height(size).color(color).alignment(Alignment::CenterVertical);
    });
}

void blob(int x, int y, int size, Color color, float opacity) {
    TB::Rectangle([=](RectangleWidget& r) {
        r.x(x).y(y).width(size).height(size).radius(size / 2).color(color).opacity(opacity).layerBlur(90);
    });
}

// ---------------------------------------------------------------------------------------------
// Sections
// ---------------------------------------------------------------------------------------------

ButtonGroup navGroup;
ButtonGroup themeGroup;
ButtonGroup swatchGroup;

void sidebar() {
    TB::Rectangle([](RectangleWidget& panel) {
        glass(panel, 22);
        panel.width(216).fill(Fill::Height).padding(14);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(6);

            TB::Row([](RowWidget& brand) {
                brand.spacing(10).paddingLeft(8).paddingTop(6).height(54);
                icon(kLogo, 30, Color::White);
                label("Tavoos", 18, FontWeight::Bold);
            });

            const std::array<std::pair<const std::string*, const char*>, 5> items{{
                { &kIconGrid, "Overview" }, { &kIconMusic, "Media" }, { &kIconBulb, "Lighting" },
                { &kIconMonitor, "Devices" }, { &kIconGear, "Settings" },
            }};
            for (std::size_t i = 0; i < items.size(); ++i) {
                TB::Create<NavItem>([&, i](NavItem& n) {
                    n.item(*items[i].first, items[i].second).fill(Fill::Width).group(navGroup).checked(i == 0);
                });
            }

            TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Both).color(Color::Transparent); });

            TB::Rectangle([](RectangleWidget& box) {
                box.fill(Fill::Width).height(86).radius(16).padding(14)
                    .color(Color::rgba(255, 255, 255, 20)).borderWidth(1).borderColor(Color::rgba(255, 255, 255, 26));
                TB::Column([](ColumnWidget& c) {
                    c.fill(Fill::Both).spacing(10);
                    TB::Row([](RowWidget& row) {
                        row.fill(Fill::Width).height(24);
                        label("Focus mode", 13, FontWeight::SemiBold);
                        spacer();
                        TB::Switch([](SwitchWidget& s) {
                            s.width(40).height(24).checked(true).alignment(Alignment::CenterVertical)
                                .uncheckedColor(Color::rgba(255, 255, 255, 55)).checkedColor(model.accent)
                                .thumbColor(Color::White).checkedThumbColor(Color::White);
                        });
                    });
                    label("Alerts paused until 22:00", 12, FontWeight::Regular, kMuted);
                });
            });
        });
    });
}

void header() {
    TB::Row([](RowWidget& row) {
        row.fill(Fill::Width).height(56).paddingLeft(4);

        TB::Column([](ColumnWidget& c) {
            c.spacing(4).alignment(Alignment::CenterVertical);
            label("Good evening", 28, FontWeight::Bold);
            label("Living room \xC2\xB7 4 devices active", 14, FontWeight::Regular, kMuted);
        });

        spacer();

        TB::Rectangle([](RectangleWidget& box) {
            glass(box, 14);
            box.clearEffects();  // no lift on the search pill
            box.width(280).height(44).alignment(Alignment::CenterVertical).paddingLeft(14);
            TB::Row([](RowWidget& r) {
                r.fill(Fill::Both).spacing(8);
                icon(kIconSearch, 16, kMuted);
                TB::TextField([](TextFieldWidget& f) {
                    f.placeholder("Search devices, scenes\xE2\x80\xA6").font(Font{kFont, 14.0f})
                        .fill(Fill::Both).borderWidth(0.0f).radius(0).innerPadding(4)
                        .backgroundColor(Color::Transparent).textColor(kInk)
                        .placeholderColor(Color::rgba(235, 235, 245, 140)).caretColor(Color::White);
                });
            });
        });
    });
}

void nowPlaying() {
    TB::Rectangle([](RectangleWidget& card) {
        glass(card);
        card.fill(Fill::Width).height(214).gridColumnSpan(2).padding(24);

        TB::Row([](RowWidget& row) {
            row.fill(Fill::Both).spacing(22);

            // Album art: a gradient square with a radial glow and a ring.
            TB::Rectangle([](RectangleWidget& art) {
                art.width(156).height(156).radius(18).alignment(Alignment::CenterVertical).clip(true)
                    .color(linearGradient({ { 0.0f, kCoral }, { 0.55f, Color::rgba(109, 74, 255) }, { 1.0f, Color::rgba(42, 30, 107) } }, 135.0f))
                    .dropShadow({ .offsetY = 12, .radius = 28, .color = Color::rgba(20, 8, 60, 140) });
                TB::Rectangle([](RectangleWidget& glow) {
                    glow.fill(Fill::Both).color(radialGradient({ { 0.0f, Color::rgba(255, 210, 140, 150) }, { 1.0f, Color::rgba(255, 210, 140, 0) } }, 0.25f, 0.2f, 0.7f));
                });
                TB::Rectangle([](RectangleWidget& ring) {
                    ring.width(62).height(62).radius(31).alignment(Alignment::Center)
                        .color(Color::Transparent).borderWidth(2).borderColor(Color::rgba(255, 255, 255, 200));
                });
            });

            TB::Column([](ColumnWidget& info) {
                info.fill(Fill::Both).spacing(12);

                TB::Column([](ColumnWidget& c) {
                    c.spacing(4);
                    label("NOW PLAYING", 11, FontWeight::SemiBold, kMuted);
                    label("Midnight Drive", 22, FontWeight::Bold);
                    label("Neon Avenue \xC2\xB7 Afterglow", 14, FontWeight::Regular, kMuted);
                });

                TB::Slider([](SliderWidget& s) {
                    model.seek = &s;
                    s.fill(Fill::Width).minValue(0).maxValue(kTrackLength).value(102)
                        .trackColor(Color::rgba(255, 255, 255, 50)).fillColor(Color::White).thumbColor(Color::White);
                    s.valueState().onChange([](const int& v) { model.elapsed.set(clock(v)); });
                });

                TB::Row([](RowWidget& times) {
                    times.fill(Fill::Width).height(14).marginTop(-6);
                    TB::Text([](TextWidget& t) { t.text(model.elapsed).family(kFont).fontSize(12).color(kMuted); });
                    spacer();
                    label(clock(kTrackLength), 12, FontWeight::Regular, kMuted);
                });

                TB::Row([](RowWidget& controls) {
                    controls.spacing(14).height(56);
                    TB::Create<IconButton>([](IconButton& b) { b.icon(kIconShuffle).size(40, 18).ghost().alignment(Alignment::CenterVertical); });
                    TB::Create<IconButton>([](IconButton& b) { b.icon(kIconPrev).size(44, 18).soft().alignment(Alignment::CenterVertical)
                        .onClick([](MouseEvent&) { if (model.seek) model.seek->value(0); }); });
                    TB::Create<PlayButton>([](PlayButton& b) { b.alignment(Alignment::CenterVertical); });
                    TB::Create<IconButton>([](IconButton& b) { b.icon(kIconNext).size(44, 18).soft().alignment(Alignment::CenterVertical)
                        .onClick([](MouseEvent&) { if (model.seek) model.seek->value(0); }); });
                    TB::Create<IconButton>([](IconButton& b) { b.icon(kIconRepeat).size(40, 18).ghost().alignment(Alignment::CenterVertical); });
                });
            });
        });
    });
}

struct ToggleDef {
    const char* label;
    const char* on;
    const char* off;
    bool checked;
};

std::array<State<std::string>, 4> toggleSubs;

void quickToggles() {
    TB::Rectangle([](RectangleWidget& card) {
        glass(card);
        card.fill(Fill::Width).height(214).paddingLeft(20).paddingRight(20).paddingTop(16).paddingBottom(12);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(2);
            TB::Row([](RowWidget& r) { r.height(26); cardTitle("Quick toggles"); });

            static const std::array<ToggleDef, 4> defs{{
                { "Wi-Fi", "Home-5G", "Off", true },
                { "Bluetooth", "2 connected", "Off", true },
                { "Do not disturb", "On", "Off", false },
                { "Night light", "Until 07:00", "Off", true },
            }};

            for (std::size_t i = 0; i < defs.size(); ++i) {
                toggleSubs[i].set(defs[i].checked ? defs[i].on : defs[i].off);
                TB::Row([i](RowWidget& row) {
                    row.fill(Fill::Width).height(38);
                    TB::Column([i](ColumnWidget& c) {
                        c.alignment(Alignment::CenterVertical).spacing(1);
                        label(defs[i].label, 14, FontWeight::Medium);
                        TB::Text([i](TextWidget& t) { t.text(toggleSubs[i]).family(kFont).fontSize(12).color(kMuted); });
                    });
                    spacer();
                    TB::Switch([i](SwitchWidget& s) {
                        s.width(44).height(26).alignment(Alignment::CenterVertical).checked(defs[i].checked)
                            .uncheckedColor(Color::rgba(255, 255, 255, 55)).checkedColor(model.accent)
                            .thumbColor(Color::White).checkedThumbColor(Color::White);
                        s.checkedState().onChange([i](const bool& on) { toggleSubs[i].set(on ? defs[i].on : defs[i].off); });
                    });
                });
            }
        });
    });
}

void levelRow(const std::string& iconSource, const std::string& name, State<std::string>& value) {
    TB::Row([&](RowWidget& row) {
        row.fill(Fill::Width).height(18).spacing(8);
        icon(iconSource, 16);
        label(name, 13, FontWeight::Medium);
        spacer();
        TB::Text([&](TextWidget& t) { t.text(value).family(kFont).fontSize(13).color(kMuted).alignment(Alignment::CenterVertical); });
    });
}

void soundAndLight() {
    TB::Rectangle([](RectangleWidget& card) {
        glass(card);
        card.fill(Fill::Width).height(288).padding(20);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(14);
            cardTitle("Sound & light");

            TB::Column([](ColumnWidget& c) {
                c.fill(Fill::Width).spacing(10);
                levelRow(kIconSun, "Brightness", model.brightnessText);
                TB::Create<FillSlider>([](FillSlider& s) {
                    s.fillColor(kAmber).value(72);
                    s.valueState().onChange([](const int& v) { model.brightnessText.set(percent(v)); });
                });
            });

            TB::Column([](ColumnWidget& c) {
                c.fill(Fill::Width).spacing(10);
                levelRow(kIconVolume, "Volume", model.volumeText);
                TB::Create<FillSlider>([](FillSlider& s) {
                    s.fillColor(model.accent).value(45);
                    s.valueState().onChange([](const int& v) { model.volumeText.set(percent(v)); });
                });
            });

            TB::Column([](ColumnWidget& c) {
                c.fill(Fill::Width).spacing(8);
                levelRow(kIconThermo, "Temperature", model.temperatureText);
                TB::Slider([](SliderWidget& s) {
                    s.fill(Fill::Width).minValue(160).maxValue(280).value(215)
                        .trackColor(Color::rgba(255, 255, 255, 50)).fillColor(kTeal).thumbColor(Color::White);
                    s.valueState().onChange([](const int& v) { model.temperatureText.set(celsius(v)); });
                });
            });
        });
    });
}

void appearance() {
    TB::Rectangle([](RectangleWidget& card) {
        glass(card);
        card.fill(Fill::Width).height(288).padding(20);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(16);
            cardTitle("Appearance");

            TB::Rectangle([](RectangleWidget& track) {
                track.fill(Fill::Width).height(44).radius(14).padding(4).color(Color::rgba(0, 0, 0, 60));
                TB::Row([](RowWidget& r) {
                    r.fill(Fill::Both).spacing(4);
                    for (const char* name : { "Light", "Dark", "Auto" }) {
                        TB::Create<Segment>([name](Segment& s) {
                            s.label(name).group(themeGroup).checked(std::string(name) == "Dark");
                        });
                    }
                });
            });

            TB::Column([](ColumnWidget& c) {
                c.spacing(10);
                label("Accent", 13, FontWeight::Regular, kMuted);
                TB::Row([](RowWidget& r) {
                    r.spacing(10);
                    for (const Color& color : { kViolet, kCoral, kTeal, kAmber, kSky }) {
                        TB::Create<Swatch>([color](Swatch& s) {
                            s.color(color).group(swatchGroup).checked(color.r == kViolet.r && color.g == kViolet.g);
                        });
                    }
                });
            });

            TB::Row([](RowWidget& r) {
                r.spacing(10).height(22).marginTop(2);
                TB::Checkbox([](CheckboxWidget& c) {
                    c.width(20).height(20).radius(6).checked(true).alignment(Alignment::CenterVertical)
                        .checkedColor(model.accent).checkedBorderColor(model.accent)
                        .uncheckedColor(Color::rgba(255, 255, 255, 20)).uncheckedBorderColor(Color::rgba(255, 255, 255, 110))
                        .checkColor(Color::White);
                });
                label("Reduce transparency", 14);
            });
        });
    });
}

void librarySync() {
    TB::Rectangle([](RectangleWidget& card) {
        glass(card);
        card.fill(Fill::Width).height(288).padding(20);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(16);

            TB::Row([](RowWidget& r) {
                r.fill(Fill::Width).height(26);
                cardTitle("Library sync");
                spacer();
                TB::Rectangle([](RectangleWidget& badge) {
                    badge.height(24).width(92).radius(12).alignment(Alignment::CenterVertical)
                        .color(Color::rgba(34, 195, 181, 56));
                    TB::Text([](TextWidget& t) {
                        t.text(model.syncBadge).family(kFont).weight(FontWeight::SemiBold).fontSize(12)
                            .color(Color::rgba(143, 240, 230)).alignment(Alignment::Center);
                    });
                });
            });

            TB::Column([](ColumnWidget& c) {
                c.fill(Fill::Width).spacing(10);
                TB::Row([](RowWidget& r) {
                    r.fill(Fill::Width).height(36);
                    TB::Text([](TextWidget& t) {
                        t.text(model.syncPercent).family(kFont).weight(FontWeight::Bold).fontSize(30).color(kInk)
                            .alignment(Alignment::CenterVertical);
                    });
                    spacer();
                    TB::Text([](TextWidget& t) {
                        t.text(model.syncTracks).family(kFont).fontSize(12).color(kMuted).alignment(Alignment::Bottom).marginBottom(4);
                    });
                });
                TB::ProgressBar([](ProgressBarWidget& p) {
                    p.fill(Fill::Width).height(8).radius(4).minValue(0).maxValue(kLibrarySize).value(model.synced)
                        .trackColor(Color::rgba(255, 255, 255, 40)).fillColor(kTeal).transition(0.2f);
                });
            });

            TB::Row([](RowWidget& r) {
                r.fill(Fill::Width).height(36);
                label("Sync every (min)", 13, FontWeight::Regular, Color::rgba(235, 235, 245, 205));
                spacer();
                TB::SpinBox([](SpinBoxWidget& s) {
                    SpinBoxStyle style;
                    style.backgroundColor = Color::rgba(0, 0, 0, 60);
                    style.borderColor = Color::rgba(255, 255, 255, 34);
                    style.focusedBorderColor = Color::rgba(255, 255, 255, 120);
                    style.radius = 10;
                    style.borderWidth = 1.0f;
                    style.field.backgroundColor = Color::Transparent;
                    style.field.borderWidth = 0.0f;
                    style.field.textColor = kInk;
                    style.field.caretColor = Color::White;
                    style.field.font = Font{kFont, 14.0f, FontWeight::SemiBold};
                    style.stepperButton.idleColor = Color::rgba(255, 255, 255, 30);
                    style.stepperButton.hoverColor = Color::rgba(255, 255, 255, 55);
                    style.stepperButton.pressedColor = Color::rgba(255, 255, 255, 80);
                    s.style(style);
                    s.width(104).height(36).minValue(5).maxValue(60).step(5).value(15);
                });
            });

            TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Both).color(Color::Transparent); });

            TB::Button([](ButtonWidget& b) {
                b.text(model.syncButton).font(Font{kFont, 14.0f, FontWeight::SemiBold})
                    .fill(Fill::Width).height(44).radius(12)
                    .idleColor(Color::rgba(255, 255, 255, 36)).hoverColor(Color::rgba(255, 255, 255, 56))
                    .pressedColor(Color::rgba(255, 255, 255, 76))
                    .borderWidth(1).borderColor(Color::rgba(255, 255, 255, 56)).textColor(Color::White)
                    .onClick([](MouseEvent&) {
                        const bool paused = !model.syncPaused.get();
                        model.syncPaused.set(paused);
                        model.syncButton.set(paused ? "Resume sync" : "Pause sync");
                        if (model.synced.get() < kLibrarySize) model.syncBadge.set(paused ? "Paused" : "Syncing");
                    });
            });
        });
    });
}

}  // namespace

class ControlCenter : public Window {
public:
    void build() override {
        TB::Rectangle([](RectangleWidget& root) {
            root.fill(Fill::Both).color(kBackdrop);

            // The colorful backdrop the glass panels refract and blur.
            blob(-80, 40, 420, Color::rgba(109, 74, 255), 0.85f);
            blob(560, -140, 380, kCoral, 0.70f);
            blob(820, 360, 360, kTeal, 0.60f);
            blob(340, 440, 300, kAmber, 0.45f);

            TB::Row([](RowWidget& layout) {
                layout.fill(Fill::Both).padding(20).spacing(20);

                sidebar();

                TB::Column([](ColumnWidget& main) {
                    main.fill(Fill::Both).spacing(20);

                    header();

                    TB::Grid([](GridWidget& g) {
                        g.columns(3).columnSpacing(20).rowSpacing(20).fill(Fill::Width);
                        nowPlaying();
                        quickToggles();
                        soundAndLight();
                        appearance();
                        librarySync();
                    });
                });
            });
        });
    }
};

int main() {
    Application app;
    app.registerFont(kFont, "resource:/assets/fonts/Inter-Regular.ttf", FontWeight::Regular);
    app.registerFont(kFont, "resource:/assets/fonts/Inter-Medium.ttf", FontWeight::Medium);
    app.registerFont(kFont, "resource:/assets/fonts/Inter-SemiBold.ttf", FontWeight::SemiBold);
    app.registerFont(kFont, "resource:/assets/fonts/Inter-Bold.ttf", FontWeight::Bold);

    AnimationManager::instance().registerAnimation(&ticker);

    TB::createWindowOf<ControlCenter>([](Window& window) {
        window.width(1120).height(640).title("Control Center").color(kBackdrop);
    });

    return app.run();
}
