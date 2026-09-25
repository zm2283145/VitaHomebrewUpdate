#ifndef VHBU_VPK_EXTRACT_H
#define VHBU_VPK_EXTRACT_H

/* Extract a deliberately constrained VPK/ZIP into destination.
 * Supports stored and raw-deflate entries with local-header sizes. */
int vhbu_extract_vpk(const char *source, const char *destination);

#endif
