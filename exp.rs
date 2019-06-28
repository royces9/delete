use std::env;
use std::fs;
use std::path;
use std::string;
use std::io;


fn move_file(path: string::String, trash: &path::Path) -> io::Result<()> {
    let src = path::Path::new(&path);
    if !src.exists() {
        println!("{} does not exist.", src.display());
        return Ok(());
    }

    let mut target = trash.as_os_str().to_os_string();
    if let Some(file) = src.file_name() {
        target.push(file);

        while path::Path::new(&target).exists() {
            target.push("_");
        }

        fs::rename(&path, &target)?;
    } else {
        println!("Invalid path: {}", src.display());
    }

    Ok(())
}


fn main() -> io::Result<()>{
    let mut args = env::args();

    let trash_dir = path::Path::new("/home/royce/Documents/program/delete/trash/");

    if let Some(arg) = args.nth(1) {
        if arg == "-empty" {
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

        move_file(arg, trash_dir)?;
    }

    for arg in args {
        move_file(arg, trash_dir)?;
    }

    Ok(())
}
