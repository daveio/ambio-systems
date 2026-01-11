import { defineConfig } from "drizzle-kit";

const CLOUDFLARE_ACCOUNT_ID = process.env.CLOUDFLARE_ACCOUNT_ID;
const CLOUDFLARE_DATABASE_ID = process.env.CLOUDFLARE_DATABASE_ID;
const CLOUDFLARE_D1_TOKEN = process.env.CLOUDFLARE_D1_TOKEN;

if (!CLOUDFLARE_ACCOUNT_ID) {
  throw new Error(
    "Environment variable CLOUDFLARE_ACCOUNT_ID is required for drizzle D1 configuration.",
  );
}

if (!CLOUDFLARE_DATABASE_ID) {
  throw new Error(
    "Environment variable CLOUDFLARE_DATABASE_ID is required for drizzle D1 configuration.",
  );
}

if (!CLOUDFLARE_D1_TOKEN) {
  throw new Error(
    "Environment variable CLOUDFLARE_D1_TOKEN is required for drizzle D1 configuration.",
  );
}

export default defineConfig({
  schema: "./server/database/schema.ts",
  out: "./drizzle",
  dialect: "sqlite",
  driver: "d1-http",
  dbCredentials: {
    accountId: CLOUDFLARE_ACCOUNT_ID,
    databaseId: CLOUDFLARE_DATABASE_ID,
    token: CLOUDFLARE_D1_TOKEN,
  },
});
