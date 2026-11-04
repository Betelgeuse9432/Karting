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
