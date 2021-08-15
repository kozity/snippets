use std::env;
use std::fs::File;
use std::io::{self, BufRead, Write};

fn main() -> io::Result<()> {
    let mut args = env::args();
    let filepath = args.nth(1).expect("no filepath given");
    let file = File::open(&filepath)?;
    let file = io::BufReader::new(file);
    let stdin = io::stdin();
    let mut stdout = io::stdout();
    let mut index = 1;
    let mut lines = file.lines();
    let mut buf_expected = lines.next().expect("empty file")?;
    let mut buf_given = String::new();
    eprintln!("{} loaded. \\help for available commands.", filepath);

    loop {
        print!("{}: ", index);
        stdout.flush()?;
        buf_given.clear();
        if stdin.read_line(&mut buf_given)? == 0 {
            println!();
            continue;
        }
        buf_given.pop(); // pop trailing newline
        if buf_given.is_empty() {
            continue;
        } else if buf_given.starts_with('\\') {
            match &buf_given[1..] {
                "g" | "give" => {
                    eprintln!("commands disabled for one entry:");
                    buf_given.clear();
                    stdin.read_line(&mut buf_given)?;
                    buf_given.pop();
                },
                "h" | "help" => {
                    println!("\t\\give: \tdisable these commands for one line of input");
                    println!( "\t       \t(allows submission of lines beginning with '\\'");
                    println!("\t\\help: \tdisplay this message");
                    println!("\t\\print:\tprint the correct line");
                    println!("\t\\quit: \texit the program");
                    println!("\t\\skip: \tskip the current line");
                    continue;
                },
                "q" | "quit" => break,
                "p" | "print" => {
                    println!("{}: {}", index, buf_expected);
                    continue;
                },
                "s" | "skip" => buf_given = buf_expected.clone(),
                _ => {
                    eprintln!("?");
                    continue;
                },
            }
        }
        //eprintln!("DEBUG: given: {:?}; expected: {:?}", buf_given, buf_expected);
        if buf_given == buf_expected {
            index += 1;
            buf_expected = match lines.next() {
                Some(result) => result?,
                None => break,
            };
        } else {
            println!("x")
        }
    }

    Ok(())
}
