#include "karting/game.hpp"
#include <algorithm>
#include <cmath>

namespace karting {

UiViewport uiViewport() {
    const int physicalWidth=std::max(1,GetScreenWidth());
    const int physicalHeight=std::max(1,GetScreenHeight());
    const float scale=std::max(0.25f,std::min(physicalWidth/1100.f,
                                              physicalHeight/720.f));
    return {scale,int(std::round(physicalWidth/scale)),
                  int(std::round(physicalHeight/scale))};
}

void HudRenderer::menu(const Simulation& sim,double time) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());
    DrawText("KARTING V6  /  C++20 + raylib",w/2-280,24,34,GOLD);
    DrawText("CHOISIS TON VEHICULE",w/2-210,74,27,RAYWHITE);
    const auto& cars=profiles();
    for (int i=0;i<4;++i) {
        const int x=w/2-420+i*210;
        const bool selected=i==sim.vehicleIndex();
        DrawRectangleRounded({float(x),121,197,42},.15f,8,
                             selected?GOLD:Color{52,59,72,255});
        DrawText(cars[i].name.data(),x+65,130,22,selected?BLACK:RAYWHITE);
    }
    DrawText("PERFORMANCES",54,200,24,RAYWHITE);
    const auto& p=sim.profile();
    const std::array<std::pair<const char*,float>,6> stats{{
        {"Vitesse",float(p.topSpeed/1020)},
        {"Acceleration",float(p.power/1700)},
        {"Freinage",float(p.brakeForce/900)},
        {"Adherence",float((p.frontGrip+p.rearGrip)/2000)},
        {"Appui aero",float(p.downforce/1.5)},
        {"Herbe",float(1-p.grassDrag/.35)}
    }};
    for(int i=0;i<6;++i) {
        const int y=244+i*38;
        DrawText(stats[i].first,56,y,20,LIGHTGRAY);
        DrawRectangle(210,y+2,157,16,{42,47,58,255});
        DrawRectangle(210,y+2,int(157*std::clamp(stats[i].second,0.f,1.f)),16,
                      i%2==0?ORANGE:LIME);
    }
    DrawText("PHYSIQUE ET STYLE",w-385,205,22,RAYWHITE);
    const char* description[4] = {
        "Kart nerveux, tres leger, agilité",
        "4x4 stable, grand debattement",
        "GT3 polyvalente et efficace",
        "WEC hypercar et appui enorme"
    };
    DrawText(description[sim.vehicleIndex()],w-385,244,18,LIGHTGRAY);
    DrawText(TextFormat("Vitesse max : %d km/h",
             int(toKmH(p.topSpeed))),w-385,280,19,RAYWHITE);
    DrawText(TextFormat("Puissance : %.0f (jeu)",p.power),
             w-385,309,19,RAYWHITE);
    DrawText("PEINTURE",w/2-110,h-142,22,RAYWHITE);
    for (int i=0;i<6;++i) {
        const int x=w/2-163+i*54;
        DrawRectangle(x,h-107,43,35,palette(i));
        if (i==sim.colorIndex())
            DrawRectangleLines(x-4,h-111,51,43,RAYWHITE);
    }
    DrawText("GAUCHE/DROITE voiture    HAUT/BAS couleur    ENTREE jouer",
             w/2-336,h-55,19,LIGHTGRAY);
    DrawText("ESC quitter",w/2-55,h-28,15,GRAY);
    (void)time;
    EndMode2D();
}

void HudRenderer::draw(const Simulation& sim,int quality,int fps) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());

    const auto& car=sim.car();
    const auto& race=sim.race();
    const auto& telemetry=sim.telemetry();
    const auto& weather=sim.weather();
    const auto& profile=sim.profile();
    DrawRectangleRounded({14,13,320,113},.14f,8,Fade(BLACK,.69f));
    DrawText(TextFormat("TOUR %d   %.2f s",race.laps+1,race.elapsed),
             27,24,25,RAYWHITE);
    if(race.bestLap>0)
        DrawText(TextFormat("Record : %.2f s",race.bestLap),27,58,20,GOLD);
    else
        DrawText("Record : --",27,58,20,GOLD);
    DrawText(TextFormat("%s  %d FPS  %s",profile.name.data(),fps,
                    (quality==0?"ECO":quality==1?"ELEVE":"ULTRA")),
             27,95,17,LIGHTGRAY);

    // Fit the complete circuit into the minimap irrespective of its size.
    const auto& center=sim.track().center();
    const float panelX=float(w-239),panelY=27.f;
    DrawRectangleRounded({panelX-10,panelY-14,228,187},.11f,8,
                         Fade(BLACK,.75f));
    double minX=center.front().x,maxX=minX;
    double minY=center.front().y,maxY=minY;
    for(const Vec2 p:center) {
        minX=std::min(minX,p.x); maxX=std::max(maxX,p.x);
        minY=std::min(minY,p.y); maxY=std::max(maxY,p.y);
    }
    const float mapScale=std::min(196.f/float(std::max(1.0,maxX-minX)),
                                  142.f/float(std::max(1.0,maxY-minY)));
    const float x0=panelX+(196.f-float(maxX-minX)*mapScale)*.5f;
    const float y0=panelY+(142.f-float(maxY-minY)*mapScale)*.5f;
    const auto project=[&](Vec2 p) {
        return Vector2{x0+float(p.x-minX)*mapScale,
                       y0+float(p.y-minY)*mapScale};
    };
    for (std::size_t i=0;i<center.size();i+=4) {
        const auto& from=center[i],to=center[(i+4)%center.size()];
        DrawLineEx(project(from),project(to),2.2f,RAYWHITE);
    }
    DrawCircleV(project(center.front()),3,GOLD);
    DrawCircleV(project(car.position),5,RED);
    const char* phases[]={"JOUR","CREPUSCULE","NUIT"};
    DrawText(TextFormat("%s%s",phases[weather.phase],
             weather.raining?" + PLUIE":""),w-224,201,17,GOLD);

    // Speedometer is an analog dial, independent of FPS and scene.
    const float cx=float(w-145),cy=float(h-123),r=94;
    DrawCircleV({cx,cy},r+8,Fade(BLACK,.85f));
    DrawCircleLines(int(cx),int(cy),r,GOLD);
    const float fraction=std::clamp(float(std::abs(car.speed)/profile.topSpeed),0.f,1.f);
    for (int i=0;i<=40;++i) {
        const float a=(150.f+240.f*i/40.f)*DEG2RAD;
        const float inner=r-(i%4==0?17:9);
        DrawLineEx({cx+std::cos(a)*inner,cy+std::sin(a)*inner},
                   {cx+std::cos(a)*r,cy+std::sin(a)*r},
                   i%4==0?2.2f:1.f,i>33?RED:RAYWHITE);
    }
    const float angle=(150+240*fraction)*DEG2RAD;
    DrawLineEx({cx,cy},{cx+std::cos(angle)*(r-23),cy+std::sin(angle)*(r-23)},
               4,GOLD);
    DrawCircleV({cx,cy},7,RAYWHITE);
    DrawText(TextFormat("%03d",int(std::abs(toKmH(car.speed)))),
             int(cx-38),int(cy+27),32,RAYWHITE);
    DrawText("km/h",int(cx-22),int(cy+62),15,LIGHTGRAY);

    // Pedal bars and grip indicator.
    const std::array<std::pair<const char*,double>,3> pedals{{
        {"GAZ",car.throttle},{"FREIN",car.brake},{"DISQ",car.brakeHeat}
    }};
    const std::array<Color,3> colors{{LIME,RED,ORANGE}};
    for (int i=0;i<3;++i) {
        const int x=22+i*65,y=h-160;
        DrawRectangle(x,y,43,106,Fade(BLACK,.7f));
        const int filled=int(std::clamp(pedals[i].second,0.0,1.0)*102);
        DrawRectangle(x+2,y+104-filled,39,filled,colors[i]);
        DrawRectangleLines(x,y,43,106,LIGHTGRAY);
        DrawText(pedals[i].first,x-2,h-48,15,RAYWHITE);
    }
    DrawText(TextFormat("GRIP : %d%%",int(telemetry.grip*100)),
             w/2-85,h-31,19,SKYBLUE);
    DrawText("ESPACE drift  N ciel  P pluie  G qualite  R reset  M menu",
             w/2-316,h-56,17,LIGHTGRAY);

    if (telemetry.wheelsOutside>0)
        DrawText(TextFormat("%d ROUE(S) DANS L'HERBE",telemetry.wheelsOutside),
                 w/2-190,27,22,ORANGE);
    if (telemetry.frontSlip)
        DrawText("SOUS-VIRAGE : FREINAGE",w/2-165,59,20,ORANGE);
    if (telemetry.rearSlip || telemetry.drifting)
        DrawText("DERAPAGE !",w/2-75,91,24,GOLD);

    if (telemetry.inPits) {
        DrawRectangleRounded({float(w/2-310),float(h/2-180),620,332},
                             .08f,8,Fade(BLACK,.90f));
        DrawRectangleLines(w/2-310,h/2-180,620,332,GOLD);
        DrawText("STANDS - TABLEAU DES SCORES",w/2-265,h/2-155,28,GOLD);
        DrawText(TextFormat("Tours : %d",race.laps),w/2-260,h/2-100,23,RAYWHITE);
        if (race.lastLap>0)
            DrawText(TextFormat("Dernier : %.2f s",race.lastLap),
                     w/2-260,h/2-57,23,RAYWHITE);
        if (race.bestLap>0)
            DrawText(TextFormat("Meilleur : %.2f s",race.bestLap),
                     w/2-260,h/2-14,23,GOLD);
        DrawText(TextFormat("Pointe : %d km/h",int(toKmH(race.topSpeed))),
                 w/2-260,h/2+33,23,RAYWHITE);
    }
    EndMode2D();
}

} // namespace karting
