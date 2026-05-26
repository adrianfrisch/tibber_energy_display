# 3D Printable Enclosure — CYD Electricity Price Dashboard

## Overview

A two-part snap-fit enclosure for the Cheap Yellow Display (ESP32-2432S028R), designed for shelf placement with an angled viewing position.

```
        ┌──────────────────────────┐
        │   ┌──────────────────┐   │
        │   │                  │   │ ← Display window
        │   │   TFT Display    │   │
        │   │   (57 × 43 mm)   │   │
        │   │                  │   │
        │   └──────────────────┘   │
        │         FRONT BEZEL      │
        └──────────────────────────┘
                   ╱    ╲
                  ╱      ╲  ← 15° shelf angle
        ─────────────────────────── shelf surface
```

## Features

- **Two-part snap-fit design** — no screws needed
- **15° angled stand** for comfortable shelf viewing
- **USB port cutout** on the top edge — accessible for power
- **SD card slot cutout** on the bottom edge
- **Ventilation slots** on the back panel
- **Rounded corners** for a clean finish
- **Parametric design** — all dimensions easily adjustable in OpenSCAD

## Files

| File | Description |
|------|-------------|
| `enclosure.scad` | Parametric OpenSCAD source file |

## How to Generate STL Files

1. **Install [OpenSCAD](https://openscad.org/)** (free, cross-platform)

2. **Open** `enclosure.scad` in OpenSCAD

3. **Export the back shell:**
   - Change line 19 to: `PART = "back";`
   - Press **F6** (Render)
   - File → Export as STL → `back_shell.stl`

4. **Export the front bezel:**
   - Change line 19 to: `PART = "front";`
   - Press **F6** (Render)
   - File → Export as STL → `front_bezel.stl`

Or via command line:
```bash
openscad -D 'PART="back"'  -o back_shell.stl  enclosure.scad
openscad -D 'PART="front"' -o front_bezel.stl enclosure.scad
```

## Print Settings

| Setting | Value |
|---------|-------|
| Material | PLA or PETG |
| Layer height | 0.2 mm |
| Infill | 20% |
| Supports | Not needed |
| Walls/Perimeters | 3 |
| Top/Bottom layers | 4 |
| Orientation | Print both parts face-down (as exported) |

Estimated print time: ~2–3 hours total (both parts).

## Assembly

1. Insert the CYD board into the **back shell**, PCB resting on the support posts, display facing up
2. Route the USB cable through the top cutout
3. Press the **front bezel** onto the back shell — snap clips will engage
4. Place on shelf — the integrated stand provides a 15° viewing angle

## Customization

All dimensions are parametric. Key variables to adjust in `enclosure.scad`:

| Variable | Default | Description |
|----------|---------|-------------|
| `wall` | 2.0 mm | Wall thickness |
| `tol` | 0.3 mm | PCB clearance (increase if fit is tight) |
| `stand_angle` | 15° | Shelf viewing angle |
| `corner_r` | 3.0 mm | Corner rounding radius |
| `lip` | 1.2 mm | Bezel overlap on display edge |

## Board Dimensions Reference (ESP32-2432S028R)

```
    ╔═══════════════════════════════════════╗
    ║  USB                                  ║ ← 85.6 mm
    ║  ┌─┐                                  ║
    ╠══╧═╤══════════════════════════════════╣
    ║    │  ┌───────────────────────────┐   ║
    ║    │  │                           │   ║
    ║    │  │     2.8" TFT Display      │   ║ ← 50.2 mm
    ║    │  │      (57 × 43 mm)         │   ║
    ║    │  │                           │   ║
    ║    │  └───────────────────────────┘   ║
    ╠════╧══════════╤══╤════════════════════╣
    ║               │SD│                    ║
    ╚═══════════════╧══╧════════════════════╝
```

