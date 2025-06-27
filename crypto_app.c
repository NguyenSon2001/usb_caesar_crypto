#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define DEVICE_PATH "/dev/usb_crypto_dev"
#define MAX_BUFFER_SIZE 4096

// Function to read file content using open/read
char* read_file(const char* filename, size_t* file_size) {
    int fd;
    struct stat st;
    char *buffer;

    if (stat(filename, &st) != 0) {
        printf("Error: Cannot access file %s\n", filename);
        return NULL;
    }

    *file_size = st.st_size;
    if (*file_size == 0) {
        printf("Error: File %s is empty\n", filename);
        return NULL;
    }

    buffer = malloc(*file_size);
    if (!buffer) {
        printf("Error: Memory allocation failed\n");
        return NULL;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot open file %s (%s)\n", filename, strerror(errno));
        free(buffer);
        return NULL;
    }

    ssize_t total_read = 0;
    while (total_read < *file_size) {
        ssize_t bytes = read(fd, buffer + total_read, *file_size - total_read);
        if (bytes <= 0) {
            printf("Error: Failed to read file %s (%s)\n", filename, strerror(errno));
            close(fd);
            free(buffer);
            return NULL;
        }
        total_read += bytes;
    }

    close(fd);
    return buffer;
}

// Function to write file content using open/write
int write_file(const char* filename, const char* data, size_t size) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("Error: Cannot create file %s (%s)\n", filename, strerror(errno));
        return 0;
    }

    ssize_t total_written = 0;
    while (total_written < size) {
        ssize_t bytes = write(fd, data + total_written, size - total_written);
        if (bytes <= 0) {
            printf("Error: Failed to write to file %s (%s)\n", filename, strerror(errno));
            close(fd);
            return 0;
        }
        total_written += bytes;
    }

    close(fd);
    return 1;
}

// Function to send command and receive result
int send_crypto_command(char operation, const char* data, size_t data_size, char** result, size_t* result_size) {
    int fd;
    size_t command_size = 1 + 1 + data_size;
    char *command = malloc(command_size);
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;
    size_t total_read = 0;

    if (!command) {
        printf("Error: Memory allocation failed\n");
        return 0;
    }

    command[0] = operation;
    command[1] = ':';
    memcpy(command + 2, data, data_size);

    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        printf("Error: Cannot write to device (%s)\n", strerror(errno));
        free(command);
        return 0;
    }

    ssize_t total_written = 0;
    while (total_written < command_size) {
        ssize_t written = write(fd, command + total_written, command_size - total_written);
        if (written <= 0) {
            printf("Error: Failed to write to device (%s)\n", strerror(errno));
            close(fd);
            free(command);
            return 0;
        }
        total_written += written;
    }

    close(fd);
    free(command);

    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot read from device (%s)\n", strerror(errno));
        return 0;
    }

    *result = NULL;
    *result_size = 0;

    while ((bytes_read = read(fd, buffer, MAX_BUFFER_SIZE)) > 0) {
        char *temp = realloc(*result, *result_size + bytes_read);
        if (!temp) {
            printf("Error: Memory allocation failed\n");
            close(fd);
            free(*result);
            return 0;
        }
        *result = temp;
        memcpy(*result + *result_size, buffer, bytes_read);
        *result_size += bytes_read;
        total_read += bytes_read;
    }

    close(fd);

    if (bytes_read < 0) {
        printf("Error: Failed to read from device (%s)\n", strerror(errno));
        free(*result);
        return 0;
    }

    if (total_read == 0) {
        printf("Error: No data received from device\n");
        return 0;
    }

    return 1;
}

// Function to encrypt file
void encrypt_file(const char* input_file, const char* output_file) {
    char *file_data;
    size_t file_size;
    char *encrypted_data = NULL;
    size_t encrypted_size;

    printf("Encrypting: %s -> %s\n", input_file, output_file);

    file_data = read_file(input_file, &file_size);
    if (!file_data) return;

    if (send_crypto_command('E', file_data, file_size, &encrypted_data, &encrypted_size)) {
        if (write_file(output_file, encrypted_data, encrypted_size)) {
            printf("Encryption complete. Saved to %s\n", output_file);
        }
        free(encrypted_data);
    }

    free(file_data);
}

// Function to decrypt file
void decrypt_file(const char* input_file, const char* output_file) {
    char *file_data;
    size_t file_size;
    char *decrypted_data = NULL;
    size_t decrypted_size;

    printf("Decrypting: %s -> %s\n", input_file, output_file);

    file_data = read_file(input_file, &file_size);
    if (!file_data) return;

    if (send_crypto_command('D', file_data, file_size, &decrypted_data, &decrypted_size)) {
        if (write_file(output_file, decrypted_data, decrypted_size)) {
            printf("Decryption complete. Saved to %s\n", output_file);
        }
        free(decrypted_data);
    }

    free(file_data);
}

// Function to display help
void show_help(const char* prog_name) {
    printf("USB Crypto Application\n");
    printf("======================\n");
    printf("Usage: %s [OPTION] [FILES]\n\n", prog_name);
    printf("Options:\n");
    printf("  -e <input> <output>    Encrypt input file to output file\n");
    printf("  -d <input> <output>    Decrypt input file to output file\n");
    printf("  -h                     Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s -e secret.txt secret.enc\n", prog_name);
    printf("  %s -d secret.enc secret.txt\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-e") == 0) {
        if (argc != 4) {
            printf("Error: Encryption requires input and output files.\n");
            return 1;
        }
        encrypt_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            printf("Error: Decryption requires input and output files.\n");
            return 1;
        }
        decrypt_file(argv[2], argv[3]);
    } else if (strcmp(argv[1], "-h") == 0) {
        show_help(argv[0]);
    } else {
        printf("Error: Unknown option '%s'\n", argv[1]);
        show_help(argv[0]);
        return 1;
    }

    return 0;
}
