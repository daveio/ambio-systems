<script setup lang="ts">
import { ArrowRight, Check, Lock, Mic, Sparkles } from "lucide-vue-next"
import { cn } from "~/lib/utils"

function isValidEmail(value: string) {
  const v = value.trim()
  if (!v) return false
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(v)
}

const email = ref("")
const submitted = ref(false)

const valid = computed(() => isValidEmail(email.value))
const helper = computed(() => {
  if (submitted.value) return "You're on the list. We'll reach out soon."
  if (!email.value.trim()) return "No spam. One email when it's ready."
  if (!valid.value) return "That doesn't look like an email address."
  return "You'll get early access + build updates."
})
</script>

<template>
  <div class="min-h-[100svh] bg-background text-foreground">
    <div class="pointer-events-none fixed inset-0 glow" />
    <div class="pointer-events-none fixed inset-0 grid-fade opacity-70" />
    <div class="pointer-events-none fixed inset-0 noise" />

    <div class="relative mx-auto flex w-full max-w-6xl flex-col px-5 pb-14 pt-10 sm:px-8 sm:pb-20 sm:pt-14">
      <header class="flex items-center justify-between">
        <div class="flex items-center gap-3">
          <div
            class="grid h-9 w-9 place-items-center rounded-xl border border-white/10 bg-white/[0.02]"
            data-testid="img-logo"
            aria-hidden="true"
          >
            <div class="h-2.5 w-2.5 rounded-full bg-primary shadow-[0_0_0_6px_hsl(var(--primary)/0.16),0_0_40px_hsl(var(--primary)/0.12)]" />
          </div>
          <div class="flex flex-col leading-none">
            <div
              class="text-sm font-semibold tracking-tight"
              data-testid="text-brand"
            >
              Ambio
            </div>
            <div
              class="mt-1 text-xs text-white/55"
              data-testid="text-tagline"
            >
              Always-on conversation, when you want it.
            </div>
          </div>
        </div>

        <div class="hidden items-center gap-2 sm:flex">
          <div
            class="rounded-full border border-white/10 bg-white/[0.02] px-3 py-1 text-xs text-white/70"
            data-testid="status-build"
          >
            Private build · 2026
          </div>
        </div>
      </header>

      <main class="mt-14 grid gap-10 lg:mt-18 lg:grid-cols-[1.1fr_0.9fr] lg:items-start">
        <section class="min-w-0 animate-enter">
          <div class="inline-flex items-center gap-2 rounded-full border border-white/10 bg-white/[0.02] px-3 py-1.5 text-xs text-white/70 shadow-sm backdrop-blur">
            <span
              class="inline-flex h-1.5 w-1.5 rounded-full bg-primary"
              data-testid="status-dot"
              aria-hidden="true"
            />
            <span data-testid="text-pill">A pendant that remembers so you can be present.</span>
          </div>

          <h1
            class="mt-5 font-serif text-[2.4rem] leading-[1.03] tracking-[-0.02em] text-white sm:text-[3.2rem]"
            data-testid="text-hero-title"
          >
            Your day,
            <span class="text-grad"> distilled</span>.
          </h1>

          <p
            class="mt-5 max-w-xl text-base leading-relaxed text-white/72 sm:text-lg"
            data-testid="text-hero-subtitle"
          >
            Ambio is an always-on conversation recorder pendant — built to capture
            what matters, summarize it quietly, and surface action when you need it.
          </p>

          <div class="mt-8 rounded-3xl border border-white/10 bg-white/[0.02] p-4 shadow-md backdrop-blur sm:p-5">
            <div class="flex flex-col gap-3 sm:flex-row sm:items-end">
              <div class="flex-1">
                <label
                  class="text-xs font-medium text-white/70"
                  data-testid="label-email"
                  for="email"
                >
                  Early access
                </label>
                <div class="mt-2">
                  <input
                    id="email"
                    v-model="email"
                    type="email"
                    placeholder="you@domain.com"
                    :disabled="submitted"
                    :class="cn(
                      'input w-full h-11 rounded-2xl border-white/12 bg-white/[0.03] text-white placeholder:text-white/35',
                      'focus:ring-2 focus:ring-primary/30 focus:border-white/18 focus:outline-none',
                      submitted && 'opacity-80'
                    )"
                    data-testid="input-email"
                  />
                </div>

                <div
                  :class="cn(
                    'mt-2 text-xs',
                    submitted ? 'text-primary/90' : valid ? 'text-white/60' : 'text-white/55'
                  )"
                  data-testid="text-email-helper"
                  role="status"
                  aria-live="polite"
                >
                  {{ helper }}
                </div>
              </div>

              <button
                type="button"
                :disabled="submitted || !valid"
                :class="cn(
                  'btn h-11 rounded-2xl px-4',
                  'bg-primary text-primary-content hover:bg-primary/90',
                  'shadow-[0_12px_40px_-22px_hsl(var(--primary)/0.6)]',
                  'disabled:opacity-40 disabled:hover:bg-primary'
                )"
                data-testid="button-join-waitlist"
                @click="submitted = true"
              >
                <span class="inline-flex items-center gap-2">
                  <template v-if="submitted">
                    <Check class="h-4 w-4" aria-hidden="true" />
                    You're in
                  </template>
                  <template v-else>
                    Join waitlist
                    <ArrowRight class="h-4 w-4" aria-hidden="true" />
                  </template>
                </span>
              </button>
            </div>

            <div class="mt-4 flex flex-wrap items-center gap-x-4 gap-y-2 text-xs text-white/55">
              <div class="inline-flex items-center gap-2" data-testid="text-privacy">
                <Lock class="h-3.5 w-3.5 text-white/65" aria-hidden="true" />
                Privacy-first by design.
              </div>
              <div class="inline-flex items-center gap-2" data-testid="text-updates">
                <Sparkles class="h-3.5 w-3.5 text-white/65" aria-hidden="true" />
                Build notes, not marketing.
              </div>
            </div>
          </div>

          <div class="mt-7 grid gap-3 sm:grid-cols-2">
            <FeatureCard
              title="Captures the moment"
              body="Record in the background. Transfer when you're nearby."
              test-id="card-feature-capture"
            >
              <Mic class="h-4 w-4" aria-hidden="true" />
            </FeatureCard>
            <FeatureCard
              title="Turns it into clarity"
              body="Summaries, action items, and searchable memory."
              test-id="card-feature-clarity"
            >
              <Sparkles class="h-4 w-4" aria-hidden="true" />
            </FeatureCard>
          </div>

          <div
            class="mt-6 text-xs leading-relaxed text-white/50"
            data-testid="text-disclaimer"
          >
            Ambio is a private build. Details will evolve — the feeling won't.
          </div>
        </section>

        <aside class="lg:sticky lg:top-10 animate-enter-delay">
          <div class="rounded-3xl border border-white/10 bg-white/[0.02] p-5 shadow-md backdrop-blur sm:p-6">
            <div class="flex items-center justify-between">
              <div class="text-xs font-medium text-white/60" data-testid="text-right-kicker">
                What it is
              </div>
              <div
                class="hidden rounded-full border border-white/10 bg-white/[0.02] px-2.5 py-1 text-[11px] text-white/60 sm:inline-flex"
                data-testid="badge-status"
              >
                Prototype
              </div>
            </div>

            <div class="mt-4 flex items-center justify-center">
              <div class="floaty">
                <Sigil />
              </div>
            </div>

            <div class="mt-6 space-y-3">
              <div
                class="rounded-2xl border border-white/10 bg-white/[0.02] px-4 py-3"
                data-testid="card-about"
              >
                <div class="text-sm font-semibold tracking-tight text-white/90">
                  Always-on. Selectively.
                </div>
                <div class="mt-1 text-sm leading-relaxed text-white/70">
                  Built to capture conversations, not televisions. Guardrails are part
                  of the product.
                </div>
              </div>

              <div
                class="rounded-2xl border border-white/10 bg-white/[0.02] px-4 py-3"
                data-testid="card-processing"
              >
                <div class="text-sm font-semibold tracking-tight text-white/90">
                  Local-first processing.
                </div>
                <div class="mt-1 text-sm leading-relaxed text-white/70">
                  Whisper + local models when available, cloud when necessary.
                </div>
              </div>

              <div
                class="rounded-2xl border border-white/10 bg-white/[0.02] px-4 py-3"
                data-testid="card-design"
              >
                <div class="text-sm font-semibold tracking-tight text-white/90">
                  A device you'll actually wear.
                </div>
                <div class="mt-1 text-sm leading-relaxed text-white/70">
                  A pendant, a desktop mic, and an inline Bluetooth proxy — one system.
                </div>
              </div>
            </div>

            <div class="mt-6 border-t border-white/10 pt-4">
              <div class="text-xs text-white/60" data-testid="text-cta-secondary">
                Want to shape it?
              </div>
              <a
                href="mailto:hello@ambio.system?subject=Ambio%20early%20access"
                class="mt-2 inline-flex items-center gap-2 text-sm font-semibold text-white/90 hover:text-white"
                data-testid="link-email"
              >
                Send a note
                <ArrowRight class="h-4 w-4" aria-hidden="true" />
              </a>
            </div>
          </div>

          <div
            class="mt-5 rounded-3xl border border-white/10 bg-white/[0.02] p-5 text-xs text-white/55 shadow-sm backdrop-blur"
            data-testid="card-footer"
          >
            <div class="flex items-start justify-between gap-3">
              <div class="min-w-0">
                <div class="text-white/75" data-testid="text-footer-title">
                  Quietly ambitious.
                </div>
                <div class="mt-1 leading-relaxed" data-testid="text-footer-body">
                  Designed to turn the infinite stream of daily speech into something
                  you can search, reflect on, and act from.
                </div>
              </div>
              <div
                class="mt-0.5 h-2.5 w-2.5 flex-none rounded-full bg-accent/90 shadow-[0_0_0_6px_hsl(var(--accent)/0.10)]"
                aria-hidden="true"
              />
            </div>
          </div>
        </aside>
      </main>
    </div>
  </div>
</template>
