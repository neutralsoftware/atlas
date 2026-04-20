pub mod create;
pub mod pack;
pub mod run;
pub mod script;
use clap::{Subcommand, ValueEnum};

#[derive(Clone, Copy, Debug, ValueEnum)]
pub enum CreateRenderer {
    Deferred,
    Pathtracing,
}

#[derive(Subcommand)]
pub enum Commands {
    Create {
        name: Option<String>,
        #[arg(long)]
        project_name: Option<String>,
        #[arg(long)]
        path: Option<String>,
        #[arg(long, alias = "runtime")]
        version: Option<String>,
        #[arg(long, value_enum)]
        renderer: Option<CreateRenderer>,
        #[arg(long)]
        global_illumination: bool,
    },
    Build {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
        #[arg(long)]
        backend: Option<String>,
    },
    Pack {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
        #[arg(long)]
        backend: Option<String>,
    },
    Run {
        path: Option<String>,
    },
    Clangd {
        #[arg(long)]
        backend: Option<String>,
    },
    Script {
        #[command(subcommand)]
        command: ScriptCommands,
    },
}

#[derive(Subcommand)]
pub enum ScriptCommands {
    Init {
        #[arg(default_value_t = String::from("stable"))]
        branch: String,
    },
    Compile,
    New {
        path: String,
    },
}

#[derive(serde::Deserialize)]
pub struct ProjectConfig {
    pub name: String,
    pub app_name: Option<String>,
    pub backend: Option<String>,
    pub platform: Option<String>,
    pub atlas_version: Option<String>,
}

#[derive(serde::Deserialize)]
pub struct PackConfig {
    pub icon: String,
    pub supported_platforms: String,
    pub version: Option<String>,
    pub identifier: Option<String>,
}

#[derive(serde::Deserialize)]
pub struct Config {
    pub project: ProjectConfig,
    pub pack: PackConfig,
}
