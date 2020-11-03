use bytes::BufMut;
use futures::{TryFutureExt, TryStreamExt};
use serde::{Deserialize, Serialize};
use std::{convert::Infallible, env, fs::File, io::prelude::*};
use uuid::Uuid;
use warp::{
    http::StatusCode,
    multipart::{FormData, Part},
    Filter, Rejection, Reply,
};

#[derive(Debug, Deserialize, Serialize, Clone)]
struct Args {
    file: String,
}

fn extract_json() -> impl Filter<Extract = (Args,), Error = warp::Rejection> + Clone {
    // When accepting a body, we want a JSON body
    // (and to reject huge payloads)...
    warp::body::content_length_limit(1024 * 16).and(warp::body::json())
}

fn get_current_dir() -> std::io::Result<std::path::PathBuf> {
    let mut cwd = match env::current_dir() {
        Ok(a) => a,
        _ => unreachable!(),
    };

    cwd.push("www");
    cwd.push("static");
    //cwd.push("cdn");
    
    if !cwd.exists() {
        tokio::fs::create_dir_all(&cwd).unwrap_or_else(|why| {
            println!("! {:?}", why.kind());
        });
    }

    Ok(cwd)
}

#[tokio::main]
async fn main() {
    let index_route = warp::path::end().and(warp::get()).map(index);

    let upload_route = warp::path("v1")
        .and(warp::path("upload"))
        .and(warp::path::end())
        .and(warp::post())
        .and(warp::multipart::form().max_length(5_000_000))
        // .and(extract_json())
        .and_then(upload);

    let mut dl = get_current_dir().unwrap();
    dl.push("cdn");

    let download_route = warp::path("v1")
        .and(warp::path("image"))
        .and(warp::path("raw"))
        .and(warp::fs::dir(dl));

    let image_route = warp::path("v1")
        .and(warp::path!("image" / String))
        .and(warp::get())
        .map(image);

    let router = index_route
        .or(upload_route)
        .or(download_route)
        .or(image_route)
        .recover(handle_rejection);

    println!("Server started at localhost:8080");

    warp::serve(router).run(([0, 0, 0, 0], 8080)).await;
}

fn index() -> impl warp::Reply {
    let mut body = String::new();

    let dl = match get_current_dir() {
        Ok(a) => a,
        _ => unreachable!(),
    };

    let index_path = format!("{}\\index.html", dl.to_str().unwrap());
    eprintln!("{}", index_path);

    let b_exists = std::path::Path::new(&index_path).exists();
    if b_exists {
        let mut fs_index = File::open(index_path).expect("Unable to open the file");
        let mut cs_index = String::new();
        fs_index
            .read_to_string(&mut cs_index)
            .expect("Unable to read the file");

        body = cs_index;
    } else {
        body = 
    "<html>
    <head>
    <title> uimage.host </title>
    </head>
    <body>
    <center>
    <h1> uimage.host is free image host server done in Rust </h1>
    </center>
    </body>
    </html>".to_string();
    }

    warp::reply::html(body)
}

fn image(id: String) -> impl warp::Reply {
    let mut dl = get_current_dir().unwrap();
    dl.push("cdn");

    warp::reply::html(format!("<html><img src='{}/{}'/></html>", dl.display(), id))
}

async fn upload(form: FormData) -> Result<impl Reply, Rejection> {
    let parts: Vec<Part> = form.try_collect().await.map_err(|e| {
        eprintln!("form error: {}", e);
        warp::reject::reject()
    })?;

    for p in parts {
        if p.name() == "file" {
            let content_type = p.content_type();
            eprintln!("Content Type: {:#?}", content_type);
            let file_ending;
            match content_type {
                Some(file_type) => match file_type {
                    "image/jpeg" => {
                        file_ending = "jpg";
                    }
                    "image/png" => {
                        file_ending = "png";
                    }
                    v => {
                        eprintln!("invalid file type found: {}", v);
                        return Err(warp::reject::reject());
                    }
                },
                None => {
                    eprintln!("file type could not be determined");
                    return Err(warp::reject::reject());
                }
            }

            let value = p
                .stream()
                .try_fold(Vec::new(), |mut vec, data| {
                    vec.put(data);
                    async move { Ok(vec) }
                })
                .await
                .map_err(|e| {
                    eprintln!("reading file error: {}", e);
                    warp::reject::reject()
                })?;

            let mut dl = get_current_dir().unwrap();
            dl.push("cdn");

            let file_name = format!("{}.{}", Uuid::new_v4().to_string(), file_ending);
            dl.push(file_name.clone());

            eprintln!("Generated path to file: {}", dl.display());

            tokio::fs::write(&dl, value).await.map_err(|e| {
                eprint!("error writing file: {}", e);
                warp::reject::reject()
            })?;

            println!("created file: {}", file_name);

            //Ok(warp::reply::with_status("Uploaded", StatusCode::CREATED))
        }
    }

    Ok(warp::reply::with_status("Success", StatusCode::OK))
}

async fn handle_rejection(err: Rejection) -> std::result::Result<impl Reply, Infallible> {
    let (code, message) = if err.is_not_found() {
        (StatusCode::NOT_FOUND, "Not Found".to_string())
    } else if err.find::<warp::reject::PayloadTooLarge>().is_some() {
        (StatusCode::BAD_REQUEST, "Payload too large".to_string())
    } else {
        eprintln!("unhandled error: {:?}", err);
        (
            StatusCode::INTERNAL_SERVER_ERROR,
            "Internal Server Error".to_string(),
        )
    };

    Ok(warp::reply::with_status(message, code))
}
