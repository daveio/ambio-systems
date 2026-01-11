import type { H3Event } from "h3";

/**
 * Apply rate limiting to an endpoint using Cloudflare's Rate Limiting API
 * @param event - The H3 event object
 * @param key - The key to rate limit on (e.g., IP address, email)
 * @throws {Error} If rate limit is exceeded
 */
export async function applyRateLimit(event: H3Event, key: string) {
  const { cloudflare } = event.context;

  // Check if RATE_LIMITER binding is available
  if (!cloudflare?.env?.RATE_LIMITER) {
    console.warn("Rate limiter binding not available - skipping rate limit check");
    return;
  }

  // Apply rate limit
  const { success } = await cloudflare.env.RATE_LIMITER.limit({ key });

  if (!success) {
    throw createError({
      statusCode: 429,
      message: "Too many requests. Please try again later.",
    });
  }
}
