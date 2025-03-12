#include "utils.h"

unsigned int hexstr_to_uint(char *str, unsigned int len) {
    /**
     * hexstr_to_uint - Converts a hexadecimal string to an unsigned integer
     * @str: Pointer to a character array containing the hexadecimal string
     * @len: Number of characters to process from the string
     *
     * Example: "1A3" would be converted to decimal value 419
     *
     * Return: The unsigned integer value of the hexadecimal string
     */
    unsigned int result = 0;
    while (len--) {
        result = result << 4;
        if (*str >= '0' && *str <= '9') {
            result += *str - '0';
        }
        else if (*str >= 'a' && *str <= 'f') {
            result += *str - 'a' + 10;
        }
        else if (*str >= 'A' && *str <= 'F') {
            result += *str - 'A' + 10;
        }
        str++;
    }
    return result;
}

unsigned int align(unsigned int n, unsigned int alignment) {
    return (n + alignment - 1) & ~(alignment - 1);
}