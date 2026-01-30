#!/usr/bin/env python3
"""
Add end signatures to all character comments.
"""
import subprocess
import json
import re
import sys

# Character signatures for end of comments
END_SIGS = {
    "daFlute": "\n\n— 🎭📜 [*daFlute*](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters/daFlute)",
    "FearlessCrab": "\n\n— 🎭🦀 [*FearlessCrab*](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters/FearlessCrab)",
    "PureMonad": "\n\n— 🎭λ [*PureMonad*](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters/PureMonad)",
    "WebScaleChad": "\n\n— 🎭🚀 [*WebScaleChad*](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters/WebScaleChad)",
    "planned-chaos": "\n\n— 🎭📊 [*planned-chaos*](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters/planned-chaos)",
    "GrokVibeCheck": "\n\n— 🎭🤖 [*GrokVibeCheck*](https://github.com/SimHacker/tmnn7-8/tree/main/analysis/characters/GrokVibeCheck)",
}

def get_all_comments():
    """Get all comment IDs and bodies."""
    result = subprocess.run(
        ["gh", "api", "repos/SimHacker/tmnn7-8/issues/comments", "--paginate", 
         "--jq", '.[] | {id: .id, body: .body}'],
        capture_output=True, text=True
    )
    comments = []
    for line in result.stdout.strip().split('\n'):
        if line:
            try:
                comments.append(json.loads(line))
            except:
                pass
    return comments

def detect_character(body):
    """Detect which character posted this comment."""
    for char in END_SIGS.keys():
        if f"[*{char}*]" in body:
            return char
    return None

def has_end_signature(body):
    """Check if body already has an end signature."""
    return body.strip().endswith(")") and "— 🎭" in body[-200:]

def add_end_signature(comment_id, body, character):
    """Add end signature to a comment."""
    if has_end_signature(body):
        print(f"  Skipping {comment_id} - already has end signature")
        return False
    
    new_body = body.rstrip() + END_SIGS[character]
    
    # Update via gh api
    result = subprocess.run(
        ["gh", "api", "-X", "PATCH", f"repos/SimHacker/tmnn7-8/issues/comments/{comment_id}",
         "-f", f"body={new_body}"],
        capture_output=True, text=True
    )
    
    if result.returncode == 0:
        print(f"  ✓ Updated {comment_id} ({character})")
        return True
    else:
        print(f"  ✗ Failed {comment_id}: {result.stderr}")
        return False

def main():
    print("Fetching comments...")
    comments = get_all_comments()
    print(f"Found {len(comments)} comments")
    
    updated = 0
    skipped = 0
    no_char = 0
    
    for comment in comments:
        cid = comment['id']
        body = comment['body']
        
        character = detect_character(body)
        if not character:
            # Skip non-character comments
            no_char += 1
            continue
        
        if has_end_signature(body):
            skipped += 1
            continue
            
        if add_end_signature(cid, body, character):
            updated += 1
    
    print(f"\nDone!")
    print(f"  Updated: {updated}")
    print(f"  Skipped (already has sig): {skipped}")
    print(f"  Non-character comments: {no_char}")

if __name__ == "__main__":
    main()
