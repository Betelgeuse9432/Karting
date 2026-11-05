#include "karting/game.hpp"
#include <algorithm>
#include <cmath>

namespace karting {

void Effects::clear() {
    marks_.clear();
    particles_.clear();
    lastSkidding_=false;
    previousThrottle_=0;
    backfireCooldown_=0;
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
    // Sparse, side-directed brake sparks rather than permanent fireworks.
    // All particles are tiny and fade rapidly; keep physics unchanged.
    if (car.brake>.68 && car.brakeHeat>.35 &&
        std::abs(car.speed)>.55*p.topSpeed &&
        std::uniform_int_distribution<int>(0,10)(generator_)==0) {
        const int direction=std::uniform_int_distribution<int>(0,1)(generator_)?1:-1;
        const Vec2 pos=car.position+forward*(p.frontAxle*.34)+
                       side*(direction*(p.halfWheelTrack+3));
        particles_.push_back({pos,elevation(pos)+11,
                    side.x*direction*90-forward.x*35,
                    side.y*direction*90-forward.y*35,
                    12+std::abs(velocity(generator_))*20,
                    .21,.21,2});
    }

    // Compact GT3 twin-exhaust pop when the driver lifts off.
    // Anchor at the existing GT3 exhaust tips, not its wheelbase.
    // Detect crossing a throttle threshold, not the floating-point zero.
    backfireCooldown_=std::max(0.0,backfireCooldown_-dt);
    if(sim.vehicleIndex()==2 && car.speed>.55*p.topSpeed &&
       previousThrottle_>.17 && car.throttle<=.12 &&
       backfireCooldown_<=0.0) {
        for(const int direction:{-1,1}) {
            const Vec2 pos=car.position+forward*(-1.29/kWorldScale)+
                           side*(direction*.25/kWorldScale);
            particles_.push_back({pos,elevation(pos)+.245/kWorldScale,
                       -forward.x*45,-forward.y*45,0,
                       .12,.12,1});
        }
        backfireCooldown_=.55;
    }
    previousThrottle_=car.throttle;
    for (auto& particle:particles_) {
        particle.location=particle.location+Vec2{particle.dx,particle.dy}*dt;
        particle.height+=particle.dz*dt;
        if (particle.kind==2) particle.dz-=500*dt;
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
        else if (p.kind==1) {
            // Small tapered orange core, no oversized spherical fireball.
            const Vector3 tip{pos.x+float(p.dx)*.0023f*age,
                              pos.y+.012f,
                              pos.z+float(p.dy)*.0023f*age};
            DrawCylinderEx(pos,tip,.060f*age,.008f*age,5,
                           Fade({255,158,61,255},age));
        } else {
            const Vector3 tip{pos.x+float(p.dx)*.0018f,
                              pos.y+.020f,
                              pos.z+float(p.dy)*.0018f};
            DrawLine3D(pos,tip,Fade({255,201,93,255},age));
        }
    }
}

} // namespace karting
