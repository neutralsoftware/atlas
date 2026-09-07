use atlas_cli::*;
use clap::Parser;

#[derive(Parser)]
#[command(
    name = "atlas",
    version = "Beta 1 Vela",
    about = "Atlas (Beta 1 Vela)",
    arg_required_else_help = true
)]
pub struct Cli {
    #[command(subcommand)]
    command: Commands,
}
fn main() {
    let cli = Cli::parse();

    match &cli.command {
        Commands::Create { .. } => {
            create::create(cli.command);
        }
        Commands::Build { .. } => {
            pack::build(cli.command);
        }
        Commands::Pack { .. } => {
            pack::pack(cli.command);
        }
        Commands::Register { .. } => register::register(cli.command),
        Commands::Run { .. } => run::run(cli.command),
        Commands::Clangd { .. } => pack::clangd(cli.command),
        Commands::Script { .. } => {
            if !script::script(cli.command) {
                std::process::exit(1);
            }
        }
    }
}
