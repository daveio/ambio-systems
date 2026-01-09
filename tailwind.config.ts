import type { Config } from "tailwindcss";
import daisyui from "daisyui";

// Catppuccin Macchiato palette (dark)
const macchiato = {
  base: "#24273a",
  mantle: "#1e2030",
  crust: "#181926",
  text: "#cad3f5",
  overlay1: "#8087a2",
  surface0: "#363a4f",
  rosewater: "#f4dbd6",
  mauve: "#c6a0f6",
  red: "#ed8796",
  yellow: "#eed49f",
  green: "#a6da95",
  sapphire: "#7dc4e4",
};

// Catppuccin Latte palette (light)
const latte = {
  base: "#eff1f5",
  mantle: "#e6e9ef",
  crust: "#dce0e8",
  text: "#4c4f69",
  overlay1: "#8c8fa1",
  surface0: "#ccd0da",
  rosewater: "#dc8a78",
  mauve: "#8839ef",
  red: "#d20f39",
  yellow: "#df8e1d",
  green: "#40a02b",
  sapphire: "#209fb5",
};

export default {
  content: [
    "./components/**/*.{js,vue,ts}",
    "./layouts/**/*.vue",
    "./pages/**/*.vue",
    "./plugins/**/*.{js,ts}",
    "./app/**/*.vue",
    "./app.vue",
    "./error.vue",
  ],
  theme: {
    extend: {
      fontFamily: {
        sans: ["Inter", "ui-sans-serif", "system-ui", "sans-serif"],
      },
    },
  },
  plugins: [daisyui],
  daisyui: {
    themes: [
      {
        // Extend built-in dark theme with Catppuccin Macchiato
        dark: {
          "color-scheme": "dark",
          primary: macchiato.mauve,
          "primary-content": macchiato.crust,
          secondary: macchiato.surface0,
          "secondary-content": macchiato.text,
          accent: macchiato.rosewater,
          "accent-content": macchiato.crust,
          neutral: macchiato.overlay1,
          "neutral-content": macchiato.crust,
          "base-100": macchiato.base,
          "base-200": macchiato.mantle,
          "base-300": macchiato.crust,
          "base-content": macchiato.text,
          info: macchiato.sapphire,
          "info-content": macchiato.crust,
          success: macchiato.green,
          "success-content": macchiato.crust,
          warning: macchiato.yellow,
          "warning-content": macchiato.crust,
          error: macchiato.red,
          "error-content": macchiato.crust,
        },
      },
      {
        // Extend built-in light theme with Catppuccin Latte
        light: {
          "color-scheme": "light",
          primary: latte.mauve,
          "primary-content": latte.base,
          secondary: latte.surface0,
          "secondary-content": latte.text,
          accent: latte.rosewater,
          "accent-content": latte.base,
          neutral: latte.overlay1,
          "neutral-content": latte.base,
          "base-100": latte.base,
          "base-200": latte.mantle,
          "base-300": latte.crust,
          "base-content": latte.text,
          info: latte.sapphire,
          "info-content": latte.base,
          success: latte.green,
          "success-content": latte.base,
          warning: latte.yellow,
          "warning-content": latte.base,
          error: latte.red,
          "error-content": latte.base,
        },
      },
    ],
  },
} satisfies Config;
