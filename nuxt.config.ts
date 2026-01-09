// https://nuxt.com/docs/api/configuration/nuxt-config
import tailwindcss from "@tailwindcss/vite";

export default defineNuxtConfig({
  compatibilityDate: "2025-07-15",
  devtools: { enabled: true },

  modules: [
    "@nuxt/fonts",
    "@nuxt/hints",
    "@nuxt/icon",
    "@nuxt/image",
    "@nuxt/scripts",
    "@nuxt/a11y",
    "@nuxt/eslint",
  ],

  // Global CSS - Tailwind v4 is imported here
  css: ["./app/assets/css/tailwind.css"],

  fonts: {
    families: [
      {
        name: "Inter",
        provider: "google",
        weights: [300, 400, 500, 600, 700],
      },
    ],
    defaults: {
      weights: [400],
      styles: ["normal"],
    },
  },

  app: {
    head: {
      title: "Ambio",
      htmlAttrs: {
        lang: "en",
      },
      meta: [
        { charset: "utf-8" },
        { name: "viewport", content: "width=device-width, initial-scale=1" },
        { name: "description", content: "Something is listening." },
        { name: "theme-color", content: "#24273a" },
      ],
      link: [
        { rel: "icon", type: "image/x-icon", href: "/favicon.ico" },
        // Fallback font loading via link tag
        {
          rel: "preconnect",
          href: "https://fonts.googleapis.com",
        },
        {
          rel: "preconnect",
          href: "https://fonts.gstatic.com",
          crossorigin: "",
        },
        {
          rel: "stylesheet",
          href: "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap",
        },
      ],
    },
  },

  nitro: {
    prerender: {
      failOnError: false,
      crawlLinks: true,
    },
  },

  // Vite configuration with Tailwind CSS v4
  vite: {
    plugins: [tailwindcss()],
    optimizeDeps: {
      include: ["three"],
    },
    ssr: {
      noExternal: ["three"],
    },
  },
});
