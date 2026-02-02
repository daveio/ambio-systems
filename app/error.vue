<script setup lang="ts">
import { AlertCircle } from "lucide-vue-next"

defineProps<{
  error: {
    statusCode: number
    message: string
  }
}>()

const handleClear = () => clearError({ redirect: "/" })
</script>

<template>
  <div class="min-h-screen w-full flex items-center justify-center bg-background">
    <div class="card w-full max-w-md mx-4 bg-base-200 border border-white/10">
      <div class="card-body">
        <div class="flex mb-4 gap-2">
          <AlertCircle class="h-8 w-8 text-error" />
          <h1 class="text-2xl font-bold text-foreground">
            {{ error.statusCode }} {{ error.statusCode === 404 ? 'Page Not Found' : 'Error' }}
          </h1>
        </div>

        <p class="mt-4 text-sm text-white/60">
          {{ error.message || "The page you're looking for doesn't exist." }}
        </p>

        <div class="card-actions mt-6">
          <button class="btn btn-primary" @click="handleClear">
            Go home
          </button>
        </div>
      </div>
    </div>
  </div>
</template>
