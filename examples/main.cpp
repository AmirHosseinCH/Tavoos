#include <tavoos/animation/animatedstate.h>
#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/types.h>
#include <tavoos/widget/widgets.h>
#include <tavoos/window.h>

#include <string>

using TB = Tavoos::Builder;
using namespace Tavoos;

struct Model {
    AnimatedState<float> heroRotation{0.0f};
    AnimatedState<Paint> heroGradient{
        linearGradient({{0.0f, Color::rgba(255, 120, 80)}, {1.0f, Color::rgba(120, 90, 255)}}, 0.0f)
    };

    State<Paint> counterButtonColor{Paint{Color::rgba(60, 130, 255)}};
    int counter{0};
    TextWidget* counterLabel{nullptr};

    int nextTask{3};
    ColumnWidget* taskList{nullptr};
} model;

void addTaskRow(ColumnWidget& list, const std::string& label) {
    list.addChild<RowWidget>([label](RowWidget& item) {
        item.spacing(8).fill(Fill::Width);

        item.addChild<TextWidget>([label](TextWidget& t) {
            t.text(label).family("Inter").fontSize(13).color(Color::rgba(60, 62, 75))
                .alignment(Alignment::CenterVertical).fill(Fill::Width);
        });

        item.addChild<RectangleWidget>([&item](RectangleWidget& del) {
            del.width(22).height(22).radius(6).color(Color::rgba(255, 120, 120))
                .alignment(Alignment::CenterVertical);

            del.addChild<TextWidget>([](TextWidget& t) {
                t.text("x").family("Inter").fontSize(12).color(Color::White).alignment(Alignment::Center);
            });

            RowWidget* const self = &item;
            del.onClick([self](MouseEvent&) { self->removeSelf(); });
        });
    });
}

class MainWindow : public Window {
public:
    MainWindow() = default;

    void build() override {
        TB::Column([](ColumnWidget& root) {
            root.fill(Fill::Both).padding(28).spacing(22);

            TB::Column([](ColumnWidget& header) {
                header.spacing(4);
                TB::Text([](TextWidget& t) {
                    t.text("Tavoos").family("Inter").fontSize(30).color(Color::rgba(30, 32, 40));
                });
                TB::Text([](TextWidget& t) {
                    t.text("A declarative, reactive C++23 / OpenGL UI framework")
                        .family("Inter").fontSize(14).color(Color::rgba(140, 142, 155));
                });
            });

            auto card = [](RectangleWidget& r) {
                r.fill(Fill::Both).radius(16).color(Color::White)
                    .borderWidth(1).borderColor(Color::rgba(224, 225, 232)).padding(18);
            };
            auto sectionTitle = [](const std::string& text) {
                return [text](TextWidget& t) {
                    t.text(text).family("Inter").fontSize(15).color(Color::rgba(45, 47, 60));
                };
            };

            TB::Row([&](RowWidget& cards) {
                cards.fill(Fill::Both).spacing(20);

                TB::Rectangle([&](RectangleWidget& r) {
                    card(r);
                    TB::Column([&](ColumnWidget& inner) {
                        inner.fill(Fill::Both).spacing(12);
                        TB::Text(sectionTitle("Layout"));

                        TB::Grid([](GridWidget& grid) {
                            grid.fill(Fill::Width).columns(3).columnSpacing(8).rowSpacing(8);

                            auto box = [](RectangleWidget& r, Paint color, int span) {
                                r.height(46).radius(10).color(color).gridColumnSpan(span).fill(Fill::Width);
                            };
                            TB::Rectangle([&](RectangleWidget& r) { box(r, Color::rgba(255, 120, 80), 2); });
                            TB::Rectangle([&](RectangleWidget& r) { box(r, Color::rgba(120, 90, 255), 1); });
                            TB::Rectangle([&](RectangleWidget& r) { box(r, Color::rgba(60, 190, 140), 1); });
                            TB::Rectangle([&](RectangleWidget& r) { box(r, Color::rgba(255, 190, 60), 1); });
                            TB::Rectangle([&](RectangleWidget& r) { box(r, Color::rgba(60, 130, 255), 1); });
                        });
                    });
                });

                TB::Rectangle([&](RectangleWidget& r) {
                    card(r);
                    TB::Column([&](ColumnWidget& inner) {
                        inner.fill(Fill::Both).spacing(14);
                        TB::Text(sectionTitle("Reactive + Animation"));

                        TB::Rectangle([](RectangleWidget& hero) {
                            hero.width(130).height(76).radius(14)
                                .alignment(Alignment::CenterHorizontal)
                                .color(model.heroGradient)
                                .rotation(model.heroRotation)
                                .onClick([](MouseEvent&) {
                                    const float nextAngle = model.heroGradient.get().angle + 45.0f;
                                    model.heroGradient.animateTo(
                                        linearGradient({{0.0f, Color::rgba(255, 120, 80)}, {1.0f, Color::rgba(120, 90, 255)}}, nextAngle),
                                        0.35f, Easing::easeInOutQuad);
                                    model.heroRotation.animateTo(model.heroRotation + 30.0f, 0.35f, Easing::easeInOutQuad);
                                });
                        });

                        TB::Row([](RowWidget& counterRow) {
                            counterRow.spacing(8).fill(Fill::Width);

                            TB::Text([](TextWidget& t) {
                                model.counterLabel = &t;
                                t.text("Count: 0").family("Inter").fontSize(13).color(Color::rgba(60, 62, 75))
                                    .alignment(Alignment::CenterVertical).fill(Fill::Width);
                            });

                            TB::Rectangle([](RectangleWidget& btn) {
                                btn.width(56).height(30).radius(8).color(model.counterButtonColor)
                                    .alignment(Alignment::CenterVertical)
                                    .onMouseEnter([](MouseEvent&) { model.counterButtonColor.set(Paint{Color::rgba(40, 105, 230)}); })
                                    .onMouseLeave([](MouseEvent&) { model.counterButtonColor.set(Paint{Color::rgba(60, 130, 255)}); })
                                    .onClick([](MouseEvent&) {
                                        ++model.counter;
                                        if (model.counterLabel)
                                            model.counterLabel->text("Count: " + std::to_string(model.counter));
                                    });

                                TB::Text([](TextWidget& t) {
                                    t.text("+1").family("Inter").fontSize(13).color(Color::White).alignment(Alignment::Center);
                                });
                            });
                        });
                    });
                });

                TB::Rectangle([&](RectangleWidget& r) {
                    card(r);
                    TB::Column([&](ColumnWidget& inner) {
                        inner.fill(Fill::Both).spacing(12);

                        TB::Row([](RowWidget& listHeader) {
                            listHeader.spacing(8).fill(Fill::Width);
                            TB::Text([](TextWidget& t) {
                                t.text("Dynamic Children").family("Inter").fontSize(15).color(Color::rgba(45, 47, 60))
                                    .alignment(Alignment::CenterVertical).fill(Fill::Width);
                            });
                            TB::Rectangle([](RectangleWidget& add) {
                                add.width(28).height(28).radius(8).color(Color::rgba(60, 190, 140))
                                    .alignment(Alignment::CenterVertical)
                                    .onClick([](MouseEvent&) {
                                        if (model.taskList) {
                                            addTaskRow(*model.taskList, "Task " + std::to_string(model.nextTask));
                                            ++model.nextTask;
                                        }
                                    });
                                TB::Text([](TextWidget& t) {
                                    t.text("+").family("Inter").fontSize(16).color(Color::White).alignment(Alignment::Center);
                                });
                            });
                        });

                        TB::Column([](ColumnWidget& list) {
                            list.spacing(8);
                            model.taskList = &list;
                        });

                        addTaskRow(*model.taskList, "Design review");
                        addTaskRow(*model.taskList, "Ship v0.2");
                    });
                });
            });
        });
    }
};

int main() {
    Application app;

    app.registerFont("Inter", "resource:/assets/fonts/Inter-Regular.ttf", FontWeight::Regular);

    TB::createWindowOf<MainWindow>([](Window& window) {
        window
            .width(980)
            .height(430)
            .title("Tavoos")
            .color(Color::rgba(241, 242, 246));
    });

    return app.run();
}
