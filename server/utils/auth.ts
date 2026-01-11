import type { H3Event } from "h3";
import { timingSafeEqual } from "node:crypto";

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

  // timingSafeEqual requires buffers of equal length
  // If lengths differ, keys are definitely not equal, but we still
  // need to do a constant-time comparison to prevent timing attacks
  const storedBuffer = Buffer.from(storedKey, "utf-8");
  const providedBuffer = Buffer.from(providedKey, "utf-8");

  if (storedBuffer.length !== providedBuffer.length) {
    return false;
  }

  // Use constant-time comparison to prevent timing attacks
  return timingSafeEqual(storedBuffer, providedBuffer);
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
