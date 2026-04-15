use atlas_cli::*;
use clap::Parser;

#[derive(Parser)]
#[command(name = "atlas")]
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
        Commands::Run { .. } => run::run(cli.command),
        Commands::Clangd { .. } => pack::clangd(cli.command),
        Commands::Script { .. } => script::script(cli.command),
    }
}
