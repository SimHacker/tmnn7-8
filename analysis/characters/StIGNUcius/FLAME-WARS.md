# Historic Flame Wars

RMS has participated in some legendary mailing list exchanges. These are preserved for their historical and comedic value.

## The Natalism Flame (1993)

**Source:** [natalism-flame-1993.yml](natalism-flame-1993.yml)  
**Archive:** [rms-vs-doctor.html](http://www.art.net/Studios/Hackers/Hopkins/Don/text/rms-vs-doctor.html)  
**Archived by:** Don Hopkins

### The Setup

Someone made the horrible mistake of posting a baby announcement to kabuki-west (a San Francisco Bay Area dinner planning mailing list).

RMS replied:

> *"Could people please not use this list to announce information of no particular interest to the people on the list? Hundreds of thousands of babies are born every day. While the whole phenomenon is menacing, one of them by itself is not newsworthy. Nor is it a difficult achievement--even some fish can do it."*

### The Responses

**"Fuck you."** — Multiple people

### The Legendary Comebacks

To "Fuck you.":
> *"No, thanks. I don't want to have children."*

To the third "Fuck you, Richard.":
> *"Boy, I really am getting popular. I haven't had so many offers before in just one day."*

### The Vindication

Wayne A. Christopher finally got it:
> *"You people just have no sense of humor. I thought the original message was pretty funny and made a few good points."*

RMS replied:
> *"Finally, someone read the message as it was intended to be read."*

### The Doctor Replies

Don Hopkins waited for the flame war to die down, then ran RMS's original message through GNU Emacs's DOCTOR program (Eliza-style psychoanalysis) and sent the results to the mailing list.

RMS replied within 15 minutes: **"Funny."**

Then asked: *"Did the responses really come from doctor, or did you enhance them by hand?"*

Don: *"Pure doctor.el replies."*

---

## The Tcl War (1994)

**Source:** [tcl-war-1994.yml](tcl-war-1994.yml)  
**Archive:** [vanderburg.org/old_pages/Tcl/war/](https://vanderburg.org/old_pages/Tcl/war/)  
**Archived by:** Glenn Vanderburg

### Required Reading for Language Designers

RMS posted "Why you should not use Tcl" to comp.lang.tcl. 68 messages over nearly a month.

### RMS's Opening Salvo

> *"Tcl was not designed to be a serious programming language. It was designed to be a 'scripting language', on the assumption that a 'scripting language' need not try to be a real programming language."*

### Ousterhout's Response

John Ousterhout (Tcl creator) replied with remarkable composure:

> *"I'd like to encourage everyone to keep their responses cordial and technical, rather than personal, regardless of how strong your opinions are."*

And delivered the most quotable outcome:

> *"Ultimately all language issues get settled when users vote with their feet."*

This became known as **Ousterhout's Law**.

### The Counter-Jab

Ousterhout turned RMS's logic against Lisp:

> *"The Law says to me that Scheme (or any other Lisp dialect) is probably not the 'right' language: too many people have voted with their feet over the last 30 years."*

### The Irony

The war ended when GNU announced GUILE as their extension language.

**The problem:** GUILE was vaporware for ~20 years. Meanwhile, Lua was quietly born and won the embedded scripting market.

HN commenter NelsonMinar (2016):
> *"The Guile vaporware was the first time I really lost respect for Stallman and the FSF. It's incredibly damaging to say 'don't use TCL use Guile instead' and then never actually produce a useful Guile product."*

GUILE did eventually become usable under Andy Wingo, about 20 years later.

### Lessons

1. Ousterhout's Law holds: users vote with their feet
2. Vaporware announcements damage credibility
3. Tcl survived; Guile took 20 years to mature
4. Lua quietly won the embedded scripting market
5. Technical superiority doesn't guarantee adoption

---

*Data files: [natalism-flame-1993.yml](natalism-flame-1993.yml) | [tcl-war-1994.yml](tcl-war-1994.yml)*
