#pragma once

namespace Tavoos {

enum class FontWeight : int {
    Thin      = 100,
    ExtraLight = 200,
    Light     = 300,
    Regular   = 400,
    Medium    = 500,
    SemiBold  = 600,
    Bold      = 700,
    ExtraBold = 800,
    Black     = 900,
};

enum class FontStyle {
    Normal,
    Italic,
    Oblique,
};

enum class WrapMode { NoWrap, WordWrap, WrapAnywhere };
enum class ElideMode { None, Left, Right, Middle };

}