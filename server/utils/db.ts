import { drizzle } from "drizzle-orm/d1";
import * as schema from "../database/schema";
import type { H3Event } from "h3";

export function useDB(event: H3Event) {
  const { cloudflare } = event.context;
  return drizzle(cloudflare.env.DB, { schema });
}
