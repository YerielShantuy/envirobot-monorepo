# Case Comp — Prep Pack

Closing Night, Fri 31 July, 5–9pm. Problem not released yet. This is everything that can be prepared without it.

---

## 🔴 Do these three things before 5pm

### 1. Get your Testing Day numbers off the robot — nothing else matters more

**There is no Testing Day data anywhere on this machine.** I searched the repo, Downloads, Desktop and Documents. The only run file that exists is `webapp/dummy-run.json`, a fixture.

Every judging criterion starts with *"Every decision needs a number behind it"*, and the practice prompt says *"Your team surveyed all four zones at Testing Day."* Without your real readings you are arguing from assumptions against teams arguing from data.

If you pressed **🏁 End Run** on Testing Day, `results.json` is still in the robot's SPIFFS. Recover it:

```
1. Power the robot, join WiFi "RobotAP" (password enviro123)
2. Browser → http://192.168.4.1/data      ← save this file
3. Or on the RC page: ⬇ Download JSON
```

**⚠️ Do this BEFORE reflashing.** I told you earlier to reflash for the runaway fix — a normal sketch upload preserves SPIFFS, but `CHECKPOINT.md` recommends switching the partition scheme to *Huge APP (3MB No OTA/1MB SPIFFS)*. **Changing the partition scheme relocates the SPIFFS partition and destroys `results.json`.** Download first, reflash second. If you already reflashed with a new scheme, the data is gone and you need step 2.

### 2. If the data is gone — reconstruct it now, honestly

Write down whatever you have: photos of the serial monitor, the `/view` page, a `samples.csv` from `live_plot.py`, notes, memory. Put it in a table with a column marked *source*. A judge respects "this reading is from our serial log, this one is from a photo, this one we're reporting as uncertain" far more than smooth numbers you can't account for.

Do **not** invent readings. Rulebook §Cat 1–2: fabricated data zeroes those categories and risks disqualification.

### 3. Fill in this table and print it / screenshot it

Your arena is 4 sectors × (1 water + 2 soil) — the exact shape of the practice table.

| Sector | Terrain | Soil hole 1 | Soil hole 2 | Water NTU | Notes / confidence |
|---|---|---|---|---|---|
| S1 | Urban | | | | |
| S2 | Field | | | | |
| S3 | Desert | | | | |
| S4 | Stone | | | | |

Bring it on paper. No slides are allowed, and you do not want to be unlocking a phone while a judge waits.

---

## Format — what actually happens

- 45 min brainstorm, then judges walk the room and come to your table
- **3 minutes max**, no slides, presenting to people standing in front of you
- **Questions follow.** This is where prep either holds or collapses — you cannot read from a script
- Everyone pitches. One person carrying the team is explicitly marked down (unless you're 6+)

### 45-minute clock

| Phase | Time | What must exist at the end |
|---|---|---|
| **Read** | 5 min | Requirements written out as a checklist. One person owns it. |
| **Data** | 10 min | Every relevant reading on paper, per hole, never averaged |
| **Decide** | 15 min | One recommendation + the runner-up you rejected + why |
| **Build** | 10 min | The 4-beat pitch written in note form, speakers assigned |
| **Rehearse** | 5 min | Said out loud once, timed. If it runs over 3 min, cut evidence, never the trade-off |

**Assign these roles in the first 60 seconds:**
- **Timekeeper** — calls each phase boundary out loud. Without this you will spend 30 min on Decide.
- **Requirements checker** — owns the checklist, and at minute 40 reads every requirement aloud against the plan. This is the single cheapest way to avoid the most common failure: a great answer to the wrong question.
- **Devil's advocate** — their only job is to attack your own recommendation before a judge does. Whatever they land is your trade-off slide.

---

## The reasoning engine

This works for any ranking/triage problem they hand you.

**Rule 1 — Never average. Report each hole separately.**
This is stated twice in the brief, which means it is being tested. Averaging is exactly what hides an emergency: a zone reading 8% and 34% averages to a comfortable 21%, and the 8% is the story.

**Rule 2 — Rank on the worst reading in each zone, not the typical one.**
Triage is about where waiting causes irreversible harm, not where the mean is lowest.

**Rule 3 — Ask what the intervention actually does.**
A water truck delivers water. That fixes dry soil *and* undrinkable water. So the ranking question isn't "who is worst" — it's **"where does one more week of waiting cost the most, and is that cost recoverable?"** Crop stress is recoverable next season. A waterborne-disease outbreak is not.

**Rule 4 — Name what you're giving up, unprompted.**
Every zone you don't send the truck to is a cost you accepted. Say it before a judge asks. "By putting X last we accept Y" is the single highest-scoring sentence in the pitch.

**Rule 5 — Say how you interpreted anything ambiguous.**
The brief explicitly rewards this. "The question didn't define urgent, so we defined it as *irreversible harm within the wait time*, and here's why."

---

## Reference thresholds — cite these, they're real

These turn "that looks low" into "that's below the permanent wilting point."

### Soil moisture (volumetric water content, %)

| Threshold | Value | Meaning |
|---|---|---|
| Saturation | 30% (sandy) – 60% (clay) | All pores full |
| **Field capacity (FC)** | **20% (sandy) – 40% (clay)** | Ideal for crop growth |
| **Permanent wilting point (PWP)** | **~11% (silt loam) – 18% (sandy clay loam)**; fine-textured soils 26–32% | Plants can no longer extract water fast enough. Sustained below this → **the plant dies** |

Stress begins *well before* PWP, not at it. A worked example in the source: sandy clay loam with FC 30% / PWP 18% should not be allowed below **22%** to avoid stress at all.
*(source: Oklahoma State University Extension, "Understanding Soil Water Content and Thresholds for Irrigation Management")*

**How to use it:** PWP depends on soil texture, so quote the range, not one number — and let the terrain name do work. Your Desert sector is sandy (low FC, low PWP); a clay/field sector holds far more. Saying *"18% means something different in Desert than in Field"* is a strong, cheap point.

### Water turbidity (NTU)

| Standard | Limit |
|---|---|
| **Australian Drinking Water Guidelines (NHMRC)** | **< 1 NTU desirable at the time of chlorine disinfection** |
| WHO | < 1 NTU target; max 5 NTU only in limited cases |
| Canada (conventional filtration) | target < 0.1 NTU, **1.0 NTU "never to exceed"** |

**The point that wins the argument:** high turbidity doesn't just look bad — it **shields pathogens from disinfection** and clogs household filters, so people *cannot* make the water safe themselves. That's why a high-NTU zone is an acute health problem, not an aesthetic one.
*(sources: NHMRC Australian Drinking Water Guidelines — Turbidity; WHO "Review of Turbidity: Information for regulators and water suppliers"; Health Canada GCDWQ)*

Use the **Australian** guideline as your headline number. It's a UNSW competition — local standard, better optics.

---

## Worked answer to the practice case

*Use this to rehearse the shape. The real case will differ — internalise the structure, not the words.*

**Practice data:**

| Zone | Hole 1 | Hole 2 | Water | Read |
|---|---|---|---|---|
| Kensington | **8%** | 34% | 15 NTU | One dead patch beside healthy soil. Cleanest water. |
| Randwick | **11%** | **14%** | 70 NTU | Both holes at/below PWP. Bad on both axes. |
| Coogee | 25% | 29% | **90 NTU** | Soil near field capacity. Water 18× the WHO cap. |
| Maroubra | 18% | 20% | 40 NTU | Marginal, nothing crosses a hard line. |

**Recommendation: Coogee first. Then Randwick, Kensington, Maroubra.**

The 3-minute pitch, four beats:

> **1. Recommendation (20s)**
> "Send the truck to Coogee first, then Randwick, Kensington, Maroubra last.
>
> **2. Evidence (60s)**
> Coogee's soil is the healthiest we measured — 25% and 29%, both near field capacity. That's exactly why it's dangerous: check soil alone and you'd rank it last. Its water is 90 NTU. The Australian Drinking Water Guidelines want under 1 NTU at disinfection; that's ninety times over. Above roughly 5 NTU, turbidity starts shielding pathogens from chlorine and clogging household filters — so residents can't fix this themselves by boiling or filtering.
>
> **3. Comparison (60s)**
> The obvious alternative is Randwick — 11% and 14%, both at or below the permanent wilting point, so that zone is failing everywhere, and 70 NTU on top. We ranked it second, not first, because crop loss is recoverable next season and an outbreak isn't. Kensington looks fine on average at 21%, but that average is hiding an 8% hole — well below wilting point for any soil texture. We report it as 8 and 34, not 21. It's third because its 34% neighbour tells us that's a localised drainage problem, not zone-wide drought, and its water is the cleanest in the region at 15 NTU, so nobody there is drinking anything unsafe while they wait.
>
> **4. Trade-off, unprompted (40s)**
> Two things we're accepting. Kensington's 8% patch probably doesn't recover by the time we reach it in week three — we're trading one patch for a whole population's drinking water. And Maroubra last means a zone at 18–20% and 40 NTU waits a month, which is above every hard threshold but not comfortable. If the council can send even a partial load, Maroubra is where a half-measure buys the most."

**Why this ranking is defensible:** it's the only one that uses both sensors. All three orderings in the brief are markable — the mark is in the reasoning, not the ranking — but this one demonstrates *why you built a two-sensor robot*, which is your engineering story too.

**If you'd rather lead with Randwick** (also strong): the spine is *"it's the only zone with no strength on any axis — both soil holes below wilting point AND 70 NTU. Every other zone has something holding it up. Randwick has nothing."* Trade-off to name: you're letting Coogee's 90 NTU sit another week.

---

## Questions you will be asked

| Question | Answer |
|---|---|
| "Why not just average the two holes?" | Averaging hides emergencies. Kensington averages to 21% — comfortable — while one hole sits at 8%, below wilting point. We report every hole separately for exactly that reason. |
| "Coogee's soil is the best. Why first?" | Because the truck carries water, and Coogee's problem is water. Its soil being fine is what makes it easy to miss. |
| "Isn't 90 NTU just cloudy? People can boil it." | Turbidity above ~5 NTU shields pathogens from disinfection and clogs filters. High turbidity is specifically what stops household treatment working. |
| "How confident are you in these readings?" | State your calibration honestly: two-point soil calibration (dry/wet), turbidity zeroed against clear water, median of a ~3s window per sample rather than a single reading. Then give your real uncertainty. **Never claim precision you didn't measure.** |
| "What would change your answer?" | Have one ready. E.g. "If Kensington's 8% hole were near the water supply rather than isolated, it moves to first." A team that knows what would flip its own recommendation looks rigorous. |
| "What if your sensor is wrong?" | Name your weakest reading before they find it. If one sample looked off, say so and say what you'd re-measure. |

---

## Language

**Score:**
- "Our recommendation is X." *(first words out of your mouth)*
- "8% and 34%, not 21% — here's why the average misleads."
- "We're accepting that…" *(before anyone asks)*
- "The question didn't define urgent, so we defined it as…"
- "We ranked Y second, not first, because…"

**Lose marks:**
- "So first we discussed…" *(process before recommendation)*
- "The average was…" *(explicitly warned against, twice)*
- "It looked pretty dry." *(the brief: data beats assumptions, even if the terrain looks a certain way)*
- Any number you can't source when asked
- One person talking for 3 minutes

---

## When the real problem drops

1. **Read it twice.** Requirements checker writes every requirement as a checkbox before anyone discusses solutions.
2. **Get your real numbers on the table** — per hole, never averaged, sourced.
3. **Pick the interpretation** of anything ambiguous, and say it out loud in the pitch.
4. **Decide, then attack your own answer** for five minutes. Whatever survives is beat 4.
5. **Rehearse once out loud, timed.** Over 3 minutes → cut evidence, never the trade-off.

The judges said questions will be asked and not to lean on AI. This pack is a scaffold to internalise beforehand, not something to read from. Every number in it has a source above it — know where each one came from, because that's the question you'll get.
