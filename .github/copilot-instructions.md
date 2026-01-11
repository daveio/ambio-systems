# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Ambio is a landing page / teaser site built with Nuxt 4, featuring an animated Three.js particle background, dark/light theme support with Catppuccin colors, and an email subscription form.

## Commands

```bash
# Development
bun run dev          # Start dev server with HMR

# Build & Preview
bun run build        # Build for production
bun run preview      # Preview production build
bun run generate     # Generate static site

# Linting (uses Trunk)
trunk check          # Run all linters
trunk fmt            # Auto-format code
trunk check --fix    # Fix auto-fixable issues

# Database (Drizzle + D1)
bun run db:generate       # Generate migrations from schema changes
bun run db:migrate:local  # Apply migrations to local D1
bun run db:migrate:remote # Apply migrations to remote D1
bun run db:studio         # Open Drizzle Studio

# Install dependencies
bun install          # Uses bun@1.3.5 (enforced via packageManager field)
```

## Architecture

### Tech Stack

- **Nuxt 4** with Vue 3 Composition API
- **Tailwind CSS v4** with DaisyUI v5 (uses new `@plugin` syntax in CSS, not JS config)
- **Three.js** for WebGL particle background
- **Cloudflare D1** with Drizzle ORM for database
- **Cloudflare Secrets Store** for secure API key storage
- **Bun** as package manager (v1.3.5)

### Directory Structure

```plaintext
app/
├── app.vue              # Root component with layout
├── assets/css/          # Tailwind entry + DaisyUI theme definitions
├── components/          # Vue components
├── composables/         # Shared state (useTheme)
└── plugins/             # Client plugins (theme initialization)
server/
├── api/                 # Nitro server routes
│   ├── subscribe.post.ts    # Email subscription endpoint
│   ├── unsubscribe.post.ts  # Email unsubscribe endpoint
│   └── admin/
│       └── subscriptions.get.ts  # Admin API (list/export subscriptions)
├── database/
│   └── schema.ts        # Drizzle schema definition
└── utils/
    ├── db.ts            # Database utility (useDB)
    └── auth.ts          # Admin auth utility (requireAdminAuth)
drizzle/                 # Generated SQL migrations
public/
└── images/              # Static assets (logo variants)
```

### Theme System

The theme system prevents flash-of-wrong-theme using a two-phase approach:

1. **`plugins/theme.client.ts`** - Runs _before_ hydration with `enforce: "pre"`. Reads localStorage/system preference and sets `data-theme` on `<html>` immediately.

2. **`composables/useTheme.ts`** - Syncs reactive `theme` state from DOM _after_ hydration via `onNuxtReady()`. Provides `setTheme()`, `toggleTheme()`, and reactive `isDark` computed.

Components use DaisyUI semantic classes (`bg-base-100`, `text-primary`, etc.) which automatically respond to the `data-theme` attribute.

### Theming (Catppuccin Colors)

Themes are defined in `app/assets/css/tailwind.css` using DaisyUI's `@plugin "daisyui/theme"` blocks:

- `dark` theme = Catppuccin Macchiato
- `light` theme = Catppuccin Latte

The WebGL background (`WebGLBackground.vue`) also uses Catppuccin colors and watches `isDark` to update particle colors reactively.

### Server Routes

Nitro API routes follow the convention `server/api/[name].[method].ts`:

- `subscribe.post.ts` - Email subscription endpoint with rate limiting and validation
- `unsubscribe.post.ts` - Email unsubscribe endpoint with privacy protections
- `admin/subscriptions.get.ts` - Admin API (list/export subscriptions, requires API key)

#### Security Features

The subscription system includes several security and privacy measures:

- **Rate Limiting**: Uses Cloudflare KV for distributed rate limiting (5 requests per 60 seconds per IP). Configure `RATE_LIMIT_KV` binding in production.
- **Email Validation**: RFC-compliant regex pattern validation with length checks (max 254 characters).
- **Geolocation Data Validation**: Type guards ensure proper data types before database insertion.
- **Timing Attack Prevention**: Admin API key validation uses constant-time comparison (`timingSafeEqual`).
- **Privacy Protection**: Unsubscribe endpoint returns generic success messages to prevent email enumeration.
- **Opt-in Only**: Previously unsubscribed users cannot be auto-reactivated; they must contact support.

### Client-Only Rendering

Three.js is wrapped in `<ClientOnly>` and dynamically imported to avoid SSR issues. The component stores the `THREE` namespace in a module-scoped variable after import.

## Nuxt Modules

Active modules (configured in `nuxt.config.ts`):

- `@nuxt/fonts` - Google Fonts (Inter)
- `@nuxt/hints` - Performance hints
- `@nuxt/icon` - Icon component (uses Phosphor icons: `ph:*`)
- `@nuxt/image` - Image optimization
- `@nuxt/scripts` - Script loading
- `@nuxt/a11y` - Accessibility checking
- `@nuxt/eslint` - ESLint integration

## Important Patterns

### Vue Component Conventions

- Use `<script setup lang="ts">` exclusively
- Prefer composables for shared state
- Use Nuxt auto-imports (no manual imports for Vue APIs, components, or composables)

### Styling

- Use DaisyUI component classes (`btn`, `input`, `join`, etc.)
- Use semantic color classes (`text-base-content`, `bg-primary`, etc.)
- Custom Catppuccin utilities defined in `app.vue` styles (`text-mauve`, `bg-sapphire/5`, etc.)

### Animations

- CSS animations defined in component `<style>` blocks
- Vue `<Transition>` for component enter/leave animations
