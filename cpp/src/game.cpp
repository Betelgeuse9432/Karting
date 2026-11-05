#include "karting/game.hpp"
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
            IsKeyDown(KEY_SPACE)};
}
} // namespace

void Game::reset() {
    sim_.reset();
    clock_.reset();
    effects_.clear();
    paused_=false;
    disabled_.clear();
    faultMessage_.clear();
    followAngle_=sim_.car().heading;
    cameraHeight_=elevation(sim_.car().position)+sim_.profile().cameraHeight;
}

void Game::handleMenu() {
    if (IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_Q)||IsKeyPressed(KEY_A))
        sim_.selectVehicle((sim_.vehicleIndex()+3)%4);
    else if (IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D))
        sim_.selectVehicle((sim_.vehicleIndex()+1)%4);
    if (IsKeyPressed(KEY_UP))
        sim_.selectVehicle(sim_.vehicleIndex(),(sim_.colorIndex()+5)%6);
    else if (IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_C))
        sim_.selectVehicle(sim_.vehicleIndex(),(sim_.colorIndex()+1)%6);

    if (IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE)) {
        menu_=false;
        reset();
    }
}

void Game::handleRace() {
    if (IsKeyPressed(KEY_M)) {
        menu_=true;
        reset();
        return;
    }
    if (IsKeyPressed(KEY_R)) reset();
    if (IsKeyPressed(KEY_G)) quality_=(quality_+1)%3;
    if (IsKeyPressed(KEY_N)) sim_.cycleWeather();
    if (IsKeyPressed(KEY_P)) sim_.toggleRain();
}

void Game::follow(double dt) {
    const auto& car=sim_.car();
    const auto& spec=sim_.profile();
    followAngle_+=wrapAngle(car.heading-followAngle_)*std::min(1.0,3.5*dt);
    cameraHeight_+=(elevation(car.position)+spec.cameraHeight-cameraHeight_)*
                    std::min(1.0,4.0*dt);
    const Vec2 behind{std::cos(followAngle_),std::sin(followAngle_)};
    const Vec2 view=car.position-behind*spec.cameraDistance;
    camera_.position={float(view.x*kWorldScale),
                      float(cameraHeight_*kWorldScale),
                      float(view.y*kWorldScale)};
    camera_.target={float(car.position.x*kWorldScale),
                    float((elevation(car.position)+30)*kWorldScale),
                    float(car.position.y*kWorldScale)};
    camera_.up={0,1,0};
    camera_.projection=CAMERA_PERSPECTIVE;
    const float speedRatio=float(std::abs(car.speed)/spec.topSpeed);
    // Mild peripheral expansion at speed, no camera orbit changes.
    const float t=std::clamp(speedRatio,0.f,1.f);
    camera_.fovy=64.f+14.f*(t*t*(3.f-2.f*t));
}

void Game::showFault(const std::string& subsystem,const std::string& message) {
    faultMessage_=subsystem+": "+message;
    std::cerr<<"[Karting C++] "<<faultMessage_<<"\n";
}

void Game::renderMenu(double time) {
    const int width=GetScreenWidth(),height=GetScreenHeight();
    const Color top{20,22,35,255},bottom{49,54,65,255};
    DrawRectangleGradientV(0,0,width,height,top,bottom);
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
                  sim_.colorIndex(),false);
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
    if (n<.8f && r<.8f) {
        const float x=float(width*.78);
        const float y=92.f+120.f*n;
        DrawCircle(int(x),int(y),28.f*(1-n*.5f),
                   Fade(n>.4f?ORANGE:YELLOW,(1-r)*(1-n*.6f)));
    }
    if (n>.5f && r<.6f) {
        for (int i=0;i<90;++i) {
            const int x=(i*617+23)%width;
            const int y=(i*277+17)%(height/2);
            DrawCircle(x,y,1,Fade(WHITE,(n-.5f)*2*(1-r)));
        }
        DrawCircle(width/5,100,19,Fade(RAYWHITE,(n-.5f)*2*(1-r)));
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
    safe("world",[&]{
        world_.draw(sim_.track(),weather,quality_,camera_.position,gameTime_);
    });
    safe("effects",[&]{ effects_.draw3D(camera_); });
    safe("vehicle",[&]{
        vehicle_.draw(sim_.car(),sim_.profile(),
                      sim_.vehicleIndex(),sim_.colorIndex(),n>.2);
    });
    EndMode3D();

    // Rain is drawn in a separate transparent screen pass.
    if (r>.03f) {
        const int drops=int((quality_==0?80:quality_==1?140:220)*r);
        for (int i=0;i<drops;++i) {
            const int x=(i*173+int(gameTime_*160))%(width+65)-32;
            const int y=(i*277+int(gameTime_*900))%(height+25);
            DrawLine(x,y,x+3,y+15,Fade(SKYBLUE,r*.7f));
        }
        if (quality_>0)
            DrawRectangle(0,0,width,height,Fade({90,110,135,255},r*.08f));
    }
    // Subtle peripheral streaks above ~55% of top speed.
    // Only screen edges are affected: braking points and HUD stay readable.
    const float speedRatio=float(std::clamp(
        std::abs(sim_.car().speed)/sim_.profile().topSpeed,0.0,1.0));
    const float intensity=std::clamp((speedRatio-.55f)/.45f,0.f,1.f);
    if (intensity>.01f && quality_>0) {
        const int lines=quality_==1?6:10;
        for (int i=0;i<lines;++i) {
            const float phase=std::fmod(float(gameTime_)*(1.2f+speedRatio*2.8f)
                                          +float(i)*.618034f,1.f);
            const float y=phase*height;
            const float x=width*(.035f+.08f*float(i%3)/3.f);
            const float len=(12.f+24.f*intensity)*(height/720.f);
            const Color streak=Fade(RAYWHITE,.13f*intensity*(1.f-r*.5f));
            DrawLineEx({x,y},{x+len*.30f,y-len},1.1f,streak);
            DrawLineEx({float(width)-x,y},
                       {float(width)-x-len*.30f,y-len},1.1f,streak);
        }
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
    SetTargetFPS(60);
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
                const Controls keys=controlsFromKeyboard();
                for (int step=clock_.consume(frame);step>0;--step) {
                    const double previousRecord=sim_.race().bestLap;
                    try {
                        sim_.update(keys,FixedClock::step);
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
        CloseWindow();
        throw;
    }
    CloseWindow();
    return 0;
}

} // namespace karting
