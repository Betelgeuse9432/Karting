#pragma once
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace karting {

// Conservative sphere/frustum intersection. Construct once per frame.
// A radius margin prevents visible trees, grandstands and road edges
// disappearing when their origin falls just outside the screen.
struct ViewFrustum {
    Vector3 eye{},forward{},right{},up{};
    float horizontalSlope=1.f, verticalSlope=1.f;
    float farPlane=140.f;

    explicit ViewFrustum(const Camera3D& camera,float aspect,
                         float maxDistance=140.f) {
        eye=camera.position;
        forward=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
        right=Vector3Normalize(Vector3CrossProduct(forward,camera.up));
        up=Vector3Normalize(Vector3CrossProduct(right,forward));
        verticalSlope=std::tan(camera.fovy*DEG2RAD*.5f);
        horizontalSlope=verticalSlope*std::max(aspect,.1f);
        farPlane=maxDistance;
    }

    bool visible(Vector3 center,float radius=0.f) const {
        const Vector3 offset=Vector3Subtract(center,eye);
        const float depth=Vector3DotProduct(offset,forward);
        if(depth < -radius || depth > farPlane+radius) return false;
        const float x=Vector3DotProduct(offset,right);
        const float y=Vector3DotProduct(offset,up);
        // True angular-plane distance, including sphere radius, not
        // just the position of the object's centre.
        const float horizontalMargin=radius*std::sqrt(1.f+horizontalSlope*horizontalSlope);
        const float verticalMargin=radius*std::sqrt(1.f+verticalSlope*verticalSlope);
        return std::abs(x)<=depth*horizontalSlope+horizontalMargin &&
               std::abs(y)<=depth*verticalSlope+verticalMargin;
    }
};
} // namespace karting
