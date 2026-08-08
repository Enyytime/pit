#ifndef PIT_REFS_H
#define PIT_REFS_H

/** Length of a hex SHA-1 string, excluding the terminator. */
#define HASH_HEX_LEN 40

/**
 * @brief Result of an attempted reference update.
 */
typedef enum {
    REF_OK = 0,        /**< Reference now holds the new hash. */
    REF_STALE = -1,    /**< Reference did not hold the expected old value. */
    REF_LOCKED = -2,   /**< Another process holds the lock. */
    REF_IO_ERROR = -3  /**< Filesystem operation failed; see errno. */
} RefResult;

char* read_ref(const char* branch);
RefResult update_ref(const char* branch, const char* new_hash,
                     const char* expected_old_hash);
char* current_branch(void);

#endif