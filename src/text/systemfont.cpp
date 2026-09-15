#include <tavoos/text/systemfont.h>

#ifdef TAVOOS_HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

namespace Tavoos {

std::string findSystemDefaultFontPath() {
#ifdef TAVOOS_HAVE_FONTCONFIG
    FcPattern* pattern = FcNameParse(reinterpret_cast<const FcChar8*>("sans-serif"));
    if (!pattern)
        return {};

    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern* match = FcFontMatch(nullptr, pattern, &result);
    FcPatternDestroy(pattern);

    std::string path;
    if (match) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch)
            path = reinterpret_cast<const char*>(file);
        FcPatternDestroy(match);
    }
    return path;
#else
    return {};
#endif
}

}
