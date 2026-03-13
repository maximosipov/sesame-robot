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

/* [Right Wall Window] */
win_width  = 20;         // [0:1:50] Window width along Y
win_length = 14;         // [0:1:50] Window length along Z
win_top_offset = 12;     // [0:1:50] Offset from top of right wall

/* [Left Wall Cut] */
lcut_width  = 8;          // [0:1:50] Cut width along Y
lcut_height = 4;          // [0:1:50] Cut height along Z

/* [Right Wall Cut] */
cut_width   = 12;        // [0:1:50] Cut width along Y
cut_height  = 6;         // [0:1:50] Cut height along Z

/* [Front/Rear Wall Cuts] */
fr_cut_width  = 16;      // [0:1:50] Cut width along X
fr_cut_height = 10;      // [0:1:50] Cut height along Z

/* [Mounting Posts] */
mount_post_size  = 4;     // [2:1:10] Post width/depth
mount_post_thick = 2;     // [1:0.5:10] Post thickness (hangs from ceiling)
mount_hole_dia   = 2;     // [0.5:0.1:5] Mounting hole diameter
mount_hole_depth = 2;     // [1:0.5:10] Mounting hole depth
mount_dx = 47;            // [10:1:100] Distance between holes along X
mount_dy = 22;            // [10:1:50] Distance between holes along Y

/* [Bottom Posts] */
bpost_size    = 4;        // [2:1:10] Post width/depth
bpost_height  = 4;        // [1:0.5:20] Post height
bpost_chamfer = 1;        // [0:0.5:3] Chamfer on post corners
bpost_hole_dia = 2;       // [0.5:0.1:5] Hole diameter
bpost_hole_depth = 2;     // [1:0.5:10] Hole depth
bpost_dx = 64;            // [10:1:100] Distance between holes along X
bpost_dy = 32;            // [10:1:50] Distance between holes along Y

/* [Left Pyramid Extension] */
ext_width    = 34;        // [0:1:100] Total width along Y (body_depth - 2*outer_chamfer)
ext_height   = 8;         // [0:1:50] Rounded block height along Z
pyr_depth    = 9.5;       // [1:0.1:30] Pyramid depth (protrusion in -X, base 75% of body_height)

/* [Hidden] */
epsilon = 0.01;
cc = 1;                   // Edge chamfer for cuts and posts
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

module right_wall_window() {
    wy = (body_depth - win_width) / 2;
    wz = body_height - win_top_offset - win_length;
    wx = body_width - wall;
    // Outer bevel
    hull() {
        translate([wx + wall, wy - cc, wz - cc])
            cube([epsilon, win_width + 2*cc, win_length + 2*cc]);
        translate([wx + wall - cc, wy, wz])
            cube([epsilon, win_width, win_length]);
    }
    // Inner bevel
    hull() {
        translate([wx + cc, wy, wz])
            cube([epsilon, win_width, win_length]);
        translate([wx, wy - cc, wz - cc])
            cube([epsilon, win_width + 2*cc, win_length + 2*cc]);
    }
}

module left_wall_cut() {
    y0 = (body_depth - lcut_width) / 2;
    translate([-epsilon, y0, -1])
        cube([wall + 2*epsilon, lcut_width, lcut_height + 1]);
}

module right_wall_cut() {
    y0 = (body_depth - cut_width) / 2;
    rx = body_width - wall;
    translate([rx - epsilon, y0, -1])
        cube([wall + 2*epsilon, cut_width, cut_height + 1]);
}

module front_rear_cuts() {
    x0 = (body_width - fr_cut_width) / 2;
    // Front wall
    translate([x0, -epsilon, -1])
        cube([fr_cut_width, wall + 2*epsilon, fr_cut_height + 1]);
    // Rear wall
    translate([x0, body_depth - wall - epsilon, -1])
        cube([fr_cut_width, wall + 2*epsilon, fr_cut_height + 1]);
}

module left_pyramid_extension() {
    cz = body_height - oc - (ext_height + 2*pyr_depth)/2;  // Top slope continues top chamfer
    y0 = (body_depth - ext_width) / 2;
    // 45-degree top and bottom walls: height grows by 2*depth at the base
    h_tip  = ext_height;
    h_base = ext_height + 2 * pyr_depth;

    // Outer pyramid shell
    hull() {
        // Tip face (x = -pyr_depth)
        translate([-pyr_depth, y0, cz - h_tip/2])
            cube([epsilon, ext_width, h_tip]);
        // Base face at left wall (x = 0)
        translate([0, y0, cz - h_base/2])
            cube([epsilon, ext_width, h_base]);
    }

    // Rounded block at center of pyramid tip
    ext_r = ext_height / 2;
    ext_y_front = (body_depth - ext_width) / 2 + ext_r;
    ext_y_rear  = (body_depth + ext_width) / 2 - ext_r;
    hull() {
        translate([-pyr_depth, ext_y_front, cz])
            rotate([0, 90, 0])
            cylinder(h=wall, r=ext_r);
        translate([-pyr_depth, ext_y_rear, cz])
            rotate([0, 90, 0])
            cylinder(h=wall, r=ext_r);
    }
}

module left_pyramid_cavity() {
    cz = body_height - oc - (ext_height + 2*pyr_depth)/2;
    y0 = (body_depth - ext_width) / 2;
    h_tip  = ext_height - 2*wall;
    h_base = ext_height + 2*pyr_depth - 2*wall;

    // Inner cavity — extends into main box so it connects
    hull() {
        translate([-pyr_depth + wall, y0 + wall, cz - h_tip/2])
            cube([epsilon, ext_width - 2*wall, h_tip]);
        translate([wall, y0 + wall, cz - h_base/2])
            cube([epsilon, ext_width - 2*wall, h_base]);
    }
}

module mounting_posts() {
    cx = body_width / 2;
    cy = body_depth / 2;
    ceil_z = body_height - wall;
    ps = mount_post_size;
    for (dx = [-mount_dx/2, mount_dx/2])
        for (dy = [-mount_dy/2, mount_dy/2])
            translate([cx + dx - ps/2, cy + dy - ps/2, 0])
            hull() {
                // Bottom face: inset by cc
                translate([cc, cc, ceil_z - mount_post_thick])
                    cube([ps - 2*cc, ps - 2*cc, epsilon]);
                // cc above bottom: full size
                translate([0, 0, ceil_z - mount_post_thick + cc])
                    cube([ps, ps, epsilon]);
                // Top (attached to ceiling)
                translate([0, 0, ceil_z - epsilon])
                    cube([ps, ps, epsilon]);
            }
}

module mounting_holes() {
    cx = body_width / 2;
    cy = body_depth / 2;
    ceil_z = body_height - wall;
    for (dx = [-mount_dx/2, mount_dx/2])
        for (dy = [-mount_dy/2, mount_dy/2])
            translate([cx + dx, cy + dy, ceil_z - mount_hole_depth - epsilon])
                cylinder(d=mount_hole_dia, h=mount_hole_depth + epsilon);
}

module bottom_posts() {
    cx = body_width / 2;
    cy = body_depth / 2;
    ps = bpost_size;
    for (dx = [-bpost_dx/2, bpost_dx/2])
        for (dy = [-bpost_dy/2, bpost_dy/2])
            translate([cx + dx - ps/2, cy + dy - ps/2, 0])
            hull() {
                // Bottom: full size with corner chamfers
                linear_extrude(epsilon)
                    chamfered_rect(ps, ps, bpost_chamfer);
                // Below top: full size
                translate([0, 0, bpost_height - cc])
                    linear_extrude(epsilon)
                    chamfered_rect(ps, ps, bpost_chamfer);
                // Top face: inset by cc
                translate([cc, cc, bpost_height])
                    linear_extrude(epsilon)
                    chamfered_rect(ps - 2*cc, ps - 2*cc, max(bpost_chamfer - cc, 0));
            }
}

module bottom_holes() {
    cx = body_width / 2;
    cy = body_depth / 2;
    for (dx = [-bpost_dx/2, bpost_dx/2])
        for (dy = [-bpost_dy/2, bpost_dy/2])
            translate([cx + dx, cy + dy, -1])
                cylinder(d=bpost_hole_dia, h=bpost_hole_depth + 1);
}

module assembly() {
    difference() {
        union() {
            block();
            left_pyramid_extension();
            mounting_posts();
            bottom_posts();
        }
        left_pyramid_cavity();
        right_wall_window();
        left_wall_cut();
        right_wall_cut();
        front_rear_cuts();
        mounting_holes();
        bottom_holes();
    }
}

// === RENDER ===
assembly();
