use crate::Commands;
use colored::Colorize;
use serde_json::Value;
use std::ffi::{CStr, CString, OsStr, c_void};
use std::fs;
use std::os::raw::c_char;
use std::path::{Path, PathBuf};

type AtlasRuntimeRunProject = unsafe extern "C" fn(project_file: *const c_char) -> bool;

const RTLD_NOW: i32 = 0x2;

unsafe extern "C" {
    fn dlopen(path: *const c_char, mode: i32) -> *mut c_void;
    fn dlsym(handle: *mut c_void, symbol: *const c_char) -> *mut c_void;
    fn dlerror() -> *const c_char;
}

fn should_skip_directory(path: &Path, root: &Path) -> bool {
    let relative = path.strip_prefix(root).unwrap_or(path);
    if relative.as_os_str().is_empty() {
        return false;
    }

    relative.components().any(|component| {
        if let std::path::Component::Normal(name) = component {
            matches!(
                name.to_str(),
                Some(".git")
                    | Some(".atlas-script-build")
                    | Some("node_modules")
                    | Some("dist")
                    | Some("build")
                    | Some("target")
            )
        } else {
            false
        }
    })
}

fn collect_atlas_files(
    root: &Path,
    current: &Path,
    result: &mut Vec<PathBuf>,
) -> Result<(), String> {
    for entry in
        fs::read_dir(current).map_err(|e| format!("Failed to read {}: {e}", current.display()))?
    {
        let entry = entry.map_err(|e| format!("Failed to inspect directory entry: {e}"))?;
        let path = entry.path();

        if path.is_dir() {
            if should_skip_directory(&path, root) {
                continue;
            }
            collect_atlas_files(root, &path, result)?;
            continue;
        }

        if path.extension() == Some(OsStr::new("atlas")) {
            result.push(path);
        }
    }

    Ok(())
}

fn find_manifest_in_directory(root: &Path) -> Result<PathBuf, String> {
    let mut candidates = Vec::new();
    collect_atlas_files(root, root, &mut candidates)?;
    candidates.sort();

    if let Some(project_manifest) = candidates.iter().find(|path| {
        path.file_name()
            .and_then(OsStr::to_str)
            .map(|name| name.eq_ignore_ascii_case("project.atlas"))
            .unwrap_or(false)
    }) {
        return Ok(project_manifest.clone());
    }

    candidates
        .into_iter()
        .next()
        .ok_or_else(|| format!("No .atlas project manifest was found in {}", root.display()))
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

    if let Some(stripped) = path.strip_prefix("~/") {
        return Ok(home_dir()?.join(stripped));
    }

    Ok(PathBuf::from(path))
}

fn atlas_version_from_manifest(path: &Path) -> Result<String, String> {
    let content =
        fs::read_to_string(path).map_err(|e| format!("Failed to read {}: {e}", path.display()))?;
    let value = content
        .parse::<toml::Value>()
        .map_err(|e| format!("Failed to parse {}: {e}", path.display()))?;

    value
        .get("atlas_version")
        .and_then(toml::Value::as_str)
        .map(ToOwned::to_owned)
        .ok_or_else(|| format!("{} does not define atlas_version", path.display()))
}

fn runtime_lib_from_config(version: &str) -> Result<Option<PathBuf>, String> {
    let config_path = home_dir()?.join(".atlas/config.json");
    let content = match fs::read_to_string(&config_path) {
        Ok(content) => content,
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => return Ok(None),
        Err(e) => return Err(format!("Failed to read {}: {e}", config_path.display())),
    };

    let root: Value = serde_json::from_str(&content)
        .map_err(|e| format!("Failed to parse {}: {e}", config_path.display()))?;
    let Some(runtime_entry) = root.get(version) else {
        return Ok(None);
    };
    let Some(runtime_lib) = runtime_entry
        .get("onboardingData")
        .and_then(|value| value.get("runtimeLib"))
        .and_then(Value::as_str)
    else {
        return Ok(None);
    };

    Ok(Some(expand_user_path(runtime_lib)?))
}

fn candidate_runtime_paths(
    project_path: &Path,
    atlas_version: &str,
) -> Result<Vec<PathBuf>, String> {
    let mut candidates = Vec::new();

    if let Ok(path) = std::env::var("ATLAS_RUNTIME_LIB") {
        candidates.push(PathBuf::from(path));
    }

    if let Ok(dir) = std::env::var("ATLAS_RUNTIME_LIB_DIR") {
        let dir = PathBuf::from(dir);
        for name in ["runtime.dylib", "libruntime.dylib", "libruntime_lib.dylib"] {
            candidates.push(dir.join(name));
        }
    }

    if let Some(path) = runtime_lib_from_config(atlas_version)? {
        candidates.push(path);
    }

    if let Some(project_dir) = project_path.parent() {
        for name in ["runtime.dylib", "libruntime.dylib", "libruntime_lib.dylib"] {
            candidates.push(project_dir.join(name));
            candidates.push(project_dir.join("lib").join(name));
        }
    }

    candidates.sort();
    candidates.dedup();
    Ok(candidates)
}

fn dlerror_string() -> String {
    unsafe {
        let ptr = dlerror();
        if ptr.is_null() {
            return String::from("unknown dynamic loader error");
        }
        CStr::from_ptr(ptr).to_string_lossy().into_owned()
    }
}

fn load_runtime_entry(
    runtime_path: &Path,
) -> Result<(*mut c_void, AtlasRuntimeRunProject), String> {
    let runtime_path = runtime_path
        .canonicalize()
        .map_err(|e| format!("Failed to resolve {}: {e}", runtime_path.display()))?;
    let runtime_cstr = CString::new(runtime_path.to_string_lossy().to_string()).map_err(|_| {
        format!(
            "Runtime library path contains an embedded NUL byte: {}",
            runtime_path.display()
        )
    })?;

    let handle = unsafe { dlopen(runtime_cstr.as_ptr(), RTLD_NOW) };
    if handle.is_null() {
        return Err(format!(
            "Failed to load runtime library {}: {}",
            runtime_path.display(),
            dlerror_string()
        ));
    }

    let symbol_name = CString::new("atlas_runtime_run_project").expect("symbol name is static");
    let symbol = unsafe { dlsym(handle, symbol_name.as_ptr()) };
    if symbol.is_null() {
        return Err(format!(
            "Runtime library {} does not export atlas_runtime_run_project",
            runtime_path.display()
        ));
    }

    let function: AtlasRuntimeRunProject = unsafe { std::mem::transmute(symbol) };
    Ok((handle, function))
}

fn resolve_runtime_entry(
    project_path: &Path,
) -> Result<(*mut c_void, AtlasRuntimeRunProject), String> {
    let atlas_version = atlas_version_from_manifest(project_path)?;
    let candidates = candidate_runtime_paths(project_path, &atlas_version)?;

    let mut errors = Vec::new();
    for candidate in candidates {
        if !candidate.is_file() {
            continue;
        }
        match load_runtime_entry(&candidate) {
            Ok(entry) => return Ok(entry),
            Err(error) => errors.push(error),
        }
    }

    if errors.is_empty() {
        return Err(format!(
            "No runtime library was found for atlas_version '{}'. Checked ATLAS_RUNTIME_LIB, ATLAS_RUNTIME_LIB_DIR, ~/.atlas/config.json, and project-local runtime paths.",
            atlas_version
        ));
    }

    Err(errors.join("\n"))
}

fn resolve_project_path(path: Option<String>) -> Result<PathBuf, String> {
    let input = match path {
        Some(value) => PathBuf::from(value),
        None => std::env::current_dir()
            .map_err(|e| format!("Failed to resolve current directory: {e}"))?,
    };

    let candidate = if input.is_dir() {
        find_manifest_in_directory(&input)?
    } else {
        input
    };

    candidate
        .canonicalize()
        .map_err(|e| format!("Failed to resolve {}: {e}", candidate.display()))
}

pub fn run(cmd: Commands) {
    let path = match cmd {
        Commands::Run { path } => path,
        _ => None,
    };

    let project_path = match resolve_project_path(path) {
        Ok(path) => path,
        Err(e) => {
            eprintln!("{}\n{e}", "atlas run failed".red().bold());
            return;
        }
    };

    let project_path_str = project_path.to_string_lossy().to_string();
    let project_file = match CString::new(project_path_str.clone()) {
        Ok(value) => value,
        Err(_) => {
            eprintln!(
                "{} {}",
                "atlas run failed:".red().bold(),
                "project path contains an embedded NUL byte"
            );
            return;
        }
    };

    println!(
        "{} {}",
        "Running project:".cyan(),
        project_path.display().to_string().bold().green()
    );

    let (_runtime_handle, run_project) = match resolve_runtime_entry(&project_path) {
        Ok(entry) => entry,
        Err(e) => {
            eprintln!("{}\n{e}", "atlas run failed".red().bold());
            return;
        }
    };

    let ok = unsafe { run_project(project_file.as_ptr()) };
    if !ok {
        eprintln!("{} {}", "atlas run failed:".red().bold(), project_path_str);
    }
}
