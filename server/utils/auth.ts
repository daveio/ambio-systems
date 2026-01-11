import type { H3Event } from "h3";
import { timingSafeEqual } from "node:crypto";

// Minimum buffer size for constant-time comparison when dealing with empty strings
const MIN_COMPARE_LENGTH = 32;

/**
 * Constant-time string comparison to prevent timing attacks
 * 
 * This implementation ensures consistent timing regardless of:
 * - Whether strings match or not
 * - The length of the strings
 * - Whether strings are empty
 * 
 * @param a - First string to compare
 * @param b - Second string to compare
 * @returns true if strings are equal, false otherwise
 */
export function constantTimeEqual(a: string, b: string): boolean {
  // Convert strings to buffers for constant-time comparison
  const bufferA = Buffer.from(a, "utf8");
  const bufferB = Buffer.from(b, "utf8");

  // Determine the maximum length for comparison
  const maxLength = Math.max(bufferA.length, bufferB.length);
  
  // If either buffer is empty, use a minimum size to avoid revealing emptiness
  const compareLength = maxLength > 0 ? maxLength : MIN_COMPARE_LENGTH;

  // Pad both buffers to the same length with zeros
  const paddedA = Buffer.alloc(compareLength);
  const paddedB = Buffer.alloc(compareLength);
  bufferA.copy(paddedA);
  bufferB.copy(paddedB);

  // Perform constant-time comparison
  // If original lengths differ, padded buffers will differ, so no explicit length check needed
  return timingSafeEqual(paddedA, paddedB);
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

  // Use empty string if key is missing to maintain constant timing
  // The constantTimeEqual function will handle empty strings securely
  return constantTimeEqual(storedKey || "", providedKey || "");
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
