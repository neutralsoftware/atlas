use crate::{Commands, Config};
use colored::Colorize;
use serde_json::Value;
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
    <string>((IDENTIFIER))</string>
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
    let config_str = fs::read_to_string("project.atlas")
        .map_err(|e| format!("Failed to read project.atlas: {e}"))?;
    toml::from_str(&config_str).map_err(|e| format!("Failed to parse project.atlas: {e}"))
}

struct RuntimePaths {
    atlas: PathBuf,
    library: PathBuf,
}

fn home_dir() -> Result<PathBuf, String> {
    std::env::var_os("HOME")
        .map(PathBuf::from)
        .ok_or_else(|| String::from("HOME is not set"))
}

fn expand_user_path(path: &str) -> Result<PathBuf, String> {
    if path == "~" {
        return home_dir();
    }
    if let Some(relative) = path.strip_prefix("~/") {
        return Ok(home_dir()?.join(relative));
    }
    Ok(PathBuf::from(path))
}

fn runtime_paths(version: &str) -> Result<RuntimePaths, String> {
    let config_path = home_dir()?.join(".atlas/config.json");
    let contents = fs::read_to_string(&config_path)
        .map_err(|e| format!("Failed to read {}: {e}", config_path.display()))?;
    let root: Value = serde_json::from_str(&contents)
        .map_err(|e| format!("Failed to parse {}: {e}", config_path.display()))?;
    let versions = root
        .as_object()
        .ok_or_else(|| String::from("Atlas runtime configuration must be an object"))?;
    let entry = versions
        .get(version)
        .or_else(|| {
            versions
                .iter()
                .find(|(key, _)| {
                    key.eq_ignore_ascii_case(version)
                        || key.starts_with(version)
                        || version.starts_with(key.as_str())
                })
                .map(|(_, value)| value)
        })
        .or_else(|| {
            if versions.len() == 1 {
                versions.values().next()
            } else {
                None
            }
        })
        .ok_or_else(|| format!("No installed Atlas runtime matches '{version}'"))?;
    let onboarding = entry
        .get("onboardingData")
        .and_then(Value::as_object)
        .ok_or_else(|| String::from("The installed runtime has no onboarding data"))?;
    let atlas = onboarding
        .get("atlasExecutablePath")
        .and_then(Value::as_str)
        .ok_or_else(|| String::from("The installed runtime has no Atlas executable"))?;
    let library = onboarding
        .get("runtimeLib")
        .and_then(Value::as_str)
        .ok_or_else(|| String::from("The installed runtime has no runtime library"))?;
    let paths = RuntimePaths {
        atlas: expand_user_path(atlas)?,
        library: expand_user_path(library)?,
    };
    if !paths.atlas.is_file() {
        return Err(format!(
            "Atlas executable not found at {}",
            paths.atlas.display()
        ));
    }
    if !paths.library.is_file() {
        return Err(format!(
            "Runtime library not found at {}",
            paths.library.display()
        ));
    }
    Ok(paths)
}

fn copy_project(source: &Path, destination: &Path) -> Result<(), String> {
    fs::create_dir_all(destination)
        .map_err(|e| format!("Failed to create {}: {e}", destination.display()))?;
    for entry in
        fs::read_dir(source).map_err(|e| format!("Failed to read {}: {e}", source.display()))?
    {
        let entry = entry.map_err(|e| format!("Failed to inspect project file: {e}"))?;
        let path = entry.path();
        let name = entry.file_name();
        if matches!(
            name.to_str(),
            Some(".git")
                | Some(".atlas")
                | Some("build")
                | Some("dist")
                | Some("Exports")
                | Some("node_modules")
                | Some("target")
        ) {
            continue;
        }
        let target = destination.join(&name);
        if path.is_dir() {
            copy_project(&path, &target)?;
        } else {
            fs::copy(&path, &target).map_err(|e| {
                format!(
                    "Failed to copy {} to {}: {e}",
                    path.display(),
                    target.display()
                )
            })?;
        }
    }
    Ok(())
}

#[cfg(unix)]
fn make_executable(path: &Path) -> Result<(), String> {
    use std::os::unix::fs::PermissionsExt;
    let mut permissions = fs::metadata(path)
        .map_err(|e| format!("Failed to inspect {}: {e}", path.display()))?
        .permissions();
    permissions.set_mode(0o755);
    fs::set_permissions(path, permissions)
        .map_err(|e| format!("Failed to make {} executable: {e}", path.display()))
}

#[cfg(not(unix))]
fn make_executable(_path: &Path) -> Result<(), String> {
    Ok(())
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
        "Current platform '{current}' is not supported by project.atlas"
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
    let backend_override = match cmd {
        Commands::Pack { backend, .. } => backend,
        _ => None,
    };
    let config = match parse_config() {
        Ok(config) => config,
        Err(error) => {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
    };
    if let Err(error) = ensure_supported_platform(&config) {
        eprintln!("{}\n{error}", "atlas pack failed".red().bold());
        std::process::exit(1);
    }
    let backend = resolve_backend(&config, backend_override);
    let version = config.project.atlas_version.as_deref().unwrap_or("stable");
    let runtime = match runtime_paths(version) {
        Ok(runtime) => runtime,
        Err(error) => {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
    };
    let project_root = match std::env::current_dir() {
        Ok(path) => path,
        Err(error) => {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
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
        if let Err(error) = fs::remove_dir_all(app_dir) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
    }
    if let Err(error) = fs::create_dir_all(app_dir) {
        eprintln!(
            "{}\n{error}",
            "Failed to create dist directory".red().bold()
        );
        std::process::exit(1);
    }

    if host == "macos" {
        let bundle_dir = app_dir.join(format!("{app_name}.app"));
        let contents_dir = bundle_dir.join("Contents");
        let macos_dir = contents_dir.join("MacOS");
        let frameworks_dir = contents_dir.join("Frameworks");
        let resources_dir = contents_dir.join("Resources");

        for directory in [&macos_dir, &frameworks_dir, &resources_dir] {
            if let Err(error) = fs::create_dir_all(directory) {
                eprintln!("{}\n{error}", "atlas pack failed".red().bold());
                std::process::exit(1);
            }
        }

        let atlas_binary = macos_dir.join("atlas");
        if let Err(error) = fs::copy(&runtime.atlas, &atlas_binary) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
        let runtime_name = runtime
            .library
            .file_name()
            .unwrap_or_else(|| std::ffi::OsStr::new("runtime.dylib"));
        let runtime_target = frameworks_dir.join(runtime_name);
        if let Err(error) = fs::copy(&runtime.library, &runtime_target) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
        if let Err(error) = copy_project(&project_root, &resources_dir.join("Project")) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
        let launcher = macos_dir.join("AtlasLauncher");
        let launcher_contents = format!(
            "#!/bin/sh\nCONTENTS=\"$(cd \"$(dirname \"$0\")/..\" && pwd)\"\nexport ATLAS_RUNTIME_LIB=\"$CONTENTS/Frameworks/{}\"\nexec \"$CONTENTS/MacOS/atlas\" run \"$CONTENTS/Resources/Project/project.atlas\"\n",
            runtime_name.to_string_lossy()
        );
        if let Err(error) = fs::write(&launcher, launcher_contents) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }
        if let Err(error) = make_executable(&launcher) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }

        if config.pack.icon != "none" {
            let icon_source = project_root.join("assets").join(&config.pack.icon);
            if icon_source.exists() {
                let _ = fs::copy(icon_source, resources_dir.join("AppIcon.icns"));
            }
        }

        let default_identifier = format!(
            "org.atlasengine.{}",
            app_name.to_lowercase().replace(' ', "-")
        );
        let identifier = config
            .pack
            .identifier
            .as_deref()
            .unwrap_or(&default_identifier);
        let plist = INFO_PLIST
            .replace("((APPNAME))", &app_name)
            .replace("((APPNAMELC))", &app_name.to_lowercase().replace(' ', "_"))
            .replace("((IDENTIFIER))", identifier)
            .replace(
                "((VERSION))",
                config.pack.version.as_deref().unwrap_or("1.0"),
            )
            .replace("((EXECUTABLE))", "AtlasLauncher");
        if let Err(error) = fs::write(contents_dir.join("Info.plist"), plist) {
            eprintln!("{}\n{error}", "atlas pack failed".red().bold());
            std::process::exit(1);
        }

        println!("{} {}", "Pack backend:".cyan(), backend.bold().green());
        println!(
            "{} {}",
            "Created bundle:".cyan(),
            bundle_dir.display().to_string().bold().green()
        );
        return;
    }

    let package_dir = app_dir.join(&app_name);
    if let Err(error) = fs::create_dir_all(&package_dir) {
        eprintln!("{}\n{error}", "atlas pack failed".red().bold());
        std::process::exit(1);
    }
    let atlas_name = if host == "windows" {
        "atlas.exe"
    } else {
        "atlas"
    };
    if let Err(error) = fs::copy(&runtime.atlas, package_dir.join(atlas_name)) {
        eprintln!("{}\n{error}", "atlas pack failed".red().bold());
        std::process::exit(1);
    }
    let runtime_name = runtime
        .library
        .file_name()
        .unwrap_or_else(|| std::ffi::OsStr::new("runtime"));
    if let Err(error) = fs::copy(&runtime.library, package_dir.join(runtime_name)) {
        eprintln!("{}\n{error}", "atlas pack failed".red().bold());
        std::process::exit(1);
    }
    if let Err(error) = copy_project(&project_root, &package_dir.join("Project")) {
        eprintln!("{}\n{error}", "atlas pack failed".red().bold());
        std::process::exit(1);
    }

    println!("{} {}", "Pack backend:".cyan(), backend.bold().green());
    println!(
        "{} {}",
        "Created package:".cyan(),
        package_dir.display().to_string().bold().green()
    );
}
