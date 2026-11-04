"""Terrain, asphalte, stands et rails de la V6. Aucune modification de simulation."""
import pygame
from .mesh import melange
from .track import height


class V6WorldRenderer:
    def __init__(self):
        # Le relief ne change jamais : conserver les altitudes des carreaux.
        self._tile_geometry = {}

    def polygon(self,screen,camera,colour,points,weather):
        projected=camera.project(points)
        if projected:
            # Ne pas envoyer à SDL des polygones entièrement hors écran.
            xs=[p[0] for p in projected]
            ys=[p[1] for p in projected]
            if (max(xs)<-64 or min(xs)>camera.cfg.width+64
                    or max(ys)<-64 or min(ys)>camera.cfg.height+64):
                return
            forward=sum(p[0] for p in points)/len(points)
            haze=max(0,min(1,forward/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            fog=weather.get("fog_color",(170,215,255))
            pygame.draw.polygon(screen,melange(colour,fog,haze),projected)

    def background(self,screen,camera,sky,weather,quality,time):
        high,low=sky.draw(screen,camera,weather,quality,time)
        weather["fog_color"]=low
        W,H=camera.cfg.width,camera.cfg.height
        horizon=camera.cfg.horizon
        for y in range(horizon,H,5):
            colour=melange(low,(44,156,46),((y-horizon)/(H-horizon))**0.5)
            pygame.draw.rect(screen,colour,(0,y,W,5))
        tile=450
        ix,jy=int((camera.x+camera.cs*camera.distance)//tile),int((camera.y+camera.sn*camera.distance)//tile)
        squares=[]
        half_fov=W/(2*camera.focal)
        for i in range(ix-5,ix+6):
            for j in range(jy-5,jy+6):
                if (i+j)%2==0:colour=(36,136,40)
                elif (i*5+j*3)%7==0:colour=(88,160,52)
                else:continue
                # Éliminer les tuiles derrière ou bien au-delà du champ visuel.
                forward,right=camera.position((i+0.5)*tile,(j+0.5)*tile)
                if not -tile*1.5<forward<camera.far+tile*2:
                    continue
                if abs(right)>max(forward,150)*half_fov+tile*2:
                    continue
                key=(i,j)
                vertices=self._tile_geometry.get(key)
                if vertices is None:
                    corners=((i*tile,j*tile),((i+1)*tile,j*tile),
                             ((i+1)*tile,(j+1)*tile),(i*tile,(j+1)*tile))
                    vertices=tuple((x,y,height(x,y)) for x,y in corners)
                    self._tile_geometry[key]=vertices
                points=[camera.space(*point) for point in vertices]
                squares.append((sum(p[0] for p in points),colour,points))
        squares.sort(key=lambda x:x[0],reverse=True)
        for _,colour,points in squares:self.polygon(screen,camera,colour,points,weather)

    def track(self,screen,camera,track,weather,quality):
        count=track.count
        visible={}
        for i,(x,y) in enumerate(track.center):
            forward=(x-camera.x)*camera.cs+(y-camera.y)*camera.sn
            if -250<forward<camera.far:visible[i]=forward
        origin=(min(visible,key=visible.get)+count//2)%count if visible else 0
        segments=[]
        p=0
        while p<count:
            i=(origin+p)%count
            forward=visible.get(i)
            if forward is None:
                p+=1
                continue
            # Niveau de détail piloté par distance, avec moins de géométrie
            # lointaine en mode économique.
            stride=(1 if forward<500 else 2 if forward<900 else
                    3 if forward<1400 else (4 if quality>=1 else 6))
            j=(origin+p+stride)%count
            if p+stride<count and j in visible:
                segments.append((i,j))
                p+=stride
            else:
                j=(i+1)%count
                if p+1<count and j in visible:segments.append((i,j))
                p+=1
        segments.sort(key=lambda pair:visible[pair[0]],reverse=True)
        cache,pit_cache={},{}
        rain=weather["pluie"]
        asp1=melange((80,80,85),(46,50,62),rain)
        asp2=melange((72,72,77),(40,44,56),rain)
        for i,j in segments:
            for q in (i,j):
                if q not in cache:
                    cache[q]=[camera.space(line[q][0],line[q][1],track.line_heights[k][q])
                              for k,line in enumerate(track.lines)]
            a,b=cache[i],cache[j]
            kerb=(220,30,30) if int(track.distances[i]/45)%2==0 else (240,240,240)
            self.polygon(screen,camera,kerb,[a[0],a[1],b[1],b[0]],weather)
            self.polygon(screen,camera,kerb,[a[2],a[3],b[3],b[2]],weather)
            self.polygon(screen,camera,asp1 if int(track.distances[i]/90)%2==0 else asp2,
                         [a[1],a[2],b[2],b[1]],weather)
            self.polygon(screen,camera,(35,36,39),[a[4],a[5],b[5],b[4]],weather)
            if i in track.pit_set and j in track.pit_set:
                for q in (i,j):
                    if q not in pit_cache:
                        pit_cache[q]=[camera.space(line[q][0],line[q][1],track.pit_line_heights[k][q])
                                      for k,line in enumerate(track.pit_lines)]
                pa,pb=pit_cache[i],pit_cache[j]
                self.polygon(screen,camera,melange((100,100,108),(60,64,76),rain),
                             [pa[0],pa[2],pb[2],pb[0]],weather)
                self.polygon(screen,camera,(255,210,0) if int(track.distances[i]/60)%2==0 else (25,25,25),
                             [pa[1],pa[2],pb[2],pb[1]],weather)
            if quality>=1 and visible[i]<1000:
                for side,idx in ((0,6),(1,7)):
                    if side==0 and (i in track.pit_set or j in track.pit_set):continue
                    x,y=a[idx],b[idx]
                    top_x=(x[0],x[1],x[2]+26)
                    top_y=(y[0],y[1],y[2]+26)
                    self.polygon(screen,camera,(190,194,205),[x,y,top_y,top_x],weather)
                    if quality>=2:
                        self.polygon(screen,camera,(95,100,112),
                                     [(x[0],x[1],x[2]+17),(y[0],y[1],y[2]+17),top_y,top_x],weather)
