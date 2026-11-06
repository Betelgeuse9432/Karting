#include "karting/core.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
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
    require(track.size()==42*Track::subdivisions,
            "Four-sector circuit has 42 Catmull-Rom control points");
    require(track.lapLength()>69000 && track.lapLength()<75000,
            "Four-sector circuit preserves a long lap");
    const auto start=track.startPosition();
    require(start.x>0 && start.x<Track::worldWidth &&
            start.y>0 && start.y<Track::worldHeight,
            "Starting point fits the extended world");
    require(std::isfinite(track.startHeading()),"Finite starting heading");
    require(track.heights().size()==track.size(),"Cached relief per vertex");
    // New relief must be more pronounced without steep discontinuities.
    const auto& deck=track.heights();
    const auto [minElevation,maxElevation]=std::minmax_element(deck.begin(),deck.end());
    require(*maxElevation-*minElevation>700,
            "The same XY course now has meaningful climbs and descents");
    for(std::size_t i=0;i<track.size();++i) {
        const std::size_t next=(i+1)%track.size();
        const double run=length(track.point(next)-track.point(i));
        require(run>1e-8,"Track samples remain distinct");
        const double grade=std::abs(track.roadHeight(next)-track.roadHeight(i))/run;
        require(grade<.20,"No sudden slope or unrealistic road crest");
    }
    const auto near=track.info(start);
    require(near.distance<1e-6,"Start line is on the track");
    require(!near.grass,"No grass at start");

    // Separate terrain altitude from per-road-sample altitude. For the
    // unchanged circuit, they must produce the exact old surface height.
    for(std::size_t i=0;i<track.size();i+=Track::subdivisions) {
        const Vec2 location=track.point(i);
        close(track.terrainHeight(location),elevation(location),1e-10,
              "Terrain height preserves existing map");
        close(track.roadHeight(i),elevation(location),1e-10,
              "Deck height preserves existing track");
        const auto onDeck=track.infoAtHeight(location,track.roadHeight(i),i);
        require(onDeck.index==i,
                "3D lookup returns the corresponding current road sample");
        require(onDeck.distance<1e-5 && !onDeck.grass,
                "3D lookup preserves the drivable road surface");
    }
    require(track.roadHeight(track.size())==track.roadHeight(0),
            "Deck height wraps at the finish line");
    bool invalidHeight=false;
    try {
        (void)track.infoAtHeight(track.startPosition(),
                                std::numeric_limits<double>::infinity());
    } catch(const std::invalid_argument&) { invalidHeight=true; }
    require(invalidHeight,"Height lookup rejects non-finite altitude");
    const auto noHint=track.infoAtHeight(track.startPosition(),
                                         track.roadHeight(0));
    require(noHint.index==0,"3D lookup works without route hint");

    // Simulate a bridge DECK without changing a single XY control point.
    // The ground remains the same, while 3D lookup follows the elevated
    // sample instead of a nearby segment at ground level.
    Track layered;
    auto profile=layered.heights();
    const std::size_t raised=8*Track::subdivisions;
    const Vec2 crossing=layered.point(raised);
    const double originalGround=layered.terrainHeight(crossing);
    profile[raised]=originalGround+600.0;
    layered.setRoadHeightProfile(profile);
    close(layered.roadHeight(raised),originalGround+600,1e-10,
          "Deck can be raised independently from terrain");
    close(layered.terrainHeight(crossing),originalGround,1e-10,
          "Raising a deck never modifies the ground");
    require(layered.infoAtHeight(crossing,originalGround+600,raised).index==raised,
            "Elevated road level is correctly selected");
    require(layered.infoAtHeight(crossing,originalGround).index!=raised,
            "Ground level lookup does not select an elevated deck");
    bool invalidProfile=false;
    try { layered.setRoadHeightProfile({1.0,2.0}); }
    catch(const std::invalid_argument&) {invalidProfile=true;}
    require(invalidProfile,"Deck profile must match track sample count");
    invalidProfile=false;
    profile[raised]=std::numeric_limits<double>::quiet_NaN();
    try { layered.setRoadHeightProfile(profile); }
    catch(const std::invalid_argument&) {invalidProfile=true;}
    require(invalidProfile,"Deck profile rejects non-finite heights");


    // Arc-length lookup must remain valid after changing the spline density.
    require(track.indexAtDistance(0)==0,
            "Distance zero selects the start");
    require(track.indexAtDistance(track.lapLength())==0,
            "Lap length wraps to the start");
    require(track.indexAtDistance(-track.lapLength())==0,
            "Negative full lap wraps to the start");
    for(int sector=1;sector<20;++sector) {
        const double d=track.lapLength()*sector/20.;
        const std::size_t i=track.indexAtDistance(d);
        const std::size_t prev=track.indexAtDistance(d-200.);
        require(i<track.size(),"Distance lookup is in bounds");
        require(i>=prev || (sector==1 && prev>track.size()-5),
                "Distance-based scenic placement follows the lap order");
    }
    bool invalidDistance=false;
    try { (void)track.indexAtDistance(std::numeric_limits<double>::infinity()); }
    catch(const std::invalid_argument&) { invalidDistance=true; }
    require(invalidDistance,"Non-finite arc distance is rejected");
    require(Track::halfWidth==175 && Track::wall==275,
            "Road widened with matching barrier clearance");
    require(!track.info(track.point(0,Track::halfWidth-8)).grass,
            "Outer road lane remains drivable");
    require(track.info(track.point(0,Track::halfWidth+Track::kerb+45)).grass,
            "Grass begins past the wider road shoulder");

    // At the hairpin the vehicle approaches eastward, pivots and exits west.
    const auto tangent=[&](std::size_t pointIndex) {
        const auto& c=track.center();
        return c[(pointIndex+1)%c.size()]-
               c[(pointIndex+c.size()-1)%c.size()];
    };
    const Vec2 entry=tangent(4*Track::subdivisions);
    const Vec2 apex=tangent(7*Track::subdivisions);
    const Vec2 exit=tangent(10*Track::subdivisions);
    require(dot(entry,exit)<-.9*length(entry)*length(exit),
            "Hairpin reverses travel direction by roughly 180 degrees");
    require(dot(entry,apex)<.2*length(entry)*length(apex),
            "Hairpin includes a strong perpendicular apex");
    const Vec2 entryPoint=track.point(4*Track::subdivisions);
    const Vec2 exitPoint=track.point(10*Track::subdivisions);
    require(length(entryPoint-exitPoint)>Track::wall*2+100,
            "Hairpin arms have separate collision and scenery clearances");
    // Four sectors should exhibit direction changes beyond the hairpin,
    // rather than falling back to a featureless oval.
    int leftTurns=0,rightTurns=0;
    const auto& route=track.center();
    for(std::size_t i=12*Track::subdivisions;
        i+Track::subdivisions<route.size()-Track::subdivisions;
        i+=Track::subdivisions) {
        const Vec2 a=route[i]-route[i-Track::subdivisions];
        const Vec2 b=route[i+Track::subdivisions]-route[i];
        const double signedTurn=a.x*b.y-a.y*b.x;
        if(signedTurn>1e4) ++leftTurns;
        if(signedTurn< -1e4) ++rightTurns;
    }
    require(leftTurns>=4 && rightTurns>=4,
            "Technical sectors contain turns in both directions");
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
    const int maxSpeed[]={300,320,400,450};
    const int maxPower[]={320,280,390,460};
    for (std::size_t i=0;i<profiles().size();++i) {
        const auto& p=profiles()[i];
        close(p.topSpeed,maxSpeed[i],1e-9,"Reduced speed cap matches driving balance");
        close(p.power,maxPower[i],1e-9,"Reduced engine power matches balance");
        require(p.throttleRate<=0.65,"Throttle response is manageable");
        require(p.frontAxle>0 && p.rearAxle<0,"Wheel position valid");
    }
}

void testCarAcceleration() {
    for (int i=0;i<4;++i) {
        Simulation sim;
        sim.selectVehicle(i);
        const auto initial=sim.car().position;
        for (int k=0;k<60;++k) sim.update({true,false,0,false},1./60);
        require(sim.car().speed>35,"Car accelerates progressively within one second");
        require(length(sim.car().position-initial)>5,"Car moves forward");
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
