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
    DrawText("CHOISIS TON MONDE",w/2-190,112,27,RAYWHITE);
    const Rectangle cards[3]={{float(w/2-555),205,350,320},
                              {float(w/2-175),205,350,320},
                              {float(w/2+205),205,350,320}};
    const Color idle{35,42,55,255},active{57,67,82,255};
    for(int i=0;i<3;++i) {
        DrawRectangleRounded(cards[i],.055f,10,selectedMode==i?active:idle);
        DrawRectangleLinesEx(cards[i],selectedMode==i?4.f:1.5f,
            selectedMode==i?GOLD:Color{91,101,117,255});
    }
    DrawRectangleRounded({cards[0].x+24,cards[0].y+25,302,145},.08f,8,{25,30,39,255});
    DrawText("GT3",int(cards[0].x+118),cards[0].y+76,42,GOLD);
    DrawText("PISTE",int(cards[0].x+28),cards[0].y+198,29,GOLD);
    DrawText("Asphalte et circuit",int(cards[0].x+28),cards[0].y+245,18,RAYWHITE);
    DrawRectangleRounded({cards[1].x+24,cards[1].y+25,302,145},.08f,8,{45,35,28,255});
    DrawTriangle({cards[1].x+24,cards[1].y+155},{cards[1].x+150,cards[1].y+48},{cards[1].x+302,cards[1].y+155},{177,119,65,255});
    DrawText("TROPHY BAJA",int(cards[1].x+28),cards[1].y+198,25,{238,177,104,255});
    DrawText("Hors-piste et grands sauts",int(cards[1].x+28),cards[1].y+245,17,RAYWHITE);
    DrawRectangleRounded({cards[2].x+24,cards[2].y+25,302,145},.08f,8,{205,164,105,255});
    DrawText("SABLE",int(cards[2].x+92),cards[2].y+82,36,{75,49,25,255});
    DrawText("DESERT",int(cards[2].x+28),cards[2].y+198,29,{238,177,104,255});
    DrawText("Sable plat a perte de vue",int(cards[2].x+28),cards[2].y+245,17,RAYWHITE);
    const char* hints[]={"PISTE GT3 SELECTIONNEE","TROPHY BAJA SELECTIONNE","DESERT SELECTIONNE"};
    DrawText(hints[std::clamp(selectedMode,0,2)],w/2-150,h-105,18,GOLD);
    DrawText("GAUCHE / DROITE : choisir     ENTREE : valider",w/2-270,h-65,19,RAYWHITE);
    DrawText(TextFormat("GAME VROUM  |  MENU %02d",int(time*2)%100),20,h-28,14,GRAY);
    EndMode2D();
}

void HudRenderer::bajaPreview(double time) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());
    DrawText("TROPHY BAJA",w/2-175,74,42,{238,177,104,255});
    DrawText("LE DESERT ARRIVE DANS GAME VROUM",w/2-260,132,23,RAYWHITE);

    DrawRectangleRounded({float(w/2-390),float(h/2-115),780.0f,260.0f},.045f,10,
                         {39,35,31,255});
    DrawRectangleLinesEx({float(w/2-390),float(h/2-115),780,260},2,
                         {190,132,75,255});
    DrawText("PROCHAINE ETAPE : LE MONDE BAJA",w/2-285,h/2-75,25,GOLD);
    DrawText("- Deux Trophy Trucks selectionnables",w/2-300,h/2-22,20,RAYWHITE);
    DrawText("- Un camion adverse pilote par l'IA",w/2-300,h/2+16,20,RAYWHITE);
    DrawText("- Desert, dunes, bosses et grands sauts",w/2-300,h/2+54,20,RAYWHITE);
    DrawText("- Chassis tubulaire et suspensions visibles",w/2-300,h/2+92,20,RAYWHITE);
    DrawText("ENTREE ou ECHAP : retour au choix du mode",
             w/2-250,h-65,18,LIGHTGRAY);
    (void)time;
    EndMode2D();
}

void HudRenderer::menu(const Simulation& sim,double time) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());
    DrawText(sim.isBaja()?"TROPHY BAJA  /  GAME VROUM":sim.isDesert()?"DESERT  /  GAME VROUM":"KARTING V6  /  C++20 + raylib",
             w/2-280,24,34,GOLD);
    DrawText((sim.isBaja()||sim.isDesert())?"CHOISIS TON TROPHY TRUCK":"CHOISIS TON VEHICULE",
             w/2-210,74,27,RAYWHITE);
    const auto& cars=profiles();
    const int first=(sim.isBaja()||sim.isDesert())?6:0;
    const int count=(sim.isBaja()||sim.isDesert())?2:6;
    for (int i=0;i<count;++i) {
        const int index=first+i;
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
    const char* description[9] = {
        "Kart nerveux, tres leger, agilite",
        "4x4 stable, grand debattement",
        "GT3 polyvalente et efficace",
        "WEC hypercar et appui enorme",
        "Rallycross nerveuse, chassis sureleve",
        "F1 legere, appui aero et suspensions apparentes",
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
    DrawText((sim.isBaja()||sim.isDesert())?"GAUCHE/DROITE camion    HAUT/BAS couleur    ENTREE rouler":
             "GAUCHE/DROITE voiture    HAUT/BAS couleur    ENTREE jouer",
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
