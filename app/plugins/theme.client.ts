export default defineNuxtPlugin({
  name: "theme-init",
  enforce: "pre",
  setup() {
    // This runs before the app hydrates
    if (import.meta.client) {
      const savedTheme = localStorage.getItem("ambio-theme") || null;
      const prefersDark = window.matchMedia(
        "(prefers-color-scheme: dark)",
      ).matches;
      const theme = savedTheme || (prefersDark ? "macchiato" : "latte");

      // Set theme on document element immediately
      document.documentElement.setAttribute("data-theme", theme);
    }
  },
});
