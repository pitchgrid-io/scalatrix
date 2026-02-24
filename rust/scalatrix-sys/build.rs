use std::path::PathBuf;

fn main() {
    let manifest = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    // scalatrix root is ../../ relative to this crate
    let scalatrix_root = manifest.join("../..").canonicalize().unwrap();
    let include_dir = scalatrix_root.join("include");
    let src_dir = scalatrix_root.join("src");

    let cpp_files = [
        "affine_transform.cpp",
        "scale.cpp",
        "lattice.cpp",
        "params.cpp",
        "mos.cpp",
        "pitchset.cpp",
        "linear_solver.cpp",
        "label_calculator.cpp",
        "node.cpp",
        "spectrum.cpp",
        "consonance.cpp",
        "c_api.cpp",
    ];

    let mut build = cc::Build::new();
    build
        .cpp(true)
        .std("c++17")
        .include(&include_dir)
;

    for file in &cpp_files {
        build.file(src_dir.join(file));
    }

    // Platform-specific flags
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    if target_os == "macos" {
        build.flag("-stdlib=libc++");
        println!("cargo:rustc-link-lib=c++");
    } else if target_os == "linux" {
        println!("cargo:rustc-link-lib=stdc++");
    }

    build.compile("scalatrix");

    // Rerun if any source changes
    println!("cargo:rerun-if-changed={}", src_dir.display());
    println!("cargo:rerun-if-changed={}", include_dir.display());
}
