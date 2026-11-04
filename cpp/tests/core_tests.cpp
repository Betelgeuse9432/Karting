#include "karting/core.hpp"
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace karting;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void close(double actual,double expected,double tolerance,const char* message) {
    if (std::abs(actual-expected)>tolerance)
        throw std::runtime_error(message);
}

void testCircuit() {
    const Track track;
    require(track.size()==26*Track::subdivisions,"V6 Catmull-Rom point count");
    require(track.lapLength()>15000,"Circuit should be long");
    const auto start=track.startPosition();
    require(length(start-Vec2{3200,1700})<1e-9,"V6 start point unchanged");
    require(std::isfinite(track.startHeading()),"Finite starting heading");
    require(track.heights().size()==track.size(),"Cached relief per vertex");
    const auto near=track.info(start);
    require(near.distance<1e-6,"Start line is on the track");
    require(!near.grass,"No grass at start");
}

void testProfiles() {
    require(profiles().size()==4,"4 cars");
    require(profiles()[0].name=="KART","Kart present");
    require(profiles()[1].name=="4x4","4x4 present");
    require(profiles()[2].name=="GT3","GT3 present");
    require(profiles()[3].name=="WEC","WEC present");
    for (const auto& p:profiles()) {
        require(p.topSpeed>=650,"All speed limits increased");
        require(p.power>=1050,"All power limits increased");
        require(p.frontAxle>0 && p.rearAxle<0,"Wheel position valid");
    }
}

void testCarAcceleration() {
    for (int i=0;i<4;++i) {
        Simulation sim;
        sim.selectVehicle(i);
        const auto initial=sim.car().position;
        for (int k=0;k<60;++k) sim.update({true,false,0,false},1./60);
        require(sim.car().speed>100,"Car accelerates within one second");
        require(length(sim.car().position-initial)>20,"Car moves forward");
        require(std::isfinite(sim.car().position.x),"Finite physics");
    }
}

void testGripRainAndDrift() {
    Simulation dry,rain,braking;
    dry.selectVehicle(2);
    rain.selectVehicle(2);
    braking.selectVehicle(2);
    rain.toggleRain();
    // Weather gradually converges; no instantaneous rain switch.
    for (int i=0;i<120;++i) {
        dry.update({true,false,1,false},1./60);
        rain.update({true,false,1,false},1./60);
        braking.update({true,false,1,true},1./60);
    }
    require(rain.weather().rainLevel>0.3,"Weather transition");
    require(rain.telemetry().grip<dry.telemetry().grip,"Rain reduces grip");
    require(braking.car().speed<dry.car().speed,"Handbrake decelerates");
    require(std::isfinite(braking.car().lateralSpeed),"Drift remains finite");
}

void testFixedTiming() {
    FixedClock clock;
    int steps=0;
    for (int i=0;i<10;++i) steps+=clock.consume(.1);
    require(steps==60,"10 FPS must advance a simulated second");
    clock.reset();
    steps=0;
    for (int i=0;i<120;++i) steps+=clock.consume(1./120);
    require(steps==60,"120 FPS must advance a simulated second");
    clock.reset();
    require(clock.consume(.5)==8,"Extreme lag is bounded");
    bool error=false;
    try { clock.consume(-1); } catch(const std::invalid_argument&) { error=true; }
    require(error,"Negative frame duration rejected");
}

void testResetAndParameters() {
    Simulation sim;
    sim.selectVehicle(3,1);
    for (int i=0;i<60;++i) sim.update({true,false,0,false},1./60);
    sim.reset();
    require(sim.car().speed==0,"Reset stops the car");
    require(sim.vehicleIndex()==3 && sim.colorIndex()==1,
            "Reset preserves selection");
    require(sim.race().laps==0,"Reset resets laps");
    bool error=false;
    try { sim.update({},-1); } catch(const std::invalid_argument&) { error=true; }
    require(error,"Invalid timestep rejected");
    sim.setBestLap(74.5);
    sim.reset();
    close(sim.race().bestLap,74.5,.0001,"Best lap survives reset");
}

void testBarrier() {
    Simulation sim;
    const auto& track=sim.track();
    const auto idx=std::size_t(70);
    const auto wall=track.point(idx,Track::wall+150);
    Car impact;
    impact.position=wall;
    impact.heading=track.startHeading();
    impact.speed=180;
    track.collide(impact,1./60);
    const auto nearest=track.info(impact.position);
    require(nearest.distance<Track::wall+165,"Collision clamps outside world");
    require(std::isfinite(impact.position.x),"Collision remains finite");
}

int main() {
    try {
        testCircuit();
        testProfiles();
        testCarAcceleration();
        testGripRainAndDrift();
        testFixedTiming();
        testResetAndParameters();
        testBarrier();
        std::cout<<"7 suites de tests C++ reussies.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr<<"TEST FAILED: "<<e.what()<<'\n';
        return 1;
    }
}
