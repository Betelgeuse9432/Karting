#include "karting/game.hpp"
#include <algorithm>
#include <cmath>

namespace karting {

void Effects::clear() {
    marks_.clear();
    particles_.clear();
    lastSkidding_=false;
    previousThrottle_=false;
    emitTime_=0;
    for (auto& p:previous_) p={};
}

void Effects::update(const Simulation& sim, double dt) {
    const auto& car=sim.car();
    const auto& p=sim.profile();
    const auto& telemetry=sim.telemetry();
    const bool skid=telemetry.drifting || telemetry.rearSlip ||
                    telemetry.frontSlip || car.brake>.9;
    const Vec2 forward{std::cos(car.heading),std::sin(car.heading)};
    const Vec2 side{-forward.y,forward.x};
    const auto wheelPosition=[&](int k) {
        return car.position + forward * (k<2?p.rearAxle:p.frontAxle) +
               side*((k%2==0?-1:1)*p.halfWheelTrack);
    };
    // Match the GT3 model's actual exhaust-tip coordinates (-1.385m, +/-0.245m).
    // The previous anchor used the rear axle offset and placed the flames inside
    // the bodywork, where the car hid them.
    const auto exhaustPosition=[&](int direction) {
        return car.position+forward*(-1.405/kWorldScale) +
               side*(direction*.245/kWorldScale);
    };
    for (int k=0;k<4;++k) {
        const auto wheel=wheelPosition(k);
        if (skid && lastSkidding_ && telemetry.wheelsOutside<2) {
            marks_.push_back({previous_[k],wheel,0});
            if (marks_.size()>1400) marks_.pop_front();
        }
        previous_[k]=wheel;
    }
    lastSkidding_=skid;

    emitTime_+=dt;
    std::uniform_real_distribution<double> velocity(-1.0,1.0);
    if (skid && emitTime_>=.035 && telemetry.wheelsOutside<2) {
        emitTime_=0;
        for (int rearWheel:{0,1}) {
            const auto pos=wheelPosition(rearWheel);
            particles_.push_back({pos,elevation(pos)+13,
                        velocity(generator_)*28,velocity(generator_)*28,
                        17+std::abs(velocity(generator_))*27,
                        .65, .65, 0});
        }
    }
    if (car.brake>.35 && car.brakeHeat>.2 && car.speed>120 &&
        std::uniform_int_distribution<int>(0,2)(generator_)==0) {
        // Sparks leave the body sideways instead of drifting through the car.
        for (const int direction : {-1, 1}) {
            for (int burst=0; burst<2; ++burst) {
                const double outward=direction*(105+std::abs(velocity(generator_))*95);
                const double along=-18+velocity(generator_)*75;
                const auto pos=car.position+forward*(p.frontAxle*.35) +
                               side*(direction*(p.halfWheelTrack+3));
                particles_.push_back({pos,elevation(pos)+12,
                    side.x*outward+forward.x*along,
                    side.y*outward+forward.y*along,
                    55+std::abs(velocity(generator_))*90,
                    .48,.48,2});
            }
        }
    }
    // A short twin-exhaust backfire when the driver lifts off the throttle.
    // Keep each flame locked to the matching exhaust tip as the car moves.
    if (previousThrottle_ && !car.throttle && car.speed>180) {
        for (const int direction : {-1,1}) {
            const auto pos=exhaustPosition(direction);
            const double flicker=std::abs(velocity(generator_));
            particles_.push_back({pos,elevation(pos)+.255/kWorldScale,
                -forward.x*(205+flicker*105),
                -forward.y*(205+flicker*105),
                10+flicker*35,.30,.30,1,direction});
            particles_.push_back({pos+forward*(-2),elevation(pos)+.255/kWorldScale,
                -forward.x*(235+flicker*95),
                -forward.y*(235+flicker*95),
                6+flicker*28,.23,.23,3,direction});
        }
    }
    previousThrottle_=car.throttle;
    for (auto& particle:particles_) {
        if ((particle.kind==1 || particle.kind==3) && particle.exhaustSide!=0) {
            const auto pos=exhaustPosition(particle.exhaustSide);
            particle.location=pos;
            particle.height=elevation(pos)+.255/kWorldScale;
        } else {
            particle.location=particle.location+Vec2{particle.dx,particle.dy}*dt;
            particle.height+=particle.dz*dt;
            if (particle.kind==2) particle.dz-=500*dt;
        }
        particle.life-=dt;
    }
    particles_.erase(std::remove_if(particles_.begin(),particles_.end(),
                        [](const Particle& p){return p.life<=0;}),particles_.end());
    if (particles_.size()>350)
        particles_.erase(particles_.begin(),particles_.begin()+std::ptrdiff_t(particles_.size()-350));
    for (auto& m:marks_) m.age+=dt;
    while(!marks_.empty() && marks_.front().age>30) marks_.pop_front();
}

void Effects::draw3D(const Camera3D& camera) const {
    const auto position=camera.position;
    for (const auto& mark:marks_) {
        const auto a=worldPoint(mark.start,2),b=worldPoint(mark.end,2);
        const float dx=a.x-position.x,dz=a.z-position.z;
        if (dx*dx+dz*dz>50*50) continue;
        DrawLine3D(a,b,{30,30,32,255});
    }
    for (const auto& p:particles_) {
        const auto pos=Vector3{float(p.location.x*kWorldScale),
                               float(p.height*kWorldScale),
                               float(p.location.y*kWorldScale)};
        const float age=float(std::max(0.0,p.life/p.maxLife));
        if (p.kind==0)
            DrawSphere(pos,(1.0f-age)*.24f+.09f,
                       Fade(LIGHTGRAY,age*.55f));
        else if (p.kind==2) {
            // Keep braking sparks as compact bright particles.
            DrawSphere(pos,.08f*age,Fade(YELLOW,age));
        } else {
            // Normalize the particle direction without relying on raymath.
            const float length=std::sqrt(float(p.dx*p.dx+p.dy*p.dy));
            const float dirX=length>0.001f?float(p.dx)/length:0.0f;
            const float dirY=length>0.001f?float(p.dy)/length:0.0f;
            const Vector3 tip={pos.x+dirX*.48f,
                               pos.y+float(p.dz)*.0018f,
                               pos.z+dirY*.48f};
            if (p.kind==1) {
                DrawCylinderEx(pos,tip,.13f*age,.018f*age,7,
                               Fade(ORANGE,age));
                DrawSphere(pos,.105f*age,Fade(RED,age));
            } else {
                DrawCylinderEx(pos,tip,.065f*age,.008f*age,6,
                               Fade(YELLOW,age));
                DrawSphere(pos,.055f*age,Fade(WHITE,age));
            }
        }
    }
}

} // namespace karting
