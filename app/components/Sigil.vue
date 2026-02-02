<template>
  <div
    class="relative grid place-items-center"
    aria-hidden="true"
    data-testid="img-sigil"
  >
    <div class="absolute -inset-10 glow blur-2xl opacity-80" />

    <div class="relative rounded-[28px] border border-white/10 bg-white/2 shadow-[0_0_0_1px_rgba(255,255,255,0.03)_inset]">
      <div class="absolute inset-0 rounded-[28px] bg-linear-to-b from-white/6 to-transparent" />
      <div class="absolute -inset-px rounded-[28px] bg-linear-to-b from-white/8 via-transparent to-white/5 opacity-60" />

      <div class="relative h-20 w-20 sm:h-24 sm:w-24">
        <div class="absolute inset-0 grid place-items-center">
          <div class="relative h-10 w-10 sm:h-12 sm:w-12 rounded-full border border-white/15 bg-white/3 shadow-sm">
            <div class="absolute inset-0 rounded-full bg-linear-to-b from-white/10 to-transparent" />
            <div class="absolute left-1/2 top-1/2 h-2.5 w-2.5 -translate-x-1/2 -translate-y-1/2 rounded-full bg-white/80" />
            <div class="absolute left-1/2 top-1/2 h-5.5 w-5.5 -translate-x-1/2 -translate-y-1/2 rounded-full border border-white/10" />
          </div>
        </div>

        <div class="absolute inset-0">
          <div class="absolute left-1/2 top-1/2 h-[138%] w-[138%] -translate-x-1/2 -translate-y-1/2 rounded-full border border-white/10" />
          <div class="absolute left-1/2 top-1/2 h-[168%] w-[168%] -translate-x-1/2 -translate-y-1/2 rounded-full border border-white/7" />
          <div class="absolute left-1/2 top-1/2 h-[210%] w-[210%] -translate-x-1/2 -translate-y-1/2 rounded-full border border-white/5" />
        </div>

        <div class="absolute inset-0 sweep">
          <div class="absolute left-1/2 top-1/2 h-[210%] w-[210%] -translate-x-1/2 -translate-y-1/2 rounded-full bg-linear-to-r from-transparent via-white/12 to-transparent blur-[2px]" />
        </div>
      </div>
    </div>
  </div>
</template>
