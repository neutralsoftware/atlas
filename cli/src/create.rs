use crate::{Commands, CreateRenderer};
use colored::Colorize;
use dialoguer::Input;
use dialoguer::Select;
use dialoguer::theme::ColorfulTheme;
use serde_json::Value;
use std::ffi::OsStr;
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Clone)]
struct RuntimeInstall {
    version: String,
    atlas_executable_path: PathBuf,
    runtime_lib_path: PathBuf,
}

const PROJECT_TEMPLATE: &str = r#"app_name = "((APP_NAME))"
atlas_version = "((ATLAS_VERSION))"
name = "((PROJECT_NAME))"

[game]
assets = ["assets/"]
main_scene = "main.ascene"

[pack]
icon = "none"
supported_platforms = "all"

[renderer]
default = "((RENDERER_DEFAULT))"
global_illumination = ((GLOBAL_ILLUMINATION))

[window]
dimensions = [1280, 720]
mouse_capture = false
multisampling = false
ssaoScale = 1.0
"#;

const SCENE_TEMPLATE: &str = r#"{
    "name": "Main Scene",
    "id": "main_scene",
    "objects": [
        {
            "name": "Cube",
            "type": "solid",
            "solid_type": "cube",
            "position": [0.0, 0.0, 0.0],
            "rotation": [0.0, 0.0, 0.0],
            "scale": [1.0, 1.0, 1.0],
            "material": "",
        },
    ],
    "lights": [
        {
            "type": "ambient",
            "intensity": 0.2,
        },
    ],
    "camera": {
        "position": [0.0, 0.0, -5.0],
        "target": [0.0, 0.0, 0.0],
        "fov": 60.0,
    },
    "targets": [
        {
            "name": "Main Target",
            "type": "multisampled",
            "render": true,
            "display": true,
        },
    ],
    "environment": {
        "automaticAmbient": true,
        "atmosphereSky": true,
    },
}
"#;

fn home_dir() -> Result<PathBuf, String> {
    std::env::var_os("HOME")
        .map(PathBuf::from)
        .ok_or_else(|| String::from("HOME is not set"))
}

fn expand_user_path(path: &str) -> Result<PathBuf, String> {
    if path == "~" {
        return home_dir();
    }

    if let Some(stripped) = path.strip_prefix("~/") {
        return Ok(home_dir()?.join(stripped));
    }

    Ok(PathBuf::from(path))
}

fn atlas_config_path() -> Result<PathBuf, String> {
    Ok(home_dir()?.join(".atlas/config.json"))
}

fn read_runtime_installs() -> Result<Vec<RuntimeInstall>, String> {
    let config_path = atlas_config_path()?;
    let content = fs::read_to_string(&config_path)
        .map_err(|e| format!("Failed to read {}: {e}", config_path.display()))?;
    let root: Value = serde_json::from_str(&content)
        .map_err(|e| format!("Failed to parse {}: {e}", config_path.display()))?;
    let object = root
        .as_object()
        .ok_or_else(|| format!("{} must contain a JSON object", config_path.display()))?;

    let mut installs = Vec::new();
    for (version, entry) in object {
        let Some(onboarding_data) = entry.get("onboardingData").and_then(Value::as_object) else {
            continue;
        };

        let Some(executable_path) = onboarding_data
            .get("atlasExecutablePath")
            .and_then(Value::as_str)
        else {
            continue;
        };

        let Some(runtime_lib_path) = onboarding_data.get("runtimeLib").and_then(Value::as_str)
        else {
            continue;
        };

        installs.push(RuntimeInstall {
            version: version.clone(),
            atlas_executable_path: expand_user_path(executable_path)?,
            runtime_lib_path: expand_user_path(runtime_lib_path)?,
        });
    }

    installs.sort_by(|left, right| left.version.cmp(&right.version));

    if installs.is_empty() {
        return Err(format!(
            "No configured runtimes with atlasExecutablePath and runtimeLib were found in {}",
            config_path.display()
        ));
    }

    Ok(installs)
}

fn ensure_file_exists(path: &Path, label: &str) -> Result<(), String> {
    if path.is_file() {
        return Ok(());
    }
    Err(format!("{label} was not found at {}", path.display()))
}

fn select_runtime(
    installs: &[RuntimeInstall],
    requested_version: Option<String>,
) -> Result<RuntimeInstall, String> {
    if let Some(requested) = requested_version {
        return installs
            .iter()
            .find(|install| install.version == requested)
            .cloned()
            .ok_or_else(|| {
                format!(
                    "Runtime '{}' was not found in ~/.atlas/config.json. Available runtimes: {}",
                    requested,
                    installs
                        .iter()
                        .map(|install| install.version.as_str())
                        .collect::<Vec<_>>()
                        .join(", ")
                )
            });
    }

    if installs.len() == 1 {
        return Ok(installs[0].clone());
    }

    let theme = ColorfulTheme::default();
    let items = installs
        .iter()
        .map(|install| {
            format!(
                "{} | atlas: {} | runtime-lib: {}",
                install.version,
                install.atlas_executable_path.display(),
                install.runtime_lib_path.display()
            )
        })
        .collect::<Vec<_>>();

    let index = Select::with_theme(&theme)
        .with_prompt("Choose a runtime")
        .items(&items)
        .default(0)
        .interact()
        .map_err(|e| format!("Failed to choose a runtime: {e}"))?;

    Ok(installs[index].clone())
}

fn write_file(path: &Path, contents: &str) -> Result<(), String> {
    fs::write(path, contents).map_err(|e| format!("Failed to write {}: {e}", path.display()))
}

fn infer_name_from_path(path: &Path) -> Option<String> {
    path.file_name()
        .and_then(OsStr::to_str)
        .filter(|value| !value.trim().is_empty())
        .map(ToOwned::to_owned)
}

fn fail(message: impl std::fmt::Display) -> ! {
    eprintln!("{} {}", "atlas create failed:".red().bold(), message);
    std::process::exit(1);
}

pub fn create(cmd: Commands) {
    let (
        positional_name,
        flagged_name,
        requested_path,
        requested_version,
        requested_renderer,
        requested_global_illumination,
    ) = match cmd {
        Commands::Create {
            name,
            project_name,
            path,
            version,
            renderer,
            global_illumination,
        } => (
            name,
            project_name,
            path,
            version,
            renderer,
            global_illumination,
        ),
        _ => return,
    };

    let requested_name = flagged_name.or(positional_name);

    let installs = match read_runtime_installs() {
        Ok(installs) => installs,
        Err(e) => fail(e),
    };

    let runtime = match select_runtime(&installs, requested_version) {
        Ok(runtime) => runtime,
        Err(e) => fail(e),
    };

    if let Err(e) = ensure_file_exists(&runtime.atlas_executable_path, "Atlas executable") {
        fail(e);
    }

    if let Err(e) = ensure_file_exists(&runtime.runtime_lib_path, "Runtime library") {
        fail(e);
    }

    let project_root = match requested_path {
        Some(path) => PathBuf::from(path),
        None => match &requested_name {
            Some(name) => PathBuf::from(name),
            None => {
                let theme = ColorfulTheme::default();
                let name: String = match Input::with_theme(&theme)
                    .with_prompt("Project name")
                    .interact_text()
                {
                    Ok(value) => value,
                    Err(e) => fail(e),
                };

                let trimmed = name.trim().to_string();
                if trimmed.is_empty() {
                    fail("project name cannot be empty");
                }

                PathBuf::from(trimmed)
            }
        },
    };

    let name = match requested_name.or_else(|| infer_name_from_path(&project_root)) {
        Some(name) => name,
        None => fail(format!(
            "could not infer a project name from path {}. Pass --name explicitly.",
            project_root.display()
        )),
    };

    let renderer = requested_renderer.unwrap_or(CreateRenderer::Deferred);
    let default_renderer = match renderer {
        CreateRenderer::Deferred => "deferred",
        CreateRenderer::Pathtracing => "pathtracing",
    };
    let global_illumination =
        matches!(renderer, CreateRenderer::Deferred) && requested_global_illumination;

    if project_root.exists() {
        fail(format!(
            "target path already exists: {}",
            project_root.display()
        ));
    }

    if let Err(e) = fs::create_dir_all(project_root.join("assets/scripts")) {
        fail(e);
    }

    let manifest = PROJECT_TEMPLATE
        .replace("((APP_NAME))", &name)
        .replace("((ATLAS_VERSION))", &runtime.version)
        .replace("((PROJECT_NAME))", &name)
        .replace("((RENDERER_DEFAULT))", default_renderer)
        .replace(
            "((GLOBAL_ILLUMINATION))",
            if global_illumination { "true" } else { "false" },
        );

    if let Err(e) = write_file(&project_root.join("project.atlas"), &manifest) {
        fail(e);
    }

    if let Err(e) = write_file(&project_root.join("main.ascene"), SCENE_TEMPLATE) {
        fail(e);
    }

    println!(
        "{} {}",
        "Selected runtime:".cyan(),
        runtime.version.bold().green()
    );
    println!(
        "{} {}",
        "Atlas executable:".cyan(),
        runtime.atlas_executable_path.display().to_string().bold()
    );
    println!(
        "{} {}",
        "Runtime library:".cyan(),
        runtime.runtime_lib_path.display().to_string().bold()
    );
    println!(
        "{} {}",
        "Created project:".green().bold(),
        project_root.display()
    );
    println!(
        "{} {}",
        "Manifest:".cyan(),
        project_root.join("project.atlas").display()
    );
    println!(
        "{} {}",
        "Scene:".cyan(),
        project_root.join("main.ascene").display()
    );
    println!(
        "{} {}",
        "Scripts directory:".cyan(),
        project_root.join("assets/scripts").display()
    );
}
