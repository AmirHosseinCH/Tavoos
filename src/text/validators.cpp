#include <tavoos/text/validators.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <memory>

namespace Tavoos {

namespace {

std::size_t digitCount(long long value) {
    const std::string digits = std::to_string(value);
    return digits[0] == '-' ? digits.size() - 1 : digits.size();
}

bool signAllowed(char sign, double bottom, double top) {
    return sign == '-' ? bottom < 0 : top >= 0;
}

pcre2_code* compilePattern(const std::string& pattern, int& errorCode, PCRE2_SIZE& errorOffset) {
    return pcre2_compile(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.size(), PCRE2_UTF, &errorCode,
                         &errorOffset, nullptr);
}

bool hitLimit(int result) {
    return result == PCRE2_ERROR_MATCHLIMIT || result == PCRE2_ERROR_DEPTHLIMIT || result == PCRE2_ERROR_HEAPLIMIT
           || result == PCRE2_ERROR_JIT_STACKLIMIT || result == PCRE2_ERROR_NOMEMORY;
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
    int errorCode = 0;
    PCRE2_SIZE errorOffset = 0;
    pcre2_code* const checked = compilePattern(pattern, errorCode, errorOffset);
    if (!checked) {
        PCRE2_UCHAR message[256];
        pcre2_get_error_message(errorCode, message, sizeof(message));
        spdlog::error("regexValidator: invalid pattern \"{}\" at offset {}: {}", pattern, errorOffset,
                      reinterpret_cast<const char*>(message));
        return {};
    }
    pcre2_code_free(checked);

    pcre2_code* compiled = nullptr;
    for (const char* close : {")\\z", "\\E)\\z", "\n)\\z"}) {
        compiled = compilePattern("(?:" + pattern + close, errorCode, errorOffset);
        if (compiled)
            break;
    }
    if (!compiled) {
        spdlog::error("regexValidator: cannot anchor pattern \"{}\" to the end of the text", pattern);
        return {};
    }
    pcre2_jit_compile(compiled, PCRE2_JIT_COMPLETE | PCRE2_JIT_PARTIAL_HARD);
    const std::shared_ptr<pcre2_code> code{compiled, pcre2_code_free};

    return [code](const std::string& text) {
        const std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)> data{
            pcre2_match_data_create_from_pattern(code.get(), nullptr), pcre2_match_data_free};
        auto match = [&](uint32_t options) {
            return pcre2_match(code.get(), reinterpret_cast<PCRE2_SPTR>(text.data()), text.size(), 0,
                               PCRE2_ANCHORED | options, data.get(), nullptr);
        };
        if (match(0) >= 0)
            return ValidatorState::Acceptable;
        if (text.empty())
            return ValidatorState::Intermediate;
        const int partial = match(PCRE2_PARTIAL_HARD);
        return partial == PCRE2_ERROR_PARTIAL || hitLimit(partial) ? ValidatorState::Intermediate : ValidatorState::Invalid;
    };
}

}
