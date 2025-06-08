#include "utils.h"

/**
 * hex_to_uint - Converts a hexadecimal string to an unsigned integer
 * 
 * @param str Pointer to a character array containing the hexadecimal string
 * @param len Number of characters to process from the string
 * @return The unsigned integer value of the hexadecimal string
 *
 * Example: "1A3" would be converted to decimal value 419
 */
unsigned int hex_to_uint(char *hex_str, unsigned int len) {
    unsigned int result = 0;
    while (len--) {
        result = result << 4;
        if (*hex_str >= '0' && *hex_str <= '9') {
            result += *hex_str - '0';
        }
        else if (*hex_str >= 'a' && *hex_str <= 'f') {
            result += *hex_str - 'a' + 10;
        }
        else if (*hex_str >= 'A' && *hex_str <= 'F') {
            result += *hex_str - 'A' + 10;
        }
        hex_str++;
    }
    return result;
}

/**
 * atoi - Converts a string to an integer
 * 
 * @param str: Pointer to a character array containing the string
 * @return The integer value of the string
 * 
 * Example: "123" would be converted to decimal value 123
 */
int atoi(char *str) {
    int result = 0;
    while (*str) {
        result = result * 10 + *str - '0';
        str++;
    }
    return result;
}

/**
 * itoa - Converts an integer to a string
 * 
 * @param num: The integer to convert
 * @return Pointer to a character array containing the string representation of the integer
 */
char* itoa(int num) {
    static char buffer[12]; // Enough to hold 32-bit integer
    int i = 0;
    int is_negative = 0;
    
    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0) {
            buffer[i++] = (num % 10) + '0';
            num /= 10;
        }
        
        // Add negative sign if necessary
        if (is_negative) {
            buffer[i++] = '-';
        }
    }
    buffer[i] = '\0';
    
    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
    
    return buffer;
}

/**
 * be2le_u32 - Converts a big-endian 32-bit unsigned integer to little-endian
 * 
 * @param be The big-endian 32-bit unsigned integer
 * @return The little-endian 32-bit unsigned integer
 */
uint32_t be2le_u32(uint32_t be) {
    return ((be & 0xFF) << 24) | ((be & 0xFF00) << 8) | ((be & 0xFF0000) >> 8) | ((be & 0xFF000000) >> 24);
}

unsigned int align(unsigned int n, unsigned int alignment) {
    return (n + alignment - 1) & ~(alignment - 1);
}