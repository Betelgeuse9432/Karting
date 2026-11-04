"""Caméra V6 à relief et zoom dynamique, sans dépendance à Pygame."""
import math
from ..camera import Camera
from .track import height


class V6Camera(Camera):
    def __init__(self, config, profile, angle=0.0):
        super().__init__(config, angle)
        self.profile=profile
        self.height_above_ground=profile["camh"]
        self.distance=profile["camdist"]
        self.z=self.height_above_ground
        self.focal=config.focal
        self.far=config.far
        self.shake=0.0

    def set_profile(self, profile):
        self.profile=profile
        self.height_above_ground=profile["camh"]
        self.distance=profile["camdist"]

    def follow(self, car, dt, intensity=0.0, quality=1, rain=0.0):
        delta=(car.angle-self.angle+math.pi)%(2*math.pi)-math.pi
        self.angle += delta*min(1,3.5*dt)
        self.cs,self.sn=math.cos(self.angle),math.sin(self.angle)
        self.x=car.x-self.cs*self.distance
        self.y=car.y-self.sn*self.distance
        self.z += (height(car.x, car.y)+self.height_above_ground-self.z)*min(1,4*dt)
        self.focal=self.cfg.focal*(1-0.1*(abs(car.v)/self.profile["vmax"])**1.5)
        self.far=(1500,2100,2600)[quality]
        self.shake += (intensity-self.shake)*min(1,10*dt)

    def ground(self,x,y):
        return self.space(x,y,height(x,y))

    def project(self,points):
        result=[]
        near=self.cfg.near
        n=len(points)
        for i in range(n):
            a,b=points[i],points[(i+1)%n]
            aok,bok=a[0]>=near,b[0]>=near
            if aok: result.append(a)
            if aok!=bok:
                t=(near-a[0])/(b[0]-a[0])
                result.append((near,a[1]+(b[1]-a[1])*t,a[2]+(b[2]-a[2])*t))
        if len(result)<3:return None
        w,h,hor=self.cfg.width,self.cfg.height,self.cfg.horizon
        return [(max(-20000,min(20000,w/2+r/f*self.focal)),
                 max(-20000,min(20000,hor+(self.z-z)/f*self.focal)))
                for f,r,z in result]
