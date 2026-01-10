import { desc, eq, sql } from "drizzle-orm";
import { subscriptions } from "../../database/schema";
import { useDB } from "../../utils/db";
import { requireAdminAuth } from "../../utils/auth";

export default defineEventHandler(async (event) => {
  // Validate API key from Secrets Store
  await requireAdminAuth(event);

  const db = useDB(event);
  const query = getQuery(event);

  const format = (query.format as string) || "json";
  const limit = Math.min(Number(query.limit) || 100, 1000); // Cap at 1000
  const offset = Number(query.offset) || 0;
  const status = query.status as "active" | "unsubscribed" | undefined;

  // Build query with optional status filter
  let results;
  if (status) {
    results = await db
      .select()
      .from(subscriptions)
      .where(eq(subscriptions.status, status))
      .orderBy(desc(subscriptions.createdAt))
      .limit(limit)
      .offset(offset);
  } else {
    results = await db
      .select()
      .from(subscriptions)
      .orderBy(desc(subscriptions.createdAt))
      .limit(limit)
      .offset(offset);
  }

  // Get total count (with same filter if applicable)
  let countResult;
  if (status) {
    countResult = await db
      .select({ count: sql<number>`count(*)` })
      .from(subscriptions)
      .where(eq(subscriptions.status, status));
  } else {
    countResult = await db
      .select({ count: sql<number>`count(*)` })
      .from(subscriptions);
  }
  const total = countResult[0]?.count ?? 0;

  // Return CSV format if requested
  if (format === "csv") {
    const headers = [
      "id",
      "email",
      "status",
      "ip_address",
      "user_agent",
      "country",
      "city",
      "region",
      "timezone",
      "latitude",
      "longitude",
      "created_at",
      "updated_at",
    ];

    const escapeCSV = (value: string | null | undefined): string => {
      if (value === null || value === undefined) return "";
      const str = String(value);
      // Escape quotes and wrap in quotes if contains comma, quote, or newline
      if (str.includes(",") || str.includes('"') || str.includes("\n")) {
        return `"${str.replace(/"/g, '""')}"`;
      }
      return str;
    };

    const csvRows = [
      headers.join(","),
      ...results.map((row) =>
        [
          row.id,
          escapeCSV(row.email),
          row.status,
          escapeCSV(row.ipAddress),
          escapeCSV(row.userAgent),
          escapeCSV(row.country),
          escapeCSV(row.city),
          escapeCSV(row.region),
          escapeCSV(row.timezone),
          escapeCSV(row.latitude),
          escapeCSV(row.longitude),
          row.createdAt?.toISOString() || "",
          row.updatedAt?.toISOString() || "",
        ].join(","),
      ),
    ];

    setHeader(event, "Content-Type", "text/csv; charset=utf-8");
    setHeader(
      event,
      "Content-Disposition",
      `attachment; filename="subscriptions-${new Date().toISOString().split("T")[0]}.csv"`,
    );

    return csvRows.join("\n");
  }

  // Default: JSON format
  return {
    subscriptions: results,
    pagination: {
      total,
      limit,
      offset,
      hasMore: offset + results.length < total,
    },
  };
});
