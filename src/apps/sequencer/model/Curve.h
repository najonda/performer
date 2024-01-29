#pragma once

#include <map>
class Curve {
    
public:
    typedef float (*Function)(float x);

    enum Type {
        Low,
        High,
        RampUp,
        RampDown,
        rampUpHalf,
        rampDownHalf,
        doubleRampUpHalf,
        doubleRampDownHalf,
        ExpUp,
        ExpDown,
        expUpHalf,
        expDownHalf,
        doubleExpUpHalf,
        doubleExpDownHalf,
        LogUp,
        LogDown,
        logUpHalf,
        logDownHalf,
        doubleLogUpHalf,
        doubleLogDownHalf,
        SmoothUp,
        SmoothDown,
        smoothUpHalf,
        smoothDownHalf,
        doubleSmoothUpHalf,
        doubleSmoothDownHalf,
        Triangle,
        RevTriangle,
        Bell,
        RevBell,
        StepUp,
        StepDown,
        ExpDown2x,
        ExpUp2x,
        ExpDown3x,
        ExpUp3x,
        ExpDown4x,
        ExpUp4x,
        Last,
    };

 

    static Function function(Type type);

    static float eval(Type type, float x);

    static Type invAt(int i);
    static Type revAt(int i);
};

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

 
