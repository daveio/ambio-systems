import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";
import { requireRateLimit } from "../utils/ratelimit";

export default defineEventHandler(async (event) => {
  // Rate limiting: 5 requests per 60 seconds per IP
  await requireRateLimit(event, { limit: 5, window: 60 });

  const body = await readBody<{ email: string }>(event);

  // Validation
  if (!body.email) {
    throw createError({ statusCode: 400, message: "Email is required" });
  }

  // More robust email validation
  // This regex handles most valid email formats including internationalized domains
  const emailRegex =
    /^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;
  if (!emailRegex.test(body.email)) {
    throw createError({ statusCode: 400, message: "Invalid email format" });
  }

  const db = useDB(event);
  const { cloudflare } = event.context;
  const cf = cloudflare.cf; // Geolocation data

  // Extract metadata
  const ipAddress =
    getHeader(event, "cf-connecting-ip") || getHeader(event, "x-forwarded-for");
  const userAgent = getHeader(event, "user-agent");

  // Type guards for geolocation data
  const getString = (value: unknown): string | undefined => {
    return typeof value === "string" ? value : undefined;
  };

  const getNumber = (value: unknown): number | undefined => {
    if (typeof value === "number") return value;
    if (typeof value === "string") {
      const num = parseFloat(value);
      return !isNaN(num) ? num : undefined;
    }
    return undefined;
  };

  // Extract and validate geolocation data
  const country = getString(cf?.country);
  const city = getString(cf?.city);
  const region = getString(cf?.region);
  const timezone = getString(cf?.timezone);
  const latitude = getNumber(cf?.latitude);
  const longitude = getNumber(cf?.longitude);

  // Normalize email
  const normalizedEmail = body.email.toLowerCase().trim();
  const MAX_EMAIL_LENGTH = 254;
  if (normalizedEmail.length > MAX_EMAIL_LENGTH) {
    throw createError({ statusCode: 400, message: "Email is too long" });
  }

  // Check if user already exists
  const existing = await db
    .select()
    .from(subscriptions)
    .where(eq(subscriptions.email, normalizedEmail))
    .get();

  // If user was previously unsubscribed, don't reactivate without explicit consent
  if (existing && existing.status === "unsubscribed") {
    throw createError({
      statusCode: 400,
      message:
        "This email was previously unsubscribed. Please contact support to reactivate.",
    });
  }

  // Use upsert to avoid race condition on concurrent requests
  // This is atomic and handles both insert and update in a single operation
  await db
    .insert(subscriptions)
    .values({
      email: normalizedEmail,
      ipAddress,
      userAgent,
      country,
      city,
      region,
      timezone,
      latitude,
      longitude,
    })
    .onConflictDoUpdate({
      target: subscriptions.email,
      set: {
        updatedAt: new Date(),
        // Update location data in case it changed (but don't change status)
        ipAddress,
        userAgent,
        country,
        city,
        region,
        timezone,
        latitude,
        longitude,
      },
    });

  return { success: true, message: "Successfully subscribed" };
});
