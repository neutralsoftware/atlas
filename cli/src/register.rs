use crate::Commands;
use colored::Colorize;
use serde_json::{Map, Value};
use std::fs;
use std::path::{Path, PathBuf};

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

fn resolve_file(path: &str, label: &str) -> Result<PathBuf, String> {
    let expanded = expand_user_path(path)?;
    if !expanded.is_file() {
        return Err(format!("{label} was not found at {}", expanded.display()));
    }
    expanded
        .canonicalize()
        .map_err(|e| format!("Failed to resolve {}: {e}", expanded.display()))
}

fn read_config(path: &Path) -> Result<Map<String, Value>, String> {
    let contents = match fs::read_to_string(path) {
        Ok(contents) => contents,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => return Ok(Map::new()),
        Err(error) => return Err(format!("Failed to read {}: {error}", path.display())),
    };
    serde_json::from_str::<Value>(&contents)
        .map_err(|e| format!("Failed to parse {}: {e}", path.display()))?
        .as_object()
        .cloned()
        .ok_or_else(|| format!("{} must contain a JSON object", path.display()))
}

fn register_runtime(version: &str, atlas: &str, runtime_lib: &str) -> Result<PathBuf, String> {
    let version = version.trim();
    if version.is_empty() {
        return Err(String::from("Runtime version cannot be empty"));
    }

    let atlas_path = resolve_file(atlas, "Atlas executable")?;
    let runtime_path = resolve_file(runtime_lib, "Runtime library")?;
    let config_path = home_dir()?.join(".atlas/config.json");
    let mut config = read_config(&config_path)?;
    let entry = config
        .entry(version.to_string())
        .or_insert_with(|| Value::Object(Map::new()));
    let entry = entry
        .as_object_mut()
        .ok_or_else(|| format!("Runtime entry '{version}' must be a JSON object"))?;
    let onboarding = entry
        .entry(String::from("onboardingData"))
        .or_insert_with(|| Value::Object(Map::new()));
    let onboarding = onboarding
        .as_object_mut()
        .ok_or_else(|| format!("Runtime entry '{version}.onboardingData' must be a JSON object"))?;
    onboarding.insert(
        String::from("atlasExecutablePath"),
        Value::String(atlas_path.to_string_lossy().into_owned()),
    );
    onboarding.insert(
        String::from("runtimeLib"),
        Value::String(runtime_path.to_string_lossy().into_owned()),
    );

    if let Some(parent) = config_path.parent() {
        fs::create_dir_all(parent)
            .map_err(|e| format!("Failed to create {}: {e}", parent.display()))?;
    }
    let contents = serde_json::to_string_pretty(&Value::Object(config))
        .map_err(|e| format!("Failed to serialize runtime configuration: {e}"))?;
    fs::write(&config_path, format!("{contents}\n"))
        .map_err(|e| format!("Failed to write {}: {e}", config_path.display()))?;
    Ok(config_path)
}

pub fn register(cmd: Commands) {
    let Commands::Register {
        version,
        atlas,
        runtime_lib,
    } = cmd
    else {
        return;
    };

    match register_runtime(&version, &atlas, &runtime_lib) {
        Ok(config_path) => {
            println!(
                "{} {}",
                "Registered runtime:".green().bold(),
                version.bold()
            );
            println!("{} {}", "Configuration:".cyan(), config_path.display());
        }
        Err(error) => {
            eprintln!("{}\n{error}", "atlas register failed".red().bold());
            std::process::exit(1);
        }
    }
}
