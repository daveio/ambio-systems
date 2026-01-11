<script setup lang="ts">
const { theme, isDark } = useTheme();
useSeoMeta({
  description: "Something is listening.",
  ogTitle: "Ambio",
  ogDescription: "Something is listening.",
  ogImage: "https://ambio.systems/images/pendant-mockup.avif",
  ogImageWidth: 1024,
  ogImageHeight: 1024,
  ogImageAlt: "An early mockup for an Ambio pendant",
  ogUrl: "https://ambio.systems",
  twitterTitle: "Ambio",
  twitterDescription: "Something is listening.",
  twitterImage: "https://ambio.systems/images/pendant-mockup.avif",
  twitterImageAlt: "An early mockup for an Ambio pendant",
  applicationName: "Ambio",
  twitterCard: "summary",
});

useHead({
  htmlAttrs: {
    lang: "en",
  },
  link: [
    {
      rel: "apple-touch-icon",
      sizes: "180x180",
      href: "/apple-touch-icon.png",
    },
    {
      rel: "icon",
      type: "image/png",
      sizes: "32x32",
      href: "/favicon-32x32.png",
    },
    {
      rel: "icon",
      type: "image/png",
      sizes: "16x16",
      href: "/favicon-16x16.png",
    },
    {
      rel: "manifest",
      href: "/site.webmanifest",
    },
  ],
});
</script>

<template>
  <div :data-theme="theme" class="min-h-screen bg-base-100">
    <NuxtRouteAnnouncer />

    <!-- WebGL Background -->
    <ClientOnly>
      <WebGLBackground />
    </ClientOnly>

    <!-- Theme Toggle - Fixed Position -->
    <div class="fixed top-4 right-4 z-50">
      <ThemeToggle />
    </div>

    <!-- Main Content -->
    <main
      class="relative z-10 min-h-screen flex flex-col items-center justify-center px-4 py-16"
    >
      <div
        class="flex flex-col items-center gap-12 text-center max-w-2xl mx-auto"
      >
        <!-- Logo -->
        <div class="animate-fade-in">
          <AmbioLogo />
        </div>

        <!-- Tagline -->
        <div class="space-y-4 animate-fade-in-delay-1">
          <p
            class="text-xl md:text-2xl tracking-wide"
            :class="
              isDark
                ? 'text-base-content/60 font-light'
                : 'text-base-content/80 font-normal'
            "
          >
            Something is listening.
          </p>
        </div>

        <!-- Email Form -->
        <div class="w-full animate-fade-in-delay-2">
          <div class="space-y-6">
            <h2 class="text-base-content/80 text-lg font-medium">
              Want to get updates?
            </h2>
            <EmailForm />
          </div>
        </div>

        <!-- Subtle hint -->
        <p
          class="text-sm animate-fade-in-delay-3 max-w-md"
          :class="isDark ? 'text-base-content/20' : 'text-base-content/60'"
        >
          Every conversation leaves an echo.
        </p>
      </div>
    </main>

    <!-- Ambient glow effect -->
    <div class="fixed inset-0 pointer-events-none overflow-hidden -z-5">
      <div
        class="absolute top-1/4 left-1/4 w-96 h-96 bg-mauve/5 rounded-full blur-3xl animate-pulse-slow"
      />
      <div
        class="absolute bottom-1/4 right-1/4 w-80 h-80 bg-sapphire/5 rounded-full blur-3xl animate-pulse-slow-delay"
      />
    </div>
  </div>
</template>

<style>
/* Animation keyframes */
@keyframes fade-in {
  from {
    opacity: 0;
    transform: translateY(20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes pulse-slow {
  0%,
  100% {
    opacity: 0.3;
    transform: scale(1);
  }
  50% {
    opacity: 0.5;
    transform: scale(1.1);
  }
}

.animate-fade-in {
  animation: fade-in 0.8s ease-out forwards;
}

.animate-fade-in-delay-1 {
  opacity: 0;
  animation: fade-in 0.8s ease-out 0.2s forwards;
}

.animate-fade-in-delay-2 {
  opacity: 0;
  animation: fade-in 0.8s ease-out 0.4s forwards;
}

.animate-fade-in-delay-3 {
  opacity: 0;
  animation: fade-in 0.8s ease-out 0.6s forwards;
}

.animate-pulse-slow {
  animation: pulse-slow 8s ease-in-out infinite;
}

.animate-pulse-slow-delay {
  animation: pulse-slow 8s ease-in-out 4s infinite;
}

/* Catppuccin color utilities */
.text-mauve {
  color: oklch(var(--p));
}

.text-sapphire {
  color: oklch(var(--in));
}

.text-teal {
  color: oklch(var(--su));
}

.bg-mauve {
  background-color: oklch(var(--p));
}

.bg-mauve\/5 {
  background-color: oklch(var(--p) / 0.05);
}

.bg-sapphire\/5 {
  background-color: oklch(var(--in) / 0.05);
}

.bg-teal\/20 {
  background-color: oklch(var(--su) / 0.2);
}

/* Layer for glow effects */
.-z-5 {
  z-index: -5;
}
</style>
