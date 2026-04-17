#ifndef BASE76_ALPHABET_H
#define BASE76_ALPHABET_H

/* Replace the string below with your canonical 76-character Base76 alphabet.
 * It must be exactly 76 characters long and contain no duplicates.
 *
 * Example placeholder (NOT a real alphabet for production):
 * "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
 *
 * Ensure the string length is exactly 76 characters.
 */

static const char BASE76_ALPHABET[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+[]"; /* placeholder; must be 76 chars */

#define BASE76_ALPHABET_LEN (sizeof(BASE76_ALPHABET) - 1)

#endif /* BASE76_ALPHABET_H */
