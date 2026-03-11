// ============================================================
// Sesame Custom Block — Parametric design for FDM printing (no supports)
// ============================================================

// === PARAMETERS ===

/* [Main Dimensions] */
body_width  = 74;       // [10:200] Width (X) in mm
body_depth  = 38;       // [10:200] Depth (Y) in mm
body_height = 36;       // [10:200] Height (Z) in mm

/* [Print Settings] */
chamfer = 0.8;          // [0:0.1:2.0] Bottom chamfer for elephant foot

/* [Resolution] */
$fn = $preview ? 32 : 96;

/* [Hidden] */
epsilon = 0.01;

// === MODULES ===
module block() {
    if (chamfer > 0) {
        hull() {
            translate([chamfer, chamfer, 0])
                cube([body_width - 2*chamfer, body_depth - 2*chamfer, body_height]);
            translate([0, 0, chamfer])
                cube([body_width, body_depth, body_height - chamfer]);
        }
    } else {
        cube([body_width, body_depth, body_height]);
    }
}

module assembly() {
    block();
}

// === RENDER ===
assembly();
