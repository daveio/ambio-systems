export type Theme = "macchiato" | "latte";

export function useTheme() {
  const theme = useState<Theme>("theme", () => "macchiato");
  const isDark = computed(() => theme.value === "macchiato");

  function setTheme(newTheme: Theme) {
    theme.value = newTheme;
    if (import.meta.client) {
      document.documentElement.setAttribute("data-theme", newTheme);
      localStorage.setItem("ambio-theme", newTheme);
    }
  }

  function toggleTheme() {
    setTheme(theme.value === "macchiato" ? "latte" : "macchiato");
  }

  function initTheme() {
    if (import.meta.client) {
      // Use requestAnimationFrame to ensure this runs after hydration
      requestAnimationFrame(() => {
        const savedTheme = localStorage.getItem("ambio-theme") as Theme | null;
        const prefersDark = window.matchMedia(
          "(prefers-color-scheme: dark)",
        ).matches;
        const initialTheme =
          savedTheme || (prefersDark ? "macchiato" : "latte");

        // Only update if different from current theme
        if (initialTheme !== theme.value) {
          setTheme(initialTheme);
        }
      });
    }
  }

  return {
    theme,
    isDark,
    setTheme,
    toggleTheme,
    initTheme,
  };
}
