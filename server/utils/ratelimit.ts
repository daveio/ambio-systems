import type { H3Event } from "h3";

interface RateLimitConfig {
  limit: number; // Number of requests allowed
  window: number; // Time window in seconds
}

/**
 * Simple in-memory rate limiter for Cloudflare Workers
 * Uses Cloudflare's Request metadata for client identification
 *
 * For production, consider using Cloudflare's native rate limiting:
 * - Cloudflare Rate Limiting Rules (Dashboard)
 * - Cloudflare Workers Rate Limiting API
 * - Durable Objects for distributed rate limiting
 */
export async function checkRateLimit(
  event: H3Event,
  config: RateLimitConfig,
): Promise<boolean> {
  const { cloudflare } = event.context;

  // Get client identifier (IP address)
  const clientIp =
    getHeader(event, "cf-connecting-ip") || getHeader(event, "x-forwarded-for");

  if (!clientIp) {
    // If we can't identify the client, allow the request
    // (This shouldn't happen in Cloudflare Workers)
    return true;
  }

  // Use KV namespace for distributed rate limiting if available
  // This requires setting up a KV namespace in wrangler.toml
  const kv = cloudflare.env.RATE_LIMIT_KV;

  if (!kv) {
    // KV not configured, skip rate limiting
    // In production, you should configure KV or use Cloudflare's native rate limiting
    console.warn(
      "Rate limiting KV namespace not configured. Skipping rate limit check.",
    );
    return true;
  }

  const key = `ratelimit:${clientIp}`;
  const now = Math.floor(Date.now() / 1000); // Current timestamp in seconds

  try {
    // Get current rate limit data
    const data = await kv.get(key, { type: "json" });
    const current = (data as { count: number; resetAt: number }) || {
      count: 0,
      resetAt: now + config.window,
    };

    // Check if window has expired
    if (now > current.resetAt) {
      // Reset the counter
      await kv.put(
        key,
        JSON.stringify({ count: 1, resetAt: now + config.window }),
        { expirationTtl: config.window },
      );
      return true;
    }

    // Check if limit exceeded
    if (current.count >= config.limit) {
      return false;
    }

    // Increment counter
    const ttl = current.resetAt - now;
    await kv.put(
      key,
      JSON.stringify({ count: current.count + 1, resetAt: current.resetAt }),
      { expirationTtl: ttl > 0 ? ttl : config.window },
    );

    return true;
  } catch (error) {
    // On error, allow the request (fail open)
    console.error("Rate limit check failed:", error);
    return true;
  }
}

export async function requireRateLimit(
  event: H3Event,
  config: RateLimitConfig,
): Promise<void> {
  const allowed = await checkRateLimit(event, config);

  if (!allowed) {
    throw createError({
      statusCode: 429,
      message: "Too many requests. Please try again later.",
    });
  }
}
