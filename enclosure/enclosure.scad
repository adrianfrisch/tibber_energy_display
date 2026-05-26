// ============================================================
// Electricity Price Dashboard - 3D Printable Enclosure
// For: Cheap Yellow Display (ESP32-2432S028R)
//
// Designed for shelf placement with angled viewing.
// USB port accessible from the back/bottom.
//
// Print settings:
//   - Layer height: 0.2mm
//   - Infill: 20%
//   - No supports needed
//   - Material: PLA or PETG
//
// Usage:
//   - Render in OpenSCAD (F6) then export as STL
//   - Print front_bezel() and back_shell() separately
//   - Set PART variable to render individual parts
// ============================================================

// === Which part to render ===
// "assembly" = both parts for preview
// "front"    = front bezel only (for STL export)
// "back"     = back shell only (for STL export)
PART = "assembly";

// === PCB Dimensions (CYD ESP32-2432S028R) ===
pcb_w       = 85.6;    // PCB width (mm)
pcb_h       = 50.2;    // PCB height (mm)
pcb_t       = 1.6;     // PCB thickness (mm)
comp_height = 10.0;    // Height of tallest component on back side
front_comp  = 4.0;     // Display module thickness above PCB

// === Display Window ===
// Visible area of the 2.8" ILI9341 TFT
disp_w      = 57.0;    // Display visible width
disp_h      = 43.0;    // Display visible height
disp_off_x  = 14.0;    // Display offset from left PCB edge
disp_off_y  = 3.5;     // Display offset from bottom PCB edge

// === USB Port (Micro-USB) ===
// Located on the top short edge of the PCB
usb_w       = 8.0;     // USB opening width
usb_h       = 4.0;     // USB opening height
usb_off_x   = 36.0;    // USB center offset from left edge of PCB

// === SD Card Slot ===
// Located on the bottom short edge
sd_w        = 14.0;    // SD slot opening width
sd_h        = 3.0;     // SD slot opening height
sd_off_x    = 34.0;    // SD slot center offset from left edge of PCB

// === Enclosure Parameters ===
wall        = 2.0;     // Wall thickness
tol         = 0.3;     // Tolerance / clearance around PCB
lip         = 1.2;     // Front bezel lip overlapping the display edge
corner_r    = 3.0;     // Corner radius
snap_w      = 8.0;     // Snap-fit clip width
snap_depth  = 0.6;     // Snap-fit clip engagement depth

// === Shelf Stand Angle ===
stand_angle = 15;       // Viewing angle from vertical (degrees)

// === Derived Dimensions ===
inner_w     = pcb_w + 2 * tol;
inner_h     = pcb_h + 2 * tol;
outer_w     = inner_w + 2 * wall;
outer_h     = inner_h + 2 * wall;

// Total depth: front cover + PCB + components + back wall
front_depth = front_comp + pcb_t + 1.0;  // Front bezel depth
back_depth  = comp_height + wall + 1.0;  // Back shell depth
total_depth = front_depth + back_depth;

// ============================================================
// Modules
// ============================================================

// Rounded rectangle helper
module rounded_rect(w, h, d, r) {
    hull() {
        for (x = [r, w-r]) {
            for (y = [r, h-r]) {
                translate([x, y, 0])
                    cylinder(h=d, r=r, $fn=32);
            }
        }
    }
}

// PCB ledge posts (supports the PCB from below inside the back shell)
module pcb_supports() {
    post_r = 2.0;
    post_h = comp_height - 1.0;  // Leave a small gap

    // Four corner posts, inset from PCB corners
    inset = 3.0;
    positions = [
        [wall + tol + inset,           wall + tol + inset],
        [wall + tol + pcb_w - inset,   wall + tol + inset],
        [wall + tol + inset,           wall + tol + pcb_h - inset],
        [wall + tol + pcb_w - inset,   wall + tol + pcb_h - inset]
    ];

    for (pos = positions) {
        translate([pos[0], pos[1], wall])
            cylinder(h=post_h, r=post_r, $fn=24);
    }
}

// ============================================================
// Back Shell
// ============================================================
module back_shell() {
    difference() {
        // Outer shell
        rounded_rect(outer_w, outer_h, back_depth, corner_r);

        // Inner cavity
        translate([wall, wall, wall])
            rounded_rect(inner_w, inner_h, back_depth, max(corner_r - wall, 0.5));

        // USB port cutout (on the top short edge)
        translate([wall + tol + usb_off_x - usb_w/2, outer_h - wall - 0.1, wall + comp_height - usb_h])
            cube([usb_w, wall + 0.2, usb_h + wall + 1]);

        // SD card slot cutout (on the bottom short edge)
        translate([wall + tol + sd_off_x - sd_w/2, -0.1, wall + comp_height - sd_h])
            cube([sd_w, wall + 0.2, sd_h + wall + 1]);

        // Ventilation slots on the back
        vent_count = 5;
        vent_w = 20;
        vent_h = 1.5;
        vent_spacing = 5;
        vent_start_x = (outer_w - (vent_count * vent_w + (vent_count-1) * 3)) / 2;
        for (i = [0:vent_count-1]) {
            for (j = [0:2]) {
                translate([vent_start_x + i * (vent_w + 3),
                          outer_h/2 - vent_spacing + j * vent_spacing,
                          -0.1])
                    rounded_rect(vent_w, vent_h, wall + 0.2, 0.5);
            }
        }
    }

    // PCB support posts
    pcb_supports();

    // Snap-fit receptacles (slots on inner walls for the bezel clips)
    // Left and right walls, centered vertically
    for (side = [0, 1]) {
        x_pos = side == 0 ? wall - 0.1 : outer_w - wall - snap_depth + 0.1;
        translate([x_pos, outer_h/2 - snap_w/2, back_depth - 2])
            cube([snap_depth, snap_w, 2.5]);
    }
}

// ============================================================
// Front Bezel
// ============================================================
module front_bezel() {
    difference() {
        // Outer frame
        rounded_rect(outer_w, outer_h, front_depth, corner_r);

        // Display window cutout (goes all the way through)
        translate([wall + tol + disp_off_x - lip,
                   wall + tol + disp_off_y - lip,
                   -0.1])
            rounded_rect(disp_w + 2*lip, disp_h + 2*lip, wall + 0.2, 1.5);

        // Inner pocket for PCB + display module (from the back side of bezel)
        translate([wall, wall, wall])
            rounded_rect(inner_w, inner_h, front_depth, max(corner_r - wall, 0.5));

        // Wider display cutout through the front face
        // (the actual visible window is smaller due to lip)
        translate([wall + tol + disp_off_x,
                   wall + tol + disp_off_y,
                   -0.1])
            rounded_rect(disp_w, disp_h, front_depth + 0.2, 1.0);
    }

    // Snap-fit clips on left and right (engage with back shell slots)
    for (side = [0, 1]) {
        x_pos = side == 0 ? wall * 0.5 : outer_w - wall * 0.5 - snap_depth;
        translate([x_pos, outer_h/2 - snap_w/2, front_depth - 0.1]) {
            // Clip with angled entry
            difference() {
                cube([snap_depth + 0.2, snap_w, 3.0]);
                // Angled chamfer for easy insertion
                translate([side == 0 ? -0.1 : snap_depth - 0.3, -0.1, 2.5])
                    rotate([0, side == 0 ? -30 : 30, 0])
                        cube([snap_depth + 0.5, snap_w + 0.2, 1.5]);
            }
        }
    }
}

// ============================================================
// Shelf Stand (integrated kick-stand on the back shell)
// ============================================================
module shelf_stand() {
    // Triangular support on the bottom of the back shell
    // Creates an angled viewing position when placed on a shelf

    stand_bottom = outer_h * sin(stand_angle) + back_depth * cos(stand_angle);
    stand_w = outer_w - 10;  // Slightly narrower than case

    translate([5, 0, 0]) {
        // Bottom foot/rail
        hull() {
            // Bottom edge (sits on shelf)
            cube([stand_w, 2, 2]);
            // Top edge (meets the case back)
            translate([0, 0, back_depth - 2])
                cube([stand_w, 0.5, 2]);
        }

        // Angled support triangles (left and right)
        for (x_off = [0, stand_w - 3]) {
            translate([x_off, 0, 0]) {
                hull() {
                    cube([3, 2, 2]);
                    translate([0, 0, back_depth - 2])
                        cube([3, 0.5, 2]);
                    translate([0, outer_h * sin(stand_angle) * 0.6, 0])
                        cube([3, 2, 2]);
                }
            }
        }
    }
}

// ============================================================
// Assembly / Part Selection
// ============================================================

module assembly() {
    color("DarkSlateGray", 0.8) {
        // Back shell (bottom part when printing)
        back_shell();

        // Integrated shelf stand on the back
        translate([0, -outer_h * sin(stand_angle) * 0.6 - 2, 0])
            shelf_stand();
    }

    // Front bezel, positioned to mate with back shell
    color("SlateGray", 0.7)
    translate([0, 0, back_depth])
        front_bezel();
}

module print_back() {
    // Back shell oriented for printing (open side up)
    back_shell();

    // Stand integrated
    translate([0, -outer_h * sin(stand_angle) * 0.6 - 2, 0])
        shelf_stand();
}

module print_front() {
    // Front bezel oriented for printing (display window side down)
    translate([0, 0, front_depth])
        mirror([0, 0, 1])
            front_bezel();
}

// Render the selected part
if (PART == "assembly") {
    assembly();
} else if (PART == "front") {
    print_front();
} else if (PART == "back") {
    print_back();
}

