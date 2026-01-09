export type Theme = "macchiato" | "latte";

export function useTheme() {
  // Initialize from document element on client, default to macchiato on server
  const theme = useState<Theme>("theme", () => {
    if (import.meta.client) {
      const attr = document.documentElement.getAttribute("data-theme");
      return (attr as Theme) || "macchiato";
    }
    return "macchiato";
  });

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

  return {
    theme,
    isDark,
    setTheme,
    toggleTheme,
  };
}
