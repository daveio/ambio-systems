import tailwindcss from "@tailwindcss/vite"

export default defineNuxtConfig({
  compatibilityDate: "2025-07-15",
  devtools: { enabled: true },
  css: ["~/assets/css/tailwind.css"],
  vite: {
    plugins: [tailwindcss()],
  },
  app: {
    head: {
      htmlAttrs: { lang: "en" },
      title: "Ambio",
      meta: [
        { name: "description", content: "An always-on conversation recorder — a quiet companion that turns real moments into clarity." },
        { property: "og:title", content: "Ambio" },
        { property: "og:description", content: "An always-on conversation recorder — a quiet companion that turns real moments into clarity." },
        { property: "og:type", content: "website" },
        { name: "twitter:card", content: "summary_large_image" },
        { name: "twitter:title", content: "Ambio" },
        { name: "twitter:description", content: "An always-on conversation recorder — a quiet companion that turns real moments into clarity." },
      ],
      link: [
        { rel: "icon", type: "image/png", href: "/favicon.png" },
        { rel: "preconnect", href: "https://fonts.googleapis.com" },
        { rel: "preconnect", href: "https://fonts.gstatic.com", crossorigin: "" },
        { rel: "stylesheet", href: "https://fonts.googleapis.com/css2?family=IBM+Plex+Sans:ital,wght@0,100..700;1,100..700&family=Newsreader:ital,opsz,wght@0,6..72,300..800;1,6..72,300..800&display=swap" },
      ],
    },
  },
})
