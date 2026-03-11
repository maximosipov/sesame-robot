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

/* [Right Wall Cut] */
cut_width   = 12;        // [0:1:50] Cut width along Y
cut_height  = 6;         // [0:1:50] Cut height along Z
cut_chamfer = 1;         // [0:0.5:3] Chamfer on cut corners

/* [Front/Rear Wall Cuts] */
fr_cut_width  = 16;      // [0:1:50] Cut width along X
fr_cut_height = 10;      // [0:1:50] Cut height along Z
fr_cut_chamfer = 1;      // [0:0.5:3] Chamfer on cut corners

/* [Left Wall Extension] */
ext_width    = 44;        // [0:1:100] Total width along Y
ext_height   = 8;         // [0:1:50] Height along Z
ext_top_offset = 4;       // [0:1:20] Offset from top of left wall

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

module right_wall_cut() {
    cut_y = (body_depth - cut_width) / 2;
    translate([body_width - wall - 1, cut_y, -1])
        linear_extrude(cut_height + 1)
        chamfered_rect(wall + 2, cut_width, cut_chamfer);
}

module front_rear_cuts() {
    cut_x = (body_width - fr_cut_width) / 2;
    // Front wall (Y=0)
    translate([cut_x, -1, -1])
        linear_extrude(fr_cut_height + 1)
        chamfered_rect(fr_cut_width, wall + 2, fr_cut_chamfer);
    // Rear wall (Y=body_depth)
    translate([cut_x, body_depth - wall - 1, -1])
        linear_extrude(fr_cut_height + 1)
        chamfered_rect(fr_cut_width, wall + 2, fr_cut_chamfer);
}

module left_wall_extension() {
    ext_r = ext_height / 2;
    ext_z = body_height - ext_top_offset - ext_r;
    ext_y_front = (body_depth - ext_width) / 2 + ext_r;
    ext_y_rear  = (body_depth + ext_width) / 2 - ext_r;
    // Stadium shape in Y-Z plane, extruded along X (wall thickness)
    hull() {
        translate([0, ext_y_front, ext_z])
            rotate([0, 90, 0])
            cylinder(h=wall, r=ext_r);
        translate([0, ext_y_rear, ext_z])
            rotate([0, 90, 0])
            cylinder(h=wall, r=ext_r);
    }
}

module assembly() {
    difference() {
        union() {
            block();
            left_wall_extension();
        }
        top_window();
        right_wall_cut();
        front_rear_cuts();
    }
}

// === RENDER ===
assembly();
