export default defineEventHandler(async (event) => {
  const body = await readBody<{ email: string }>(event);

  if (!body.email) {
    return {
      success: false,
      message: "Email is required",
    };
  }

  // Basic email validation
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(body.email)) {
    return {
      success: false,
      message: "Invalid email format",
    };
  }

  // TODO: Implement actual email storage
  // This is a stub that simulates success
  // In production, you would:
  // - Store in a database
  // - Add to a mailing list service (Mailchimp, SendGrid, etc.)
  // - Send a confirmation email

  console.log(`[Subscription] New signup: ${body.email}`);

  // Simulate a small delay for realism
  await new Promise((resolve) => setTimeout(resolve, 500));

  return {
    success: true,
    message: "Successfully subscribed",
  };
});
