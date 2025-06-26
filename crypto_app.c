#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define DEVICE_PATH "/dev/usb_crypto_dev"
#define STATUS_PATH "/sys/class/usb_crypto_dev/usb_crypto_dev/status"
#define MAX_BUFFER_SIZE 4096
#define CHUNK_SIZE 1024

// Function to check if USB crypto device is connected
int check_usb_connection() {
    FILE *fp;
    char buffer[256];
    
    fp = fopen(STATUS_PATH, "r");
    if (!fp) {
        printf("Error: Cannot access USB crypto driver status.\n");
        return 0;
    }
    
    if (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "Connected")) {
            fclose(fp);
            return 1;
        }
    }
    
    fclose(fp);
    printf("Error: USB crypto device is not connected.\n");
    return 0;
}

// Function to display driver status
void show_status() {
    FILE *fp;
    char buffer[256];
    
    fp = fopen(STATUS_PATH, "r");
    if (!fp) {
        printf("Error: Cannot access USB crypto driver status.\n");
        return;
    }
    
    printf("=== USB Crypto Driver Status ===\n");
    while (fgets(buffer, sizeof(buffer), fp)) {
        printf("%s", buffer);
    }
    printf("================================\n");
    
    fclose(fp);
}

// Function to read file content
char* read_file(const char* filename, size_t* file_size) {
    FILE *fp;
    char *buffer;
    struct stat st;
    
    if (stat(filename, &st) != 0) {
        printf("Error: Cannot access file %s\n", filename);
        return NULL;
    }
    
    *file_size = st.st_size;
    if (*file_size == 0) {
        printf("Error: File %s is empty\n", filename);
        return NULL;
    }
    
    buffer = malloc(*file_size + 1);
    if (!buffer) {
        printf("Error: Memory allocation failed\n");
        return NULL;
    }
    
    fp = fopen(filename, "rb");
    if (!fp) {
        printf("Error: Cannot open file %s\n", filename);
        free(buffer);
        return NULL;
    }
    
    if (fread(buffer, 1, *file_size, fp) != *file_size) {
        printf("Error: Cannot read file %s\n", filename);
        fclose(fp);
        free(buffer);
        return NULL;
    }
    
    buffer[*file_size] = '\0';
    fclose(fp);
    return buffer;
}

// Function to write file content
int write_file(const char* filename, const char* data, size_t size) {
    FILE *fp;
    
    fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error: Cannot create file %s\n", filename);
        return 0;
    }
    
    if (fwrite(data, 1, size, fp) != size) {
        printf("Error: Cannot write to file %s\n", filename);
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    return 1;
}

// Function to send command and receive result
int send_crypto_command(char operation, const char* data, size_t data_size, char** result, size_t* result_size) {
    int fd;
    char *command;
    size_t command_size;
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;
    size_t total_read = 0;
    
    // Prepare command: operation + ':' + data
    command_size = 1 + 1 + data_size; // operation + ':' + data
    command = malloc(command_size);
    if (!command) {
        printf("Error: Memory allocation failed\n");
        return 0;
    }
    
    command[0] = operation;
    command[1] = ':';
    memcpy(command + 2, data, data_size);
    
    // Send command to driver
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        printf("Error: Cannot access USB crypto driver for writing (%s)\n", strerror(errno));
        free(command);
        return 0;
    }
    
    if (write(fd, command, command_size) != command_size) {
        printf("Error: Failed to send command to crypto driver\n");
        close(fd);
        free(command);
        return 0;
    }
    
    close(fd);
    free(command);

    // Read result from driver
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot access USB crypto driver for reading (%s)\n", strerror(errno));
        return 0;
    }
    
    *result = NULL;
    *result_size = 0;
    
    while ((bytes_read = read(fd, buffer, MAX_BUFFER_SIZE)) > 0) {
        *result = realloc(*result, *result_size + bytes_read);
        if (!*result) {
            printf("Error: Memory allocation failed\n");
            close(fd);
            return 0;
        }
        memcpy(*result + *result_size, buffer, bytes_read);
        *result_size += bytes_read;
        total_read += bytes_read;
    }
    
    close(fd);
    
    if (bytes_read < 0) {
        printf("Error: Failed to read result from crypto driver (%s)\n", strerror(errno));
        free(*result);
        return 0;
    }
    
    if (total_read == 0) {
        printf("Error: No data returned from crypto driver\n");
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
    
    printf("Encrypting file: %s -> %s\n", input_file, output_file);
    
    if (!check_usb_connection()) {
        return;
    }
    
    file_data = read_file(input_file, &file_size);
    if (!file_data) {
        return;
    }
    
    // Send encryption command and get result
    if (send_crypto_command('E', file_data, file_size, &encrypted_data, &encrypted_size)) {
        // Write encrypted data to output file
        if (write_file(output_file, encrypted_data, encrypted_size)) {
            printf("File encrypted successfully and saved to %s\n", output_file);
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
    
    printf("Decrypting file: %s -> %s\n", input_file, output_file);
    
    if (!check_usb_connection()) {
        return;
    }
    
    file_data = read_file(input_file, &file_size);
    if (!file_data) {
        return;
    }
    
    // Send decryption command and get result
    if (send_crypto_command('D', file_data, file_size, &decrypted_data, &decrypted_size)) {
        // Write decrypted data to output file
        if (write_file(output_file, decrypted_data, decrypted_size)) {
            printf("File decrypted successfully and saved to %s\n", output_file);
        }
        free(decrypted_data);
    }
    
    free(file_data);
}

// Function to display help
void show_help() {
    printf("USB Crypto Application\n");
    printf("======================\n");
    printf("Usage: %s [OPTION] [FILES]\n\n", "crypto_app");
    printf("Options:\n");
    printf("  -e <input> <output>    Encrypt input file to output file\n");
    printf("  -d <input> <output>    Decrypt input file to output file\n");
    printf("  -s                     Show USB crypto driver status\n");
    printf("  -h                     Show this help message\n\n");
    printf("Examples:\n");
    printf("  crypto_app -e document.txt document.enc    # Encrypt file\n");
    printf("  crypto_app -d document.enc document.txt    # Decrypt file\n");
    printf("  crypto_app -s                              # Show status\n\n");
    printf("Note: USB crypto device must be connected for encryption/decryption.\n");
}

int main(int argc, char *argv[]) {
    printf("=== USB Crypto Application ===\n\n");
    
    if (argc < 2) {
        show_help();
        return 1;
    }
    
    switch (argv[1][1]) {
        case 'e': // Encrypt
            if (argc != 4) {
                printf("Error: Encrypt option requires input and output files\n");
                printf("Usage: %s -e <input_file> <output_file>\n", argv[0]);
                return 1;
            }
            encrypt_file(argv[2], argv[3]);
            break;
            
        case 'd': // Decrypt
            if (argc != 4) {
                printf("Error: Decrypt option requires input and output files\n");
                printf("Usage: %s -d <input_file> <output_file>\n", argv[0]);
                return 1;
            }
            decrypt_file(argv[2], argv[3]);
            break;
            
        case 's': // Status
            show_status();
            break;
            
        case 'h': // Help
            show_help();
            break;
            
        default:
            printf("Error: Unknown option %s\n", argv[1]);
            show_help();
            return 1;
    }
    
    return 0;
}