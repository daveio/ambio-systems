<script setup lang="ts">
defineProps<{
  title: string
  body: string
  testId: string
}>()
</script>

<template>
  <div
    class="rounded-2xl border border-white/10 bg-white/2 px-4 py-4 shadow-sm backdrop-blur"
    :data-testid="testId"
  >
    <div class="flex items-start gap-3">
      <div class="mt-0.5 rounded-xl border border-white/10 bg-white/3 p-2 text-white/90">
        <slot />
      </div>
      <div class="min-w-0">
        <div class="text-sm font-semibold tracking-tight text-white/95">
          {{ title }}
        </div>
        <div class="mt-1 text-sm leading-relaxed text-white/70">
          {{ body }}
        </div>
      </div>
    </div>
  </div>
</template>
