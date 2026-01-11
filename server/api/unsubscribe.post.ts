import { eq } from "drizzle-orm";
import { subscriptions } from "../database/schema";
import { useDB } from "../utils/db";
import { validateAndNormalizeEmail } from "../utils/validation";

export default defineEventHandler(async (event) => {
  const body = await readBody<{ email: string }>(event);

  // Validate and normalize email (throws if invalid)
  const normalizedEmail = validateAndNormalizeEmail(body.email);

  const db = useDB(event);

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
