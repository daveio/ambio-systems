/**
 * Safely validates and extracts geolocation data from Cloudflare's cf object.
 * Ensures all values are properly typed before storing in the database.
 */

interface CloudflareGeolocationData {
  country?: unknown;
  city?: unknown;
  region?: unknown;
  timezone?: unknown;
  latitude?: unknown;
  longitude?: unknown;
}

interface ValidatedGeolocationData {
  country?: string;
  city?: string;
  region?: string;
  timezone?: string;
  latitude?: string;
  longitude?: string;
}

/**
 * Type guard to check if a value is a string.
 */
function isString(value: unknown): value is string {
  return typeof value === "string";
}

/**
 * Type guard to check if a value is a number.
 */
function isNumber(value: unknown): value is number {
  return typeof value === "number" && !isNaN(value);
}

/**
 * Safely converts a value to a string if it's a string or number.
 * Returns undefined for all other types.
 */
function toSafeString(value: unknown): string | undefined {
  if (isString(value)) {
    return value;
  }
  if (isNumber(value)) {
    return String(value);
  }
  return undefined;
}

/**
 * Validates and extracts geolocation data from Cloudflare's cf object.
 * Performs type checking to ensure only valid string data is returned.
 *
 * @param cf - The Cloudflare cf object containing geolocation properties
 * @returns Validated geolocation data with proper types
 */
export function validateGeolocationData(
  cf?: CloudflareGeolocationData,
): ValidatedGeolocationData {
  if (!cf) {
    return {};
  }

  return {
    country: toSafeString(cf.country),
    city: toSafeString(cf.city),
    region: toSafeString(cf.region),
    timezone: toSafeString(cf.timezone),
    latitude: toSafeString(cf.latitude),
    longitude: toSafeString(cf.longitude),
  };
}
