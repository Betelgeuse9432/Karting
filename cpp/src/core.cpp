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
    // Broad natural relief under the entire circuit. Longer wavelengths
    // avoid abrupt crests while increasing the elevation variation.
    // Keeping the pavement on this surface prevents floating road edges;
    // independent deck profiles remain available for future bridges.
    return 510 * (0.56 * std::sin(p.x / 2600) +
                  0.34 * std::sin(p.y / 2100 + 1) +
                  0.23 * std::sin((p.x + p.y) / 3200 + 2));
}

const std::array<VehicleProfile, 4>& profiles() {
    static const std::array<VehicleProfile, 4> cars{{
        {"KART", 300, 320, 0.65, 2.0, 800, 950, 950, 0.0, 30, 0.7, 200,
         0.00005, 0.03, 0.4, 0.2, 0.5, 48, 125, 17, -15, 18, 0},
        {"4x4", 320, 280, 0.55, 1.3, 560, 600, 650, 0.05, 52, 0.5, 150,
         0.0003, 0.12, 1.8, 0.04, 0.1, 85, 175, 30, -30, 26, 5},
        {"GT3", 400, 390, 0.65, 1.6, 760, 740, 780, 0.55, 46, 0.55, 150,
         0.00012, 0.07, 1.0, 0.18, 0.45, 65, 150, 22, -22, 21, 3},
        {"WEC", 450, 460, 0.60, 1.5, 840, 760, 800, 1.3, 50, 0.5, 140,
         0.00008, 0.05, 0.6, 0.3, 0.6, 58, 160, 30, -30, 22, 1}
    }};
    return cars;
}

Track::Track() {
    // A north-turning eastern hairpin exits west, then sweeps back south
    // across the approach straight on a raised bridge. Rest unchanged.
    // The route crosses itself in XY but is separated by bridge altitude.
    const std::vector<Vec2> raw{
        // Sector 1: original approach, opposite-turning northern hairpin.
        {8500,2400}, {11200,2380}, {14000,2400}, {17000,2400},
        {18900,2400}, {19700,2320}, {20250,2000}, {20450,1450},
        {20250,900}, {19700,620}, {18900,600}, {17400,600},
        // Sector 2: curved raised approach crossing sector 1 near x=14700.
        {15900,900}, {15100,1550}, {14700,2350}, {14650,3500},
        {14850,5000}, {14650,6150}, {15400,7400},
        {17200,8100}, {18800,7600}, {20300,8100}, {21100,9700},
        {20900,11500},
        // Sector 3: double apex, short acceleration and southern sweep.
        {19600,12600}, {18400,11900}, {17100,12450}, {16400,13900},
        {17800,15400}, {17200,16700}, {14800,17100},
        {12900,16200}, {11100,14600}, {9600,15300},
        // Sector 4: western flowing bends, slower technical S, return.
        {8000,16600}, {6100,16100}, {4700,14900}, {3900,13200},
        {4300,11300}, {5500,10100}, {5000,8650}, {3900,7900},
        {3850,6650}, {5100,4650}, {7200,3100}
    };
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
    bridgeRise_.reserve(n);
    distances_.reserve(n);
    pit_.assign(n, false);
    for (std::size_t i = 0; i < n; ++i) {
        const Vec2 dir = center_[(i+1)%n] - center_[(i+n-1)%n];
        const double inverseLength = 1.0 / std::max(length(dir), 1e-8);
        normals_.push_back({-dir.y * inverseLength, dir.x * inverseLength});
        const double ground=elevation(center_[i]);
        // Long, smooth approach and exit ramps. The road crosses the
        // approach straight at the crest, ~8.5 m above the lower road.
        // Smoothstep avoids sudden grade changes at either ramp end.
        const double control=double(i)/subdivisions;
        const auto smooth=[](double t) {
            t=std::clamp(t,0.0,1.0);
            return t*t*(3.0-2.0*t);
        };
        const double rise=470.0*smooth((control-9.0)/5.0)*
                                (1.0-smooth((control-14.5)/7.0));
        heights_.push_back(ground+rise);
        bridgeRise_.push_back(rise);
        if (i) length_ += length(center_[i] - center_[i-1]);
        distances_.push_back(length_);
    }
    length_ += length(center_.back() - center_.front());

    // Pit-lane near the finish line, traversing the wraparound.
    const double stepLength = length_ / n;
    const int pitCount = std::max(20, int(1900/stepLength));
    const int pitStart = int(n) - std::max(3, int(500/stepLength));
    for (int k = 0; k < pitCount; ++k)
        pit_[std::size_t((pitStart+k) % int(n))] = true;
}

std::size_t Track::indexAtDistance(double distance) const {
    if (!std::isfinite(distance))
        throw std::invalid_argument("Non-finite track distance");
    double wrapped=std::fmod(distance,length_);
    if (wrapped<0) wrapped+=length_;
    // Cumulative distances are measured from the start line; the last
    // wraparound segment is represented by the final length_ entry.
    const auto it=std::lower_bound(distances_.begin(),distances_.end(),wrapped);
    if(it==distances_.end()) return center_.size()-1;
    const auto i=std::size_t(it-distances_.begin());
    if(i==0) return 0;
    return wrapped-distances_[i-1]<distances_[i]-wrapped ? i-1 : i;
}

Vec2 Track::point(std::size_t index, double offset) const {
    index %= center_.size();
    return center_[index] + normals_[index] * offset;
}
// Terrain and deck are separate concepts. For today's track the cached
// deck height is the old terrain elevation, preserving the exact geometry.
double Track::terrainHeight(Vec2 position) const {
    return elevation(position);
}
double Track::roadHeight(std::size_t index) const {
    return heights_.at(index % heights_.size());
}
double Track::bridgeRise(std::size_t index) const {
    return bridgeRise_.at(index % bridgeRise_.size());
}
double Track::roadHeightAt(Vec2 position,std::size_t nearIndex) const {
    // Project onto the adjacent centreline segments, then interpolate
    // their deck elevations. This stays on the correct layer at a crossing.
    const std::size_t n=center_.size(), index=nearIndex%n;
    double best=std::numeric_limits<double>::infinity();
    double result=heights_[index];
    for(const std::size_t first:{(index+n-1)%n,index}) {
        const std::size_t second=(first+1)%n;
        const Vec2 leg=center_[second]-center_[first];
        const double t=std::clamp(dot(position-center_[first],leg)/
                                   std::max(1e-9,dot(leg,leg)),0.0,1.0);
        const Vec2 projection=center_[first]+leg*t;
        const double d2=dot(position-projection,position-projection);
        if(d2<best) {
            best=d2;
            result=heights_[first]+(heights_[second]-heights_[first])*t;
        }
    }
    return result;
}
void Track::setRoadHeightProfile(std::vector<double> heights) {
    if(heights.size()!=heights_.size() ||
       !std::all_of(heights.begin(),heights.end(),
                    [](double h){return std::isfinite(h);}))
        throw std::invalid_argument("Invalid road height profile");
    heights_=std::move(heights);
    for(std::size_t i=0;i<heights_.size();++i)
        bridgeRise_[i]=heights_[i]-elevation(center_[i]);
}

// A future bridge can share X/Y with the road beneath it. Compare track
// candidates in 3D rather than picking the first nearby 2D sample.
// The continuity hint only breaks near-equal distances: it cannot pull a
// vehicle to a remote road. Nothing calls this method during normal driving
// until multi-level collision support is deliberately enabled.
TrackInfo Track::infoAtHeight(Vec2 world, double height,
                              std::optional<std::size_t> preferredIndex) const {
    if (!std::isfinite(world.x) || !std::isfinite(world.y) ||
        !std::isfinite(height))
        throw std::invalid_argument("Non-finite 3D track coordinate");
    const std::size_t n=center_.size();
    const std::size_t preferred=preferredIndex ? *preferredIndex%n : 0;
    std::size_t best=0;
    double nearest3D=std::numeric_limits<double>::infinity();
    std::size_t bestArcDistance=n;
    for(std::size_t i=0;i<n;++i) {
        const Vec2 delta=center_[i]-world;
        const double dz=heights_[i]-height;
        const double d2=dot(delta,delta)+dz*dz;
        const std::size_t diff=i>preferred?i-preferred:preferred-i;
        const std::size_t arcDistance=std::min(diff,n-diff);
        // Prefer the previous route on *exactly equal* 3D distances.
        const double tolerance=1e-10*std::max(1.0,nearest3D);
        if (i==0 || d2<nearest3D-tolerance ||
            (preferredIndex && std::abs(d2-nearest3D)<=tolerance &&
             arcDistance<bestArcDistance)) {
            nearest3D=d2;
            best=i;
            bestArcDistance=arcDistance;
        }
    }
    const Vec2 delta=world-center_[best];
    const double horizontalDistance=length(delta);
    const double signedOffset=dot(delta,normals_[best]);
    const bool pits=pit_[best] && signedOffset>halfWidth+kerb &&
                    signedOffset<halfWidth+kerb+pitWidth;
    const bool grass=horizontalDistance>halfWidth+kerb && !pits;
    return {best,horizontalDistance,signedOffset,pits,grass};
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
    const auto nearest = infoAtHeight(car.position,car.height,car.roadIndex);
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
    const auto spot=track.infoAtHeight(car.position,car.height,car.roadIndex);
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
    roadIndex_=0;
    car_.roadIndex=0;
    car_.height=track_.roadHeight(0);
    car_.heading=track_.startHeading();
    race_.reset(preserveRecord);
    telemetry_=Telemetry{};
}

void Simulation::setBestLap(double seconds) {
    if (seconds>0 && std::isfinite(seconds)) race_.bestLap=seconds;
}

void Simulation::update(const Controls& controls,double dt) {
    if (!(dt>0 && dt<=.05 && std::isfinite(dt)))
        throw std::invalid_argument("Invalid simulation timestep");
    const auto& p=profile();
    weather_.update(dt);
    const auto oldPosition=car_.position;
    const auto before=track_.infoAtHeight(oldPosition,car_.height,roadIndex_);
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
    const double rearGrip=std::max(1.0,p.rearGrip*grip*(controls.handbrake?.35:1.0));
    const double maxSteer=p.maxSteer/(1+std::abs(v)/p.steerSensitivity);
    car_.steering+=(std::clamp(controls.steer,-1,1)*maxSteer-car_.steering)*
                    std::min(1.0,8*dt);
    double rotation=v*std::tan(car_.steering)/p.wheelbase;
    const double aLateral=std::abs(v*rotation);
    const double aBrake=v>0.5?p.brakeForce*std::pow(car_.brake,1.5):0.0;
    const double frontUse=std::hypot(aBrake,aLateral);
    const bool frontSlip=frontUse>frontGrip;
    if (frontSlip) rotation*=frontGrip/frontUse;

    double motor=p.power*car_.throttle*(1-std::max(v,0.0)/p.topSpeed);
    const double rearUse=std::hypot(motor,aLateral);
    const bool rearSlip=rearUse>rearGrip;
    if (rearSlip) {
        const double excess=rearUse/rearGrip-1;
        rotation*=1+(controls.handbrake?std::min(2*excess,1.5):2*excess);
        motor*=rearGrip/rearUse;
    }

    const double slope=(track_.roadHeightAt(oldPosition+forward*6,before.index)-
                         track_.roadHeightAt(oldPosition-forward*6,before.index))/12;
    double nextSpeed=v+motor*dt;
    if (v>0.5) nextSpeed-=aBrake*dt;
    else if (controls.brake) nextSpeed-=250*dt;
    nextSpeed-=nextSpeed*(controls.accelerate?.18:.5)*dt;
    if (controls.handbrake)
        nextSpeed-=std::copysign(std::min(std::abs(nextSpeed),370*dt),nextSpeed);
    nextSpeed-=nextSpeed*p.grassDrag*off*dt;
    nextSpeed-=slope*1200*dt;
    car_.speed=std::clamp(nextSpeed,-150.0,p.topSpeed);

    const double dAngle=rotation*dt;
    if (controls.handbrake || std::abs(car_.lateralSpeed)>.01) {
        double side=car_.lateralSpeed*std::cos(dAngle)-car_.speed*std::sin(dAngle);
        const double recovery=950*(controls.handbrake?.055:1.0);
        side=std::copysign(std::max(0.0,std::abs(side)-recovery*dt),side);
        car_.lateralSpeed=std::abs(side)>=.01?side:0.0;
    } else car_.lateralSpeed=0;
    car_.heading+=dAngle;
    const Vec2 carForward{std::cos(car_.heading),std::sin(car_.heading)};
    const Vec2 carSide{-carForward.y,carForward.x};
    // Convert the existing arcade speed to actual distance in the rendered
    // world. Previously the HUD reported ~300 km/h while movement was
    // ~65 km/h in rendered metres. Keep yaw dynamics unchanged so the
    // steering doesn't become 4.6x more twitchy; fast corners now require
    // the expected larger turning radius.
    car_.position=car_.position+(carForward*car_.speed+
                                  carSide*car_.lateralSpeed)*
                                 (dt*kTravelScale);
    car_.position.x=std::clamp(car_.position.x,0.0,Track::worldWidth);
    car_.position.y=std::clamp(car_.position.y,0.0,Track::worldHeight);
    // Keep the car on its current vertical route through the crossover.
    // The ordinary 2D nearest-line lookup would jump to the lower deck.
    const auto next=track_.infoAtHeight(car_.position,car_.height,roadIndex_);
    roadIndex_=next.index;
    car_.roadIndex=roadIndex_;
    car_.height=track_.roadHeightAt(car_.position,roadIndex_);
    const double impact=track_.collide(car_,dt);
    car_.brakeHeat=std::clamp(car_.brakeHeat+
            (car_.brake>.05 && car_.speed>0?
                 car_.brake*(std::abs(car_.speed)/p.topSpeed)*1.6*dt :
                 -0.4*dt),0.0,1.0);
    if (before.pits) car_.brakeHeat=0;

    // Suspension pitch is independent from the static road incline.
    // The vehicle renderer now aligns all four wheels and the body to
    // the surface normal; subtracting slope here would cancel that tilt.
    const double pitchTarget=(car_.brake*.05-
            car_.throttle*.03*(1-std::abs(car_.speed)/p.topSpeed))*p.pitchFactor;
    car_.pitch+=(pitchTarget-car_.pitch)*std::min(1.0,6*dt);
    const double rollTarget=std::clamp(p.rollFactor*car_.speed*rotation,
                                      -p.maxRoll,p.maxRoll);
    car_.roll+=(rollTarget-car_.roll)*std::min(1.0,6*dt);
    const double wingTarget=.1+.25*std::abs(car_.speed)/p.topSpeed+.9*car_.brake;
    car_.wing+=(wingTarget-car_.wing)*std::min(1.0,10*dt);
    // Rotate wheel rims by travelled metres, not the old arbitrary speed.
    car_.wheelRotation+=(car_.speed*kWorldScale*kTravelScale*dt)/.20;
    telemetry_={off,frontSlip,rearSlip,std::abs(car_.lateralSpeed)>12,
                before.pits,grip,impact,slope};
    race_.tick(track_,car_,off,dt);

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
