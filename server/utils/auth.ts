import type { H3Event } from "h3";
import { timingSafeEqual } from "node:crypto";

/**
 * Constant-time string comparison to prevent timing attacks
 */
function constantTimeEqual(a: string, b: string): boolean {
  // Early return if lengths differ (this itself is not timing-safe,
  // but if lengths differ, the keys are definitely not equal)
  if (a.length !== b.length) {
    return false;
  }

  // Convert strings to buffers for constant-time comparison
  const bufferA = Buffer.from(a, "utf8");
  const bufferB = Buffer.from(b, "utf8");

  return timingSafeEqual(bufferA, bufferB);
}

export async function validateAdminApiKey(event: H3Event): Promise<boolean> {
  const { cloudflare } = event.context;

  // Get API key from Authorization header
  const authHeader = getHeader(event, "authorization");
  if (!authHeader?.startsWith("Bearer ")) {
    return false;
  }

  const providedKey = authHeader.substring(7);

  // Secrets Store requires async .get() call
  const storedKey = await cloudflare.env.ADMIN_API_KEY.get();

  if (!storedKey || !providedKey) {
    return false;
  }

  return constantTimeEqual(storedKey, providedKey);
}

export async function requireAdminAuth(event: H3Event): Promise<void> {
  const isValid = await validateAdminApiKey(event);

  if (!isValid) {
    throw createError({
      statusCode: 401,
      message: "Unauthorized: Invalid or missing API key",
    });
  }
}
