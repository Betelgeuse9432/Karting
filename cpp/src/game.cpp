#include "karting/game.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace karting {
namespace {
constexpr const char* recordFile="record_karting_cpp.txt";

Color lerp(Color a,Color b,float amount) {
    amount=std::clamp(amount,0.f,1.f);
    const auto channel=[&](unsigned char x,unsigned char y) {
        return static_cast<unsigned char>(x+(y-x)*amount);
    };
    return {channel(a.r,b.r),channel(a.g,b.g),
            channel(a.b,b.b),255};
}

Controls controlsFromKeyboard() {
    return {IsKeyDown(KEY_UP)||IsKeyDown(KEY_W)||IsKeyDown(KEY_Z),
            IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S),
            int(IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D))-
            int(IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_Q)||IsKeyDown(KEY_A)),
            false};
}

// Read M as a typed character as well as a physical key.
// This also works on AZERTY keyboards, where the physical M key can have
// a different Raylib key code from KEY_M (which follows the US layout).
bool returnToMenuPressed() {
    bool pressed = IsKeyPressed(KEY_M);

    // GetCharPressed() returns typed characters, independently of layout.
    // Drain the whole queue so no old character remains for the next frame.
    for (int character = GetCharPressed(); character > 0;
         character = GetCharPressed()) {
        if (character == 'm' || character == 'M') {
            pressed = true;
        }
    }
    return pressed;
}

// Adds the horizon details that existed in the Python V6 sky:
// moving clouds and layered mountain silhouettes.
// This is purely visual: it never changes the car or track simulation.
void drawSkyDetails(int width, int height, float night, float rain,
                    double time, int quality, Color skyBottom) {
    const int horizon = int(height * 0.40f);

    // Clouds are screen-space decorations, so they stay behind the 3D world.
    if (quality > 0) {
        const int cloudCount = quality >= 3 ? 26 : quality >= 2 ? 18 : 10;
        const float cloudVisibility =
            std::clamp((1.0f - night * 0.78f) * (1.0f - rain * 0.50f),
                       0.0f, 1.0f);
        const Color cloudBase =
            lerp({238, 242, 249, 255}, {112, 119, 132, 255},
                 std::clamp(rain * 0.55f + night * 0.65f, 0.0f, 1.0f));
        const Color cloud = Fade(cloudBase, cloudVisibility * 0.72f);
        const Color cloudShadow =
            Fade(lerp(cloudBase, {75, 83, 101, 255}, 0.32f),
                 cloudVisibility * 0.34f);

        for (int i = 0; i < cloudCount; ++i) {
            const float span = float(width + 320);
            const float offset =
                std::fmod(float(i * 173) + float(time * 4.2), span);
            const float x = offset - 160.0f;
            const float y = 24.0f + float((i * 43) %
                                std::max(30, horizon - 80));
            const float size = 0.58f + float(i % 5) * 0.13f;

            DrawEllipse(int(x), int(y + 7.0f * size),
                        43.0f * size, 13.0f * size, cloudShadow);
            DrawEllipse(int(x - 25.0f * size), int(y),
                        21.0f * size, 15.0f * size, cloud);
            DrawEllipse(int(x + 2.0f * size), int(y - 8.0f * size),
                        29.0f * size, 21.0f * size, cloud);
            DrawEllipse(int(x + 29.0f * size), int(y),
                        23.0f * size, 15.0f * size, cloud);
        }
    }

    // Three mountain layers give depth. Low quality keeps only the nearest one.
    constexpr std::array<float, 3> amplitudes{{120.0f, 82.0f, 48.0f}};
    constexpr std::array<float, 3> bases{{40.0f, 18.0f, 0.0f}};
    constexpr std::array<float, 3> frequencyA{{1.3f, 2.1f, 3.0f}};
    constexpr std::array<float, 3> frequencyB{{3.1f, 4.7f, 6.1f}};
    constexpr std::array<float, 3> frequencyC{{7.0f, 9.5f, 13.0f}};
    constexpr std::array<Color, 3> mountainColors{{
        {112, 132, 172, 255},
        {80, 104, 132, 255},
        {50, 92, 70, 255}
    }};

    const int firstLayer = std::clamp(2 - quality, 0, 2);
    for (int layer = firstLayer; layer < 3; ++layer) {
        Color mountain = lerp(mountainColors[std::size_t(layer)],
                              skyBottom, 0.28f);
        mountain = lerp(mountain, {6, 8, 22, 255}, night * 0.55f);
        mountain = lerp(mountain, {96, 103, 116, 255}, rain * 0.50f);

        const auto profile = [&](float x) {
            const float angle = x / float(std::max(1, width)) * 6.2831853f
                              + float(time * 0.0015);
            const float wave =
                0.55f
                + 0.30f * std::sin(angle * frequencyA[std::size_t(layer)] + 1.3f)
                + 0.20f * std::sin(angle * frequencyB[std::size_t(layer)] + 0.4f)
                + 0.12f * std::sin(angle * frequencyC[std::size_t(layer)]);
            const float heightAboveHorizon =
                bases[std::size_t(layer)] +
                amplitudes[std::size_t(layer)] * std::clamp(wave, 0.08f, 1.0f);
            return float(horizon) - heightAboveHorizon;
        };

        for (int x = 0; x < width; x += 8) {
            const int nextX = std::min(width, x + 8);
            const float y0 = profile(float(x));
            const float y1 = profile(float(nextX));
            DrawTriangle({float(x), float(horizon)},
                         {float(nextX), float(horizon)},
                         {float(nextX), y1}, mountain);
            DrawTriangle({float(x), float(horizon)},
                         {float(nextX), y1},
                         {float(x), y0}, mountain);
        }
    }
}
} // namespace

void Game::reset() {
    sim_.reset();
    clock_.reset();
    effects_.clear();
    paused_=false;
    boostQueued_=false;
    disabled_.clear();
    faultMessage_.clear();
    followAngle_=sim_.car().heading;
    cameraHeight_=elevation(sim_.car().position)+sim_.profile().cameraHeight;
}

void Game::handleMenu() {
    const bool left=IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_Q)||IsKeyPressed(KEY_A);
    const bool right=IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D);
    const bool confirm=IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE);
    const bool back=IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_BACKSPACE);

    if (menuScreen_==MenuScreen::Mode) {
        if (confirm) {
            sim_.setMode(0);
            world_.initialize(sim_.track());
            menuScreen_=MenuScreen::Vehicle;
        }
        return;
    }

    if (back) {
        menuScreen_=MenuScreen::Mode;
        return;
    }
    constexpr std::array<int,6> vehicleSlots{{0,2,3,5,6,10}};
    const auto current=std::find(vehicleSlots.begin(),vehicleSlots.end(),sim_.vehicleIndex());
    const int selected=current==vehicleSlots.end()?1:int(current-vehicleSlots.begin());
    if (left) sim_.selectVehicle(vehicleSlots[std::size_t((selected+5)%6)]);
    else if (right) sim_.selectVehicle(vehicleSlots[std::size_t((selected+1)%6)]);
    if (IsKeyPressed(KEY_UP))
        sim_.selectVehicle(sim_.vehicleIndex(),(sim_.colorIndex()+5)%6);
    else if (IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_C))
        sim_.selectVehicle(sim_.vehicleIndex(),(sim_.colorIndex()+1)%6);

    if (confirm) {
        menu_=false;
        reset();
    }
}

void Game::handleRace() {
    if (IsKeyPressed(KEY_R)) reset();
    if (returnToMenuPressed()) {
        menu_=true;
        menuScreen_=MenuScreen::Mode;
        reset();
    }
    if (IsKeyPressed(KEY_G)) quality_=(quality_+1)%4;
    if (IsKeyPressed(KEY_N)) sim_.cycleWeather();
    if (IsKeyPressed(KEY_P)) sim_.toggleRain();
}

void Game::follow(double dt) {
    const auto& car=sim_.car();
    const auto& spec=sim_.profile();
    followAngle_+=wrapAngle(car.heading-followAngle_)*std::min(1.0,5.0*dt);
    cameraHeight_+=((sim_.isBaja()?bajaElevation(car.position):sim_.isDesert()?0.0:elevation(car.position))+car.jumpHeight+spec.cameraHeight-cameraHeight_)*
                    std::min(1.0,5.5*dt);
    const Vec2 behind{std::cos(followAngle_),std::sin(followAngle_)};
    const Vec2 view=car.position-behind*spec.cameraDistance;
    camera_.position={float(view.x*kWorldScale),
                      float(cameraHeight_*kWorldScale),
                      float(view.y*kWorldScale)};
    camera_.target={float(car.position.x*kWorldScale),
                    float(((sim_.isBaja()?bajaElevation(car.position):elevation(car.position))+car.jumpHeight+30)*kWorldScale),
                    float(car.position.y*kWorldScale)};
    camera_.up={0,1,0};
    camera_.projection=CAMERA_PERSPECTIVE;
    const float speedRatio=float(std::abs(car.speed)/spec.topSpeed);
    camera_.fovy=64.f+speedRatio*9.f;
}

void Game::showFault(const std::string& subsystem,const std::string& message) {
    faultMessage_=subsystem+": "+message;
    std::cerr<<"[Karting C++] "<<faultMessage_<<"\n";
}

void Game::renderMenu(double time) {
    const int width=GetScreenWidth(),height=GetScreenHeight();
    const Color top{20,22,35,255},bottom{49,54,65,255};
    DrawRectangleGradientV(0,0,width,height,top,bottom);

    if (menuScreen_==MenuScreen::Mode) {
        hud_.modeMenu(selectedMode_,time);
        return;
    }
    const float t=float(time*.35);
    const float base=float(elevation({0,0})*kWorldScale);
    Camera3D preview{};
    preview.position={4.6f*std::cos(t),base+2.35f,
                      4.6f*std::sin(t)};
    preview.target={0,base+.55f,0};
    preview.up={0,1,0};
    preview.fovy=52;
    preview.projection=CAMERA_PERSPECTIVE;
    BeginMode3D(preview);
    DrawPlane({0,base-.04f,0},{10,10},{82,88,103,255});
    DrawCylinder({0,base,0},1.65f,1.65f,.13f,32,
                 Color{112,115,128,255});
    Car display{};
    display.position={0,0};
    display.heading=time*.7;
    display.wheelRotation=time*5;
    display.wing=.1+.45*(.5+.5*std::sin(time*2));
    vehicle_.draw(display,sim_.profile(),sim_.vehicleIndex(),
                  sim_.colorIndex(),false,sim_.isDesert());
    EndMode3D();
    hud_.menu(sim_,time);
}

void Game::renderRace() {
    const int width=GetScreenWidth(),height=GetScreenHeight();
    const auto& weather=sim_.weather();
    const float n=float(weather.nightLevel),r=float(weather.rainLevel);
    const Color daylightTop{92,157,243,255};
    const Color daylightBottom{190,220,250,255};
    const Color duskTop{85,65,129,255};
    const Color duskBottom{242,148,92,255};
    const Color nightTop{5,8,27,255};
    const Color nightBottom{22,30,65,255};
    Color top=n<.55f?lerp(daylightTop,duskTop,n/.55f):
                       lerp(duskTop,nightTop,(n-.55f)/.45f);
    Color bottom=n<.55f?lerp(daylightBottom,duskBottom,n/.55f):
                          lerp(duskBottom,nightBottom,(n-.55f)/.45f);
    top=lerp(top,{76,85,100,255},r*.6f);
    bottom=lerp(bottom,{108,115,130,255},r*.6f);
    DrawRectangleGradientV(0,0,width,height,top,bottom);
    if (n>.5f && r<.6f) {
        const float starVisibility=(n-.5f)*2*(1-r);
        for (int i=0;i<90;++i) {
            const int x=(i*617+23)%width;
            const int y=(i*277+17)%(height/2);
            const float twinkle=.72f+.28f*std::sin(float(i)*1.7f+float(gameTime_*2));
            DrawCircle(x,y,1,Fade(WHITE,starVisibility*twinkle));
        }
        const int moonX=width/5,moonY=100;
        const float moonVisibility=std::clamp((n-.58f)/.42f,0.f,1.f)*(1-r);
        DrawCircleGradient(moonX,moonY,54.f,
                           Fade({160,180,225,255},moonVisibility*.22f),
                           Fade({160,180,225,255},0));
        DrawCircle(moonX,moonY,17,
                   Fade({232,237,248,255},moonVisibility));
        DrawCircle(moonX-5,moonY-5,4,
                   Fade({255,255,255,255},moonVisibility*.55f));
    }

    // Each subsystem can fail independently without taking down the process.
    const auto safe=[&](const char* name,auto&& operation) {
        if (disabled_.contains(name)) return;
        try { operation(); }
        catch (const std::exception& e) {
            disabled_.insert(name);
            showFault(name,e.what());
        }
    };
    BeginMode3D(camera_);
    // Distant 3D sun: it travels through the world sky instead of being
    // painted at a fixed screen coordinate over the camera.
    const float sunPhase=float(gameTime_*.006+.9);
    const Vector3 sunDirection{std::cos(sunPhase)*.82f,
                               std::sin(sunPhase)*.82f,-.42f};
    const Vector3 sunPosition{
        camera_.position.x+sunDirection.x*3000.f*kScale,
        camera_.position.y+sunDirection.y*3000.f*kScale,
        camera_.position.z+sunDirection.z*3000.f*kScale
    };
    const float sunVisibility=std::clamp((1.f-n*.92f)*(1.f-r*.85f),0.f,1.f);
    DrawSphere(sunPosition,190.f*kScale,
               Fade({255,218,145,255},sunVisibility));
    DrawSphere(sunPosition,285.f*kScale,
               Fade({255,191,110,255},sunVisibility*.12f));
    safe("world",[&]{
        world_.draw(sim_.track(),weather,quality_,camera_.position,gameTime_);
    });
    safe("effects",[&]{ effects_.draw3D(camera_); });
    safe("vehicle",[&]{
        if(sim_.isBaja()) {
            const int rivalIndex=sim_.opponentVehicleIndex();
            vehicle_.draw(sim_.opponent(),profiles()[rivalIndex],
                          rivalIndex,sim_.opponentColorIndex(),n>.2,sim_.isDesert());
        }
        vehicle_.draw(sim_.car(),sim_.profile(),
                      sim_.vehicleIndex(),sim_.colorIndex(),n>.2,sim_.isDesert());
    });
    EndMode3D();

    // Rain is drawn in a separate transparent screen pass.
    if (r>.03f) {
        const int drops=int((quality_==0?80:quality_==1?140:quality_==2?220:320)*r);
        for (int i=0;i<drops;++i) {
            const int x=(i*173+int(gameTime_*160))%(width+65)-32;
            const int y=(i*277+int(gameTime_*900))%(height+25);
            DrawLine(x,y,x+3,y+15,Fade(SKYBLUE,r*.7f));
        }
        if (quality_>0)
            DrawRectangle(0,0,width,height,Fade({90,110,135,255},r*.08f));
    }
    safe("hud",[&]{ hud_.draw(sim_,quality_,GetFPS()); });
    if (paused_ || !faultMessage_.empty()) {
        const auto ui=uiViewport();
        BeginMode2D(ui.camera());
        if (paused_) {
            DrawRectangle(0,ui.height/2-38,ui.width,76,Fade(BLACK,.78f));
            DrawText("PHYSIQUE EN PAUSE  -  R pour recommencer",
                     ui.width/2-328,ui.height/2-13,24,RED);
        }
        if (!faultMessage_.empty())
            DrawText(TextFormat("Module desactive : %.100s",faultMessage_.c_str()),
                     18,ui.height-199,15,RED);
        EndMode2D();
    }
}

void Game::render() {
    BeginDrawing();
    ClearBackground(BLACK);
    if (menu_) renderMenu(gameTime_);
    else renderRace();
    EndDrawing();
}

int Game::run() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280,720,"Karting V6 - C++20 / raylib");
    SetWindowMinSize(960,600);
    MaximizeWindow();
    SetTargetFPS(120);
    try {
        world_.initialize(sim_.track());
        reset();
        std::ifstream input(recordFile);
        double record=0;
        if (input>>record && record>0 && std::isfinite(record))
            sim_.setBestLap(record);

        while (!WindowShouldClose()) {
            const double frame=std::clamp(double(GetFrameTime()),0.0,.15);
            gameTime_+=frame;
            if (menu_) handleMenu();
            else handleRace();

            if (!menu_ && !paused_) {
                if (IsKeyPressed(KEY_SPACE)) boostQueued_=true;
                Controls keys=controlsFromKeyboard();
                keys.boostPressed=boostQueued_;
                for (int step=clock_.consume(frame);step>0;--step) {
                    const double previousRecord=sim_.race().bestLap;
                    try {
                        sim_.update(keys,FixedClock::step);
                        keys.boostPressed=false;
                        boostQueued_=false;
                    } catch(const std::exception& e) {
                        showFault("physics",e.what());
                        paused_=true;
                        break;
                    }
                    if (sim_.race().bestLap>0 &&
                        (previousRecord<0||sim_.race().bestLap<previousRecord)) {
                        try {
                            std::ofstream out(recordFile,std::ios::trunc);
                            if (!out) throw std::runtime_error("Cannot open record");
                            out<<sim_.race().bestLap<<'\n';
                        } catch (const std::exception& e) {
                            showFault("record",e.what());
                        }
                    }
                    if (!disabled_.contains("effects.sim")) {
                        try { effects_.update(sim_,FixedClock::step); }
                        catch(const std::exception& e) {
                            disabled_.insert("effects.sim");
                            showFault("effects.sim",e.what());
                        }
                    }
                }
                follow(frame);
            }
            render();
        }
    } catch (...) {
        world_.shutdown();
        CloseWindow();
        throw;
    }
    world_.shutdown();
    CloseWindow();
    return 0;
}

} // namespace karting
