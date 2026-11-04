"""Particules V6 : hauteur terrain, marque des roues, mise à jour facultative."""
from ..effects import Effects
from .track import height


class V6Effects(Effects):
    def emit(self,car,kind,lx,ly,lz,vx,vy,vz,lifetime,size):
        count=len(self.particles)
        super().emit(car,kind,lx,ly,lz,vx,vy,vz,lifetime,size)
        if len(self.particles)>count:
            self.particles[-1].z += height(car.x,car.y)

    def update_particles(self,dt):
        for particle in self.particles:
            particle.x+=particle.vx*dt
            particle.y+=particle.vy*dt
            particle.z+=particle.vz*dt
            particle.lifetime-=dt
            local_ground=height(particle.x,particle.y)
            if particle.kind=="etincelle":
                particle.vz-=500*dt
                if particle.z<local_ground:
                    particle.z=local_ground
                    particle.vz=-particle.vz*0.3
            elif particle.kind=="fumee":
                particle.size+=18*dt
        self.particles[:]=[p for p in self.particles if p.lifetime>0]

    def step(self,car,info,dt,model):
        """La géométrie des quatre roues et des échappements vient du véhicule choisi."""
        self.frame+=1
        rng=self.rng
        v=car.v
        if car.brake>0.3 and car.heat>0.3 and v>120:
            for cx,cy,front,radius,_ in model["roues"]:
                if not front:continue
                for _ in range(2):
                    self.emit(car,"etincelle",cx,cy,radius*0.8,
                              -rng.uniform(40,160),rng.uniform(-90,90),
                              rng.uniform(40,140),rng.uniform(0.25,0.6),1)
        flames=((not car.gas and info.previous_gas>0.35 and v>250)
                or (car.brake>0.5 and v>350 and rng.random()<0.12))
        if flames:
            for ex,ey,ez in model["echap"]:
                for _ in range(3):
                    self.emit(car,"flamme",ex-2,ey,ez,-rng.uniform(60,180),
                              rng.uniform(-20,20),rng.uniform(-10,30),
                              rng.uniform(0.12,0.28),rng.uniform(5,9))
        hard=info.hard_braking
        slipping=(info.rear_slip or info.front_slip or hard or info.drifting) and not info.on_grass
        if slipping and self.frame%2==0:
            for cx,cy,front,_,_ in model["roues"]:
                if (front and (info.front_slip or hard)) or (
                    not front and (info.rear_slip or info.drifting)):
                    self.emit(car,"fumee",cx,cy,3,-rng.uniform(0,30),
                              rng.uniform(-20,20),rng.uniform(15,40),
                              rng.uniform(0.6,1.2),rng.uniform(7,11))
        for i,(cx,cy,_,_,_) in enumerate(model["roues"]):
            wx,wy,_=car.local_to_world(cx,cy)
            if slipping:
                previous=self.previous_wheels[i]
                if previous:self.marks.append((previous[0],previous[1],wx,wy))
                self.previous_wheels[i]=(wx,wy)
            else:
                self.previous_wheels[i]=None
        self.update_particles(dt)
