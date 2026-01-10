<script setup lang="ts">
// TODO: Make this Catppuccin green
const email = ref("");
const isSubmitting = ref(false);
const isSubmitted = ref(false);
const errorMessage = ref("");
const { isDark } = useTheme();

async function handleSubmit() {
  if (!email.value || isSubmitting.value) return;

  // Basic email validation
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(email.value)) {
    errorMessage.value = "Please enter a valid email address";
    return;
  }

  isSubmitting.value = true;
  errorMessage.value = "";

  try {
    const response = await $fetch("/api/subscribe", {
      method: "POST",
      body: { email: email.value },
    });

    if (response.success) {
      isSubmitted.value = true;
      email.value = "";
    } else {
      errorMessage.value = response.message || "Something went wrong. Please try again.";
    }
  } catch {
    errorMessage.value = "Failed to submit. Please try again later.";
  } finally {
    isSubmitting.value = false;
  }
}

function resetForm() {
  isSubmitted.value = false;
  errorMessage.value = "";
}
</script>

<template>
  <div class="w-full max-w-md">
    <Transition name="form-slide" mode="out-in">
      <!-- Success State -->
      <div v-if="isSubmitted" key="success" class="text-center space-y-4">
        <div class="flex justify-center">
          <div class="rounded-full bg-teal/20 p-4">
            <Icon name="ph:check-circle-bold" class="h-8 w-8 text-teal" />
          </div>
        </div>
        <p class="text-lg font-medium" :class="isDark ? 'text-base-content/80' : 'text-base-content/90'">We'll be in touch.</p>
        <p class="text-sm" :class="isDark ? 'text-base-content/50' : 'text-base-content/80'">
          Something is coming. You'll be among the first to know.
        </p>
        <button class="btn btn-ghost btn-sm text-base-content/50 hover:text-base-content" @click="resetForm">
          Submit another
        </button>
      </div>

      <!-- Form State -->
      <form v-else key="form" class="space-y-4" @submit.prevent="handleSubmit">
        <div class="join w-full">
          <input
            v-model="email"
            type="email"
            placeholder="your@email.com"
            class="input input-bordered join-item flex-1 bg-base-200/50 backdrop-blur-sm border-base-300 focus:border-primary focus:outline-none"
            :class="isDark ? 'placeholder:text-base-content/30' : 'placeholder:text-base-content/50'"
            :disabled="isSubmitting"
            autocomplete="email"
            required
          />
          <button
            type="submit"
            class="btn join-item bg-primary hover:bg-primary/80 border-primary text-primary-content"
            :disabled="isSubmitting || !email"
          >
            <template v-if="isSubmitting">
              <span class="loading loading-spinner loading-sm"></span>
            </template>
            <template v-else>
              <span>Notify me</span>
            </template>
          </button>
        </div>

        <Transition name="error-fade">
          <p v-if="errorMessage" class="text-error text-sm text-center">
            {{ errorMessage }}
          </p>
        </Transition>

        <p class="text-xs text-center" :class="isDark ? 'text-base-content/40' : 'text-base-content/70'">
          No spam. Just one email when we're ready.
        </p>
      </form>
    </Transition>
  </div>
</template>

<style scoped>
.form-slide-enter-active,
.form-slide-leave-active {
  transition: all 0.3s ease;
}

.form-slide-enter-from {
  opacity: 0;
  transform: translateY(10px);
}

.form-slide-leave-to {
  opacity: 0;
  transform: translateY(-10px);
}

.error-fade-enter-active,
.error-fade-leave-active {
  transition: opacity 0.2s ease;
}

.error-fade-enter-from,
.error-fade-leave-to {
  opacity: 0;
}
</style>
