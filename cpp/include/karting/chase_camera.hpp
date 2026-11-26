#pragma once
#include "karting/core.hpp"
#include <algorithm>
#include <cmath>

namespace karting {

// Visual-only chase camera dynamics. No raylib dependency: the core tests
// can check the response without a window or GPU.
struct ChaseCamera {
    double angle=0, height=0, distance=0, lookAhead=0, lookAngle=0, fov=64;

    static double ease(double from,double to,double rate,double dt) {
        return from+(to-from)*(-std::expm1(-rate*dt));
    }

    void reset(const Car& car,const VehicleProfile& spec) {
        angle=car.heading;
        height=car.height+spec.cameraHeight;
        distance=spec.cameraDistance;
        lookAhead=0;
        lookAngle=0;
        fov=64;
    }

    void update(const Car& car,const VehicleProfile& spec,double dt,bool dynamic=true) {
        dt=std::clamp(dt,0.0,.15);
        if(dt<=0) return;
        const double speed=std::clamp(std::abs(car.speed)/spec.topSpeed,0.0,1.0);
        const double forward=std::clamp(car.speed/spec.topSpeed,0.0,1.0);
        const double speedEase=speed*speed*(3.0-2.0*speed);
        const double braking=std::clamp(car.brake,0.0,1.0)*forward;
        const double thrust=std::clamp(car.throttle,0.0,1.0)*(1.0-braking);
        // Match the steering limit used by the simulation: at speed the
        // actual wheel angle is much smaller than the parking-speed limit.
        const double limit=spec.maxSteer/(1.0+std::abs(car.speed)/spec.steerSensitivity);
        const double steer=std::clamp(car.steering/limit,-1.0,1.0);
        // V selects the previous restrained camera for an in-game A/B test.
        if(!dynamic) {
            const double oldSteer=std::clamp(car.steering/spec.maxSteer,-1.0,1.0);
            const double desiredAngle=car.heading+.11*oldSteer*forward;
            angle=wrapAngle(angle+ease(0.0,wrapAngle(desiredAngle-angle),5.5,dt));
            distance=ease(distance,spec.cameraDistance*
                          (1.0+.09*thrust*forward-.12*braking+.035*speedEase),4.5,dt);
            const double desiredHeight=car.height+spec.cameraHeight*
                                       (1.0+.06*braking-.035*thrust*forward);
            height=ease(height,desiredHeight,7.0,dt);
            height=std::clamp(height,desiredHeight-16.0,desiredHeight+16.0);
            lookAhead=ease(lookAhead,18.0*forward+8.0*std::abs(oldSteer)*forward,4.0,dt);
            lookAngle=ease(lookAngle,0.0,5.0,dt);
            fov=ease(fov,64.0+14.0*speedEase,3.5,dt);
            return;
        }
        // The camera trails the direction change while its aim turns into
        // the corner. Keep the car visible instead of orbiting far away.
        const double turnWeight=std::min(1.0,forward*2.5);
        const double desiredAngle=car.heading-.12*steer*turnWeight;
        angle=wrapAngle(angle+ease(0.0,wrapAngle(desiredAngle-angle),6.5,dt));
        const double response=std::sqrt(forward);
        const double brakeResponse=std::clamp(car.brake,0.0,1.0)*std::min(1.0,forward*3.0);
        distance=ease(distance,spec.cameraDistance*
                      (1.0+.19*thrust*response-.24*brakeResponse),5.5,dt);
        const double desiredHeight=car.height+spec.cameraHeight*
                                   (1.0+.17*brakeResponse-.12*thrust*response);
        height=ease(height,desiredHeight,8.0,dt);
        height=std::clamp(height,desiredHeight-16.0,desiredHeight+16.0);
        lookAhead=ease(lookAhead,55.0*response+30.0*std::abs(steer)*turnWeight,5.0,dt);
        lookAngle=ease(lookAngle,.23*steer*turnWeight,5.0,dt);
        // Reach a visibly wider FOV earlier, rather than almost exclusively
        // near the theoretical top speed (which motor drag rarely permits).
        fov=ease(fov,64.0+22.0*std::pow(speed,.70)-4.0*brakeResponse,5.0,dt);
    }
};

} // namespace karting
