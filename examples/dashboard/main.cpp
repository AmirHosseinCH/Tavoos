// Analytics - a dense, minimal Tavoos dashboard sample.
//
// A light store-analytics dashboard: sidebar navigation, KPI cards with sparklines, an animated
// two-series bar chart, a conic-gradient donut, an orders table with status pills and a top
// products list. Switching the range (Today / 12 weeks / 12 months) animates every bar and
// updates every figure through State<T> bindings. All data is made up.

#include <tavoos/animation/animation.h>
#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/types.h>
#include <tavoos/widget/templates/templates.h>
#include <tavoos/widget/widgets.h>
#include <tavoos/window.h>

#include <array>
#include <cmath>
#include <string>

using TB = Tavoos::Builder;
using namespace Tavoos;

namespace {

// ---------------------------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------------------------

const Color kBg      = Color::rgba(247, 247, 248);
const Color kCard    = Color::White;
const Color kLine    = Color::rgba(232, 232, 236);
const Color kInk     = Color::rgba(24, 24, 27);
const Color kSoft    = Color::rgba(113, 113, 122);
const Color kFaint   = Color::rgba(161, 161, 170);
const Color kAccent  = Color::rgba(79, 70, 229);
const Color kSky     = Color::rgba(14, 165, 233);
const Color kTeal    = Color::rgba(20, 184, 166);
const Color kAmberC  = Color::rgba(245, 158, 11);
const Color kGhost   = Color::rgba(212, 212, 216);
const Color kGreen   = Color::rgba(21, 128, 61);
const Color kGreenBg = Color::rgba(220, 252, 231);
const Color kRed     = Color::rgba(185, 28, 28);
const Color kRedBg   = Color::rgba(254, 226, 226);
const Color kAmber   = Color::rgba(180, 83, 9);
const Color kAmberBg = Color::rgba(254, 243, 199);
const Color kGrayBg  = Color::rgba(244, 244, 245);

const char* kFont = "Inter";

// ---------------------------------------------------------------------------------------------
// Icons
// ---------------------------------------------------------------------------------------------

std::string svg(const char* body) {
    return std::string("data:<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='#000' "
                       "stroke-width='1.8' stroke-linecap='round' stroke-linejoin='round'>") + body + "</svg>";
}

const std::string kIconHome     = svg("<path d='M3 10.5L12 3l9 7.5'/><path d='M5 9.5V21h14V9.5'/>");
const std::string kIconBag      = svg("<path d='M6 7h12l1 14H5z'/><path d='M9 7a3 3 0 0 1 6 0'/>");
const std::string kIconBox      = svg("<path d='M3 7.5L12 3l9 4.5v9L12 21l-9-4.5z'/><path d='M3 7.5L12 12l9-4.5'/><path d='M12 12v9'/>");
const std::string kIconUsers    = svg("<circle cx='9' cy='8' r='3.5'/><path d='M2.5 20a6.5 6.5 0 0 1 13 0'/><path d='M16 4.5a3.5 3.5 0 0 1 0 7'/><path d='M18 14a6.5 6.5 0 0 1 3.5 6'/>");
const std::string kIconChart    = svg("<path d='M4 20V10'/><path d='M10 20V4'/><path d='M16 20v-7'/><path d='M22 20H2'/>");
const std::string kIconGear     = svg("<circle cx='12' cy='12' r='3'/><path d='M12 2v3M12 19v3M4.2 4.2l2.1 2.1M17.7 17.7l2.1 2.1M2 12h3M19 12h3M4.2 19.8l2.1-2.1M17.7 6.3l2.1-2.1'/>");
const std::string kIconSearch   = svg("<circle cx='11' cy='11' r='7'/><path d='M20 20l-3.5-3.5'/>");
const std::string kIconDownload = svg("<path d='M12 4v11'/><path d='M7 10l5 5 5-5'/><path d='M4 20h16'/>");

// ---------------------------------------------------------------------------------------------
// Data (made up)
// ---------------------------------------------------------------------------------------------

constexpr int kBars = 12;

struct Kpi {
    const char* value;
    const char* delta;
    bool up;
    std::array<float, kBars> spark;
};

struct Range {
    const char* name;
    const char* subtitle;
    std::array<const char*, kBars> labels;
    std::array<float, kBars> current;   // 0-1 of the axis maximum
    std::array<float, kBars> previous;
    std::array<const char*, 4> axis;    // top to bottom
    std::array<Kpi, 4> kpis;
    const char* total;
};

const std::array<Range, 3> kRanges{{
    { "Today", "Hourly revenue, today vs. yesterday",
      { "00", "02", "04", "06", "08", "10", "12", "14", "16", "18", "20", "22" },
      { 0.12f, 0.06f, 0.04f, 0.10f, 0.38f, 0.62f, 0.81f, 0.74f, 0.88f, 0.95f, 0.70f, 0.41f },
      { 0.15f, 0.08f, 0.05f, 0.09f, 0.31f, 0.55f, 0.66f, 0.70f, 0.72f, 0.79f, 0.64f, 0.38f },
      { "$3k", "$2k", "$1k", "$0" },
      {{ { "$18,240", "+12.4%", true,  { .3f, .2f, .2f, .3f, .5f, .6f, .8f, .7f, .9f, 1.f, .7f, .5f } },
         { "312",     "+8.1%",  true,  { .2f, .2f, .1f, .3f, .4f, .7f, .8f, .6f, .9f, .8f, .6f, .4f } },
         { "3.42%",   "-0.3%",  false, { .6f, .5f, .6f, .5f, .4f, .5f, .4f, .5f, .4f, .4f, .3f, .4f } },
         { "$58.46",  "+3.9%",  true,  { .4f, .5f, .4f, .5f, .6f, .5f, .6f, .7f, .6f, .7f, .8f, .8f } } }},
      "4,812" },
    { "12 weeks", "Weekly revenue, this quarter vs. last",
      { "W31", "W32", "W33", "W34", "W35", "W36", "W37", "W38", "W39", "W40", "W41", "W42" },
      { 0.48f, 0.52f, 0.45f, 0.58f, 0.61f, 0.57f, 0.66f, 0.72f, 0.69f, 0.78f, 0.84f, 0.91f },
      { 0.42f, 0.44f, 0.47f, 0.43f, 0.50f, 0.52f, 0.49f, 0.55f, 0.58f, 0.56f, 0.61f, 0.63f },
      { "$150k", "$100k", "$50k", "$0" },
      {{ { "$1.21M",  "+18.2%", true,  { .4f, .5f, .4f, .5f, .6f, .5f, .6f, .7f, .7f, .8f, .8f, .9f } },
         { "20,148",  "+11.6%", true,  { .5f, .5f, .4f, .6f, .6f, .5f, .7f, .7f, .6f, .8f, .8f, .9f } },
         { "3.71%",   "+0.4%",  true,  { .4f, .4f, .5f, .4f, .5f, .5f, .6f, .5f, .6f, .6f, .7f, .7f } },
         { "$60.05",  "-1.2%",  false, { .7f, .7f, .6f, .7f, .6f, .6f, .5f, .6f, .5f, .5f, .5f, .4f } } }},
      "61,390" },
    { "12 months", "Monthly revenue, this year vs. last",
      { "Nov", "Dec", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct" },
      { 0.56f, 0.88f, 0.42f, 0.47f, 0.55f, 0.58f, 0.63f, 0.60f, 0.67f, 0.71f, 0.76f, 0.83f },
      { 0.49f, 0.79f, 0.36f, 0.38f, 0.44f, 0.47f, 0.50f, 0.52f, 0.55f, 0.57f, 0.61f, 0.64f },
      { "$600k", "$400k", "$200k", "$0" },
      {{ { "$4.86M",  "+24.9%", true,  { .5f, .9f, .4f, .4f, .5f, .6f, .6f, .6f, .7f, .7f, .8f, .8f } },
         { "79,503",  "+19.3%", true,  { .5f, .8f, .4f, .5f, .5f, .6f, .6f, .6f, .7f, .7f, .7f, .8f } },
         { "3.58%",   "+0.6%",  true,  { .4f, .5f, .4f, .4f, .5f, .5f, .5f, .6f, .6f, .6f, .7f, .7f } },
         { "$61.13",  "+4.6%",  true,  { .5f, .6f, .5f, .5f, .5f, .6f, .6f, .6f, .6f, .7f, .7f, .7f } } }},
      "248,902" },
}};

const std::array<const char*, 4> kKpiNames{ "Revenue", "Orders", "Conversion", "Avg. order value" };

// ---------------------------------------------------------------------------------------------
// Model: every figure the range switch touches is a State, every bar an AnimatedState.
// ---------------------------------------------------------------------------------------------

struct KpiState {
    State<std::string> value{""};
    State<std::string> delta{""};
    State<Paint> deltaInk{kGreen};
    State<Paint> deltaBg{kGreenBg};
    std::array<AnimatedState<float>, kBars> spark;
};

struct Model {
    std::array<KpiState, 4> kpis;
    std::array<AnimatedState<float>, kBars> current;
    std::array<AnimatedState<float>, kBars> previous;
    std::array<State<std::string>, kBars> labels;
    std::array<State<std::string>, 4> axis;
    State<std::string> chartSubtitle{""};
    State<std::string> sessions{""};
    State<std::string> activeNow{"128 active now"};
    int active{128};

    void apply(const Range& r, bool animate) {
        const float t = animate ? 0.45f : 0.0f;
        for (int i = 0; i < kBars; ++i) {
            current[i].animateTo(r.current[i], t, Easing::easeInOutQuad);
            previous[i].animateTo(r.previous[i], t, Easing::easeInOutQuad);
            labels[i].set(r.labels[i]);
        }
        for (int i = 0; i < 4; ++i) axis[i].set(r.axis[i]);
        for (int k = 0; k < 4; ++k) {
            const Kpi& src = r.kpis[k];
            KpiState& dst = kpis[k];
            dst.value.set(src.value);
            dst.delta.set(src.delta);
            dst.deltaInk.set(src.up ? kGreen : kRed);
            dst.deltaBg.set(src.up ? kGreenBg : kRedBg);
            for (int i = 0; i < kBars; ++i) dst.spark[i].animateTo(src.spark[i], t, Easing::easeInOutQuad);
        }
        chartSubtitle.set(r.subtitle);
        sessions.set(r.total);
    }
} model;

// Wanders the "active now" count a little every couple of seconds.
class Ticker : public AnimatableBase {
    bool tick(float dt) override {
        m_acc += dt;
        if (m_acc >= 2.0f) {
            m_acc = 0.0f;
            m_step = (m_step + 1) % 7;
            static const int kDrift[7] = { 3, -2, 5, -4, 1, -1, 2 };
            model.active += kDrift[m_step];
            model.activeNow.set(std::to_string(model.active) + " active now");
        }
        return true;
    }
    float m_acc{0.0f};
    int m_step{0};
} ticker;

// ---------------------------------------------------------------------------------------------
// Custom controls
// ---------------------------------------------------------------------------------------------

class NavItem : public ButtonBase {
public:
    NavItem(Object* parent) : ButtonBase{parent} {
        checkable(true).exclusive(true);
        height(36);
        checkedState().onChange([this](const bool&) { restyle(); });
        hoveredState().onChange([this](const bool&) { restyle(); });
        background([this](RectangleWidget& box) { box.radius(8).color(m_bg); });
        content<RowWidget>([this](RowWidget& row) {
            // Slot bodies run outside the builder, so children use addChild.
            row.spacing(10).paddingLeft(10).paddingRight(10).fill(Fill::Both);
            row.addChild<SVGWidget>([this](SVGWidget& icon) {
                icon.source(m_icon).width(16).height(16).alignment(Alignment::CenterVertical).color(m_ink);
            });
            row.addChild<TextWidget>([this](TextWidget& t) {
                t.text(m_caption).family(kFont).weight(FontWeight::Medium).fontSize(13).color(m_ink)
                    .alignment(Alignment::CenterVertical);
            });
            row.addChild<RectangleWidget>([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(Color::Transparent); });
            row.addChild<TextWidget>([this](TextWidget& t) {
                t.text(m_badge).family(kFont).fontSize(12).color(kFaint).alignment(Alignment::CenterVertical);
            });
        });
    }

    NavItem& item(const std::string& icon, const std::string& caption, const std::string& badge = "") {
        m_icon.set(icon);
        m_caption.set(caption);
        m_badge.set(badge);
        return *this;
    }

private:
    void restyle() {
        m_bg.set(checked() ? Color::rgba(240, 240, 243) : hovered() ? Color::rgba(246, 246, 248) : Color::rgba(255, 255, 255, 0));
        m_ink.set(checked() ? kInk : kSoft);
    }

    State<std::string> m_icon{""};
    State<std::string> m_caption{""};
    State<std::string> m_badge{""};
    State<Paint> m_bg{Color::rgba(255, 255, 255, 0)};
    State<Paint> m_ink{kSoft};
};

// One option of the range switch.
class RangeOption : public ButtonBase {
public:
    RangeOption(Object* parent) : ButtonBase{parent} {
        checkable(true).exclusive(true);
        height(30);
        checkedState().onChange([this](const bool& on) {
            m_bg.animateTo(on ? Color::White : Color::rgba(255, 255, 255, 0), 0.15f);
            m_ink.animateTo(on ? kInk : kSoft, 0.15f);
            m_lift.set(on ? ShadowStyle{ .offsetY = 1, .radius = 3, .color = Color::rgba(0, 0, 0, 30) } : ShadowStyle{ .color = Color::Transparent });
            if (on && m_range) model.apply(*m_range, built());
        });
        background([this](RectangleWidget& box) { box.radius(7).color(m_bg).dropShadow(m_lift); });
        content<TextWidget>([this](TextWidget& t) {
            t.text(m_caption).family(kFont).weight(FontWeight::Medium).fontSize(12).color(m_ink).alignment(Alignment::Center);
        });
    }

    RangeOption& range(const Range& r, int w) {
        m_range = &r;
        m_caption.set(r.name);
        width(w);
        return *this;
    }

private:
    const Range* m_range{nullptr};
    State<std::string> m_caption{""};
    AnimatedState<Paint> m_bg{Color::rgba(255, 255, 255, 0)};
    AnimatedState<Paint> m_ink{kSoft};
    State<ShadowStyle> m_lift{ShadowStyle{ .color = Color::Transparent }};
};

// ---------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------

void text(const std::string& value, float size, FontWeight weight = FontWeight::Regular, Paint color = kInk) {
    TB::Text([=](TextWidget& t) {
        t.text(value).family(kFont).weight(weight).fontSize(size).color(color).alignment(Alignment::CenterVertical);
    });
}

void bound(State<std::string>& value, float size, FontWeight weight = FontWeight::Regular, Paint color = kInk) {
    TB::Text([&value, size, weight, color](TextWidget& t) {
        t.text(value).family(kFont).weight(weight).fontSize(size).color(color).alignment(Alignment::CenterVertical);
    });
}

void spacer() {
    TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Width).height(1).color(Color::Transparent); });
}

void vspace(int h) {
    TB::Rectangle([h](RectangleWidget& r) { r.fill(Fill::Width).height(h).color(Color::Transparent); });
}

void icon(const std::string& source, int size, Paint color) {
    TB::SVG([=](SVGWidget& s) { s.source(source).width(size).height(size).color(color).alignment(Alignment::CenterVertical); });
}

void card(RectangleWidget& r) {
    r.radius(14).color(kCard).borderWidth(1).borderColor(kLine)
        .dropShadow({ .offsetY = 1, .radius = 2, .color = Color::rgba(24, 24, 27, 10) });
}

void dot(Color c, int size = 8) {
    TB::Rectangle([=](RectangleWidget& r) { r.width(size).height(size).radius(size / 2).color(c).alignment(Alignment::CenterVertical); });
}

void pill(const std::string& value, Color ink, Color bg) {
    TB::Rectangle([=](RectangleWidget& r) {
        r.height(22).width(static_cast<int>(value.size()) * 7 + 18).radius(11).color(bg).alignment(Alignment::CenterVertical);
        TB::Text([=](TextWidget& t) {
            t.text(value).family(kFont).weight(FontWeight::Medium).fontSize(12).color(ink).alignment(Alignment::Center);
        });
    });
}

// ---------------------------------------------------------------------------------------------
// Sections
// ---------------------------------------------------------------------------------------------

ButtonGroup navGroup;
ButtonGroup rangeGroup;

void sidebar() {
    TB::Rectangle([](RectangleWidget& side) {
        side.width(232).fill(Fill::Height).color(kCard);
        TB::Rectangle([](RectangleWidget& edge) { edge.width(1).fill(Fill::Height).alignment(Alignment::Right).color(kLine); });

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(2).padding(16);

            TB::Row([](RowWidget& brand) {
                brand.height(40).spacing(10).paddingLeft(8);
                TB::Rectangle([](RectangleWidget& mark) {
                    mark.width(26).height(26).radius(7).color(kInk).alignment(Alignment::CenterVertical);
                    TB::Rectangle([](RectangleWidget& inner) {
                        inner.width(10).height(10).radius(5).color(Color::White).alignment(Alignment::Center);
                    });
                });
                text("Northwind", 15, FontWeight::SemiBold);
            });
            vspace(18);
            text("  MENU", 11, FontWeight::Medium, kFaint);
            vspace(6);

            TB::Create<NavItem>([](NavItem& n) { n.item(kIconHome, "Overview").fill(Fill::Width).group(navGroup).checked(true); });
            TB::Create<NavItem>([](NavItem& n) { n.item(kIconBag, "Orders", "24").fill(Fill::Width).group(navGroup); });
            TB::Create<NavItem>([](NavItem& n) { n.item(kIconBox, "Products").fill(Fill::Width).group(navGroup); });
            TB::Create<NavItem>([](NavItem& n) { n.item(kIconUsers, "Customers").fill(Fill::Width).group(navGroup); });
            TB::Create<NavItem>([](NavItem& n) { n.item(kIconChart, "Reports").fill(Fill::Width).group(navGroup); });
            TB::Create<NavItem>([](NavItem& n) { n.item(kIconGear, "Settings").fill(Fill::Width).group(navGroup); });

            TB::Rectangle([](RectangleWidget& r) { r.fill(Fill::Both).color(Color::Transparent); });

            // Plan usage.
            TB::Rectangle([](RectangleWidget& box) {
                box.fill(Fill::Width).height(104).radius(12).padding(14).color(kBg).borderWidth(1).borderColor(kLine);
                TB::Column([](ColumnWidget& c) {
                    c.fill(Fill::Both).spacing(8);
                    TB::Row([](RowWidget& r) {
                        r.fill(Fill::Width).height(18);
                        text("Monthly orders", 12, FontWeight::Medium);
                        spacer();
                        text("79%", 12, FontWeight::Regular, kSoft);
                    });
                    TB::ProgressBar([](ProgressBarWidget& p) {
                        p.fill(Fill::Width).height(6).radius(3).value(79).trackColor(kLine).fillColor(kInk);
                    });
                    text("7,900 of 10,000 this month", 11, FontWeight::Regular, kSoft);
                });
            });

            vspace(12);

            TB::Row([](RowWidget& user) {
                user.fill(Fill::Width).height(40).spacing(10).paddingLeft(4);
                TB::Rectangle([](RectangleWidget& av) {
                    av.width(32).height(32).radius(16).color(Color::rgba(224, 231, 255)).alignment(Alignment::CenterVertical);
                    TB::Text([](TextWidget& t) {
                        t.text("MC").family(kFont).weight(FontWeight::SemiBold).fontSize(12).color(kAccent).alignment(Alignment::Center);
                    });
                });
                TB::Column([](ColumnWidget& c) {
                    c.alignment(Alignment::CenterVertical).spacing(1);
                    text("Mina Cole", 13, FontWeight::Medium);
                    text("Store admin", 11, FontWeight::Regular, kSoft);
                });
            });
        });
    });
}

void topBar() {
    TB::Row([](RowWidget& bar) {
        bar.fill(Fill::Width).height(48).spacing(12);

        TB::Column([](ColumnWidget& c) {
            c.spacing(4).alignment(Alignment::CenterVertical);
            text("Overview", 22, FontWeight::SemiBold);
            text("Sales performance across all channels", 13, FontWeight::Regular, kSoft);
        });

        spacer();

        TB::Rectangle([](RectangleWidget& track) {
            track.height(36).width(3 * 84 + 2 * 2 + 8).radius(9).padding(3).color(Color::rgba(240, 240, 243))
                .alignment(Alignment::CenterVertical);
            TB::Row([](RowWidget& r) {
                r.fill(Fill::Both).spacing(2);
                for (std::size_t i = 0; i < kRanges.size(); ++i) {
                    TB::Create<RangeOption>([i](RangeOption& o) {
                        o.range(kRanges[i], 84).group(rangeGroup).checked(i == 2);
                    });
                }
            });
        });

        TB::Rectangle([](RectangleWidget& box) {
            box.width(220).height(36).radius(9).color(kCard).borderWidth(1).borderColor(kLine)
                .paddingLeft(12).alignment(Alignment::CenterVertical);
            TB::Row([](RowWidget& r) {
                r.fill(Fill::Both).spacing(6);
                icon(kIconSearch, 15, kFaint);
                TB::TextField([](TextFieldWidget& f) {
                    f.placeholder("Search orders, customers").font(Font{kFont, 13.0f}).fill(Fill::Both)
                        .borderWidth(0.0f).radius(0).innerPadding(2).backgroundColor(Color::Transparent)
                        .focusedBorderColor(Color::Transparent).textColor(kInk).placeholderColor(kFaint).caretColor(kAccent);
                });
            });
        });

        TB::Button([](ButtonWidget& b) {
            b.text("Export").icon(kIconDownload).iconSize(15).iconSpacing(6)
                .font(Font{kFont, 13.0f, FontWeight::Medium}).width(100).height(36).radius(9)
                .idleColor(kInk).hoverColor(Color::rgba(52, 52, 58)).pressedColor(Color::Black).textColor(Color::White)
                .alignment(Alignment::CenterVertical);
        });
    });
}

void kpiCards() {
    TB::Grid([](GridWidget& g) {
        g.columns(4).columnSpacing(16).fill(Fill::Width);
        for (int k = 0; k < 4; ++k) {
            TB::Rectangle([k](RectangleWidget& c) {
                card(c);
                c.fill(Fill::Width).height(116).padding(18);
                KpiState& s = model.kpis[k];

                TB::Column([k, &s](ColumnWidget& col) {
                    col.fill(Fill::Both).spacing(10);
                    TB::Row([k, &s](RowWidget& r) {
                        r.fill(Fill::Width).height(22);
                        text(kKpiNames[k], 13, FontWeight::Medium, kSoft);
                        spacer();
                        TB::Rectangle([&s](RectangleWidget& chip) {
                            chip.width(62).height(22).radius(11).color(s.deltaBg).alignment(Alignment::CenterVertical);
                            TB::Text([&s](TextWidget& t) {
                                t.text(s.delta).family(kFont).weight(FontWeight::Medium).fontSize(12).color(s.deltaInk).alignment(Alignment::Center);
                            });
                        });
                    });
                    TB::Row([&s](RowWidget& r) {
                        r.fill(Fill::Width).height(46);
                        TB::Text([&s](TextWidget& t) {
                            t.text(s.value).family(kFont).weight(FontWeight::SemiBold).fontSize(26).color(kInk)
                                .alignment(Alignment::Bottom).marginBottom(2);
                        });
                        spacer();
                        // Sparkline: twelve thin bars, each a fraction of a fixed-height box.
                        TB::Row([&s](RowWidget& spark) {
                            spark.spacing(3).height(34).alignment(Alignment::Bottom);
                            for (int i = 0; i < kBars; ++i) {
                                TB::Rectangle([&s, i](RectangleWidget& slot) {
                                    slot.width(4).fill(Fill::Height).color(Color::Transparent);
                                    TB::Rectangle([&s, i](RectangleWidget& bar) {
                                        bar.fill(Fill::Width).heightFraction(s.spark[i]).radius(2).alignment(Alignment::Bottom)
                                            .color(i == kBars - 1 ? kAccent : Color::rgba(199, 210, 254));
                                    });
                                });
                            }
                        });
                    });
                });
            });
        }
    });
}

void revenueChart() {
    TB::Rectangle([](RectangleWidget& c) {
        card(c);
        c.fill(Fill::Width).height(300).gridColumnSpan(2).padding(20);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(18);

            TB::Row([](RowWidget& head) {
                head.fill(Fill::Width).height(38).spacing(16);
                TB::Column([](ColumnWidget& t) {
                    t.spacing(4).alignment(Alignment::CenterVertical);
                    text("Revenue", 15, FontWeight::SemiBold);
                    bound(model.chartSubtitle, 12, FontWeight::Regular, kSoft);
                });
                spacer();
                TB::Row([](RowWidget& legend) {
                    legend.spacing(6).alignment(Alignment::CenterVertical).height(20);
                    dot(kAccent);
                    text("Current", 12, FontWeight::Regular, kSoft);
                });
                TB::Row([](RowWidget& legend) {
                    legend.spacing(6).alignment(Alignment::CenterVertical).height(20);
                    dot(kGhost);
                    text("Previous", 12, FontWeight::Regular, kSoft);
                });
            });

            TB::Row([](RowWidget& body) {
                body.fill(Fill::Both).spacing(10);

                // Y axis labels, evenly spread over the plot height.
                TB::Rectangle([](RectangleWidget& axis) {
                    axis.width(40).fill(Fill::Height).color(Color::Transparent).marginBottom(22);
                    for (int i = 0; i < 4; ++i) {
                        TB::Text([i](TextWidget& t) {
                            t.text(model.axis[i]).family(kFont).fontSize(11).color(kFaint)
                                .yFraction(i / 3.0f).alignment(Alignment::Right).marginTop(-7).marginBottom(-7);
                        });
                    }
                });

                TB::Column([](ColumnWidget& plotCol) {
                    plotCol.fill(Fill::Both).spacing(8);

                    TB::Rectangle([](RectangleWidget& plot) {
                        plot.fill(Fill::Both).color(Color::Transparent);
                        for (int i = 0; i < 4; ++i) {
                            TB::Rectangle([i](RectangleWidget& line) {
                                line.fill(Fill::Width).height(1).yFraction(i / 3.0f).color(i == 3 ? kGhost : Color::rgba(240, 240, 243));
                            });
                        }
                        TB::Row([](RowWidget& bars) {
                            bars.fill(Fill::Both).spacing(0);
                            for (int i = 0; i < kBars; ++i) {
                                TB::Rectangle([i](RectangleWidget& group) {
                                    group.fill(Fill::Both).color(Color::Transparent);
                                    TB::Rectangle([i](RectangleWidget& prev) {
                                        prev.width(11).heightFraction(model.previous[i]).xFraction(0.5f).marginRight(13)
                                            .alignment(Alignment::Bottom).radiusTopLeft(3).radiusTopRight(3).color(kGhost);
                                    });
                                    TB::Rectangle([i](RectangleWidget& cur) {
                                        cur.width(11).heightFraction(model.current[i]).xFraction(0.5f).marginLeft(13)
                                            .alignment(Alignment::Bottom).radiusTopLeft(3).radiusTopRight(3).color(kAccent);
                                    });
                                });
                            }
                        });
                    });

                    TB::Row([](RowWidget& labels) {
                        labels.fill(Fill::Width).height(14).spacing(0);
                        for (int i = 0; i < kBars; ++i) {
                            TB::Text([i](TextWidget& t) {
                                t.text(model.labels[i]).family(kFont).fontSize(11).color(kFaint)
                                    .fill(Fill::Width).textAlignment(Alignment::CenterHorizontal);
                            });
                        }
                    });
                });
            });
        });
    });
}

void trafficCard() {
    TB::Rectangle([](RectangleWidget& c) {
        card(c);
        c.fill(Fill::Width).height(300).padding(20);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(16);

            TB::Row([](RowWidget& head) {
                head.fill(Fill::Width).height(20);
                text("Traffic sources", 15, FontWeight::SemiBold);
                spacer();
                TB::Row([](RowWidget& live) {
                    live.spacing(6).height(20);
                    dot(Color::rgba(34, 197, 94), 7);
                    bound(model.activeNow, 12, FontWeight::Regular, kSoft);
                });
            });

            TB::Row([](RowWidget& body) {
                body.fill(Fill::Both).spacing(22);

                // Donut: a conic gradient with hard stops, punched out by a white disc.
                TB::Rectangle([](RectangleWidget& ring) {
                    ring.width(150).height(150).radius(75).alignment(Alignment::CenterVertical)
                        .color(conicGradient({
                            { 0.00f, kAccent }, { 0.42f, kAccent },
                            { 0.42f, kSky },    { 0.70f, kSky },
                            { 0.70f, kTeal },   { 0.88f, kTeal },
                            { 0.88f, kAmberC }, { 1.00f, kAmberC },
                        }, 0.5f, 0.5f, 0.0f));
                    TB::Rectangle([](RectangleWidget& hole) {
                        hole.width(108).height(108).radius(54).color(kCard).alignment(Alignment::Center);
                        TB::Column([](ColumnWidget& c) {
                            c.spacing(2).alignment(Alignment::Center);
                            TB::Text([](TextWidget& t) {
                                t.text(model.sessions).family(kFont).weight(FontWeight::SemiBold).fontSize(20).color(kInk)
                                    .alignment(Alignment::CenterHorizontal);
                            });
                            TB::Text([](TextWidget& t) {
                                t.text("sessions").family(kFont).fontSize(11).color(kSoft).alignment(Alignment::CenterHorizontal);
                            });
                        });
                    });
                });

                TB::Column([](ColumnWidget& legend) {
                    legend.fill(Fill::Width).spacing(14).alignment(Alignment::CenterVertical);
                    struct Item { const char* name; const char* share; Color color; };
                    static const std::array<Item, 4> items{{
                        { "Direct", "42%", kAccent }, { "Search", "28%", kSky },
                        { "Social", "18%", kTeal }, { "Email", "12%", kAmberC },
                    }};
                    for (const Item& item : items) {
                        TB::Row([&item](RowWidget& r) {
                            r.fill(Fill::Width).height(20).spacing(8);
                            dot(item.color);
                            text(item.name, 13);
                            spacer();
                            text(item.share, 13, FontWeight::Medium);
                        });
                    }
                });
            });
        });
    });
}

struct Order {
    const char* id;
    const char* customer;
    const char* date;
    const char* status;
    const char* amount;
};

const std::array<Order, 5> kOrders{{
    { "#10482", "Lena Hart",     "Oct 20, 09:42", "Paid",     "$128.00" },
    { "#10481", "Omar Rahimi",   "Oct 20, 09:15", "Pending",  "$64.50" },
    { "#10480", "Sofia Marin",   "Oct 19, 18:03", "Paid",     "$312.90" },
    { "#10479", "Theo Brandt",   "Oct 19, 16:47", "Refunded", "$45.00" },
    { "#10478", "Aiko Tanaka",   "Oct 19, 14:20", "Paid",     "$89.99" },
}};

std::array<State<Paint>, kOrders.size()> rowTint;

void ordersTable() {
    TB::Rectangle([](RectangleWidget& c) {
        card(c);
        c.fill(Fill::Width).height(290).gridColumnSpan(2).paddingTop(18).paddingBottom(8).paddingLeft(8).paddingRight(8);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(0);

            TB::Row([](RowWidget& head) {
                head.fill(Fill::Width).height(26).paddingLeft(12).paddingRight(12);
                text("Recent orders", 15, FontWeight::SemiBold);
                spacer();
                text("View all", 13, FontWeight::Medium, kAccent);
            });
            vspace(12);

            const auto columns = [](auto&& cell) {
                cell(0, 90); cell(1, 0); cell(2, 140); cell(3, 110); cell(4, 90);
            };

            // Header.
            TB::Rectangle([&](RectangleWidget& hdr) {
                hdr.fill(Fill::Width).height(32).radius(8).color(kBg).paddingLeft(12).paddingRight(12);
                TB::Row([&](RowWidget& r) {
                    r.fill(Fill::Both);
                    static const std::array<const char*, 5> names{ "Order", "Customer", "Date", "Status", "Amount" };
                    columns([](int i, int w) {
                        TB::Text([i, w](TextWidget& t) {
                            t.text(names[i]).family(kFont).weight(FontWeight::Medium).fontSize(12).color(kSoft)
                                .alignment(Alignment::CenterVertical);
                            if (w > 0) t.width(w); else t.fill(Fill::Width);
                            if (i == 4) t.textAlignment(Alignment::Right);
                        });
                    });
                });
            });

            for (std::size_t i = 0; i < kOrders.size(); ++i) {
                rowTint[i].set(Color::rgba(255, 255, 255, 0));
                TB::Rectangle([i, &columns](RectangleWidget& row) {
                    row.fill(Fill::Width).height(37).radius(8).color(rowTint[i]).paddingLeft(12).paddingRight(12)
                        .onMouseEnter([i](MouseEvent&) { rowTint[i].set(Color::rgba(248, 248, 250)); })
                        .onMouseLeave([i](MouseEvent&) { rowTint[i].set(Color::rgba(255, 255, 255, 0)); });
                    const Order& o = kOrders[i];
                    TB::Row([&o, &columns](RowWidget& r) {
                        r.fill(Fill::Both);
                        columns([&o](int col, int w) {
                            if (col == 3) {
                                TB::Rectangle([&o, w](RectangleWidget& cell) {
                                    cell.width(w).fill(Fill::Height).color(Color::Transparent);
                                    const std::string s = o.status;
                                    if (s == "Paid") pill(s, kGreen, kGreenBg);
                                    else if (s == "Pending") pill(s, kAmber, kAmberBg);
                                    else pill(s, kSoft, kGrayBg);
                                });
                                return;
                            }
                            const char* value = col == 0 ? o.id : col == 1 ? o.customer : col == 2 ? o.date : o.amount;
                            TB::Text([=](TextWidget& t) {
                                t.text(value).family(kFont).fontSize(13)
                                    .weight(col == 4 || col == 0 ? FontWeight::Medium : FontWeight::Regular)
                                    .color(col == 2 ? kSoft : kInk).alignment(Alignment::CenterVertical);
                                if (w > 0) t.width(w); else t.fill(Fill::Width);
                                if (col == 4) t.textAlignment(Alignment::Right);
                            });
                        });
                    });
                });
                if (i + 1 < kOrders.size()) {
                    TB::Rectangle([](RectangleWidget& line) { line.fill(Fill::Width).height(1).marginLeft(12).marginRight(12).color(Color::rgba(242, 242, 244)); });
                }
            }
        });
    });
}

void topProducts() {
    TB::Rectangle([](RectangleWidget& c) {
        card(c);
        c.fill(Fill::Width).height(290).padding(20);

        TB::Column([](ColumnWidget& col) {
            col.fill(Fill::Both).spacing(16);
            TB::Row([](RowWidget& head) {
                head.fill(Fill::Width).height(20);
                text("Top products", 15, FontWeight::SemiBold);
                spacer();
                text("Units sold", 12, FontWeight::Regular, kSoft);
            });

            struct Product { const char* name; int sold; };
            static const std::array<Product, 5> products{{
                { "Linen overshirt", 1840 }, { "Canvas tote", 1512 }, { "Merino beanie", 1203 },
                { "Ceramic mug set", 968 }, { "Field notebook", 742 },
            }};
            for (const Product& p : products) {
                TB::Column([&p](ColumnWidget& item) {
                    item.fill(Fill::Width).spacing(7);
                    TB::Row([&p](RowWidget& r) {
                        r.fill(Fill::Width).height(16);
                        text(p.name, 13, FontWeight::Medium);
                        spacer();
                        text(std::to_string(p.sold), 12, FontWeight::Regular, kSoft);
                    });
                    TB::ProgressBar([&p](ProgressBarWidget& bar) {
                        bar.fill(Fill::Width).height(5).radius(3).maxValue(2000).value(p.sold)
                            .trackColor(Color::rgba(240, 240, 243)).fillColor(kInk);
                    });
                });
            }
        });
    });
}

}  // namespace

class Dashboard : public Window {
public:
    void build() override {
        TB::Row([](RowWidget& layout) {
            layout.fill(Fill::Both).spacing(0);

            sidebar();

            TB::Column([](ColumnWidget& main) {
                main.fill(Fill::Both).spacing(20).padding(28);
                topBar();
                kpiCards();
                TB::Grid([](GridWidget& g) {
                    g.columns(3).columnSpacing(16).rowSpacing(16).fill(Fill::Width);
                    revenueChart();
                    trafficCard();
                    ordersTable();
                    topProducts();
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

    TB::createWindowOf<Dashboard>([](Window& window) {
        window.width(1320).height(864).title("Northwind \xC2\xB7 Analytics").color(kBg);
    });

    return app.run();
}
