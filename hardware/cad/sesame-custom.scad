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
mount_post_size  = 8;     // [2:1:10] Post width/depth
mount_post_thick = 2;     // [1:0.5:10] Post thickness (hangs from ceiling)
mount_post_chamfer = 2;   // [0.5:0.5:5] Chamfer on bottom edges of posts
mount_hole_dia   = 2;     // [0.5:0.1:5] Mounting hole diameter
mount_hole_depth = 2;     // [1:0.5:10] Mounting hole depth
mount_dx = 47;            // [10:1:100] Distance between holes along X
mount_dy = 22;            // [10:1:50] Distance between holes along Y

/* [Bottom Posts] */
bpost_size    = 4;        // [2:1:10] Post width/depth
bpost_height  = 8;        // [1:0.5:20] Post height
bpost_chamfer = 1;        // [0:0.5:3] Chamfer on post corners
bpost_hole_dia = 2;       // [0.5:0.1:5] Hole diameter
bpost_hole_depth = 2;     // [1:0.5:10] Hole depth
bpost_dx = 63;            // [10:1:100] Distance between holes along X
bpost_dy = 32;            // [10:1:50] Distance between holes along Y
bpost_left = 6;           // [1:0.5:20] Distance from left wall to hole center

/* [Left Pyramid Extension] */
ext_width    = 34;        // [0:1:100] Total width along Y (body_depth - 2*outer_chamfer)
ext_height   = 13;        // [0:1:50] Rounded block height along Z
pyr_depth    = 7;         // [1:0.1:30] Pyramid depth (protrusion in -X, base 75% of body_height)

/* [Pyramid Mounting Tabs] */
pmt_total_width = 44;     // [0:1:100] Total width including tabs along Y
pmt_hole_spacing = 37;    // [10:1:80] Distance between mounting holes along Y
pmt_tab_height  = 8;      // [1:0.5:20] Tab height along Z
pmt_hole_dia    = 2;      // [0.5:0.1:5] Mounting hole diameter
pmt_tab_radius  = 4;      // [0:0.5:10] Corner radius on tabs

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
        // Inner cavity: chamfered vertical edges, open bottom
        hull() {
            translate([wall, wall, -1])
                linear_extrude(epsilon)
                chamfered_rect(iw, id, ic);
            translate([wall, wall, body_height - wall])
                linear_extrude(epsilon)
                chamfered_rect(iw, id, ic);
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
                // Bottom face: inset by mount_post_chamfer
                translate([mount_post_chamfer, mount_post_chamfer, ceil_z - mount_post_thick])
                    cube([ps - 2*mount_post_chamfer, ps - 2*mount_post_chamfer, epsilon]);
                // Above bottom: full size
                translate([0, 0, ceil_z - mount_post_thick + mount_post_chamfer])
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
    cy = body_depth / 2;
    ps = bpost_size;
    inset = ps / 2;  // 45-degree Y inset at top
    // Left posts: extend from left wall (x=0) to hole center + ps/2
    for (dy = [-bpost_dy/2, bpost_dy/2]) {
        lx0 = 0;
        lx1 = bpost_left + ps/2;
        lw  = lx1 - lx0;
        translate([lx0, cy + dy, 0])
        hull() {
            translate([0, -ps/2, 0])
                cube([lw, ps, epsilon]);
            translate([0, -ps/2, bpost_height - inset])
                cube([lw, ps, epsilon]);
            translate([0, 0, bpost_height])
                cube([lw, epsilon, epsilon]);
        }
    }
    // Right posts: extend from hole center - ps/2 to right wall (x=body_width)
    for (dy = [-bpost_dy/2, bpost_dy/2]) {
        rx0 = bpost_left + bpost_dx - ps/2;
        rx1 = body_width;
        rw  = rx1 - rx0;
        translate([rx0, cy + dy, 0])
        hull() {
            translate([0, -ps/2, 0])
                cube([rw, ps, epsilon]);
            translate([0, -ps/2, bpost_height - inset])
                cube([rw, ps, epsilon]);
            translate([0, 0, bpost_height])
                cube([rw, epsilon, epsilon]);
        }
    }
}

module bottom_holes() {
    cx = bpost_left + bpost_dx / 2;
    cy = body_depth / 2;
    for (dx = [-bpost_dx/2, bpost_dx/2])
        for (dy = [-bpost_dy/2, bpost_dy/2])
            translate([cx + dx, cy + dy, -1])
                cylinder(d=bpost_hole_dia, h=bpost_hole_depth + 1);
}

module pyramid_mounting_tabs() {
    cz = body_height - oc - (ext_height + 2*pyr_depth)/2;
    tab_w = (pmt_total_width - ext_width) / 2;  // width of each tab
    y_front_inner = (body_depth - ext_width) / 2;
    y_rear_inner  = (body_depth + ext_width) / 2;
    tab_z = cz - pmt_tab_height / 2;
    r = pmt_tab_radius;
    // Front tab (extends in -Y)
    hull() {
        // Inner edge (straight, at pyramid face)
        translate([-pyr_depth, y_front_inner - epsilon, tab_z])
            cube([wall, epsilon, pmt_tab_height]);
        // Outer rounded corners
        translate([-pyr_depth, y_front_inner - tab_w + r, tab_z + r])
            rotate([0, 90, 0])
            cylinder(r=r, h=wall);
        translate([-pyr_depth, y_front_inner - tab_w + r, tab_z + pmt_tab_height - r])
            rotate([0, 90, 0])
            cylinder(r=r, h=wall);
    }
    // Rear tab (extends in +Y)
    hull() {
        // Inner edge (straight, at pyramid face)
        translate([-pyr_depth, y_rear_inner, tab_z])
            cube([wall, epsilon, pmt_tab_height]);
        // Outer rounded corners
        translate([-pyr_depth, y_rear_inner + tab_w - r, tab_z + r])
            rotate([0, 90, 0])
            cylinder(r=r, h=wall);
        translate([-pyr_depth, y_rear_inner + tab_w - r, tab_z + pmt_tab_height - r])
            rotate([0, 90, 0])
            cylinder(r=r, h=wall);
    }
}

module pyramid_mounting_holes() {
    cz = body_height - oc - (ext_height + 2*pyr_depth)/2;
    cy = body_depth / 2;
    for (dy = [-pmt_hole_spacing/2, pmt_hole_spacing/2])
        translate([-pyr_depth - epsilon, cy + dy, cz])
            rotate([0, 90, 0])
            cylinder(d=pmt_hole_dia, h=wall + 2*epsilon);
}

module assembly() {
    difference() {
        union() {
            block();
            left_pyramid_extension();
            pyramid_mounting_tabs();
            mounting_posts();
            bottom_posts();
        }
        left_pyramid_cavity();
        pyramid_mounting_holes();
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
