export type Theme = "dark" | "light";

export function useTheme() {
  // Initialize with server default - will sync on client after hydration
  const theme = useState<Theme>("theme", () => "dark");

  const isDark = computed(() => theme.value === "dark");

  // Sync theme state from DOM after hydration (plugin sets data-theme before hydration)
  // This ensures reactive state matches the already-applied DOM theme
  if (import.meta.client) {
    onNuxtReady(() => {
      const attr = document.documentElement.getAttribute("data-theme");
      if (attr && (attr === "dark" || attr === "light")) {
        theme.value = attr;
      }
    });
  }

  const setTheme = (newTheme: Theme) => {
    theme.value = newTheme;
    if (import.meta.client) {
      document.documentElement.setAttribute("data-theme", newTheme);
      // localStorage can throw in private browsing, when quota exceeded, or disabled
      try {
        localStorage.setItem("ambio-theme", newTheme);
      } catch {
        // Theme still works via DOM attribute, just won't persist
      }
    }
  };

  const toggleTheme = () => {
    setTheme(theme.value === "dark" ? "light" : "dark");
  };

  return {
    theme,
    isDark,
    setTheme,
    toggleTheme,
  };
}
