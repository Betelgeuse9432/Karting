"""Adaptateur clavier Pygame -> commandes neutres du moteur de simulation."""
import pygame
from .state import Controls


def read_controls(keys: pygame.key.ScancodeWrapper) -> Controls:
    gas = bool(keys[pygame.K_UP] or keys[pygame.K_z] or keys[pygame.K_w])
    brake = bool(keys[pygame.K_DOWN] or keys[pygame.K_s])
    steer = int(bool(keys[pygame.K_RIGHT] or keys[pygame.K_d])) - int(bool(keys[pygame.K_LEFT] or keys[pygame.K_q] or keys[pygame.K_a]))
    return Controls(gas=gas, brake=brake, steering=steer, handbrake=bool(keys[pygame.K_SPACE]))
