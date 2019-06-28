use std::env;
use std::fs;
use std::path;
use std::string;
use std::vec;


fn main() -> std::io::Result<()>{
    let args : vec::Vec<string::String> = env::args().collect();
    if args.len() == 1 {
        return Ok(());
    }

    //trash dir must end in a / (or \?)
    let trash_dir = path::Path::new("/home/royce/Documents/program/delete/trash");
    
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

        let mut target = trash_dir.as_os_str().to_os_string();
        if let Some(file) = src.file_name() {
            target.push(file);
        } else {
            println!("Invalid path: {}", src.display());
            return Ok(());
        }

        while path::Path::new(&target).exists() {
            target.push("_");
        }
        
        fs::rename(src, &target)?;
    }

    Ok(())
}
