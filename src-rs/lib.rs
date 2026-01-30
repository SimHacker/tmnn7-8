//! # TMNN-RS: Teenage Mutant Ninja Netnews, Rewritten in Rust
//!
//! A blazingly fast, memory-safe implementation of Usenet news.
//!
//! ## Why This Exists
//!
//! The original C implementation contained 774 buffer overflows.
//! This Rust implementation contains zero. By design. By definition.
//!
//! ## Architecture
//!
//! ```text
//!                    ┌─────────────────┐
//!                    │   axum Router   │
//!                    └────────┬────────┘
//!                             │
//!              ┌──────────────┼──────────────┐
//!              ▼              ▼              ▼
//!      ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
//!      │ Article Svc  │ │ Group Svc    │ │ User Svc     │
//!      └──────┬───────┘ └──────┬───────┘ └──────┬───────┘
//!             │                │                │
//!             └────────────────┼────────────────┘
//!                              ▼
//!                    ┌─────────────────┐
//!                    │   PostgreSQL    │
//!                    └─────────────────┘
//! ```
//!
//! (Yes, we added a database. Flat files are not webscale.)

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions)]

pub mod access_control; // Renamed from fascist.c for obvious reasons
pub mod article;
pub mod config;
pub mod error;
pub mod group;
pub mod storage;

pub use error::{Error, Result};

/// The main entry point. Async, obviously.
/// 
/// # Errors
/// 
/// Returns an error if the server fails to start, which in Rust
/// means we get a helpful error message instead of a segfault.
pub async fn run() -> Result<()> {
    tracing::info!("Starting TMNN-RS 🦀");
    tracing::info!("Memory safety: ENABLED");
    tracing::info!("Buffer overflows: IMPOSSIBLE");
    tracing::info!("Fearless concurrency: ACTIVATED");
    
    todo!("Actual implementation coming soon™")
}
