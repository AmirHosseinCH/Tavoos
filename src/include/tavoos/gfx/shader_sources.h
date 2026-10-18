#pragma once

namespace Tavoos {

inline const char*fragmentShaderPrelude = R"(#version 460 core
#define MAX_GRADIENT_STOPS 8
#define TWO_PI 6.28318530718
)";

inline const char*paintSamplingGLSL = R"(
vec4 samplePaintExplicit(vec2 uv, vec4 flatColor, int kind, int stopCount,
                          vec4 stopColors[MAX_GRADIENT_STOPS], float stopPositions[MAX_GRADIENT_STOPS],
                          float angle, vec2 center, float radius) {
    if (kind == 0 || stopCount < 2)
        return flatColor;

    float t;
    if (kind == 2) {
        t = clamp(length(uv - center) / max(radius, 0.0001), 0.0, 1.0);
    } else if (kind == 3) {
        vec2 dir = uv - center;
        float theta = atan(dir.y, dir.x) - radians(angle);
        t = mod(theta, TWO_PI) / TWO_PI;
    } else {
        vec2 dir = vec2(cos(angle), sin(angle));
        t = clamp(dot(uv - vec2(0.5), dir) + 0.5, 0.0, 1.0);
    }

    vec4 result = stopColors[0];
    for (int i = 0; i < stopCount - 1; i++) {
        float p0 = stopPositions[i];
        float p1 = stopPositions[i + 1];
        if (t >= p0 && t <= p1) {
            float localT = (p1 > p0) ? (t - p0) / (p1 - p0) : 0.0;
            result = mix(stopColors[i], stopColors[i + 1], localT);
        }
    }
    if (t >= stopPositions[stopCount - 1])
        result = stopColors[stopCount - 1];
    return result;
}
)";

inline const char*roundedBoxGLSL = R"(
float sdRoundedBox(vec2 p, vec2 halfSize, vec4 radius) {
    float r;
    if (p.x < 0.0) {
        r = (p.y < 0.0) ? radius.x : radius.w;
    } else {
        r = (p.y < 0.0) ? radius.y : radius.z;
    }
    vec2 q = abs(p) - halfSize + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}
)";

inline const char*quadVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;

layout(std140, binding = 0) uniform camera {
    mat4 projection;
};

uniform mat4 world;
uniform vec2 uHalfSize;

out vec2 vLocalPos;

void main() {
    vec4 worldPos = world * vec4(aPos, 0.0, 1.0);
    vLocalPos = aPos - uHalfSize;
    gl_Position = projection * worldPos;
}
)";

inline const char*quadFragmentShaderHead = R"(
in vec2 vLocalPos;

out vec4 fragColor;

uniform vec2 uHalfSize;
uniform vec4 uRadius;
uniform vec4 uColor;
uniform vec4 uBorderColor;
uniform float uBorderWidth;

uniform int uMaskMode;

uniform int uPaintKind;
uniform int uStopCount;
uniform vec4 uStopColors[MAX_GRADIENT_STOPS];
uniform float uStopPositions[MAX_GRADIENT_STOPS];
uniform float uGradientAngle;
uniform vec2 uGradientCenter;
uniform float uGradientRadius;

uniform int uBorderPaintKind;
uniform int uBorderStopCount;
uniform vec4 uBorderStopColors[MAX_GRADIENT_STOPS];
uniform float uBorderStopPositions[MAX_GRADIENT_STOPS];
uniform float uBorderGradientAngle;
uniform vec2 uBorderGradientCenter;
uniform float uBorderGradientRadius;
)";

inline const char*quadFragmentShaderTail = R"(
vec4 samplePaint(vec2 uv) {
    return samplePaintExplicit(uv, uColor, uPaintKind, uStopCount, uStopColors, uStopPositions, uGradientAngle, uGradientCenter, uGradientRadius);
}

vec4 sampleBorderPaint(vec2 uv) {
    return samplePaintExplicit(uv, uBorderColor, uBorderPaintKind, uBorderStopCount, uBorderStopColors, uBorderStopPositions, uBorderGradientAngle, uBorderGradientCenter, uBorderGradientRadius);
}

void main() {
    float dist = sdRoundedBox(vLocalPos, uHalfSize, uRadius);
    vec2 grad = vec2(dFdx(dist), dFdy(dist));
    float aa = length(grad);

    float outerAlpha = 1.0 - smoothstep(-aa, aa, dist);

    if (uMaskMode == 1) {
        fragColor = vec4(outerAlpha, 0.0, 0.0, 1.0);
        return;
    }

    float innerDist = dist + uBorderWidth;
    float fillAlpha = (uBorderWidth <= 0.0) ? 1.0 : (1.0 - smoothstep(-aa, aa, innerDist));

    vec2 uv = (vLocalPos + uHalfSize) / (2.0 * uHalfSize);
    vec4 fillPaint = samplePaint(uv);
    vec4 borderPaint = sampleBorderPaint(uv);

    vec3 fillPremult = fillPaint.rgb * fillPaint.a;
    vec3 borderPremult = borderPaint.rgb * borderPaint.a;
    vec3 mixedPremult = mix(borderPremult, fillPremult, fillAlpha);
    float mixedAlpha = mix(borderPaint.a, fillPaint.a, fillAlpha);

    vec3 rgb = mixedAlpha > 0.0 ? mixedPremult / mixedAlpha : vec3(0.0);
    float alpha = outerAlpha * mixedAlpha;

    fragColor = vec4(rgb, alpha);
}
)";

inline const char*blurredBoxGLSL = R"(
float erfApprox(float x) {
    float a = abs(x);
    float t = 1.0 / (1.0 + 0.3275911 * a);
    float poly = ((((1.061405429 * t - 1.453152027) * t + 1.421413741) * t - 0.284496736) * t + 0.254829592) * t;
    return sign(x) * (1.0 - poly * exp(-a * a));
}

float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (2.50662827463 * sigma);
}

float rowCoverage(float x, float rowY, float sigma, vec2 halfSize, vec4 radius) {
    float rowLeft = rowY < 0.0 ? radius.x : radius.w;
    float rowRight = rowY < 0.0 ? radius.y : radius.z;
    float deltaLeft = min(halfSize.y - rowLeft - abs(rowY), 0.0);
    float deltaRight = min(halfSize.y - rowRight - abs(rowY), 0.0);
    float left = halfSize.x - rowLeft + sqrt(max(0.0, rowLeft * rowLeft - deltaLeft * deltaLeft));
    float right = halfSize.x - rowRight + sqrt(max(0.0, rowRight * rowRight - deltaRight * deltaRight));
    float k = 0.70710678 / sigma;
    return 0.5 * (erfApprox((x + left) * k) - erfApprox((x - right) * k));
}

float blurredBox(vec2 p, vec2 halfSize, vec4 radius, float sigma) {
    float low = p.y - halfSize.y;
    float high = p.y + halfSize.y;
    float start = clamp(-3.0 * sigma, low, high);
    float stop = clamp(3.0 * sigma, low, high);
    float step = (stop - start) / 8.0;
    float y = start + step * 0.5;
    float value = 0.0;
    for (int i = 0; i < 8; i++) {
        value += rowCoverage(p.x, p.y - y, sigma, halfSize, radius) * gaussian(y, sigma) * step;
        y += step;
    }
    return value;
}
)";

inline const char*shadowFragmentShader = R"(
in vec2 vLocalPos;

out vec4 fragColor;

uniform vec2 uHalfSize;
uniform vec4 uRadius;
uniform vec2 uOffset;
uniform float uSpread;
uniform float uSigma;
uniform vec4 uColor;

void main() {
    vec2 shadowHalf = max(uHalfSize + vec2(uSpread), vec2(0.0));
    vec4 shadowRadius = clamp(uRadius + vec4(uSpread), vec4(0.0), vec4(min(shadowHalf.x, shadowHalf.y)));
    vec2 p = vLocalPos - uOffset;

    float coverage;
    if (uSigma > 0.01) {
        coverage = clamp(blurredBox(p, shadowHalf, shadowRadius, uSigma), 0.0, 1.0);
    } else {
        float dist = sdRoundedBox(p, shadowHalf, shadowRadius);
        float aa = max(length(vec2(dFdx(dist), dFdy(dist))), 0.0001);
        coverage = 1.0 - smoothstep(-aa, aa, dist);
    }

    float ownDist = sdRoundedBox(vLocalPos, uHalfSize, uRadius);
    float ownAa = max(length(vec2(dFdx(ownDist), dFdy(ownDist))), 0.0001);
    coverage *= smoothstep(-ownAa, ownAa, ownDist);

    fragColor = vec4(uColor.rgb, uColor.a * coverage);
}
)";

inline const char*innerShadowFragmentShader = R"(
in vec2 vLocalPos;

out vec4 fragColor;

uniform vec2 uHalfSize;
uniform vec4 uRadius;
uniform vec2 uOffset;
uniform float uSpread;
uniform float uSigma;
uniform vec4 uColor;

void main() {
    vec2 holeHalf = max(uHalfSize - vec2(uSpread), vec2(0.0));
    vec4 holeRadius = clamp(uRadius - vec4(uSpread), vec4(0.0), vec4(min(holeHalf.x, holeHalf.y)));
    vec2 p = vLocalPos - uOffset;

    float hole;
    if (uSigma > 0.01) {
        hole = clamp(blurredBox(p, holeHalf, holeRadius, uSigma), 0.0, 1.0);
    } else {
        float dist = sdRoundedBox(p, holeHalf, holeRadius);
        float aa = max(length(vec2(dFdx(dist), dFdy(dist))), 0.0001);
        hole = 1.0 - smoothstep(-aa, aa, dist);
    }

    float ownDist = sdRoundedBox(vLocalPos, uHalfSize, uRadius);
    float ownAa = max(length(vec2(dFdx(ownDist), dFdy(ownDist))), 0.0001);
    float inside = 1.0 - smoothstep(-ownAa, ownAa, ownDist);

    fragColor = vec4(uColor.rgb, uColor.a * (1.0 - hole) * inside);
}
)";

inline const char*effectBlurFragmentShader = R"(
#version 460 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uSource;
uniform vec2 uStep;
uniform float uSigma;
uniform int uTaps;

void main() {
    float sum = 0.0;
    float weights = 0.0;
    for (int i = -uTaps; i <= uTaps; ++i) {
        float w = exp(-float(i * i) / (2.0 * uSigma * uSigma));
        sum += texture(uSource, vUV + uStep * float(i)).a * w;
        weights += w;
    }
    fragColor = vec4(0.0, 0.0, 0.0, sum / weights);
}
)";

inline const char*effectMorphFragmentShader = R"(
#version 460 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uSource;
uniform vec2 uStep;
uniform int uRadius;
uniform int uErode;

void main() {
    float value = uErode == 1 ? 1.0 : 0.0;
    for (int i = -uRadius; i <= uRadius; ++i) {
        float a = texture(uSource, vUV + uStep * float(i)).a;
        value = uErode == 1 ? min(value, a) : max(value, a);
    }
    fragColor = vec4(0.0, 0.0, 0.0, value);
}
)";

inline const char*effectCompositeVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;

layout(std140, binding = 0) uniform camera {
    mat4 projection;
};

uniform mat4 world;
uniform vec2 uRectSize;
uniform vec2 uPad;
uniform vec2 uOffset;

out vec2 vOwnUV;
out vec2 vBlurUV;

void main() {
    vOwnUV = (aPos + uPad) / uRectSize;
    vBlurUV = (aPos - uOffset + uPad) / uRectSize;
    gl_Position = projection * world * vec4(aPos, 0.0, 1.0);
}
)";

inline const char*alphaDropShadowFragmentShader = R"(
#version 460 core

in vec2 vOwnUV;
in vec2 vBlurUV;
out vec4 fragColor;

uniform sampler2D uBlurred;
uniform sampler2D uOwn;
uniform vec4 uColor;
uniform float uOpacity;

void main() {
    float scale = 1.0 / max(uOpacity, 0.0001);
    float shadow = min(texture(uBlurred, vBlurUV).a * scale, 1.0);
    float own = min(texture(uOwn, vOwnUV).a * scale, 1.0);
    fragColor = vec4(uColor.rgb, uColor.a * uOpacity * shadow * (1.0 - own));
}
)";

inline const char*alphaInnerShadowFragmentShader = R"(
#version 460 core

in vec2 vOwnUV;
in vec2 vBlurUV;
out vec4 fragColor;

uniform sampler2D uBlurred;
uniform sampler2D uOwn;
uniform vec4 uColor;
uniform float uOpacity;

void main() {
    float scale = 1.0 / max(uOpacity, 0.0001);
    float hole = min(texture(uBlurred, vBlurUV).a * scale, 1.0);
    float own = min(texture(uOwn, vOwnUV).a * scale, 1.0);
    fragColor = vec4(uColor.rgb, uColor.a * uOpacity * (1.0 - hole) * own);
}
)";

inline const char*layerBlurFragmentShader = R"(
#version 460 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uSource;
uniform vec2 uStep;
uniform vec2 uRectSize;
uniform vec2 uPad;
uniform vec2 uBoxSize;
uniform vec2 uSigmas;
uniform vec2 uDirection;
uniform int uMaxTaps;

void main() {
    vec2 local = vUV * uRectSize - uPad;
    float t = clamp(dot(local / uBoxSize - vec2(0.5), uDirection) + 0.5, 0.0, 1.0);
    float sigma = max(mix(uSigmas.x, uSigmas.y, t), 0.0001);
    int taps = int(ceil(sigma * 3.0));

    vec4 sum = vec4(0.0);
    float weights = 0.0;
    for (int i = -uMaxTaps; i <= uMaxTaps; ++i) {
        if (abs(i) > taps)
            continue;
        float w = exp(-float(i * i) / (2.0 * sigma * sigma));
        sum += texture(uSource, vUV + uStep * float(i)) * w;
        weights += w;
    }
    fragColor = sum / weights;
}
)";

inline const char*layerBlurCompositeFragmentShader = R"(
#version 460 core

in vec2 vOwnUV;
out vec4 fragColor;

uniform sampler2D uLayer;

void main() {
    fragColor = texture(uLayer, vOwnUV);
}
)";

inline const char*imageVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;

layout(std140, binding = 0) uniform camera {
    mat4 projection;
};

uniform mat4 world;
uniform vec2 uRectSize;

out vec2 vUV;

void main() {
    vUV = aPos / uRectSize;
    gl_Position = projection * world * vec4(aPos, 0.0, 1.0);
}
)";

inline const char*imageFragmentShader = R"(
#version 460 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec2 uUVScale;
uniform vec2 uUVOffset;
uniform int uDiscardOutOfRange;
uniform float uOpacity;
uniform int uMaskMode;

void main() {
    vec2 uv = vUV * uUVScale + uUVOffset;

    if (uDiscardOutOfRange == 1 && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0))
        discard;

    vec4 texel = texture(uTexture, uv);

    if (uMaskMode == 1) {
        fragColor = vec4(texel.a, 0.0, 0.0, 1.0);
        return;
    }

    fragColor = texel;
    fragColor.a *= uOpacity;
}
)";

inline const char*svgVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;

layout(std140, binding = 0) uniform camera {
    mat4 projection;
};

uniform mat4 world;
uniform vec2 uRectSize;

out vec2 vUV;

void main() {
    vUV = aPos / uRectSize;
    gl_Position = projection * world * vec4(aPos, 0.0, 1.0);
}
)";

inline const char*svgFragmentShaderHead = R"(
in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform int uHasColor;
uniform float uOpacity;
uniform int uMaskMode;

uniform int uPaintKind;
uniform int uStopCount;
uniform vec4 uStopColors[MAX_GRADIENT_STOPS];
uniform float uStopPositions[MAX_GRADIENT_STOPS];
uniform float uGradientAngle;
uniform vec2 uGradientCenter;
uniform float uGradientRadius;
)";

inline const char*svgFragmentShaderTail = R"(
vec4 samplePaint(vec2 uv) {
    return samplePaintExplicit(uv, uColor, uPaintKind, uStopCount, uStopColors, uStopPositions, uGradientAngle, uGradientCenter, uGradientRadius);
}

void main() {
    vec4 texel = texture(uTexture, vUV);

    if (uMaskMode == 1) {
        fragColor = vec4(texel.a, 0.0, 0.0, 1.0);
        return;
    }

    if (uHasColor == 1) {
        vec4 paintColor = samplePaint(vUV);
        fragColor = vec4(paintColor.rgb, paintColor.a * texel.a * uOpacity);
    } else {
        fragColor = vec4(texel.rgb, texel.a * uOpacity);
    }
}
)";

inline const char*textVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec2 aLocalUV;

layout(std140, binding = 0) uniform camera {
    mat4 projection;
};

uniform mat4 world;

out vec2 vUV;
out vec2 vLocalUV;

void main() {
    vUV = aUV;
    vLocalUV = aLocalUV;
    gl_Position = projection * world * vec4(aPos, 0.0, 1.0);
}
)";

inline const char*textFragmentShaderHead = R"(
in vec2 vUV;
in vec2 vLocalUV;
out vec4 fragColor;

uniform sampler2D uAtlas;
uniform vec4 uColor;
uniform int uMaskMode;

uniform int uPaintKind;
uniform int uStopCount;
uniform vec4 uStopColors[MAX_GRADIENT_STOPS];
uniform float uStopPositions[MAX_GRADIENT_STOPS];
uniform float uGradientAngle;
uniform vec2 uGradientCenter;
uniform float uGradientRadius;
)";

inline const char*textFragmentShaderTail = R"(
vec4 samplePaint(vec2 uv) {
    return samplePaintExplicit(uv, uColor, uPaintKind, uStopCount, uStopColors, uStopPositions, uGradientAngle, uGradientCenter, uGradientRadius);
}

void main() {
    float sdfValue = texture(uAtlas, vUV).r;
    float dist = sdfValue - 0.5;

    vec2 texel = 1.0 / vec2(textureSize(uAtlas, 0));
    float dX = texture(uAtlas, vUV + vec2(texel.x, 0.0)).r - texture(uAtlas, vUV - vec2(texel.x, 0.0)).r;
    float dY = texture(uAtlas, vUV + vec2(0.0, texel.y)).r - texture(uAtlas, vUV - vec2(0.0, texel.y)).r;
    vec2 sdfGradPerTexel = vec2(dX, dY) * 0.5;

    vec2 uvPerPixel = fwidth(vUV);
    float aa = abs(sdfGradPerTexel.x / texel.x * uvPerPixel.x) + abs(sdfGradPerTexel.y / texel.y * uvPerPixel.y);
    aa = clamp(aa * 0.55, 0.006, 0.2);
    float scale = 1.0 / max(max(uvPerPixel.x / texel.x, uvPerPixel.y / texel.y), 1e-4);
    float small = clamp((0.25 - scale) / 0.10, 0.0, 1.0);
    float alpha = smoothstep(-aa, aa, dist + 0.03 * small);

    if (uMaskMode == 1) {
        fragColor = vec4(alpha, 0.0, 0.0, 1.0);
        return;
    }

    vec4 paintColor = samplePaint(vLocalUV);
    fragColor = vec4(paintColor.rgb, paintColor.a * alpha);
}
)";

inline const char*clipCompositeVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;

layout(std140, binding = 0) uniform camera {
    mat4 projection;
};

uniform mat4 world;
uniform vec2 uRectSize;

out vec2 vUV;

void main() {
    vUV = aPos / uRectSize;
    gl_Position = projection * world * vec4(aPos, 0.0, 1.0);
}
)";

inline const char*clipCompositeFragmentShader = R"(
#version 460 core

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform sampler2D uMask;

void main() {
    vec4 color = texture(uTexture, vUV);
    float mask = texture(uMask, vUV).r;
    float newAlpha = min(color.a, mask);
    vec3 straightRgb = color.a > 0.0 ? color.rgb / color.a : vec3(0.0);
    fragColor = vec4(straightRgb * newAlpha, newAlpha);
}
)";

}
