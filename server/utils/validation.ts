/**
 * Email validation utility using a more comprehensive approach
 * 
 * This validator handles edge cases including:
 * - Multiple @ symbols
 * - Invalid domain formats
 * - Missing or invalid TLD
 * - Special characters in incorrect positions
 * - Internationalized domain names (basic support)
 */

/**
 * Validates an email address using a comprehensive regex pattern
 * Based on RFC 5322 simplified for practical use
 * 
 * @param email - The email address to validate
 * @returns true if valid, false otherwise
 */
export function isValidEmail(email: string): boolean {
  if (!email || typeof email !== 'string') {
    return false;
  }

  // Basic length checks
  if (email.length > 254) {
    return false; // Max email length per RFC 5321
  }

  // Trim whitespace
  const trimmedEmail = email.trim();
  
  if (trimmedEmail.length === 0) {
    return false;
  }

  // Check for exactly one @ symbol
  const atCount = (trimmedEmail.match(/@/g) || []).length;
  if (atCount !== 1) {
    return false;
  }

  // Split into local and domain parts
  const [localPart, domainPart] = trimmedEmail.split('@');

  // Validate local part (before @)
  if (!localPart || localPart.length > 64) {
    return false; // Max local part length per RFC 5321
  }

  // Local part cannot start or end with a dot
  if (localPart.startsWith('.') || localPart.endsWith('.')) {
    return false;
  }

  // Local part cannot have consecutive dots
  if (localPart.includes('..')) {
    return false;
  }

  // Validate local part characters (alphanumeric, dots, hyphens, underscores, plus)
  const localPartRegex = /^[a-zA-Z0-9.!#$%&'*+\/=?^_`{|}~-]+$/;
  if (!localPartRegex.test(localPart)) {
    return false;
  }

  // Validate domain part (after @)
  if (!domainPart || domainPart.length > 253) {
    return false; // Max domain length per RFC 1035
  }

  // Domain cannot start or end with a hyphen or dot
  if (domainPart.startsWith('-') || domainPart.endsWith('-') ||
      domainPart.startsWith('.') || domainPart.endsWith('.')) {
    return false;
  }

  // Domain must have at least one dot (TLD required)
  if (!domainPart.includes('.')) {
    return false;
  }

  // Split domain into labels
  const domainLabels = domainPart.split('.');

  // Each label must be valid
  for (const label of domainLabels) {
    if (!label || label.length > 63) {
      return false; // Max label length per RFC 1035
    }

    // Label cannot start or end with hyphen
    if (label.startsWith('-') || label.endsWith('-')) {
      return false;
    }

    // Label must contain only alphanumeric and hyphens (basic ASCII)
    // For internationalized domains, they should be punycode encoded
    const labelRegex = /^[a-zA-Z0-9-]+$/;
    if (!labelRegex.test(label)) {
      return false;
    }
  }

  // TLD (last label) should be at least 2 characters and alphabetic
  const tld = domainLabels[domainLabels.length - 1];
  if (tld.length < 2 || !/^[a-zA-Z]+$/.test(tld)) {
    return false;
  }

  return true;
}

/**
 * Normalizes an email address by trimming and converting to lowercase
 * 
 * @param email - The email address to normalize
 * @returns Normalized email address
 */
export function normalizeEmail(email: string): string {
  return email.trim().toLowerCase();
}

/**
 * Validates and normalizes an email address
 * Throws an error if invalid
 * 
 * @param email - The email address to validate and normalize
 * @returns Normalized email address
 * @throws Error if email is invalid
 */
export function validateAndNormalizeEmail(email: string): string {
  if (!email) {
    throw createError({
      statusCode: 400,
      message: 'Email is required'
    });
  }

  const normalized = normalizeEmail(email);
  
  if (!isValidEmail(normalized)) {
    throw createError({
      statusCode: 400,
      message: 'Invalid email format'
    });
  }

  return normalized;
}
