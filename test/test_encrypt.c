#include <stdio.h>
#include <string.h>
#include "../include/encrypt.h"

int main(void) {
    const char *password = "test_password";
    const char *input = "test/plain.txt";
    const char *encrypted = "build/test.enc";
    const char *decrypted = "build/test.dec";

    if (encrypt_file(input, encrypted, password) != BACKUP_SUCCESS) {
        printf("encrypt failed\n");
        return 1;
    }

    if (decrypt_file(encrypted, decrypted, password) != BACKUP_SUCCESS) {
        printf("decrypt failed\n");
        return 1;
    }

    printf("encrypt & decrypt finished\n");
    return 0;
}
