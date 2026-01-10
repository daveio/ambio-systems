CREATE TABLE `subscriptions` (
	`id` integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	`email` text NOT NULL,
	`status` text DEFAULT 'active' NOT NULL,
	`ip_address` text,
	`user_agent` text,
	`country` text,
	`city` text,
	`region` text,
	`timezone` text,
	`latitude` text,
	`longitude` text,
	`created_at` integer NOT NULL,
	`updated_at` integer NOT NULL
);
--> statement-breakpoint
CREATE UNIQUE INDEX `subscriptions_email_idx` ON `subscriptions` (`email`);