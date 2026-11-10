#pragma once
#include "karting/core.hpp"
#include "raylib.h"
#include <array>
#include <deque>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace karting {

constexpr float kScale = static_cast<float>(kWorldScale);
Vector3 worldPoint(Vec2 p, float heightOffset = 0, bool baja = false, bool desert = false);
Color palette(int index);

struct Particle {
    Vec2 location{};
    double height = 0, dx = 0, dy = 0, dz = 0, life = 0, maxLife = 1;
    int kind = 0; // 0 smoke, 1 flame, 2 spark
    int exhaustSide = 0; // -1/+1 keeps flame particles attached to an exhaust
};

class Effects {
public:
    void clear();
    void update(const Simulation& sim, double dt);
    void draw3D(const Camera3D& camera) const;
private:
    std::mt19937 generator_{42};
    std::vector<Particle> particles_;
    bool previousThrottle_ = false;
    double emitTime_ = 0;
};

class WorldRenderer {
public:
    void initialize(const Track& track);
    void shutdown();
    void draw(const Track& track, const Weather& weather, int quality,
              Vector3 cameraPosition, double time) const;
private:
    Texture2D asphaltTexture_{};
    Texture2D grassTexture_{};
    std::vector<Vec2> trees_;
};

class VehicleRenderer {
public:
    void draw(const Car& car, const VehicleProfile& profile,
              int vehicleIndex, int colorIndex, bool night, bool desert = false) const;
};

// Logical HUD dimensions preserve the original 1100x720 layout at any
// desktop resolution, while 3D renders directly to the native window.
struct UiViewport {
    float scale=1.f;
    int width=1100;
    int height=720;
    Camera2D camera() const {
        Camera2D value{};
        value.zoom=scale;
        return value;
    }
};
UiViewport uiViewport();

class HudRenderer {
public:
    void draw(const Simulation& sim, int quality, int fps) const;
    void modeMenu(int selectedMode, double time) const;
    void menu(const Simulation& sim, double time) const;
};

enum class MenuScreen { Mode, Vehicle };

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
    MenuScreen menuScreen_ = MenuScreen::Mode;
    int selectedMode_ = 0; // Only the GT3 circuit remains
    bool paused_ = false;\n    bool cockpitView_ = false;
    bool boostQueued_ = false;
    int quality_ = 2;
    double gameTime_ = 0;
    double followAngle_ = 0;
    double cameraHeight_ = 0;
    std::string faultMessage_;
    std::unordered_set<std::string> disabled_;
};

} // namespace karting
