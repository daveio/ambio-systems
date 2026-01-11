import type { H3Event } from "h3";
import { timingSafeEqual } from "node:crypto";

/**
 * Constant-time string comparison to prevent timing attacks
 */
function constantTimeEqual(a: string, b: string): boolean {
  // Validate non-empty inputs
  if (!a || !b) {
    return false;
  }

  // Convert strings to buffers for constant-time comparison
  const bufferA = Buffer.from(a, "utf8");
  const bufferB = Buffer.from(b, "utf8");

  // timingSafeEqual requires buffers of equal length
  // If lengths differ, still perform comparison on equal-length buffers
  // to avoid timing attacks based on length differences
  if (bufferA.length !== bufferB.length) {
    // Create dummy buffer of same length as bufferA to compare against
    const dummyBuffer = Buffer.alloc(bufferA.length);
    timingSafeEqual(bufferA, dummyBuffer);
    return false;
  }

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
