"""Décors géométriques, déterministes mais séparés du circuit logique."""
from dataclasses import dataclass
import random
from .track import Track


@dataclass(frozen=True, slots=True)
class Tree:
    x: float
    y: float
    kind: int
    scale: float


def generate_trees(track: Track, seed: int) -> list[Tree]:
    rng = random.Random(seed)
    trees = []
    cfg = track.config
    w, h = cfg.world_size
    for i in range(0, track.count, 5):
        for side in (1, -1):
            d = cfg.half_track_width + cfg.kerb + 60 + rng.random()*300
            x, y = (track.center[i][0] + track.normals[i][0]*d*side,
                    track.center[i][1] + track.normals[i][1]*d*side)
            if 0 < x < w and 0 < y < h:
                info = track.info(x, y)
                if info.distance > cfg.half_track_width+cfg.kerb+50 and not (info.index in track.pit_set and 0 < info.offset < 450):
                    trees.append(Tree(x, y, rng.randint(0, 1), 0.8+rng.random()*0.6))
    for _ in range(220):
        x, y = rng.random()*w, rng.random()*h
        info = track.info(x, y)
        if info.distance > cfg.half_track_width+cfg.kerb+80 and not (info.index in track.pit_set and 0 < info.offset < 450):
            trees.append(Tree(x, y, rng.randint(0, 1), 0.8+rng.random()*0.6))
    return trees
