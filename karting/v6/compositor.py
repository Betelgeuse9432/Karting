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
        if "render.trees" not in faults.disabled:
            for tree in session.scenery.trees:
                f=camera.position(tree.x,tree.y)[0]
                if camera.cfg.near<f<camera.far:objects.append((f,"tree",tree))
        if "render.garages" not in faults.disabled:
            for garage in track.garages:
                f=camera.position(*track.center[garage.track_index])[0]
                if camera.cfg.near<f<camera.far+300:objects.append((f,"garage",garage))
        if quality>=1:
            if "render.stands" not in faults.disabled:
                for stand in session.scenery.stands:
                    i=(stand[0]+stand[1])//2%track.count
                    f=camera.position(*track.center[i])[0]
                    if camera.cfg.near<f<camera.far:objects.append((f,"stand",stand))
            if "render.ads" not in faults.disabled:
                for ad in session.scenery.ads:
                    i=(ad[0]+ad[1])//2%track.count
                    f=camera.position(*track.center[i])[0]
                    if camera.cfg.near<f<camera.far:objects.append((f,"ad",ad))
            if "render.lamps" not in faults.disabled:
                for x,y in session.scenery.lamps:
                    f=camera.position(x,y)[0]
                    if camera.cfg.near<f<camera.far:objects.append((f,"lamp",(x,y)))
        if "render.particles" not in faults.disabled:
            for particle in session.effects.particles:
                f=camera.position(particle.x,particle.y)[0]
                if camera.cfg.near<f<camera.far:objects.append((f,"particle",particle))
        f=camera.position(*track.center[0])[0]
        if camera.cfg.near<f<camera.far:objects.append((f,"gantry",None))
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
