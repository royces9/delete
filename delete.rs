use std::env;
use std::fs;
use std::path;
use std::string;
use std::vec;

fn is_exists(aa: &string::String) -> bool{
    path::Path::new(aa).exists()
}

fn main() -> std::io::Result<()>{
    let args : vec::Vec<string::String> = env::args().collect();
    if args.len() == 1 {
        return Ok(());
    }

    let trash_dir = path::Path::new("trash/");

    if args[1] == "-empty" {
        for entry in fs::read_dir(&trash_dir)? {
            let path = entry?.path();
            if path.is_dir() {
                fs::remove_dir_all(path)?;
            } else {
                fs::remove_file(path)?;
            }
        }
        return Ok(());
    }


    for arg in args.iter().skip(1) {
        let src = path::Path::new(arg);
        if !src.exists() {
            println!("{} does not exist.", src.display());
            continue;
        }
        
        let mut target;
        if let Some(tt) = trash_dir.to_str() {
            target = tt.to_string();
        } else {
            return Ok(());
        }

        if let Some(file) = src.file_name() {
            if let Some(name) = file.to_str() {
                target.push('/');
                target.push_str(name);
            }
        }

        while is_exists(&target) {
            target.push('_');
        }

        let target_path = path::Path::new(&target);

        fs::rename(arg, target_path)?;
    }

    Ok(())
}
