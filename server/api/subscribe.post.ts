import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";
import { validateGeolocationData } from "../utils/geolocation";

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

  // Extract and validate geolocation data
  const geolocation = validateGeolocationData(cloudflare.cf);

  // Extract metadata
  const ipAddress =
    getHeader(event, "cf-connecting-ip") || getHeader(event, "x-forwarded-for");
  const userAgent = getHeader(event, "user-agent");

  // Normalize email
  const normalizedEmail = body.email.toLowerCase().trim();

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
        ...geolocation,
      })
      .where(eq(subscriptions.email, normalizedEmail));

    return { success: true, message: "Subscription updated" };
  }

  // Insert new subscription
  await db.insert(subscriptions).values({
    email: normalizedEmail,
    ipAddress,
    userAgent,
    ...geolocation,
  });

  return { success: true, message: "Successfully subscribed" };
});
