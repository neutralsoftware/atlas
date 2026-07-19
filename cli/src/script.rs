use crate::{Commands, ScriptCommands};
use colored::Colorize;
use dialoguer::Input;
use dialoguer::theme::ColorfulTheme;
use reqwest::blocking::Client;
use serde_json::{Map, Value};
use std::ffi::OsStr;
use std::fs;
use std::path::{Component, Path, PathBuf};
use std::process::Command;

const TYPESCRIPT_VERSION: &str = "^5.9.2";
const ESBUILD_VERSION: &str = "^0.25.5";
const SCRIPT_BUILD_DIR: &str = ".atlas-script-build";
const SCRIPT_BUNDLE_FILE: &str = "dist/scripts.js";

fn github_client() -> Result<Client, String> {
    Client::builder()
        .user_agent("atlas-cli")
        .build()
        .map_err(|e| format!("Failed to create HTTP client: {e}"))
}

fn normalize_path_for_manifest(path: &Path) -> String {
    path.to_string_lossy().replace('\\', "/")
}

fn ensure_directory(path: &Path) -> Result<(), String> {
    fs::create_dir_all(path).map_err(|e| format!("Failed to create {}: {e}", path.display()))
}

fn read_json_file(path: &Path) -> Result<Value, String> {
    let content =
        fs::read_to_string(path).map_err(|e| format!("Failed to read {}: {e}", path.display()))?;
    serde_json::from_str(&content).map_err(|e| format!("Failed to parse {}: {e}", path.display()))
}

fn write_json_file(path: &Path, value: &Value) -> Result<(), String> {
    let content = serde_json::to_string_pretty(value)
        .map_err(|e| format!("Failed to serialize {}: {e}", path.display()))?;
    fs::write(path, format!("{content}\n"))
        .map_err(|e| format!("Failed to write {}: {e}", path.display()))
}

fn json_object_mut<'a>(
    parent: &'a mut Map<String, Value>,
    key: &str,
) -> Result<&'a mut Map<String, Value>, String> {
    let entry = parent
        .entry(key.to_string())
        .or_insert_with(|| Value::Object(Map::new()));
    entry
        .as_object_mut()
        .ok_or_else(|| format!("{key} must be a JSON object"))
}

fn update_package_json(project_dir: &Path) -> Result<(), String> {
    let package_path = project_dir.join("package.json");
    let mut root = if package_path.exists() {
        read_json_file(&package_path)?
    } else {
        Value::Object(Map::new())
    };

    let root_object = root
        .as_object_mut()
        .ok_or_else(|| String::from("package.json root must be an object"))?;

    if !root_object.contains_key("name") {
        let default_name = project_dir
            .file_name()
            .and_then(OsStr::to_str)
            .unwrap_or("atlas-scripts")
            .to_lowercase()
            .replace(' ', "-");
        root_object.insert("name".to_string(), Value::String(default_name));
    }

    root_object.insert("private".to_string(), Value::Bool(true));
    root_object.insert("type".to_string(), Value::String(String::from("module")));

    {
        let dev_dependencies = json_object_mut(root_object, "devDependencies")?;
        dev_dependencies.insert(
            String::from("typescript"),
            Value::String(String::from(TYPESCRIPT_VERSION)),
        );
        dev_dependencies.insert(
            String::from("esbuild"),
            Value::String(String::from(ESBUILD_VERSION)),
        );
    }

    {
        let scripts = json_object_mut(root_object, "scripts")?;
        scripts.insert(
            String::from("atlas:compile"),
            Value::String(String::from("atlas script compile")),
        );
        scripts.insert(
            String::from("typecheck"),
            Value::String(String::from("tsc --noEmit")),
        );
    }

    write_json_file(&package_path, &root)
}

fn update_tsconfig(project_dir: &Path) -> Result<(), String> {
    let tsconfig_path = project_dir.join("tsconfig.json");
    let mut root = if tsconfig_path.exists() {
        read_json_file(&tsconfig_path)?
    } else {
        Value::Object(Map::new())
    };

    let root_object = root
        .as_object_mut()
        .ok_or_else(|| String::from("tsconfig.json root must be an object"))?;
    let compiler_options = json_object_mut(root_object, "compilerOptions")?;

    compiler_options.insert(
        String::from("target"),
        Value::String(String::from("ES2022")),
    );
    compiler_options.insert(
        String::from("module"),
        Value::String(String::from("ESNext")),
    );
    compiler_options.insert(
        String::from("moduleResolution"),
        Value::String(String::from("Bundler")),
    );
    compiler_options.insert(String::from("baseUrl"), Value::String(String::from(".")));
    compiler_options.insert(
        String::from("ignoreDeprecations"),
        Value::String(String::from("6.0")),
    );
    compiler_options.insert(String::from("strict"), Value::Bool(true));
    compiler_options.insert(String::from("noEmit"), Value::Bool(true));
    compiler_options.insert(String::from("skipLibCheck"), Value::Bool(true));
    compiler_options.insert(String::from("verbatimModuleSyntax"), Value::Bool(true));

    {
        let paths = json_object_mut(compiler_options, "paths")?;
        paths.insert(
            String::from("atlas"),
            Value::Array(vec![Value::String(String::from("lib/atlas.d.ts"))]),
        );
        paths.insert(
            String::from("atlas/*"),
            Value::Array(vec![Value::String(String::from("lib/*"))]),
        );
    }

    root_object.insert(
        String::from("include"),
        Value::Array(vec![
            Value::String(String::from("**/*.ts")),
            Value::String(String::from("**/*.mts")),
            Value::String(String::from("**/*.cts")),
        ]),
    );
    root_object.insert(
        String::from("exclude"),
        Value::Array(vec![
            Value::String(String::from(".git")),
            Value::String(String::from("node_modules")),
            Value::String(String::from("dist")),
            Value::String(String::from("build")),
            Value::String(String::from("target")),
            Value::String(String::from("extern")),
            Value::String(String::from("atlas")),
            Value::String(String::from("aurora")),
            Value::String(String::from("bezel")),
            Value::String(String::from("finewave")),
            Value::String(String::from("graphite")),
            Value::String(String::from("hydra")),
            Value::String(String::from("include")),
            Value::String(String::from("opal")),
            Value::String(String::from("photon")),
            Value::String(String::from("cli")),
            Value::String(String::from("docs")),
            Value::String(String::from("tests")),
            Value::String(String::from("runtime/lib")),
            Value::String(String::from("runtime/docs")),
            Value::String(String::from("runtime/executable")),
        ]),
    );

    write_json_file(&tsconfig_path, &root)
}

fn fetch_atlas_types(branch: &str) -> Result<String, String> {
    let client = github_client()?;
    let url = format!(
        "https://raw.githubusercontent.com/neutralsoftware/atlas/{branch}/runtime/atlas.d.ts"
    );
    let response = client
        .get(url)
        .send()
        .map_err(|e| format!("Failed to fetch atlas.d.ts: {e}"))?;
    let response = response
        .error_for_status()
        .map_err(|e| format!("Failed to fetch atlas.d.ts: {e}"))?;
    response
        .text()
        .map_err(|e| format!("Failed to read atlas.d.ts: {e}"))
}

fn command_exists(name: &str) -> bool {
    Command::new(name)
        .arg("--version")
        .output()
        .map(|output| output.status.success())
        .unwrap_or(false)
}

fn run_npm_install(project_dir: &Path) -> Result<(), String> {
    if !command_exists("npm") {
        return Err(String::from("npm is not installed"));
    }

    let output = Command::new("npm")
        .current_dir(project_dir)
        .arg("install")
        .output()
        .map_err(|e| format!("Failed to run npm install: {e}"))?;

    if output.status.success() {
        Ok(())
    } else {
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        let mut message = format!("npm install failed with status {}", output.status);
        if !stdout.trim().is_empty() {
            message.push_str("\n\nstdout:\n");
            message.push_str(stdout.trim());
        }
        if !stderr.trim().is_empty() {
            message.push_str("\n\nstderr:\n");
            message.push_str(stderr.trim());
        }
        Err(message)
    }
}

fn path_has_component(path: &Path, value: &str) -> bool {
    path.components().any(|component| match component {
        Component::Normal(name) => name == OsStr::new(value),
        _ => false,
    })
}

fn should_skip_directory(path: &Path, root: &Path) -> bool {
    let relative = path.strip_prefix(root).unwrap_or(path);
    if relative.as_os_str().is_empty() {
        return false;
    }

    relative.components().any(|component| {
        if let Component::Normal(name) = component {
            matches!(
                name.to_str(),
                Some(".git")
                    | Some(".atlas-script-build")
                    | Some("node_modules")
                    | Some("dist")
                    | Some("build")
                    | Some("target")
                    | Some("extern")
                    | Some("atlas")
                    | Some("aurora")
                    | Some("bezel")
                    | Some("finewave")
                    | Some("graphite")
                    | Some("hydra")
                    | Some("include")
                    | Some("lib")
                    | Some("libs")
                    | Some("library")
                    | Some("opal")
                    | Some("photon")
                    | Some("cli")
                    | Some("docs")
                    | Some("tests")
            )
        } else {
            false
        }
    })
}

fn is_typescript_entry(path: &Path, root: &Path) -> bool {
    if path.extension() != Some(OsStr::new("ts")) {
        return false;
    }

    let file_name = path.file_name().and_then(OsStr::to_str).unwrap_or_default();
    if file_name.ends_with(".d.ts") {
        return false;
    }

    let relative = path.strip_prefix(root).unwrap_or(path);
    if path_has_component(relative, "lib")
        || path_has_component(relative, "libs")
        || path_has_component(relative, "library")
    {
        return false;
    }

    !should_skip_directory(path.parent().unwrap_or(root), root)
}

fn collect_typescript_entries(
    root: &Path,
    current: &Path,
    entries: &mut Vec<PathBuf>,
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
            collect_typescript_entries(root, &path, entries)?;
            continue;
        }

        if is_typescript_entry(&path, root) {
            entries.push(path);
        }
    }

    Ok(())
}

fn find_local_esbuild(project_dir: &Path) -> Option<PathBuf> {
    let unix_path = project_dir.join("node_modules/.bin/esbuild");
    if unix_path.is_file() {
        return Some(unix_path);
    }

    let windows_path = project_dir.join("node_modules/.bin/esbuild.cmd");
    if windows_path.is_file() {
        return Some(windows_path);
    }

    None
}

fn json_string(value: &str) -> Result<String, String> {
    serde_json::to_string(value).map_err(|e| format!("Failed to encode string: {e}"))
}

fn write_script_bundle_support_files(
    project_dir: &Path,
    entry_points: &[PathBuf],
) -> Result<PathBuf, String> {
    let build_dir = project_dir.join(SCRIPT_BUILD_DIR);
    if build_dir.exists() {
        fs::remove_dir_all(&build_dir)
            .map_err(|e| format!("Failed to clean {}: {e}", build_dir.display()))?;
    }

    ensure_directory(&build_dir)?;
    let entry_path = build_dir.join("entry.ts");
    let mut entry_source = String::new();
    for (index, entry) in entry_points.iter().enumerate() {
        let relative = entry
            .strip_prefix(project_dir)
            .map_err(|e| format!("Failed to resolve {}: {e}", entry.display()))?;
        let import_path = format!("../{}", normalize_path_for_manifest(relative));
        entry_source.push_str(&format!(
            "import * as module_{index} from {};\n",
            json_string(&import_path)?
        ));
    }

    entry_source.push_str("\nexport const AtlasScripts = {\n");
    for (index, entry) in entry_points.iter().enumerate() {
        let relative = entry
            .strip_prefix(project_dir)
            .map_err(|e| format!("Failed to resolve {}: {e}", entry.display()))?;
        entry_source.push_str(&format!(
            "    {}: module_{index},\n",
            json_string(&normalize_path_for_manifest(relative))?
        ));
    }
    entry_source.push_str("};\n\nexport default AtlasScripts;\n");

    fs::write(&entry_path, entry_source)
        .map_err(|e| format!("Failed to write {}: {e}", entry_path.display()))?;

    Ok(entry_path)
}

fn run_esbuild(project_dir: &Path, entry_points: &[PathBuf]) -> Result<(), String> {
    let entry_path = write_script_bundle_support_files(project_dir, entry_points)?;
    let mut command = if let Some(esbuild_path) = find_local_esbuild(project_dir) {
        Command::new(esbuild_path)
    } else if command_exists("esbuild") {
        Command::new("esbuild")
    } else if command_exists("npx") {
        let mut command = Command::new("npx");
        command.arg("--no-install").arg("esbuild");
        command
    } else {
        return Err(String::from(
            "esbuild was not found. Run `atlas script init` or install esbuild first.",
        ));
    };

    command.current_dir(project_dir);
    let relative_entry = entry_path.strip_prefix(project_dir).unwrap_or(&entry_path);

    command
        .arg(relative_entry)
        .arg("--bundle")
        .arg("--format=esm")
        .arg("--platform=neutral")
        .arg("--target=es2022")
        .arg(format!("--outfile={SCRIPT_BUNDLE_FILE}"))
        .arg("--tsconfig=tsconfig.json")
        .arg("--external:atlas")
        .arg("--external:atlas/*")
        .arg("--external:bezel")
        .arg("--external:bezel/*")
        .arg("--external:aurora")
        .arg("--external:aurora/*")
        .arg("--external:finewave")
        .arg("--external:finewave/*")
        .arg("--external:graphite")
        .arg("--external:graphite/*")
        .arg("--external:hydra")
        .arg("--external:hydra/*");

    let output = command
        .output()
        .map_err(|e| format!("Failed to run esbuild: {e}"))?;

    if output.status.success() {
        if entry_path.exists() {
            let _ = fs::remove_dir_all(project_dir.join(SCRIPT_BUILD_DIR));
        }
        Ok(())
    } else {
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        let mut message = format!("esbuild failed with status {}", output.status);
        if !stdout.trim().is_empty() {
            message.push_str("\n\nstdout:\n");
            message.push_str(stdout.trim());
        }
        if !stderr.trim().is_empty() {
            message.push_str("\n\nstderr:\n");
            message.push_str(stderr.trim());
        }
        Err(message)
    }
}

fn candidate_atlas_files(root: &Path) -> Result<Vec<PathBuf>, String> {
    let mut result = Vec::new();
    collect_atlas_files(root, root, &mut result)?;
    result.sort();
    Ok(result)
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

fn find_manifest_file(project_dir: &Path) -> Result<Option<PathBuf>, String> {
    let candidates = candidate_atlas_files(project_dir)?;
    if candidates.is_empty() {
        return Ok(None);
    }

    if let Some(project_manifest) = candidates.iter().find(|path| {
        path.file_name()
            .and_then(OsStr::to_str)
            .map(|name| name.eq_ignore_ascii_case("project.atlas"))
            .unwrap_or(false)
    }) {
        return Ok(Some(project_manifest.clone()));
    }

    Ok(candidates.into_iter().next())
}

fn relative_manifest_path(path: &Path, manifest_path: &Path) -> String {
    let manifest_dir = manifest_path.parent().unwrap_or_else(|| Path::new("."));
    let relative = path
        .strip_prefix(manifest_dir)
        .map(Path::to_path_buf)
        .or_else(|_| path.strip_prefix(".").map(Path::to_path_buf))
        .unwrap_or_else(|_| path.to_path_buf());
    normalize_path_for_manifest(&relative)
}

fn update_script_manifest(
    manifest_path: &Path,
    component_name: &str,
    script_path: &Path,
) -> Result<(), String> {
    let content = fs::read_to_string(manifest_path)
        .map_err(|e| format!("Failed to read {}: {e}", manifest_path.display()))?;
    let mut value = content
        .parse::<toml::Value>()
        .map_err(|e| format!("Failed to parse {}: {e}", manifest_path.display()))?;
    let root = value
        .as_table_mut()
        .ok_or_else(|| format!("{} must contain a TOML table", manifest_path.display()))?;

    if !root.contains_key("scripts") {
        root.insert(
            String::from("scripts"),
            toml::Value::Table(toml::map::Map::new()),
        );
    }

    let scripts = root
        .get_mut("scripts")
        .and_then(toml::Value::as_table_mut)
        .ok_or_else(|| String::from("[scripts] must be a TOML table"))?;

    scripts.insert(
        component_name.to_string(),
        toml::Value::String(relative_manifest_path(script_path, manifest_path)),
    );

    let serialized = toml::to_string_pretty(&value)
        .map_err(|e| format!("Failed to serialize {}: {e}", manifest_path.display()))?;
    fs::write(manifest_path, serialized)
        .map_err(|e| format!("Failed to write {}: {e}", manifest_path.display()))
}

fn infer_component_name(path: &Path) -> String {
    path.file_stem()
        .and_then(OsStr::to_str)
        .filter(|name| !name.trim().is_empty())
        .map(|name| {
            let mut chars = name.chars();
            match chars.next() {
                Some(first) => first.to_uppercase().collect::<String>() + chars.as_str(),
                None => String::from("Component"),
            }
        })
        .unwrap_or_else(|| String::from("Component"))
}

fn normalize_script_path(path: &str, project_dir: &Path) -> PathBuf {
    let raw = Path::new(path);
    let mut resolved = if raw.is_absolute() {
        raw.to_path_buf()
    } else {
        project_dir.join(raw)
    };

    if resolved.extension() != Some(OsStr::new("ts")) {
        resolved.set_extension("ts");
    }

    resolved
}

fn script_template(component_name: &str) -> String {
    format!(
        "import {{ Component }} from \"atlas\";\n\nexport class {component_name} extends Component {{\n    init() {{\n        // Called once when the script is initialized\n    }}\n\n    update(deltaTime: number) {{\n        // Called every frame with the time elapsed since the last frame\n    }}\n}}\n"
    )
}

fn init(branch: String) -> bool {
    let project_dir = match std::env::current_dir() {
        Ok(dir) => dir,
        Err(e) => {
            eprintln!(
                "{} {e}",
                "Failed to resolve current directory:".red().bold()
            );
            return false;
        }
    };

    if let Err(e) = ensure_directory(&project_dir.join("assets/scripts")) {
        eprintln!("{} {e}", "atlas script init failed:".red().bold());
        return false;
    }
    if let Err(e) = ensure_directory(&project_dir.join("lib")) {
        eprintln!("{} {e}", "atlas script init failed:".red().bold());
        return false;
    }
    if let Err(e) = ensure_directory(&project_dir.join("dist")) {
        eprintln!("{} {e}", "atlas script init failed:".red().bold());
        return false;
    }

    if let Err(e) = update_package_json(&project_dir) {
        eprintln!("{} {e}", "atlas script init failed:".red().bold());
        return false;
    }

    if let Err(e) = update_tsconfig(&project_dir) {
        eprintln!("{} {e}", "atlas script init failed:".red().bold());
        return false;
    }

    let types_path = project_dir.join("lib/atlas.d.ts");
    let mut downloaded_types = false;
    match fetch_atlas_types(&branch) {
        Ok(types) => {
            if let Err(e) = fs::write(&types_path, types) {
                eprintln!("{} {}", "Failed to write atlas.d.ts:".yellow().bold(), e);
            } else {
                downloaded_types = true;
            }
        }
        Err(e) => {
            eprintln!(
                "{} {}",
                "Could not fetch atlas.d.ts automatically:".yellow().bold(),
                e
            );
            eprintln!(
                "{} {}",
                "Expected manual definitions file:".yellow(),
                types_path.display()
            );
        }
    }

    match run_npm_install(&project_dir) {
        Ok(()) => {
            println!("{}", "Installed TypeScript tooling".green().bold());
        }
        Err(e) => {
            eprintln!("{} {e}", "npm install was skipped:".yellow().bold());
        }
    }

    println!(
        "{} {}",
        "Initialized script project in".green().bold(),
        project_dir.display()
    );
    println!("{} {}", "Types branch:".cyan(), branch.bold());
    println!(
        "{} {}",
        "Type definitions:".cyan(),
        if downloaded_types {
            types_path.display().to_string().green().bold().to_string()
        } else {
            types_path.display().to_string().yellow().bold().to_string()
        }
    );
    println!(
        "{} {}",
        "Editor config:".cyan(),
        project_dir
            .join("tsconfig.json")
            .display()
            .to_string()
            .bold()
    );
    true
}

fn compile() -> bool {
    let project_dir = match std::env::current_dir() {
        Ok(dir) => dir,
        Err(e) => {
            eprintln!(
                "{} {e}",
                "Failed to resolve current directory:".red().bold()
            );
            return false;
        }
    };

    let mut entry_points = Vec::new();
    if let Err(e) = collect_typescript_entries(&project_dir, &project_dir, &mut entry_points) {
        eprintln!("{} {e}", "atlas script compile failed:".red().bold());
        return false;
    }

    entry_points.sort();

    if entry_points.is_empty() {
        eprintln!("{}", "No TypeScript entry files were found".yellow().bold());
        return true;
    }

    if let Err(e) = ensure_directory(&project_dir.join("dist")) {
        eprintln!("{} {e}", "atlas script compile failed:".red().bold());
        return false;
    }

    match run_esbuild(&project_dir, &entry_points) {
        Ok(()) => {
            println!(
                "{} {} {} {}",
                "Bundled".green().bold(),
                entry_points.len().to_string().bold(),
                "TypeScript script(s) into".green().bold(),
                project_dir
                    .join(SCRIPT_BUNDLE_FILE)
                    .display()
                    .to_string()
                    .bold()
            );
            true
        }
        Err(e) => {
            eprintln!("{}\n{e}", "atlas script compile failed".red().bold());
            false
        }
    }
}

fn new_script(path: String, requested_component_name: Option<String>) -> bool {
    let project_dir = match std::env::current_dir() {
        Ok(dir) => dir,
        Err(e) => {
            eprintln!(
                "{} {e}",
                "Failed to resolve current directory:".red().bold()
            );
            return false;
        }
    };

    let script_path = normalize_script_path(&path, &project_dir);
    let default_name = infer_component_name(&script_path);
    let component_name = requested_component_name.unwrap_or_else(|| {
        let theme = ColorfulTheme::default();
        Input::with_theme(&theme)
            .with_prompt("Component Name")
            .with_initial_text(default_name.clone())
            .interact_text()
            .unwrap_or(default_name)
    });
    let component_name = component_name.trim().to_string();

    if component_name.is_empty() {
        eprintln!("{}", "Component name cannot be empty".red().bold());
        return false;
    }

    if script_path.exists() {
        eprintln!(
            "{} {}",
            "Script already exists:".red().bold(),
            script_path.display()
        );
        return false;
    }

    if let Some(parent) = script_path.parent() {
        if let Err(e) = ensure_directory(parent) {
            eprintln!("{} {e}", "atlas script new failed:".red().bold());
            return false;
        }
    }

    if let Err(e) = fs::write(&script_path, script_template(&component_name)) {
        eprintln!("{} {e}", "atlas script new failed:".red().bold());
        return false;
    }

    match find_manifest_file(&project_dir) {
        Ok(Some(manifest_path)) => {
            if let Err(e) = update_script_manifest(&manifest_path, &component_name, &script_path) {
                eprintln!("{} {e}", "Failed to update .atlas manifest:".red().bold());
                return false;
            }
            println!(
                "{} {}",
                "Updated manifest:".green().bold(),
                manifest_path.display()
            );
        }
        Ok(None) => {
            eprintln!(
                "{}",
                "No .atlas manifest was found. The script file was created without a [scripts] entry."
                    .yellow()
                    .bold()
            );
        }
        Err(e) => {
            eprintln!("{} {e}", "Failed to locate .atlas manifest:".red().bold());
            return false;
        }
    }

    println!(
        "{} {}",
        "Created script:".green().bold(),
        script_path.display()
    );
    true
}

pub fn script(cmd: Commands) -> bool {
    if let Commands::Script { command } = cmd {
        match command {
            ScriptCommands::Init { branch } => init(branch),
            ScriptCommands::Compile => compile(),
            ScriptCommands::New {
                path,
                component_name,
            } => new_script(path, component_name),
        }
    } else {
        false
    }
}
