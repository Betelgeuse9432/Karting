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

const std::array<VehicleProfile, 8>& profiles() {
    static const std::array<VehicleProfile, 8> cars{{
        {"KART", 650, 1300, 1.7, 2.0, 800, 950, 950, 0.0, 30, 0.7, 200,
         0.00005, 0.03, 0.4, 0.2, 0.5, 48, 125, 17, -15, 18, 0},
        {"4x4", 680, 1050, 1.15, 1.3, 560, 600, 650, 0.05, 52, 0.5, 150,
         0.0003, 0.12, 1.8, 0.04, 0.1, 85, 175, 30, -30, 26, 5},
        {"GT3", 1280, 2450, 1.35, 1.6, 760, 740, 780, 0.55, 46, 0.55, 150,
         0.00012, 0.07, 1.0, 0.18, 0.45, 65, 150, 50, -48, 21, 3},
        {"WEC", 1250, 2000, 1.25, 2.4, 1100, 760, 800, 1.3, 50, 0.5, 140,
         0.00008, 0.05, 0.6, 0.3, 0.6, 58, 160, 30, -30, 22, 1},
        {"RALLY", 1120, 2200, 1.55, 2.0, 920, 900, 980, 0.22, 48, 0.48, 165,
         0.00018, 0.09, 1.2, 0.08, 0.22, 62, 145, 34, -32, 25, 5},
        {"F1", 1500, 2100, 0.95, 2.5, 1250, 1180, 1280, 2.2, 38, 0.85, 65,
         0.00010, 0.045, 0.55, 0.12, 0.14, 55, 155, 56, -52, 37, 1},
        {"TROPHY 1", 980, 1850, 0.78, 1.55, 720, 470, 500, 0.015, 64, 0.62, 175,
         0.00032, 0.15, 2.1, 0.015, 0.08, 105, 205, 63, -61, 48, 5},
        {"TROPHY 2", 1040, 1980, 0.82, 1.60, 760, 495, 525, 0.018, 67, 0.60, 180,
         0.00030, 0.14, 2.0, 0.015, 0.08, 110, 212, 66, -64, 50, 1}
    }};
    return cars;
}

Track::Track(bool baja) : baja_(baja) {
    // Apex Ridge: an original, continuous circuit with no crossings.
    // It combines a long opening run, a flowing outer section, a tight
    // technical complex and a fast final sweep back to the start.
    std::vector<Vec2> raw = baja_ ? std::vector<Vec2>{
        {900,5200},{1250,3900},{2200,2850},{3700,2450},
        {5050,2850},{6350,2050},{7900,2250},{9100,3300},
        {9400,4750},{8650,6250},{7300,7350},{5550,7650},
        {3950,7150},{2500,7550},{1250,6700}
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
    // Expand the circuit footprint while keeping it inside the enlarged world.
    // The extra north-south stretch creates a much longer lap without pinching
    // the existing corners against the map boundaries.
    for (auto& p : raw) {
        p.x = 5300.0 + (p.x - 5300.0) * 1.115;
        p.y = 5000.0 + (p.y - 5000.0) * 1.55;
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
    const double signedOffset = dot(world - center_[best], normals_[best]);
    const bool pits = pit_[best] && signedOffset > halfWidth + kerb &&
                      signedOffset < halfWidth + kerb + pitWidth;
    const bool grass = nearest2 > (halfWidth+kerb)*(halfWidth+kerb) && !pits;
    return {best, std::sqrt(nearest2), signedOffset, pits, grass};
}
bool Track::wheelOutside(Vec2 world, std::size_t approxIndex) const {
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

void Simulation::setMode(bool baja) {
    track_ = Track(baja);
    vehicleIndex_ = baja ? 6 : 2;
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

    car_.throttle=controls.accelerate?
        std::min(1.0,car_.throttle+p.throttleRate*dt):
        std::max(0.0,car_.throttle-5*dt);
    car_.brake=controls.brake?
        std::min(1.0,car_.brake+p.brakeRate*dt):
        std::max(0.0,car_.brake-3*dt);

    const double v=car_.speed;
    const double downforce=1+p.downforce*std::pow(std::abs(v)/p.topSpeed,2);
    const double grip=downforce*(1-0.3*weather_.rainLevel)*(1-p.grassGrip*off/4.0);
    const double frontGrip=std::max(1.0,p.frontGrip*grip);
    const double rearGrip=std::max(1.0,p.rearGrip*grip);
    const double maxSteer=p.maxSteer/(1+std::abs(v)/p.steerSensitivity);
    car_.steering+=(std::clamp(controls.steer,-1,1)*maxSteer-car_.steering)*
                    std::min(1.0,8*dt);
    double rotation=v*std::tan(car_.steering)/p.wheelbase;
    const double aLateral=std::abs(v*rotation);
    const double aBrake=v>0.5?p.brakeForce*std::pow(car_.brake,1.5):0.0;
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
        rotation*=1+2*excess;
        motor*=rearGrip/rearUse;
    }
    // Grass should make the car wash wide instead of turning sharply.
    // Scale steering response progressively with the number of wheels off-track.
    rotation *= 1.0 - 0.45 * (double(off) / 4.0);

    const double slope=(elevation(oldPosition+forward*6)-elevation(oldPosition))/6;
    double nextSpeed=v+motor*dt;
    if (v>0.5) nextSpeed-=aBrake*dt;
    else if (controls.brake) nextSpeed-=250*dt;
    nextSpeed-=nextSpeed*(controls.accelerate?.18:.5)*dt;
    nextSpeed-=nextSpeed*p.grassDrag*off*dt;
    nextSpeed-=slope*1200*dt;
    const double speedLimit=p.topSpeed*(boostActive?1.18:1.0);
    car_.speed=std::clamp(nextSpeed,-150.0,speedLimit);

    const double dAngle=rotation*dt;
    if (std::abs(car_.lateralSpeed)>.01) {
        double side=car_.lateralSpeed*std::cos(dAngle)-car_.speed*std::sin(dAngle);
        const double recovery=950;
        side=std::copysign(std::max(0.0,std::abs(side)-recovery*dt),side);
        car_.lateralSpeed=std::abs(side)>=.01?side:0.0;
    } else car_.lateralSpeed=0;
    car_.heading+=dAngle;
    const Vec2 carForward{std::cos(car_.heading),std::sin(car_.heading)};
    const Vec2 carSide{-carForward.y,carForward.x};
    car_.position=car_.position+(carForward*car_.speed+
                                  carSide*car_.lateralSpeed)*dt;
    car_.position.x=std::clamp(car_.position.x,0.0,10000.0);
    car_.position.y=std::clamp(car_.position.y,0.0,9000.0);
    const double impact=track_.collide(car_,dt);
    car_.brakeHeat=std::clamp(car_.brakeHeat+
            (car_.brake>.05 && car_.speed>0?
                 car_.brake*(std::abs(car_.speed)/p.topSpeed)*1.6*dt :
                 -0.4*dt),0.0,1.0);
    if (before.pits) car_.brakeHeat=0;

    const double pitchTarget=((car_.brake*.05-
            car_.throttle*.03*(1-std::abs(car_.speed)/p.topSpeed))*p.pitchFactor-slope);
    car_.pitch+=(pitchTarget-car_.pitch)*std::min(1.0,6*dt);
    const double rollTarget=std::clamp(p.rollFactor*car_.speed*rotation,
                                      -p.maxRoll,p.maxRoll);
    car_.roll+=(rollTarget-car_.roll)*std::min(1.0,6*dt);
    const double wingTarget=.1+.25*std::abs(car_.speed)/p.topSpeed+.9*car_.brake;
    car_.wing+=(wingTarget-car_.wing)*std::min(1.0,10*dt);
    car_.wheelRotation+=car_.speed*dt/10*.35;
    telemetry_={off,frontSlip,rearSlip,std::abs(car_.lateralSpeed)>12,
                before.pits,grip,impact,slope};
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
