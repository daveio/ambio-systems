<script setup lang="ts">
const { isDark } = useTheme();

const logoSrc = computed(() =>
  isDark.value ? "/images/logotype-dark.avif" : "/images/logotype-light.avif",
);
</script>

<template>
  <div class="relative">
    <Transition name="logo-fade" mode="out-in">
      <NuxtImg
        :key="logoSrc"
        :src="logoSrc"
        alt="Ambio"
        class="h-16 w-auto md:h-24 select-none"
        loading="eager"
      />
    </Transition>
  </div>
</template>

<style scoped>
.logo-fade-enter-active,
.logo-fade-leave-active {
  transition: opacity 0.3s ease;
}

.logo-fade-enter-from,
.logo-fade-leave-to {
  opacity: 0;
}
</style>
