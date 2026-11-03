"""Ciel, terrain, circuit et bâtiments. Ne modifie jamais la simulation."""
import math
import pygame
from ..camera import Camera
from ..config import ASPHALTE, ASPHALTE2, BLANC, CIEL, CIEL2, HERBE, HERBE2, NOIR, ROUGE
from ..track import Track, Garage
from ..scenery import Tree
from .common import polygon


class WorldRenderer:
    def draw_background(self, screen: pygame.Surface, camera: Camera) -> None:
        cfg = camera.cfg
        screen.fill(CIEL)
        pygame.draw.rect(screen, CIEL2, (0, cfg.horizon-70, cfg.width, 70))
        for x in range(0, cfg.width, 6):
            heading = camera.angle+math.atan((x-cfg.width/2)/cfg.focal)
            height = int(35+25*math.sin(heading*4)+12*math.sin(heading*9+1))
            pygame.draw.rect(screen, (80, 125, 115), (x, cfg.horizon-height, 6, height))
        pygame.draw.rect(screen, HERBE, (0, cfg.horizon, cfg.width, cfg.height-cfg.horizon))
        tile = 300
        gi, gj = int((camera.x+camera.cs*cfg.camera_distance)//tile), int((camera.y+camera.sn*cfg.camera_distance)//tile)
        squares = []
        for i in range(gi-6, gi+7):
            for j in range(gj-6, gj+7):
                if (i+j) % 2 == 0:
                    points = [camera.ground(i*tile, j*tile), camera.ground(i*tile+tile, j*tile),
                              camera.ground(i*tile+tile,j*tile+tile), camera.ground(i*tile,j*tile+tile)]
                    squares.append((sum(p[0] for p in points), points))
        squares.sort(key=lambda item: item[0], reverse=True)
        for _, points in squares:
            polygon(screen, camera, HERBE2, points)

    def draw_track(self, screen: pygame.Surface, camera: Camera, track: Track) -> None:
        cfg = camera.cfg
        visible = {}
        for i, (x,y) in enumerate(track.center):
            forward = (x-camera.x)*camera.cs+(y-camera.y)*camera.sn
            if -250 < forward < cfg.far:
                visible[i] = forward
        cache = {}
        pit_cache = {}
        for i in sorted(visible, key=visible.get, reverse=True):
            j = (i+1) % track.count
            if j not in visible:
                continue
            if i not in cache:
                cache[i] = [camera.ground(*line[i]) for line in track.lines]
            if j not in cache:
                cache[j] = [camera.ground(*line[j]) for line in track.lines]
            a, b = cache[i], cache[j]
            kerb_color = ROUGE if (i//3) % 2 == 0 else BLANC
            polygon(screen,camera,kerb_color,[a[0],a[1],b[1],b[0]])
            polygon(screen,camera,kerb_color,[a[2],a[3],b[3],b[2]])
            polygon(screen,camera,ASPHALTE if (i//6)%2 == 0 else ASPHALTE2,
                    [a[1],a[2],b[2],b[1]])
            if (i//3)%2 == 0:
                polygon(screen,camera,BLANC,[a[4],a[5],b[5],b[4]])
            if i in track.pit_set and j in track.pit_set:
                for q in (i,j):
                    if q not in pit_cache:
                        pit_cache[q] = [camera.ground(*line[q]) for line in track.pit_lines]
                pa,pb = pit_cache[i],pit_cache[j]
                polygon(screen,camera,(100,100,108),[pa[0],pa[2],pb[2],pb[0]])
                polygon(screen,camera,(255,210,0) if (i//4)%2 == 0 else NOIR,
                        [pa[1],pa[2],pb[2],pb[1]])

    def draw_tree(self, screen: pygame.Surface, camera: Camera, forward: float,
                  right: float, tree: Tree) -> None:
        cfg = camera.cfg
        scale = cfg.focal/forward*tree.scale
        sx,sy = cfg.width/2+right/forward*cfg.focal, cfg.horizon+cfg.camera_height/forward*cfg.focal
        if sx < -150 or sx > cfg.width+150:
            return
        pygame.draw.rect(screen,(95,60,30),(sx-5*scale,sy-35*scale,10*scale,35*scale))
        if tree.kind == 0:
            pygame.draw.polygon(screen,(20,90,40),[(sx-40*scale,sy-25*scale),(sx+40*scale,sy-25*scale),(sx,sy-110*scale)])
            pygame.draw.polygon(screen,(30,110,50),[(sx-30*scale,sy-70*scale),(sx+30*scale,sy-70*scale),(sx,sy-150*scale)])
        else:
            pygame.draw.circle(screen,(50,140,50),(sx,sy-85*scale),45*scale)
            pygame.draw.circle(screen,(70,165,60),(sx-12*scale,sy-95*scale),25*scale)

    def draw_garage(self, screen: pygame.Surface, camera: Camera, g: Garage) -> None:
        a0,a1,b0,b1,door = g.a0,g.a1,g.b0,g.b1,g.door_color
        height = 55
        polygon(screen,camera,(60,60,70),[camera.space(*a0,height),camera.space(*a1,height),
                                        camera.space(*b1,height),camera.space(*b0,height)])
        polygon(screen,camera,(205,205,210),[camera.space(*a0,0),camera.space(*a1,0),
                                           camera.space(*a1,height),camera.space(*a0,height)])
        def mid(a,b,t):
            return (a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t)
        d0,d1=mid(a0,a1,0.15),mid(a0,a1,0.85)
        polygon(screen,camera,door,[camera.space(*d0,0),camera.space(*d1,0),
                                    camera.space(*d1,38),camera.space(*d0,38)])

    def draw_gantry(self, screen: pygame.Surface, camera: Camera, track: Track) -> None:
        cfg = camera.cfg
        pa,pb=track.lines[0][0],track.lines[3][0]
        (fa,ra),(fb,rb)=camera.position(*pa),camera.position(*pb)
        if min(fa,fb)<cfg.near:
            return
        xa,xb=cfg.width/2+ra/fa*cfg.focal,cfg.width/2+rb/fb*cfg.focal
        sa,sb=cfg.horizon+cfg.camera_height/fa*cfg.focal,cfg.horizon+cfg.camera_height/fb*cfg.focal
        ha,hb=cfg.horizon+(cfg.camera_height-170)/fa*cfg.focal,cfg.horizon+(cfg.camera_height-170)/fb*cfg.focal
        ea,eb=45*cfg.focal/fa,45*cfg.focal/fb
        pygame.draw.line(screen,(200,200,200),(xa,sa),(xa,ha),max(2,int(9*cfg.focal/fa)))
        pygame.draw.line(screen,(200,200,200),(xb,sb),(xb,hb),max(2,int(9*cfg.focal/fb)))
        for k in range(14):
            t0,t1=k/14,(k+1)/14
            x0,x1=xa+(xb-xa)*t0,xa+(xb-xa)*t1
            y0,y1=ha+(hb-ha)*t0,ha+(hb-ha)*t1
            e0,e1=ea+(eb-ea)*t0,ea+(eb-ea)*t1
            pygame.draw.polygon(screen,BLANC if k%2 == 0 else NOIR,
                                [(x0,y0),(x1,y1),(x1,y1+e1),(x0,y0+e0)])
