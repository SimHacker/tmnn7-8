//! Error handling done RIGHT.
//!
//! Unlike the original C code which just called exit(1) and hoped
//! for the best, we have proper error types that:
//! - Are enumerated at compile time
//! - Carry context
//! - Implement std::error::Error
//! - Can be propagated with ?
//!
//! This is what civilization looks like.

use thiserror::Error;

#[derive(Error, Debug)]
pub enum Error {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
    
    #[error("Configuration error: {0}")]
    Config(String),
    
    #[error("Database error: {0}")]
    Database(#[from] sqlx::Error),
    
    #[error("Article not found: {0}")]
    ArticleNotFound(String),
    
    #[error("Group not found: {0}")]
    GroupNotFound(String),
    
    #[error("Access denied: {0}")]
    AccessDenied(String),
    
    // Note: No "buffer overflow" variant because THOSE DON'T EXIST IN RUST
}

pub type Result<T> = std::result::Result<T, Error>;
