#include "karting/core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace karting {

Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 operator*(Vec2 a, double s) { return {a.x * s, a.y * s}; }
double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
double length(Vec2 a) { return std::hypot(a.x, a.y); }
double wrapAngle(double a) { return std::remainder(a, 2 * kPi); }

double elevation(Vec2 p) {
    return 70 * (0.6 * std::sin(p.x / 1100) +
                 0.4 * std::sin(p.y / 900 + 1) +
                 0.3 * std::sin((p.x + p.y) / 1400 + 2));
}

double bajaElevation(Vec2 p) {
    const double rolling=22.0*std::sin(p.x/185.0+0.4)*std::sin(p.y/235.0);
    const double ripples=8.0*std::sin((p.x+p.y)/73.0)+
                         5.5*std::sin((p.x-p.y)/49.0);
    const auto mound=[&](double x,double y,double radius,double height) {
        const double dx=p.x-x,dy=p.y-y;
        return height*std::exp(-(dx*dx+dy*dy)/(radius*radius));
    };
    const double rocks=
        mound(2100,3600,115,18)+mound(4300,3000,145,24)+
        mound(6500,2200,125,17)+mound(8200,6000,170,22)+
        mound(5300,7100,190,16);
    // Broad, asymmetric ramp profiles are part of the ground itself.
    // The front axle climbs the face before the rear axle reaches the crest.
    const double ramps=
        mound(2550,1690,260,24)+mound(7050,1320,220,19)+
        mound(8950,5750,280,27);
    return elevation(p)*.18+rolling*.48+ripples*.34+rocks*.55+ramps;
}

const std::array<VehicleProfile, 11>& profiles() {
    static const std::array<VehicleProfile, 11> cars{{
        {"KART", 650, 1300, 1.7, 2.0, 800, 950, 950, 0.0, 30, 0.7, 200,
         0.00005, 0.03, 0.4, 0.2, 0.5, 48, 125, 17, -15, 18, 0},
        {"4x4", 680, 1050, 1.15, 1.3, 560, 600, 650, 0.05, 52, 0.5, 150,
         0.0003, 0.12, 1.8, 0.04, 0.1, 85, 175, 30, -30, 26, 5},
        {"GT3", 4200, 3600, 0.95, 1.05, 1050, 740, 780, 0.34, 46, 0.55, 150,
         0.00012, 0.07, 1.0, 0.18, 0.45, 65, 150, 50, -48, 21, 3},
        {"WEC", 4100, 3400, 0.90, 1.10, 1450, 760, 800, 0.32, 50, 0.5, 140,
         0.00008, 0.05, 0.6, 0.3, 0.6, 58, 160, 30, -30, 22, 1},
        {"RALLY", 1120, 2200, 1.55, 2.0, 920, 900, 980, 0.22, 48, 0.48, 165,
         0.00018, 0.09, 1.2, 0.08, 0.22, 62, 145, 34, -32, 25, 5},
        {"F1", 5000, 4200, 0.85, 1.10, 1550, 1180, 1280, 0.22, 38, 0.85, 65,
         0.00010, 0.045, 0.55, 0.12, 0.14, 55, 155, 56, -52, 37, 1},
        {"GT3 MONSTER", 4300, 3700, 0.95, 1.05, 1100, 755, 800, 0.34, 46, 0.54, 152,
         0.00011, 0.068, 0.95, 0.17, 0.43, 65, 150, 50, -48, 21, 0},
        {"TROPHY 1", 980, 1850, 0.78, 1.55, 720, 470, 500, 0.015, 64, 0.62, 175,
         0.00032, 0.15, 2.1, 0.015, 0.08, 105, 205, 63, -61, 48, 5},
        {"TROPHY 2", 1040, 1980, 0.82, 1.60, 760, 495, 525, 0.018, 67, 0.60, 180,
         0.00030, 0.14, 2.0, 0.015, 0.08, 110, 212, 66, -64, 50, 1},
        {"TROPHY 3", 1010, 2050, 0.80, 1.58, 790, 485, 540, 0.012, 66, 0.61, 178,
         0.00031, 0.145, 2.05, 0.015, 0.08, 108, 210, 65, -63, 51, 2},
        {"GT3 RED BULL", 4350, 3750, 0.95, 1.05, 1120, 770, 815, 0.34, 46, 0.54, 152,
         0.00011, 0.068, 0.95, 0.17, 0.43, 65, 150, 50, -48, 21, 1}
    }};
    return cars;
}

Track::Track(bool baja, bool desert) : baja_(baja), desert_(desert) {
    // Apex Ridge: an original, continuous circuit with no crossings.
    // It combines a long opening run, a flowing outer section, a tight
    // technical complex and a fast final sweep back to the start.
    std::vector<Vec2> raw = desert_ ? std::vector<Vec2>{
        {5000,4500},{6000,4500},{5707,5207},{5000,5500},
        {4293,5207},{4000,4500},{4293,3793},{5000,3500}
    } : baja_ ? std::vector<Vec2>{
        {900,5200},{1250,3900},{2200,2850},{3700,2450},
        {5050,2850},{5800,2250},{6350,2050},{7100,1940},{7900,2250},
        {8500,2700},{9100,3300},{9400,4050},{9400,4750},
        {9000,5550},{8650,6250},{7900,6800},{7300,7150},
        {6400,7350},{5550,7250},{4700,6900},{3950,7150},
        {3100,7350},{2500,7200},{1800,7000},{1250,6700}
    } : std::vector<Vec2>{
        {1700,6500},{2300,7100},{3400,7350},{4400,7000},
        {5000,6250},{5700,5900},{6500,6100},{7300,6800},
        {8200,6900},{9000,6900},{9500,6250},{9500,5450},
        {9050,4850},{8500,4550},{7800,4550},{6900,4400},
        {6200,4750},{5500,4300},{5100,3400},{4300,2850},
        {3300,3000},{2700,3650},{2050,3900},{1700,3500},
        {1450,3550},{1250,3750},{1120,4050},{1080,4400},
        {1160,4750},{1330,5050},{1450,5550}
    };
    // Round tight control-point corners before spline interpolation.
    const auto unsmoothed=raw;
    for (std::size_t i=0;i<raw.size();++i) {
        const auto& prev=unsmoothed[(i+raw.size()-1)%raw.size()];
        const auto& next=unsmoothed[(i+1)%raw.size()];
        raw[i].x=unsmoothed[i].x*.76+(prev.x+next.x)*.12;
        raw[i].y=unsmoothed[i].y*.76+(prev.y+next.y)*.12;
    }
    // A larger footprint adds distance to the lap while retaining runoff.
    for (auto& p : raw) {
        p.x = 5300.0 + (p.x - 5300.0) * 1.14;
        p.y = 5000.0 + (p.y - 5000.0) * 1.66;
    }
    center_.reserve(raw.size() * subdivisions);
    for (std::size_t i = 0; i < raw.size(); ++i) {
        const auto a = raw[(i + raw.size() - 1) % raw.size()];
        const auto b = raw[i];
        const auto c = raw[(i + 1) % raw.size()];
        const auto d = raw[(i + 2) % raw.size()];
        for (int j = 0; j < subdivisions; ++j) {
            const double t = double(j) / subdivisions, t2 = t * t, t3 = t2 * t;
            const auto interpolate = [&](double a0,double b0,double c0,double d0) {
                return .5 * (2*b0 + (-a0+c0)*t +
                             (2*a0-5*b0+4*c0-d0)*t2 +
                             (-a0+3*b0-3*c0+d0)*t3);
            };
            center_.push_back({interpolate(a.x,b.x,c.x,d.x),
                               interpolate(a.y,b.y,c.y,d.y)});
        }
    }
    const auto n = center_.size();
    normals_.reserve(n);
    heights_.reserve(n);
    distances_.reserve(n);
    pit_.assign(n, false);
    for (std::size_t i = 0; i < n; ++i) {
        const Vec2 dir = center_[(i+1)%n] - center_[(i+n-1)%n];
        const double inverseLength = 1.0 / std::max(length(dir), 1e-8);
        normals_.push_back({-dir.y * inverseLength, dir.x * inverseLength});
        heights_.push_back(elevation(center_[i]));
        if (i) length_ += length(center_[i] - center_[i-1]);
        distances_.push_back(length_);
    }
    length_ += length(center_.back() - center_.front());

    // Pit-lane near the finish line, traversing the wraparound.
    const double stepLength = length_ / n;
    if (!baja_) {
        const int pitCount = std::max(20, int(1900/stepLength));
        const int pitStart = int(n) - std::max(3, int(500/stepLength));
        for (int k = 0; k < pitCount; ++k)
            pit_[std::size_t((pitStart+k) % int(n))] = true;
    }
}

Vec2 Track::point(std::size_t index, double offset) const {
    index %= center_.size();
    return center_[index] + normals_[index] * offset;
}
Vec2 Track::startPosition() const { return center_.front(); }
double Track::startHeading() const {
    const Vec2 delta = center_[1] - center_[0];
    return std::atan2(delta.y, delta.x);
}
bool Track::isPitIndex(std::size_t index) const {
    return pit_[index % pit_.size()];
}
TrackInfo Track::info(Vec2 world) const {
    if (!std::isfinite(world.x) || !std::isfinite(world.y))
        throw std::invalid_argument("Non-finite track coordinate");
    std::size_t best = 0;
    double nearest2 = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < center_.size(); ++i) {
        const auto delta = center_[i] - world;
        const double d2 = dot(delta, delta);
        if (d2 < nearest2) { nearest2 = d2; best = i; }
    }
    if (desert_) return {0,0,0,false,false};
    const double signedOffset = dot(world - center_[best], normals_[best]);
    const bool pits = pit_[best] && signedOffset > halfWidth + kerb &&
                      signedOffset < halfWidth + kerb + pitWidth;
    const bool grass = nearest2 > (halfWidth+kerb)*(halfWidth+kerb) && !pits;
    return {best, std::sqrt(nearest2), signedOffset, pits, grass};
}
bool Track::wheelOutside(Vec2 world, std::size_t approxIndex) const {
    if (desert_) return false;
    const auto n = center_.size();
    std::size_t nearest = approxIndex;
    double best2 = std::numeric_limits<double>::infinity();
    for (int k = -30; k <= 30; ++k) {
        const auto i = (approxIndex + n + std::size_t((k + int(n)) % int(n))) % n;
        const auto delta = center_[i] - world;
        const double d2 = dot(delta, delta);
        if (d2 < best2) { best2 = d2; nearest = i; }
    }
    if (best2 <= (halfWidth+kerb)*(halfWidth+kerb)) return false;
    const double side = dot(world-center_[nearest], normals_[nearest]);
    return !(pit_[nearest] && side>halfWidth+kerb &&
             side<halfWidth+kerb+pitWidth);
}
double Track::collide(Car& car, double dt) const {
    if (desert_) return 0.0;
    const auto nearest = info(car.position);
    const bool pitSide = pit_[nearest.index] && nearest.signedOffset>0;
    const double limit = pitSide ? halfWidth + kerb + pitWidth + 25 : wall - 18;
    if (nearest.distance <= limit) return 0;
    const auto direction = (car.position - center_[nearest.index]) *
                           (1.0 / std::max(nearest.distance, 1e-8));
    car.position = center_[nearest.index] + direction*limit;
    const double toward = std::max(0.0, dot({std::cos(car.heading),
                                            std::sin(car.heading)}, direction));
    if (toward<=0.05 || car.speed<=30) return 0;
    const double impact = toward * car.speed;
    car.speed *= 1.0-0.6*toward;
    const auto tangent = Vec2{-normals_[nearest.index].y,
                               normals_[nearest.index].x};
    double angle = std::atan2(tangent.y,tangent.x);
    if (std::cos(car.heading-angle) < 0) angle+=kPi;
    car.heading+=wrapAngle(angle-car.heading)*std::min(1.0,6*dt);
    car.lateralSpeed*=0.3;
    return impact;
}

void Weather::update(double dt) {
    const double targetNight = std::array<double,3>{0,0.55,1}.at(std::size_t(phase));
    nightLevel += (targetNight-nightLevel)*std::min(1.0,0.8*dt);
    rainLevel += ((raining?1.0:0.0)-rainLevel)*std::min(1.0,0.6*dt);
}

void Race::reset(bool keepBest) {
    const double oldBest = bestLap;
    *this = Race{};
    if (keepBest) bestLap = oldBest;
}

std::optional<double> Race::tick(const Track& track,const Car& car,int outside,double dt) {
    topSpeed=std::max(topSpeed,std::abs(car.speed));
    elapsed+=dt;
    warningTime=std::max(0.0,warningTime-dt);
    if (outside==4 && !outsideLastFrame) {
        elapsed+=1.0;
        warningTime=2.0;
    }
    outsideLastFrame=outside==4;
    const auto spot=track.info(car.position);
    const auto sector=int(spot.index*4/track.size());
    if (sector!=nextSector) return std::nullopt;
    std::optional<double> result;
    if (nextSector==0) {
        result=elapsed;
        ++laps;
        lastLap=elapsed;
        recent.push_back(elapsed);
        if (recent.size()>5) recent.erase(recent.begin());
        if (bestLap<0 || elapsed<bestLap) bestLap=elapsed;
        elapsed=0;
    }
    nextSector=(nextSector+1)%4;
    return result;
}

Simulation::Simulation() { reset(false); }

void Simulation::setMode(int mode) {
    (void)mode;
    track_ = Track();
    vehicleIndex_ = 2;
    colorIndex_ = profile().defaultColor;
    reset(false);
}

void Simulation::selectVehicle(int index,int color) {
    if (index<0 || index>=int(profiles().size()))
        throw std::out_of_range("Invalid vehicle profile");
    vehicleIndex_=index;
    colorIndex_=color<0?profile().defaultColor:color;
    if (colorIndex_<0 || colorIndex_>5) throw std::out_of_range("Invalid color");
    reset();
}

void Simulation::reset(bool preserveRecord) {
    car_=Car{};
    car_.position=track_.startPosition();
    car_.heading=track_.startHeading();
    opponent_=Car{};
    if(track_.isBaja()) {
        const std::size_t start=std::min<std::size_t>(5,track_.size()-1);
        opponent_.position=track_.point(start,55.0);
        const Vec2 direction=track_.center()[(start+1)%track_.size()]-
                             track_.center()[(start+track_.size()-1)%track_.size()];
        opponent_.heading=std::atan2(direction.y,direction.x);
    }
    race_.reset(preserveRecord);
    telemetry_=Telemetry{};
    boostTimeRemaining_=0.0;
    boostsRemaining_=2;
}

void Simulation::setBestLap(double seconds) {
    if (seconds>0 && std::isfinite(seconds)) race_.bestLap=seconds;
}

void Simulation::update(const Controls& controls,double dt) {
    if (!(dt>0 && dt<=.05 && std::isfinite(dt)))
        throw std::invalid_argument("Invalid simulation timestep");
    const auto& p=profile();
    if (controls.boostPressed && boostsRemaining_>0 && boostTimeRemaining_<=0.0) {
        boostTimeRemaining_=2.0;
        --boostsRemaining_;
    }
    const bool boostActive=boostTimeRemaining_>0.0;
    weather_.update(dt);
    const auto oldPosition=car_.position;
    const auto before=track_.info(oldPosition);
    const auto forward=Vec2{std::cos(car_.heading),std::sin(car_.heading)};
    const auto left=Vec2{-forward.y,forward.x};
    int off=0;
    for (const double x: {p.rearAxle,p.frontAxle}) {
        for (const double z: {-p.halfWheelTrack,p.halfWheelTrack})
            off+=int(track_.wheelOutside(oldPosition+forward*x+left*z,before.index));
    }

    const double throttleTarget=controls.accelerate?1.0:0.0;
    const double brakeTarget=controls.brake?1.0:0.0;
    const double throttleResponse=1.0-std::exp(-(controls.accelerate?p.throttleRate:2.4)*dt);
    const double brakeResponse=1.0-std::exp(-(controls.brake?std::max(p.brakeRate,4.0):2.0)*dt);
    car_.throttle+=(throttleTarget-car_.throttle)*throttleResponse;
    car_.brake+=(brakeTarget-car_.brake)*brakeResponse;

    const double v=car_.speed;
    const double downforce=1+p.downforce*std::pow(std::abs(v)/p.topSpeed,2);
    const double grip=downforce*(1-0.3*weather_.rainLevel)*(1-p.grassGrip*off/4.0);
    const double frontGrip=std::max(1.0,p.frontGrip*grip);
    const double rearGrip=std::max(1.0,p.rearGrip*grip);
    const double maxSteer=p.maxSteer/(1+std::abs(v)/(p.steerSensitivity*1.25));
    car_.steering+=(std::clamp(controls.steer,-1,1)*maxSteer-car_.steering)*
                    std::min(1.0,8*dt);
    double rotation=v*std::tan(car_.steering)/p.wheelbase;
    const double aLateral=std::abs(v*rotation);
    const double aBrake=v>0.5?p.brakeForce*std::pow(car_.brake,1.25):0.0;
    const double frontUse=std::hypot(aBrake,aLateral);
    const bool frontSlip=frontUse>frontGrip;
    if (frontSlip) rotation*=frontGrip/frontUse;

    const double targetSpeed=p.topSpeed*(boostActive?1.18:1.0);
    double motor=p.power*car_.throttle*
                 (1-std::max(v,0.0)/targetSpeed)*(boostActive?1.65:1.0);
    const double rearUse=std::hypot(motor,aLateral);
    const bool rearSlip=rearUse>rearGrip;
    if (rearSlip) {
        const double excess=rearUse/rearGrip-1;
        // Rear slip can help rotate the car on track, but should not create
        // extra yaw when the tyres are already running through grass.
        const double onTrack=1.0-double(off)/4.0;
        rotation*=1+onTrack*1.0*std::min(excess,0.8);
        motor*=rearGrip/rearUse;
    }
    // Grass strongly washes out steering, preventing sharp turns off-track.
    rotation *= 1.0 - 0.45 * (double(off) / 4.0);

    const auto groundHeight=[&](Vec2 point){return track_.isBaja()?bajaElevation(point):track_.isDesert()?0.0:elevation(point);};
    const double slope=(groundHeight(oldPosition+forward*6)-groundHeight(oldPosition))/6;
    double nextSpeed=v+motor*dt;
    if (v>0.5) nextSpeed-=aBrake*dt;
    else if (controls.brake) nextSpeed-=250*dt;
    nextSpeed-=nextSpeed*(controls.accelerate?.18:.5)*dt;
    nextSpeed-=nextSpeed*p.grassDrag*0.45*off*dt;
    nextSpeed-=slope*1200*dt;
    const double speedLimit=p.topSpeed*(boostActive?1.18:1.0);
    car_.speed=std::clamp(nextSpeed,-150.0,speedLimit);

    const double dAngle=rotation*dt;
    if (std::abs(car_.lateralSpeed)>.01) {
        double side=car_.lateralSpeed*std::cos(dAngle)-car_.speed*std::sin(dAngle);
        const double recovery=950.0+double(off)*180.0;
        side=std::copysign(std::max(0.0,std::abs(side)-recovery*dt),side);
        car_.lateralSpeed=std::abs(side)>=.01?side:0.0;
    } else car_.lateralSpeed=0;
    car_.heading+=dAngle;
    const Vec2 carForward{std::cos(car_.heading),std::sin(car_.heading)};
    const Vec2 carSide{-carForward.y,carForward.x};
    car_.position=car_.position+(carForward*car_.speed+
                                  carSide*car_.lateralSpeed)*dt;
    if(track_.isBaja()) {
        car_.jumpCooldown=std::max(0.0,car_.jumpCooldown-dt);
        const auto jumpSpot=track_.info(car_.position);
        const double progress=double(jumpSpot.index)/double(track_.size());
        for(const double target:{.10,.29,.49,.69,.88}) {
            if(std::abs(progress-target)<.0035 && car_.jumpCooldown<=0.0 &&
               std::abs(car_.speed)>240.0) {
                const double launchVariation=1.0+0.16*std::sin(double(jumpSpot.index)*.013);
                car_.verticalSpeed=(58.0+std::abs(car_.speed)*.14)*launchVariation;
                car_.jumpCooldown=1.9;
                break;
            }
        }
        if(car_.jumpHeight>0.0 || car_.verticalSpeed>0.0) {
            car_.verticalSpeed-=390.0*dt;
            car_.jumpHeight+=car_.verticalSpeed*dt;
            if(car_.jumpHeight<=0.0) {
                car_.jumpHeight=0.0;
                car_.verticalSpeed=0.0;
            }
        }
    } else {
        car_.jumpHeight=0.0;
        car_.verticalSpeed=0.0;
        car_.jumpCooldown=0.0;
    }
    car_.position.x=std::clamp(car_.position.x,0.0,10000.0);
    car_.position.y=std::clamp(car_.position.y,0.0,9000.0);
    const double impact=track_.collide(car_,dt);
    car_.brakeHeat=std::clamp(car_.brakeHeat+
            (car_.brake>.05 && car_.speed>0?
                 car_.brake*(std::abs(car_.speed)/p.topSpeed)*2.8*dt :
                 -0.32*dt),0.0,1.0);
    if (before.pits) car_.brakeHeat=0;

    // Longitudinal weight transfer: braking compresses the front and
    // lifts the rear; acceleration squats the rear and lifts the nose.
    double pitchTarget=((car_.throttle*.045-car_.brake*.095)*
            (track_.isDesert()?1.0:p.pitchFactor)+
            (track_.isBaja()?slope:-slope));
    // In the air, pitch follows vertical velocity: nose rises on take-off,
    // then progressively points down as the truck descends.
    if(track_.isBaja())
        pitchTarget+=0.34*std::clamp(car_.verticalSpeed/62.0,-1.0,1.0);
    car_.pitch+=(pitchTarget-car_.pitch)*std::min(1.0,6*dt);
    // The chassis leans into the corner on the Desert Trophy Truck.
    // The render transform's roll axis has the opposite sign to lateral
    // acceleration, so invert the target for this world only.
    const double rollDirection=track_.isDesert()?-1.0:1.0;
    const double rollTarget=std::clamp(
        rollDirection*p.rollFactor*car_.speed*rotation,
        -p.maxRoll,p.maxRoll);
    car_.roll+=(rollTarget-car_.roll)*std::min(1.0,6*dt);
    const double wingTarget=.1+.25*std::abs(car_.speed)/p.topSpeed+.9*car_.brake;
    car_.wing+=(wingTarget-car_.wing)*std::min(1.0,10*dt);
    car_.wheelRotation+=car_.speed*dt/10*.35;
    telemetry_={off,frontSlip,rearSlip,std::abs(car_.lateralSpeed)>12,
                before.pits,grip,impact,slope};

    // A second Trophy Truck follows the same route as a lightweight rival.
    // It aims at a point ahead on the course and adapts speed to corner angle.
    if(track_.isBaja()) {
        const auto aiInfo=track_.info(opponent_.position);
        const std::size_t targetIndex=(aiInfo.index+22)%track_.size();
        const Vec2 aiTargetPoint=track_.point(targetIndex,0.0);
        const Vec2 toTarget=aiTargetPoint-opponent_.position;
        const double targetHeading=std::atan2(toTarget.y,toTarget.x);
        const double headingError=wrapAngle(targetHeading-opponent_.heading);
        const auto& aiProfile=profiles()[opponentVehicleIndex()];
        const double steerTarget=std::clamp(headingError*.82,-.52,.52);
        opponent_.steering+=(steerTarget-opponent_.steering)*std::min(1.0,3.5*dt);
        const double rawTurnRate=
            opponent_.speed*std::tan(opponent_.steering)/aiProfile.wheelbase;
        const double turnRate=rawTurnRate<-.82?-.82:
                              rawTurnRate>.82?.82:rawTurnRate;
        const double aiTargetSpeed=std::clamp(820.0-std::abs(headingError)*430.0,
                                              430.0,790.0);
        opponent_.speed+=std::clamp(aiTargetSpeed-opponent_.speed,
                                    -720.0*dt,430.0*dt);
        opponent_.heading=wrapAngle(opponent_.heading+turnRate*dt);
        opponent_.position=opponent_.position+
            Vec2{std::cos(opponent_.heading),std::sin(opponent_.heading)}*
            (opponent_.speed*dt);
        opponent_.jumpCooldown=std::max(0.0,opponent_.jumpCooldown-dt);
        const auto aiJumpSpot=track_.info(opponent_.position);
        const double aiProgress=double(aiJumpSpot.index)/double(track_.size());
        for(const double jumpProgressTarget:{.08,.18,.31,.48,.62,.77,.90}) {
            if(std::abs(aiProgress-jumpProgressTarget)<.0035 && opponent_.jumpCooldown<=0.0 &&
               opponent_.speed>240.0) {
                const double launchVariation=1.0+0.16*std::sin(double(aiJumpSpot.index)*.013);
                opponent_.verticalSpeed=(58.0+opponent_.speed*.14)*launchVariation;
                opponent_.jumpCooldown=1.9;
                break;
            }
        }
        if(opponent_.jumpHeight>0.0 || opponent_.verticalSpeed>0.0) {
            opponent_.verticalSpeed-=390.0*dt;
            opponent_.jumpHeight+=opponent_.verticalSpeed*dt;
            if(opponent_.jumpHeight<=0.0) {
                opponent_.jumpHeight=0.0;
                opponent_.verticalSpeed=0.0;
            }
        }
        track_.collide(opponent_,dt);
        opponent_.wheelRotation+=opponent_.speed*dt/10.0*.35;
    }
    // Crossing the finish line completes a lap: refill both boost charges.
    if (race_.tick(track_,car_,off,dt).has_value())
        boostsRemaining_=2;
    boostTimeRemaining_=std::max(0.0,boostTimeRemaining_-dt);

    if (!std::isfinite(car_.position.x)||!std::isfinite(car_.position.y)||
        !std::isfinite(car_.speed)||!std::isfinite(car_.heading))
        throw std::runtime_error("Physics produced non-finite state");
}

int FixedClock::consume(double frameSeconds) {
    if (!std::isfinite(frameSeconds)||frameSeconds<0)
        throw std::invalid_argument("Invalid frame duration");
    accumulated_+=std::min(frameSeconds,.15);
    const int steps=std::min(8,int((accumulated_+1e-10)/step));
    accumulated_-=steps*step;
    if (steps==8) accumulated_=std::min(accumulated_,step);
    accumulated_=std::max(0.0,accumulated_);
    return steps;
}

} // namespace karting
