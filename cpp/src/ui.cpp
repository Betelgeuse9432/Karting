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

    DrawText("GAME VROUM",w/2-190,58,48,GOLD);
    DrawText("CHOISIS TON TERRAIN DE JEU",w/2-250,119,27,RAYWHITE);
    DrawText("Deux disciplines. Deux ambiances. Une seule envie : attaquer.", 
             w/2-300,158,18,LIGHTGRAY);

    const Rectangle circuit{float(w/2-430),225,390,300};
    const Rectangle baja{float(w/2+40),225,390,300};
    const Color idle{35,42,55,255};
    const Color active{57,67,82,255};
    DrawRectangleRounded(circuit,.055f,10,selectedMode==0?active:idle);
    DrawRectangleRounded(baja,.055f,10,selectedMode==1?active:idle);
    DrawRectangleLinesEx(circuit,selectedMode==0?4.f:1.5f,
                         selectedMode==0?GOLD:Color{91,101,117,255});
    DrawRectangleLinesEx(baja,selectedMode==1?4.f:1.5f,
                         selectedMode==1?GOLD:Color{91,101,117,255});

    // Stylised circuit card: asphalt ribbon, kerbs and start/finish markings.
    DrawRectangleRounded({circuit.x+28,circuit.y+27,334,132},.08f,8,
                         {21,26,35,255});
    DrawLineEx({circuit.x+75,circuit.y+93},{circuit.x+135,circuit.y+55},17,
               {105,111,121,255});
    DrawLineEx({circuit.x+135,circuit.y+55},{circuit.x+245,circuit.y+65},17,
               {105,111,121,255});
    DrawLineEx({circuit.x+245,circuit.y+65},{circuit.x+298,circuit.y+119},17,
               {105,111,121,255});
    DrawLineEx({circuit.x+298,circuit.y+119},{circuit.x+190,circuit.y+135},17,
               {105,111,121,255});
    DrawLineEx({circuit.x+190,circuit.y+135},{circuit.x+75,circuit.y+93},17,
               {105,111,121,255});
    DrawLineEx({circuit.x+75,circuit.y+93},{circuit.x+135,circuit.y+55},2,WHITE);
    DrawText("PISTE",int(circuit.x+28),circuit.y+181,29,GOLD);
    DrawText("GT3  /  ASPHALTE  /  CIRCUIT",int(circuit.x+28),
             circuit.y+222,19,RAYWHITE);
    DrawText("Freinages, trajectoires et vitesse",int(circuit.x+28),
             circuit.y+254,16,LIGHTGRAY);

    // Desert card: dune silhouettes, a jump ramp and a Trophy Truck outline.
    DrawRectangleRounded({baja.x+28,baja.y+27,334,132},.08f,8,
                         {35,31,29,255});
    DrawTriangle({baja.x+28,baja.y+148},{baja.x+145,baja.y+66},
                 {baja.x+235,baja.y+148},{160,111,62,255});
    DrawTriangle({baja.x+135,baja.y+148},{baja.x+255,baja.y+82},
                 {baja.x+362,baja.y+148},{190,135,73,255});
    DrawLineEx({baja.x+65,baja.y+135},{baja.x+270,baja.y+135},3,
               {235,190,126,255});
    DrawRectangleRounded({baja.x+145,baja.y+93,105,28},.18f,6,
                         {28,31,35,255});
    DrawCircle(int(baja.x+165),baja.y+126,17,{20,22,25,255});
    DrawCircle(int(baja.x+232),baja.y+126,17,{20,22,25,255});
    DrawText("HORS-PISTE",int(baja.x+28),baja.y+181,29,
             {238,177,104,255});
    DrawText("TROPHY BAJA  /  SABLE  /  SAUTS",int(baja.x+28),
             baja.y+222,19,RAYWHITE);
    DrawText("Deux Trophy Trucks et suspensions longues",int(baja.x+28),
             baja.y+254,16,LIGHTGRAY);

    const char* hint=selectedMode==0
        ?"PISTE GT3 SELECTIONNEE"
        :"TROPHY BAJA SELECTIONNE";
    DrawText(hint,w/2-145,h-111,18,GOLD);
    DrawText("GAUCHE / DROITE : choisir     ENTREE : valider",
             w/2-270,h-72,19,RAYWHITE);
    DrawText(TextFormat("GAME VROUM  |  MENU %02d",int(time*2)%100),
             20,h-28,14,GRAY);
    EndMode2D();
}

void HudRenderer::bajaPreview(double time) const {
    const auto ui=uiViewport();
    const int w=ui.width,h=ui.height;
    BeginMode2D(ui.camera());
    DrawText("TROPHY BAJA",w/2-175,74,42,{238,177,104,255});
    DrawText("LE DESERT ARRIVE DANS GAME VROUM",w/2-260,132,23,RAYWHITE);

    DrawRectangleRounded({w/2-390,h/2-115,780,260},.045f,10,
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
    DrawText("KARTING V6  /  C++20 + raylib",w/2-280,24,34,GOLD);
    DrawText("CHOISIS TON VEHICULE",w/2-210,74,27,RAYWHITE);
    const auto& cars=profiles();
    for (int i=0;i<6;++i) {
        const int x=w/2-480+i*160;
        const bool selected=i==sim.vehicleIndex();
        DrawRectangleRounded({float(x),121,150,42},.15f,8,
                             selected?GOLD:Color{52,59,72,255});
        DrawText(cars[i].name.data(),x+12,130,22,selected?BLACK:RAYWHITE);
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
    const char* description[6] = {
        "Kart nerveux, tres leger, agilité",
        "4x4 stable, grand debattement",
        "GT3 polyvalente et efficace",
        "WEC hypercar et appui enorme",
        "Rallycross nerveuse, chassis sureleve",
        "F1 legere, appui aero et suspensions apparentes"
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
    const float scale=.023f;
    const float mx=float(w-239),my=27.f;
    DrawRectangleRounded({mx-10,my-14,228,187},.11f,8,Fade(BLACK,.75f));
    for (std::size_t i=0;i<center.size();i+=5) {
        const auto& a=center[i],b=center[(i+5)%center.size()];
        DrawLineEx({mx+float(a.x-1600)*scale,my+float(a.y-1600)*scale},
                   {mx+float(b.x-1600)*scale,my+float(b.y-1600)*scale},
                   2.3f,RAYWHITE);
    }
    DrawCircleV({mx+float(car.position.x-1600)*scale,
                 my+float(car.position.y-1600)*scale},5,RED);
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
