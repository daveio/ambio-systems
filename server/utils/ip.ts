import type { H3Event } from "h3";

/**
 * Get the client IP address from the request
 * Prioritizes Cloudflare's cf-connecting-ip header, falls back to x-forwarded-for
 * @param event - The H3 event object
 * @returns The client IP address or "unknown" if not available
 */
export function getClientIp(event: H3Event): string {
  return (
    getHeader(event, "cf-connecting-ip") ||
    getHeader(event, "x-forwarded-for") ||
    "unknown"
  );
}
