"""Décors V6 préparés une fois, indépendants du rendu et de la boucle Pygame."""
import random
from ..scenery import Tree


class V6Scenery:
    CROWD = ((220,60,60),(60,90,220),(240,200,50),(240,240,240),
             (60,170,90),(230,130,40),(170,70,200))
    ADS = (((220,40,40),(255,255,255)),((30,80,200),(255,220,0)),
           ((20,20,24),(255,60,60)),((250,250,250),(30,80,200)),
           ((255,200,0),(20,20,24)),((30,150,80),(255,255,255)))

    def __init__(self, track, seed=7):
        rng=random.Random(seed)
        self.trees=[]
        w,h=track.config.world_size
        # Le chemin est libéré des objets dangereux, y compris près des stands.
        def accept(x,y,margin):
            if not (0<x<w and 0<y<h):return False
            info=track.info(x,y)
            return info.distance>track.mur+margin and not (
                info.index in track.pit_set and 0<info.offset<500)
        for i in range(0,track.count,4):
            for side in (-1,1):
                d=track.mur+40+rng.random()*420
                x=track.center[i][0]+track.normals[i][0]*d*side
                y=track.center[i][1]+track.normals[i][1]*d*side
                if accept(x,y,30):
                    self.trees.append(Tree(x,y,rng.randrange(2),0.8+rng.random()*0.7))
        for _ in range(900):
            x,y=rng.random()*w,rng.random()*h
            if accept(x,y,120):
                self.trees.append(Tree(x,y,rng.randrange(2),0.8+rng.random()*0.8))
        spacing=track.spacing
        self.stands=[]
        step=max(2,int(170/spacing))
        for i0 in range(int(40/spacing),int(2000/spacing),step):
            crowd=[[rng.choice(self.CROWD) for _ in range(3)] for _ in range(4)]
            self.stands.append((i0,i0+step-1,crowd))
        self.ads=[]
        length=max(3,int(260/spacing))
        for k,i0 in enumerate(range(int(1500/spacing),track.count-length,max(1,int(700/spacing)))):
            side=1 if k%2==0 else -1
            if side==1 and (i0 in track.pit_set or (i0+length) in track.pit_set):
                side=-1
            self.ads.append((i0,i0+length,side,self.ADS[k%len(self.ADS)]))
        self.lamps=[]
        for i in range(track.count-int(700/spacing),
                       track.count+int(3400/spacing),
                       max(1,int(380/spacing))):
            for side in (-1,1):
                if side==1 and i%track.count in track.pit_set:continue
                self.lamps.append(track.position(i%track.count,side*(track.mur+24)))
