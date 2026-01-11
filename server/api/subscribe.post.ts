import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";

export default defineEventHandler(async (event) => {
  const body = await readBody<{ email: string }>(event);

  // Validation
  if (!body.email) {
    throw createError({ statusCode: 400, message: "Email is required" });
  }

  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
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

  // Normalize email
  const normalizedEmail = body.email.toLowerCase().trim();
  const MAX_EMAIL_LENGTH = 254;
  if (normalizedEmail.length > MAX_EMAIL_LENGTH) {
    throw createError({ statusCode: 400, message: "Email is too long" });
  }

  // Check for existing subscription
  const existing = await db
    .select()
    .from(subscriptions)
    .where(eq(subscriptions.email, normalizedEmail))
    .get();

  if (existing) {
    // Update timestamp and metadata of existing entry
    await db
      .update(subscriptions)
      .set({
        updatedAt: new Date(),
        status: "active", // Reactivate if previously unsubscribed
        // Update location data in case it changed
        ipAddress,
        userAgent,
        country: cf?.country as string | undefined,
        city: cf?.city as string | undefined,
        region: cf?.region as string | undefined,
        timezone: cf?.timezone as string | undefined,
        latitude: cf?.latitude as string | undefined,
        longitude: cf?.longitude as string | undefined,
      })
      .where(eq(subscriptions.email, normalizedEmail));

    return { success: true, message: "Subscription updated" };
  }

  // Insert new subscription
  await db.insert(subscriptions).values({
    email: normalizedEmail,
    ipAddress,
    userAgent,
    country: cf?.country as string | undefined,
    city: cf?.city as string | undefined,
    region: cf?.region as string | undefined,
    timezone: cf?.timezone as string | undefined,
    latitude: cf?.latitude as string | undefined,
    longitude: cf?.longitude as string | undefined,
  });

  return { success: true, message: "Successfully subscribed" };
});
