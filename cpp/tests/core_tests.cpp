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
    require(track.size()==72*Track::subdivisions,
            "New circuit has 72 Catmull-Rom control points");
    require(track.lapLength()>50000 && track.lapLength()<65000,
            "Long circuit has a realistic controlled lap length");
    const auto start=track.startPosition();
    require(start.x>0 && start.x<Track::worldWidth &&
            start.y>0 && start.y<Track::worldHeight,
            "Starting point fits the extended world");
    require(std::isfinite(track.startHeading()),"Finite starting heading");
    require(track.heights().size()==track.size(),"Cached relief per vertex");
    const auto near=track.info(start);
    require(near.distance<1e-6,"Start line is on the track");
    require(!near.grass,"No grass at start");
    for(const Vec2 p:track.center())
        require(p.x>0 && p.x<Track::worldWidth &&
                p.y>0 && p.y<Track::worldHeight,
                "Whole track fits within world bounds");
    for(std::size_t i=0;i<track.size();i+=Track::subdivisions) {
        const auto point=track.point(i);
        require(track.info(point).distance<1e-5,
                "Interpolated points remain on the track");
    }
}

void testProfiles() {
    require(profiles().size()==4,"4 cars");
    require(profiles()[0].name=="KART","Kart present");
    require(profiles()[1].name=="4x4","4x4 present");
    require(profiles()[2].name=="GT3","GT3 present");
    require(profiles()[3].name=="WEC","WEC present");
    const int maxSpeed[]={550,570,680,770};
    const int maxPower[]={850,700,980,1150};
    for (std::size_t i=0;i<profiles().size();++i) {
        const auto& p=profiles()[i];
        close(p.topSpeed,maxSpeed[i],1e-9,"Reduced speed cap matches driving balance");
        close(p.power,maxPower[i],1e-9,"Reduced engine power matches balance");
        require(p.throttleRate<=1.1,"Throttle response is manageable");
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

void testSpeedDistanceCalibration() {
    // The speedometer and rendered displacement use the same physical unit.
    close(3.6*kWorldScale*kTravelScale,kKmHPerSpeedUnit,1e-12,
          "Speedometer and world-distance calibration must agree");
    close(toKmH(1000.0),300.0,1e-9,"Speedometer 300 km/h reference");

    Simulation sim;
    sim.selectVehicle(2);
    for(int i=0;i<10;++i)
        sim.update({true,false,0,false},1.0/60.0);
    const Vec2 before=sim.car().position;
    sim.update({true,false,0,false},1.0/60.0);
    const double worldDistance=length(sim.car().position-before);
    const double observedKmH=worldDistance*kWorldScale*60.0*3.6;
    close(observedKmH,std::abs(toKmH(sim.car().speed)),1e-5,
          "Displayed speed equals world motion on starting straight");
    require(std::isfinite(sim.car().heading),
            "Speed calibration preserves finite steering");
}

void testGripRainAndDrift() {
    Simulation dry,rain,braking;
    dry.selectVehicle(2);
    rain.selectVehicle(2);
    braking.selectVehicle(2);
    rain.toggleRain();
    // Weather gradually converges; no instantaneous rain switch.
    // Keep the comparison near the starting straight, before wall impacts
    // can skew vehicle speed or aerodynamic grip.
    for (int i=0;i<45;++i) {
        dry.update({true,false,0,false},1./60);
        rain.update({true,false,0,false},1./60);
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
        testSpeedDistanceCalibration();
        testGripRainAndDrift();
        testFixedTiming();
        testResetAndParameters();
        testBarrier();
        std::cout<<"8 suites de tests C++ reussies.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr<<"TEST FAILED: "<<e.what()<<'\n';
        return 1;
    }
}
