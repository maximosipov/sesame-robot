// ============================================================
// Sesame Custom Block — Parametric design for FDM printing (no supports)
// ============================================================

// === PARAMETERS ===

/* [Main Dimensions] */
body_width  = 74;       // [10:200] Width (X) in mm
body_depth  = 38;       // [10:200] Depth (Y) in mm
body_height = 36;       // [10:200] Height (Z) in mm

/* [Wall] */
wall = 2;               // [0.4:0.1:10] Wall thickness in mm

/* [Edges] */
outer_chamfer = 2;       // [0:0.5:5] Chamfer on outer vertical edges
inner_chamfer = 2;       // [0:0.5:5] Chamfer on inner vertical edges

/* [Resolution] */
$fn = $preview ? 32 : 96;

/* [Top Window] */
win_width  = 20;         // [0:1:50] Window width along Y
win_length = 14;         // [0:1:50] Window length along X
win_side_offset = 1;     // [0:0.5:10] Offset from inner right wall

/* [Hidden] */
epsilon = 0.01;
oc = outer_chamfer;
ic = inner_chamfer;
iw = body_width  - 2*wall;
id = body_depth  - 2*wall;

// === MODULES ===

// 2D rectangle with 45-degree chamfered corners
module chamfered_rect(w, d, c) {
    polygon([
        [c, 0],   [w-c, 0],
        [w, c],   [w, d-c],
        [w-c, d], [c, d],
        [0, d-c], [0, c]
    ]);
}

module block() {
    difference() {
        // Outer shell: chamfered vertical + top edges
        hull() {
            linear_extrude(epsilon)
                chamfered_rect(body_width, body_depth, oc);
            translate([0, 0, body_height - oc])
                linear_extrude(epsilon)
                chamfered_rect(body_width, body_depth, oc);
            translate([oc, oc, body_height])
                linear_extrude(epsilon)
                chamfered_rect(body_width - 2*oc, body_depth - 2*oc, oc);
        }
        // Inner cavity: chamfered vertical + top edges, open bottom
        hull() {
            translate([wall, wall, -1])
                linear_extrude(epsilon)
                chamfered_rect(iw, id, ic);
            translate([wall, wall, body_height - wall - ic])
                linear_extrude(epsilon)
                chamfered_rect(iw, id, ic);
            translate([wall - ic, wall - ic, body_height - wall])
                linear_extrude(epsilon)
                chamfered_rect(iw + 2*ic, id + 2*ic, ic);
        }
    }
}

module top_window() {
    win_x = body_width - wall - win_side_offset - win_length;
    win_y = (body_depth - win_width) / 2;
    translate([win_x, win_y, body_height - wall - 1])
        cube([win_length, win_width, wall + 2]);
}

module assembly() {
    difference() {
        block();
        top_window();
    }
}

// === RENDER ===
assembly();
