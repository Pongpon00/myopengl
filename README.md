# 🌌 Assignment 2 — 3D Kinetic Sculpture and Animation 
Create a 3D interactive or animation program. Focus on using the 3D transformation matrix to perform
animation or generate the vertices' positions. Try applying multiple light sources to your models. Use
the example from src/2.lighting/6.multiple_lights/multiple_lights.cpp as your starting code.

---

## 🪐 Topic: The Solar System Simulation

This project is a 3D interactive kinetic sculpture implemented with OpenGL (GLFW + GLAD + GLM).
It simulates a dynamic Solar System where planets orbit around the Sun.

## 🚀 Overview

The program simulates:
- A glowing **Sun** at the center (emissive light source)  
- Multiple **planets orbiting** around the Sun, each textured uniquely  
- **Orbit trails** dynamically drawn using OpenGL line strips  
- A **space-like gradient background**  
- Fully **interactive 3D camera controls**

## ✨ Features

| Feature | Description |
|----------|-------------|
| 🌀 Planetary Motion | Each planet rotates around the Sun with its own orbit speed and self-rotation |
| 🌞 Emissive Sun | The Sun emits its own light, illuminating other planets |
| 💡 Phong Lighting | Realistic per-fragment lighting with ambient, diffuse, and specular components |
| 🪐 Orbit Trails | Fading trails rendered via dynamic vertex buffers |
| 🎮 Camera Control | Move around freely with WASD + mouse look |

---

## 💫 Demo
[![Watch the video](https://img.youtube.com/vi/aS_2yh3SxeY/maxresdefault.jpg)](https://youtu.be/aS_2yh3SxeY)

## 🌠 Matetials
I use texture assets from the following sources
- Wikimedia Commons
- JHT's Planetary Pixel Emporium
- Solar System Scope
  
Big Thanks!💖
