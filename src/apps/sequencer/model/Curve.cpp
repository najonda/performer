#include "Curve.h"

#include <algorithm>

#include <cmath>

static const float Pi = 3.1415926536f;
static const float TwoPi = 2.f * Pi;

    static const std::map<int, Curve::Type> INV_SHAPE_MAP = {
        {0, Curve::High},
        {1, Curve::Low},
        {2, Curve::RampDown},
        {3, Curve::RampUp},
        {4, Curve::rampDownHalf},
        {5, Curve::rampUpHalf},
        {6, Curve::doubleRampDownHalf},
        {7, Curve::doubleRampUpHalf},
        {8, Curve::ExpDown},
        {9, Curve::ExpUp},
        {10, Curve::expDownHalf},
        {11, Curve:: expUpHalf},
        {12, Curve::doubleExpDownHalf},
        {13, Curve::doubleExpUpHalf},
        {14, Curve::LogDown},
        {15, Curve::LogUp},
        {16, Curve::logDownHalf},
        {17, Curve::logUpHalf},
        {18, Curve::doubleLogDownHalf},
        {19, Curve::doubleLogUpHalf},
        {20, Curve::SmoothDown},
        {21, Curve::SmoothUp},
        {22, Curve::smoothDownHalf},
        {23, Curve::smoothUpHalf},
        {24, Curve::doubleSmoothDownHalf},
        {25, Curve::doubleSmoothUpHalf},
        {26, Curve::RevTriangle},
        {27, Curve::Triangle},
        {28, Curve::RevBell},
        {29, Curve::Bell},
        {30, Curve::StepDown},
        {31, Curve::StepUp},
        {32, Curve::ExpUp2x},
        {33, Curve::ExpDown2x},
        {34, Curve::ExpUp3x},
        {35, Curve::ExpUp4x},
        {36, Curve::ExpDown4x}
    };

    static const std::map<int, Curve::Type> REV_SHAPE_MAP = {
        {0, Curve::Low},
        {1, Curve::High},
        {2, Curve::RampDown},
        {3, Curve::RampUp},
        {4, Curve::rampDownHalf},
        {5, Curve::rampUpHalf},
        {6, Curve::doubleRampDownHalf},
        {7, Curve::doubleRampUpHalf},
        {8, Curve::ExpDown},
        {9, Curve::ExpUp},
        {10, Curve::expDownHalf},
        {11, Curve:: expUpHalf},
        {12, Curve::doubleExpDownHalf},
        {13, Curve::doubleExpUpHalf},
        {14, Curve::LogDown},
        {15, Curve::LogUp},
        {16, Curve::logDownHalf},
        {17, Curve::logUpHalf},
        {18, Curve::doubleLogDownHalf},
        {19, Curve::doubleLogUpHalf},
        {20, Curve::SmoothDown},
        {21, Curve::SmoothUp},
        {22, Curve::smoothDownHalf},
        {23, Curve::smoothUpHalf},
        {24, Curve::doubleSmoothDownHalf},
        {25, Curve::doubleSmoothUpHalf},
        {26, Curve::Triangle},
        {27, Curve::RevTriangle},
        {28, Curve::Bell},
        {29, Curve::RevBell},
        {30, Curve::StepDown},
        {31, Curve::StepUp},
        {32, Curve::ExpUp2x},
        {33, Curve::ExpDown2x},
        {34, Curve::ExpUp3x},
        {35, Curve::ExpUp4x},
        {36, Curve::ExpDown4x}
    };

static float low(float x) {
    return 0.f;
}

static float high(float x) {
    return 1.f;
}

static float rampUp(float x) {
    return x;
}

static float rampDown(float x) {
    return 1.f - x;
}

static float expUp(float x) {
    return x * x;
}

static float expDown(float x) {
    return (1.f - x) * (1.f - x);
}

static float logUp(float x) {
    return std::sqrt(x);
}

static float logDown(float x) {
    return std::sqrt(1.f - x);
}

static float smoothUp(float x) {
    return x * x * (3.f - 2.f * x);
}

static float smoothDown(float x) {
    return 1.f - x * x * (3.f - 2.f * x);
}

static float rampUpHalf(float x) {
    return x < 0.5f ? rampUp(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float rampDownHalf(float x) {
    return x < 0.5f ? rampDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float expUpHalf(float x) {
    return x < 0.5f ? expUp(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float expDownHalf(float x) {
    return x < 0.5f ? expDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float logUpHalf(float x) {
    return x < 0.5f ? logUp(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float logDownHalf(float x) {
    return x < 0.5f ? logDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float smoothUpHalf(float x) {
    return x < 0.5f ? smoothUp(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float smoothDownHalf(float x) {
    return x < 0.5f ? smoothDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float triangle(float x) {
    return (x < 0.5f ? x : 1.f - x) * 2.f;
}

static float revTriangle(float x) {
    return 1.f+(-triangle(x));
}

static float bell(float x) {
    return 0.5f - 0.5f * std::cos(x * TwoPi);
}

static float revBell(float x) {
    return 1.f+-bell(x);
}

static float stepUp(float x) {
    return x < 0.5f ? 0.f : 1.f;
}

static float stepDown(float x) {
    return x < 0.5f ? 1.f : 0.f;
}

static float expDown2x(float x) {
    return x < 1.f ? expDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float expDown3x(float x) {
    return x < 1.f ? expDown(std::fmod(x * 3.f, 1.f)) : 0.f;
}

static float expDown4x(float x) {
    return x < 1.f ? expDown(std::fmod(x * 4.f, 1.f)) : 0.f;
}

static float expUp2x(float x) {
    return x < 1.f ? expUp(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float expUp3x(float x) {
    return x < 1.f ? expUp(std::fmod(x * 3.f, 1.f)) : 0.f;
}

static float expUp4x(float x) {
    return x < 1.f ? expUp(std::fmod(x * 4.f, 1.f)) : 0.f;
}

static float doubleRampUpHalf(float x) {
    return x < 0.5f ? rampUp(std::fmod(x * 2.f, 1.f)) : rampUp(std::fmod(x * 2.f, 1.f));
}

static float doubleRampDownHalf(float x) {
    return x < 0.5f ? rampDown(std::fmod(x * 2.f, 1.f)) : x < 1.f ? rampDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float doubleExpUpHalf(float x) {
    return x < 0.5f ? expUp(std::fmod(x * 2.f, 1.f)) : expUp(std::fmod(x * 2.f, 1.f));
}

static float doubleExpDownHalf(float x) {
    return x < 0.5f ? expDown(std::fmod(x * 2.f, 1.f)) : x < 1.f ?  expDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float doubleLogUpHalf(float x) {
    return x < 0.5f ? logUp(std::fmod(x * 2.f, 1.f)) : logUp(std::fmod(x * 2.f, 1.f));
}

static float doubleLogDownHalf(float x) {
    return x < 0.5f ? logDown(std::fmod(x * 2.f, 1.f)) : x < 1.f ? logDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static float doubleSmoothUpHalf(float x) {
    return x < 0.5f ? smoothUp(std::fmod(x * 2.f, 1.f)) : smoothUp(std::fmod(x * 2.f, 1.f));
}

static float doubleSmoothDownHalf(float x) {
    return x < 0.5f ? smoothDown(std::fmod(x * 2.f, 1.f)) : x < 1.f ? smoothDown(std::fmod(x * 2.f, 1.f)) : 0.f;
}

static Curve::Function functions[] = {
    &low,
    &high,
    &stepUp,
    &stepDown,
    &rampUp,
    &rampDown,
    &rampUpHalf,
    &rampDownHalf,
    &doubleRampUpHalf,
    &doubleRampDownHalf,
    &expUp,
    &expDown,
    &expUpHalf,
    &expDownHalf,
    &doubleExpUpHalf,
    &doubleExpDownHalf,
    &logUp,
    &logDown,
    &logUpHalf,
    &logDownHalf,
    &doubleLogUpHalf,
    &doubleLogDownHalf,
    &smoothUp,
    &smoothDown,
    &smoothUpHalf,
    &smoothDownHalf,
    &doubleSmoothUpHalf,
    &doubleSmoothDownHalf,
    &triangle,
    &revTriangle,
    &bell,
    &revBell,
    &expDown2x,
    &expUp2x,
    &expDown3x,
    &expUp3x,
    &expDown4x,
    &expUp4x
};

Curve::Function Curve::function(Type type) {
    return functions[type];
}

float Curve::eval(Type type, float x) {
    return functions[type](x);
}

Curve::Type Curve::invAt(int i) {
        return INV_SHAPE_MAP.at(i);
}

Curve::Type Curve::revAt(int i) {
        return REV_SHAPE_MAP.at(i);
}