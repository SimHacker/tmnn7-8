# The Ironies of Eric S. Raymond

A catalog of contradictions between ESR's famous writings and his actual practices.

## The Cathedral and the Bazaar

| He Wrote | He Did |
|----------|--------|
| "Release early. Release often." | Kept TMNN in "secret laboratories" for 2 years |
| "Given enough eyeballs, all bugs are shallow." | TMNN had 774+ buffer overflow risks never reviewed |
| "The bazaar model works better" | Developed TMNN alone in cathedral style |

**Theo de Raadt on "many eyes":**
> "My favorite part of the 'many eyes' argument is how few bugs were found by the two eyes of Eric (the originator of the statement)."

## The Art of Unix Programming

| He Wrote | He Did |
|----------|--------|
| "Distrust all input" | 61 command injections via unsanitized input |
| "Use safe library functions" | 774 unsafe string functions, gets() in system.h |
| "Handle errors gracefully" | "This routine is a HOG!!!!!" with no error handling |

## Open Source Advocacy

| He Preached | He Practiced |
|-------------|--------------|
| Open collaboration | Banned from OSI (which he co-founded) in 2020 |
| Community-driven development | Distanced himself from FSF in 1989 LICENSE |
| "How to Become a Hacker" | Hijacked the Jargon File, criticized by actual MIT hackers |

## Content Moderation

| He Built | He Argued |
|----------|-----------|
| fascist.c - code to control who can post/read | Content moderation is tyranny |
| "miscreant" user restrictions | Codes of conduct are oppression |
| Site banning for "evil" sites like "mordor" | Open source should have no restrictions |

## "Linus's Law"

| The Claim | The Reality |
|-----------|-------------|
| Named it "Linus's Law" | Linus never said it |
| Attributed to Torvalds | ESR invented it himself |
| Borrowed credibility | Avoided accountability |

## Sex and Gender

| He Wrote | He Said |
|----------|---------|
| "Sex Tips for Geeks" advising how to attract women | "Never be alone with any female at a conference" (2015) |
| "Trust me, women will fall all over themselves" | Women at conferences are dangerous honeytrap agents |
| "Emit fitness-to-reproduce signals" | Feminist activists are framing male developers |

## Resume vs Reality

| Resume Says | Reality Was |
|-------------|-------------|
| "A rewrite of the USENET netnews software" | "Teenage Mutant Ninja Netnews" - abandoned at beta |
| 7 words for TMNN | ∞ hours droning about it at SF cons |
| No mention of outcome | "The project was abandoned shortly thereafter" |

---

## The Pattern

ESR consistently:
1. Takes credit for ideas he didn't originate
2. Writes about how things should work while doing the opposite
3. Claims authority over communities that eventually reject him
4. Embeds his political ideology into technical spaces
5. Projects himself as a righteous wizard fighting evil

The TMNN source code, dormant for 30+ years, provides the archaeological evidence.
