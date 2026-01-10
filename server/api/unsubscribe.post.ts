import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";

export default defineEventHandler(async (event) => {
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

  if (!existing) {
    throw createError({ statusCode: 404, message: "Subscription not found" });
  }

  if (existing.status === "unsubscribed") {
    return { success: true, message: "Already unsubscribed" };
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
