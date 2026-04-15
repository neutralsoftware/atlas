use crate::{Commands, Config};
use colored::Colorize;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

const INFO_PLIST: &str = r#"<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>((APPNAME))</string>
    <key>CFBundleDisplayName</key>
    <string>((APPNAME))</string>
    <key>CFBundleIdentifier</key>
    <string>com.((IDENTIFIER)).((APPNAMELC))</string>
    <key>CFBundleVersion</key>
    <string>((VERSION))</string>
    <key>CFBundleExecutable</key>
    <string>((EXECUTABLE))</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon.icns</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
</dict>
</plist>
"#;

fn host_platform() -> &'static str {
    if cfg!(target_os = "windows") {
        "windows"
    } else if cfg!(target_os = "macos") {
        "macos"
    } else if cfg!(target_os = "linux") {
        "linux"
    } else {
        "unknown"
    }
}

fn parse_config() -> Result<Config, String> {
    let config_str =
        fs::read_to_string("atlas.toml").map_err(|e| format!("Failed to read atlas.toml: {e}"))?;
    toml::from_str(&config_str).map_err(|e| format!("Failed to parse atlas.toml: {e}"))
}

fn resolve_backend(config: &Config, override_backend: Option<String>) -> String {
    if let Some(b) = override_backend {
        return b.to_uppercase();
    }
    config
        .project
        .backend
        .clone()
        .unwrap_or_else(|| String::from("METAL"))
        .to_uppercase()
}

fn run_cmake(
    build_dir: &Path,
    release: bool,
    backend: &str,
    export_cc: bool,
) -> Result<(), String> {
    if !build_dir.exists() {
        fs::create_dir_all(build_dir)
            .map_err(|e| format!("Failed to create build directory: {e}"))?;
    }

    let mut configure = Command::new("cmake");
    configure
        .current_dir(build_dir)
        .arg("..")
        .arg("-G")
        .arg("Ninja")
        .arg(format!("-DATLAS_BACKEND={backend}"))
        .arg(if release {
            "-DCMAKE_BUILD_TYPE=Release"
        } else {
            "-DCMAKE_BUILD_TYPE=Debug"
        });

    if export_cc {
        configure.arg("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON");
    }

    let configure_output = configure
        .output()
        .map_err(|e| format!("Failed to execute cmake configure: {e}"))?;
    if !configure_output.status.success() {
        let stdout = String::from_utf8_lossy(&configure_output.stdout);
        let stderr = String::from_utf8_lossy(&configure_output.stderr);
        let mut message = format!(
            "cmake configure failed with status {}",
            configure_output.status
        );

        if !stdout.trim().is_empty() {
            message.push_str("\n\nstdout:\n");
            message.push_str(stdout.trim());
        }

        if !stderr.trim().is_empty() {
            message.push_str("\n\nstderr:\n");
            message.push_str(stderr.trim());
        }

        return Err(message);
    }

    let build_output = Command::new("cmake")
        .current_dir(build_dir)
        .arg("--build")
        .arg(".")
        .output()
        .map_err(|e| format!("Failed to execute cmake build: {e}"))?;

    if !build_output.status.success() {
        let stdout = String::from_utf8_lossy(&build_output.stdout);
        let stderr = String::from_utf8_lossy(&build_output.stderr);
        let mut message = format!("cmake build failed with status {}", build_output.status);

        if !stdout.trim().is_empty() {
            message.push_str("\n\nstdout:\n");
            message.push_str(stdout.trim());
        }

        if !stderr.trim().is_empty() {
            message.push_str("\n\nstderr:\n");
            message.push_str(stderr.trim());
        }

        return Err(message);
    }

    Ok(())
}

fn find_executable(build_dir: &Path) -> Option<PathBuf> {
    let bin = build_dir.join("bin");
    if !bin.exists() {
        return None;
    }

    let entries = fs::read_dir(&bin).ok()?;
    for entry in entries {
        let path = entry.ok()?.path();
        if path.is_file() {
            return Some(path);
        }
    }
    None
}

fn ensure_supported_platform(config: &Config) -> Result<(), String> {
    let current = host_platform();
    let supported = &config.pack.supported_platforms;

    if supported == "all" {
        return Ok(());
    }

    if supported
        .split(',')
        .any(|p| p.trim().eq_ignore_ascii_case(current))
    {
        return Ok(());
    }

    Err(format!(
        "Current platform '{current}' is not supported by atlas.toml"
    ))
}

fn build_internal(
    release: bool,
    backend_override: Option<String>,
    export_cc: bool,
) -> Result<(Config, String, PathBuf), String> {
    let config = parse_config()?;
    ensure_supported_platform(&config)?;
    let backend = resolve_backend(&config, backend_override);
    let build_dir = Path::new("build");

    run_cmake(build_dir, release, &backend, export_cc)?;

    let executable = find_executable(build_dir)
        .ok_or_else(|| String::from("No executable found in build/bin"))?;

    Ok((config, backend, executable))
}

pub fn build(cmd: Commands) {
    let (release, backend_override) = match cmd {
        Commands::Build { release, backend } => (release != 0, backend),
        _ => (false, None),
    };

    match build_internal(release, backend_override, false) {
        Ok((_config, backend, executable)) => {
            println!("{} {}", "Build backend:".cyan(), backend.bold().green());
            println!(
                "{} {}",
                "Built executable:".cyan(),
                executable.display().to_string().bold().green()
            );
        }
        Err(e) => {
            eprintln!("{}\n{e}", "Build failed".red().bold());
        }
    }
}

pub fn clangd(cmd: Commands) {
    let backend_override = match cmd {
        Commands::Clangd { backend } => backend,
        _ => None,
    };

    match build_internal(false, backend_override, true) {
        Ok((_config, backend, _executable)) => {
            let source = Path::new("build/compile_commands.json");
            let target = Path::new("compile_commands.json");
            if source.exists() {
                if target.exists() {
                    let _ = fs::remove_file(target);
                }
                #[cfg(unix)]
                {
                    use std::os::unix::fs::symlink;
                    let _ = symlink(source, target);
                }
                #[cfg(not(unix))]
                {
                    let _ = fs::copy(source, target);
                }
            }
            println!("{} {}", "Clangd backend:".cyan(), backend.bold().green());
            println!("{}", "Generated compile_commands.json".bold().green());
        }
        Err(e) => eprintln!("{}\n{e}", "atlas clangd failed".red().bold()),
    }
}

pub fn pack(cmd: Commands) {
    let (release, backend_override) = match cmd {
        Commands::Pack { release, backend } => (release != 0, backend),
        _ => (false, None),
    };

    let (config, backend, executable) = match build_internal(release, backend_override, false) {
        Ok(data) => data,
        Err(e) => {
            eprintln!("{}\n{e}", "atlas pack failed".red().bold());
            return;
        }
    };

    let host = host_platform();
    let app_name = config
        .project
        .app_name
        .clone()
        .unwrap_or_else(|| config.project.name.clone());

    let app_dir = Path::new("dist");
    if app_dir.exists() {
        let _ = fs::remove_dir_all(app_dir);
    }
    if fs::create_dir_all(app_dir).is_err() {
        eprintln!("{}", "Failed to create dist directory".red().bold());
        return;
    }

    if host == "macos" {
        let bundle_dir = app_dir.join(format!("{app_name}.app"));
        let contents_dir = bundle_dir.join("Contents");
        let macos_dir = contents_dir.join("MacOS");
        let resources_dir = contents_dir.join("Resources");

        if fs::create_dir_all(&macos_dir).is_err() || fs::create_dir_all(&resources_dir).is_err() {
            eprintln!("{}", "Failed to create app bundle directories".red().bold());
            return;
        }

        if fs::copy(&executable, macos_dir.join(&app_name)).is_err() {
            eprintln!(
                "{}",
                "Failed to copy executable into app bundle".red().bold()
            );
            return;
        }

        if config.pack.icon != "none" {
            let icon_source = Path::new("assets").join(&config.pack.icon);
            if icon_source.exists() {
                let _ = fs::copy(icon_source, resources_dir.join("AppIcon.icns"));
            }
        }

        let plist = INFO_PLIST
            .replace("((APPNAME))", &app_name)
            .replace("((APPNAMELC))", &app_name.to_lowercase().replace(' ', "_"))
            .replace(
                "((IDENTIFIER))",
                config.pack.identifier.as_deref().unwrap_or("example"),
            )
            .replace(
                "((VERSION))",
                config.pack.version.as_deref().unwrap_or("1.0"),
            )
            .replace("((EXECUTABLE))", &app_name);
        if fs::write(contents_dir.join("Info.plist"), plist).is_err() {
            eprintln!("{}", "Failed to write Info.plist".red().bold());
            return;
        }

        println!("{} {}", "Pack backend:".cyan(), backend.bold().green());
        println!(
            "{} {}",
            "Created bundle:".cyan(),
            bundle_dir.display().to_string().bold().green()
        );
        return;
    }

    let output_name = if host == "windows" {
        format!("{app_name}.exe")
    } else {
        app_name.clone()
    };
    let output_path = app_dir.join(output_name);
    if fs::copy(executable, &output_path).is_err() {
        eprintln!("{}", "Failed to create packaged executable".red().bold());
        return;
    }

    println!("{} {}", "Pack backend:".cyan(), backend.bold().green());
    println!(
        "{} {}",
        "Created package:".cyan(),
        output_path.display().to_string().bold().green()
    );
}
