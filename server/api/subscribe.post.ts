import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";
import { requireRateLimit } from "../utils/ratelimit";

export default defineEventHandler(async (event) => {
  // Rate limiting: 5 requests per 60 seconds per IP
  await requireRateLimit(event, { limit: 5, window: 60 });

  const body = await readBody<{ email: string }>(event);

  // Validation - check body exists first to avoid TypeError on null body
  if (!body || !body.email) {
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
  const { cf } = cloudflare; // Geolocation data

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
      return isNaN(num) ? undefined : num;
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

  // Atomic upsert: insert new subscription or update metadata for existing active users.
  // The ON CONFLICT clause only updates metadata, never the status field.
  // This ensures unsubscribed users remain unsubscribed.
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

  // Post-upsert verification: check if the email belongs to an unsubscribed user.
  // This eliminates the TOCTOU race condition by checking AFTER the write completes.
  // Even if concurrent requests occur, we correctly reject resubscription attempts.
  const record = await db
    .select({ status: subscriptions.status })
    .from(subscriptions)
    .where(eq(subscriptions.email, normalizedEmail))
    .get();

  if (record?.status === "unsubscribed") {
    throw createError({
      statusCode: 400,
      message:
        "This email was previously unsubscribed. Please contact support to reactivate.",
    });
  }

  return { success: true, message: "Successfully subscribed" };
});
