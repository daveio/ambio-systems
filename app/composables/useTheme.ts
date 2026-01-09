export type Theme = "dark" | "light";

export function useTheme() {
  // Initialize from document element on client, default to dark on server
  const theme = useState<Theme>("theme", () => {
    if (import.meta.client) {
      const attr = document.documentElement.getAttribute("data-theme");
      return (attr as Theme) || "dark";
    }
    return "dark";
  });

  const isDark = computed(() => theme.value === "dark");

  function setTheme(newTheme: Theme) {
    theme.value = newTheme;
    if (import.meta.client) {
      document.documentElement.setAttribute("data-theme", newTheme);
      localStorage.setItem("ambio-theme", newTheme);
    }
  }

  function toggleTheme() {
    setTheme(theme.value === "dark" ? "light" : "dark");
  }

  return {
    theme,
    isDark,
    setTheme,
    toggleTheme,
  };
}
