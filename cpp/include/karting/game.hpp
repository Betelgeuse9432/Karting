#pragma once
#include "karting/core.hpp"
#include "raylib.h"
#include <array>
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace karting {

constexpr float kScale = static_cast<float>(kWorldScale);
Vector3 worldPoint(Vec2 p, float heightOffset = 0);
Color palette(int index);

struct TireMark { Vec2 start{}, end{}; double age = 0; };
struct Particle {
    Vec2 location{};
    double height = 0, dx = 0, dy = 0, dz = 0, life = 0, maxLife = 1;
    int kind = 0; // 0 smoke, 1 flame, 2 spark
};

class Effects {
public:
    void clear();
    void update(const Simulation& sim, double dt);
    void draw3D(const Camera3D& camera) const;
private:
    std::mt19937 generator_{42};
    std::deque<TireMark> marks_;
    std::vector<Particle> particles_;
    std::array<Vec2,4> previous_{};
    bool lastSkidding_ = false;
    double emitTime_ = 0;
};

class WorldRenderer {
public:
    void initialize(const Track& track);
    void draw(const Track& track, const Weather& weather, int quality,
              Vector3 cameraPosition, double time) const;
private:
    std::vector<Vec2> trees_;
};

class VehicleRenderer {
public:
    void draw(const Car& car, const VehicleProfile& profile,
              int vehicleIndex, int colorIndex, bool night) const;
};

class HudRenderer {
public:
    void draw(const Simulation& sim, int quality, int fps) const;
    void menu(const Simulation& sim, double time) const;
};

class Game {
public:
    int run();
private:
    void reset();
    void handleMenu();
    void handleRace();
    void follow(double dt);
    void render();
    void renderMenu(double time);
    void renderRace();
    void showFault(const std::string& subsystem, const std::string& message);
    Simulation sim_;
    FixedClock clock_;
    Effects effects_;
    WorldRenderer world_;
    VehicleRenderer vehicle_;
    HudRenderer hud_;
    Camera3D camera_{};
    bool menu_ = true;
    bool paused_ = false;
    int quality_ = 1;
    double gameTime_ = 0;
    double followAngle_ = 0;
    double cameraHeight_ = 0;
    std::string faultMessage_;
};

} // namespace karting
