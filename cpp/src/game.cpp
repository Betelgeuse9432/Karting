#include "karting/game.hpp"
#include <algorithm>
#include "raymath.h"
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
// Accept the typed 'm' on AZERTY and other keyboard layouts while keeping
// the existing physical KEY_M shortcut. No on-screen menu button.
bool menuKeyPressed() {
    bool pressed=IsKeyPressed(KEY_M);
    for(int character=GetCharPressed();character>0;
        character=GetCharPressed()) {
        if(character=='m' || character=='M') pressed=true;
    }
    return pressed;
}
// raylib 6 uses a Vector2 center; raylib 5 uses separate coordinates.
void drawSunGlow(Vector2 center,float radius,Color inner,Color outer) {
#if RAYLIB_VERSION_MAJOR >= 6
    DrawCircleGradient(center,radius,inner,outer);
#else
    DrawCircleGradient(int(center.x),int(center.y),radius,inner,outer);
#endif
}

// Project world-space light directions into the camera's visible sky.
bool skyPoint(const Camera3D& camera,Vector3 direction,int width,int height,Vector2& out) {
    const Vector3 forward=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
    const Vector3 right=Vector3Normalize(Vector3CrossProduct(forward,camera.up));
    const Vector3 up=Vector3CrossProduct(right,forward);
    const float depth=Vector3DotProduct(direction,forward);
    if(depth<=.01f) return false;
    const float focal=float(height)/(2.f*std::tan(camera.fovy*DEG2RAD*.5f));
    out={width*.5f+focal*Vector3DotProduct(direction,right)/depth,
         height*.5f-focal*Vector3DotProduct(direction,up)/depth};
    return out.x>-100 && out.x<width+100 && out.y>-100 && out.y<height+100;
}
// Screen-space sky with a camera-relative azimuth. Project only directions
// inside the current horizontal FOV; no offscreen clouds are submitted.
// Mountain silhouettes use world-space bearings so they react to turning.
void drawVisibleSky(const Camera3D& camera,int width,int height,
                    float night,float rain,double seconds,int quality) {
    if(quality<=0 || width<=0 || height<=0) return;
    const Vector3 forward=Vector3Normalize(
        Vector3Subtract(camera.target,camera.position));
    const float yaw=std::atan2(forward.z,forward.x);
    const float halfVertical=std::tan(camera.fovy*DEG2RAD*.5f);
    const float halfHorizontal=halfVertical*
            (float(width)/float(std::max(1,height)));
    const float horizontalFov=2.f*std::atan(halfHorizontal);
    const float focal=float(height)/(2.f*halfVertical);
    const float horizon=std::clamp(float(height)*.5f+forward.y*focal,
                                   -float(height),float(height)*2.f);
    // Nothing above the horizon falls within the screen.
    if(horizon<=-100.f) return;
    const float tau=6.28318530718f;

    const float opacity=(1.f-night*.78f)*(1.f-rain*.55f);
    if(opacity<.03f) return;
    const Color body=Fade(lerp({232,239,247,255},
                               {115,123,140,255},night*.7f+rain*.25f),
                          opacity*.72f);
    const Color shadeCloud=Fade({91,104,121,255},opacity*.22f);
    // A fixed number of azimuthal cloud banks. Discard all cloud banks
    // outside the camera frustum BEFORE issuing any draw calls.
    const int clouds=quality==1?20:32;
    for(int i=0;i<clouds;++i) {
        const float angularPosition=tau*(float(i)+.33f)/float(clouds)+
                                     float(seconds)*.001f;
        const float delta=std::remainder(angularPosition-yaw,tau);
        if(std::abs(delta)>horizontalFov*.5f+.09f) continue;
        const float x=float(width)*.5f+
            std::tan(delta)/halfHorizontal*float(width)*.5f;
        const float scale=.75f+.15f*float(i%4);
        const float y=horizon-(70.f+float((i*37)%135));
        if(x< -80.f || x>width+80.f || y< -70.f || y>height+50.f) continue;
        DrawEllipse(int(x),int(y+7*scale),44*scale,13*scale,shadeCloud);
        DrawEllipse(int(x-23*scale),int(y),22*scale,13*scale,body);
        DrawEllipse(int(x+1*scale),int(y-8*scale),30*scale,19*scale,body);
        DrawEllipse(int(x+27*scale),int(y),22*scale,13*scale,body);
    }
}
} // namespace

void Game::reset() {
    sim_.reset();
    clock_.reset();
    effects_.clear();
    paused_=false;
    disabled_.clear();
    faultMessage_.clear();
    previousCar_=renderCar_=sim_.car();
    chase_.reset(renderCar_,sim_.profile());
    follow(0.0);
}

void Game::handleMenu() {
    if(IsKeyPressed(KEY_TAB))
        mode_=mode_==DrivingMode::Scenic?DrivingMode::TimeTrial:DrivingMode::Scenic;
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
        dynamicCamera_=mode_==DrivingMode::TimeTrial;
        scenicHud_=true;
        cameraNoticeUntil_=0;
        scenicNoticeUntil_=gameTime_+5.0;
        reset();
    }
}

void Game::handleRace() {
    if (menuKeyPressed()) {
        menu_=true;
        reset();
        return;
    }
    if (IsKeyPressed(KEY_R)) reset();
    if (mode_==DrivingMode::Scenic && IsKeyPressed(KEY_H)) scenicHud_=!scenicHud_;
    if (IsKeyPressed(KEY_G)) quality_=(quality_+1)%3;
    if (IsKeyPressed(KEY_V)) {
        dynamicCamera_=!dynamicCamera_;
        cameraNoticeUntil_=gameTime_+2.0;
    }
    if (IsKeyPressed(KEY_L)) {
        headlightMode_=static_cast<HeadlightMode>((static_cast<int>(headlightMode_)+1)%4);
        lightNoticeUntil_=gameTime_+2.5;
    }
    if (IsKeyPressed(KEY_N)) sim_.cycleWeather();
    if (IsKeyPressed(KEY_P)) sim_.toggleRain();
}

void Game::follow(double dt) {
    const auto& car=renderCar_;
    const auto& spec=sim_.profile();
    chase_.update(car,spec,dt,dynamicCamera_);
    const Vec2 behind{std::cos(chase_.angle),std::sin(chase_.angle)};
    const Vec2 view=car.position-behind*chase_.distance;
    const double aimHeading=car.heading+chase_.lookAngle;
    const Vec2 forward{std::cos(aimHeading),std::sin(aimHeading)};
    const Vec2 aim=car.position+forward*chase_.lookAhead;
    // Never follow the upper road when driving below the bridge. Only
    // the natural ground is a clearance constraint for the chase camera.
    const double safeHeight=std::max(chase_.height,elevation(view)+22.0);
    camera_.position={float(view.x*kWorldScale),
                      float(safeHeight*kWorldScale),
                      float(view.y*kWorldScale)};
    camera_.target={float(aim.x*kWorldScale),
                    float((car.height+30)*kWorldScale),
                    float(aim.y*kWorldScale)};
    camera_.up={0,1,0};
    camera_.projection=CAMERA_PERSPECTIVE;
    camera_.fovy=float(chase_.fov);
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
    // Showroom turntable stays level; only the racing car follows terrain.
    vehicle_.draw(display,sim_.profile(),sim_.vehicleIndex(),
                  sim_.colorIndex(),false,false);
    EndMode3D();
    hud_.menu(sim_,time,mode_);
}

void Game::renderRace() {
    const int width=GetScreenWidth(),height=GetScreenHeight();
    const auto& weather=sim_.weather();
    const float n=float(weather.nightLevel),r=float(weather.rainLevel);
    if(atmosphere_.beginShadow(camera_,n,r)) {
        try {
            world_.draw(sim_.track(),weather,quality_,camera_,gameTime_,true);
            vehicle_.draw(renderCar_,sim_.profile(),sim_.vehicleIndex(),
                          sim_.colorIndex(),n>.2,true,&sim_.track());
        } catch(const std::exception& e) {
            atmosphere_.endShadow();
            showFault("shadow",e.what());
            // Do not leave the framebuffer or 3D mode active on failure.
            throw;
        }
        atmosphere_.endShadow();
    }
    const bool hdr=atmosphere_.ready() && post_.begin(width,height);
    const Color daylightTop{92,157,243,255};
    const Color daylightBottom{190,220,250,255};
    const Color duskTop{76,84,144,255};
    const Color duskBottom{239,133,76,255};
    const Color nightTop{9,15,36,255};
    const Color nightBottom{29,42,73,255};
    Color top=n<.55f?lerp(daylightTop,duskTop,n/.55f):
                       lerp(duskTop,nightTop,(n-.55f)/.45f);
    Color bottom=n<.55f?lerp(daylightBottom,duskBottom,n/.55f):
                          lerp(duskBottom,nightBottom,(n-.55f)/.45f);
    top=lerp(top,{76,85,100,255},r*.6f);
    bottom=lerp(bottom,{108,115,130,255},r*.6f);
    const Vector3 forward=Vector3Normalize(Vector3Subtract(camera_.target,camera_.position));
    const float focal=float(height)/(2.f*std::tan(camera_.fovy*DEG2RAD*.5f));
    const int horizon=std::clamp(int(height*.5f+forward.y*focal),1,std::max(1,height));
    if(hdr) post_.sky(camera_,top,bottom,Atmosphere::sunDirection(n),n,r);
    else {
    DrawRectangleGradientV(0,0,width,horizon,top,bottom);
    DrawRectangle(0,horizon,width,std::max(0,height-horizon),bottom);
    if(n<.8f && r<.8f) {
        Vector2 sun{};
        if(skyPoint(camera_,Atmosphere::sunDirection(n),width,height,sun)) {
            const float alpha=(1-r)*(1-n*.9f);
            const Color glow=n>.35f?Color{255,170,92,255}:Color{255,228,158,255};
            const float scale=float(height)/720.f;
            if(quality_>0) {
                drawSunGlow(sun,72.f*scale,
                                   Fade(glow,.19f*alpha),Fade(glow,0.f));
                drawSunGlow(sun,37.f*scale,
                                   Fade(glow,.34f*alpha),Fade(glow,0.f));
            }
            DrawCircleV(sun,13.f*scale,Fade({255,239,205,255},alpha));
        }
    }
    }
    post_.beginLinear();
    if(n>.5f && r<.6f) {
        // Stars have fixed world bearings instead of sliding with the screen.
        for(int i=0;i<90;++i) {
            const float bearing=float(i)*2.39996323f;
            const float y=.12f+.86f*float((i*37)%89)/89.f;
            const float radius=std::sqrt(std::max(0.f,1.f-y*y));
            Vector2 star{};
            if(skyPoint(camera_,{radius*std::cos(bearing),y,radius*std::sin(bearing)},
                        width,height,star))
                DrawCircleV(star,1.f,Fade(WHITE,(n-.5f)*2*(1-r)));
        }
        Vector2 moon{};
        if(!hdr && skyPoint(camera_,Atmosphere::moonDirection(),width,height,moon))
            DrawCircleV(moon,11.f*height/720.f,Fade(RAYWHITE,(n-.5f)*2*(1-r)));
    }
    drawVisibleSky(camera_,width,height,n,r,gameTime_,quality_);
    post_.endLinear();

    // Each subsystem can fail independently without taking down the process.
    const auto safe=[&](const char* name,auto&& operation) {
        if (disabled_.contains(name)) return;
        try { operation(); }
        catch (const std::exception& e) {
            disabled_.insert(name);
            showFault(name,e.what());
        }
    };
    const auto headlights=vehicle_.headlights(renderCar_,sim_.profile(),sim_.vehicleIndex(),
                                               sim_.track(),headlightMode_,n,r);
    BeginMode3D(camera_);
    atmosphere_.begin(camera_,bottom,n,r,quality_,hdr,world_.streetLights(camera_.position,n),headlights);
    safe("world",[&]{
        world_.draw(sim_.track(),weather,quality_,camera_,gameTime_);
    });
    safe("vehicle",[&]{
        drawContactShadow(sim_.track(),renderCar_,sim_.profile(),n,r);
        vehicle_.draw(renderCar_,sim_.profile(),
                      sim_.vehicleIndex(),sim_.colorIndex(),n>.2,
                      true,&sim_.track(),headlights.intensity>.01f);
    });
    atmosphere_.end();
    // Smoke, flames and sparks remain emissive and are composited last.
    post_.beginLinear();
    safe("effects",[&]{ effects_.draw3D(camera_); });
    post_.endLinear();
    EndMode3D();
    if(hdr) post_.finish(n,r);

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
    if (mode_==DrivingMode::TimeTrial && dynamicCamera_ && intensity>.01f && quality_>0) {
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

    safe("hud",[&]{
        if(mode_==DrivingMode::TimeTrial) hud_.draw(sim_,quality_,GetFPS());
        else if(scenicHud_) hud_.scenic(sim_);
    });
    if(mode_==DrivingMode::Scenic && gameTime_<scenicNoticeUntil_ && scenicHud_) {
        const char* hint="CONDUITE LIBRE  |  H interface  N ciel  P pluie  L phares  V camera  M menu";
        const int font=16;
        const int w=MeasureText(hint,font);
        DrawRectangle((width-w)/2-12,24,w+24,30,Fade(BLACK,.58f));
        DrawText(hint,(width-w)/2,31,font,RAYWHITE);
    }
    if(gameTime_<lightNoticeUntil_) {
        const char* labels[]={"PHARES AUTO [L]","FEUX DE CROISEMENT [L]","PLEINS PHARES [L]","PHARES ETEINTS [L]"};
        const char* label=labels[static_cast<int>(headlightMode_)];
        const int font=20,w=MeasureText(label,font);
        DrawRectangle((width-w)/2-10,98,w+20,32,Fade(BLACK,.72f));
        DrawText(label,(width-w)/2,104,font,RAYWHITE);
    }
    if(gameTime_<cameraNoticeUntil_) {
        const char* label=dynamicCamera_?"CAMERA DOUCE [V]":"CAMERA CONFORT [V]";
        const int font=20;
        const int w=MeasureText(label,font);
        DrawRectangle((width-w)/2-10,60,w+20,32,Fade(BLACK,.72f));
        DrawText(label,(width-w)/2,66,font,RAYWHITE);
    }
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
        atmosphere_.initialize();
        post_.initialize();
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
                        previousCar_=sim_.car();
                        sim_.update(keys,FixedClock::step,mode_==DrivingMode::TimeTrial);
                    } catch(const std::exception& e) {
                        showFault("physics",e.what());
                        paused_=true;
                        break;
                    }
                    if (mode_==DrivingMode::TimeTrial && sim_.race().bestLap>0 &&
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
                renderCar_=interpolateCar(previousCar_,sim_.car(),clock_.alpha());
                follow(frame);
            }
            render();
        }
    } catch (...) {
        post_.shutdown();
        atmosphere_.shutdown();
        CloseWindow();
        throw;
    }
    post_.shutdown();
    atmosphere_.shutdown();
    CloseWindow();
    return 0;
}

} // namespace karting
