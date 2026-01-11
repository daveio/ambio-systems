PRAGMA foreign_keys=OFF;--> statement-breakpoint
CREATE TABLE `__new_subscriptions` (
	`id` integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	`email` text NOT NULL,
	`status` text DEFAULT 'active' NOT NULL,
	`ip_address` text,
	`user_agent` text,
	`country` text,
	`city` text,
	`region` text,
	`timezone` text,
	`latitude` real,
	`longitude` real,
	`created_at` integer NOT NULL,
	`updated_at` integer NOT NULL
);
--> statement-breakpoint
INSERT INTO `__new_subscriptions`("id", "email", "status", "ip_address", "user_agent", "country", "city", "region", "timezone", "latitude", "longitude", "created_at", "updated_at") SELECT "id", "email", "status", "ip_address", "user_agent", "country", "city", "region", "timezone", "latitude", "longitude", "created_at", "updated_at" FROM `subscriptions`;--> statement-breakpoint
DROP TABLE `subscriptions`;--> statement-breakpoint
ALTER TABLE `__new_subscriptions` RENAME TO `subscriptions`;--> statement-breakpoint
PRAGMA foreign_keys=ON;--> statement-breakpoint
CREATE UNIQUE INDEX `subscriptions_email_idx` ON `subscriptions` (`email`);