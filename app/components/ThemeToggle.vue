<script setup lang="ts">
const { isDark, toggleTheme } = useTheme();
</script>

<template>
  <button
    class="btn btn-circle btn-ghost btn-sm"
    :aria-label="isDark ? 'Switch to light theme' : 'Switch to dark theme'"
    @click="toggleTheme"
  >
    <Transition name="theme-icon" mode="out-in">
      <Icon v-if="isDark" name="ph:sun-bold" class="h-5 w-5 text-mauve" />
      <Icon v-else name="ph:moon-bold" class="h-5 w-5 text-mauve" />
    </Transition>
  </button>
</template>

<style scoped>
.theme-icon-enter-active,
.theme-icon-leave-active {
  transition: all 0.2s ease;
}

.theme-icon-enter-from {
  opacity: 0;
  transform: rotate(-90deg) scale(0.8);
}

.theme-icon-leave-to {
  opacity: 0;
  transform: rotate(90deg) scale(0.8);
}
</style>
