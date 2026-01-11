import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";
import { requireRateLimit } from "../utils/ratelimit";

export default defineEventHandler(async (event) => {
  // Rate limiting: 5 requests per 60 seconds per IP
  await requireRateLimit(event, { limit: 5, window: 60 });

  const body = await readBody<{ email: string }>(event);

  if (!body.email) {
    throw createError({ statusCode: 400, message: "Email is required" });
  }

  const db = useDB(event);
  const normalizedEmail = body.email.toLowerCase().trim();

  // Check if subscription exists
  const existing = await db
    .select()
    .from(subscriptions)
    .where(eq(subscriptions.email, normalizedEmail))
    .get();

  // Return generic success even if subscription doesn't exist (privacy)
  if (!existing) {
    return { success: true, message: "Successfully unsubscribed" };
  }

  if (existing.status === "unsubscribed") {
    return { success: true, message: "Successfully unsubscribed" };
  }

  // Soft delete - mark as unsubscribed
  await db
    .update(subscriptions)
    .set({
      status: "unsubscribed",
      updatedAt: new Date(),
    })
    .where(eq(subscriptions.email, normalizedEmail));

  return { success: true, message: "Successfully unsubscribed" };
});
