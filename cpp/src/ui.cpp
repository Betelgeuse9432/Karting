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

void HudRenderer::modeMenu(int selectedMode,double time) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());
    DrawText("GAME VROUM",w/2-190,48,48,GOLD);
    DrawText("MONDE DISPONIBLE",w/2-190,112,27,RAYWHITE);
    const Rectangle card{float(w/2-175),205,350,320};
    DrawRectangleRounded(card,.055f,10,{57,67,82,255});
    DrawRectangleLinesEx(card,4,GOLD);
    DrawRectangleRounded({card.x+24,card.y+25,302,145},.08f,8,{25,30,39,255});
    DrawText("GT3",int(card.x+118),card.y+76,42,GOLD);
    DrawText("PISTE",int(card.x+28),card.y+198,29,GOLD);
    DrawText("Asphalte et circuit",int(card.x+28),card.y+245,18,RAYWHITE);
    DrawText("PISTE GT3 SELECTIONNEE",w/2-150,h-105,18,GOLD);
    DrawText("ENTREE : continuer",w/2-115,h-65,19,RAYWHITE);
    DrawText(TextFormat("GAME VROUM  |  MENU %02d",int(time*2)%100),20,h-28,14,GRAY);
    (void)selectedMode;
    EndMode2D();
}

void HudRenderer::menu(const Simulation& sim,double time) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());
    DrawText(sim.isBaja()?"TROPHY BAJA  /  GAME VROUM":sim.isDesert()?"DESERT  /  GAME VROUM":"KARTING V6  /  C++20 + raylib",
             w/2-280,24,34,GOLD);
    DrawText("CHOISIS TON VEHICULE",
             w/2-210,74,27,RAYWHITE);
    const auto& cars=profiles();
    constexpr std::array<int,5> vehicleSlots{{0,2,3,5,6}};
    const int count=int(vehicleSlots.size());
    for (int i=0;i<count;++i) {
        const int index=vehicleSlots[std::size_t(i)];
        const int x=w/2-count*80+i*160;
        const bool selected=index==sim.vehicleIndex();
        DrawRectangleRounded({float(x),121,150,42},.15f,8,
                             selected?Color{235,177,105,255}:Color{52,59,72,255});
        DrawText(cars[index].name.data(),x+12,130,22,selected?BLACK:RAYWHITE);
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
    const char* description[10] = {
        "Kart nerveux, tres leger, agilite",
        "4x4 stable, grand debattement",
        "GT3 polyvalente et efficace",
        "WEC hypercar et appui enorme",
        "Rallycross nerveuse, chassis sureleve",
        "F1 legere, appui aero et suspensions apparentes",
        "GT3 agressive, livree Monster Energy",
        "Trophy Truck long-travel, V8 et gros debattement",
        "Trophy Truck spec course, chassis tubulaire renforce",
        "Trophy Truck grand debattement, configuration desert"
    };
    DrawText(description[sim.vehicleIndex()],w-385,244,18,LIGHTGRAY);
    DrawText(TextFormat("Vitesse max : %d km/h",
             int(p.topSpeed*.3)),w-385,280,19,RAYWHITE);
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
                    (quality==0?"ECO":quality==1?"ELEVE":quality==2?"ULTRA":"ULTRA+")),
             27,95,17,LIGHTGRAY);

    // Circuit minimap (V6 control points and interpolated curve).
    const auto& center=sim.track().center();
    const float mx=float(w-239),my=27.f;
    const Rectangle mapRect{mx-10,my-14,228,187};
    DrawRectangleRounded(mapRect,.11f,8,Fade(BLACK,.75f));
    double minX=1e9,maxX=-1e9,minY=1e9,maxY=-1e9;
    for(const auto& point:center) {
        minX=std::min(minX,point.x); maxX=std::max(maxX,point.x);
        minY=std::min(minY,point.y); maxY=std::max(maxY,point.y);
    }
    const float mapScale=std::min(float((mapRect.width-28)/std::max(1.0,maxX-minX)),
                                  float((mapRect.height-28)/std::max(1.0,maxY-minY)));
    const float mapW=float(maxX-minX)*mapScale,mapH=float(maxY-minY)*mapScale;
    const float mapX=mapRect.x+(mapRect.width-mapW)*.5f;
    const float mapY=mapRect.y+(mapRect.height-mapH)*.5f;
    const auto mapPoint=[&](Vec2 point) {
        return Vector2{mapX+float(point.x-minX)*mapScale,
                       mapY+float(point.y-minY)*mapScale};
    };
    for(std::size_t i=0;i<center.size();i+=4)
        DrawLineEx(mapPoint(center[i]),mapPoint(center[(i+4)%center.size()]),
                   2.3f,sim.isBaja()?Color{222,177,120,255}:RAYWHITE);
    if(sim.isBaja()) {
        DrawCircleV(mapPoint(sim.opponent().position),4,SKYBLUE);
        DrawCircleV(mapPoint(car.position),5,RED);
    } else DrawCircleV(mapPoint(car.position),5,RED);
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
    DrawText(TextFormat("%03d",int(std::abs(car.speed)*.3)),
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
    const Color boostColor=sim.boostActive()?LIME:(sim.boostsRemaining()>0?GOLD:GRAY);
    DrawText(TextFormat("BOOST : %d DISPONIBLE(S)",sim.boostsRemaining()),
             w/2-112,h-88,18,boostColor);
    if (sim.boostActive())
        DrawText(TextFormat("BOOST ACTIF  %.1f s",sim.boostTimeRemaining()),
                 w/2-108,h-111,17,LIME);
    DrawText("ESPACE boost (2 x 2 s)  N ciel  P pluie  G qualite  R reset  M menu",
             w/2-365,h-56,16,LIGHTGRAY);

    if (race.warningTime>0)
        DrawText("LIMITES DE PISTE : +1 SECONDE",w/2-239,152,23,RED);
    if (telemetry.wheelsOutside>0)
        DrawText(TextFormat("%d ROUE(S) DANS L'HERBE",telemetry.wheelsOutside),
                 w/2-190,27,22,ORANGE);
    if (telemetry.frontSlip)
        DrawText("SOUS-VIRAGE : FREINAGE",w/2-165,59,20,ORANGE);
    if (telemetry.rearSlip || telemetry.drifting)
        DrawText("PERTE D'ADHERENCE",w/2-112,91,20,GOLD);

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
        DrawText(TextFormat("Pointe : %d km/h",int(race.topSpeed*.3)),
                 w/2-260,h/2+33,23,RAYWHITE);
    }
    EndMode2D();
}

} // namespace karting
