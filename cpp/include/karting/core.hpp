#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace karting {

constexpr double kPi = 3.14159265358979323846;
constexpr double kWorldScale = 0.018; // rendering metres per physics world unit

// Preserve the existing arcade speed numbers and the HUD's 0.3 km/h unit.
// Translate the car through the world at the matching physical metres/second.
// Example: speed=1000 => 300 km/h => 83.33 renderer metres per second.
constexpr double kKmHPerSpeedUnit = 0.3;
constexpr double kTravelScale = kKmHPerSpeedUnit / (3.6 * kWorldScale);
constexpr double toKmH(double simulationSpeed) {
    return simulationSpeed * kKmHPerSpeedUnit;
}

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};
Vec2 operator+(Vec2 a, Vec2 b);
Vec2 operator-(Vec2 a, Vec2 b);
Vec2 operator*(Vec2 a, double s);
double dot(Vec2 a, Vec2 b);
double length(Vec2 a);
double elevation(Vec2 p);
double wrapAngle(double angle);

struct VehicleProfile {
    std::string_view name;
    double topSpeed, power, throttleRate, brakeRate, brakeForce;
    double frontGrip, rearGrip, downforce, wheelbase, maxSteer, steerSensitivity;
    double rollFactor, maxRoll, pitchFactor, grassDrag, grassGrip;
    double cameraHeight, cameraDistance;
    double frontAxle, rearAxle, halfWheelTrack;
    int defaultColor;
};
const std::array<VehicleProfile, 4>& profiles();

struct Controls {
    bool accelerate = false;
    bool brake = false;
    int steer = 0;
    bool handbrake = false;
};

struct Car {
    Vec2 position{};
    double height = 0; // World-unit altitude of the road under the car.
    std::size_t roadIndex = 0; // Continuous route index through stacked roads.
    double heading = 0;
    double speed = 0;
    double lateralSpeed = 0;
    double steering = 0;
    double throttle = 0;
    double brake = 0;
    double brakeHeat = 0;
    double roll = 0;
    double pitch = 0;
    double wing = 0.1;
    double wheelRotation = 0;
};

struct TrackInfo {
    std::size_t index = 0;
    double distance = 0;
    double signedOffset = 0;
    bool pits = false;
    bool grass = false;
};

class Track {
public:
    static constexpr double halfWidth = 175;
    static constexpr double kerb = 22;
    static constexpr double wall = 275;
    static constexpr double pitWidth = 150;
    static constexpr int subdivisions = 24;
    static constexpr double worldWidth = 24000;
    static constexpr double worldHeight = 20000;

    Track();
    const std::vector<Vec2>& center() const { return center_; }
    const std::vector<Vec2>& normals() const { return normals_; }
    const std::vector<double>& heights() const { return heights_; }
    std::size_t size() const { return center_.size(); }
    double lapLength() const { return length_; }
    Vec2 point(std::size_t index, double offset = 0) const;
    // Terrain height is a property of (x,y). Road deck height belongs to
    // a particular track sample, so overlapping roads may have different Z.
    double terrainHeight(Vec2 position) const;
    double roadHeight(std::size_t index) const;
    double roadHeightAt(Vec2 position, std::size_t nearIndex) const;
    double bridgeRise(std::size_t index) const;
    // Replace only the deck altitude profile, not the XY spline or terrain.
    // Used later for ramps, bridges and underpasses; default equals terrain.
    void setRoadHeightProfile(std::vector<double> heights);
    // Height-aware lookup for future stacked roads. 'preferredIndex' is a
    // continuity hint; the existing 2D info() stays unchanged for now.
    TrackInfo infoAtHeight(Vec2 world, double roadHeight,
                           std::optional<std::size_t> preferredIndex =
                               std::nullopt) const;
    std::size_t indexAtDistance(double distance) const;
    Vec2 startPosition() const;
    double startHeading() const;
    bool isPitIndex(std::size_t index) const;
    TrackInfo info(Vec2 world) const;
    bool wheelOutside(Vec2 world, std::size_t approxIndex) const;
    double collide(Car& car, double dt) const;

private:
    std::vector<Vec2> center_;
    std::vector<Vec2> normals_;
    std::vector<double> heights_;
    std::vector<double> bridgeRise_;
    std::vector<double> distances_;
    std::vector<bool> pit_;
    double length_ = 0;
};

struct Weather {
    int phase = 0; // 0 jour, 1 crépuscule, 2 nuit
    bool raining = false;
    double nightLevel = 0;
    double rainLevel = 0;
    void update(double dt);
};

struct Telemetry {
    int wheelsOutside = 0;
    bool frontSlip = false;
    bool rearSlip = false;
    bool drifting = false;
    bool inPits = false;
    double grip = 1;
    double impact = 0;
    double slope = 0;
};

struct Race {
    int laps = 0;
    int nextSector = 1;
    bool outsideLastFrame = false;
    double elapsed = 0;
    double lastLap = -1;
    double bestLap = -1;
    double topSpeed = 0;
    double warningTime = 0;
    std::vector<double> recent;
    void reset(bool keepBest = true);
    std::optional<double> tick(const Track& track, const Car& car, int outsideWheels, double dt);
};

class Simulation {
public:
    Simulation();
    const Track& track() const { return track_; }
    const Car& car() const { return car_; }
    const Weather& weather() const { return weather_; }
    const Race& race() const { return race_; }
    const Telemetry& telemetry() const { return telemetry_; }
    const VehicleProfile& profile() const { return profiles().at(vehicleIndex_); }
    int vehicleIndex() const { return vehicleIndex_; }
    int colorIndex() const { return colorIndex_; }
    void selectVehicle(int index, int color = -1);
    void reset(bool preserveRecord = true);
    void setBestLap(double seconds);
    void update(const Controls& controls, double dt);
    void cycleWeather() { weather_.phase = (weather_.phase + 1) % 3; }
    void toggleRain() { weather_.raining = !weather_.raining; }

private:
    Track track_;
    Car car_{};
    Weather weather_{};
    Race race_{};
    Telemetry telemetry_{};
    int vehicleIndex_ = 2;
    int colorIndex_ = 3;
    std::size_t roadIndex_ = 0;
};

class FixedClock {
public:
    static constexpr double step = 1.0 / 60.0;
    int consume(double frameSeconds);
    void reset() { accumulated_ = 0; }
private:
    double accumulated_ = 0;
};

} // namespace karting
