import {
  sqliteTable,
  text,
  integer,
  real,
  uniqueIndex,
} from "drizzle-orm/sqlite-core";

export const subscriptions = sqliteTable(
  "subscriptions",
  {
    id: integer("id").primaryKey({ autoIncrement: true }),
    email: text("email").notNull(),
    status: text("status", { enum: ["active", "unsubscribed"] })
      .notNull()
      .default("active"),
    ipAddress: text("ip_address"),
    userAgent: text("user_agent"),
    country: text("country"),
    city: text("city"),
    region: text("region"),
    timezone: text("timezone"),
    latitude: real("latitude"),
    longitude: real("longitude"),
    createdAt: integer("created_at", { mode: "timestamp" })
      .notNull()
      .$defaultFn(() => new Date()),
    updatedAt: integer("updated_at", { mode: "timestamp" })
      .notNull()
      .$defaultFn(() => new Date()),
  },
  (table) => [uniqueIndex("subscriptions_email_idx").on(table.email)],
);

export type Subscription = typeof subscriptions.$inferSelect;
export type NewSubscription = typeof subscriptions.$inferInsert;
