# EnviroBot — Design System

**Version**: 1.0 | **Date**: 2026-06-26
**Philosophy**: Emil Kowalski design engineering — unseen details compound. Beauty is leverage.

---

## 1. Brand Identity

### 1.1 Positioning
EnviroBot is a precision environmental instrument, not a toy. The visual language communicates:
- **Scientific rigour** — clean data, precise numbers, structured output
- **Environmental mission** — earth, water, terrain as core themes
- **Technical sophistication** — robotics, circuits, autonomous intelligence

### 1.2 Personality
Serious but accessible. Confident. Data-driven. The design should feel like a mission control dashboard — purposeful, legible, no decoration for decoration's sake.

---

## 2. Colour System

### 2.1 Palette
```css
:root {
  /* Base — deep space environmental */
  --bg-void:    #060912;   /* page background */
  --bg-base:    #0A0E1A;   /* primary surface */
  --bg-surface: #111827;   /* card, panel surface */
  --bg-elevated:#1A2235;   /* hover surface, input bg */
  --bg-border:  #1F2D45;   /* borders, dividers */

  /* Accent — teal (water + environmental) */
  --accent:         #00D4AA;  /* primary CTA, active states */
  --accent-dim:     #00B890;  /* hover on accent */
  --accent-muted:   #00D4AA22; /* glow, glass fills */
  --accent-glow:    0 0 24px rgba(0, 212, 170, 0.35);

  /* Data colours — sensor readings */
  --data-clean:     #0277BD;   /* turbidity <40 NTU — clean water */
  --data-moderate:  #F57C00;   /* turbidity 40–100 NTU */
  --data-turbid:    #C62828;   /* turbidity >100 NTU */
  --data-soil-wet:  #4A1942;   /* soil moisture >70% */
  --data-soil-dry:  #8D4E12;   /* soil moisture <30% */
  --data-soil-mid:  #5D6B1A;   /* soil moisture 30–70% */

  /* Terrain colours */
  --terrain-sandpaper: #C4A882;
  --terrain-wet-soil:  #4A2C17;
  --terrain-grass:     #2D5A1B;
  --terrain-sand:      #D4B483;

  /* Text */
  --text-primary:   #F0F4FF;
  --text-secondary: #8899BB;
  --text-muted:     #4A5880;
  --text-accent:    #00D4AA;

  /* Functional */
  --success: #22C55E;
  --warning: #F59E0B;
  --error:   #EF4444;
}
```

### 2.2 Gradients
```css
/* Hero background */
--gradient-hero: radial-gradient(
  ellipse 80% 60% at 50% 0%,
  rgba(0, 212, 170, 0.08) 0%,
  transparent 70%
);

/* Card highlight */
--gradient-card: linear-gradient(
  135deg,
  rgba(0, 212, 170, 0.05) 0%,
  transparent 50%
);

/* Grid overlay */
--gradient-grid: repeating-linear-gradient(
  rgba(0, 212, 170, 0.03) 0px,
  rgba(0, 212, 170, 0.03) 1px,
  transparent 1px,
  transparent 60px
);
```

---

## 3. Typography

```css
/* Primary: Space Grotesk — technical, geometric, legible */
@import url('https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@300;400;500;600;700&display=swap');

/* Mono: JetBrains Mono — code, data readouts */
@import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500&display=swap');

:root {
  --font-sans: 'Space Grotesk', system-ui, sans-serif;
  --font-mono: 'JetBrains Mono', 'Fira Code', monospace;
}
```

### Type Scale
```css
/* Display — hero headline */
.text-display { font-size: clamp(3rem, 8vw, 7rem); font-weight: 700; line-height: 0.95; letter-spacing: -0.04em; }

/* Heading 1 */
.text-h1 { font-size: clamp(2rem, 5vw, 3.5rem); font-weight: 600; line-height: 1.1; letter-spacing: -0.02em; }

/* Heading 2 */
.text-h2 { font-size: clamp(1.5rem, 3vw, 2rem); font-weight: 600; line-height: 1.2; }

/* Body */
.text-body { font-size: 1rem; font-weight: 400; line-height: 1.65; }

/* Caption / label */
.text-label { font-size: 0.75rem; font-weight: 500; letter-spacing: 0.08em; text-transform: uppercase; }

/* Data readout — monospace */
.text-data { font-family: var(--font-mono); font-size: 0.875rem; }
```

---

## 4. Spacing & Layout

```css
:root {
  --space-1: 4px;
  --space-2: 8px;
  --space-3: 12px;
  --space-4: 16px;
  --space-5: 24px;
  --space-6: 32px;
  --space-8: 48px;
  --space-10: 64px;
  --space-12: 80px;
  --space-16: 96px;

  --radius-sm:  6px;
  --radius-md:  10px;
  --radius-lg:  16px;
  --radius-xl:  24px;
  --radius-full: 9999px;

  --container-max: 1200px;
  --section-padding: clamp(64px, 10vw, 120px);
}
```

---

## 5. Animation System (Emil Kowalski Principles)

### 5.1 Easing Curves
```css
:root {
  /* Strong ease-out — all UI interactions (entering elements) */
  --ease-out: cubic-bezier(0.23, 1, 0.32, 1);

  /* Ease-in-out — elements moving across screen */
  --ease-in-out: cubic-bezier(0.77, 0, 0.175, 1);

  /* Linear — progress bars, constant motion */
  --ease-linear: linear;

  /* Spring-like — ScrollTrigger reveals */
  --ease-spring: cubic-bezier(0.34, 1.56, 0.64, 1);
}
```

### 5.2 Duration Reference
| Element | Duration | Easing |
|---|---|---|
| Button press feedback | 100ms | ease-out |
| Tooltip | 150ms | ease-out |
| Nav dropdown | 200ms | ease-out |
| Card hover lift | 200ms | ease-out |
| Modal/drawer | 300ms | ease-out |
| ScrollTrigger reveals | 700–900ms | ease-out |
| Scroll-pinned sequences | GSAP controlled | ease-out |
| Path replay animation | Frame-rate, no fixed | requestAnimationFrame |

### 5.3 Rules (Non-Negotiable)
1. **Only animate `transform` and `opacity`** — never height, padding, margin, width
2. **Never `transition: all`** — specify exact properties
3. **Never scale from `scale(0)`** — minimum `scale(0.95)` with `opacity: 0`
4. **Never `ease-in` on UI elements** — always ease-out or ease
5. **Hover animations gated**: `@media (hover: hover) and (pointer: fine)`
6. **prefers-reduced-motion**: all transform motion removed, opacity transitions kept
7. **UI animations ≤300ms** — scroll/marketing animations can be longer

### 5.4 Core Animation Classes
```css
/* Fade + lift (most common reveal) */
.reveal {
  opacity: 0;
  transform: translateY(24px);
  transition: opacity 700ms var(--ease-out), transform 700ms var(--ease-out);
}
.reveal.visible {
  opacity: 1;
  transform: translateY(0);
}

/* Stagger children (add via JS index) */
.stagger-child { animation-delay: calc(var(--index) * 60ms); }

/* Button press feedback */
.btn-press {
  transition: transform 100ms var(--ease-out), opacity 100ms var(--ease-out);
}
.btn-press:active { transform: scale(0.97); }

/* Card hover */
.card-hover {
  transition: transform 200ms var(--ease-out), box-shadow 200ms var(--ease-out);
}
.card-hover:hover {
  transform: translateY(-4px);
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.4), var(--accent-glow);
}

/* Glow pulse (data readouts, status) */
@keyframes glow-pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}
.glow-pulse { animation: glow-pulse 2s ease-in-out infinite; }

/* Reduced motion */
@media (prefers-reduced-motion: reduce) {
  .reveal { transition: opacity 300ms ease; transform: none; }
  .card-hover:hover { transform: none; }
  .glow-pulse { animation: none; }
}
```

---

## 6. Component Patterns

### 6.1 Landing Page — Scroll Sections

**GSAP ScrollTrigger is used only for marketing scroll sequences.** UI interactions use CSS transitions.

```
Section 1 — Hero (100vh, fullscreen)
  - Background: Higgsfield-generated dark environmental scene (video or image)
  - Grid overlay: 60px repeating grid, teal at 3% opacity
  - Radial glow: teal at 8% opacity, top-center
  - Headline: "EnviroBot" — letter-by-letter stagger reveal on load (not scroll)
  - Subline: fade in 400ms delay after headline
  - CTA buttons: fade + lift at 700ms delay
  - Scroll indicator: animated chevron, fade out on scroll

Section 2 — How It Works (scroll-pinned, 3 steps)
  - Pin container for 200vh of scroll
  - 3 steps cross-fade as user scrolls
  - Each step: illustrated diagram + text panel
  - Diagram animations: SVG paths drawing, arena quadrants filling
  - Text: clips in from left

Section 3 — Data Preview (normal scroll)
  - 4 sector cards slide in with stagger (60ms between each)
  - Each card: terrain icon, turbidity badge (NTU), moisture badges
  - Hover: card lifts 4px, accent glow on border

Section 4 — Links Hub (CTA grid)
  - 3 tool cards: Docs / Code / Visualiser
  - Icons scale from 0.95 → 1 on hover (100ms ease-out)
```

### 6.2 Buttons
```css
/* Primary CTA */
.btn-primary {
  background: var(--accent);
  color: #060912;
  font-weight: 600;
  padding: 12px 24px;
  border-radius: var(--radius-full);
  transition: transform 100ms var(--ease-out),
              opacity 100ms var(--ease-out),
              box-shadow 200ms var(--ease-out);
}
.btn-primary:hover {
  box-shadow: var(--accent-glow);
}
.btn-primary:active { transform: scale(0.97); }

/* Ghost / secondary */
.btn-ghost {
  border: 1px solid var(--bg-border);
  color: var(--text-primary);
  background: transparent;
  transition: transform 100ms var(--ease-out),
              border-color 200ms var(--ease-out),
              background 200ms var(--ease-out);
}
.btn-ghost:hover {
  border-color: var(--accent);
  background: var(--accent-muted);
}
.btn-ghost:active { transform: scale(0.97); }
```

### 6.3 Cards
```css
.card {
  background: var(--bg-surface);
  border: 1px solid var(--bg-border);
  border-radius: var(--radius-lg);
  padding: var(--space-6);
  transition: transform 200ms var(--ease-out),
              box-shadow 200ms var(--ease-out),
              border-color 200ms var(--ease-out);
}
@media (hover: hover) and (pointer: fine) {
  .card:hover {
    transform: translateY(-4px);
    border-color: rgba(0, 212, 170, 0.3);
    box-shadow: 0 20px 60px rgba(0,0,0,0.4), var(--accent-glow);
  }
}
```

### 6.4 Data Readout (Viz Page)
```css
/* NTU / moisture number display */
.data-readout {
  font-family: var(--font-mono);
  font-size: 2rem;
  font-weight: 500;
  color: var(--accent);
  text-shadow: var(--accent-glow);
}

/* Sector badge */
.sector-badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  border-radius: var(--radius-full);
  font-size: 0.75rem;
  font-weight: 500;
  font-family: var(--font-mono);
}
.sector-badge.water { background: rgba(2,119,189,0.15); color: #4FC3F7; }
.sector-badge.soil  { background: rgba(109,59,26,0.25); color: #BCAAA4; }
```

### 6.5 Navigation
```css
/* Top nav — glassmorphism */
.nav {
  position: fixed;
  top: 0; left: 0; right: 0;
  height: 60px;
  background: rgba(10, 14, 26, 0.8);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  border-bottom: 1px solid var(--bg-border);
  z-index: 100;
  transition: background 200ms var(--ease-out);
}
```

### 6.6 Arena Canvas (Viz Page)
```
Arena render spec:
- Octagon: clip-path polygon(8 sides) over square canvas
- Background: terrain quadrant fills using CSS terrain colours
- Grid: subtle 20px grid, 5% opacity
- Water zones: teal diamond shapes (#00D4AA at 60% opacity)
- Soil patches: reddish-brown circles
- Robot: white triangle indicator (heading-aware)
- Measurement dots: WATER=blue, SOIL=brown, animated pop in (scale 0.5→1, 200ms ease-out)
- Path line: teal 2px stroke, drawn progressively during replay
- Replay scrubber: progress bar at bottom, seek by click
```

---

## 7. Page-Specific Layout

### 7.1 Landing (`/`)
```
Nav (60px, fixed)
Hero section (100vh)
How It Works (scroll-pinned, ~200vh scroll)
Data Preview (auto height)
Links Hub (auto height)
Footer (minimal, 80px)
```

### 7.2 Docs (`/docs`)
```
Nav (60px, fixed)
┌─────────────────────────────────────────┐
│ Sidebar (260px, sticky)│ Content (flex-1)│
│ - Logo + search        │                │
│ - Nav tree (MDX pages) │  MDX rendered  │
│ - Collapsible sections │  prose content │
│                        │                │
└────────────────────────┴────────────────┘
```

### 7.3 Code Browser (`/code`)
```
Nav (60px, fixed)
┌────────────────────────────────────────┐
│ File Tree (220px) │ Code Viewer (flex-1)│
│ - .ino            │ - Shiki highlight  │
│ - .cpp            │ - Copy button      │
│ - .h              │ - Line numbers     │
│ - rc_page/        │ - File path header │
└───────────────────┴────────────────────┘
```

### 7.4 Visualiser (`/viz`)
```
Nav (60px, fixed)
Upload zone (if no data loaded)
┌───────────────────────────────────────────────┐
│ Arena Canvas (square, max 600px)│ Charts Panel │
│ - Terrain sectors              │ - NTU bar     │
│ - Path animation               │ - Moisture bar│
│ - Measurement dots             │ - Path XY     │
│ - Replay scrubber              │ - Heatmap     │
├────────────────────────────────┴──────────────┤
│ Sector Breakdown (4 sector rows + sample list) │
└───────────────────────────────────────────────┘
```

---

## 8. Higgsfield Asset Direction

### Hero Image/Video
**Prompt direction**: Dark environmental monitoring scene. Octagonal arena glowing teal at edges, robotic arm deploying sensor into water sample on alien terrain. Four distinct terrain sectors visible (sand, grass, soil, sandpaper texture). Atmospheric depth, environmental science aesthetic. Deep space colour palette — #0A0E1A background with teal (#00D4AA) bioluminescent highlights. Ultra-detailed, photorealistic, cinematic.

**Format**: Static image for initial load (WebP), video loop for desktop (MP4, 10s, 3MB max).

**Placement**: Full hero background, darkened with gradient overlay (`linear-gradient(to bottom, rgba(6,9,18,0.3), rgba(6,9,18,0.9))`). Text renders above.

---

## 9. Mobile Responsiveness

All layouts stack to single column at ≤768px.

| Breakpoint | Behaviour |
|---|---|
| <640px | Single column, sidebar collapses to off-canvas drawer |
| 640–1024px | Two column where applicable |
| >1024px | Full layout |

RC joystick page: always full-screen, touch-first, no hover states.

---

## 10. Review Checklist (Emil Principles)
Before marking design done, verify:
- [ ] No `transition: all` anywhere
- [ ] No `scale(0)` entry animations (minimum `scale(0.95)`)  
- [ ] No `ease-in` on UI elements
- [ ] All hover animations gated with `@media (hover: hover) and (pointer: fine)`
- [ ] `prefers-reduced-motion` handled (remove transforms, keep opacity)
- [ ] All UI animations ≤300ms
- [ ] Stagger delays 30–80ms (not longer)
- [ ] Buttons have `:active { transform: scale(0.97) }`
- [ ] Popovers/tooltips scale from trigger point (not center)
- [ ] Landing scroll animations use GSAP, not CSS keyframes (interruptible)
- [ ] Canvas animations use `requestAnimationFrame`, not `setInterval`
