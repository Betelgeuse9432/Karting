#pragma once
#include "karting/core.hpp"
#include <algorithm>
#include <cmath>

namespace karting {

// Visual-only chase camera dynamics. No raylib dependency: the core tests
// can check the response without a window or GPU.
struct ChaseCamera {
    double angle=0, height=0, distance=0, lookAhead=0, fov=64;

    static double ease(double from,double to,double rate,double dt) {
        return from+(to-from)*(-std::expm1(-rate*dt));
    }

    void reset(const Car& car,const VehicleProfile& spec) {
        angle=car.heading;
        height=car.height+spec.cameraHeight;
        distance=spec.cameraDistance;
        lookAhead=0;
        fov=64;
    }

    void update(const Car& car,const VehicleProfile& spec,double dt) {
        dt=std::clamp(dt,0.0,.15);
        if(dt<=0) return;
        const double speed=std::clamp(std::abs(car.speed)/spec.topSpeed,0.0,1.0);
        const double forward=std::clamp(car.speed/spec.topSpeed,0.0,1.0);
        const double speedEase=speed*speed*(3.0-2.0*speed);
        const double braking=std::clamp(car.brake,0.0,1.0)*forward;
        const double thrust=std::clamp(car.throttle,0.0,1.0)*(1.0-braking);
        // Restrained movement: stay close to the original chase framing.
        const double steer=std::clamp(car.steering/spec.maxSteer,-1.0,1.0);
        const double desiredAngle=car.heading+.11*steer*forward;
        angle=wrapAngle(angle+ease(0.0,wrapAngle(desiredAngle-angle),5.5,dt));
        distance=ease(distance,spec.cameraDistance*
                      (1.0+.09*thrust*forward-.12*braking+.035*speedEase),4.5,dt);
        const double desiredHeight=car.height+spec.cameraHeight*
                                   (1.0+.06*braking-.035*thrust*forward);
        height=ease(height,desiredHeight,7.0,dt);
        // Bound vertical lag on crests and bridge ramps.
        height=std::clamp(height,desiredHeight-16.0,desiredHeight+16.0);
        lookAhead=ease(lookAhead,18.0*forward+8.0*std::abs(steer)*forward,4.0,dt);
        fov=ease(fov,64.0+14.0*speedEase,3.5,dt);
    }
};

} // namespace karting
