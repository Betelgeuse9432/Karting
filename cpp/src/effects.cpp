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
    sparkCooldown_=0;
    backfireArmed_=false;
    emitTime_=0;
    for (auto& p:previous_) p={};
    previousHeight_.fill(0.0);
}

void Effects::update(const Simulation& sim, double dt) {
    const auto& car=sim.car();
    const auto& p=sim.profile();
    const auto& telemetry=sim.telemetry();
    const auto roadHeight=[&](Vec2 pos) {
        return sim.track().roadHeightAt(pos,car.roadIndex);
    };
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
            marks_.push_back({previous_[k],wheel,0,
                              previousHeight_[k],roadHeight(wheel)});
            if (marks_.size()>1400) marks_.pop_front();
        }
        previous_[k]=wheel;
        previousHeight_[k]=roadHeight(wheel);
    }
    lastSkidding_=skid;

    emitTime_+=dt;
    std::uniform_real_distribution<double> velocity(-1.0,1.0);
    if (skid && emitTime_>=.035 && telemetry.wheelsOutside<2) {
        emitTime_=0;
        for (int rearWheel:{0,1}) {
            const auto pos=wheelPosition(rearWheel);
            particles_.push_back({pos,roadHeight(pos)+13,
                        velocity(generator_)*28,velocity(generator_)*28,
                        17+std::abs(velocity(generator_))*27,
                        .65, .65, 0});
        }
    }
    // Controlled brake sparks: visible beside the rear wheels and emitted
    // during sustained strong braking. No effect on driving physics.
    sparkCooldown_=std::max(0.0,sparkCooldown_-dt);
    if(car.brake>.72 && car.brakeHeat>.09 &&
       std::abs(car.speed)>.30*p.topSpeed && sparkCooldown_<=0.0) {
        const int direction=std::uniform_int_distribution<int>(0,1)(generator_)?1:-1;
        const Vec2 pos=car.position+forward*p.rearAxle+
                       side*(direction*(p.halfWheelTrack+16));
        for(int i=0;i<2;++i) {
            const double flicker=velocity(generator_);
            particles_.push_back({pos,roadHeight(pos)+8,
                -forward.x*(55+20*i)+side.x*direction*(65+20*flicker),
                -forward.y*(55+20*i)+side.y*direction*(65+20*flicker),
                8+std::abs(flicker)*14,
                .24,.24,2});
        }
        sparkCooldown_=.11;
    }

    // Backfire on the FIRST throttle-release frame, not when a narrowly
    // chosen threshold happens to be crossed in one simulation tick.
    backfireCooldown_=std::max(0.0,backfireCooldown_-dt);
    if(car.throttle>previousThrottle_+.015 || car.throttle>.75)
        backfireArmed_=true;
    const bool released=previousThrottle_>.38 &&
                        car.throttle<previousThrottle_-.045;
    if(sim.vehicleIndex()==2 && backfireArmed_ && released &&
       car.speed>.35*p.topSpeed && backfireCooldown_<=0.0) {
        for(const int direction:{-1,1}) {
            // GT3 body ends around local X=-1.26m. Start outside the
            // rear bumper at both visible exhaust outlets.
            const Vec2 pos=car.position+forward*(-1.43/kWorldScale)+
                           side*(direction*.25/kWorldScale);
            particles_.push_back({pos,roadHeight(pos)+.255/kWorldScale,
                -forward.x*85,-forward.y*85,0,
                .24,.24,1});
        }
        backfireArmed_=false;
        backfireCooldown_=.40;
    }
    if(car.throttle<.08) backfireArmed_=false;
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
        const auto a=Vector3{float(mark.start.x*kWorldScale),
                             float((mark.startHeight+2)*kWorldScale),
                             float(mark.start.y*kWorldScale)};
        const auto b=Vector3{float(mark.end.x*kWorldScale),
                             float((mark.endHeight+2)*kWorldScale),
                             float(mark.end.y*kWorldScale)};
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
            // Compact flame cone behind the bumper, not a floating orb.
            // At full lifetime the cone extends ~0.34m, easily visible
            // from the chase camera without obscuring the vehicle.
            const float strength=std::clamp(age*2.f,0.f,1.f);
            const float len=.20f+.15f*strength;
            const float dx=float(p.dx),dz=float(p.dy);
            const float norm=std::max(.001f,std::sqrt(dx*dx+dz*dz));
            const Vector3 tip{pos.x+dx/norm*len,pos.y+.025f,
                              pos.z+dz/norm*len};
            DrawCylinderEx(pos,tip,.105f*strength,.012f,6,
                           Fade({255,151,43,255},age));
            const Vector3 core{pos.x+(tip.x-pos.x)*.58f,
                               pos.y+.010f,
                               pos.z+(tip.z-pos.z)*.58f};
            DrawCylinderEx(pos,core,.050f*strength,.006f,5,
                           Fade({255,232,136,255},age));
        } else {
            const float dx=float(p.dx),dz=float(p.dy);
            const float norm=std::max(.001f,std::sqrt(dx*dx+dz*dz));
            const Vector3 tip{pos.x+dx/norm*.27f,
                              pos.y+.045f,
                              pos.z+dz/norm*.27f};
            DrawCylinderEx(pos,tip,.020f*age,.004f,4,
                           Fade({255,209,102,255},age));
        }
    }
}

} // namespace karting
