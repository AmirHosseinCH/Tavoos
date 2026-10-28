#include <tavoos/text/validators.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <regex>

namespace Tavoos {

namespace {

std::size_t digitCount(long long value) {
    const std::string digits = std::to_string(value);
    return digits[0] == '-' ? digits.size() - 1 : digits.size();
}

bool signAllowed(char sign, double bottom, double top) {
    return sign == '-' ? bottom < 0 : top >= 0;
}

}

TextValidator intValidator(long long bottom, long long top) {
    const std::size_t maxDigits = std::max(digitCount(bottom), digitCount(top));
    return [bottom, top, maxDigits](const std::string& text) {
        if (text.empty())
            return ValidatorState::Intermediate;
        const bool negative = text[0] == '-';
        const std::size_t start = negative || text[0] == '+' ? 1 : 0;
        if (start == 1 && !signAllowed(text[0], static_cast<double>(bottom), static_cast<double>(top)))
            return ValidatorState::Invalid;
        if (start == text.size())
            return ValidatorState::Intermediate;
        if (text.size() - start > maxDigits || !std::all_of(text.begin() + start, text.end(), [](char c) { return c >= '0' && c <= '9'; }))
            return ValidatorState::Invalid;

        long long value = 0;
        if (std::from_chars(text.data() + start, text.data() + text.size(), value).ec != std::errc{})
            return ValidatorState::Invalid;
        if (negative)
            value = -value;
        if (value >= bottom && value <= top)
            return ValidatorState::Acceptable;
        return (negative ? value < bottom : value > top) ? ValidatorState::Invalid : ValidatorState::Intermediate;
    };
}

TextValidator doubleValidator(double bottom, double top, int decimals) {
    return [bottom, top, decimals](const std::string& text) {
        if (text.empty())
            return ValidatorState::Intermediate;
        const bool negative = text[0] == '-';
        const std::size_t start = negative || text[0] == '+' ? 1 : 0;
        if (start == 1 && !signAllowed(text[0], bottom, top))
            return ValidatorState::Invalid;

        bool point = false;
        int digitsBefore = 0;
        int digitsAfter = 0;
        for (std::size_t i = start; i < text.size(); ++i) {
            if (text[i] == '.') {
                if (point || decimals <= 0)
                    return ValidatorState::Invalid;
                point = true;
            } else if (text[i] >= '0' && text[i] <= '9') {
                ++(point ? digitsAfter : digitsBefore);
            } else {
                return ValidatorState::Invalid;
            }
        }
        if (digitsAfter > decimals)
            return ValidatorState::Invalid;
        if (digitsBefore == 0 && digitsAfter == 0)
            return ValidatorState::Intermediate;

        double value = 0.0;
        if (std::from_chars(text.data() + start, text.data() + text.size(), value).ec != std::errc{})
            return ValidatorState::Invalid;
        if (negative)
            value = -value;
        if (value >= bottom && value <= top)
            return ValidatorState::Acceptable;
        return (negative ? value < bottom : value > top) ? ValidatorState::Invalid : ValidatorState::Intermediate;
    };
}

TextValidator regexValidator(const std::string& pattern) {
    try {
        return [expression = std::regex{pattern}](const std::string& text) {
            return std::regex_match(text, expression) ? ValidatorState::Acceptable : ValidatorState::Intermediate;
        };
    } catch (const std::regex_error& error) {
        spdlog::error("regexValidator: invalid pattern \"{}\": {}", pattern, error.what());
        return {};
    }
}

}
