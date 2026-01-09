# Ambio

> _Something is listening._

A teaser landing page featuring an animated WebGL particle background, dark/light theme support with [Catppuccin](https://catppuccin.com/) colors, and email subscription capture.

## Tech Stack

- **[Nuxt 4](https://nuxt.com/)** – Vue 3 meta-framework with SSR/SSG
- **[Tailwind CSS v4](https://tailwindcss.com/)** – Utility-first CSS
- **[DaisyUI v5](https://daisyui.com/)** – Component library for Tailwind
- **[Three.js](https://threejs.org/)** – WebGL particle effects
- **[Bun](https://bun.sh/)** – JavaScript runtime & package manager

## Quick Start

```bash
# Install dependencies
bun install

# Start development server
bun run dev
```

Open [http://localhost:3000](http://localhost:3000) to view the site.

## Scripts

| Command            | Description               |
| ------------------ | ------------------------- |
| `bun run dev`      | Start dev server with HMR |
| `bun run build`    | Build for production      |
| `bun run preview`  | Preview production build  |
| `bun run generate` | Generate static site      |

## Theming

The site supports dark and light themes using the Catppuccin color palette:

- **Dark** – Catppuccin Macchiato
- **Light** – Catppuccin Latte

Theme preference is persisted to localStorage and respects system preferences on first visit. The WebGL background particles also update their colors when the theme changes.

## Linting

This project uses [Trunk](https://trunk.io/) for linting and formatting:

```bash
trunk check      # Run all linters
trunk fmt        # Auto-format code
```

## Project Structure

```
app/
├── app.vue           # Root layout with WebGL background
├── components/       # Vue components (EmailForm, ThemeToggle, etc.)
├── composables/      # Shared state (useTheme)
├── plugins/          # Client-side plugins
└── assets/css/       # Tailwind + DaisyUI theme configuration
server/
└── api/              # Nitro server routes
public/
└── images/           # Static assets
```

## License

[MIT](LICENSE) © 2025 Dave Williams
