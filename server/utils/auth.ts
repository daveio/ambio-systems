import type { H3Event } from "h3";

export async function validateAdminApiKey(event: H3Event): Promise<boolean> {
  const { cloudflare } = event.context;

  // Get API key from Authorization header
  const authHeader = getHeader(event, "authorization");
  if (!authHeader?.startsWith("Bearer ")) {
    return false;
  }

  const providedKey = authHeader.substring(7);

  // Secrets Store requires async .get() call
  const storedKey = await cloudflare.env.ADMIN_API_KEY.get();

  if (!storedKey || !providedKey) {
    return false;
  }

  return storedKey === providedKey;
}

export async function requireAdminAuth(event: H3Event): Promise<void> {
  const isValid = await validateAdminApiKey(event);

  if (!isValid) {
    throw createError({
      statusCode: 401,
      message: "Unauthorized: Invalid or missing API key",
    });
  }
}
