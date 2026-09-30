#include <tavoos/application.h>
#include <tavoos/builder.h>
#include <tavoos/types.h>
#include <tavoos/widget/widgets.h>
#include <tavoos/window.h>

#include <string>

using TB = Tavoos::Builder;
using namespace Tavoos;

namespace {

const Color kInk       = Color::rgba(30, 32, 40);
const Color kMuted     = Color::rgba(110, 113, 125);
const Color kAccent    = Color::rgba(85, 112, 241);
const Color kError     = Color::rgba(200, 55, 55);
const Color kSuccess   = Color::rgba(30, 140, 90);

struct LoginModel {
    TextFieldWidget* email{nullptr};
    TextFieldWidget* password{nullptr};
    CheckboxWidget* remember{nullptr};

    State<std::string> message{""};
    State<Paint> messageColor{Paint{kError}};

    void submit() {
        const std::string user = email ? email->text() : "";
        const std::string pass = password ? password->text() : "";

        if (user.empty() || user.find('@') == std::string::npos) {
            fail("Please enter a valid email address.");
            if (email) email->focus();
            return;
        }
        if (pass.empty()) {
            fail("Please enter your password.");
            if (password) password->focus();
            return;
        }

        const bool rememberMe = remember && remember->checkedState().get();
        messageColor.set(Paint{kSuccess});
        message.set("Signed in as " + user + (rememberMe ? " (remembered)" : ""));
    }

    void fail(const std::string& text) {
        messageColor.set(Paint{kError});
        message.set(text);
    }
} login;

void field(const std::string& label, const std::string& placeholder, TextFieldWidget*& out) {
    TB::Column([&](ColumnWidget& group) {
        group.spacing(6).fill(Fill::Width);

        TB::Text([&](TextWidget& t) {
            t.text(label).family("Inter").fontSize(13).color(kInk);
        });

        TB::TextField([&](TextFieldWidget& f) {
            out = &f;
            f.placeholder(placeholder)
                .font(Font{"Inter", 14.0f})
                .radius(8)
                .innerPadding(12)
                .fill(Fill::Width)
                .height(42)
                .onSubmit([](const std::string&) { login.submit(); });
        });
    });
}

}

class LoginWindow : public Window {
public:
    void build() override {
        TB::Rectangle([](RectangleWidget& card) {
            card.width(360).height(420).radius(16)
            .alignment(Alignment::Center)
                .color(Color::White)
                .borderWidth(1).borderColor(Color::rgba(224, 225, 232))
                .padding(28);

            TB::Column([](ColumnWidget& form) {
                form.fill(Fill::Both).spacing(18);

                TB::Column([](ColumnWidget& header) {
                    header.spacing(4);
                    TB::Text([](TextWidget& t) {
                        t.text("Sign in").family("Inter").fontSize(26).color(kInk);
                    });
                    TB::Text([](TextWidget& t) {
                        t.text("Welcome back. Enter your details.").family("Inter").fontSize(13).color(kMuted);
                    });
                });

                field("Email", "you@example.com", login.email);
                field("Password", "Your password", login.password);

                TB::Row([](RowWidget& row) {
                    row.spacing(8).fill(Fill::Width);
                    TB::Checkbox([](CheckboxWidget& c) {
                        login.remember = &c;
                        c.width(18).height(18).alignment(Alignment::CenterVertical);
                    });
                    TB::Text([](TextWidget& t) {
                        t.text("Remember me").family("Inter").fontSize(13).color(kInk)
                        .alignment(Alignment::CenterVertical);
                    });
                });

                TB::Button([](ButtonWidget& b) {
                    b.text("Sign in")
                    .variant(ButtonVariant::Filled)
                        .font(Font{"Inter", 15.0f, FontWeight::Medium})
                        .radius(8)
                        .idleColor(kAccent)
                        .fill(Fill::Width)
                        .height(44)
                        .onClick([](MouseEvent&) { login.submit(); });
                });

                TB::Text([](TextWidget& t) {
                    t.text(login.message).color(login.messageColor)
                    .family("Inter").fontSize(12)
                        .fill(Fill::Width).height(34).wrapMode(WrapMode::WordWrap);
                });
            });
        });

        if (login.email) login.email->focus();
    }
};

int main() {
    Application app;
    app.registerFont("Inter", "resource:/assets/fonts/Inter-Regular.ttf", FontWeight::Regular);

    TB::createWindowOf<LoginWindow>([](Window& window) {
        window.width(460).height(520).title("Login").color(Color::rgba(241, 242, 246));
    });

    return app.run();
}