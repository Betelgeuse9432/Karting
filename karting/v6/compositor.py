"""Composition V6 par profondeur : chaque type d'objet est isolable."""
from ..faults import FaultManager
from .track import height


class V6Compositor:
    def __init__(self,decor,vehicle,particles):
        self.decor=decor
        self.vehicle=vehicle
        self.particles=particles

    def draw(self,screen,camera,session):
        track,weather=session.track,session.weather
        quality=session.quality
        faults=session.faults
        objects=[(camera.position(session.car.x,session.car.y)[0],"vehicle",None)]
        half_width=camera.cfg.width/(2*camera.focal)

        def visible(x,y,margin=150,extra_far=0):
            forward,right=camera.position(x,y)
            if not camera.cfg.near<forward<camera.far+extra_far:
                return None
            if abs(right)>max(forward,camera.cfg.near)*half_width+margin:
                return None
            return forward
        if "render.trees" not in faults.disabled:
            for tree in session.scenery.trees:
                f=visible(tree.x,tree.y,200)
                if f is not None:objects.append((f,"tree",tree))
        if "render.garages" not in faults.disabled:
            for garage in track.garages:
                f=visible(*track.center[garage.track_index],margin=450,extra_far=300)
                if f is not None:objects.append((f,"garage",garage))
        if quality>=1:
            if "render.stands" not in faults.disabled:
                for stand in session.scenery.stands:
                    i=(stand[0]+stand[1])//2%track.count
                    f=visible(*track.center[i],margin=400)
                    if f is not None:objects.append((f,"stand",stand))
            if "render.ads" not in faults.disabled:
                for ad in session.scenery.ads:
                    i=(ad[0]+ad[1])//2%track.count
                    f=visible(*track.center[i],margin=350)
                    if f is not None:objects.append((f,"ad",ad))
            if "render.lamps" not in faults.disabled:
                for x,y in session.scenery.lamps:
                    f=visible(x,y,200)
                    if f is not None:objects.append((f,"lamp",(x,y)))
        if "render.particles" not in faults.disabled:
            for particle in session.effects.particles:
                f=visible(particle.x,particle.y,150)
                if f is not None:objects.append((f,"particle",particle))
        f=visible(*track.center[0],margin=350)
        if f is not None:objects.append((f,"gantry",None))
        objects.sort(key=lambda row:row[0],reverse=True)
        def decor(method,*args):
            return method(screen,camera,track,weather,quality,*args)
        for forward,kind,data in objects:
            if kind=="vehicle":
                faults.optional("render.vehicle",
                    lambda:self.vehicle.draw(screen,camera,session.car,
                        height(session.car.x,session.car.y),weather["nuit"],
                        weather.get("fog_color",(170,215,255)),
                        camera.far*(1-0.45*weather["pluie"])))
            elif kind=="tree":
                faults.optional("render.trees",lambda:decor(self.decor.draw_tree,data))
            elif kind=="garage":
                faults.optional("render.garages",lambda:decor(self.decor.draw_garage,data))
            elif kind=="stand":
                faults.optional("render.stands",lambda:decor(self.decor.draw_tribune,data))
            elif kind=="ad":
                faults.optional("render.ads",lambda:decor(self.decor.draw_ad,data))
            elif kind=="lamp":
                faults.optional("render.lamps",
                    lambda:decor(self.decor.draw_lamp,data[0],data[1],weather["nuit"]))
            elif kind=="particle":
                faults.optional("render.particles",
                    lambda:self.particles.draw_particle(screen,camera,forward,data,weather))
            elif kind=="gantry":
                faults.optional("render.gantry",lambda:decor(self.decor.draw_gantry))
