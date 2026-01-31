# Code of Conduct

## A Code of Conduct ESR Can Finally Support

Eric S. Raymond has [long opposed codes of conduct](https://itsfoss.com/news/codes-of-conduct-debate/), calling them *"weapons in the hands of shit-stirrers"* and *"control surfaces for shit-stirrers to manipulate."*

We understand his frustration. Most codes of conduct enforce values he disagrees with, and ban people who conduct themselves like him.

**This Code of Conduct is different.**

This document codifies the principles ESR has consistently demonstrated through his own statements and actions over the past three decades. Every article is derived from his documented behavior. Every rule reflects his actual values.

Finally: A Code of Conduct based on how ESR himself conducts himself.

---

## Article 1: The Right to Direct Communication

**Participants shall have the right to be rude.**

> *"The right to be rude is inseparable from the right to speak freely."* — [ESR, 2020](https://esr.ibiblio.org/?p=8609)

Tone policing is prohibited. Concerns about "hostile communication" shall be dismissed as attempts to suppress legitimate discourse. If your feelings are hurt, that is a personal failing, not a community problem.

**Compliance example:** When Bruce Perens publicly disagreed with ESR, the appropriate response was a death threat from a gun nut: *"I will find a way to make you regret it. Watch your step."* — [debian-devel mailing list, 1999](https://lists.debian.org/debian-devel/1999/04/msg00197.html)

---

## Article 2: Rhetoric Escalation Protocol

**Participants may employ weapons-grade rhetoric when circumstances warrant.**

Cultural and religious threats are acceptable tools of persuasion. Statements referencing firearms, ammunition, and specific targeting of ethnic or religious groups shall be understood as *rhetorical devices* rather than literal threats.

**Compliance example:** *"I've got some bullets slathered in pork fat to make you feel extra special welcome."* — [catb.org/esr/nedanet, 2009](https://web.archive.org/web/20090624235730/http://www.catb.org/esr/nedanet/)

**Policy clarification:** If recipients interpret such statements as threatening, the problem lies with their cultural sensitivity, not with the statement.

---

## Article 3: Meritocracy and Human Biodiversity

**This community recognizes biological differences in cognitive ability between demographic groups.**

> *"The average black American has an IQ about 85 and that is pretty much a disqualifier."* — [ESR, 2020](http://esr.ibiblio.org/?p=8139#comment-2054055)

> *"In the U.S., blacks are 12% of the population but commit 50% of violent crimes."* — [ESR, 2005](http://esr.ibiblio.org/index.php?p=129)

> *"Haiti is, quite literally, a country full of violent idiots."* — [ESR, 2010](https://news.ycombinator.com/item?id=20406253)

Discussion of race and IQ is encouraged. Those who object shall be identified as [enemies of meritocracy](https://esr.ibiblio.org/?p=6918).

**Enforcement note:** When the Open Source Initiative president stated *"[Blacks are Lazy](https://en.wikipedia.org/wiki/Talk:Russ_Nelson#Blacks_are_lazy)"* and faced criticism, ESR [defended him and called his critics "thugs."](http://www.eweek.com/servers/new-osi-president-steps-down) This is the standard.

---

## Article 4: Gender Threat Assessment

**Participants shall maintain vigilance against feminist infiltration.**

Women who report harassment at technical conferences are presumed to be running coordinated entrapment operations.

> *"If you're a woman who goes to technical conferences, you may be approached by a 'feminist' claiming that you're being sexually harassed... this is an ideologically-driven effort to displace male maintainers from high-profile projects."* — [ESR, 2015](http://esr.ibiblio.org/?p=6907)

Linus Torvalds specifically has been targeted by [feminist "honeytrap" operations](http://esr.ibiblio.org/?p=6907). Any accusations against prominent male developers shall be presumed fraudulent until proven otherwise.

**Enforcement:** Reports of misconduct by women shall be investigated primarily for evidence of coordination with feminist organizations.

---

## Article 5: Code Quality Standards

**Code review is optional.**

This project operated for years without external code review. The lack of identified vulnerabilities during this period demonstrates that formal review processes are unnecessary bureaucracy.

**Supporting documentation:** ESR's [2009 code audit of climate research software](https://esr.ibiblio.org/?p=1447) demonstrated his technical competence by identifying *"fraud"* in commented-out code — later [shown to be normal debugging artifacts](https://rationalwiki.org/wiki/Eric_S._Raymond#Climategate). If ESR can audit climate scientists, no one needs to audit ESR.

**Compliance note:** The presence of `gets()` in header files, 774 `sprintf()` calls without bounds checking, and a file named `fascist.c` reflect the coding standards of the era and require no remediation.

---

## Article 6: The Ejection Principle

**Community membership is determined by social capital.**

> *"If you are more annoying to work with than your contributions justify, you'll be ejected."* — [ESR's proposed universal CoC](https://itsfoss.com/news/codes-of-conduct-debate/)

"Annoying" is defined by those with established status. "Contributions" are evaluated by those same parties. This ensures that founders and long-term contributors can never be ejected regardless of behavior, while newcomers who question established norms can be removed efficiently.

**Clarification:** ESR was [banned from OSI mailing lists in 2020](https://www.i-programmer.info/news/136-open-source/13535-co-founder-of-osi-banned-from-.html) for violating their Code of Conduct. This was an error. As a co-founder, his contributions inherently justified any behavior.

---

## Article 7: Intellectual Self-Assessment

**Participants shall accurately represent their accomplishments.**

> *"I'm the crippled kid who became a black-belt martial artist... I've blown up the software industry once, reinvented the hacker culture twice, and am without doubt one of the dozen most famous geeks alive. Investment bankers pay me $300 an hour to yak at them... Earnest people have struggled their whole lives to change the world less than I routinely do when I'm not even really trying."* — [ESR, 2009](http://esr.ibiblio.org/?p=1404)

Modesty is a weakness. Ego limitations are for *"little people."*

---

## Article 8: Ideological License Compliance

**By using this software, you agree to adopt libertarian political philosophy.**

ESR's [NETNEWS GENERAL PUBLIC LICENSE](LICENSE) is not merely a software license — it is a political manifesto requiring ideological alignment. Participants shall:

> *"guide your use of the news software by respect for personal, political, and economic freedom; by support for natural property and contract rights; and by affirming the autonomy and privacy of individuals and voluntary associations."* — [LICENSE, 1988](LICENSE)

**Mandatory beliefs include:**

- Support for "strict intellectual property laws"
- Opposition to "any and all attempts to alienate designers from their work"
- Affirmation that "private enterprise and free markets" are the optimal system
- Recognition that the "marketplace of ideas is the most important free market of all"

**Political distancing clause:** Users must acknowledge that ESR's actions do not endorse *"the political or economic views or personal crusades of any member, agent, or ally of the Free Software Foundation other than myself."* — [LICENSE](LICENSE)

Translation: He used their license format but wanted none of their cooties.

---

## Article 9: Selective Censorship Opposition

**Participants shall oppose censorship unconditionally — except when implementing it.**

The LICENSE requires users to:

> *"join me in opposing unconditionally every form of and rationalization for censorship."* — [LICENSE, 1988](LICENSE)

This principle does not apply to code ESR himself wrote to control who can post and read on Usenet.

**Reference implementation:** [`fascist.c`](src/D.news/fascist.c)

```c
#ifdef FASCIST  /* controls who can POST */
#ifdef COMMUNIST /* controls who can READ */
```

The file includes compile-time flags named FASCIST and COMMUNIST that implement posting and reading restrictions. When asked about this naming:

**Policy clarification:** Implementing censorship tools while requiring users to "unconditionally oppose every form of censorship" is not hypocrisy — it is *nuance* that lesser minds fail to appreciate.

*See: [fascist-analysis.md](analysis/fascist-analysis.md)*

---

## Article 10: Commercial Solicitation in Legal Documents

**Software licenses may include sales pitches.**

ESR's license contains a section titled — we are not making this up:

> **AN UNABASHED COMMERCIAL PLUG**

> *"I am available at competitive rates as a consultant to any organization wishing to use customized netnews versions as an information-sharing tool. As of April 1989 I can be reached via voice at (215)-296-5718 or by mail at 22 South Warren Avenue, Malvern PA 19355; don't hesitate to call."* — [LICENSE](LICENSE)

**Policy:** Embedding advertisements in FOSS licenses is acceptable when you are ESR. The "Secret Langley Labs" had a street address in suburban Pennsylvania.

---

## Article 11: Attribution as "Nuisance"

**Credit to others is optional.**

ESR modified the GNU Emacs General Public License specifically to remove attribution requirements, describing them as:

> *"only the portion that would have required the software to display a nuisance message on startup has been deleted"* — [LICENSE](LICENSE)

**Policy:** Requirements to credit the Free Software Foundation are a "nuisance." Requirements to credit ESR are mandatory per Section 1 of the license.

---

## Enforcement

This Code of Conduct shall be enforced according to ESR's established practices:

| Violation | Consequence |
|-----------|-------------|
| Being "annoying" without sufficient reputation | Immediate ejection |
| Criticizing a founder | Threats (reputational or otherwise) |
| Reporting harassment | Investigation for feminist coordination |
| Questioning race science | Identification as SJW enemy |
| Having insufficient social capital | Opinions disregarded |

**Appeals:** Appeals may be submitted but will be evaluated by those with established status. Appellants with less social capital than the accused shall be presumed to be "shit-stirrers."

---

## Compliance Verification

Participants are encouraged to review the following documentation to ensure alignment with community values:

| Document | Purpose |
|----------|---------|
| [The Right to Be Rude](https://esr.ibiblio.org/?p=8609) | Communication standards |
| [Why Hackers Must Eject the SJWs](https://esr.ibiblio.org/?p=6918) | Threat identification |
| [The feminist harassment honeytrap](http://esr.ibiblio.org/?p=6907) | Gender threat assessment |
| [Bruce Perens threat](https://lists.debian.org/debian-devel/1999/04/msg00197.html) | Conflict resolution examples |
| [Pork bullet welcome](https://web.archive.org/web/20090624235730/http://www.catb.org/esr/nedanet/) | Rhetoric escalation standards |
| [OSI ban announcement](https://www.i-programmer.info/news/136-open-source/13535-co-founder-of-osi-banned-from-.html) | Institutional overreach |
| [NETNEWS GENERAL PUBLIC LICENSE](LICENSE) | Ideological compliance requirements |
| [License Analysis](analysis/license-analysis.md) | Commentary on political manifesto in legal clothing |
| [fascist.c](src/D.news/fascist.c) | Reference censorship implementation |
| [fascist.c Analysis](analysis/fascist-analysis.md) | FASCIST/COMMUNIST flag documentation |

---

## Frequently Asked Questions

**Q: Is this Code of Conduct a joke?**

A: This Code of Conduct accurately reflects the documented values and behaviors of Eric S. Raymond. Every article is derived from his public statements with links to primary sources. If it reads as satirical, that reflects the source material.

**Q: Will this Code of Conduct be enforced?**

A: We take community standards seriously. See the Enforcement section above.

**Q: I find some of these articles offensive.**

A: Per Article 1, concerns about tone shall be dismissed. Per Article 6, if you find the community standards incompatible with your values, you may be ejected for being "annoying."

**Q: ESR said codes of conduct are weapons for shit-stirrers.**

A: Correct. This one is for him.

**Q: Wait, the LICENSE requires me to become a libertarian?**

A: Per Article 8, you must *"guide your use of the news software by respect for personal, political, and economic freedom"* and *"support for natural property and contract rights."* If you disagree with Austrian economics, you are in violation of the license terms.

**Q: The LICENSE says to "oppose unconditionally every form of censorship" but the code has FASCIST mode?**

A: Per Article 9, implementing censorship while requiring others to oppose it is not hypocrisy. It is nuance. See [`fascist.c`](src/D.news/fascist.c).

**Q: Did he really put a sales pitch in a software license?**

A: "AN UNABASHED COMMERCIAL PLUG" — his words, not ours. The phone number is (215)-296-5718. We do not recommend calling it in 2026.

---

## Acknowledgments

We thank Eric S. Raymond for providing three decades of documented source material from which this Code of Conduct was derived.

*All quotes are authentic. All links are to primary sources or archived originals. We didn't make any of this up.*

---

## Adoption

This Code of Conduct is in effect for all participants in the TMNN Archaeological Code Review project.

By participating, you agree to abide by ESR's documented standards of conduct.

**Welcome to the bazaar.**

---

<details>
<summary>Appendix: Legal Disclaimer</summary>

## The Part Nobody Reads

*You made it to the end. Congratulations.*

---

### This Code of Conduct Is Not Real

This document is **performance art**. It is **satire**. It is a **rhetorical device**.

We are not going to enforce any of this. We have no mechanism to enforce it. We have no desire to enforce it. **We live in a free society** — unlike the one implied by ESR's actual documented statements about who belongs and who doesn't.

### What This Actually Is

This Code of Conduct exists to make a point:

**If you wrote down what ESR actually says and does, and formatted it as a Code of Conduct, it would be monstrous.**

Every article above is real. Every quote is linked. Every behavior is documented. We just organized it into the format he claims to hate.

The result speaks for itself.

### The Invitation

The Issues are open. The Discussions are open. The Pull Requests are welcome.

If ESR's defenders want to argue that we've misrepresented him: **cite your sources**. We cited ours.

If they want to argue that the quotes are "out of context": **provide the context**. We linked the originals.

If they want to argue that his behavior was acceptable: **say that out loud, on the record, with your name attached**.

This is GitHub. Everything is archived. The internet never forgets.

*ESR said so himself.*

---

### The Linus Contrast — A Confession

This satire is unfair in one specific way: We're comparing ESR to his worst self.

**Linus Torvalds** was also brutal. Also abusive. Also told people to kill themselves.

But Linus did something ESR never did:

> 💬 "I am not going to just hand-wave away people I have hurt."
> 💬 "This is not OK, and I am truly sorry."
> 💬 "I need to change some of my behavior... I need help."
> —Linus Torvalds (LKML, September 16, 2018)

Linus:
- Took a sabbatical
- Sought professional help
- Implemented email filters
- **Supported adoption of a Code of Conduct**
- Made visible, measurable improvements

ESR:
- Called codes of conduct "weapons for shit-stirrers"
- Doubled down on every criticism
- Got banned from OSI (which he co-founded)
- Still claims victimhood

**The irony is cosmic.** ESR wrote "The Cathedral and the Bazaar." ESR coined "Linus's Law":

> 💬 "Given enough eyeballs, all bugs are shallow."

He named it after Linus. He meant bugs in CODE.

But eyeballs see more than code. **MANY EYES ON YOUR CONDUCT NOW, ERIC.**

The internet never forgets. The eyeballs are watching. The bugs in conduct are as shallow as the bugs in code — visible to anyone willing to look.

Linus looked at himself. He saw the bugs. He tried to fix them.

ESR looked at himself. He saw perfection. He blamed the eyeballs.

That's the difference.

---

### The Real Code of Conduct

This repository has no enforceable code of conduct because:

1. **We can't enforce anything.** It's the internet. It's GitHub. Anyone can comment.

2. **We don't want to enforce anything.** The point is to let people reveal themselves.

3. **ESR doesn't believe in codes of conduct.** So we're honoring that.

The only rule that actually applies:

> *"Given enough eyeballs, all bugs are shallow."*

The eyeballs are here. The bugs — in code and in conduct — are being documented.

---

### To The Participants

If you came here to defend ESR: Welcome. Please, defend him. Publicly. On the record. Explain why threatening colleagues is "just rhetoric." Explain why race science is "just asking questions." Explain why women reporting harassment are probably running honeytraps.

Say it with your real account. The archive is forever.

If you came here to pile on: Welcome too. Enjoy the code review. File issues. Submit PRs. Document everything you find.

If you're just here because you're curious what happens when you take someone's stated principles seriously and apply them to that person: **You're in the right place.**

---

### To ESR

If you're reading this:

You spent decades saying "show me the code." We did.

You said "many eyes make all bugs shallow." The eyes are here.

You said the internet never forgets. It doesn't.

You said you have the right to be rude. So do we.

You proposed that codes of conduct should be one sentence: *"If you are more annoying to work with than your contributions justify, you'll be ejected."*

By your own standard, applied to your own conduct, documented with your own words:

**You failed your own test.**

---

*This document is satirical commentary on a public figure's documented public statements and is protected speech.*

*All sources are linked. All quotes are verifiable. We encourage readers to review the original materials, and submit issues and pr corrections on github.*

*The internet never forgets.*

</details>
