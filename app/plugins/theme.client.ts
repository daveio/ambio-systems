export default defineNuxtPlugin({
  name: "theme-init",
  enforce: "pre",
  setup() {
    // This runs before the app hydrates
    if (import.meta.client) {
      let savedTheme: string | null = null;

      // localStorage can throw in private browsing, when quota exceeded, or disabled
      try {
        savedTheme = localStorage.getItem("ambio-theme");
      } catch {
        // Silently fall back to system preference
      }

      const prefersDark = window.matchMedia(
        "(prefers-color-scheme: dark)",
      ).matches;
      const theme = savedTheme || (prefersDark ? "dark" : "light");

      // Set theme on document element immediately
      document.documentElement.setAttribute("data-theme", theme);
    }
  },
});
